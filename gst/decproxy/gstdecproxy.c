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

static void gst_dec_proxy_dispose (GObject * obj);
static void gst_dec_proxy_finalize (GObject * obj);
static void gst_dec_proxy_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * spec);
static void gst_dec_proxy_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * spec);
static GstStateChangeReturn gst_dec_proxy_change_state (GstElement * element,
    GstStateChange transition);
static gboolean gst_dec_proxy_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
static gboolean gst_dec_proxy_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event);

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

  gobject_klass->set_property = gst_dec_proxy_set_property;
  gobject_klass->get_property = gst_dec_proxy_get_property;

  gobject_klass->dispose = gst_dec_proxy_dispose;
  gobject_klass->finalize = gst_dec_proxy_finalize;

  g_object_class_install_property (gobject_klass, PROP_ACTUAL_DECODER,
      g_param_spec_object ("actual-decoder", "Actual Decoder",
          "the actual element to decode",
          GST_TYPE_ELEMENT, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

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
  gst_pad_set_event_function (GST_PAD_CAST (decproxy->sinkpad),
      GST_DEBUG_FUNCPTR (gst_dec_proxy_sink_event));

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

  gst_pad_set_event_function (GST_PAD_CAST (decproxy->srcpad),
      GST_DEBUG_FUNCPTR (gst_dec_proxy_src_event));

  gst_element_add_pad (GST_ELEMENT (decproxy), decproxy->srcpad);
  gst_object_unref (src_pad_template);

  decproxy->caps = NULL;
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
  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static gboolean
append_media_field (GQuark field_id, const GValue * value, gpointer user_data)
{
  GstStructure *media = user_data;
  gchar *value_str = gst_value_serialize (value);

  GST_DEBUG_OBJECT (media, "field [%s:%s]", g_quark_to_string (field_id),
      value_str);
  gst_structure_id_set_value (media, field_id, value);

  g_free (value_str);

  return TRUE;
}

static void
posting_media_info_msg (GstDecProxy * decproxy, GstCaps * caps,
    gchar * stream_id)
{
  GstStructure *s;
  GstStructure *media_info;
  GstMessage *message;
  const gchar *structure_name;
  gint type = 0;

  GST_DEBUG_OBJECT (decproxy, "getting caps of %" GST_PTR_FORMAT, caps);

  s = gst_caps_get_structure (caps, 0);
  structure_name = gst_structure_get_name (s);

  if (g_str_has_prefix (structure_name, "video")
      || g_str_has_prefix (structure_name, "image")) {
    type = STREAM_VIDEO;
  } else if (g_str_has_prefix (structure_name, "audio")) {
    type = STREAM_AUDIO;
  } else if (g_str_has_prefix (structure_name, "text/")
      || g_str_has_prefix (structure_name, "application/")
      || g_str_has_prefix (structure_name, "subpicture/")) {
    type = STREAM_TEXT;
  }

  media_info =
      gst_structure_new ("media-info", "stream-id", G_TYPE_STRING, stream_id,
      "type", G_TYPE_INT, type, NULL);
  /* append media information from caps's structure to media-info */
  gst_structure_foreach (s, append_media_field, media_info);
  GST_INFO_OBJECT (decproxy, "create new media info : %" GST_PTR_FORMAT,
      media_info);

  message =
      gst_message_new_custom (GST_MESSAGE_APPLICATION, GST_OBJECT (decproxy),
      media_info);
  gst_element_post_message (GST_ELEMENT_CAST (decproxy), message);
  GST_INFO_OBJECT (decproxy, "posted media-info message");
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

      gst_event_parse_caps (event, &caps);
      GST_INFO_OBJECT (decproxy, "getting caps of %" GST_PTR_FORMAT, caps);

      if (!decproxy->caps)
        decproxy->caps = gst_caps_ref (caps);

      /* post media-info */
      stream_id = gst_pad_get_stream_id (pad);
      posting_media_info_msg (decproxy, caps, stream_id);
      g_free (stream_id);

      res = gst_pad_event_default (pad, parent, event);
      break;
    }
    default:
      res = gst_pad_event_default (pad, parent, event);
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
    factory = GST_ELEMENT_FACTORY_CAST (g_list_nth_data (filtered, 1));
  }

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
  GstPad *srcpad, *sinkpad;
  GstElementFactory *factory;

  GST_DEBUG_OBJECT (decproxy, "setup decoder");

  factory = gst_dec_proxy_update_factories_list (decproxy);
  if (!factory) {
    GST_WARNING_OBJECT (decproxy, "Could not find actual decoder element");
    return FALSE;
  }

  /* generate actual decoder element */
  GST_DEBUG_OBJECT (decproxy, "%s wiil be deployed in decproxy bin",
      gst_plugin_feature_get_name (GST_PLUGIN_FEATURE (factory)));
  if (!(decproxy->dec_elem = gst_element_factory_create (factory, NULL))) {
    GST_WARNING_OBJECT (decproxy, "Could not create a decoder element ");
    return FALSE;
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

  /* try to target from ghostpad to sinkpad */
  gst_ghost_pad_set_target (GST_GHOST_PAD_CAST (decproxy->sinkpad), sinkpad);

  /* try to target from ghostpad to srcpad */
  gst_ghost_pad_set_target (GST_GHOST_PAD_CAST (decproxy->srcpad), srcpad);

  if ((gst_element_set_state (decproxy->dec_elem,
              GST_STATE_PAUSED)) == GST_STATE_CHANGE_FAILURE) {
    GST_WARNING_OBJECT (decproxy, "Couldn't set %s to PAUSED",
        GST_ELEMENT_NAME (decproxy->dec_elem));
  }

  g_object_unref (srcpad);
  g_object_unref (sinkpad);

  return ret;
}

static void
remove_decoder (GstDecProxy * decproxy)
{
  if (decproxy->srcpad)
    gst_ghost_pad_set_target (GST_GHOST_PAD_CAST (decproxy->srcpad), NULL);

  if (decproxy->sinkpad)
    gst_ghost_pad_set_target (GST_GHOST_PAD_CAST (decproxy->sinkpad), NULL);

  if (decproxy->dec_elem) {
    GST_DEBUG_OBJECT (decproxy, "removing old decoder element");
    gst_element_set_state (decproxy->dec_elem, GST_STATE_NULL);

    gst_bin_remove (GST_BIN_CAST (decproxy), decproxy->dec_elem);
    decproxy->dec_elem = NULL;
  }
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
        gboolean active = FALSE;
        const GstStructure *st = gst_event_get_structure (event);
        gst_structure_get_boolean (st, "active", &active);
        GST_INFO_OBJECT (decproxy,
            "received event : %s, active : %d",
            GST_EVENT_TYPE_NAME (event), active);

        if (active) {
          if (!setup_decoder (decproxy))
            goto decoder_element_failed;
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
  GstDecProxy *decproxy;

  decproxy = GST_DEC_PROXY (element);

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  if (ret == GST_STATE_CHANGE_FAILURE)
    goto failure;

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      GST_DEBUG_OBJECT (decproxy, "paused to ready");
      remove_decoder (decproxy);
      break;
    default:
      break;
  }

  /* ERRORS */
failure:
  return ret;

}
