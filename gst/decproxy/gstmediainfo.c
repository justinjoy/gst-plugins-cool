/* GStreamer Plugins Cool
 * Copyright (C) 2014 LG Electronics, Inc.
 *	Author : HoonHee Lee <hoonhee.lee@lge.com>
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

#include "gstmediainfo.h"

#define SINK_CAPS \
    "text/x-raw;" \
    "text/x-avi-internal;" \
    "text/x-avi-unknown;" \
    "application/x-ass;" \
    "application/x-ssa;" \
    "subpicture/x-dvd;" \
    "subpicture/x-dvb;" \
    "subpicture/x-xsub;" \
    "audio/x-raw"

enum
{
  STREAM_AUDIO = 0,
  STREAM_VIDEO,
  STREAM_TEXT,
  STREAM_LAST
};

static GstStaticPadTemplate gst_media_info_sink_pad_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (SINK_CAPS)
    );

static GstStaticPadTemplate gst_media_info_src_pad_template =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("subtitle/x-media;audio/x-media")
    );

GST_DEBUG_CATEGORY_STATIC (media_info_debug);
#define GST_CAT_DEFAULT media_info_debug

static GstStateChangeReturn
gst_media_info_change_state (GstElement * element, GstStateChange transition);

static gboolean gst_media_info_set_caps (GstMediaInfo * info, GstCaps * caps);
static gboolean gst_media_info_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
static GstFlowReturn gst_media_info_chain (GstPad * pad, GstObject * parent,
    GstBuffer * buffer);

#define gst_media_info_parent_class parent_class
G_DEFINE_TYPE (GstMediaInfo, gst_media_info, GST_TYPE_ELEMENT);


static gboolean
gst_media_info_set_caps (GstMediaInfo * info, GstCaps * caps)
{
  gboolean ret;
  GstCaps *outcaps;

  outcaps = gst_caps_copy (caps);
  ret = gst_pad_set_caps (info->srcpad, outcaps);
  gst_caps_unref (outcaps);

  return ret;
}

static void
gst_media_info_class_init (GstMediaInfoClass * klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_media_info_src_pad_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_media_info_sink_pad_template));

  gst_element_class_set_static_metadata (element_class,
      "Media Info converter for text", "Codec/Parser/Decoder/Audio",
      "Pass data to decoder", "HoonHee Lee <hoonhee.lee@lge.com>");

  element_class->change_state = GST_DEBUG_FUNCPTR (gst_media_info_change_state);

  GST_DEBUG_CATEGORY_INIT (media_info_debug, "mediainfo", 0,
      "Media info converter for text");
}

static void
gst_media_info_init (GstMediaInfo * info)
{
  info->sinkpad =
      gst_pad_new_from_static_template (&gst_media_info_sink_pad_template,
      "sink");
  gst_pad_set_event_function (info->sinkpad,
      GST_DEBUG_FUNCPTR (gst_media_info_sink_event));
  gst_pad_set_chain_function (info->sinkpad,
      GST_DEBUG_FUNCPTR (gst_media_info_chain));
  gst_element_add_pad (GST_ELEMENT (info), info->sinkpad);

  info->srcpad =
      gst_pad_new_from_static_template (&gst_media_info_src_pad_template,
      "src");
  gst_element_add_pad (GST_ELEMENT (info), info->srcpad);
}

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

static void
posting_media_info_msg (GstMediaInfo * info, GstCaps * caps, gchar * stream_id)
{
  GstStructure *s;
  GstStructure *media_info;
  GstMessage *message;
  const gchar *structure_name;

  GST_DEBUG_OBJECT (info, "getting caps of %" GST_PTR_FORMAT, caps);

  s = gst_caps_get_structure (caps, 0);
  structure_name = gst_structure_get_name (s);

  media_info =
      gst_structure_new ("media-info", "stream-id", G_TYPE_STRING, stream_id,
      "type", G_TYPE_INT, STREAM_TEXT, "mime-type", G_TYPE_STRING,
      structure_name, NULL);
  /* append media information from caps's structure to media-info */
  gst_structure_foreach (s, append_media_field, media_info);
  GST_INFO_OBJECT (info, "create new media info : %" GST_PTR_FORMAT,
      media_info);

  message =
      gst_message_new_custom (GST_MESSAGE_APPLICATION, GST_OBJECT (info),
      media_info);
  gst_element_post_message (GST_ELEMENT_CAST (info), message);
  GST_INFO_OBJECT (info, "posted media-info message");
}

