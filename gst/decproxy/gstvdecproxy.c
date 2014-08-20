/* GStreamer Plugins Cool
 * Copyright (C) 2014 LG Electronics, Inc.
 *	Author : Wonchul Lee <wonchul86.lee@lge.com>
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

#include "gstvdecproxy.h"

GST_DEBUG_CATEGORY_STATIC (gst_vdec_proxy_debug);
#define GST_CAT_DEFAULT gst_vdec_proxy_debug

static GstStaticPadTemplate gst_vdec_proxy_sink_pad_template =
GST_STATIC_PAD_TEMPLATE ("sink", GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (DECODE_VIDEO_CAPS));

static GstStaticPadTemplate gst_vdec_proxy_src_pad_template =
    GST_STATIC_PAD_TEMPLATE ("src", GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw;"));

#define gst_vdec_proxy_parent_class parent_class
G_DEFINE_TYPE (GstVDecProxy, gst_vdec_proxy, GST_TYPE_DEC_PROXY);

static void
gst_vdec_proxy_class_init (GstVDecProxyClass * klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_vdec_proxy_sink_pad_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_vdec_proxy_src_pad_template));

  gst_element_class_set_static_metadata (element_class,
      "Proxy Video Decoder Bin", "Codec/Decoder/Bin",
      "Proxy Video Decoder holds actual decoder inside of it",
      "Wonchul Lee <wonchul86.lee@lge.com>, HoonHee Lee <hoonhee.lee@lge.com>");

  GST_DEBUG_CATEGORY_INIT (gst_vdec_proxy_debug, "vdecproxy", 0,
      "vdecproxy bin");
}

static void
gst_vdec_proxy_init (GstVDecProxy * vdecproxy)
{
}
