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

enum
{
  STREAM_AUDIO = 0,
  STREAM_VIDEO,
  STREAM_TEXT,
  STREAM_LAST
};

static gboolean
append_media_field (GQuark field_id, const GValue * value, gpointer user_data)
{
  GstStructure *media = user_data;
  gchar *value_str = gst_value_serialize (value);

  GST_DEBUG_OBJECT (media, "field [%s:%s]", g_quark_to_string (field_id),
      value_str);
  gst_structure_id_set_value (media, field_id, value);

  g_free (value_str);

  return TRUE;
}

/* Return whether stream type is video, audio or subtitle.
  * To add media-info structure.
  * Returns : (guint) typee or STREAM_LAST if mime_type is NULL.
  */
guint
gst_cool_find_type (const gchar * mime_type)
{
  g_return_val_if_fail (mime_type != NULL, STREAM_LAST);

  guint type;
  if (g_strrstr (mime_type, "video"))
    type = STREAM_VIDEO;
  else if (g_strrstr (mime_type, "audio"))
    type = STREAM_AUDIO;
  else
    type = STREAM_TEXT;

  GST_LOG ("return type = %u", type);
  return type;
}

/* Return media-info structure that is converted from caps information.
  * Returns : (GstStructure *) structure or NULL if caps is NULL.
  */
GstStructure *
gst_cool_caps_to_info (GstCaps * caps, char *stream_id)
{
  g_return_val_if_fail (caps != NULL, NULL);

  GstStructure *s = gst_caps_get_structure (caps, 0);
  GstStructure *media_info;
  const gchar *mime_type = gst_structure_get_name (s);
  guint type = gst_cool_find_type (mime_type);

  GST_DEBUG ("getting caps information: %" GST_PTR_FORMAT, caps);

  media_info = gst_structure_new ("media-info",
      "stream-id", G_TYPE_STRING, stream_id,
      "type", G_TYPE_INT, type,
      "mime-type", G_TYPE_STRING, g_strdup (mime_type), NULL);

  /* append media information from caps's structure to media-info */
  gst_structure_foreach (s, append_media_field, media_info);
  GST_DEBUG ("Complete: structure information = [%" GST_PTR_FORMAT "]",
      media_info);

  return media_info;
}

/* Return media-info structure that is converted from taglist.
  * Returns : (GstStructure *) structure or NULL if taglist is NULL.
  */
GstStructure *
gst_cool_taglist_to_info (GstTagList * taglist, char *stream_id,
    const char *mime_type)
{
  g_return_val_if_fail (taglist != NULL, NULL);

  const gchar *str_structure;
  GstStructure *s;
  GstStructure *media_info;
  guint type = gst_cool_find_type (mime_type);

  GST_DEBUG ("getting taglist information: %" GST_PTR_FORMAT, taglist);

  media_info =
      gst_structure_new ("media-info", "stream-id", G_TYPE_STRING,
      stream_id, "type", G_TYPE_INT, type, "mime-type", G_TYPE_STRING,
      g_strdup (mime_type), NULL);

  str_structure = gst_tag_list_to_string (taglist);
  s = gst_structure_new_from_string (str_structure);

  /* append media information from taglist's structure to media-info */
  gst_structure_foreach (s, append_media_field, media_info);
  GST_DEBUG ("Complete: structure information = [%" GST_PTR_FORMAT "]",
      media_info);

  return media_info;
}
