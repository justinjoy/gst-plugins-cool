/* GStreamer Plugins Cool
 * Copyright (C) 2013-2014 LG Electronics, Inc.
 *  Author : Wonchul Lee <wonchul86.lee@lge.com>
 *           Jeongseok Kim <jeongseok.kim@lge.com>
 *           HoonHee Lee <hoonhee.lee@lge.com>
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

#include "gstfakevdec.h"
#include "gstfdcaps.h"

static GstStaticPadTemplate gst_fakevdec_sink_pad_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (FD_VIDEO_CAPS)
    );

static GstStaticPadTemplate gst_fakevdec_src_pad_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw")
    );

GST_DEBUG_CATEGORY_STATIC (fakevdec_debug);
#define GST_CAT_DEFAULT fakevdec_debug

static gboolean gst_fakevdec_set_caps (GstFakeVdec * fakevdec, GstCaps * caps);
static gboolean gst_fakevdec_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
static GstFlowReturn gst_fakevdec_chain (GstPad * pad, GstObject * parent,
    GstBuffer * buffer);

#define gst_fakevdec_parent_class parent_class
G_DEFINE_TYPE (GstFakeVdec, gst_fakevdec, GST_TYPE_ELEMENT);


static gboolean
gst_fakevdec_set_caps (GstFakeVdec * fakevdec, GstCaps * caps)
{
  gboolean ret;
  GstCaps *outcaps;

  outcaps = gst_caps_new_empty_simple ("video/x-raw");
  ret = gst_pad_set_caps (fakevdec->srcpad, outcaps);
  gst_caps_unref (outcaps);

  return ret;
}

static void
gst_fakevdec_class_init (GstFakeVdecClass * klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fakevdec_src_pad_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fakevdec_sink_pad_template));

  gst_element_class_set_static_metadata (element_class, "Fake Video decoder",
      "Codec/Decoder/Video",
      "Pass data to backend decoder",
      "Wonchul Lee <wonchul86.lee@lge.com>, Jeongseok Kim <jeongseok.kim@lge.com>");

  GST_DEBUG_CATEGORY_INIT (fakevdec_debug, "fakevdec", 0, "Fake video decoder");
}

static void
gst_fakevdec_init (GstFakeVdec * fakevdec)
{
  fakevdec->sinkpad =
      gst_pad_new_from_static_template (&gst_fakevdec_sink_pad_template,
      "sink");
  gst_pad_set_event_function (fakevdec->sinkpad,
      GST_DEBUG_FUNCPTR (gst_fakevdec_sink_event));
  gst_pad_set_chain_function (fakevdec->sinkpad,
      GST_DEBUG_FUNCPTR (gst_fakevdec_chain));
  gst_element_add_pad (GST_ELEMENT (fakevdec), fakevdec->sinkpad);

  fakevdec->srcpad =
      gst_pad_new_from_static_template (&gst_fakevdec_src_pad_template, "src");
  gst_element_add_pad (GST_ELEMENT (fakevdec), fakevdec->srcpad);
}

static gboolean
gst_fakevdec_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstFakeVdec *fakevdec;
  gboolean res;

  fakevdec = GST_FAKEVDEC (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      if (!gst_pad_has_current_caps (fakevdec->srcpad)) {
        GstCaps *caps;
        gst_event_parse_caps (event, &caps);
        gst_fakevdec_set_caps (fakevdec, caps);
      }
      gst_event_unref (event);
      res = TRUE;
    }
      break;
    default:
      res = gst_pad_event_default (pad, parent, event);
      break;
  }

  return res;
}


static GstFlowReturn
gst_fakevdec_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  GstFakeVdec *fakevdec;
  GstFlowReturn ret;

  fakevdec = GST_FAKEVDEC (parent);

  buffer = gst_buffer_make_writable (buffer);
  GST_BUFFER_FLAG_SET (buffer, GST_BUFFER_FLAG_CORRUPTED);

  ret = gst_pad_push (fakevdec->srcpad, buffer);

  return ret;

}
