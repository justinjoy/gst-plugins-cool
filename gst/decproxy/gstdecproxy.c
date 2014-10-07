/* GStreamer Plugins Cool
 * Copyright (C) 2014 LG Electronics, Inc.
 *    Author : Wonchul Lee <wonchul86.lee@lge.com>
 *         Myoungsun Lee <mysunny.lee@lge.com>
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

static GstStaticPadTemplate gst_dec_proxy_sink_pad_template =
    GST_STATIC_PAD_TEMPLATE ("sink", GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (DECODE_VIDEO_CAPS ";" DECODE_AUDIO_CAPS));

static GstStaticPadTemplate gst_dec_proxy_src_pad_template =
    GST_STATIC_PAD_TEMPLATE ("src", GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw;audio/x-media;video/x-raw"));

#define gst_dec_proxy_parent_class parent_class
G_DEFINE_TYPE (GstDecProxy, gst_dec_proxy, GST_TYPE_BIN);

static void gst_dec_proxy_dispose (GObject * obj);
static void gst_dec_proxy_finalize (GObject * obj);
static void gst_dec_proxy_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * spec);
static GstStateChangeReturn gst_dec_proxy_change_state (GstElement * element,
    GstStateChange transition);
static gboolean gst_dec_proxy_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
static gboolean gst_dec_proxy_handle_sink_query (GstPad * pad,
    GstObject * parent, GstQuery * query);
static void caps_notify_cb (GstPad * pad, GParamSpec * unused,
    GstDecProxy * decproxy);
static gboolean gst_dec_proxy_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
static gboolean gst_dec_proxy_handle_src_query (GstPad * pad,
    GstObject * parent, GstQuery * query);
static void remove_decoder (GstDecProxy * decproxy);
//static void gst_dec_proxy_class_init (GstDecProxyClass * klass);
//static void gst_dec_proxy_init (GstDecProxy * decproxy,
//    GstDecProxyClass * g_class);

//static GstElementClass *parent_class;


#if 0
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
#endif

enum
{
  PROP_0,
  PROP_ACTUAL_DECODER,
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

  gobject_klass->get_property = gst_dec_proxy_get_property;

  gobject_klass->dispose = gst_dec_proxy_dispose;
  gobject_klass->finalize = gst_dec_proxy_finalize;

  g_object_class_install_property (gobject_klass, PROP_ACTUAL_DECODER,
      g_param_spec_object ("actual-decoder", "Actual Decoder",
          "the actual element to decode",
          GST_TYPE_ELEMENT, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  element_class->change_state = GST_DEBUG_FUNCPTR (gst_dec_proxy_change_state);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_dec_proxy_sink_pad_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_dec_proxy_src_pad_template));

  gst_element_class_set_static_metadata (element_class,
      "Proxy for Decoders", "Codec/Decoder/Bin",
      "Acutal decoder deployment controller by resource permissions",
      "Wonchul Lee <wonchul86.lee@lge.com>, HoonHee Lee <hoonhee.lee@lge.com>");

  GST_DEBUG_CATEGORY_INIT (dec_proxy_debug, "decproxy", 0, "Decoder Proxy Bin");
}

static void
gst_dec_proxy_init (GstDecProxy * decproxy)
{
  GstPadTemplate *sink_pad_template, *src_pad_template;
  GstElementClass *element_class = GST_ELEMENT_GET_CLASS (decproxy);
  GstPad *valve_sinkpad, *valve_srcpad;

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
  gst_pad_set_event_function (GST_PAD_CAST (decproxy->sinkpad),
      GST_DEBUG_FUNCPTR (gst_dec_proxy_sink_event));
  gst_pad_set_query_function (GST_PAD_CAST (decproxy->sinkpad),
      GST_DEBUG_FUNCPTR (gst_dec_proxy_handle_sink_query));

  gst_element_add_pad (GST_ELEMENT (decproxy), decproxy->sinkpad);

  /* get srcpad template */
  src_pad_template = gst_element_class_get_pad_template (element_class, "src");

  if (src_pad_template) {
    decproxy->srcpad =
        gst_ghost_pad_new_no_target_from_template ("src", src_pad_template);
  } else {
    g_warning ("Subclass didn't specify a src pad template");
    g_assert_not_reached ();
  }

  gst_pad_use_fixed_caps (decproxy->srcpad);
  gst_pad_set_active (decproxy->srcpad, TRUE);

  gst_pad_set_event_function (GST_PAD_CAST (decproxy->srcpad),
      GST_DEBUG_FUNCPTR (gst_dec_proxy_src_event));
  gst_pad_set_query_function (GST_PAD_CAST (decproxy->srcpad),
      GST_DEBUG_FUNCPTR (gst_dec_proxy_handle_src_query));

  gst_element_add_pad (GST_ELEMENT (decproxy), decproxy->srcpad);

  decproxy->valve_elem = gst_element_factory_make ("valve", "proxy_queue");

  if (!decproxy->valve_elem) {
    g_warning ("Cannot make valve element");
    g_assert_not_reached ();
  }

  /* add queue element to decproxy */
  gst_bin_add (GST_BIN_CAST (decproxy), decproxy->valve_elem);
  valve_srcpad = gst_element_get_static_pad (decproxy->valve_elem, "src");
  valve_sinkpad = gst_element_get_static_pad (decproxy->valve_elem, "sink");

  /* try to target from ghostpad to sinkpad */
  gst_ghost_pad_set_target (GST_GHOST_PAD_CAST (decproxy->sinkpad),
      valve_sinkpad);

  /* try to target from ghostpad to srcpad */
  gst_ghost_pad_set_target (GST_GHOST_PAD_CAST (decproxy->srcpad),
      valve_srcpad);

  gst_element_sync_state_with_parent (decproxy->valve_elem);

  decproxy->block_id = 0;
  decproxy->caps = NULL;
  decproxy->pending_remove_probe = FALSE;
  decproxy->state_flag = GST_STATE_DEC_PROXY_NONE;
  decproxy->resource_info = NULL;

  g_object_unref (valve_srcpad);
  g_object_unref (valve_sinkpad);
  g_mutex_init (&decproxy->lock);
}

