/* GStreamer
 *
 * Copyright (C) 2014 HoonHee Lee <hoonhee.lee@lge.com>
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gst/gst.h>
#include <gst/check/gstcheck.h>

/* Fake decoder */
static GType gst_fake_h264_decoder_get_type (void);

#undef parent_class
#define parent_class fake_h264_decoder_parent_class
typedef struct _GstFakeH264Decoder GstFakeH264Decoder;
typedef GstElementClass GstFakeH264DecoderClass;

struct _GstFakeH264Decoder
{
  GstElement parent;
};

G_DEFINE_TYPE (GstFakeH264Decoder, gst_fake_h264_decoder, GST_TYPE_ELEMENT);

static void
gst_fake_h264_decoder_class_init (GstFakeH264DecoderClass * klass)
{
  static GstStaticPadTemplate sink_templ = GST_STATIC_PAD_TEMPLATE ("sink",
      GST_PAD_SINK, GST_PAD_ALWAYS,
      GST_STATIC_CAPS ("video/x-h264, " "stream-format=(string) byte-stream"));
  static GstStaticPadTemplate src_templ = GST_STATIC_PAD_TEMPLATE ("src",
      GST_PAD_SRC, GST_PAD_ALWAYS,
      GST_STATIC_CAPS ("video/x-raw"));
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_templ));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_templ));
  gst_element_class_set_metadata (element_class,
      "FakeH264Decoder", "Codec/Decoder/Video", "yep", "me");
}

static GstFlowReturn
gst_fake_h264_decoder_sink_chain (GstPad * pad, GstObject * parent,
    GstBuffer * buf)
{
  GstElement *self = GST_ELEMENT (parent);
  GstPad *otherpad = gst_element_get_static_pad (self, "src");
  GstFlowReturn ret = GST_FLOW_OK;

  buf = gst_buffer_make_writable (buf);

  ret = gst_pad_push (otherpad, buf);

  gst_object_unref (otherpad);

  return ret;
}

static void
gst_fake_h264_decoder_init (GstFakeH264Decoder * self)
{
  GstPad *pad;

  pad =
      gst_pad_new_from_template (gst_element_class_get_pad_template
      (GST_ELEMENT_GET_CLASS (self), "sink"), "sink");
  gst_pad_set_chain_function (pad, gst_fake_h264_decoder_sink_chain);
  gst_element_add_pad (GST_ELEMENT (self), pad);

  pad =
      gst_pad_new_from_template (gst_element_class_get_pad_template
      (GST_ELEMENT_GET_CLASS (self), "src"), "src");
  gst_element_add_pad (GST_ELEMENT (self), pad);
}

static gboolean
compare_media_field (GQuark field_id, const GValue * value, gpointer user_data)
{
  const GstStructure *s2;
  gchar *value_str1, *value_str2;

  /* No need to compare stream-id and type */
  if ((g_strcmp0 (g_quark_to_string (field_id), "stream-id") == 0)
      || (g_strcmp0 (g_quark_to_string (field_id), "type") == 0)
      || (g_strcmp0 (g_quark_to_string (field_id), "mime-type") == 0))
    return TRUE;

  s2 = user_data;
  value_str1 = gst_value_serialize (value);
  value_str2 = gst_value_serialize (gst_structure_id_get_value (s2, field_id));

  if (g_strcmp0 (value_str1, value_str2) != 0)
    return FALSE;

  g_free (value_str1);
  g_free (value_str2);

  return TRUE;
}

/*
 * When caps event is coming, decproxy should trans caps's information to
 * media-info application message and post.
 * It should be confirmed by bus.
 */
