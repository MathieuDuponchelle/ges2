/* GStreamer Editing Services
 * Copyright (C) 2015 Mathieu Duponchelle <mathieu.duponchelle@opencreed.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <glib/gprintf.h>
#include <stdlib.h>
#include <string.h>
#ifdef G_OS_UNIX
#include <glib-unix.h>
#endif
#include "ges-launcher.h"
#include "utils.h"
#include "ges-structure-parser.h"
#include "parse_lex.h"

#define GES_LAUNCHER_PRIV(self) (ges_launcher_get_instance_private (GES_LAUNCHER (self)))

typedef struct
{
  gboolean mute;
  gboolean disable_mixing;
  gchar *save_path;
  gchar *save_only_path;
  gchar *load_path;
  GESMediaType track_types;
  gboolean needs_set_state;
  gboolean smartrender;
  gchar *scenario;
  gchar *format;
  gchar *outputuri;
  gchar *encoding_profile;
  gchar *videosink;
  gchar *audiosink;
  gboolean list_transitions;
  gboolean inspect_action_type;
  gchar *sanitized_timeline;
} ParsedOptions;

typedef struct _GESLauncherPrivate
{
  GESTimeline *timeline;
  gboolean seenerrors;
#ifdef G_OS_UNIX
  guint signal_watch_id;
#endif
  ParsedOptions parsed_options;
  GHashTable *function_map;
} GESLauncherPrivate;

struct _GESLauncher
{
  GApplication parent;
};

G_DEFINE_TYPE_WITH_PRIVATE (GESLauncher, ges_launcher, G_TYPE_APPLICATION);

static const gchar *HELP_SUMMARY =
    "ges-launch-2.0 creates a multimedia timeline and plays it back,\n"
    "  or renders it to the specified format.\n\n"
    " It can load a timeline from an existing project, or create one\n"
    " from the specified commands.\n\n"
    " Updating an existing project can be done through --set-scenario\n"
    " if ges-launch-1.0 has been compiled with gst-validate, see\n"
    " ges-launch-1.0 --inspect-action-type for the available commands.\n\n"
    " You can learn more about individual ges-launch-1.0 commands with\n"
    " \"ges-launch-1.0 help command\".\n\n"
    " By default, ges-launch-1.0 is in \"playback-mode\".";

static gboolean
_parse_track_type (const gchar * option_name, const gchar * value,
    GESLauncher * self, GError ** error)
{
  GESLauncherPrivate *priv = GES_LAUNCHER_PRIV (self);
  ParsedOptions *opts = &priv->parsed_options;

  opts->track_types = get_flags_from_string (GES_TYPE_MEDIA_TYPE, value);

  if (opts->track_types == 0)
    return FALSE;

  return TRUE;
}

/*
static gboolean
_timeline_set_user_options (GESLauncher * self, GESTimeline * timeline,
    const gchar * load_path)
{
  GList *tmp;
  GESTrack *tracka, *trackv;
  gboolean has_audio = FALSE, has_video = FALSE;
  ParsedOptions *opts = &priv->parsed_options;

retry:
  for (tmp = timeline->tracks; tmp; tmp = tmp->next) {

    if (GES_TRACK (tmp->data)->type == GES_TRACK_TYPE_VIDEO)
      has_video = TRUE;
    else if (GES_TRACK (tmp->data)->type == GES_TRACK_TYPE_AUDIO)
      has_audio = TRUE;

    if (opts->disable_mixing)
      ges_track_set_mixing (tmp->data, FALSE);

    if (!(GES_TRACK (tmp->data)->type & opts->track_types)) {
      ges_timeline_remove_track (timeline, tmp->data);
      goto retry;
    }
  }

  if (opts->scenario && !load_path) {
    if (!has_video && opts->track_types & GES_TRACK_TYPE_VIDEO) {
      trackv = GES_TRACK (ges_video_track_new ());

      if (opts->disable_mixing)
        ges_track_set_mixing (trackv, FALSE);

      if (!(ges_timeline_add_track (timeline, trackv)))
        return FALSE;
    }

    if (!has_audio && opts->track_types & GES_TRACK_TYPE_AUDIO) {
      tracka = GES_TRACK (ges_audio_track_new ());
      if (opts->disable_mixing)
        ges_track_set_mixing (tracka, FALSE);

      if (!(ges_timeline_add_track (timeline, tracka)))
        return FALSE;
    }
  }

  return TRUE;
}
*/

