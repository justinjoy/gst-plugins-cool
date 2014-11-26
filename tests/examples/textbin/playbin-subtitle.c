#include <gst/gst.h>

/* Copied from gst-plugins-base/gst/playback/gstplay-enum.h */
typedef enum
{
  GST_PLAY_FLAG_VIDEO = (1 << 0),
  GST_PLAY_FLAG_AUDIO = (1 << 1),
  GST_PLAY_FLAG_TEXT = (1 << 2),
  GST_PLAY_FLAG_VIS = (1 << 3),
  GST_PLAY_FLAG_SOFT_VOLUME = (1 << 4),
  GST_PLAY_FLAG_NATIVE_AUDIO = (1 << 5),
  GST_PLAY_FLAG_NATIVE_VIDEO = (1 << 6),
  GST_PLAY_FLAG_DOWNLOAD = (1 << 7),
  GST_PLAY_FLAG_BUFFERING = (1 << 8),
  GST_PLAY_FLAG_DEINTERLACE = (1 << 9),
  GST_PLAY_FLAG_SOFT_COLORBALANCE = (1 << 10),
  GST_PLAY_FLAG_NATIVE_TEXT = (1 << 11)
} GstPlayFlags;

static GstElement *playbin;

static gboolean
my_bus_callback (GstBus * bus, GstMessage * message, gpointer data)
{
  GMainLoop *loop = data;

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ASYNC_DONE:{
      g_message ("async-done");
      //GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN_CAST (playbin), GST_DEBUG_GRAPH_SHOW_ALL, "subtitle");
      break;
    }
    case GST_MESSAGE_APPLICATION:{
      if (gst_message_has_name (message, "subtitle_data")) {
        /* it's our message */
        const GstStructure *s;
        GstStructure *caps_st;
        const gchar *name;
        GstSample *sample;
        GstCaps *caps;
        GstBuffer *buf;

        s = gst_message_get_structure (message);
        GST_WARNING ("posted : %" GST_PTR_FORMAT, s);
        sample = gst_value_get_sample (gst_structure_get_value (s, "sample"));

        if (!sample) {
          g_message ("invalid sample value");
          break;
        }

        caps = gst_sample_get_caps (sample);
        buf = gst_sample_get_buffer (sample);

        GST_WARNING ("caps %" GST_PTR_FORMAT, caps);

        caps_st = gst_caps_get_structure (caps, 0);
        name = gst_structure_get_name (caps_st);

        if (!g_strcmp0 (name, "application/x-ass") ||
            !g_strcmp0 (name, "application/x-ssa")) {
          g_message ("ASS TYPE");
          GST_WARNING ("buffer %" GST_PTR_FORMAT, buf);
        }

        if (!g_strcmp0 (name, "text/plain") || !g_strcmp0 (name, "text/x-raw")) {
          g_message ("TEXT PLAIN TYPE");
          GstMapInfo map_info;
          gchar *text;

          gst_buffer_map (buf, &map_info, GST_MAP_READ);

          if (map_info.size == 0 || map_info.data == NULL) {
            gst_buffer_unmap (buf, &map_info);
            break;
          }

          text = g_memdup (map_info.data, map_info.size + 1);
          (text)[map_info.size] = '\0';

          gst_buffer_unmap (buf, &map_info);

          g_message ("text : %s", text);
          g_free (text);
        }

        if (!g_strcmp0 (name, "video/x-avi-unknown") ||
            !g_strcmp0 (name, "text/x-avi-internal") ||
            !g_strcmp0 (name, "subpicture/x-xsub")) {
          const gchar *fourcc = gst_structure_get_string (caps_st, "format");
          g_message ("x-xsub TYPE");
          if (!g_strcmp0 (fourcc, "DXSA")) {
            g_message ("DXSA TYPE");
          } else if (!g_strcmp0 (fourcc, "DXSB")) {
            g_message ("DXSB TYPE");
          }
          GST_WARNING ("buffer %" GST_PTR_FORMAT, buf);
        }

        gst_sample_unref (sample);
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

static void
cb_newpad (GstElement * decodebin, GstPad * pad, gpointer data)
{
  g_message ("pad-added");
}

static void
cb_no_more_pads (GstElement * element, gpointer user_data)
{
  g_message ("no-more-pads");
}

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

  if (g_str_has_prefix (elem_name, "uridecodebin")) {
    g_signal_connect (element, "pad-added", G_CALLBACK (cb_newpad), NULL);
    g_signal_connect (element, "no-more-pads", G_CALLBACK (cb_no_more_pads),
        NULL);
  }

  g_free (elem_name);
}

gint
main (gint argc, gchar * argv[])
{
  GMainLoop *loop;
  GstBus *bus;
  GstPluginFeature *feature;
  gint flags;

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
  playbin = gst_element_factory_make ("playbin", NULL);
  g_object_get (playbin, "flags", &flags, NULL);
  flags |= GST_PLAY_FLAG_NATIVE_TEXT;
  flags &= ~(GST_PLAY_FLAG_TEXT);
  flags &= ~(GST_PLAY_FLAG_DEINTERLACE);
  g_object_set (G_OBJECT (playbin), "flags", flags, "uri", argv[1], NULL);

  bus = gst_pipeline_get_bus (GST_PIPELINE (playbin));
  gst_bus_add_watch (bus, my_bus_callback, loop);
  gst_object_unref (bus);

  g_signal_connect (playbin, "element-added", G_CALLBACK (element_added_cb),
      loop);

  /* run */
  gst_element_set_state (playbin, GST_STATE_PLAYING);
  g_main_loop_run (loop);

  /* cleanup */
  gst_element_set_state (playbin, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (playbin));

  /* don't want to interfere with any other of the other tests */
  gst_plugin_feature_set_rank (feature, GST_RANK_NONE);
  gst_object_unref (feature);

  return 0;
}