GST_START_TEST (test_media_info)
{
  GstElement *decproxy;
  GstCaps *caps_list[3];
  GstBus *bus;
  GstMessage *message;
  gint i;
  GstStructure *s;

  gst_element_register (NULL, "fakeh264dec", GST_RANK_PRIMARY + 101,
      gst_fake_h264_decoder_get_type ());

  decproxy = gst_element_factory_make ("decproxy", "decproxy");
  fail_unless (decproxy != NULL, "Failed to create decproxy element");

  bus = gst_bus_new ();
  gst_element_set_bus (decproxy, bus);

  /* setup caps */
  caps_list[0] =
      gst_caps_new_simple ("video/x-h264", "stream-format", G_TYPE_STRING,
      "byte-stream", NULL);
  caps_list[1] =
      gst_caps_new_simple ("video/x-h264", "stream-format", G_TYPE_STRING,
      "avc", NULL);
  caps_list[2] =
      gst_caps_new_simple ("video/x-h264", "stream-format", G_TYPE_STRING,
      "byte-stream", "alignment", G_TYPE_STRING, "nal", NULL);

  fail_unless (gst_element_send_event (decproxy,
          gst_event_new_stream_start ("test")));

  /*
   * pushs caps event 3 times.
   * Then, passed media-info from caps should be confirmed by bus message.
   */
  for (i = 0; i < 3; i++) {
    fail_unless (gst_element_send_event (decproxy,
            gst_event_new_caps (caps_list[i])));

    message = gst_bus_poll (bus, GST_MESSAGE_APPLICATION, 2 * GST_SECOND);
    fail_unless (message != NULL, "Failed to receive media-info msg");
    if (gst_message_has_name (message, "media-info")) {
      const GstStructure *s1;
      GstStructure *s2;

      s1 = gst_message_get_structure (message);
      s2 = gst_caps_get_structure (caps_list[i], 0);
      fail_unless (gst_structure_foreach (s1, compare_media_field, s2),
          "Found not matched value");

      if (i == 0) {
        /* puch active event to generate actual decoder element */
        s = gst_structure_new ("acquired-resource",
            "active", G_TYPE_BOOLEAN, TRUE, NULL);
        fail_if (s == NULL);
        fail_unless (gst_element_send_event (decproxy,
                gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM, s)));
      }
    }
    gst_message_unref (message);


  }

  /* cleanup */
  gst_element_set_bus (decproxy, NULL);
  gst_object_unref (bus);
  for (i = 0; i < 3; i++)
    gst_caps_unref (caps_list[i]);
  gst_object_unref (decproxy);
}

GST_END_TEST;


static GstStaticPadTemplate src_pad_template =
    GST_STATIC_PAD_TEMPLATE ("src", GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-h264;"));

/*
 * When active event is coming, decproxy should deploy a decode element
 * and connect.
 */
GST_START_TEST (test_active_decodable_element)
{
  GstPluginFeature *feature;
  GstElement *decproxy;
  GstElement *dec_elem;
  GstCaps *caps, *caps_sinkpad, *caps_srcpad;
  GstEvent *event;
  GstStructure *s;
  gchar *name;
  GstPad *mysrcpad, *sinkpad;
  GstStateChangeReturn state_ret;

  gst_element_register (NULL, "fakeh264dec", GST_RANK_PRIMARY + 99,
      gst_fake_h264_decoder_get_type ());

  feature =
      gst_registry_find_feature (gst_registry_get (), "decproxy",
      GST_TYPE_ELEMENT_FACTORY);
  fail_unless (feature != NULL, "Failed to get feature of decproxy");
  gst_plugin_feature_set_rank (feature, GST_RANK_PRIMARY + 100);

  decproxy = gst_element_factory_make ("decproxy", "decproxy");
  fail_unless (decproxy != NULL, "Failed to create decproxy element");

  /* link srcpad and decproxy */
  mysrcpad = gst_check_setup_src_pad (decproxy, &src_pad_template);
  gst_pad_set_active (mysrcpad, TRUE);

  /* sinkpad of decproxy has a finxed-caps through the padtemplate of srcpad */
  sinkpad = gst_element_get_static_pad (decproxy, "sink");
  caps_sinkpad = gst_pad_get_allowed_caps (sinkpad);
  caps_srcpad = gst_pad_get_pad_template_caps (mysrcpad);
  fail_unless (gst_caps_is_equal (caps_sinkpad, caps_srcpad),
      "Failed to set fixed-caps of sinkpad in decproxy");
  gst_caps_unref (caps_srcpad);
  gst_caps_unref (caps_sinkpad);
  gst_object_unref (sinkpad);

  fail_unless (gst_element_send_event (decproxy,
          gst_event_new_stream_start ("test")));

  /* push caps */
  caps = gst_caps_new_empty_simple ("video/x-h264");
  gst_pad_push_event (mysrcpad, gst_event_new_caps (caps));

  state_ret = gst_element_set_state (decproxy, GST_STATE_PAUSED);
  fail_unless (state_ret != GST_STATE_CHANGE_FAILURE);

  /* puch active event to generate actual decoder element */
  s = gst_structure_new ("acquired-resource",
      "active", G_TYPE_BOOLEAN, TRUE, NULL);
  fail_if (s == NULL);
  event = gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM, s);
  fail_unless (gst_element_send_event (decproxy, event));

  g_object_get (decproxy, "actual-decoder", &dec_elem, NULL);
  fail_unless (dec_elem != NULL, "Failed to get decoder element");
  name = gst_element_get_name (dec_elem);

  fail_unless (g_str_has_prefix (name, "fakeh264dec"),
      "Unexpected decoder element has deployed");

  fail_unless_equals_int (gst_element_set_state (decproxy, GST_STATE_NULL),
      GST_STATE_CHANGE_SUCCESS);

  /* clean up */
  gst_caps_unref (caps);
  g_free (name);
  gst_check_teardown_src_pad (decproxy);
  gst_object_unref (dec_elem);
  gst_object_unref (decproxy);
}

