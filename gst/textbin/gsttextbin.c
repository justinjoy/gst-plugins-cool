/*
 * GStreamer textbin element
 *
 * Copyright 2014 LG Electronics, Inc.
 *  @author: HoonHee Lee <hoonhee.lee@lge.com>
 *
 * gsttextbin.c: Deserializable element for serialized text-stream and posting text data
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

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "gsttextbin.h"

GST_DEBUG_CATEGORY_STATIC (text_bin_debug);
#define GST_CAT_DEFAULT text_bin_debug

#define parent_class gst_text_bin_parent_class

enum
{
  PROP_0,
  PROP_LAST
};

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static void gst_text_bin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_text_bin_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_text_bin_finalize (GObject * self);
static GstStateChangeReturn gst_text_bin_change_state (GstElement *
    element, GstStateChange transition);

static void setup_child_element (GstTextBin * bin);
static void release_child_element (GstTextBin * bin);
static void pad_added_cb (GstElement * element, GstPad * pad, GstTextBin * bin);

G_DEFINE_TYPE (GstTextBin, gst_text_bin, GST_TYPE_BIN);

static void
gst_text_bin_class_init (GstTextBinClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gstelement_class = GST_ELEMENT_CLASS (klass);

  gobject_class->set_property = gst_text_bin_set_property;
  gobject_class->get_property = gst_text_bin_get_property;
  gobject_class->finalize = gst_text_bin_finalize;

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_template));

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_text_bin_change_state);

  gst_element_class_set_static_metadata (gstelement_class,
      "Text Bin", "Source/Bin",
      "Deserializable element for serialized text-stream and post text data",
      "HoonHee Lee <hoonhee.lee@lge.com>");

  GST_DEBUG_CATEGORY_INIT (text_bin_debug, "textbin", 0, "Text Bin");
}

static void
gst_text_bin_init (GstTextBin * bin)
{
  GstPadTemplate *pad_tmpl;

  g_rec_mutex_init (&bin->lock);

  /* get sink pad template */
  pad_tmpl = gst_static_pad_template_get (&sink_template);
  bin->sinkpad = gst_ghost_pad_new_no_target_from_template ("sink", pad_tmpl);
  gst_pad_set_active (bin->sinkpad, TRUE);

  /* add sink ghost pad */
  gst_element_add_pad (GST_ELEMENT (bin), bin->sinkpad);
  gst_object_unref (pad_tmpl);

  bin->streamiddemux = NULL;
  bin->mq = NULL;
  bin->tsinkbin = NULL;
}

static void
gst_text_bin_finalize (GObject * self)
{
  GstTextBin *bin = GST_TEXT_BIN (self);

  g_rec_mutex_clear (&bin->lock);

  release_child_element (bin);

  G_OBJECT_CLASS (parent_class)->finalize (self);
}

static void
gst_text_bin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  //GstTextBin *bin = GST_TEXT_BIN (object);
  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_text_bin_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  //GstTextBin *bin = GST_TEXT_BIN (object);

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
setup_child_element (GstTextBin * bin)
{
  GstPad *sinkpad = NULL;

  GST_DEBUG_OBJECT (bin, "starts to generate child elements");

  if (bin->streamiddemux) {
    GST_DEBUG_OBJECT (bin, "streamiddemux already exist");
    return;
  }

  /* generate streamiddemux */
  bin->streamiddemux = gst_element_factory_make ("streamiddemux", NULL);
  gst_element_set_state (bin->streamiddemux, GST_STATE_PAUSED);
  gst_bin_add (GST_BIN (bin), bin->streamiddemux);
  GST_DEBUG_OBJECT (bin, "generated streamiddemux");

  /* add signal for pad-added from streamiddemux */
  g_signal_connect (G_OBJECT (bin->streamiddemux), "pad-added",
      G_CALLBACK (pad_added_cb), bin);

  /* try to target from ghost sinkpad to sinkpad of streamiddemux */
  sinkpad = gst_element_get_static_pad (bin->streamiddemux, "sink");
  gst_ghost_pad_set_target (GST_GHOST_PAD_CAST (bin->sinkpad), sinkpad);

  g_object_unref (sinkpad);
}

static void
release_child_element (GstTextBin * bin)
{
  GST_DEBUG_OBJECT (bin, "starts to release child elements");

  if (bin->mq) {
    GstIterator *it;
    GValue data = { 0, };
    /* unlink streamiddemux, multiqueue and tsinkbin */
    GST_DEBUG_OBJECT (bin, "unlink mq and tsinkbin");
    it = gst_element_iterate_pads (bin->mq);
    while (gst_iterator_next (it, &data) == GST_ITERATOR_OK) {
      GstPad *pad = g_value_get_object (&data);
      GstPad *peerpad = gst_pad_get_peer (pad);

      if (peerpad) {
        GST_DEBUG_OBJECT (pad, "unlink to %s", GST_OBJECT_NAME (peerpad));
        if (gst_pad_get_direction (pad) == GST_PAD_SRC) {
          gst_pad_unlink (pad, peerpad);
          gst_element_release_request_pad (bin->tsinkbin, peerpad);
        } else if (gst_pad_get_direction (pad) == GST_PAD_SINK) {
          gst_pad_unlink (peerpad, pad);
          gst_element_release_request_pad (bin->mq, pad);
        }
        gst_object_unref (peerpad);
      }
      g_value_reset (&data);
    }
    g_value_unset (&data);
    gst_iterator_free (it);
  }

  if (bin->tsinkbin) {
    GST_DEBUG_OBJECT (bin, "release tsinkbin element");
    gst_element_set_state (bin->tsinkbin, GST_STATE_NULL);
    gst_bin_remove (GST_BIN_CAST (bin), bin->tsinkbin);
    bin->tsinkbin = NULL;
  }

  if (bin->mq) {
    GST_DEBUG_OBJECT (bin, "release mq element");
    gst_element_set_state (bin->mq, GST_STATE_NULL);
    gst_bin_remove (GST_BIN_CAST (bin), bin->mq);
    bin->mq = NULL;
  }

  if (bin->streamiddemux) {
    GST_DEBUG_OBJECT (bin, "unlink ghostpad and streamiddemux");
    gst_pad_set_active (bin->sinkpad, FALSE);
    gst_ghost_pad_set_target (GST_GHOST_PAD_CAST (bin->sinkpad), NULL);
    gst_element_remove_pad (GST_ELEMENT_CAST (bin), bin->sinkpad);

    GST_DEBUG_OBJECT (bin, "release streamiddemux element");
    gst_element_set_state (bin->streamiddemux, GST_STATE_NULL);
    gst_bin_remove (GST_BIN_CAST (bin), bin->streamiddemux);
    bin->streamiddemux = NULL;
  }
}

