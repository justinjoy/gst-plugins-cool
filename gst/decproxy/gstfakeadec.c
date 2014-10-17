/* GStreamer Plugins Cool
 * Copyright (C) 2013-2014 LG Electronics, Inc.
 *	Author : Wonchul Lee <wonchul86.lee@lge.com>
 *	         Jeongseok Kim <jeongseok.kim@lge.com>
 *	         HoonHee Lee <hoonhee.lee@lge.com>
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
#include <gst/audio/audio.h>

#define GST_AUDIO_FORMATS_ALL " { S8, U8, " \
    "S16LE, S16BE, U16LE, U16BE, " \
    "S24_32LE, S24_32BE, U24_32LE, U24_32BE, " \
    "S32LE, S32BE, U32LE, U32BE, " \
    "S24LE, S24BE, U24LE, U24BE, " \
    "S20LE, S20BE, U20LE, U20BE, " \
    "S18LE, S18BE, U18LE, U18BE, " \
    "F32LE, F32BE, F64LE, F64BE }"

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
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " GST_AUDIO_FORMATS_ALL ", "
        "layout = (string) interleaved, "
        "rate = (int) [ 1, MAX ], " "channels = (int) [ 1, MAX ]")
    );

GST_DEBUG_CATEGORY_STATIC (fakeadec_debug);
#define GST_CAT_DEFAULT fakeadec_debug

#define gst_fakeadec_parent_class parent_class
G_DEFINE_TYPE (GstFakeAdec, gst_fakeadec, GST_TYPE_AUDIO_DECODER);

static gboolean gst_fakeadec_start (GstAudioDecoder * dec);
static gboolean gst_fakeadec_stop (GstAudioDecoder * dec);
static gboolean gst_fakeadec_set_format (GstAudioDecoder * dec, GstCaps * caps);
static GstFlowReturn gst_fakeadec_handle_frame (GstAudioDecoder * dec,
    GstBuffer * buffer);

static void
gst_fakeadec_class_init (GstFakeAdecClass * klass)
{
  GstElementClass *gstelement_class;
  GstAudioDecoderClass *base_class;

  gstelement_class = (GstElementClass *) klass;
  base_class = (GstAudioDecoderClass *) klass;

  base_class->start = GST_DEBUG_FUNCPTR (gst_fakeadec_start);
  base_class->stop = GST_DEBUG_FUNCPTR (gst_fakeadec_stop);
  base_class->set_format = GST_DEBUG_FUNCPTR (gst_fakeadec_set_format);
  base_class->handle_frame = GST_DEBUG_FUNCPTR (gst_fakeadec_handle_frame);

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_fakeadec_src_pad_template));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_fakeadec_sink_pad_template));

  gst_element_class_set_static_metadata (gstelement_class, "Fake Audio decoder",
      "Codec/Decoder/Audio",
      "Pass data to backend decoder",
      "Wonchul Lee <wonchul86.lee@lge.com>,Justin Joy <justin.joy.9to5@gmail.com>,HoonHee Lee <hoonhee.lee@lge.com>");

  GST_DEBUG_CATEGORY_INIT (fakeadec_debug, "fakeadec", 0, "Fake audio decoder");
}

static void
gst_fakeadec_init (GstFakeAdec * fakeadec)
{
  fakeadec->src_caps_set = FALSE;
}

static gboolean
gst_fakeadec_start (GstAudioDecoder * dec)
{
  GST_DEBUG_OBJECT (dec, "start");

  return TRUE;
}

static gboolean
gst_fakeadec_stop (GstAudioDecoder * dec)
{
  GST_DEBUG_OBJECT (dec, "stop");

  return TRUE;
}

static gboolean
gst_fakeadec_set_format (GstAudioDecoder * bdec, GstCaps * caps)
{
  GstFakeAdec *dec = GST_FAKEADEC (bdec);
  gboolean ret = TRUE;

  /* try to set output's caps */
  if (!dec->src_caps_set) {
    GstStructure *s;
    GstCaps *output_caps;
    gint rate, channels;

    dec->src_caps_set = TRUE;

    GST_DEBUG_OBJECT (dec, "got caps %" GST_PTR_FORMAT, caps);
    s = gst_caps_get_structure (caps, 0);

    output_caps = gst_caps_new_empty_simple ("audio/x-raw");

    /* set format */
    gst_caps_set_simple (output_caps, "format", G_TYPE_STRING, "F32LE", NULL);
    /* set layout */
    gst_caps_set_simple (output_caps, "layout", G_TYPE_STRING, "interleaved",
        NULL);
    /* set rate */
    gst_structure_get_int (s, "rate", &rate);
    gst_caps_set_simple (output_caps, "rate", G_TYPE_INT, rate, NULL);
    /* set channels */
    gst_structure_get_int (s, "channels", &channels);
    gst_caps_set_simple (output_caps, "channels", G_TYPE_INT, channels, NULL);

    gst_pad_set_caps (bdec->srcpad, output_caps);
    gst_caps_unref (output_caps);
  }

  return ret;
}

static GstFlowReturn
gst_fakeadec_handle_frame (GstAudioDecoder * bdec, GstBuffer * buf)
{
  GstFlowReturn res;
  GstFakeAdec *dec = GST_FAKEADEC (bdec);

  GST_DEBUG_OBJECT (dec, "got frame %" GST_PTR_FORMAT, buf);

  /* no draining etc */
  if (G_UNLIKELY (!buf))
    return GST_FLOW_OK;

  gst_audio_decoder_finish_frame (dec, NULL, 1);
  res = GST_FLOW_OK;

  return res;
}