GST_END_TEST;

/*
 * Wnen caps event is coming at the first time, decproxy should block all of
 * data flow. It can be unblocked after 'acquired-resource' event is coming.
 * 
 */
GST_START_TEST (test_block_event_and_data)
{
}

GST_END_TEST;

/*
 * When active event is coming, decproxy should deploy a decode element
 * and connect. Then remove it when deactive event is coming.
 * When status is changed, data flow should be guarantee.
 */
GST_START_TEST (test_transition_active_deactive)
{
}

GST_END_TEST;

/* Fake parser/decoder for parser_negotiation test */
static GType gst_fake_h264_parser_get_type (void);

#undef parent_class
#define parent_class fake_h264_parser_parent_class
typedef struct _GstFakeH264Parser GstFakeH264Parser;
typedef GstElementClass GstFakeH264ParserClass;

struct _GstFakeH264Parser
{
  GstElement parent;
};

G_DEFINE_TYPE (GstFakeH264Parser, gst_fake_h264_parser, GST_TYPE_ELEMENT);

static void
gst_fake_h264_parser_class_init (GstFakeH264ParserClass * klass)
{
  static GstStaticPadTemplate sink_templ = GST_STATIC_PAD_TEMPLATE ("sink",
      GST_PAD_SINK, GST_PAD_ALWAYS,
      GST_STATIC_CAPS ("video/x-h264"));
  static GstStaticPadTemplate src_templ = GST_STATIC_PAD_TEMPLATE ("src",
      GST_PAD_SRC, GST_PAD_ALWAYS,
      GST_STATIC_CAPS ("video/x-h264, "
          "stream-format=(string) { avc, byte-stream }"));
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_templ));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_templ));
  gst_element_class_set_metadata (element_class,
      "FakeH264Parser", "Codec/Parser/Converter/Video", "yep", "me");
}

static gboolean
gst_fake_h264_parser_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstElement *self = GST_ELEMENT (parent);
  GstPad *otherpad = gst_element_get_static_pad (self, "src");
  GstCaps *accepted_caps;
  GstStructure *s;
  const gchar *stream_format;
  gboolean ret = TRUE;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
      accepted_caps = gst_pad_get_allowed_caps (otherpad);
      accepted_caps = gst_caps_truncate (accepted_caps);

      s = gst_caps_get_structure (accepted_caps, 0);
      stream_format = gst_structure_get_string (s, "stream-format");
      if (!stream_format)
        gst_structure_set (s, "stream-format", G_TYPE_STRING, "byte-stream",
            NULL);

      gst_pad_set_caps (otherpad, accepted_caps);
      gst_caps_unref (accepted_caps);
      gst_event_unref (event);
      event = NULL;
      break;
    default:
      break;
  }

  if (event)
    ret = gst_pad_push_event (otherpad, event);
  gst_object_unref (otherpad);

  return ret;
}

static GstFlowReturn
gst_fake_h264_parser_sink_chain (GstPad * pad, GstObject * parent,
    GstBuffer * buf)
{
  GstElement *self = GST_ELEMENT (parent);
  GstPad *otherpad = gst_element_get_static_pad (self, "src");
  GstFlowReturn ret = GST_FLOW_OK;

  buf = gst_buffer_make_writable (buf);

  ret = gst_pad_push (otherpad, buf);

  gst_object_unref (otherpad);

  return ret;
}

static void
gst_fake_h264_parser_init (GstFakeH264Parser * self)
{
  GstPad *pad;

  pad =
      gst_pad_new_from_template (gst_element_class_get_pad_template
      (GST_ELEMENT_GET_CLASS (self), "sink"), "sink");
  gst_pad_set_event_function (pad, gst_fake_h264_parser_sink_event);
  gst_pad_set_chain_function (pad, gst_fake_h264_parser_sink_chain);
  gst_element_add_pad (GST_ELEMENT (self), pad);

  pad =
      gst_pad_new_from_template (gst_element_class_get_pad_template
      (GST_ELEMENT_GET_CLASS (self), "src"), "src");
  gst_element_add_pad (GST_ELEMENT (self), pad);
}

