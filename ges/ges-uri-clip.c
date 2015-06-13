#include <grilo.h>
#include <gst/pbutils/gstdiscoverer.h>

#include "ges-timeline.h"
#include "ges-uri-clip.h"

/* Structure definitions */

#define GES_URI_CLIP_PRIV(self) (ges_uri_clip_get_instance_private (GES_URI_CLIP (self)))

typedef struct _GESUriClipPrivate
{
  gchar *uri;
  GstDiscovererInfo *info;
} GESUriClipPrivate;

struct _GESUriClip
{
  GESClip parent;
};

G_DEFINE_TYPE_WITH_CODE (GESUriClip, ges_uri_clip, GES_TYPE_CLIP,
    G_ADD_PRIVATE (GESUriClip)
    )

/* Implementation */

void
ges_uri_clip_set_uri (GESUriClip *self, const gchar *uri)
{
  GESUriClipPrivate *priv = GES_URI_CLIP_PRIV (self);

  if (priv->uri) {
    GST_ERROR_OBJECT (self, "changing uri is not supported yet");
  }

  priv->uri = g_strdup (uri);
}

static GrlMedia *
grl_media_from_uri (const gchar * uri)
{
  GrlMedia *media = NULL;
  GError *error = NULL;
  GList *tmp;
  GrlOperationOptions *options;
  GrlRegistry *registry = grl_registry_get_default ();
  GrlSource *source;
  GrlKeyID disco_key_id =
    grl_registry_lookup_metadata_key (registry, "discovery");
  GList *keys = grl_metadata_key_list_new (GRL_METADATA_KEY_EXTERNAL_URL,
      GRL_METADATA_KEY_URL,
      disco_key_id,
      GRL_METADATA_KEY_INVALID);

  if (disco_key_id == GRL_METADATA_KEY_INVALID)
    return NULL;

  registry = grl_registry_get_default ();

  options = grl_operation_options_new (NULL);
  grl_operation_options_set_count (options, 1);
  grl_operation_options_set_resolution_flags (options, GRL_RESOLVE_FULL);

  for (tmp = grl_registry_get_sources_by_operations (registry, GRL_OP_MEDIA_FROM_URI, TRUE); tmp; tmp=tmp->next)  {
    source = GRL_SOURCE (tmp->data);
    if (grl_source_test_media_from_uri (source, uri)) {
      error = NULL;
      media = grl_source_get_media_from_uri_sync (source,
          uri, keys, options, &error);
      if (media) {
        break;
      }
    }
  }

  return media;
}

static GstDiscovererInfo *
_get_discoverer_info_from_grl_media (GrlMedia *media)
{
  GrlRegistry *registry = grl_registry_get_default ();
  const GValue *info_value;
  GstDiscovererInfo *info;

  GrlKeyID disco_key_id =
    grl_registry_lookup_metadata_key (registry, "discovery");

  info_value = grl_data_get (GRL_DATA (media), disco_key_id);
  info = g_value_get_object (info_value);

  return info;
}

GESClip *
ges_uri_clip_new (const gchar *uri, GESMediaType media_type)
{
  GrlMedia *media = grl_media_from_uri (uri);
  GstDiscovererInfo *info;
  GList *streams;
  GESClip *res;
  GESUriClipPrivate *priv;

  if (!media) {
    GST_WARNING ("We couldn't recognize this uri with grilo : %s, beware", uri);
    /* We still return a clip, the user might have its own asset management
     * tools, but he will need to set the duration himself */
    return g_object_new (GES_TYPE_URI_CLIP, "uri", grl_media_get_url (media), "media-type", media_type, NULL);
  }

  info = _get_discoverer_info_from_grl_media (media);
  if (media_type == GES_MEDIA_TYPE_AUDIO) {
    streams = gst_discoverer_info_get_audio_streams (info);
    if (!streams) {
      gst_discoverer_info_unref (info);
      return NULL;
    }
  } else if (media_type == GES_MEDIA_TYPE_VIDEO) {
    streams = gst_discoverer_info_get_video_streams (info);
    if (!streams) {
      gst_discoverer_info_unref (info);
      return NULL;
    }
  } else {
    return NULL;
  }

  gst_discoverer_stream_info_list_free (streams);

  res = g_object_new (GES_TYPE_URI_CLIP, "uri", grl_media_get_url (media), "media-type", media_type, NULL);

  priv = GES_URI_CLIP_PRIV (res);
  priv->info = info;
  GST_DEBUG_OBJECT (res, "Actual media uri : %s", grl_media_get_url (media));
  g_object_set (res, "duration", gst_discoverer_info_get_duration (info), NULL);

  return res;
}