static GESStructureParser *
_parse_structures (const gchar * string)
{
  yyscan_t scanner;
  GESStructureParser *parser = ges_structure_parser_new ();

  priv_ges_parse_yylex_init_extra (parser, &scanner);
  priv_ges_parse_yy_scan_string (string, scanner);
  priv_ges_parse_yylex (scanner);
  priv_ges_parse_yylex_destroy (scanner);

  ges_structure_parser_end_of_file (parser);
  return parser;
}

typedef gboolean (*StructuredFunction)(GESLauncher *self, const GstStructure *structure);

static void
_add_source_for_media_type (GESLauncher *self, const GstStructure *structure, GESMediaType media_type)
{
  GESLauncherPrivate *priv = GES_LAUNCHER_PRIV (self);
  const gchar *uri = gst_structure_get_string (structure, "uri");
  GESSource *source = ges_uri_source_new (uri, media_type);
  gdouble time;

  if (!source)
    return;

  if (gst_structure_get_double (structure, "inpoint", &time)) {
    GST_ERROR ("setting inpoint dude");
    ges_object_set_inpoint (GES_OBJECT (source), time * GST_SECOND);
  }

  if (gst_structure_get_double (structure, "start", &time)) {
    ges_object_set_start (GES_OBJECT (source), time * GST_SECOND);
  }

  if (gst_structure_get_double (structure, "duration", &time)) {
    ges_object_set_duration (GES_OBJECT (source), time * GST_SECOND);
  }

  ges_timeline_add_object (priv->timeline, GES_OBJECT (source));
}

static gboolean
_add_source (GESLauncher *self, const GstStructure *structure)
{
  _add_source_for_media_type (self, structure, GES_MEDIA_TYPE_VIDEO);
  _add_source_for_media_type (self, structure, GES_MEDIA_TYPE_AUDIO);

  return TRUE;
}

static gboolean
_create_timeline (GESLauncher * self, const gchar * serialized_timeline,
    const gchar * proj_uri, const gchar * scenario)
{
  GESLauncherPrivate *priv = GES_LAUNCHER_PRIV (self);
  GESStructureParser *parser = _parse_structures (serialized_timeline);
  GList *tmp;

  priv->timeline = ges_timeline_new (GES_MEDIA_TYPE_VIDEO | GES_MEDIA_TYPE_AUDIO);
  for (tmp = parser->structures; tmp; tmp = tmp->next) {
    GstStructure *structure = GST_STRUCTURE (tmp->data);
    StructuredFunction func = g_hash_table_lookup (priv->function_map, gst_structure_get_name (structure));
    GST_ERROR ("one structure : %s", gst_structure_to_string (structure));
    func (self, structure);
  }

  return TRUE;
}

static gboolean
_set_playback_details (GESLauncher * self)
{
  return TRUE;
}

#ifdef G_OS_UNIX
static gboolean
intr_handler (GESLauncher * self)
{
  g_print ("interrupt received.\n");

  g_application_release (G_APPLICATION (self));

  /* remove signal handler */
  return TRUE;
}
#endif /* G_OS_UNIX */

static gboolean
_release (GESLauncher *launcher)
{
  g_application_release (G_APPLICATION (launcher));
  return G_SOURCE_REMOVE;
}

static void
_on_error_cb (GstPlayer *player, GError *error, GESLauncher *launcher)
{
  gst_debug_bin_to_dot_file_with_ts (GST_BIN (gst_player_get_pipeline (player)), GST_DEBUG_GRAPH_SHOW_ALL, "plopthat");
  g_idle_add ((GSourceFunc) _release, launcher);
}