static void
parser_negotiation_pad_added_cb (GstElement * dec, GstPad * pad,
    gpointer user_data)
{
  GstBin *pipe = user_data;
  GstElement *sink;
  GstPad *sinkpad;

  sink = gst_element_factory_make ("fakesink", NULL);
  gst_bin_add (pipe, sink);
  gst_element_sync_state_with_parent (sink);
  sinkpad = gst_element_get_static_pad (sink, "sink");
  gst_pad_link (pad, sinkpad);
  gst_object_unref (sinkpad);
}

static GstElement *vdec = NULL;
static GCond deploy_cond;
static GMutex deploy_mutex;

static void
decodebin_element_added_cb (GstBin * decodebin, GstElement * element,
    gpointer user_data)
{
  gboolean *deployed_decproxy = user_data;

  if (g_strrstr (GST_ELEMENT_NAME (element), "decproxy")) {
    g_mutex_lock (&deploy_mutex);
    vdec = element;
    *deployed_decproxy = TRUE;
    g_cond_signal (&deploy_cond);
    g_mutex_unlock (&deploy_mutex);
  }
}

GST_START_TEST (test_deploy_at_decodebin)
{
  GstStateChangeReturn sret;
  GstMessage *msg;
  GstCaps *caps;
  GstElement *pipe, *src, *filter, *dec;
  gboolean deployed_decproxy = FALSE;
  GstPluginFeature *feature;
  GstStructure *s;
  GstEvent *event;

  g_mutex_init (&deploy_mutex);
  g_cond_init (&deploy_cond);

  gst_element_register (NULL, "fakeh264parse", GST_RANK_PRIMARY + 101,
      gst_fake_h264_parser_get_type ());
  gst_element_register (NULL, "fakeh264dec", GST_RANK_PRIMARY + 99,
      gst_fake_h264_decoder_get_type ());
  feature = gst_registry_find_feature (gst_registry_get (), "decproxy",
      GST_TYPE_ELEMENT_FACTORY);
  gst_plugin_feature_set_rank (feature, GST_RANK_PRIMARY + 100);

  pipe = gst_pipeline_new (NULL);

  src = gst_element_factory_make ("fakesrc", NULL);
  fail_unless (src != NULL);
  g_object_set (G_OBJECT (src), "num-buffers", 5, "sizetype", 2, "filltype", 2,
      "can-activate-pull", FALSE, NULL);

  filter = gst_element_factory_make ("capsfilter", NULL);
  fail_unless (filter != NULL);
  caps = gst_caps_from_string ("video/x-h264");
  g_object_set (G_OBJECT (filter), "caps", caps, NULL);
  gst_caps_unref (caps);

  dec = gst_element_factory_make ("decodebin", NULL);
  fail_unless (dec != NULL);

  g_signal_connect (dec, "pad-added",
      G_CALLBACK (parser_negotiation_pad_added_cb), pipe);
  g_signal_connect (dec, "element-added",
      G_CALLBACK (decodebin_element_added_cb), &deployed_decproxy);

  gst_bin_add_many (GST_BIN (pipe), src, filter, dec, NULL);
  gst_element_link_many (src, filter, dec, NULL);

  sret = gst_element_set_state (pipe, GST_STATE_PLAYING);
  fail_unless_equals_int (sret, GST_STATE_CHANGE_ASYNC);

  g_mutex_lock (&deploy_mutex);
  while (!vdec)
    g_cond_wait (&deploy_cond, &deploy_mutex);
  g_mutex_unlock (&deploy_mutex);

  /* puch active event to generate actual decoder element */
  s = gst_structure_new ("acquired-resource",
      "active", G_TYPE_BOOLEAN, TRUE, NULL);
  fail_if (s == NULL);
  event = gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM, s);
  fail_unless (gst_element_send_event (vdec, event));

  /* wait for EOS or error */
  msg = gst_bus_timed_pop_filtered (GST_ELEMENT_BUS (pipe),
      GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
  fail_unless (msg != NULL);
  fail_unless (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_EOS);
  gst_message_unref (msg);

  gst_element_set_state (pipe, GST_STATE_NULL);
  gst_object_unref (pipe);

  gst_plugin_feature_set_rank (feature, GST_RANK_NONE);
  gst_object_unref (feature);

  g_mutex_clear (&deploy_mutex);
  g_cond_clear (&deploy_cond);
}

GST_END_TEST;

static Suite *
decproxy_suite (void)
{
  Suite *s = suite_create ("decproxy");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);

  tcase_add_test (tc_chain, test_media_info);
  tcase_add_test (tc_chain, test_active_decodable_element);
  tcase_add_test (tc_chain, test_block_event_and_data);
  tcase_add_test (tc_chain, test_transition_active_deactive);
  tcase_add_test (tc_chain, test_deploy_at_decodebin);

  return s;
}

GST_CHECK_MAIN (decproxy);
