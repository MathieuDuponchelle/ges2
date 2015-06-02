#ifndef _GES_ENUMS_H_
#define _GES_ENUMS_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define GES_TYPE_MEDIA_TYPE (ges_media_type_get_type ())
GType ges_media_type_get_type (void);

typedef enum {
  GES_MEDIA_TYPE_UNKNOWN = 1 << 0,
  GES_MEDIA_TYPE_AUDIO   = 1 << 1,
  GES_MEDIA_TYPE_VIDEO   = 1 << 2,
} GESMediaType;

#define GES_RAW_AUDIO_CAPS "audio/x-raw"
#define GES_RAW_VIDEO_CAPS "video/x-raw"

G_END_DECLS

#endif