static void
gst_dec_proxy_dispose (GObject * object)
{
  GstDecProxy *decproxy;

  decproxy = GST_DEC_PROXY (object);

  if (decproxy->caps)
    gst_caps_unref (decproxy->caps);
  decproxy->caps = NULL;

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_dec_proxy_finalize (GObject * obj)
{
  GstDecProxy *decproxy = GST_DEC_PROXY (obj);

  g_mutex_clear (&decproxy->lock);

  if (decproxy->resource_info) {
    gst_structure_free (decproxy->resource_info);
    decproxy->resource_info = NULL;
  }

  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
posting_media_info_msg (GstDecProxy * decproxy, GstStructure * media_info)
{
  GstMessage *message;

  GST_INFO_OBJECT (decproxy, "posted media-info message: %" GST_PTR_FORMAT,
      media_info);

  message =
      gst_message_new_custom (GST_MESSAGE_APPLICATION, GST_OBJECT (decproxy),
      media_info);
  gst_element_post_message (GST_ELEMENT_CAST (decproxy), message);
}

static GstPadProbeReturn
sinkpad_block_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  GstDecProxy *decproxy = GST_DEC_PROXY (user_data);

  if (decproxy->pending_remove_probe) {
    decproxy->pending_remove_probe = FALSE;
    decproxy->block_id = 0;
    return GST_PAD_PROBE_REMOVE;
  }
  return GST_PAD_PROBE_OK;
}

static gboolean
gst_dec_proxy_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstDecProxy *decproxy = NULL;
  gboolean res = TRUE;

  decproxy = GST_DEC_PROXY (parent);
  GST_DEBUG_OBJECT (decproxy, "event : %s", GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;
      gchar *stream_id;
      GstStructure *s, *media_info;
      const gchar *structure_name;

      gst_event_parse_caps (event, &caps);
      GST_INFO_OBJECT (decproxy, "getting caps of %" GST_PTR_FORMAT, caps);

      GST_DEC_PROXY_LOCK (decproxy);
      if (!decproxy->caps)
        decproxy->caps = gst_caps_ref (caps);
      GST_DEC_PROXY_UNLOCK (decproxy);

      s = gst_caps_get_structure (caps, 0);
      structure_name = gst_structure_get_name (s);

      if (g_str_has_prefix (structure_name, "video")
          || g_str_has_prefix (structure_name, "image")) {
        decproxy->stream_type = STREAM_VIDEO;
      } else if (g_str_has_prefix (structure_name, "audio")) {
        decproxy->stream_type = STREAM_AUDIO;
      } else if (g_str_has_prefix (structure_name, "text/")
          || g_str_has_prefix (structure_name, "application/")
          || g_str_has_prefix (structure_name, "subpicture/")) {
        decproxy->stream_type = STREAM_TEXT;
      }

      /* post media-info */
      stream_id = gst_pad_get_stream_id (pad);
      media_info = gst_cool_caps_to_info (caps, stream_id);
      posting_media_info_msg (decproxy, media_info);

      /* if it is not a first caps event then just post media info */
      if (decproxy->state_flag != GST_STATE_DEC_PROXY_NONE) {
        res = gst_pad_event_default (pad, parent, event);
        g_free (stream_id);
        break;
      }

      gst_pad_push_event (decproxy->srcpad,
          gst_event_new_stream_start (stream_id));
      g_free (stream_id);

      //FIXME: change to better way
      if (decproxy->stream_type == STREAM_VIDEO) {
        gst_pad_set_caps (decproxy->srcpad,
            gst_caps_from_string ("video/x-raw"));
      } else if (decproxy->stream_type == STREAM_AUDIO) {
        gst_pad_set_caps (decproxy->srcpad,
            gst_caps_from_string ("audio/x-media"));

        gst_pad_set_caps (decproxy->srcpad, caps);
      }

      res = gst_pad_event_default (pad, parent, event);

      GST_DEC_PROXY_LOCK (decproxy);
      if (!decproxy->block_id &&
          decproxy->state_flag == GST_STATE_DEC_PROXY_NONE) {
        decproxy->block_id =
            gst_pad_add_probe (pad,
            GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, sinkpad_block_cb, decproxy,
            NULL);
        GST_INFO_OBJECT (pad, "locked pad %ld", decproxy->block_id);
      }
      GST_DEC_PROXY_UNLOCK (decproxy);
      break;
    }
    default:
      res = gst_pad_event_default (pad, parent, event);
      break;
  }

  return res;
}

