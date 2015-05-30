#include "ges-timeline.h"
#include "ges-composition-bin.h"
#include "ges-clip.h"
#include "ges-playable.h"
#include "nle.h"

/* Structure definitions */

typedef struct _GESTimelinePrivate
{
  GList *compositions;
  GList *nleobjects;
  GSequence *object_by_start;
  GESCompositionBin *composition_bin;
} GESTimelinePrivate;

struct _GESTimeline
{
  GESObject parent;
  GESTimelinePrivate *priv;
};

static void ges_playable_interface_init (GESPlayableInterface * iface);

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

  GST_DEBUG_OBJECT (self, "creating composition %s with caps %s", name, caps_string);
  g_object_set (composition, "caps", gst_caps_from_string (caps_string), NULL);
  g_object_set (wrapper, "caps", gst_caps_from_string (caps_string), NULL);

  gst_bin_add (GST_BIN (wrapper), composition);

  self->priv->compositions = g_list_append (self->priv->compositions, composition);
  self->priv->nleobjects = g_list_append (self->priv->nleobjects, wrapper);

  return composition;
}

static void
_make_nle_objects (GESTimeline *self, GESMediaType media_type)
{
  GstElement *composition;

  if (media_type & GES_MEDIA_TYPE_AUDIO) {
    composition = _create_composition (self, GES_RAW_AUDIO_CAPS, "audio-composition");
    _add_expandable_operation (composition, "audiomixer", 0, "timeline-audiomixer");
    _add_expandable_source (composition, gst_element_factory_make ("videotestsrc", NULL), 1, "timeline-audio-background");
  }

  if (media_type & GES_MEDIA_TYPE_VIDEO) {
    GstElement *background = gst_parse_bin_from_description (
        "videotestsrc ! framepositioner name=background_video_positioner", TRUE, NULL);
    GstElement *pos = gst_bin_get_by_name (GST_BIN (background), "background_video_positioner");
    g_object_set (pos, "alpha", 0.0, NULL);
    gst_object_unref (pos);
    composition = _create_composition (self, GES_RAW_VIDEO_CAPS, "video-composition");
    _add_expandable_operation (composition, "smartvideomixer", 0, "timeline-videomixer");
    _add_expandable_source (composition, background, 1, "timeline-video-background");
  }
}

static void
_update_transitions (GESTimeline *self)
{
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

  g_sequence_insert_sorted (self->priv->object_by_start, object, (GCompareDataFunc) _compare_starts, NULL);
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

enum
{
  PROP_0,
  PROP_MEDIA_TYPE,
};

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
ges_timeline_class_init (GESTimelineClass *klass)
{
  GObjectClass *g_object_class = G_OBJECT_CLASS (klass);
  GESObjectClass *ges_object_class = GES_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GESTimelinePrivate));

  g_object_class->set_property = _set_property;
  g_object_class->get_property = _get_property;

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
  self->priv->composition_bin = ges_composition_bin_new ();
  self->priv->object_by_start = g_sequence_new (NULL);
}