static void
_on_eos_cb (GstPlayer *player, GESLauncher *launcher)
{
  g_idle_add ((GSourceFunc) _release, launcher);
}

static void
_position_cb (GstPlayer *player, GstClockTime position, GESLauncher *self)
{
  g_print ("\rposition is now %" GST_TIME_FORMAT, GST_TIME_ARGS (position));
}

static gboolean
_run_pipeline (GESLauncher * self)
{
  GESLauncherPrivate *priv = GES_LAUNCHER_PRIV (self);
  GstPlayer *player;

  player = ges_playable_make_player (GES_PLAYABLE (priv->timeline));
  g_signal_connect (player, "error", G_CALLBACK (_on_error_cb), self);
  g_signal_connect (player, "end-of-stream", G_CALLBACK (_on_eos_cb), self);
  g_signal_connect (player, "position-updated", G_CALLBACK (_position_cb), self);
  gst_player_play (player);

  g_application_hold (G_APPLICATION (self));

  return TRUE;
}

static gboolean
_create_pipeline (GESLauncher * self, const gchar * serialized_timeline)
{
  gchar *uri = NULL;
  gboolean res = TRUE;
  GESLauncherPrivate *priv = GES_LAUNCHER_PRIV (self);
  ParsedOptions *opts = &priv->parsed_options;

  /* Timeline creation */
  if (opts->load_path) {
    g_printf ("Loading project from : %s\n", opts->load_path);

    if (!(uri = ensure_uri (opts->load_path))) {
      g_error ("couldn't create uri for '%s'", opts->load_path);
      goto failure;
    }
  }

  if (!_create_timeline (self, serialized_timeline, uri, opts->scenario)) {
    GST_ERROR ("Could not create the timeline");
    goto failure;
  }

  if (!opts->load_path)
    ges_timeline_commit (priv->timeline);

failure:
  return res;
}

static GOptionGroup *
ges_launcher_get_project_option_group (GESLauncher * self)
{
  GOptionGroup *group;
  GESLauncherPrivate *priv = GES_LAUNCHER_PRIV (self);
  ParsedOptions *opts = &priv->parsed_options;

  GOptionEntry options[] = {
    {"load", 'l', 0, G_OPTION_ARG_STRING, &opts->load_path,
          "Load project from file. The project can be saved "
          "again with the --save option.",
        "<path>"},
    {"save", 's', 0, G_OPTION_ARG_STRING, &opts->save_path,
          "Save project to file before rendering. "
          "It can then be loaded with the --load option",
        "<path>"},
    {"save-only", 0, 0, G_OPTION_ARG_STRING, &opts->save_only_path,
          "Same as save project, except exit as soon as the timeline "
          "is saved instead of playing it back",
        "<path>"},
    {NULL}
  };
  group = g_option_group_new ("project", "Project Options",
      "Show project-related options", NULL, NULL);

  g_option_group_add_entries (group, options);

  return group;
}

static GOptionGroup *
ges_launcher_get_info_option_group (GESLauncher * self)
{
  GOptionGroup *group;
  GESLauncherPrivate *priv = GES_LAUNCHER_PRIV (self);
  ParsedOptions *opts = &priv->parsed_options;

  GOptionEntry options[] = {
#ifdef HAVE_GST_VALIDATE
    {"inspect-action-type", 0, 0, G_OPTION_ARG_NONE, &opts->inspect_action_type,
          "Inspect the available action types that can be defined in a scenario "
          "set with --set-scenario. "
          "Will list all action-types if action-type is empty.",
        "<[action-type]>"},
#endif
    {"list-transitions", 0, 0, G_OPTION_ARG_NONE, &opts->list_transitions,
          "List all valid transition types and exit. "
          "See ges-launch-1.0 help transition for more information.",
        NULL},
    {NULL}
  };

  group = g_option_group_new ("informative", "Informative Options",
      "Show informative options", NULL, NULL);

  g_option_group_add_entries (group, options);

  return group;
}