static gboolean
gst_dec_proxy_handle_sink_query (GstPad * pad, GstObject * parent,
    GstQuery * query)
{
  gboolean res = FALSE;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CAPS:
    {
      gst_query_set_caps_result (query, gst_pad_get_pad_template_caps (pad));

      res = TRUE;
      break;
    }
    default:
      res = gst_pad_query_default (pad, parent, query);
      break;
  }

  return res;
}

gint
dec_proxy_compare_factories_func (gconstpointer p1, gconstpointer p2)
{
  /* sort by rank */
  return gst_plugin_feature_rank_compare_func (p1, p2);
}

static GstElementFactory *
gst_dec_proxy_update_factories_list (GstDecProxy * decproxy)
{
  GList *decoders = NULL;
  GList *filtered = NULL;
  GstElementFactory *factory = NULL;

  decoders =
      gst_element_factory_list_get_elements
      (GST_ELEMENT_FACTORY_TYPE_DECODER, GST_RANK_MARGINAL);

  if (decoders == NULL) {
    GST_WARNING_OBJECT (decproxy, "Cannot find any of decoders");
    goto fail;
  }

  GST_DEBUG_OBJECT (decproxy, "got factory list %p \n", decoders);
  gst_plugin_feature_list_debug (decoders);

  decoders = g_list_sort (decoders, dec_proxy_compare_factories_func);

  filtered =
      gst_element_factory_list_filter (decoders, decproxy->caps, GST_PAD_SINK,
      FALSE);

  GST_DEBUG_OBJECT (filtered, "got filtered list %p", filtered);

  if (filtered == NULL) {
    gchar *tmp = gst_caps_to_string (decproxy->caps);
    GST_WARNING_OBJECT (decproxy, "Cannot find any decoder for caps %s", tmp);
    g_free (tmp);
    goto fail;
  }

  gst_plugin_feature_list_debug (filtered);

  factory = GST_ELEMENT_FACTORY_CAST (g_list_nth_data (filtered, 0));

  /* Note that decproxy can not be a child element in decproxy bin */
  if (factory == gst_element_get_factory (GST_ELEMENT_CAST (decproxy))) {
    if (g_list_length (filtered) > 1)
      factory = GST_ELEMENT_FACTORY_CAST (g_list_nth_data (filtered, 1));
    else
      factory = NULL;
  }

  if (factory == NULL)
    GST_DEBUG_OBJECT (filtered, "factory is null");

fail:
  if (decoders)
    gst_plugin_feature_list_free (decoders);
  if (filtered)
    gst_plugin_feature_list_free (filtered);

  return factory;

}

