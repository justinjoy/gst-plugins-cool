/* GStreamer Plugins Cool
 * Copyright (C) 2014 LG Electronics, Inc.
 *    Author : Wonchul Lee <wonchul86.lee@lge.com>
 *             HoonHee Lee <hoonhee.lee@lge.com>
 *             Jeongseok Kim <jeongseok.kim@lge.com>
 *             Myoungsun Lee <mysunny.lee@lge.com>
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

#include "gstdecproxy2.h"

GST_DEBUG_CATEGORY_STATIC (decproxy_debug);
#define GST_CAT_DEFAULT decproxy_debug
#define ABS(x)    ((x) < 0 ? -(x) : (x))

static GstStaticPadTemplate decproxy_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink", GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (DECODE_VIDEO_CAPS ";" DECODE_AUDIO_CAPS));

static GstStaticPadTemplate decproxy_src_template =
    GST_STATIC_PAD_TEMPLATE ("src", GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw;audio/x-media;video/x-raw"));

#define gst_decproxy_parent_class parent_class
G_DEFINE_TYPE (GstDecProxy, gst_decproxy, GST_TYPE_BIN);

static void gst_decproxy_dispose (GObject * object);
static void gst_decproxy_finalize (GObject * object);

static gboolean gst_decproxy_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
static gboolean gst_decproxy_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event);

static void replace_decoder (GstDecProxy * decproxy, gboolean active);

static GstPadProbeReturn analyze_new_caps (GstPad * pad, GstPadProbeInfo * info,
    gpointer user_data);

static void
gst_decproxy_class_init (GstDecProxyClass * klass)
{
  GObjectClass *gobject_klass = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (decproxy_debug, "decproxy", 0, "Decoder Proxy Bin");

  parent_class = g_type_class_peek_parent (klass);

  gobject_klass->dispose = GST_DEBUG_FUNCPTR (gst_decproxy_dispose);
  gobject_klass->finalize = GST_DEBUG_FUNCPTR (gst_decproxy_finalize);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&decproxy_sink_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&decproxy_src_template));

  gst_element_class_set_static_metadata (element_class,
      "Proxy for Decoders", "Codec/Decoder/Bin",
      "Acutal decoder deployment controller by resource permissions",
      "Wonchul Lee <wonchul86.lee@lge.com>, HoonHee Lee <hoonhee.lee@lge.com>");
}

static void
gst_decproxy_init (GstDecProxy * decproxy)
{
  GstPad *pad;
  GstPad *gpad;
  GstPadTemplate *pad_tmpl;

  g_rec_mutex_init (&decproxy->lock);

  decproxy->type = GST_COOL_STREAM_TYPE_UNKNOWN;

  decproxy->decoder = NULL;
  decproxy->puppet = NULL;
  decproxy->state = GST_DECPROXY_STATE_UNKNOWN;
  decproxy->pending_switch_decoder = FALSE;

  decproxy->front = gst_element_factory_make ("identity", NULL);
  decproxy->back = gst_element_factory_make ("identity", NULL);

  if (!gst_bin_add (GST_BIN (decproxy), decproxy->front)) {
    g_warning ("Could not add front identity element, decproxy will not work");
    gst_object_unref (decproxy->front);
    decproxy->front = NULL;
  }

  if (!gst_bin_add (GST_BIN (decproxy), decproxy->back)) {
    g_warning ("Could not add back identity element, decproxy will not work");
    gst_object_unref (decproxy->back);
    decproxy->back = NULL;
  }

  pad = gst_element_get_static_pad (decproxy->front, "sink");
  pad_tmpl = gst_static_pad_template_get (&decproxy_sink_template);

  GST_DEBUG_OBJECT (pad, "Trying to connect front identity sink pad");

  gpad = gst_ghost_pad_new_from_template ("sink", pad, pad_tmpl);
  gst_pad_set_active (gpad, TRUE);
  gst_element_add_pad (GST_ELEMENT (decproxy), gpad);

  gst_pad_set_event_function (GST_PAD_CAST (gpad),
      GST_DEBUG_FUNCPTR (gst_decproxy_sink_event));

  gst_object_unref (pad_tmpl);
  gst_object_unref (pad);

  pad = gst_element_get_static_pad (decproxy->front, "src");
  decproxy->blocked_id =
      gst_pad_add_probe (pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
      analyze_new_caps, decproxy, NULL);
  gst_object_unref (pad);

  pad = gst_element_get_static_pad (decproxy->back, "src");
  pad_tmpl = gst_static_pad_template_get (&decproxy_src_template);

  GST_DEBUG_OBJECT (pad, "Trying to connect back identity src pad");

  gpad = gst_ghost_pad_new_from_template ("src", pad, pad_tmpl);
  gst_pad_set_active (gpad, TRUE);
  gst_element_add_pad (GST_ELEMENT (decproxy), gpad);

  gst_pad_set_event_function (GST_PAD_CAST (gpad),
      GST_DEBUG_FUNCPTR (gst_decproxy_src_event));
  gst_object_unref (pad_tmpl);
  gst_object_unref (pad);
}

static void
gst_decproxy_dispose (GObject * object)
{
  GstDecProxy *decproxy = GST_DECPROXY (object);

  if (decproxy->back != NULL) {
    GST_DEBUG_OBJECT (decproxy->back, "Trying to remove back identity");
    gst_element_set_state (decproxy->back, GST_STATE_NULL);
    gst_bin_remove (GST_BIN_CAST (decproxy), decproxy->back);
    decproxy->back = NULL;
  }

  if (decproxy->state == GST_DECPROXY_STATE_PUPPET) {
    if (decproxy->puppet != NULL) {
      GST_DEBUG_OBJECT (decproxy->back, "Trying to remove puppet");
      gst_element_set_state (decproxy->puppet, GST_STATE_NULL);
      gst_bin_remove (GST_BIN_CAST (decproxy), decproxy->puppet);
      decproxy->puppet = NULL;
    }
  } else {
    if (decproxy->decoder != NULL) {
      GST_DEBUG_OBJECT (decproxy->back, "Trying to remove decoder");
      gst_element_set_state (decproxy->decoder, GST_STATE_NULL);
      gst_bin_remove (GST_BIN_CAST (decproxy), decproxy->decoder);
      decproxy->decoder = NULL;
    }
  }

  if (decproxy->front != NULL) {
    GST_DEBUG_OBJECT (decproxy->back, "Trying to remove front identity");
    gst_element_set_state (decproxy->front, GST_STATE_NULL);
    gst_bin_remove (GST_BIN_CAST (decproxy), decproxy->front);
    decproxy->front = NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_decproxy_finalize (GObject * object)
{
  GstDecProxy *decproxy = GST_DECPROXY (object);

  g_rec_mutex_clear (&decproxy->lock);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gst_decproxy_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstDecProxy *decproxy = GST_DECPROXY (parent);
  gboolean ret = TRUE;

  GST_DEBUG_OBJECT (pad, "got event %" GST_PTR_FORMAT, event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstMessage *message = NULL;
      GstCaps *caps = NULL;
      GstStructure *media_info = NULL;
      gchar *stream_id;

      gst_event_parse_caps (event, &caps);

      GST_DEBUG_OBJECT (decproxy, "recieved caps %" GST_PTR_FORMAT, caps);
      GST_DECPROXY_LOCK (decproxy);

      stream_id = gst_pad_get_stream_id (pad);
      media_info = gst_cool_caps_to_info (caps, stream_id);

      /* store type of media */
      gst_structure_get_int (media_info, "type", &decproxy->type);

      /* post media-info */
      message =
          gst_message_new_custom (GST_MESSAGE_APPLICATION,
          GST_OBJECT (decproxy), media_info);
      gst_element_post_message (GST_ELEMENT_CAST (decproxy), message);

      GST_DECPROXY_UNLOCK (decproxy);

      g_free (stream_id);
      ret = gst_pad_event_default (pad, parent, event);
      break;
    }
    case GST_EVENT_TAG:
    {
      GstTagList *tags;
      GstMessage *message = NULL;
      GstCaps *caps = NULL;
      GstStructure *media_info = NULL;
      gchar *stream_id;
      const gchar *mime_type;

      gst_event_parse_tag (event, &tags);
      GST_DEBUG_OBJECT (pad, "got taglist %" GST_PTR_FORMAT, tags);

      GST_DECPROXY_LOCK (decproxy);
      caps = gst_pad_get_current_caps (pad);
      mime_type = gst_structure_get_name (gst_caps_get_structure (caps, 0));

      stream_id = gst_pad_get_stream_id (pad);
      media_info = gst_cool_taglist_to_info (tags, stream_id, mime_type);

      /* store type of media */
      gst_structure_get_int (media_info, "type", &decproxy->type);

      /* post media-info */
      message =
          gst_message_new_custom (GST_MESSAGE_APPLICATION,
          GST_OBJECT (decproxy), media_info);
      gst_element_post_message (GST_ELEMENT_CAST (decproxy), message);

      GST_DECPROXY_UNLOCK (decproxy);

      gst_caps_unref (caps);
      g_free (stream_id);
      ret = gst_pad_event_default (pad, parent, event);
      break;
    }
    case GST_EVENT_SEGMENT:
    {
      if (decproxy->puppet) {
        GstSegment segment;
        gst_event_copy_segment (event, &segment);
        if (ABS (segment.rate - 1.0) > 1.0) {
          g_object_set (decproxy->puppet, "active-mode", TRUE, NULL);
          GST_DEBUG_OBJECT (decproxy, "Set active-mode for trick play");
        } else
          g_object_set (decproxy->puppet, "active-mode", FALSE, NULL);
      }
      ret = gst_pad_event_default (pad, parent, event);
      break;
    }
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }

  return ret;
}

