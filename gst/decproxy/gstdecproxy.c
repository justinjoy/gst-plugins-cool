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


static void gst_dec_proxy_finalize (GObject * obj);
static void gst_dec_proxy_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * spec);
static void gst_dec_proxy_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * spec);
static GstStateChangeReturn gst_dec_proxy_change_state (GstElement * element,
    GstStateChange transition);
static GstFlowReturn gst_dec_proxy_chain (GstPad * pad, GstObject * parent,
    GstBuffer * buffer);

GST_DEBUG_CATEGORY_STATIC (dec_proxy_debug);
#define GST_CAT_DEFAULT dec_proxy_debug

static void gst_dec_proxy_class_init (GstDecProxyClass * klass);
static void gst_dec_proxy_init (GstDecProxy * decproxy,
    GstDecProxyClass * g_class);

static GstElementClass *parent_class;

/* we can't use G_DEFINE_ABSTRACT_TYPE because we need the klass in the _init
 * method to get to the padtemplates */
GType
gst_dec_proxy_get_type (void)
{
  static volatile gsize dec_proxy_type = 0;

  if (g_once_init_enter (&dec_proxy_type)) {
    GType _type;

    _type = g_type_register_static_simple (GST_TYPE_BIN,
        "GstDecProxy", sizeof (GstDecProxyClass),
        (GClassInitFunc) gst_dec_proxy_class_init, sizeof (GstDecProxy),
        (GInstanceInitFunc) gst_dec_proxy_init, G_TYPE_FLAG_ABSTRACT);

    g_once_init_leave (&dec_proxy_type, _type);
  }
  return dec_proxy_type;
}

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

static void
gst_dec_proxy_class_init (GstDecProxyClass * klass)
{
  GObjectClass *gobject_klass = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_klass->set_property = gst_dec_proxy_set_property;
  gobject_klass->get_property = gst_dec_proxy_get_property;

  gobject_klass->finalize = gst_dec_proxy_finalize;

  element_class->change_state = GST_DEBUG_FUNCPTR (gst_dec_proxy_change_state);

  GST_DEBUG_CATEGORY_INIT (dec_proxy_debug, "decproxy", 0, "Decoder Proxy Bin");
}

static void
gst_dec_proxy_init (GstDecProxy * decproxy, GstDecProxyClass * g_class)
{
  GstPadTemplate *sink_pad_template, *src_pad_template;
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  /* get sinkpad template */
  sink_pad_template =
      gst_element_class_get_pad_template (element_class, "sink");

  if (sink_pad_template) {
    decproxy->sinkpad =
        gst_ghost_pad_new_no_target_from_template ("sink", sink_pad_template);
  } else {
    g_warning ("Subclass didn't specify a src pad template");
    g_assert_not_reached ();
  }

  gst_pad_set_active (decproxy->sinkpad, TRUE);
  gst_pad_set_chain_function (GST_PAD_CAST (decproxy->sinkpad),
      GST_DEBUG_FUNCPTR (gst_dec_proxy_chain));
  gst_element_add_pad (GST_ELEMENT (decproxy), decproxy->sinkpad);
  gst_object_unref (sink_pad_template);

  /* get srcpad template */
  src_pad_template = gst_element_class_get_pad_template (element_class, "src");

  if (src_pad_template) {
    decproxy->srcpad =
        gst_ghost_pad_new_no_target_from_template ("src", src_pad_template);
  } else {
    g_warning ("Subclass didn't specify a src pad template");
    g_assert_not_reached ();
  }

  gst_pad_set_active (decproxy->srcpad, TRUE);
  gst_element_add_pad (GST_ELEMENT (decproxy), decproxy->srcpad);
  gst_object_unref (src_pad_template);
}

static void
gst_dec_proxy_finalize (GObject * obj)
{
  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static GstFlowReturn
gst_dec_proxy_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  GstDecProxy *decproxy = NULL;
  GstMapInfo inmap;
  GstBuffer *outbuf;
  GstFlowReturn ret;

  decproxy = GST_DEC_PROXY (parent);

  GST_LOG_OBJECT (decproxy, "buffer with ts=%" GST_TIME_FORMAT,
      GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buffer)));

  gst_buffer_map (buffer, &inmap, GST_MAP_READ);

  outbuf = gst_buffer_copy (buffer);

  gst_buffer_unmap (buffer, &inmap);
  gst_buffer_unref (buffer);

  ret = gst_pad_push (decproxy->srcpad, outbuf);

  return ret;

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
