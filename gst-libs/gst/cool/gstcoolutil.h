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

#ifndef __GST_COOLUTIL_H__
#define __GST_COOLUTIL_H__

#include <gst/gst.h>

G_BEGIN_DECLS

typedef enum {
  GST_COOL_STREAM_TYPE_UNKNOWN = -1,
  GST_COOL_STREAM_TYPE_AUDIO = 0,
  GST_COOL_STREAM_TYPE_VIDEO,
  GST_COOL_STREAM_TYPE_TEXT
} GstCoolStreamType;

GstStructure *  gst_cool_caps_to_info           (GstCaps * caps,
                                                 char *stream_id);

GstStructure *  gst_cool_taglist_to_info        (GstTagList * taglist,
                                                 char *stream_id, const char *mime_type);

GstCoolStreamType gst_cool_find_type            (const gchar * mime_type);

G_END_DECLS

#endif