static GstElement *
_make_element (GESClip *clip)
{
  GESMediaType media_type;
  GstElement *decodebin;
  GstCaps *caps;
  GESUriClipPrivate *priv = GES_URI_CLIP_PRIV (clip);
  g_object_get (clip, "media-type", &media_type, NULL);

  if (media_type == GES_MEDIA_TYPE_VIDEO)
    caps = gst_caps_from_string (GES_RAW_VIDEO_CAPS);
  else if (media_type == GES_MEDIA_TYPE_AUDIO)
    caps = gst_caps_from_string (GES_RAW_AUDIO_CAPS);
  else {
    GST_ERROR_OBJECT (clip, "media type not supported : %d", media_type);
    return NULL;
  }

  decodebin = gst_element_factory_make ("uridecodebin", NULL);
  g_object_set (decodebin, "caps", caps,
      "expose-all-streams", FALSE, "uri", priv->uri, NULL);

  /* In case this is a remote source, we'll want to use download buffering and
   * have a large queue so nle can perform the switches peacefully */
  g_object_set (decodebin, "use-buffering", FALSE, "download", TRUE, "buffer-size", 10 * 1024 * 1024, NULL);
  gst_caps_unref (caps);

  return decodebin;
}

static gboolean
_set_inpoint (GESObject *object, GstClockTime inpoint)
{
  GESUriClipPrivate *priv = GES_URI_CLIP_PRIV (object);
  GstClockTime media_duration;

  if (!priv->info)
    goto chain_up;

  media_duration = gst_discoverer_info_get_duration (priv->info);

  if (inpoint >= media_duration) {
    GST_WARNING_OBJECT (object, "Cannot set an inpoint superior to the media duration");
    return FALSE;
  }

  if (inpoint + ges_object_get_duration (object) > media_duration)
    ges_object_set_duration (object, media_duration - inpoint);

chain_up:
  return GES_OBJECT_CLASS (ges_uri_clip_parent_class)->set_inpoint (object, inpoint);
}

static gboolean
_set_duration (GESObject *object, GstClockTime duration)
{
  GESUriClipPrivate *priv = GES_URI_CLIP_PRIV (object);
  GstClockTime media_duration;

  if (!priv->info)
    goto chain_up;

  media_duration = gst_discoverer_info_get_duration (priv->info);

  if (ges_object_get_inpoint (object) + duration > media_duration) {
    GST_WARNING_OBJECT (object, "Cannot set a duration which, added to the"
        "inpoint, would be superior to the media duration");
    return FALSE;
  }

chain_up:
  return GES_OBJECT_CLASS (ges_uri_clip_parent_class)->set_duration (object, duration);
}

/* GObject initialization */

enum
{
  PROP_0,
  PROP_URI,
};

static void
_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GESUriClip *self = GES_URI_CLIP (object);

  switch (property_id) {
    case PROP_URI:
      ges_uri_clip_set_uri (self, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GESUriClipPrivate *priv = GES_URI_CLIP_PRIV (object);

  switch (property_id) {
    case PROP_URI:
      g_value_set_string (value, priv->uri);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_dispose (GObject *object)
{
  GESUriClipPrivate *priv = GES_URI_CLIP_PRIV (object);

  if (priv->uri)
    g_free (priv->uri);

  G_OBJECT_CLASS (ges_uri_clip_parent_class)->dispose (object);
}

static void
ges_uri_clip_class_init (GESUriClipClass *klass)
{
  GObjectClass *g_object_class = G_OBJECT_CLASS (klass);
  GESClipClass *clip_class = GES_CLIP_CLASS (klass);
  GESObjectClass *ges_object_class = GES_OBJECT_CLASS (klass);

  g_object_class->set_property = _set_property;
  g_object_class->get_property = _get_property;
  g_object_class->dispose = _dispose;

  g_object_class_install_property (g_object_class, PROP_URI,
      g_param_spec_string ("uri", "URI", "uri of the resource", NULL,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  ges_object_class->set_duration = _set_duration;
  ges_object_class->set_inpoint = _set_inpoint;
  clip_class->make_element = _make_element;
}

static void
ges_uri_clip_init (GESUriClip *self)
{
  GESUriClipPrivate *priv = GES_URI_CLIP_PRIV (self);

  priv->uri = NULL;
  priv->info = NULL;
}
