#include "ges-timeline.h"
#include "ges-clip.h"
#include "ges-playable.h"

/* Structure definitions */

#define GES_CLIP_PRIV(self) (ges_clip_get_instance_private (GES_CLIP (self)))

typedef struct _GESClipPrivate
{
  GstElement *nleobject;
  GstObject *old_parent;
  GstPad *ghostpad;
  GstPad *static_sinkpad;
  guint track_index;
  GstElement *playable_bin;
  GESTransition *transition;
} GESClipPrivate;

static void ges_playable_interface_init (GESPlayableInterface * iface);

G_DEFINE_TYPE_WITH_CODE (GESClip, ges_clip, GES_TYPE_OBJECT,
    G_ADD_PRIVATE (GESClip)
    G_IMPLEMENT_INTERFACE (GES_TYPE_PLAYABLE, ges_playable_interface_init)
    )

/* Implementation */

static void
_pad_added_cb (GstElement *element, GstPad *srcpad, GESClip *self)
{
  GESClipPrivate *priv = GES_CLIP_PRIV (self);
  gst_element_no_more_pads (element);
  gst_pad_link (srcpad, priv->static_sinkpad);
}

static void
_make_nle_object (GESClip *self)
{
  GESClipPrivate *priv = GES_CLIP_PRIV (self);
  GstElement *element, *rate, *converter;
  GstElement *topbin;
  GstPad *srcpad, *ghost;
  GstCaps *caps;
  GESMediaType media_type;
  GESClipClass *klass = GES_CLIP_GET_CLASS (self);

  g_object_get (self, "media-type", &media_type, NULL);

  if (!klass->make_element)
    return;

  topbin = gst_bin_new (NULL);

  element = klass->make_element (self);
  if (!element) {
    GST_ERROR_OBJECT (self, "couldn't make source element");
    return;
  }

  priv->nleobject = gst_element_factory_make ("nlesource", NULL);

  if (media_type == GES_MEDIA_TYPE_VIDEO) {
    GstElement *framepositioner = gst_element_factory_make ("framepositioner", "framepositioner");

    caps = gst_caps_from_string(GES_RAW_VIDEO_CAPS);
    converter = gst_element_factory_make ("videoconvert", NULL);
    rate = gst_element_factory_make ("videorate", NULL);
    gst_bin_add_many (GST_BIN(topbin), element, converter, rate, framepositioner, NULL);
    gst_element_link_many (converter, rate, framepositioner, NULL);
    g_object_set (priv->nleobject, "caps", caps, NULL);
    srcpad = gst_element_get_static_pad (framepositioner, "src");
    gst_child_proxy_child_added (GST_CHILD_PROXY (self), G_OBJECT (framepositioner), "framepositioner");
  } else {
    GstElement *samplecontroller = gst_element_factory_make ("samplecontroller", "samplecontroller");

    caps = gst_caps_from_string(GES_RAW_AUDIO_CAPS);
    converter = gst_element_factory_make ("audioconvert", NULL);
    rate = gst_element_factory_make ("audioresample", NULL);
    gst_bin_add_many (GST_BIN(topbin), element, converter, rate, samplecontroller, NULL);
    gst_element_link_many (converter, rate, samplecontroller, NULL);
    g_object_set (priv->nleobject, "caps", caps, NULL);
    srcpad = gst_element_get_static_pad (samplecontroller, "src");
    gst_child_proxy_child_added (GST_CHILD_PROXY (self), G_OBJECT (samplecontroller), "samplecontroller");
  }

  priv->static_sinkpad = gst_element_get_static_pad (converter, "sink");


  ghost = gst_ghost_pad_new ("src", srcpad);
  gst_pad_set_active (ghost, TRUE);
  gst_element_add_pad (topbin, ghost);

  gst_object_unref (srcpad);

  gst_caps_unref (caps);

  srcpad = gst_element_get_static_pad (element, "src");
  if (srcpad) {
    gst_pad_link (srcpad, priv->static_sinkpad);
    gst_object_unref (srcpad);
  } else {
    g_signal_connect (element, "pad-added",
        G_CALLBACK (_pad_added_cb),
        self);
  }

  if (!gst_bin_add (GST_BIN (priv->nleobject), topbin))
    return;
}

static void
_expose_nle_object (GESClip *self)
{
  GstPad *pad;
  GESClipPrivate *priv = GES_CLIP_PRIV (self);

  g_object_get (priv->nleobject, "composition", &priv->old_parent, NULL);

  if (priv->old_parent) {
    gst_object_ref (priv->nleobject);
    gst_bin_remove (GST_BIN (priv->old_parent), priv->nleobject);
  }

  gst_bin_add (GST_BIN (priv->playable_bin), priv->nleobject);

  pad = gst_element_get_static_pad (priv->nleobject, "src");

  gst_ghost_pad_set_target (GST_GHOST_PAD (priv->ghostpad), pad);
  gst_pad_set_active (priv->ghostpad, TRUE);
}