static gboolean
setup_decoder (GstDecProxy * decproxy)
{
  gboolean ret = TRUE;
  GstPad *srcpad, *sinkpad, *val_srcpad;
  GstElementFactory *factory;

  GST_DEBUG_OBJECT (decproxy, "setup decoder");

  factory = gst_dec_proxy_update_factories_list (decproxy);
  if (!factory) {
    GST_WARNING_OBJECT (decproxy, "Could not find actual decoder element");
    return FALSE;
  }

  /* generate actual decoder element */
  GST_DEBUG_OBJECT (decproxy, "%s will be deployed in decproxy bin",
      gst_plugin_feature_get_name (GST_PLUGIN_FEATURE (factory)));
  if (!(decproxy->dec_elem = gst_element_factory_create (factory, NULL))) {
    GST_WARNING_OBJECT (decproxy, "Could not create a decoder element ");
    return FALSE;
  }
  // FIXME: Do not access configuration directly
  if (g_object_class_find_property (G_OBJECT_GET_CLASS (decproxy->dec_elem),
          "input-buffers")) {
    GError *err = NULL;
    GKeyFile *config = gst_cool_get_configuration ();

    gint in_size = g_key_file_get_integer (config, "decode", "in_size", &err);

    if (err) {
      GST_WARNING_OBJECT (decproxy, "Unable to read in_size: %s", err->message);
      g_error_free (err);
      err = NULL;
    } else {
      g_object_set (decproxy->dec_elem, "input-buffers", in_size, NULL);
      GST_DEBUG_OBJECT (decproxy->dec_elem, "decoder in-buffers changed: %d",
          in_size);

      // TODO: How about output-buffers?
    }
  }

  if (decproxy->resource_info &&
      g_object_class_find_property (G_OBJECT_GET_CLASS (decproxy->dec_elem),
          "resource-info")) {
    g_object_set (decproxy->dec_elem, "resource-info", decproxy->resource_info,
        NULL);
    GST_DEBUG_OBJECT (decproxy->dec_elem,
        "set resource info to decoder, %" GST_PTR_FORMAT,
        decproxy->resource_info);
  } else {
    // FIXME: this is just for legacy sink property interface.
    if (decproxy->stream_type == STREAM_AUDIO) {
      g_object_set (decproxy->dec_elem, "index", decproxy->acquired_port, NULL);
      GST_DEBUG_OBJECT (decproxy->dec_elem, "Auido index set as %d",
          decproxy->acquired_port);
    } else if (decproxy->stream_type == STREAM_VIDEO) {
      if (g_object_class_find_property (G_OBJECT_GET_CLASS (decproxy->dec_elem),
              "vdec-ch")) {
        g_object_set (decproxy->dec_elem, "vdec-ch", decproxy->acquired_port,
            NULL);
        GST_DEBUG_OBJECT (decproxy->dec_elem, "Video channel set as %d",
            decproxy->acquired_port);
      }

      /* set port property for video-decoder in lm15u */
      if (g_object_class_find_property (G_OBJECT_GET_CLASS (decproxy->dec_elem),
              "port")) {
        g_object_set (decproxy->dec_elem, "port", decproxy->acquired_port,
            NULL);
        GST_DEBUG_OBJECT (decproxy->dec_elem, "set port[%d] to video decoder",
            decproxy->acquired_port);
      }
    }
  }

  /* add decoder element to decproxy */
  if (!(gst_bin_add (GST_BIN_CAST (decproxy), decproxy->dec_elem))) {
    GST_WARNING_OBJECT (decproxy, "Couldn't add decoder element to bin");
    return FALSE;
  }

  /* try to get a sinkpad from decoder element */
  if (!(sinkpad = gst_element_get_static_pad (decproxy->dec_elem, "sink"))) {
    GST_WARNING_OBJECT (decproxy, "Element %s doesn't have a sinkpad",
        GST_ELEMENT_NAME (decproxy->dec_elem));
    return FALSE;
  }

  /* try to get a srcpad from decoder element */
  if (!(srcpad = gst_element_get_static_pad (decproxy->dec_elem, "src"))) {
    GST_WARNING_OBJECT (decproxy, "Element %s doesn't have a srcpad",
        GST_ELEMENT_NAME (decproxy->dec_elem));
    return FALSE;
  }

  /* try to get a srcpad from decoder element */
  if (!(val_srcpad = gst_element_get_static_pad (decproxy->valve_elem, "src"))) {
    GST_WARNING_OBJECT (decproxy, "Element %s doesn't have a srcpad",
        GST_ELEMENT_NAME (decproxy->valve_elem));
    return FALSE;
  }

  /* try to target from ghostpad to srcpad */
  gst_ghost_pad_set_target (GST_GHOST_PAD_CAST (decproxy->srcpad), srcpad);

  /* try to target from ghostpad to sinkpad */
  gst_pad_link (val_srcpad, sinkpad);

  if (!gst_element_sync_state_with_parent (decproxy->dec_elem))
    GST_WARNING_OBJECT (decproxy, "Couldn't sync state with parent");

  g_object_unref (srcpad);
  g_object_unref (sinkpad);
  g_object_unref (val_srcpad);
  return ret;
}

