/*
 * GStreamer reformatter element
 *
 * Copyright 2015 LG Electronics, Inc.
 * Authors:
 *   SuHwang Kim <swhwang.kim@lge.com>
 *   HonHee Lee <hoonhee.lee@lge.com>
 *   YongJin Ohn <yongjin.ohn@lge.com>
 *
 * gstreformatter.c: Converting bin which contains audioconvert and audioreample
 * for PCM format
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gstreformatter.h"


GST_DEBUG_CATEGORY_STATIC (reformatter_debug);
#define GST_CAT_DEFAULT reformatter_debug

static GstStaticPadTemplate sinktemplate =
GST_STATIC_PAD_TEMPLATE ("sink", GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw"));

static GstStaticPadTemplate srctemplate =
    GST_STATIC_PAD_TEMPLATE ("src", GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw;audio/x-media"));

#define gst_reformatter_parent_class parent_class
G_DEFINE_TYPE (GstReformatter, gst_reformatter, GST_TYPE_BIN);

static void gst_reformatter_dispose (GObject * object);
static void gst_reformatter_finalize (GObject * object);

static gboolean gst_reformatter_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event);

static void
gst_reformatter_class_init (GstReformatterClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (reformatter_debug, "reformatter", 0,
      "Reformatter Bin");

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->dispose = GST_DEBUG_FUNCPTR (gst_reformatter_dispose);
  gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_reformatter_finalize);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sinktemplate));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&srctemplate));

  gst_element_class_set_static_metadata (element_class,
      "Reformatter for PCM", "Codec/Parser/Decoder/Audio",
      "Pass converted data to Inputselector",
      "Suhwang Kim <suhwang.kim@lge.com>");
}

static void
gst_reformatter_init (GstReformatter * reform)
{
  GstPad *pad;
  GstPad *gpad;
  GstPadTemplate *pad_tmpl;

  g_rec_mutex_init (&reform->lock);

  reform->aconverter = gst_element_factory_make ("audioconvert", NULL);
  if (!reform->aconverter)
    goto no_audiomixer;

  reform->aresampler = gst_element_factory_make ("audioresample", NULL);
  if (!reform->aresampler)
    goto no_audiomixer;

  gst_bin_add (GST_BIN (reform), reform->aconverter);
  gst_bin_add (GST_BIN (reform), reform->aresampler);

  pad = gst_element_get_static_pad (reform->aconverter, "sink");
  pad_tmpl = gst_static_pad_template_get (&sinktemplate);

  GST_DEBUG_OBJECT (pad, "Trying to connect audioconvert sink pad");

  gpad = gst_ghost_pad_new_from_template ("sink", pad, pad_tmpl);
  gst_pad_set_active (gpad, TRUE);
  gst_element_add_pad (GST_ELEMENT (reform), gpad);

  gst_pad_set_event_function (GST_PAD_CAST (gpad),
      GST_DEBUG_FUNCPTR (gst_reformatter_sink_event));

  gst_object_unref (pad_tmpl);
  gst_object_unref (pad);

  pad = gst_element_get_static_pad (reform->aresampler, "src");
  pad_tmpl = gst_static_pad_template_get (&srctemplate);

  GST_DEBUG_OBJECT (pad, "Trying to connect audioresample src pad");

  gpad = gst_ghost_pad_new_from_template ("src", pad, pad_tmpl);
  gst_pad_set_active (gpad, TRUE);
  gst_element_add_pad (GST_ELEMENT (reform), gpad);

  gst_object_unref (pad_tmpl);
  gst_object_unref (pad);

  GST_DEBUG_OBJECT (reform,
      "Trying to connect with audioconvert and audioresample");
  if (!gst_element_link_pads_full (reform->aconverter, "src",
          reform->aresampler, "sink", GST_PAD_LINK_CHECK_TEMPLATE_CAPS)) {
    goto could_not_link;
  }

  return;

no_audiomixer:
  {
    GST_ELEMENT_ERROR (reform, CORE, MISSING_PLUGIN, (NULL),
        ("No audiomixer element, check your installation"));
    return;
  }

could_not_link:
  {
    GST_ELEMENT_ERROR (reform, CORE, NEGOTIATION, (NULL),
        ("Can't link audioconvert to audioresample element"));
    gst_bin_remove (GST_BIN_CAST (reform), reform->aconverter);
    gst_bin_remove (GST_BIN_CAST (reform), reform->aresampler);
    return;
  }

}

static void
gst_reformatter_dispose (GObject * object)
{
  GstReformatter *reform = GST_REFORMATTER (object);

  if (reform->aresampler != NULL) {
    GST_DEBUG_OBJECT (reform->aresampler, "Trying to remove audioresample");
    gst_element_set_state (reform->aresampler, GST_STATE_NULL);
    gst_bin_remove (GST_BIN_CAST (reform), reform->aresampler);
    reform->aresampler = NULL;
  }

  if (reform->aconverter != NULL) {
    GST_DEBUG_OBJECT (reform->aconverter, "Trying to remove audioconvert");
    gst_element_set_state (reform->aconverter, GST_STATE_NULL);
    gst_bin_remove (GST_BIN_CAST (reform), reform->aconverter);
    reform->aconverter = NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_reformatter_finalize (GObject * object)
{
  GstReformatter *reform = GST_REFORMATTER (object);

  g_rec_mutex_clear (&reform->lock);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gst_reformatter_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstReformatter *reform = GST_REFORMATTER (parent);
  gboolean ret = TRUE;

  GST_DEBUG_OBJECT (pad, "got event %" GST_PTR_FORMAT, event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstMessage *message = NULL;
      GstCaps *caps = NULL;
      GstStructure *media_info = NULL;
      gchar *stream_id;
      GstPad *target_pad;
      GstCaps *target_caps;

      gst_event_parse_caps (event, &caps);

      GST_DEBUG_OBJECT (reform, "recieved caps %" GST_PTR_FORMAT, caps);
      GST_REFORMATTER_LOCK (reform);

      stream_id = gst_pad_get_stream_id (pad);
      media_info = gst_cool_caps_to_info (caps, stream_id);

      /* post media-info */
      message =
          gst_message_new_custom (GST_MESSAGE_APPLICATION,
          GST_OBJECT (reform), media_info);
      gst_element_post_message (GST_ELEMENT_CAST (reform), message);

      GST_REFORMATTER_UNLOCK (reform);

      g_free (stream_id);

      /* set up the outcaps in order to finish auto-plugging */
      target_pad = gst_element_get_static_pad (GST_ELEMENT (reform), "src");
      target_caps = gst_caps_from_string ("audio/x-media");
      gst_pad_set_caps (target_pad, target_caps);

      gst_object_unref (target_pad);
      gst_caps_unref (target_caps);

      ret = gst_pad_event_default (pad, parent, event);
      break;
    }
    case GST_EVENT_TAG:
    {
      GstTagList *tags;
      GstMessage *message = NULL;
      GstCaps *caps = NULL;
      GstStructure *media_info = NULL;
      gchar *stream_id;
      const gchar *mime_type;

      gst_event_parse_tag (event, &tags);
      GST_DEBUG_OBJECT (pad, "got taglist %" GST_PTR_FORMAT, tags);

      GST_REFORMATTER_LOCK (reform);
      caps = gst_pad_get_current_caps (pad);
      mime_type = gst_structure_get_name (gst_caps_get_structure (caps, 0));

      stream_id = gst_pad_get_stream_id (pad);
      media_info = gst_cool_taglist_to_info (tags, stream_id, mime_type);

      if (media_info) {
        /* post media-info */
        message =
            gst_message_new_custom (GST_MESSAGE_APPLICATION,
            GST_OBJECT (reform), media_info);
        gst_element_post_message (GST_ELEMENT_CAST (reform), message);
      }

      GST_REFORMATTER_UNLOCK (reform);

      gst_caps_unref (caps);
      g_free (stream_id);
      ret = gst_pad_event_default (pad, parent, event);
      break;
    }
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }

  return ret;
}