static GOptionGroup *
ges_launcher_get_rendering_option_group (GESLauncher * self)
{
  GOptionGroup *group;
  GESLauncherPrivate *priv = GES_LAUNCHER_PRIV (self);
  ParsedOptions *opts = &priv->parsed_options;

  GOptionEntry options[] = {
    {"outputuri", 'o', 0, G_OPTION_ARG_STRING, &opts->outputuri,
          "If set, ges-launch-1.0 will render the timeline instead of playing "
          "it back. The default rendering format is ogv, containing theora and vorbis.",
        "<URI>"},
    {"format", 'f', 0, G_OPTION_ARG_STRING, &opts->format,
          "Set an encoding profile on the command line. "
          "See ges-launch-1.0 help profile for more information. "
          "This will have no effect if no outputuri has been specified.",
        "<profile>"},
    {"encoding-profile", 'e', 0, G_OPTION_ARG_STRING, &opts->encoding_profile,
          "Set an encoding profile from a preset file. "
          "See ges-launch-1.0 help profile for more information. "
          "This will have no effect if no outputuri has been specified.",
        "<profile-name>"},
    {NULL}
  };

  group = g_option_group_new ("rendering", "Rendering Options",
      "Show rendering options", NULL, NULL);

  g_option_group_add_entries (group, options);

  return group;
}

static GOptionGroup *
ges_launcher_get_playback_option_group (GESLauncher * self)
{
  GOptionGroup *group;
  GESLauncherPrivate *priv = GES_LAUNCHER_PRIV (self);
  ParsedOptions *opts = &priv->parsed_options;

  GOptionEntry options[] = {
    {"videosink", 'v', 0, G_OPTION_ARG_STRING, &opts->videosink,
        "Set the videosink used for playback.", "<videosink>"},
    {"audiosink", 'a', 0, G_OPTION_ARG_STRING, &opts->audiosink,
        "Set the audiosink used for playback.", "<audiosink>"},
    {"mute", 'm', 0, G_OPTION_ARG_NONE, &opts->mute,
        "Mute playback output. This has no effect when rendering.", NULL},
    {NULL}
  };

  group = g_option_group_new ("playback", "Playback Options",
      "Show playback options", NULL, NULL);

  g_option_group_add_entries (group, options);

  return group;
}

static gboolean
_local_command_line (GApplication * application, gchar ** arguments[],
    gint * exit_status)
{
  GESLauncher *self = GES_LAUNCHER (application);
  GESLauncherPrivate *priv = GES_LAUNCHER_PRIV (self);
  GError *error = NULL;
  gchar **argv;
  gint argc;
  GOptionContext *ctx;
  ParsedOptions *opts = &priv->parsed_options;
  GOptionGroup *main_group;
  GOptionEntry options[] = {
    {"disable-mixing", 0, 0, G_OPTION_ARG_NONE, &opts->disable_mixing,
        "Do not use mixing elements to mix layers together.", NULL},
    {"track-types", 't', 0, G_OPTION_ARG_CALLBACK, &_parse_track_type,
          "Specify the track types to be created. "
          "When loading a project, only relevant tracks will be added to the timeline.",
        "<track-types>"},
#ifdef HAVE_GST_VALIDATE
    {"set-scenario", 0, 0, G_OPTION_ARG_STRING, &opts->scenario,
          "ges-launch-1.0 exposes gst-validate functionalities, such as scenarios."
          " Scenarios describe actions to execute, such as seeks or setting of properties. "
          "GES implements editing-specific actions such as adding or removing sources. "
          "See gst-validate-1.0 --help for more info about validate and scenarios, "
          "and --inspect-action-type.",
        "<scenario_name>"},
#endif
    {NULL}
  };

  ctx = g_option_context_new ("- plays or renders a timeline.");
  g_option_context_set_summary (ctx, HELP_SUMMARY);

  main_group =
      g_option_group_new ("launcher", "launcher options",
      "Main launcher options", self, NULL);
  g_option_group_add_entries (main_group, options);
  g_option_context_set_main_group (ctx, main_group);
  g_option_context_add_group (ctx, gst_init_get_option_group ());
  g_option_context_add_group (ctx,
      ges_launcher_get_project_option_group (self));
  g_option_context_add_group (ctx,
      ges_launcher_get_rendering_option_group (self));
  g_option_context_add_group (ctx,
      ges_launcher_get_playback_option_group (self));
  g_option_context_add_group (ctx, ges_launcher_get_info_option_group (self));
  g_option_context_set_ignore_unknown_options (ctx, TRUE);

  argv = *arguments;
  argc = g_strv_length (argv);
  *exit_status = 0;

  if (!g_option_context_parse (ctx, &argc, &argv, &error)) {
    g_printerr ("Error initializing: %s\n", error->message);
    g_option_context_free (ctx);
    *exit_status = 1;
  }

  if (opts->inspect_action_type) {
    return TRUE;
  }

  if (!opts->load_path && !opts->scenario && !opts->list_transitions
      && (argc <= 1)) {
    g_printf ("%s", g_option_context_get_help (ctx, TRUE, NULL));
    g_option_context_free (ctx);
    *exit_status = 1;
    return TRUE;
  }

  g_option_context_free (ctx);

  opts->sanitized_timeline = sanitize_timeline_description (argc, argv);

  if (!g_application_register (application, NULL, &error)) {
    *exit_status = 1;
    g_error_free (error);
    return FALSE;
  }

  return TRUE;
}