static void
remove_decoder (GstDecProxy * decproxy)
{
  GstPad *sinkpad, *val_srcpad;

  if (!(val_srcpad = gst_element_get_static_pad (decproxy->valve_elem, "src"))) {
    GST_WARNING_OBJECT (decproxy, "Element %s doesn't have a srcpad",
        GST_ELEMENT_NAME (decproxy->valve_elem));
    return FALSE;
  }
  if (!(sinkpad = gst_element_get_static_pad (decproxy->dec_elem, "sink"))) {
    GST_WARNING_OBJECT (decproxy, "Element %s doesn't have a sinkpad",
        GST_ELEMENT_NAME (decproxy->dec_elem));
    return FALSE;
  }

  gst_pad_unlink (val_srcpad, sinkpad);

  gst_ghost_pad_set_target (GST_GHOST_PAD_CAST (decproxy->srcpad), val_srcpad);

  if (decproxy->dec_elem) {
    GST_DEBUG_OBJECT (decproxy, "removing old decoder element");
    gst_element_set_state (decproxy->dec_elem, GST_STATE_NULL);

    gst_bin_remove (GST_BIN_CAST (decproxy), decproxy->dec_elem);
    decproxy->dec_elem = NULL;
  }
  g_object_unref (sinkpad);
  g_object_unref (val_srcpad);
}

static void
remove_valve (GstDecProxy * decproxy)
{
  if (decproxy->srcpad)
    gst_ghost_pad_set_target (GST_GHOST_PAD_CAST (decproxy->srcpad), NULL);

  if (decproxy->sinkpad)
    gst_ghost_pad_set_target (GST_GHOST_PAD_CAST (decproxy->sinkpad), NULL);

  if (decproxy->valve_elem) {
    GST_DEBUG_OBJECT (decproxy, "removing valve element");
    gst_element_set_state (decproxy->valve_elem, GST_STATE_NULL);

    gst_bin_remove (GST_BIN_CAST (decproxy), decproxy->valve_elem);
    decproxy->valve_elem = NULL;
  }
}

static void
caps_notify_cb (GstPad * pad, GParamSpec * unused, GstDecProxy * decproxy)
{
  GST_INFO_OBJECT (decproxy, "in caps_notify_cb, active = %d",
      decproxy->active);

  if (!decproxy->active)
    return;

  if (!setup_decoder (decproxy))
    GST_WARNING_OBJECT (decproxy, "Failed to configure decoder element");

  GST_DEC_PROXY_LOCK (decproxy);

  if (decproxy->block_id) {
    gst_pad_remove_probe (decproxy->sinkpad, decproxy->block_id);
    decproxy->block_id = 0;
  }

  GST_DEC_PROXY_UNLOCK (decproxy);

  if (decproxy->notify_caps_id) {
    g_signal_handler_disconnect (decproxy->sinkpad, decproxy->notify_caps_id);
    decproxy->notify_caps_id = 0;
  }
}

