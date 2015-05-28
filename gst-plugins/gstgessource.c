/* GStreamer GES plugin
 *
 * Copyright (C) 2015 Thibault Saunier <thibault.saunier@collabora.com>
 *
 * gstges.c
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
 *

 **
 * SECTION:gstgessource
 * @short_description: A GstBin subclasses use to use GESTimeline
 * as sources inside any GstPipeline.
 * @see_also: #GESTimeline
 *
 * The gstgessource is a bin that will simply expose the track source pads
 * and implements the GstUriHandler interface using a custom ges://0Xpointer
 * uri scheme.
 **/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>

#include "ges-playable.h"
#include "gstgessource.h"

GST_DEBUG_CATEGORY_STATIC (gstgessource);
#define GST_CAT_DEFAULT gstgessource

struct _GstGesSource {
  GstBin parent;
  GESPlayable *playable;
};

enum
{
  PROP_0,
  PROP_PLAYABLE,
  PROP_LAST
};

static void
gst_ges_source_uri_handler_init (gpointer g_iface, gpointer iface_data);

G_DEFINE_TYPE_WITH_CODE (GstGesSource, gst_ges_source, GST_TYPE_BIN,
    G_IMPLEMENT_INTERFACE (GST_TYPE_URI_HANDLER,
        gst_ges_source_uri_handler_init));

static GParamSpec *properties[PROP_LAST];

static gboolean
gst_ges_source_set_playable (GstGesSource * self, GESPlayable *playable)
{
  GstBin *sbin = GST_BIN (self);
  GstIterator *pads;
  gboolean done = FALSE;
  GValue paditem = { 0, };

  if (self->playable) {
    GST_FIXME_OBJECT (self, "Implement changing playable support");

    return FALSE;
  }

  self->playable = playable;
  ges_playable_make_playable (playable, TRUE);

  pads = gst_element_iterate_src_pads (GST_ELEMENT (playable));

  gst_bin_add (sbin, GST_ELEMENT (playable));

  while (!done) {
    switch (gst_iterator_next (pads, &paditem)) {
      case GST_ITERATOR_OK:
      {
        GstPad *gpad;
        GstPad *pad = g_value_get_object (&paditem);

        GST_ERROR_OBJECT (pad, "adding pad to bin : %s", gst_caps_to_string(gst_pad_query_caps (pad, NULL)));
        gpad = gst_ghost_pad_new (NULL, pad);

        gst_pad_set_active (gpad, TRUE);
        gst_element_add_pad (GST_ELEMENT (self), gpad);
        g_value_reset (&paditem);
      }
        break;
      case GST_ITERATOR_DONE:
      case GST_ITERATOR_ERROR:
        done = TRUE;
        break;
      case GST_ITERATOR_RESYNC:
        gst_iterator_resync (pads);
        break;
    }
  }
  g_value_reset (&paditem);
  gst_iterator_free (pads);

  gst_element_no_more_pads (GST_ELEMENT (self));
  return TRUE;
}

/*** GSTURIHANDLER INTERFACE *************************************************/

static GstURIType
gst_ges_source_uri_get_type (GType type)
{
  return GST_URI_SRC;
}

static const gchar *const *
gst_ges_source_uri_get_protocols (GType type)
{
  static const gchar *protocols[] = { "ges", NULL };

  return protocols;
}

static gchar *
gst_ges_source_uri_get_uri (GstURIHandler * handler)
{
  GstGesSource *self = GST_GES_SOURCE (handler);

  return self->playable ? g_strdup_printf ("ges://%p", self->playable) : NULL;
}

static gboolean
gst_ges_source_uri_set_uri (GstURIHandler * handler, const gchar * uri,
    GError ** error)
{
  void *ptr;

  ptr = (void *) g_ascii_strtoull (uri + 7, NULL, 16);
  gst_ges_source_set_playable (GST_GES_SOURCE (handler), GES_PLAYABLE (ptr));
  return TRUE;
}

static void
gst_ges_source_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *) g_iface;

  iface->get_type = gst_ges_source_uri_get_type;
  iface->get_protocols = gst_ges_source_uri_get_protocols;
  iface->get_uri = gst_ges_source_uri_get_uri;
  iface->set_uri = gst_ges_source_uri_set_uri;
}

static void
gst_ges_source_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstGesSource *self = GST_GES_SOURCE (object);

  switch (property_id) {
    case PROP_PLAYABLE:
      g_value_set_object (value, self->playable);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
gst_ges_source_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstGesSource *self = GST_GES_SOURCE (object);

  switch (property_id) {
    case PROP_PLAYABLE:
      gst_ges_source_set_playable (self, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_dispose (GObject *object)
{
  GstGesSource *self = GST_GES_SOURCE (object);
  GstBin *sbin = GST_BIN (self);

  if (self->playable) {
    gst_object_ref (self->playable);
    gst_bin_remove (sbin, GST_ELEMENT (self->playable));
    ges_playable_make_playable (self->playable, FALSE);
  }

  G_OBJECT_CLASS (gst_ges_source_parent_class)->dispose (object);
}

static void
gst_ges_source_class_init (GstGesSourceClass * self_class)
{
  GObjectClass *gclass = G_OBJECT_CLASS (self_class);

  GST_DEBUG_CATEGORY_INIT (gstgessource, "gstgessource", 0,
      "ges source element");

  gclass->get_property = gst_ges_source_get_property;
  gclass->set_property = gst_ges_source_set_property;

  /**
   * GstGesSource:clip:
   *
   * Clip to use in this source.
   */
  properties[PROP_PLAYABLE] = g_param_spec_object ("playable", "Playable",
      "GESPlayable to use in this source.",
      GST_TYPE_ELEMENT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gclass, PROP_LAST, properties);

  gclass->dispose = _dispose;
}

static void
gst_ges_source_init (GstGesSource * self)
{
}
