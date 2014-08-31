#include <gst/gst.h>

static GMainLoop *loop = NULL;

static gboolean
bus_call (GstBus * bus, GstMessage * msg, GstElement * playbin)
{
  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_ASYNC_DONE:{
      g_message ("async done");
      g_usleep (G_USEC_PER_SEC * 3);
      break;
    }
    case GST_MESSAGE_EOS:{
      g_print ("End-of-stream\n");
      g_main_loop_quit (loop);
      break;
    }
    case GST_MESSAGE_ERROR:{
      gchar *debug;
      GError *err;

      gst_message_parse_error (msg, &err, &debug);
      g_printerr ("Debugging info: %s\n", (debug) ? debug : "none");
      g_free (debug);

      g_print ("Error: %s\n", err->message);
      g_error_free (err);

      g_main_loop_quit (loop);

      break;
    }
    case GST_MESSAGE_APPLICATION:{
      const GstStructure *msg_structure = gst_message_get_structure (msg);
      GstStructure *structure;
      GstEvent *event;

      if (gst_structure_has_name (msg_structure, "media-info"))
        GST_INFO ("media info = %" GST_PTR_FORMAT, msg_structure);

      if (!g_strrstr (gst_structure_get_name (msg_structure),
              "request-resource"))
        break;

      structure = gst_structure_new ("acquired-resource",
          "core-type", G_TYPE_STRING, "vp9",
          "video-port", G_TYPE_INT, 1, "audio-port", G_TYPE_INT, 1, NULL);

      event = gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM, structure);
      gst_element_send_event (playbin, event);

      break;
    }
    default:
      break;
  }
  return TRUE;
}

gint
main (gint argc, gchar * argv[])
{
  GstElement *playbin;
  GstBus *bus;
  guint bus_watch_id;
  gchar *uri;
  GstPluginFeature *feature;

  gst_init (&argc, &argv);

  feature = gst_registry_find_feature (gst_registry_get (), "decproxy",
      GST_TYPE_ELEMENT_FACTORY);
  gst_plugin_feature_set_rank (feature, GST_RANK_PRIMARY + 10);

  if (argc < 2) {
    g_print ("usage: %s <media file or uri>\n", argv[0]);
    return 1;
  }

  playbin = gst_element_factory_make ("playbin", NULL);
  if (!playbin) {
    g_print ("'playbin' gstreamer plugin missing\n");
    return 1;
  }

  /* take the commandline argument and ensure that it is a uri */
  if (gst_uri_is_valid (argv[1]))
    uri = g_strdup (argv[1]);
  else
    uri = gst_filename_to_uri (argv[1], NULL);
  g_object_set (playbin, "uri", uri, NULL);
  g_free (uri);

  /* create and event loop and feed gstreamer bus mesages to it */
  loop = g_main_loop_new (NULL, FALSE);

  bus = gst_element_get_bus (playbin);
  bus_watch_id = gst_bus_add_watch (bus, (GstBusFunc) bus_call, playbin);
  g_object_unref (bus);

  /* start play back and listed to events */
  gst_element_set_state (playbin, GST_STATE_PLAYING);
  g_main_loop_run (loop);

  /* cleanup */
  gst_element_set_state (playbin, GST_STATE_NULL);
  g_object_unref (playbin);
  g_source_remove (bus_watch_id);
  g_main_loop_unref (loop);

  return 0;
}
