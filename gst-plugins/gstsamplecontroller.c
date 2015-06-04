/* GStreamer
 * Copyright (C) 2013 Mathieu Duponchelle <mduponchelle1@gmail.com>
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
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>

#include "gstsamplecontroller.h"

static void gst_sample_controller_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_sample_controller_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static GstFlowReturn gst_sample_controller_transform_ip (GstBaseTransform *
    trans, GstBuffer * buf);

static gboolean
gst_sample_controller_meta_transform (GstBuffer * dest, GstMeta * meta,
    GstBuffer * buffer, GQuark type, gpointer data);

enum
{
  PROP_0,
  PROP_VOLUME,
  PROP_ZORDER,
};

static GstStaticPadTemplate gst_sample_controller_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw")
    );

static GstStaticPadTemplate gst_sample_controller_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw")
    );

G_DEFINE_TYPE (GstSampleController, gst_sample_controller,
    GST_TYPE_BASE_TRANSFORM);

static void
gst_sample_controller_dispose (GObject * object)
{
  G_OBJECT_CLASS (gst_sample_controller_parent_class)->dispose (object);
}

static void
gst_sample_controller_class_init (GstSampleControllerClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseTransformClass *base_transform_class =
      GST_BASE_TRANSFORM_CLASS (klass);

  gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
      gst_static_pad_template_get (&gst_sample_controller_src_template));
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
      gst_static_pad_template_get (&gst_sample_controller_sink_template));

  gobject_class->set_property = gst_sample_controller_set_property;
  gobject_class->get_property = gst_sample_controller_get_property;
  gobject_class->dispose = gst_sample_controller_dispose;
  base_transform_class->transform_ip =
      GST_DEBUG_FUNCPTR (gst_sample_controller_transform_ip);

  /**
   * gstframepositionner:alpha:
   *
   * The desired alpha for the stream.
   */
  g_object_class_install_property (gobject_class, PROP_VOLUME,
      g_param_spec_double ("volume", "volume", "volume of the stream",
          0.0, 1.0, 1.0, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

  /**
   * gstframepositionner:zorder:
   *
   * The desired z order for the stream.
   */
  g_object_class_install_property (gobject_class, PROP_ZORDER,
      g_param_spec_uint ("zorder", "zorder", "z order of the stream",
          0, 10000, 0, G_PARAM_READWRITE));

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "sample controller", "Metadata",
      "This element provides with tagging facilities",
      "mduponchelle1@gmail.com");
}

static void
gst_sample_controller_init (GstSampleController *samplecontroller)
{
  samplecontroller->volume = 1.0;
  samplecontroller->zorder = 0;
  samplecontroller->capsfilter = NULL;
}

void
gst_sample_controller_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstSampleController *samplecontroller = GST_SAMPLE_CONTROLLER (object);


  GST_OBJECT_LOCK (samplecontroller);
  switch (property_id) {
    case PROP_VOLUME:
      samplecontroller->volume = g_value_get_double (value);
      break;
    case PROP_ZORDER:
      samplecontroller->zorder = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (samplecontroller);
}

void
gst_sample_controller_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstSampleController *samplecontroller = GST_SAMPLE_CONTROLLER (object);

  switch (property_id) {
    case PROP_VOLUME:
      g_value_set_double (value, samplecontroller->volume);
      break;
    case PROP_ZORDER:
      g_value_set_uint (value, samplecontroller->zorder);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

GType
gst_sample_controller_meta_api_get_type (void)
{
  static volatile GType type;
  static const gchar *tags[] = { "audio", NULL };

  if (g_once_init_enter (&type)) {
    GType _type = gst_meta_api_type_register ("GstSampleControllerApi", tags);
    g_once_init_leave (&type, _type);
  }
  return type;
}

static const GstMetaInfo *
gst_sample_controller_get_info (void)
{
  static const GstMetaInfo *meta_info = NULL;

  if (g_once_init_enter (&meta_info)) {
    const GstMetaInfo *meta =
        gst_meta_register (gst_sample_controller_meta_api_get_type (),
        "GstSampleControllerMeta",
        sizeof (GstSampleControllerMeta), (GstMetaInitFunction) NULL,
        (GstMetaFreeFunction) NULL,
        (GstMetaTransformFunction) gst_sample_controller_meta_transform);
    g_once_init_leave (&meta_info, meta);
  }
  return meta_info;
}

static gboolean
gst_sample_controller_meta_transform (GstBuffer * dest, GstMeta * meta,
    GstBuffer * buffer, GQuark type, gpointer data)
{
  GstSampleControllerMeta *dmeta, *smeta;

  smeta = (GstSampleControllerMeta *) meta;

  if (GST_META_TRANSFORM_IS_COPY (type)) {
    /* only copy if the complete data is copied as well */
    dmeta =
        (GstSampleControllerMeta *) gst_buffer_add_meta (dest,
        gst_sample_controller_get_info (), NULL);
    dmeta->volume = smeta->volume;
    dmeta->zorder = smeta->zorder;
  }

  return TRUE;
}

static GstFlowReturn
gst_sample_controller_transform_ip (GstBaseTransform * trans, GstBuffer * buf)
{
  GstSampleControllerMeta *meta;
  GstSampleController *samplecontroller = GST_SAMPLE_CONTROLLER (trans);
  GstClockTime timestamp = GST_BUFFER_TIMESTAMP (buf);

  if (GST_CLOCK_TIME_IS_VALID (timestamp)) {
    gst_object_sync_values (GST_OBJECT (trans), timestamp);
  }

  meta =
      (GstSampleControllerMeta *) gst_buffer_add_meta (buf,
      gst_sample_controller_get_info (), NULL);

  GST_OBJECT_LOCK (samplecontroller);
  meta->volume = samplecontroller->volume;
  meta->zorder = samplecontroller->zorder;
  GST_OBJECT_UNLOCK (samplecontroller);

  GST_DEBUG_OBJECT (samplecontroller, "controlled volume @ %" GST_TIME_FORMAT " : %lf",
      GST_TIME_ARGS (timestamp), samplecontroller->volume);
  return GST_FLOW_OK;
}