static void
_unexpose_nle_object (GESClip *self)
{
  GESClipPrivate *priv = GES_CLIP_PRIV (self);

  gst_ghost_pad_set_target (GST_GHOST_PAD (priv->ghostpad), NULL);
  gst_pad_set_active (GST_PAD (priv->ghostpad), FALSE);

  if (priv->old_parent) {
    gst_object_ref (priv->nleobject);
    gst_bin_remove (GST_BIN (priv->playable_bin), priv->nleobject);
    gst_bin_add (GST_BIN (priv->old_parent), priv->nleobject);
    gst_object_unref (priv->old_parent);
    priv->old_parent = NULL;
  }
}

/* Interface implementation */

/* GESPlayable */

static GstBin *
_make_playable (GESPlayable *playable, gboolean is_playable)
{
  GESClip *self = GES_CLIP (playable);
  GESClipPrivate *priv = GES_CLIP_PRIV (self);

  if (is_playable)
    _expose_nle_object (self);
  else
    _unexpose_nle_object (self);

  return GST_BIN (priv->playable_bin);
}

static void
ges_playable_interface_init (GESPlayableInterface * iface)
{
  iface->make_playable = _make_playable;
}

/* API */

gboolean
ges_clip_set_transition (GESClip *self, GESTransition *transition)
{
  GESClipPrivate *priv = GES_CLIP_PRIV (self);

  if (priv->transition) {
    ges_transition_reset (priv->transition);
    g_object_unref (priv->transition);
  }

  priv->transition = transition;

  return TRUE;
}

GESTransition *
ges_clip_get_transition (GESClip *self)
{
  GESClipPrivate *priv = GES_CLIP_PRIV (self);
  return priv->transition;
}

/* GESObject implementation */

static gboolean
_set_inpoint (GESObject *object, GstClockTime inpoint)
{
  GESClipPrivate *priv = GES_CLIP_PRIV (object);

  g_object_set (priv->nleobject, "inpoint", inpoint, NULL);

  return TRUE;
}

static gboolean
_set_duration (GESObject *object, GstClockTime duration)
{
  GESClipPrivate *priv = GES_CLIP_PRIV (object);

  g_object_set (priv->nleobject, "duration", duration, NULL);

  return TRUE;
}

static gboolean
_set_start (GESObject *object, GstClockTime start)
{
  GESClipPrivate *priv = GES_CLIP_PRIV (object);

  g_object_set (priv->nleobject, "start", start, NULL);

  return TRUE;
}

static gboolean
_set_media_type (GESObject *object, GESMediaType media_type)
{
  GESClipPrivate *priv = GES_CLIP_PRIV (object);

  if (priv->nleobject) {
    GST_ERROR_OBJECT (object, "changing media type isn't supported yet");
    return FALSE;
  }

  return TRUE;
}

static GList *
_get_nle_objects (GESObject *object)
{
  GESClipPrivate *priv = GES_CLIP_PRIV (object);

  return g_list_append (NULL, priv->nleobject);
}

static gboolean
_set_track_index (GESObject *object, GESMediaType media_type, guint index)
{
  GESClipPrivate *priv = GES_CLIP_PRIV (object);
  guint new_priority;

  if (!(media_type & ges_object_get_media_type (object)))
      return FALSE;

  new_priority = (TRACK_PRIORITY_HEIGHT * index) + TIMELINE_PRIORITY_OFFSET;

  g_object_set (priv->nleobject, "priority", new_priority, NULL);

  return TRUE;
}

/* GObject initialization */

enum
{
  PROP_0,
  PROP_URI,
  PROP_MEDIA_TYPE,
};

static void
_dispose (GObject *object)
{
  GESClip *self = GES_CLIP (object);
  GESClipPrivate *priv = GES_CLIP_PRIV (self);

  if (priv->static_sinkpad)
    gst_object_unref (priv->static_sinkpad);

  gst_object_unref (priv->playable_bin);
  G_OBJECT_CLASS (ges_clip_parent_class)->dispose (object);
}

static void
_constructed (GObject *object)
{
  _make_nle_object (GES_CLIP (object));
  G_OBJECT_CLASS (ges_clip_parent_class)->constructed (object);
}

static void
ges_clip_class_init (GESClipClass *klass)
{
  GObjectClass *g_object_class = G_OBJECT_CLASS (klass);
  GESObjectClass *ges_object_class = GES_OBJECT_CLASS (klass);

  g_object_class->dispose = _dispose;
  g_object_class->constructed = _constructed;

  ges_object_class->set_start = _set_start;
  ges_object_class->set_duration = _set_duration;
  ges_object_class->set_inpoint = _set_inpoint;
  ges_object_class->set_media_type = _set_media_type;
  ges_object_class->set_track_index = _set_track_index;
  ges_object_class->get_nle_objects = _get_nle_objects;
}

static void
ges_clip_init (GESClip *self)
{
  gchar *padname;
  GESClipPrivate *priv = GES_CLIP_PRIV (self);

  priv->old_parent = NULL;
  priv->playable_bin = gst_object_ref_sink (gst_bin_new (NULL));
  priv->transition = NULL;
  padname = g_strdup_printf ("clip_%p_src", priv->nleobject);
  priv->ghostpad = gst_ghost_pad_new_no_target (padname, GST_PAD_SRC);
  g_free (padname);
  gst_element_add_pad (GST_ELEMENT (priv->playable_bin), priv->ghostpad);
}
