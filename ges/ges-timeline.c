#include "ges-timeline.h"
#include "ges-composition-bin.h"
#include "ges-playable.h"
#include "nle.h"
#include "ges-transition.h"
#include "gst/controller/controller.h"
#include "ges-clip.h"

/* Structure definitions */

typedef struct _GESTimelinePrivate
{
  GList *compositions;
  GList *nleobjects;
  GSequence *object_by_start;
  GESCompositionBin *composition_bin;
  /* Tracks sorted by media-type */
  GHashTable *tracks;
} GESTimelinePrivate;

struct _GESTimeline
{
  GESObject parent;
  GESTimelinePrivate *priv;
};

/* Used to create and update transitions */
typedef struct
{
  GSequence *objects_by_start;
  GESObject *prev;
  GESTimeline *timeline;
} GESTrack;

enum
{
  PROP_0,
  PROP_MEDIA_TYPE,
};

enum
{
  TRANSITION_ADDED,
  TRANSITION_REMOVED,
  LAST_SIGNAL
};

static void ges_playable_interface_init (GESPlayableInterface * iface);
static guint ges_timeline_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE (GESTimeline, ges_timeline, GES_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (GES_TYPE_PLAYABLE, ges_playable_interface_init))

/* Implementation */

static GList *
_get_compositions (GESTimeline *self, GESMediaType media_type)
{
  GList *res = NULL, *tmp;

  if (media_type == GES_MEDIA_TYPE_UNKNOWN)
    return g_list_copy (self->priv->compositions);

  for (tmp = self->priv->compositions; tmp; tmp = tmp->next) {
    GstCaps *caps;
    GstStructure *structure;

    g_object_get (tmp->data, "caps", &caps, NULL);
    structure = gst_caps_get_structure (caps, 0);

    if (media_type == GES_MEDIA_TYPE_AUDIO && gst_structure_has_name (structure, GES_RAW_AUDIO_CAPS))
      res = g_list_append (res, tmp->data);
    else if (media_type == GES_MEDIA_TYPE_VIDEO && gst_structure_has_name (structure, GES_RAW_VIDEO_CAPS))
      res = g_list_append (res, tmp->data);

    gst_caps_unref (caps);
  }

  return res;
}

static GstElement *
_get_first_composition (GESTimeline *self, GESMediaType media_type)
{
  GstElement *res;
  GList *compositions = _get_compositions (self, media_type);

  if (!compositions)
    return NULL;

  res = GST_ELEMENT (compositions->data);
  g_list_free (compositions);

  return res;
}

static void
_add_expandable_operation (GstElement *composition, const gchar *element_name, guint priority, const gchar *name)
{
  GstElement *expandable = gst_element_factory_make ("nleoperation", name);
  GstElement *element = gst_element_factory_make (element_name, NULL);

  gst_bin_add (GST_BIN (expandable), element);

  g_object_set (expandable, "expandable", TRUE, "priority", priority, NULL);
  gst_bin_add (GST_BIN (composition), expandable);
}

static void
_add_expandable_source (GstElement *composition, GstElement *element, guint priority, const gchar *name)
{
  GstElement *expandable = gst_element_factory_make ("nlesource", name);

  gst_bin_add (GST_BIN (expandable), element);

  g_object_set (expandable, "expandable", TRUE, "priority", priority, NULL);
  gst_bin_add (GST_BIN (composition), expandable);
}

static GstElement *
_create_composition (GESTimeline *self, const gchar *caps_string, const gchar *name)
{
  GstElement *composition = gst_element_factory_make ("nlecomposition", name);
  GstElement *wrapper = gst_element_factory_make ("nlesource", NULL);
  GstCaps *caps = gst_caps_from_string (caps_string);

  GST_DEBUG_OBJECT (self, "creating composition %s with caps %s", name, caps_string);
  g_object_set (composition, "caps", caps, NULL);
  g_object_set (wrapper, "caps", caps, NULL);
  gst_caps_unref (caps);

  gst_bin_add (GST_BIN (wrapper), composition);

  self->priv->compositions = g_list_append (self->priv->compositions, composition);
  self->priv->nleobjects = g_list_append (self->priv->nleobjects, wrapper);

  return composition;
}

static void
_add_track (GESTimeline *self, GESMediaType media_type)
{
  GESTrack *track = g_malloc0 (sizeof (GESTrack));
  GList *tracks = g_hash_table_lookup (self->priv->tracks, GINT_TO_POINTER (media_type));

  track->objects_by_start = g_sequence_new (NULL);
  track->timeline = self;
  tracks = g_list_append (tracks, track);
  g_hash_table_replace (self->priv->tracks, GINT_TO_POINTER (media_type), tracks);
}

static void
_free_track (GESTrack *track)
{
  g_sequence_free (track->objects_by_start);
  g_free (track);
}

