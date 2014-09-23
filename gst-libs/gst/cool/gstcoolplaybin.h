/* GStreamer Plugins Cool
 * Copyright (C) 2014 LG Electronics, Inc.
 *	Author : Jeongseok Kim <jeongseok.kim@lge.com>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#ifndef __GST_COOL_PLAYBIN_H__
#define __GST_COOL_PLAYBIN_H__

#include <gst/gst.h>
#include <gst/cool/gstcoolutil.h>

G_BEGIN_DECLS

gboolean        gst_cool_playbin_init           (GstElement * playbin);

GstElement *    gst_cool_playbin_get_q2         (GstElement * playbin);

void            gst_cool_playbin_set_q2_conf    (GstElement * playbin,
                                                 const gchar * firstfield, ...);

G_END_DECLS

#endif
