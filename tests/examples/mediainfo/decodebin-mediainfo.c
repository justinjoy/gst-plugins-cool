#include <gst/gst.h>

#define SUPPORTED_CAPS "\
video/x-raw(ANY); audio/x-raw(ANY); \
subtitle/x-media; \
"

static gboolean
_print_media_field (GQuark field_id, const GValue * value, gpointer user_data)
{
  gchar *value_str = gst_value_serialize (value);

  g_message ("field [%s:%s]", g_quark_to_string (field_id), value_str);
  g_free (value_str);

  return TRUE;
}

static gboolean
my_bus_callback (GstBus * bus, GstMessage * message, gpointer data)
{
  GMainLoop *loop = data;

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_APPLICATION:{
      if (gst_message_has_name (message, "media-info")) {
        /* it's our message */
        const GstStructure *s;

        s = gst_message_get_structure (message);
        g_message ("posted : %" GST_PTR_FORMAT, s);
        gst_structure_foreach (s, _print_media_field, NULL);
      }
      break;
    }
    case GST_MESSAGE_ERROR:{
      GError *err;
      gchar *debug;

      gst_message_parse_error (message, &err, &debug);
      g_print ("Error: %s\n", err->message);
      g_error_free (err);
      g_free (debug);

      g_main_loop_quit (loop);
      break;
    }
    case GST_MESSAGE_EOS:
      /* end-of-stream */
      g_main_loop_quit (loop);
      break;
    default:
      /* unhandled message */
      break;
  }

  /* remove message from the queue */
  return TRUE;
}

GstElement *pipeline, *audio;

static void
element_added_cb (GstElement * playbin, GstElement * element, gpointer u_data)
{
  GstElementFactory *factory = NULL;
  const gchar *klass = NULL;
  gchar *elem_name = NULL;

  factory = gst_element_get_factory (element);
  klass =
      gst_element_factory_get_metadata (factory, GST_ELEMENT_METADATA_KLASS);
  elem_name = gst_element_get_name (element);
  g_message ("added element : %s", elem_name);

  if (g_strrstr (klass, "Bin")) {
    g_signal_connect (element, "element-added", G_CALLBACK (element_added_cb),
        playbin);
  }
}

static void
cb_newpad (GstElement * decodebin, GstPad * pad, gpointer data)
{
  GstElement *sink;
  GstPad *sinkpad;

  g_message ("pad-added");

  sink = gst_element_factory_make ("fakesink", NULL);
  gst_bin_add (GST_BIN (pipeline), sink);

  sinkpad = gst_element_get_static_pad (sink, "sink");
  gst_pad_link (pad, sinkpad);
  gst_object_unref (sinkpad);

  gst_element_set_state (sink, GST_STATE_PLAYING);
}

static void
cb_no_more_pads (GstElement * element, gpointer user_data)
{
  g_message ("no-more-pads");
}

gint
main (gint argc, gchar * argv[])
{
  GMainLoop *loop;
  GstElement *src, *dec;
  GstBus *bus;
  GstCaps *caps;
  GstPluginFeature *feature;

  /* init GStreamer */
  gst_init (&argc, &argv);
  loop = g_main_loop_new (NULL, FALSE);

  /* make sure we have input */
  if (argc != 2) {
    g_print ("Usage: %s <filename>\n", argv[0]);
    return -1;
  }

  feature = gst_registry_find_feature (gst_registry_get (),
      "mediainfo", GST_TYPE_ELEMENT_FACTORY);
  gst_plugin_feature_set_rank (feature, GST_RANK_PRIMARY + 100);

  /* setup */
  pipeline = gst_pipeline_new ("pipeline");

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_watch (bus, my_bus_callback, loop);
  gst_object_unref (bus);

  src = gst_element_factory_make ("filesrc", "source");
  g_object_set (G_OBJECT (src), "location", argv[1], NULL);
  dec = gst_element_factory_make ("decodebin", "decoder");
  caps = gst_caps_from_string (SUPPORTED_CAPS);
  g_object_set (G_OBJECT (dec), "caps", caps, NULL);
  gst_caps_unref (caps);
  g_signal_connect (dec, "pad-added", G_CALLBACK (cb_newpad), NULL);
  g_signal_connect (dec, "no-more-pads", G_CALLBACK (cb_no_more_pads), loop);
  g_signal_connect (dec, "element-added", G_CALLBACK (element_added_cb), loop);
  gst_bin_add_many (GST_BIN (pipeline), src, dec, NULL);
  gst_element_link (src, dec);

  /* run */
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  g_main_loop_run (loop);

  /* cleanup */
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (pipeline));

  /* don't want to interfere with any other of the other tests */
  gst_plugin_feature_set_rank (feature, GST_RANK_NONE);
  gst_object_unref (feature);

  return 0;
}