static void
_startup (GApplication * application)
{
  GESLauncher *self = GES_LAUNCHER (application);
  GESLauncherPrivate *priv = GES_LAUNCHER_PRIV (self);
  ParsedOptions *opts = &priv->parsed_options;

#ifdef G_OS_UNIX
  priv->signal_watch_id =
      g_unix_signal_add (SIGINT, (GSourceFunc) intr_handler, self);
#endif

  /* Initialize the GStreamer Editing Services */
  if (!ges_init ()) {
    g_printerr ("Error initializing GES\n");
    goto done;
  }

  if (opts->list_transitions) {
    goto done;
  }

  if (!_create_pipeline (self, opts->sanitized_timeline))
    goto failure;

  if (opts->save_only_path)
    goto done;

  if (!_set_playback_details (self))
    goto failure;

  if (!_run_pipeline (self))
    goto failure;

done:
  G_APPLICATION_CLASS (ges_launcher_parent_class)->startup (application);

  return;

failure:
  priv->seenerrors = TRUE;

  goto done;
}

static void
_shutdown (GApplication * application)
{
  gint validate_res = 0;
  GESLauncher *self = GES_LAUNCHER (application);
  GESLauncherPrivate *priv = GES_LAUNCHER_PRIV (self);

  if (priv->seenerrors == FALSE)
    priv->seenerrors = validate_res;

#ifdef G_OS_UNIX
  g_source_remove (priv->signal_watch_id);
#endif

  G_APPLICATION_CLASS (ges_launcher_parent_class)->shutdown (application);
}

static void
ges_launcher_class_init (GESLauncherClass * klass)
{
  G_APPLICATION_CLASS (klass)->local_command_line = _local_command_line;
  G_APPLICATION_CLASS (klass)->startup = _startup;
  G_APPLICATION_CLASS (klass)->shutdown = _shutdown;
}

static void
ges_launcher_init (GESLauncher * self)
{
  GESLauncherPrivate *priv = GES_LAUNCHER_PRIV (self);

  priv->function_map = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  g_hash_table_insert (priv->function_map, g_strdup ("source"), _add_source);
}

gint
ges_launcher_get_exit_status (GESLauncher * self)
{
  GESLauncherPrivate *priv = GES_LAUNCHER_PRIV (self);
  return priv->seenerrors;
}

GESLauncher *
ges_launcher_new (void)
{
  return GES_LAUNCHER (g_object_new (ges_launcher_get_type (), "application-id",
          "org.gstreamer.geslaunch2", "flags",
          G_APPLICATION_NON_UNIQUE | G_APPLICATION_HANDLES_COMMAND_LINE, NULL));
}