static void
_free_tracks (GESMediaType *unused, GList *tracks, gpointer unused_data)
{
  g_list_free_full (tracks, (GDestroyNotify) _free_track);
}

static void
_make_nle_objects (GESTimeline *self, GESMediaType media_type)
{
  GstElement *composition;

  if (media_type & GES_MEDIA_TYPE_AUDIO) {
    GstElement *background = gst_parse_bin_from_description (
        "audiotestsrc ! samplecontroller name=background_audio_controller", TRUE, NULL);
    GstElement *samplecontroller = gst_bin_get_by_name (GST_BIN (background), "background_audio_controller");
    g_object_set (samplecontroller, "volume", 0.0, NULL);
    gst_object_unref (samplecontroller);
    composition = _create_composition (self, GES_RAW_AUDIO_CAPS, "audio-composition");
    _add_expandable_operation (composition, "smartaudiomixer", 0, "timeline-audiomixer");
    _add_expandable_source (composition, background, 1, "timeline-audio-background");
    _add_track (self, GES_MEDIA_TYPE_AUDIO);
  }

  if (media_type & GES_MEDIA_TYPE_VIDEO) {
    GstElement *background = gst_parse_bin_from_description (
        "videotestsrc ! framepositioner name=background_video_positioner zorder=1000", TRUE, NULL);
    GstElement *pos = gst_bin_get_by_name (GST_BIN (background), "background_video_positioner");
    g_object_set (pos, "alpha", 0.0, NULL);
    gst_object_unref (pos);
    composition = _create_composition (self, GES_RAW_VIDEO_CAPS, "video-composition");
    _add_expandable_operation (composition, "smartvideomixer", 0, "timeline-videomixer");
    _add_expandable_source (composition, background, 1, "timeline-video-background");
    _add_track (self, GES_MEDIA_TYPE_VIDEO);
  }
}

 static gint
_compare_starts (GESObject *object1, GESObject *object2, gpointer unused)
{
  GstClockTime start1, start2;

  start1 = ges_object_get_start (object1);
  start2 = ges_object_get_start (object2);

  if (start1 < start2)
    return -42;
  else if (start1 == start2)
    return 0;
  else
    return 42;
}

static void
_create_or_update_transition (GESTimeline *self, GESObject *prev, GESObject *next)
{
  GESTransition *transition = ges_clip_get_transition (GES_CLIP (prev));

  if (!transition) {
    transition = ges_transition_new (prev, next);

    ges_clip_set_transition (GES_CLIP (prev), transition);
    g_signal_emit (self, ges_timeline_signals[TRANSITION_ADDED], 0,
        transition);
  } else {
    GESObject *faded_in_clip;

    g_object_get (transition, "faded-in-clip", &faded_in_clip, NULL);

    if (faded_in_clip != next)  {
      g_object_ref (transition);
      ges_clip_set_transition (GES_CLIP (prev), NULL);
      g_signal_emit (self, ges_timeline_signals[TRANSITION_REMOVED], 0,
          transition);
      g_object_unref (transition);
      transition = ges_transition_new (prev, next);

      ges_clip_set_transition (GES_CLIP (prev), transition);
      g_signal_emit (self, ges_timeline_signals[TRANSITION_ADDED], 0,
          transition);
    } else  {
      ges_transition_update (transition);
    }
    g_object_unref (faded_in_clip);
  }
}

static void
_check_transition (GESObject *object, GESTrack *track)
{
  if (!track->prev) {
    track->prev = object;
    return;
  }

  /* We don't overlap */
  if (ges_object_get_start (track->prev) + ges_object_get_duration (track->prev) <= ges_object_get_start (object)) {
    GESTransition *transition = ges_clip_get_transition (GES_CLIP (track->prev));

    if (transition) {
      g_object_ref (transition);
      ges_clip_set_transition (GES_CLIP (track->prev), NULL);
      g_signal_emit (track->timeline, ges_timeline_signals[TRANSITION_REMOVED], 0,
          transition);
      g_object_unref (transition);
    }
    track->prev = object;
    return;
  }

  _create_or_update_transition (track->timeline, track->prev, object);
  track->prev = object;
}

static void
_update_transitions_for_track (GESTrack *track)
{
  g_sequence_sort (track->objects_by_start, (GCompareDataFunc) _compare_starts, NULL);
  track->prev = NULL;
  g_sequence_foreach (track->objects_by_start, (GFunc) _check_transition, track);
}

static void
_update_transitions_for_media_type (GESTimeline *self, GESMediaType media_type)
{
  GList *tracks = g_hash_table_lookup (self->priv->tracks, GINT_TO_POINTER (media_type));

  if (!tracks)
    return;

  for (; tracks; tracks = tracks->next) {
    GESTrack *track = (GESTrack *) tracks->data;

    _update_transitions_for_track (track);
  }
}

