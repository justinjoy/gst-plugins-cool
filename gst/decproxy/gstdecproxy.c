/* GStreamer Plugins Cool
 * Copyright (C) 2014 LG Electronics, Inc.
 *	Author : Wonchul Lee <wonchul86.lee@lge.com>
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

#include "gstdecproxy.h"

#include <gst/gst.h>
#include <stdlib.h>
#include <string.h>

GST_DEBUG_CATEGORY_STATIC (dec_proxy_debug);
#define GST_CAT_DEFAULT dec_proxy_debug

#define parent_class gst_dec_proxy_parent_class
G_DEFINE_TYPE (GstDecProxy, gst_dec_proxy, GST_TYPE_BIN);

enum
{
  PROP_0,
  PROP_LAST
};

/* signals */
enum
{
  LAST_SIGNAL
};

//static guint gst_dec_proxy_signals[LAST_SIGNAL] = { 0 };

static void gst_dec_proxy_finalize (GObject * obj);
static void gst_dec_proxy_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * spec);
static void gst_dec_proxy_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * spec);
static GstStateChangeReturn gst_dec_proxy_change_state (GstElement * element,
    GstStateChange transition);

static GstStaticPadTemplate gst_dec_proxy_sink_pad_template =
GST_STATIC_PAD_TEMPLATE ("sink", GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (DECODE_CAPS));

static GstStaticPadTemplate gst_dec_proxy_src_pad_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw; audio/x-raw")
    );

static void
gst_dec_proxy_class_init (GstDecProxyClass * klass)
{
  GObjectClass *gobject_klass;
  GstElementClass *element_class = (GstElementClass *) klass;

  gobject_klass = (GObjectClass *) klass;

  gobject_klass->set_property = gst_dec_proxy_set_property;
  gobject_klass->get_property = gst_dec_proxy_get_property;

  gobject_klass->finalize = gst_dec_proxy_finalize;

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_dec_proxy_src_pad_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_dec_proxy_sink_pad_template));

  gst_element_class_set_static_metadata (element_class, "Proxy Decoder Bin",
      "Codec/Decoder/Bin",
      "Proxy Decoder holds actual decoder inside of it",
      "Wonchul Lee <wonchul86.lee@lge.com>, HoonHee Lee <hoonhee.lee@lge.com>");

  element_class->change_state = GST_DEBUG_FUNCPTR (gst_dec_proxy_change_state);

  GST_DEBUG_CATEGORY_INIT (dec_proxy_debug, "decproxy", 0, "Decoder Proxy Bin");
}

static void
gst_dec_proxy_init (GstDecProxy * decproxy)
{
}

static void
gst_dec_proxy_finalize (GObject * obj)
{
  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
gst_dec_proxy_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  //GstDecProxy *decproxy = GST_DEC_PROXY (object);

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static void
gst_dec_proxy_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  //GstDecProxy *decproxy = GST_DEC_PROXY (object);

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstStateChangeReturn
gst_dec_proxy_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  //GstDecProxy *decproxy;

  //decproxy = GST_DEC_PROXY (element);

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  if (ret == GST_STATE_CHANGE_FAILURE)
    goto failure;

  switch (transition) {
    default:
      break;
  }

  /* ERRORS */
failure:
  return ret;

}