static void
gst_decproxy_switch_decoder (GstDecProxy * decproxy, gboolean active)
{
  GstPad *front_sinkpad = NULL;
  gulong probe_front = 0;

  /* FIXME : To prevent event loss on front element */
  front_sinkpad = gst_element_get_static_pad (decproxy->front, "sink");
  probe_front =
      gst_pad_add_probe (front_sinkpad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
      NULL, NULL, NULL);

  GST_DEBUG_OBJECT (front_sinkpad, "Add pad block");

  if (decproxy->blocked_id) {
    GstPad *front_srcpad;

    front_srcpad = gst_element_get_static_pad (decproxy->front, "src");
    GST_DEBUG_OBJECT (front_srcpad, "Remove pad block");

    gst_pad_remove_probe (front_srcpad, decproxy->blocked_id);
    decproxy->blocked_id = 0;
    gst_object_unref (front_srcpad);
  }

  GST_DECPROXY_LOCK (decproxy);

  if (active && decproxy->state == GST_DECPROXY_STATE_PUPPET) {
    GST_DEBUG_OBJECT (decproxy, "Trying to remove puppet");
    replace_decoder (decproxy, active);
  } else if (!active && decproxy->state == GST_DECPROXY_STATE_DECODER) {
    GST_DEBUG_OBJECT (decproxy, "Trying to remove decoder");
    replace_decoder (decproxy, active);
  }

  GST_DECPROXY_UNLOCK (decproxy);

  /* FIXME : To prevent event loss on front element */
  GST_DEBUG_OBJECT (front_sinkpad, "Remove pad block");
  gst_pad_remove_probe (front_sinkpad, probe_front);
  gst_object_unref (front_sinkpad);
}