static void
pad_added_cb (GstElement * element, GstPad * pad, GstTextBin * bin)
{
  GstPad *mq_sinkpad = NULL;
  GstPad *mq_srcpad = NULL;
  GstPad *tsinkbin_sinkpad = NULL;
  gchar *padname;
  GstIterator *it = NULL;
  gint nb_srcpad = 0;
  GValue item = { 0, };

  GST_DEBUG_OBJECT (pad, "new pad added in streamiddemux");

  GST_TEXT_BIN_LOCK (bin);

  /* generate multiqueue */
  if (!bin->mq) {
    bin->mq = gst_element_factory_make ("multiqueue", NULL);
    /* No limits */
    g_object_set (bin->mq,
        "max-size-bytes", (guint) 0,
        "max-size-buffers", (guint) 0,
        "max-size-time", (guint64) 0,
        "extra-size-bytes", (guint) 0,
        "extra-size-buffers", (guint) 0, "extra-size-time", (guint64) 0, NULL);
    gst_element_set_state (bin->mq, GST_STATE_PAUSED);
    gst_bin_add (GST_BIN (bin), bin->mq);
    GST_DEBUG_OBJECT (bin->mq, "generated multiqueue");
  }

  /* create multiqueue request pad */
  mq_sinkpad = gst_element_get_request_pad (bin->mq, "sink_%u");
  if (!mq_sinkpad) {
    GST_TEXT_BIN_UNLOCK (bin);
    GST_WARNING_OBJECT (bin, "failed to create multiqueue request pad");
    return;
  }

  /* link to srcpad of streamiddemux to sinkpad of mq */
  gst_pad_link_full (pad, mq_sinkpad, GST_PAD_LINK_CHECK_NOTHING);

  /* get number of srcpad in mq */
  it = gst_element_iterate_src_pads (bin->mq);

  while (it && gst_iterator_next (it, &item) == GST_ITERATOR_OK) {
    nb_srcpad++;
    g_value_unset (&item);
  }
  gst_iterator_free (it);
  GST_DEBUG_OBJECT (bin, "current number of srcpad in mq : %d", nb_srcpad);

  /* get srcpad of mq */
  padname = g_strdup_printf ("src_%u", --nb_srcpad);
  mq_srcpad = gst_element_get_static_pad (bin->mq, padname);
  g_free (padname);

  if (!mq_srcpad) {
    GST_TEXT_BIN_UNLOCK (bin);
    GST_WARNING_OBJECT (bin, "failed to get srcpad from mq");
    return;
  }

  /* generate tsinkbin */
  if (!bin->tsinkbin) {
    bin->tsinkbin = gst_element_factory_make ("tsinkbin", NULL);
    gst_element_set_state (bin->tsinkbin, GST_STATE_PAUSED);
    gst_bin_add (GST_BIN_CAST (bin), bin->tsinkbin);
    GST_OBJECT_FLAG_SET (bin->tsinkbin, GST_ELEMENT_FLAG_SINK);
  }

  /* create tsinkbin request pad */
  tsinkbin_sinkpad = gst_element_get_request_pad (bin->tsinkbin, "text_sink%d");

  if (!tsinkbin_sinkpad) {
    GST_TEXT_BIN_UNLOCK (bin);
    GST_WARNING_OBJECT (bin, "failed to create tsinkbin request pad");
    return;
  }

  /* link to srcpad of mq to sinkpad of tsinkbin */
  gst_pad_link_full (mq_srcpad, tsinkbin_sinkpad, GST_PAD_LINK_CHECK_NOTHING);

  g_object_unref (mq_sinkpad);
  g_object_unref (mq_srcpad);
  g_object_unref (tsinkbin_sinkpad);

  GST_DEBUG_OBJECT (bin, "configured %d path", nb_srcpad);

  GST_TEXT_BIN_UNLOCK (bin);
}

static GstStateChangeReturn
gst_text_bin_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstTextBin *bin;

  bin = GST_TEXT_BIN (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      GST_DEBUG ("ready to paused");
      /* try to generate streamiddemux */
      setup_child_element (bin);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      GST_DEBUG ("paused to ready");
      /* release to child elements */
      release_child_element (bin);
      break;
    default:
      break;
  }
  return ret;

}
