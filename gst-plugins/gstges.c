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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include "gstgessource.h"
#include "gstframepositioner.h"
#include "gstsamplecontroller.h"
#include "ges-smart-video-mixer.h"
#include "ges-smart-audio-mixer.h"

static gboolean
plugin_init (GstPlugin * plugin)
{
  gst_element_register (plugin, "gessource", GST_RANK_NONE,
      GST_GES_SOURCE_TYPE);
  gst_element_register (plugin, "framepositioner", GST_RANK_NONE,
      GST_TYPE_FRAME_POSITIONNER);
  gst_element_register (plugin, "samplecontroller", GST_RANK_NONE,
      GST_TYPE_SAMPLE_CONTROLLER);
  gst_element_register (plugin, "smartvideomixer", GST_RANK_NONE,
      GES_TYPE_SMART_MIXER);
  gst_element_register (plugin, "smartaudiomixer", GST_RANK_NONE,
      GES_TYPE_SMART_AUDIO_MIXER);

  return TRUE;
}

#define PACKAGE "gstreamer-editing-services"

/* plugin export resolution */
GST_PLUGIN_DEFINE
    (1,
    5,
    ges,
    "GStreamer Editing Services Plugin",
    plugin_init, "1.5.0", "LGPL", "ges", "meh")