static gboolean
gst_decproxy_src_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstDecProxy *decproxy = GST_DECPROXY (parent);
  gboolean ret = TRUE;

  GST_DEBUG_OBJECT (decproxy, "event : %s", GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CUSTOM_UPSTREAM:
    {
      const GstStructure *s;
      gboolean active = FALSE;

      if (!gst_event_has_name (event, "acquired-resource")) {
        GST_DEBUG_OBJECT (event, "Unknown custom event");
        ret = gst_pad_event_default (pad, parent, event);
        break;
      }

      s = gst_event_get_structure (event);
      gst_structure_get_boolean (s, "active", &active);

      /* store resource info for set on decoder */
      if (!decproxy->resource_info) {
        decproxy->resource_info = gst_structure_copy (s);
      } else if (decproxy->resource_info) {
        gst_structure_set (decproxy->resource_info, "active", G_TYPE_BOOLEAN,
            active, NULL);
      }

      GST_INFO_OBJECT (decproxy, "got resource-info : %" GST_PTR_FORMAT,
          decproxy->resource_info);

      GST_DECPROXY_LOCK (decproxy);

      /* pending to create fake decoder before doing analyze_new_caps */
      if (decproxy->state == GST_DECPROXY_STATE_UNKNOWN) {
        decproxy->pending_switch_decoder = TRUE;
        GST_DECPROXY_UNLOCK (decproxy);

        GST_INFO_OBJECT (decproxy, "pending switching decoder");
        break;
      }

      GST_DECPROXY_UNLOCK (decproxy);

      gst_decproxy_switch_decoder (decproxy, active);
      break;
    }
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }

  return ret;
}