/*
  * This is event block probe and remove decoder.
  * Checked EOS event.
  * The decproxy change to deactive, we have to remove decoder element after the EOS came.
  */
static GstPadProbeReturn
deactive_event_probe_cb (GstPad * pad, GstPadProbeInfo * info,
    gpointer user_data)
{
  GstDecProxy *decproxy = GST_DEC_PROXY (user_data);

  if (GST_EVENT_TYPE (GST_PAD_PROBE_INFO_DATA (info)) != GST_EVENT_EOS)
    return GST_PAD_PROBE_OK;
  GST_INFO_OBJECT (decproxy, "deactive_event_probe_cb");
  GST_DEC_PROXY_LOCK (decproxy);
  gst_pad_remove_probe (pad, GST_PAD_PROBE_INFO_ID (info));
  remove_decoder (decproxy);
  GST_DEC_PROXY_UNLOCK (decproxy);
  return GST_PAD_PROBE_DROP;
}

/*
  * If the decproxy active mode is changed active to deactive,
  * then this function call back.
  * Add event probe for setting element state to NULL.
  * For this, we need to push EOS event firstly to delete all data.
  */
static GstPadProbeReturn
multi_block_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  GstDecProxy *decproxy = GST_DEC_PROXY (user_data);
  GstPad *dec_elem_sink, *dec_elem_src;
  GST_INFO_OBJECT (pad, "call multi_block_cb");

  if (!decproxy->dec_elem) {
    GST_WARNING_OBJECT (decproxy, "There is no decoder element");
    return GST_PAD_PROBE_OK;
  }

  GST_DEC_PROXY_LOCK (decproxy);
  dec_elem_sink = gst_element_get_static_pad (decproxy->dec_elem, "sink");
  dec_elem_src = gst_element_get_static_pad (decproxy->dec_elem, "src");

  /* remove the probe first */
  gst_pad_remove_probe (pad, GST_PAD_PROBE_INFO_ID (info));
  gst_pad_add_probe (dec_elem_src,
      GST_PAD_PROBE_TYPE_BLOCK | GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
      deactive_event_probe_cb, decproxy, NULL);
  GST_DEC_PROXY_UNLOCK (decproxy);
  gst_pad_send_event (dec_elem_sink, gst_event_new_eos ());
  gst_object_unref (dec_elem_src);
  gst_object_unref (dec_elem_sink);

  return GST_PAD_PROBE_OK;
}

