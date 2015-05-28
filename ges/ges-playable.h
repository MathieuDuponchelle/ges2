#ifndef _GES_PLAYABLE
#define _GES_PLAYABLE

#include <glib-object.h>
#include <gst/gst.h>
#include <gst/player/gstplayer.h>
#include <ges.h>

G_BEGIN_DECLS

#define GES_TYPE_PLAYABLE ges_playable_get_type ()

G_DECLARE_INTERFACE (GESPlayable, ges_playable, GES, PLAYABLE, GObject)

struct _GESPlayableInterface
{
  GTypeInterface g_iface;

  gboolean (*make_playable) (GESPlayable *, gboolean);
};

GstPlayer *
ges_playable_make_player (GESPlayable *playable); 

gboolean
ges_playable_make_playable (GESPlayable *playable, gboolean is_playable);

G_END_DECLS

#endif