static GstElementFactory *
gst_decproxy_update_factories_list (GstDecProxy * decproxy)
{
  GstCaps *caps = NULL;
  GstPad *pad = NULL;
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

  decoders = g_list_sort (decoders, gst_plugin_feature_rank_compare_func);

  pad = gst_element_get_static_pad (decproxy->front, "src");
  caps = gst_pad_get_current_caps (pad);

  if (caps == NULL)
    caps = gst_pad_query_caps (pad, NULL);

  if (!(filtered =
          gst_element_factory_list_filter (decoders, caps, GST_PAD_SINK,
              FALSE))) {
    gchar *tmp = gst_caps_to_string (caps);
    GST_WARNING_OBJECT (decproxy, "Cannot find any decoder for caps %s", tmp);
    g_free (tmp);

    goto not_found;
  }

  GST_DEBUG_OBJECT (filtered, "got filtered list %p", filtered);

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

not_found:
  gst_caps_unref (caps);
  gst_object_unref (pad);

fail:
  if (decoders)
    gst_plugin_feature_list_free (decoders);
  if (filtered)
    gst_plugin_feature_list_free (filtered);

  return factory;
}

static GstElement *
find_and_create_decoder (GstDecProxy * decproxy)
{
  GstElement *decoder = NULL;
  GstElementFactory *factory;

  if (decproxy->state == GST_DECPROXY_STATE_PUPPET) {

    if (decproxy->type == GST_COOL_STREAM_TYPE_AUDIO) {
      GST_DEBUG_OBJECT (decproxy, "Fake decoder for audio will be deployed");
      decoder = gst_element_factory_make ("fakeadec", NULL);
    } else if (decproxy->type == GST_COOL_STREAM_TYPE_VIDEO) {
      GST_DEBUG_OBJECT (decproxy, "Fake decoder for video will be deployed");
      decoder = gst_element_factory_make ("fakevdec", NULL);
    }

    return decoder;
  }

  GST_DEBUG_OBJECT (decproxy, "Actual Decoder will be deployed");

  if (!(factory = gst_decproxy_update_factories_list (decproxy))) {
    GST_WARNING_OBJECT (decproxy, "Could not find actual decoder element");
    return NULL;
  }

  if (!(decoder = gst_element_factory_create (factory, NULL))) {
    GST_WARNING_OBJECT (decproxy, "Could not create actual decoder element");
    return NULL;
  }
  // FIXME: Do not access configuration directly
  if (g_object_class_find_property (G_OBJECT_GET_CLASS (decoder),
          "input-buffers")) {
    GError *err = NULL;
    GKeyFile *config = gst_cool_get_configuration ();

    gint in_size = g_key_file_get_integer (config, "decode", "in_size", &err);

    if (err) {
      GST_WARNING_OBJECT (decproxy, "Unable to read in_size: %s", err->message);
      g_error_free (err);
      err = NULL;
    } else {
      g_object_set (decoder, "input-buffers", in_size, NULL);
      GST_DEBUG_OBJECT (decoder, "decoder in-buffers changed: %d", in_size);

      // TODO: How about output-buffers?
    }
  }

  if (decproxy->resource_info
      && g_object_class_find_property (G_OBJECT_GET_CLASS (decoder),
          "resource-info")) {
    g_object_set (decoder, "resource-info", decproxy->resource_info, NULL);
    GST_DEBUG_OBJECT (decoder,
        "set resource info to decoder, %" GST_PTR_FORMAT,
        decproxy->resource_info);
  }

  return decoder;
}