static gboolean
gst_dec_proxy_src_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  gboolean res = TRUE;
  GstDecProxy *decproxy = GST_DEC_PROXY (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CUSTOM_UPSTREAM:
    {
      if (gst_event_has_name (event, "acquired-resource")) {
        const GstStructure *st = gst_event_get_structure (event);

        gst_structure_get_boolean (st, "active", &decproxy->active);

        // FIXME: this is just for legacy property interface.
        if (decproxy->stream_type == STREAM_AUDIO)
          gst_structure_get_int (st, "audio-port", &decproxy->acquired_port);
        else if (decproxy->stream_type == STREAM_VIDEO)
          gst_structure_get_int (st, "video-port", &decproxy->acquired_port);

        /* store resource info for set on decoder */
        if (!decproxy->resource_info &&
            (gst_structure_has_field (st, "audio-port")
                || gst_structure_has_field (st, "video-port")))
          decproxy->resource_info = gst_structure_copy (st);

        GST_INFO_OBJECT (decproxy,
            "received event : %s, resource-info : %" GST_PTR_FORMAT,
            GST_EVENT_TYPE_NAME (event), st);

        /* acquired-resource event is arrived before receiving caps event
         * on sinkpad
         */
        if (!decproxy->caps) {
          decproxy->notify_caps_id = g_signal_connect (decproxy->sinkpad,
              "notify::caps", G_CALLBACK (caps_notify_cb), decproxy);

          GST_INFO_OBJECT (decproxy,
              "wait caps event, it is not yet reached at sink pad");
          gst_event_unref (event);
          break;
        }

        if (decproxy->active
            && (decproxy->state_flag != GST_STATE_DEC_PROXY_ACTIVE)) {
          GST_INFO_OBJECT (pad, "active pad %s:%s", GST_DEBUG_PAD_NAME (pad));
          GST_DEC_PROXY_LOCK (decproxy);
          if (decproxy->state_flag == GST_STATE_DEC_PROXY_DEACTIVE) {
            /* Pad status changed deactive to active. */
            GST_DEBUG_OBJECT (pad, "this pad switched [deactive] to [active]");
            if (!decproxy->block_id) {
              decproxy->block_id =
                  gst_pad_add_probe (decproxy->sinkpad,
                  GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, NULL, decproxy, NULL);
            }
          }
          if (decproxy->state_flag == GST_STATE_DEC_PROXY_NONE)
            GST_DEBUG_OBJECT (pad, "this pad switched [null] to [active]");
          GST_DEC_PROXY_UNLOCK (decproxy);

          if (!setup_decoder (decproxy))
            goto decoder_element_failed;

          GST_DEC_PROXY_LOCK (decproxy);

          if (!gst_pad_is_blocking (decproxy->sinkpad))
            decproxy->pending_remove_probe = TRUE;

          if (decproxy->block_id) {
            gst_pad_remove_probe (decproxy->sinkpad, decproxy->block_id);
            decproxy->block_id = 0;
          }
          decproxy->state_flag = GST_STATE_DEC_PROXY_ACTIVE;
          GST_DEC_PROXY_UNLOCK (decproxy);
        } else if (!decproxy->active
            && (decproxy->state_flag != GST_STATE_DEC_PROXY_DEACTIVE)) {
          GST_INFO_OBJECT (pad, "deactive pad %s:%s", GST_DEBUG_PAD_NAME (pad));
          GST_DEC_PROXY_LOCK (decproxy);
          if (decproxy->state_flag == GST_STATE_DEC_PROXY_ACTIVE) {
            /* Pad status changed active to deactive. */
            GST_DEBUG_OBJECT (pad, "this pad switched [active] to [deactive]");
            gst_pad_add_probe (decproxy->sinkpad,
                GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, multi_block_cb, decproxy,
                NULL);
          }
          if (decproxy->state_flag == GST_STATE_DEC_PROXY_NONE) {
            GST_DEBUG_OBJECT (pad, "this pad switched [null] to [deactive]");

            if (!gst_pad_is_blocking (decproxy->sinkpad))
              decproxy->pending_remove_probe = TRUE;
            if (decproxy->block_id) {
              gst_pad_remove_probe (decproxy->sinkpad, decproxy->block_id);
              decproxy->block_id = 0;
            }
          }
          decproxy->state_flag = GST_STATE_DEC_PROXY_DEACTIVE;
          GST_DEC_PROXY_UNLOCK (decproxy);
        }
        gst_event_unref (event);
      }
      break;
    }
    default:
      res = gst_pad_event_default (pad, parent, event);
      break;
  }

  return res;

  /* ERRORS */
decoder_element_failed:
  {
    GST_WARNING_OBJECT (decproxy, "Failed to configure decoder element");
    return FALSE;
  }
}

static gboolean
gst_dec_proxy_handle_src_query (GstPad * pad, GstObject * parent,
    GstQuery * query)
{
  gboolean res = FALSE;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CAPS:
    {
      gst_query_set_caps_result (query, gst_pad_get_pad_template_caps (pad));
      res = TRUE;
      break;
    }
    default:
      res = gst_pad_query_default (pad, parent, query);
      break;
  }

  return res;
}

static void
gst_dec_proxy_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstDecProxy *decproxy = GST_DEC_PROXY (object);

  switch (prop_id) {
    case PROP_ACTUAL_DECODER:
      GST_OBJECT_LOCK (decproxy);
      g_value_set_object (value, decproxy->dec_elem);
      GST_OBJECT_UNLOCK (decproxy);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstStateChangeReturn
gst_dec_proxy_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  GstDecProxy *decproxy;

  decproxy = GST_DEC_PROXY (element);

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  if (ret == GST_STATE_CHANGE_FAILURE)
    goto failure;

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      GST_DEBUG_OBJECT (decproxy, "paused to ready");
      remove_decoder (decproxy);
      remove_valve (decproxy);
      break;
    default:
      break;
  }

/* ERRORS */
failure:
  return ret;;
}
