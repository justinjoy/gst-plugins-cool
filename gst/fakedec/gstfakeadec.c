/* GStreamer Plugins Cool
 * Copyright (C) 2013-2014 LG Electronics, Inc.
 *    Author : Wonchul Lee <wonchul86.lee@lge.com>
 *           Jeongseok Kim <jeongseok.kim@lge.com>
 *           HoonHee Lee <hoonhee.lee@lge.com>
 *           Myoungsun Lee <mysunny.lee@lge.com>
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

#include "gstfakeadec.h"
#include "gstfdcaps.h"

static GstStaticPadTemplate gst_fakeadec_sink_pad_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (FD_AUDIO_CAPS)
    );

static GstStaticPadTemplate gst_fakeadec_src_pad_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw")
    );

enum
{
  PROP_0,
  PROP_ACTIVE_MODE,
  PROP_LAST
};

GST_DEBUG_CATEGORY_STATIC (fakeadec_debug);
#define GST_CAT_DEFAULT fakeadec_debug

static gboolean gst_fakeadec_set_caps (GstFakeAdec * fakeadec, GstCaps * caps);
static gboolean gst_fakeadec_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
static void gst_fakeadec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static GstFlowReturn gst_fakeadec_chain (GstPad * pad, GstObject * parent,
    GstBuffer * buffer);

#define gst_fakeadec_parent_class parent_class
G_DEFINE_TYPE (GstFakeAdec, gst_fakeadec, GST_TYPE_ELEMENT);


static gboolean
gst_fakeadec_set_caps (GstFakeAdec * fakeadec, GstCaps * caps)
{
  gboolean ret;

  GstStructure *s;
  GstCaps *output_caps;
  gint rate, channels;
  const gchar *format;

  GST_DEBUG_OBJECT (fakeadec, "got caps %" GST_PTR_FORMAT, caps);
  s = gst_caps_get_structure (caps, 0);

  output_caps = gst_caps_new_empty_simple ("audio/x-raw");

  /* set format */
  format = gst_structure_get_string (s, "format");

  //FIXME: hardcoded format should be fixed
  if (!format)
    gst_caps_set_simple (output_caps, "format", G_TYPE_STRING, "S32LE", NULL);
  else
    gst_caps_set_simple (output_caps, "format", G_TYPE_STRING, format, NULL);
  /* set layout */
  gst_caps_set_simple (output_caps, "layout", G_TYPE_STRING, "interleaved",
      NULL);
  /* set rate */
  gst_structure_get_int (s, "rate", &rate);
  gst_caps_set_simple (output_caps, "rate", G_TYPE_INT, rate, NULL);
  /* set channels */
  gst_structure_get_int (s, "channels", &channels);
  /* FIXME: omxaudiosink can handle less than two channels */
  if (channels > 2)
    channels = 2;

  gst_caps_set_simple (output_caps, "channels", G_TYPE_INT, channels, NULL);

  ret = gst_pad_set_caps (fakeadec->srcpad, output_caps);
  gst_caps_unref (output_caps);

  return ret;
}

static void
gst_fakeadec_class_init (GstFakeAdecClass * klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = gst_fakeadec_set_property;

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fakeadec_src_pad_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fakeadec_sink_pad_template));

  gst_element_class_set_static_metadata (element_class, "Fake Audio decoder",
      "Codec/Decoder/Audio",
      "Pass data to backend decoder",
      "Wonchul Lee <wonchul86.lee@lge.com>, Jeongseok Kim <jeongseok.kim@lge.com>");

  g_object_class_install_property (gobject_class, PROP_ACTIVE_MODE,
      g_param_spec_boolean ("active-mode", "Active mode",
          "Set active mode to fakeadec",
          FALSE, G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  GST_DEBUG_CATEGORY_INIT (fakeadec_debug, "fakeadec", 0, "Fake audio decoder");
}

static void
gst_fakeadec_init (GstFakeAdec * fakeadec)
{
  fakeadec->sinkpad =
      gst_pad_new_from_static_template (&gst_fakeadec_sink_pad_template,
      "sink");
  gst_pad_set_chain_function (fakeadec->sinkpad,
      GST_DEBUG_FUNCPTR (gst_fakeadec_chain));
  gst_pad_set_event_function (fakeadec->sinkpad,
      GST_DEBUG_FUNCPTR (gst_fakeadec_sink_event));
  gst_element_add_pad (GST_ELEMENT (fakeadec), fakeadec->sinkpad);

  fakeadec->srcpad =
      gst_pad_new_from_static_template (&gst_fakeadec_src_pad_template, "src");
  gst_element_add_pad (GST_ELEMENT (fakeadec), fakeadec->srcpad);

  fakeadec->active_mode = FALSE;        /* whether the fakeadec is activated pad or not */
  fakeadec->need_gap = FALSE;   /* whether the fakeadec needs to send a gap event or not */
}

static gboolean
gst_fakeadec_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstFakeAdec *fakeadec;
  gboolean res;

  fakeadec = GST_FAKEADEC (parent);

  GST_DEBUG_OBJECT (pad, "got event %" GST_PTR_FORMAT, event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      if (!gst_pad_has_current_caps (fakeadec->srcpad)) {
        GstCaps *caps;
        gst_event_parse_caps (event, &caps);
        gst_fakeadec_set_caps (fakeadec, caps);
      }
      gst_event_unref (event);
      res = TRUE;
      break;
    }
    case GST_EVENT_SEGMENT:
    {
      GST_LOG_OBJECT (fakeadec, "Save segment event");
      gst_event_copy_segment (event, &fakeadec->segment);
      res = gst_pad_event_default (pad, parent, event);
      fakeadec->need_gap = TRUE;
      break;
    }
    default:
      res = gst_pad_event_default (pad, parent, event);
      break;
  }

  return res;
}

static void
gst_fakeadec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstFakeAdec *fakeadec = GST_FAKEADEC (object);

  switch (prop_id) {
    case PROP_ACTIVE_MODE:
      GST_DEBUG_OBJECT (fakeadec, "Set active mode");
      fakeadec->active_mode = g_value_get_boolean (value);
      fakeadec->need_gap = TRUE;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstFlowReturn
gst_fakeadec_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  GstFakeAdec *fakeadec;
  GstFlowReturn ret = GST_FLOW_OK;

  fakeadec = GST_FAKEADEC (parent);

  GST_DEBUG_OBJECT (pad, "got buffer %" GST_PTR_FORMAT, buffer);

  if (fakeadec->active_mode) {
    if (fakeadec->need_gap) {
      GstEvent *gap =
          gst_event_new_gap (fakeadec->segment.start, fakeadec->segment.stop);
      GST_DEBUG_OBJECT (fakeadec,
          "push gap event with time :%" GST_TIME_FORMAT " duration: %"
          GST_TIME_FORMAT, GST_TIME_ARGS (fakeadec->segment.start),
          GST_TIME_ARGS (fakeadec->segment.stop));
      if (gst_pad_push_event (fakeadec->srcpad, gap))
        fakeadec->need_gap = FALSE;
    }
    gst_buffer_unref (buffer);
  } else {
    buffer = gst_buffer_make_writable (buffer);
    GST_BUFFER_FLAG_SET (buffer, GST_BUFFER_FLAG_CORRUPTED);
    GST_DEBUG_OBJECT (fakeadec, "Send packet");
    ret = gst_pad_push (fakeadec->srcpad, buffer);
  }

  return ret;
}