static GstPadProbeReturn
replace_decoder_stage2_cb (GstPad * pad, GstPadProbeInfo * info,
    gpointer user_data)
{
  GstElement *decoder;

  GstDecProxy *decproxy = GST_DECPROXY (user_data);

  if (GST_EVENT_TYPE (GST_PAD_PROBE_INFO_DATA (info)) != GST_EVENT_EOS)
    return GST_PAD_PROBE_PASS;

  GST_DEBUG_OBJECT (pad, "blocked");

  gst_pad_remove_probe (pad, GST_PAD_PROBE_INFO_ID (info));

  if (decproxy->state == GST_DECPROXY_STATE_PUPPET)
    decoder = decproxy->decoder;
  else
    decoder = decproxy->puppet;

  GST_DEBUG_OBJECT (decproxy, "removing %" GST_PTR_FORMAT, decoder);
  gst_element_set_state (decoder, GST_STATE_NULL);
  gst_bin_remove (GST_BIN (user_data), decoder);
  if (decproxy->state == GST_DECPROXY_STATE_PUPPET)
    decproxy->decoder = NULL;
  else
    decproxy->puppet = NULL;

  if (!(decoder = find_and_create_decoder (decproxy))) {
    GST_INFO_OBJECT (decproxy, "Failed to find proper decoder");
    if (decproxy->type == GST_COOL_STREAM_TYPE_AUDIO) {
      decoder = gst_element_factory_make ("fakeadec", NULL);
      g_object_set (decoder, "active-mode", TRUE, NULL);
      GST_ELEMENT_WARNING (GST_ELEMENT_CAST (decproxy), STREAM, CODEC_NOT_FOUND,
          ("This audio is not supported by decoder"),
          ("This audio is not supported by decoder"));
    } else
      return GST_PAD_PROBE_REMOVE;
  }

  gst_bin_add (GST_BIN_CAST (decproxy), decoder);
  if (!gst_element_link_many (decproxy->front, decoder, decproxy->back, NULL)) {
    GST_ERROR_OBJECT (decproxy, "Cannot make link for all internal elements");
  }

  if (!gst_element_sync_state_with_parent (decoder)) {
    GST_WARNING_OBJECT (decproxy, "Couldn't sync state with parent");
  }

  if (decproxy->state == GST_DECPROXY_STATE_PUPPET)
    decproxy->puppet = decoder;
  else
    decproxy->decoder = decoder;

  return GST_PAD_PROBE_DROP;
}

static GstPadProbeReturn
replace_decoder_stage1_cb (GstPad * pad, GstPadProbeInfo * info,
    gpointer user_data)
{
  GstPad *target_pad;
  GstElement *decoder;
  GstDecProxy *decproxy = GST_DECPROXY (user_data);

  GST_DEBUG_OBJECT (pad, "blocked");

  gst_pad_remove_probe (pad, GST_PAD_PROBE_INFO_ID (info));

  if (decproxy->state == GST_DECPROXY_STATE_PUPPET)
    decoder = decproxy->decoder;
  else
    decoder = decproxy->puppet;

  target_pad = gst_element_get_static_pad (decoder, "src");
  GST_DEBUG_OBJECT (target_pad, "Registered pad block to remove decoder");

  gst_pad_add_probe (target_pad,
      GST_PAD_PROBE_TYPE_BLOCK | GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
      replace_decoder_stage2_cb, user_data, NULL);
  gst_object_unref (target_pad);

  target_pad = gst_element_get_static_pad (decoder, "sink");
  gst_pad_send_event (target_pad, gst_event_new_eos ());
  GST_DEBUG_OBJECT (target_pad, "Sent EOS to remove decoder element");
  gst_object_unref (target_pad);

  return GST_PAD_PROBE_OK;
}