static void
_update_transitions (GESTimeline *self)
{
  _update_transitions_for_media_type (self, GES_MEDIA_TYPE_VIDEO);
  _update_transitions_for_media_type (self, GES_MEDIA_TYPE_AUDIO);
}

static void
_add_object_to_track (GESTimeline *self, GESObject *object, GESMediaType media_type)
{
  GList *tracks = g_hash_table_lookup (self->priv->tracks, GINT_TO_POINTER (media_type));
  GESTrack *track;

  g_assert (tracks != NULL);

  track = (GESTrack *) g_list_nth_data (tracks, ges_object_get_track_index (object, media_type));

  g_assert (track != NULL);

  g_sequence_insert_sorted (track->objects_by_start, object, (GCompareDataFunc) _compare_starts, NULL);
}

/* GESObject implementation */

static gboolean
_set_inpoint (GESObject *object, GstClockTime inpoint)
{
  GESTimeline *timeline = GES_TIMELINE (object);
  GList *tmp;

  for (tmp = timeline->priv->nleobjects; tmp; tmp = tmp->next) {
    g_object_set (tmp->data, "inpoint", inpoint, NULL);
  }

  return TRUE;
}

static gboolean
_set_duration (GESObject *object, GstClockTime duration)
{
  GESTimeline *timeline = GES_TIMELINE (object);
  GList *tmp;

  for (tmp = timeline->priv->nleobjects; tmp; tmp = tmp->next) {
    g_object_set (tmp->data, "duration", duration, NULL);
  }

  return TRUE;
}

static gboolean
_set_start (GESObject *object, GstClockTime start)
{
  GESTimeline *timeline = GES_TIMELINE (object);
  GList *tmp;

  for (tmp = timeline->priv->nleobjects; tmp; tmp = tmp->next) {
    g_object_set (tmp->data, "start", start, NULL);
  }

  return TRUE;
}

static GList *
_get_nle_objects (GESObject *object)
{
  return g_list_copy (GES_TIMELINE (object)->priv->nleobjects);
}

static gboolean
_set_track_index (GESObject *object, GESMediaType media_type, guint index)
{
  GESTimeline *self = GES_TIMELINE (object);
  GList *tmp;

  for (tmp = self->priv->nleobjects; tmp; tmp = tmp->next)
  {
    GstCaps *caps;
    GstStructure *structure;
    GESMediaType object_type;
    guint new_priority;

    g_object_get (tmp->data, "caps", &caps, NULL);
    structure = gst_caps_get_structure (caps, 0);
    gst_caps_unref (caps);

    if (gst_structure_has_name (structure, GES_RAW_AUDIO_CAPS))
      object_type = GES_MEDIA_TYPE_AUDIO;
    else if (gst_structure_has_name (structure, GES_RAW_VIDEO_CAPS))
      object_type = GES_MEDIA_TYPE_VIDEO;
    else
      continue;

    if (object_type & media_type) {
      new_priority = (TRACK_PRIORITY_HEIGHT * index) + TIMELINE_PRIORITY_OFFSET;
      g_object_set (tmp->data, "priority", new_priority, NULL);
    }
  }

  return TRUE;
}

static gboolean
_set_media_type (GESObject *object, GESMediaType media_type)
{
  GESTimeline *self = GES_TIMELINE (object);

  if (media_type == GES_MEDIA_TYPE_UNKNOWN)
    return TRUE;

  if (self->priv->nleobjects) {
    GST_ERROR_OBJECT (self, "changing media type is not supported yet");
    return FALSE;
  }

  _make_nle_objects (self, media_type);

  return TRUE;
}

/* GESPlayable implementation */

static GstBin *
_make_playable (GESPlayable *playable, gboolean is_playable)
{
  GESTimeline *self = GES_TIMELINE (playable);

  if (is_playable) {
    ges_composition_bin_set_nle_objects (self->priv->composition_bin, self->priv->nleobjects);
  } else {
    ges_composition_bin_set_nle_objects (self->priv->composition_bin, NULL);
  }

  return GST_BIN (self->priv->composition_bin);
}

static void
ges_playable_interface_init (GESPlayableInterface * iface)
{
  iface->make_playable = _make_playable;
}

/* API */