static void
posting_media_info_msg_by_tag (GstMediaInfo * info, GstTagList * tags,
    gchar * stream_id)
{
  GstStructure *media_info;
  GstMessage *message;
  const gchar *fieldname, *structure_name;
  const GValue *value;
  GstStructure *s;
  GstCaps *caps;
  gint tags_size = gst_tag_list_n_tags (tags);
  guint i;

  GST_DEBUG_OBJECT (info, "getting tags of %" GST_PTR_FORMAT, tags);
  caps = gst_pad_get_current_caps (info->sinkpad);
  s = gst_caps_get_structure (caps, 0);
  structure_name = gst_structure_get_name (s);

  media_info =
      gst_structure_new ("media-info", "stream-id", G_TYPE_STRING, stream_id,
      "type", G_TYPE_INT, STREAM_TEXT, "mime-type", G_TYPE_STRING,
      structure_name, NULL);

  for (i = 0; i < tags_size; i++) {
    fieldname = gst_tag_list_nth_tag_name (tags, i);
    value = gst_tag_list_get_value_index (tags, fieldname, i);
    gst_structure_set_value (media_info, fieldname, value);
  }
  message =
      gst_message_new_custom (GST_MESSAGE_APPLICATION, GST_OBJECT (info),
      media_info);
  gst_element_post_message (GST_ELEMENT_CAST (info), message);
  GST_INFO_OBJECT (info, "posted media-info message : %" GST_PTR_FORMAT,
      media_info);

  gst_caps_unref (caps);
}

static gboolean
gst_media_info_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstMediaInfo *info;
  gboolean res = TRUE;

  info = GST_MEDIA_INFO (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;
      gchar *stream_id;
      GstStructure *s;
      const gchar *structure_name;

      gst_event_parse_caps (event, &caps);
      GST_DEBUG_OBJECT (pad, "got caps %" GST_PTR_FORMAT, caps);

      s = gst_caps_get_structure (caps, 0);
      structure_name = gst_structure_get_name (s);

      if (g_str_has_prefix (structure_name, "audio/"))
        gst_pad_set_caps (info->srcpad, gst_caps_from_string ("audio/x-media"));
      else
        gst_pad_set_caps (info->srcpad,
            gst_caps_from_string ("subtitle/x-media"));

      gst_media_info_set_caps (info, caps);

      /* post media-info */
      stream_id = gst_pad_get_stream_id (pad);
      posting_media_info_msg (info, caps, stream_id);
      g_free (stream_id);

      gst_event_unref (event);
      gst_caps_unref (caps);
    }
      break;
    case GST_EVENT_TAG:
    {
      GstTagList *tags;;
      gchar *stream_id;

      gst_event_parse_tag (event, &tags);
      GST_DEBUG_OBJECT (pad, "got tags %" GST_PTR_FORMAT, tags);

      /* post media-info */
      stream_id = gst_pad_get_stream_id (pad);
      posting_media_info_msg_by_tag (info, tags, stream_id);
      g_free (stream_id);

      gst_event_unref (event);
    }
      break;
    default:
      res = gst_pad_event_default (pad, parent, event);
      break;
  }

  return res;
}


static GstFlowReturn
gst_media_info_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  GstMediaInfo *info;
  GstMapInfo inmap;
  GstBuffer *outbuf;
  GstFlowReturn ret;

  info = GST_MEDIA_INFO (parent);

  GST_LOG_OBJECT (info, "buffer with ts=%" GST_TIME_FORMAT,
      GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buffer)));

  gst_buffer_map (buffer, &inmap, GST_MAP_READ);

  outbuf = gst_buffer_copy (buffer);

  gst_buffer_unmap (buffer, &inmap);
  gst_buffer_unref (buffer);

  ret = gst_pad_push (info->srcpad, outbuf);

  return ret;

}

static GstStateChangeReturn
gst_media_info_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret != GST_STATE_CHANGE_SUCCESS)
    return ret;

  switch (transition) {
    default:
      break;
  }

  return ret;
}
