#include <gst/controller/controller.h>

#include "ges-transition.h"
#include "ges-source.h"

typedef struct _GESTransitionPrivate
{
  GESObject *fadeout_source;
  GESObject *fadein_source;
  GstControlSource *fadeout_control_source;
  GstControlSource *fadein_control_source;
} GESTransitionPrivate;

struct _GESTransition
{
  GObject parent;
  GESTransitionPrivate *priv;
};

G_DEFINE_TYPE (GESTransition, ges_transition, G_TYPE_OBJECT)

/* GObject initialization */

enum
{
  PROP_0,
  PROP_TRANSITION_TYPE,
  PROP_FADEDOUT_SOURCE,
  PROP_FADEDIN_SOURCE,
};

void
ges_transition_update (GESTransition *self)
{
  GstTimedValueControlSource *source;
  GstClockTime transition_start, transition_stop, duration;
  gdouble val;

  if (!self->priv->fadeout_source || !self->priv->fadein_source)
    return;

  source = GST_TIMED_VALUE_CONTROL_SOURCE (self->priv->fadeout_control_source);

  transition_start = ges_object_get_inpoint (self->priv->fadeout_source) +
      ges_object_get_start (self->priv->fadein_source) -
      ges_object_get_start (self->priv->fadeout_source);
  transition_stop = ges_object_get_inpoint (self->priv->fadeout_source) +
    ges_object_get_duration (self->priv->fadeout_source);
  duration = transition_stop - transition_start;

  if (!gst_control_source_get_value (self->priv->fadeout_control_source,
        ges_object_get_inpoint(self->priv->fadeout_source), &val))
    gst_timed_value_control_source_set (source,
        ges_object_get_inpoint(self->priv->fadeout_source), 1.0);

  gst_timed_value_control_source_set (source, transition_start, 1.0);
  gst_timed_value_control_source_set (source, transition_stop, 0.0);

  source = GST_TIMED_VALUE_CONTROL_SOURCE (self->priv->fadein_control_source);

  transition_start = ges_object_get_inpoint (self->priv->fadein_source);

  gst_timed_value_control_source_set (source, transition_start, 0.0);
  gst_timed_value_control_source_set (source, transition_start + duration, 1.0);
  gst_timed_value_control_source_set (source, ges_object_get_inpoint (self->priv->fadein_source) +
      ges_object_get_duration (self->priv->fadein_source), 1.0);
}

static void
_create_control_sources (GESTransition *self)
{
  if (ges_object_get_media_type (self->priv->fadeout_source) & GES_MEDIA_TYPE_VIDEO) {
    self->priv->fadeout_control_source =
      ges_object_get_interpolation_control_source (GES_OBJECT (self->priv->fadeout_source),
          "framepositioner::alpha", G_TYPE_NONE);
    self->priv->fadein_control_source =
      ges_object_get_interpolation_control_source (GES_OBJECT (self->priv->fadein_source),
          "framepositioner::alpha", G_TYPE_NONE);
  } else if (ges_object_get_media_type (self->priv->fadeout_source) & GES_MEDIA_TYPE_AUDIO) {
    self->priv->fadeout_control_source =
      ges_object_get_interpolation_control_source (GES_OBJECT (self->priv->fadeout_source),
          "samplecontroller::volume", G_TYPE_NONE);
    self->priv->fadein_control_source =
      ges_object_get_interpolation_control_source (GES_OBJECT (self->priv->fadein_source),
          "samplecontroller::volume", G_TYPE_NONE);
  }

  g_object_set (self->priv->fadeout_control_source, "mode", GST_INTERPOLATION_MODE_LINEAR, NULL);
  g_object_set (self->priv->fadein_control_source, "mode", GST_INTERPOLATION_MODE_LINEAR, NULL);
}

void
ges_transition_reset (GESTransition *self)
{
  GstClockTime end;
  GstTimedValueControlSource *source;

  if (!self->priv->fadeout_source || !self->priv->fadein_source)
    return;

  source = GST_TIMED_VALUE_CONTROL_SOURCE (self->priv->fadeout_control_source);

  end = ges_object_get_inpoint (GES_OBJECT (self->priv->fadeout_source)) +
    ges_object_get_duration (GES_OBJECT (self->priv->fadeout_source));

  gst_timed_value_control_source_set (source,
      ges_object_get_inpoint (GES_OBJECT (self->priv->fadeout_source)), 1.0);
  gst_timed_value_control_source_set (source, end, 1.0);

  source = GST_TIMED_VALUE_CONTROL_SOURCE (self->priv->fadein_control_source);

  end = ges_object_get_inpoint (GES_OBJECT (self->priv->fadein_source)) +
    ges_object_get_duration (GES_OBJECT (self->priv->fadein_source));

  gst_timed_value_control_source_set (source,
      ges_object_get_inpoint (GES_OBJECT (self->priv->fadein_source)), 1.0);
  gst_timed_value_control_source_set (source, end, 1.0);

  self->priv->fadein_source = NULL;
  self->priv->fadeout_source = NULL;
}

static void
_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GESTransition *self = GES_TRANSITION (object);

  switch (property_id) {
    case PROP_FADEDOUT_SOURCE:
      self->priv->fadeout_source = g_value_get_object (value);
      break;
    case PROP_FADEDIN_SOURCE:
      self->priv->fadein_source = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      return;
  }
}

static void
_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GESTransitionPrivate *priv = GES_TRANSITION (object)->priv;

  switch (property_id) {
    case PROP_FADEDOUT_SOURCE:
      g_value_set_object (value, priv->fadeout_source);
      break;
    case PROP_FADEDIN_SOURCE:
      g_value_set_object (value, priv->fadein_source);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_constructed (GObject *object)
{
  GESTransition *self = GES_TRANSITION (object);
  _create_control_sources (self);
  ges_transition_update (self);

  G_OBJECT_CLASS (ges_transition_parent_class)->constructed (object);
}

static void
_dispose (GObject *object)
{
  ges_transition_reset (GES_TRANSITION (object));
}

static void
ges_transition_class_init (GESTransitionClass *klass)
{
  GObjectClass *g_object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GESTransitionPrivate));

  g_object_class->set_property = _set_property;
  g_object_class->get_property = _get_property;
  g_object_class->constructed = _constructed;
  g_object_class->dispose = _dispose;

  g_object_class_install_property (g_object_class, PROP_FADEDOUT_SOURCE,
      g_param_spec_object ("faded-out-source", "faded-out-source", "The source to fade out",
        GES_TYPE_OBJECT, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (g_object_class, PROP_FADEDIN_SOURCE,
      g_param_spec_object ("faded-in-source", "faded-in-source", "The source to fade in",
        GES_TYPE_OBJECT, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
ges_transition_init (GESTransition *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      GES_TYPE_TRANSITION, GESTransitionPrivate);

  self->priv->fadein_source = NULL;
  self->priv->fadeout_source = NULL;
  self->priv->fadeout_control_source = NULL;
  self->priv->fadein_control_source = NULL;
}

GESTransition *
ges_transition_new (GESObject *fadeout_source, GESObject *fadein_source)
{
  return g_object_new (GES_TYPE_TRANSITION,
      "faded-out-source", fadeout_source,
      "faded-in-source", fadein_source,
      NULL);
}