gboolean
ges_timeline_add_object (GESTimeline *self, GESObject *object)
{
  GList *tmp;
  GList *nleobjects = ges_object_get_nle_objects (object);

  for (tmp = nleobjects; tmp; tmp = tmp->next) {
    GstCaps *caps;
    GstStructure *structure;
    GESMediaType media_type;
    GstElement *composition;
    GstObject *parent = gst_object_get_parent (tmp->data);

    g_object_get (tmp->data, "caps", &caps, NULL);
    structure = gst_caps_get_structure (caps, 0);
    gst_caps_unref (caps);

    if (gst_structure_has_name (structure, GES_RAW_AUDIO_CAPS))
      media_type = GES_MEDIA_TYPE_AUDIO;
    else if (gst_structure_has_name (structure, GES_RAW_VIDEO_CAPS))
      media_type = GES_MEDIA_TYPE_VIDEO;
    else
      continue;

    composition = _get_first_composition (self, media_type);
    if (!composition)
      continue;

    g_object_set (tmp->data, "priority", 2, NULL);
    if (parent) {
      gst_object_ref (tmp->data);
      gst_bin_remove (GST_BIN (parent), GST_ELEMENT (tmp->data));
      gst_object_unref (parent);
    }
    GST_DEBUG_OBJECT (tmp->data, "got added in composition %p", composition);
    gst_bin_add (GST_BIN (composition), GST_ELEMENT (tmp->data));
  }

  if (ges_object_get_media_type (object) & GES_MEDIA_TYPE_VIDEO)
    _add_object_to_track (self, object, GES_MEDIA_TYPE_VIDEO);
  if (ges_object_get_media_type (object) & GES_MEDIA_TYPE_AUDIO)
    _add_object_to_track (self, object, GES_MEDIA_TYPE_AUDIO);

  g_sequence_insert_sorted (self->priv->object_by_start, g_object_ref_sink (object), (GCompareDataFunc) _compare_starts, NULL);
  g_list_free (nleobjects);
  return TRUE;
}

GESTimeline *
ges_timeline_new (GESMediaType media_type)
{
  return g_object_new (GES_TYPE_TIMELINE, "media-type", media_type, NULL);
}

gboolean
ges_timeline_commit (GESTimeline *self)
{
  GList *tmp;

  _update_transitions (self);

  for (tmp = self->priv->compositions; tmp; tmp = tmp->next) {
    nle_object_commit (NLE_OBJECT(tmp->data), TRUE);
  }

  for (tmp = self->priv->nleobjects; tmp; tmp = tmp->next) {
    nle_object_commit (NLE_OBJECT(tmp->data), TRUE);
  }

  return TRUE;
}

GList *ges_timeline_get_compositions_by_media_type (GESTimeline *self, GESMediaType media_type)
{
  return _get_compositions (self, media_type);
}

/* GObject initialization */

static void
_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GESTimeline *self = GES_TIMELINE (object);
  (void) self;

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GESTimelinePrivate *priv = GES_TIMELINE (object)->priv;
  (void) priv;

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_dispose (GObject *object)
{
  GESTimeline *self = GES_TIMELINE (object);

  GST_ERROR ("timeline disposed");
  g_sequence_free (self->priv->object_by_start);
  g_hash_table_foreach (self->priv->tracks, (GHFunc) _free_tracks, NULL);
  g_hash_table_unref (self->priv->tracks);
  g_list_free_full (self->priv->nleobjects, gst_object_unref);
  g_list_free (self->priv->compositions);
  gst_object_unref (self->priv->composition_bin);

  G_OBJECT_CLASS (ges_timeline_parent_class)->dispose (object);
}

static void
ges_timeline_class_init (GESTimelineClass *klass)
{
  GObjectClass *g_object_class = G_OBJECT_CLASS (klass);
  GESObjectClass *ges_object_class = GES_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GESTimelinePrivate));

  g_object_class->set_property = _set_property;
  g_object_class->get_property = _get_property;
  g_object_class->dispose = _dispose;

  ges_timeline_signals[TRANSITION_ADDED] =
      g_signal_new ("transition-added", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_FIRST, 0, NULL, NULL, g_cclosure_marshal_generic,
      G_TYPE_NONE, 1, GES_TYPE_TRANSITION);

  ges_timeline_signals[TRANSITION_REMOVED] =
      g_signal_new ("transition-removed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_FIRST, 0, NULL, NULL, g_cclosure_marshal_generic,
      G_TYPE_NONE, 1, GES_TYPE_TRANSITION);

  ges_object_class->set_inpoint = _set_inpoint;
  ges_object_class->set_duration = _set_duration;
  ges_object_class->set_start = _set_start;
  ges_object_class->set_track_index = _set_track_index;
  ges_object_class->get_nle_objects = _get_nle_objects;
  ges_object_class->set_media_type = _set_media_type;
}

static void
ges_timeline_init (GESTimeline *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      GES_TYPE_TIMELINE, GESTimelinePrivate);

  self->priv->compositions = NULL;
  self->priv->nleobjects = NULL;
  self->priv->composition_bin = gst_object_ref_sink (ges_composition_bin_new ());
  self->priv->object_by_start = g_sequence_new (g_object_unref);
  self->priv->tracks = g_hash_table_new (g_direct_hash, g_direct_equal);
}
