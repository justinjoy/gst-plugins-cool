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

#define gst_fakevdec_parent_class parent_class
G_DEFINE_TYPE (GstFakeVdec, gst_fakevdec, GST_TYPE_VIDEO_DECODER);

static void gst_fakevdec_finalize (GObject * object);

/* GstVideoDecoder base class method */
static gboolean gst_fakevdec_open (GstVideoDecoder * decoder);
static gboolean gst_fakevdec_close (GstVideoDecoder * decoder);
static gboolean gst_fakevdec_start (GstVideoDecoder * decoder);
static gboolean gst_fakevdec_stop (GstVideoDecoder * decoder);
static gboolean gst_fakevdec_set_format (GstVideoDecoder * decoder,
    GstVideoCodecState * state);
static gboolean gst_fakevdec_flush (GstVideoDecoder * decoder);
static GstFlowReturn gst_fakevdec_finish (GstVideoDecoder * decoder);
static GstFlowReturn gst_fakevdec_handle_frame (GstVideoDecoder * decoder,
    GstVideoCodecFrame * frame);
static gboolean gst_fakevdec_decide_allocation (GstVideoDecoder * decoder,
    GstQuery * query);

static void
gst_fakevdec_class_init (GstFakeVdecClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstVideoDecoderClass *video_decoder_class = GST_VIDEO_DECODER_CLASS (klass);

  gobject_class->finalize = gst_fakevdec_finalize;

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fakevdec_src_pad_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_fakevdec_sink_pad_template));

  gst_element_class_set_static_metadata (element_class, "Fake Video decoder",
      "Codec/Decoder/Video",
      "Pass data to backend decoder",
      "Wonchul Lee <wonchul86.lee@lge.com>,Jeongseok Kim <Jeongseok Kim@lge.com>,HoonHee Lee <hoonhee.lee@lge.com>");

  video_decoder_class->open = GST_DEBUG_FUNCPTR (gst_fakevdec_open);
  video_decoder_class->close = GST_DEBUG_FUNCPTR (gst_fakevdec_close);
  video_decoder_class->start = GST_DEBUG_FUNCPTR (gst_fakevdec_start);
  video_decoder_class->stop = GST_DEBUG_FUNCPTR (gst_fakevdec_stop);
  video_decoder_class->flush = GST_DEBUG_FUNCPTR (gst_fakevdec_flush);
  video_decoder_class->set_format = GST_DEBUG_FUNCPTR (gst_fakevdec_set_format);
  video_decoder_class->handle_frame =
      GST_DEBUG_FUNCPTR (gst_fakevdec_handle_frame);
  video_decoder_class->finish = GST_DEBUG_FUNCPTR (gst_fakevdec_finish);
  video_decoder_class->decide_allocation =
      GST_DEBUG_FUNCPTR (gst_fakevdec_decide_allocation);

  GST_DEBUG_CATEGORY_INIT (fakevdec_debug, "fakevdec", 0, "Fake video decoder");
}

static void
gst_fakevdec_init (GstFakeVdec * fakevdec)
{
  gst_video_decoder_set_packetized (GST_VIDEO_DECODER (fakevdec), TRUE);
  //gst_video_decoder_set_needs_format (GST_VIDEO_DECODER (fakevdec), TRUE);

  fakevdec->src_caps_set = FALSE;
}

static void
gst_fakevdec_finalize (GObject * object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gst_fakevdec_open (GstVideoDecoder * decoder)
{
  return TRUE;
}

static gboolean
gst_fakevdec_close (GstVideoDecoder * decoder)
{
  return TRUE;
}

static gboolean
gst_fakevdec_set_format (GstVideoDecoder * decoder, GstVideoCodecState * state)
{
  GstFakeVdec *fakevdec = GST_FAKEVDEC (decoder);
  GstCaps *caps = NULL;

  /* try to set caps */
  if (!fakevdec->src_caps_set && state->caps) {
    GstCaps *output_caps;

    fakevdec->src_caps_set = TRUE;

    GST_DEBUG_OBJECT (fakevdec, "got caps %" GST_PTR_FORMAT, state->caps);

    output_caps = gst_caps_new_empty_simple ("video/x-raw");

    gst_pad_set_caps (decoder->srcpad, output_caps);
    gst_caps_unref (output_caps);
  }

  return TRUE;
}

static gboolean
gst_fakevdec_start (GstVideoDecoder * decoder)
{
  return TRUE;
}

static gboolean
gst_fakevdec_stop (GstVideoDecoder * decoder)
{
  return TRUE;
}

static gboolean
gst_fakevdec_flush (GstVideoDecoder * decoder)
{
  return TRUE;
}

static GstFlowReturn
gst_fakevdec_finish (GstVideoDecoder * decoder)
{
  return GST_FLOW_OK;
}

static gboolean
gst_fakevdec_decide_allocation (GstVideoDecoder * decoder, GstQuery * query)
{
  /* Now chain up to the parent class to guarantee that we can
   * get a buffer pool from the query */
  if (!GST_VIDEO_DECODER_CLASS (parent_class)->decide_allocation (decoder,
          query))
    return FALSE;

  return TRUE;
}

static GstFlowReturn
gst_fakevdec_handle_frame (GstVideoDecoder * decoder,
    GstVideoCodecFrame * frame)
{
  //GstFakeVdec *fakevdec = GST_FAKEVDEC (decoder);
  GstBuffer *buf = frame->input_buffer;

  /* no draining etc */
  if (G_UNLIKELY (!buf))
    return GST_FLOW_OK;

  GST_DEBUG_OBJECT (decoder, "got buffer %" GST_PTR_FORMAT, buf);

  gst_video_codec_frame_unref (frame);

  return GST_FLOW_OK;
}