static void
replace_decoder (GstDecProxy * decproxy, gboolean active)
{
  GstPad *pad;

  pad = gst_element_get_static_pad (decproxy->front, "src");

  GST_DEBUG_OBJECT (pad, "Blocking pad to remove decoder element: state = %d",
      active);

  gst_pad_add_probe (pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
      replace_decoder_stage1_cb, decproxy, NULL);

  gst_object_unref (pad);

  if (active)
    decproxy->state = GST_DECPROXY_STATE_DECODER;
  else
    decproxy->state = GST_DECPROXY_STATE_PUPPET;
}

static GstPadProbeReturn
analyze_new_caps (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  GstPad *target_pad = NULL;
  GstCaps *caps = NULL;
  GstStructure *s = NULL;
  gchar *stream_id = NULL;

  GstDecProxy *decproxy = GST_DECPROXY (user_data);

  GST_DEBUG_OBJECT (pad, "got probe %" GST_PTR_FORMAT,
      GST_PAD_PROBE_INFO_EVENT (info));

  if (GST_EVENT_TYPE (GST_PAD_PROBE_INFO_EVENT (info)) ==
      GST_EVENT_STREAM_START) {
    GST_DEBUG_OBJECT (pad, "Passing stream start event");
    return GST_PAD_PROBE_PASS;
  }

  GST_DEBUG_OBJECT (pad, "Trying to make pipeline for decproxy");

  caps = gst_pad_get_current_caps (pad);

  if (caps == NULL)
    caps = gst_pad_query_caps (pad, NULL);

  s = gst_caps_get_structure (caps, 0);
  decproxy->type = gst_cool_find_type (gst_structure_get_name (s));
  gst_caps_unref (caps);

  if (decproxy->state != GST_DECPROXY_STATE_UNKNOWN) {
    GST_DEBUG_OBJECT (pad, "waiting for acquired resource event to unblock");
    return GST_PAD_PROBE_OK;
  }

  decproxy->state = GST_DECPROXY_STATE_PUPPET;
  if (!(decproxy->puppet = find_and_create_decoder (decproxy))) {
    GST_ERROR_OBJECT (decproxy, "Failed to find proper decoder");
    return GST_PAD_PROBE_REMOVE;
  }

  if (!gst_bin_add (GST_BIN (decproxy), decproxy->puppet)) {
    g_warning ("Could not add puppet element, puppet will not work");
    gst_object_unref (decproxy->puppet);
    decproxy->puppet = NULL;
  }

  GST_ERROR_OBJECT (decproxy->puppet, "Trying to connect with fake decoder");
  if (!gst_element_link_many (decproxy->front, decproxy->puppet, decproxy->back,
          NULL)) {
    GST_ERROR_OBJECT (decproxy, "Cannot make link for all internal elements");
  }

  if (!gst_element_sync_state_with_parent (decproxy->puppet)) {
    GST_WARNING_OBJECT (decproxy, "Couldn't sync state with parent");
  }

  target_pad = gst_element_get_static_pad (GST_ELEMENT_CAST (decproxy), "src");

  /* send stream-start event to downsteram to guarantee order of track */
  stream_id = gst_pad_get_stream_id (pad);
  GST_DEBUG_OBJECT (pad, "try to send stream-start event : %s", stream_id);
  gst_pad_push_event (target_pad, gst_event_new_stream_start (stream_id));
  g_free (stream_id);

  /* set up the outcaps in order to finish auto-plugging */
  if (decproxy->type == GST_COOL_STREAM_TYPE_VIDEO)
    gst_pad_set_caps (target_pad, gst_caps_from_string ("video/x-raw"));
  else if (decproxy->type == GST_COOL_STREAM_TYPE_AUDIO)
    gst_pad_set_caps (target_pad, gst_caps_from_string ("audio/x-media"));

  gst_object_unref (target_pad);

  GST_DECPROXY_LOCK (decproxy);
  /* received acquired-resource event before called analyze_new_caps */
  if (decproxy->pending_switch_decoder) {
    gboolean active = FALSE;
    decproxy->pending_switch_decoder = FALSE;
    GST_DECPROXY_UNLOCK (decproxy);

    gst_structure_get_boolean (decproxy->resource_info, "active", &active);
    gst_decproxy_switch_decoder (decproxy, active);

    return GST_PAD_PROBE_OK;
  }

  GST_DECPROXY_UNLOCK (decproxy);
  return GST_PAD_PROBE_OK;
}
