/* GStreamer Editing Services
 * Copyright (C) 2015 Mathieu Duponchelle <mathieu.duponchelle@opencreed.com>
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
 */

#ifndef _GES_LAUNCHER
#define _GES_LAUNCHER

#include <gio/gio.h>
#include <ges.h>

G_BEGIN_DECLS

#define GES_TYPE_LAUNCHER (ges_launcher_get_type ())

G_DECLARE_FINAL_TYPE (GESLauncher, ges_launcher, GES, LAUNCHER, GApplication)

GESLauncher* ges_launcher_new (void);
gint ges_launcher_get_exit_status (GESLauncher *self);

G_END_DECLS

#endif /* _GES_LAUNCHER */
