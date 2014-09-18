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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstcool.h"
#include "gstcoolutil.h"

GstStructure *
gst_cool_caps_to_info (GstCaps * caps)
{
  g_return_val_if_fail (caps != NULL, NULL);
  // TODO:
  return NULL;
}

GstStructure *
gst_cool_taglist_to_info (GstTagList * taglist)
{
  g_return_val_if_fail (taglist != NULL, NULL);
  // TODO:
  return NULL;
}
