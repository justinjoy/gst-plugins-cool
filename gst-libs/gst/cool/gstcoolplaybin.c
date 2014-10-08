/* GStreamer Plugins Cool
 * Copyright (C) 2014 LG Electronics, Inc.
 *	Author : Jeongseok Kim <jeongseok.kim@lge.com>
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

#include "gstcool.h"
#include "gstcoolrawcaps.h"
#include "gstcoolutil.h"
#include "gstcoolplaybin.h"

#include <gobject/gvaluecollector.h>

static GstStaticCaps cool_raw_caps = GST_STATIC_CAPS (COOL_RAW_CAPS);

static void gst_cool_playbin_load_configuration (GstElement * playbin);

static void
q2_set_property_by_configuration (GstElement * q2)
{
  gint high_percent = 99;
  gint low_percent = 10;

  gint max_size_buffers = 0;
  gint max_size_bytes = 0;
  guint64 max_size_time = 0;
  guint64 ring_buffer_max_size = 0;

  gboolean use_rate_estimate = TRUE;

  GError *err = NULL;

  GKeyFile *config = gst_cool_get_configuration ();

  high_percent =
      g_key_file_get_integer (config, "buffering", "high-percent", &err);
  if (err) {
    GST_WARNING ("Unable to read high-percent: %s", err->message);
    g_error_free (err);
    err = NULL;
  }

  low_percent =
      g_key_file_get_integer (config, "buffering", "low-percent", &err);

  if (err) {
    GST_WARNING ("Unable to read low-percent: %s", err->message);
    g_error_free (err);
    err = NULL;
  }

  max_size_buffers =
      g_key_file_get_integer (config, "buffering", "max-size-buffers", &err);

  if (err) {
    GST_WARNING ("Unable to read max-size-buffers: %s", err->message);
    g_error_free (err);
    err = NULL;
  }

  max_size_bytes =
      g_key_file_get_integer (config, "buffering", "max-size-bytes", &err);

  if (err) {
    GST_WARNING ("Unable to read max-size-bytes: %s", err->message);
    g_error_free (err);
    err = NULL;
  }

  max_size_time =
      g_key_file_get_uint64 (config, "buffering", "max-size-time", &err);

  if (err) {
    GST_WARNING ("Unable to read max-size-time: %s", err->message);
    g_error_free (err);
    err = NULL;
  }

  ring_buffer_max_size =
      g_key_file_get_uint64 (config, "buffering", "ring-buffer-max-size", &err);

  if (err) {
    GST_WARNING ("Unable to read max-size-time: %s", err->message);
    g_error_free (err);
    err = NULL;
  }

  use_rate_estimate =
      g_key_file_get_boolean (config, "buffering", "use-rate-estimate", &err);

  if (err) {
    GST_WARNING ("Unable to read use-rate-estimate: %s", err->message);
    g_error_free (err);
    err = NULL;
  }


  GST_DEBUG ("queue2 has properties: high-percent: %d,"
      "low-percent: %d, "
      "max-size-buffers: %d, "
      "max-size-bytes: %d, "
      "max-size-time: %" G_GUINT64_FORMAT ", "
      "ring-buffer-max-size: %" G_GUINT64_FORMAT ", "
      "use-rate-estimate: %d",
      high_percent, low_percent, max_size_buffers, max_size_bytes,
      max_size_time, ring_buffer_max_size, use_rate_estimate);

  g_object_set (q2, "high-percent", high_percent,
      "low-percent", low_percent,
      "max-size-buffers", max_size_buffers,
      "max-size-bytes", max_size_bytes, "max-size-time", max_size_time,
      "ring-buffer-max-size", ring_buffer_max_size,
      "use-rate-estimate", use_rate_estimate, NULL);
}

static void
uridecodebin_element_added (GstElement * uridecodebin, GstElement * element,
    gpointer user_data)
{
  if (!g_strrstr (GST_ELEMENT_NAME (element), "queue2"))
    return;

  GST_DEBUG ("queue2 is deployed");

  q2_set_property_by_configuration (element);
}

static void
playbin_element_added (GstElement * playbin, GstElement * element,
    gpointer user_data)
{
  if (!g_strrstr (GST_ELEMENT_NAME (element), "uridecodebin"))
    return;

  // FIXME: reclaim connected signal handle when destorying playbin
  g_signal_connect (GST_BIN_CAST (element), "element-added",
      (GCallback) uridecodebin_element_added, playbin);

  if (!g_object_class_find_property (G_OBJECT_GET_CLASS (element), "caps")) {
    GST_ERROR ("%s does not has caps property", GST_ELEMENT_NAME (element));
    return;
  }
  // FIXME: cool_raw_caps should come from configuration
  g_object_set (element, "caps", gst_static_caps_get (&cool_raw_caps), NULL);
}

static void
gst_cool_playbin_set_default_sink (GstElement * playbin)
{
  const gchar *videosink = NULL;
  const gchar *audiosink = NULL;
  GKeyFile *config = gst_cool_get_configuration ();
  GError *err = NULL;
  GstIterator *iter;
  gboolean done = FALSE;

  videosink = g_key_file_get_string (config, "default_sink", "video", &err);
  if (err) {
    GST_WARNING ("Unable to read default video sink: %s", err->message);
    g_error_free (err);
    err = NULL;
  }

  audiosink = g_key_file_get_string (config, "default_sink", "audio", &err);
  if (err) {
    GST_WARNING ("Unable to read default video sink: %s", err->message);
    g_error_free (err);
    err = NULL;
  }

  if (!audiosink && !videosink)
    return;

  iter = gst_bin_iterate_sinks (GST_BIN (playbin));
  while (!done) {
    GstIteratorResult ires;
    GValue item = { 0 };

    ires = gst_iterator_next (iter, &item);
    switch (ires) {
      case GST_ITERATOR_DONE:
        done = TRUE;
        break;
      case GST_ITERATOR_OK:
      {
        GstElement *playsink = g_value_get_object (&item);

        if (audiosink) {
          /* for sending media-info in audio/x-raw cases,
           * audiosink should be set to 0. */
          gst_cool_set_rank (audiosink, 0);

          g_object_set (playsink, "default-audio-sink", audiosink, NULL);
        }
        if (videosink)
          g_object_set (playsink, "default-video-sink", videosink, NULL);

        // FIXME: https://jira2.lgsvl.com/browse/BHV-15462
        g_object_set (playsink, "delay-config-mode", TRUE, NULL);

        GST_INFO_OBJECT (playsink,
            "set default sinks, audiosink:%s, videosink:%s", audiosink,
            videosink);

        g_value_reset (&item);
        break;
      }
      case GST_ITERATOR_RESYNC:
        gst_iterator_resync (iter);
        break;
      default:
        done = TRUE;
        break;
    }
    g_value_unset (&item);
  }

  gst_iterator_free (iter);
}

gboolean
gst_cool_playbin_init (GstElement * playbin)
{
  g_return_val_if_fail (playbin != NULL, FALSE);

  // FIXME: reclaim connected signal handle when destorying playbin
  /* change default caps on uridecodebin */
  g_signal_connect (GST_BIN_CAST (playbin), "element-added",
      (GCallback) playbin_element_added, NULL);

  gst_cool_playbin_set_default_sink (playbin);

  gst_cool_playbin_load_configuration (playbin);

  return TRUE;
}

static gint
find_q2_element (const GValue * item, GstElement * playbin)
{
  GstElement *element = g_value_get_object (item);

  if (!g_strrstr (GST_ELEMENT_NAME (element), "queue2"))
    return 1;

  return 0;
}

GstElement *
gst_cool_playbin_get_q2 (GstElement * playbin)
{
  GstElement *q2 = NULL;
  GstIterator *it;
  gboolean found = FALSE;
  GValue item = { 0, };

  g_return_val_if_fail (playbin != NULL, NULL);

  it = gst_bin_iterate_recurse (GST_BIN_CAST (playbin));
  found = gst_iterator_find_custom (it,
      (GCompareFunc) find_q2_element, &item, &playbin);
  gst_iterator_free (it);

  if (found) {
    q2 = g_value_get_object (&item);
    g_value_unset (&item);
  }
  return q2;
}

void
gst_cool_playbin_set_q2_conf_valist (GstElement * playbin,
    const gchar * fieldname, va_list vaargs)
{
  GType type;
  GValue value = G_VALUE_INIT;

  GKeyFile *config = gst_cool_get_configuration ();
  GError *err = NULL;

  GstElement *q2 = gst_cool_playbin_get_q2 (playbin);

  while (fieldname) {
    type = va_arg (vaargs, GType);
    G_VALUE_COLLECT_INIT (&value, type, vaargs, 0, &err);

    if (g_key_file_has_key (config, "buffering", fieldname, &err)) {
      switch (type) {
        case G_TYPE_INT:
        case G_TYPE_UINT:
          g_key_file_set_integer (config, "buffering", fieldname,
              g_value_get_int (&value));
          break;
        case G_TYPE_INT64:
          g_key_file_set_int64 (config, "buffering", fieldname,
              g_value_get_int64 (&value));
          break;
        case G_TYPE_UINT64:
          g_key_file_set_uint64 (config, "buffering", fieldname,
              g_value_get_uint64 (&value));
          break;
        case G_TYPE_BOOLEAN:
          g_key_file_set_boolean (config, "buffering", fieldname,
              g_value_get_boolean (&value));
          break;
        default:
          GST_WARNING ("Unsupported configuration format");
          break;
      }
    } else {
      GST_WARNING ("Failed to set %s property to q2: %s", fieldname,
          err->message);
      g_error_free (err);
      err = NULL;
    }
    fieldname = va_arg (vaargs, gchar *);
  }

  if (q2 != NULL) {
    q2_set_property_by_configuration (q2);
    gst_object_unref (GST_OBJECT (q2));
    GST_DEBUG ("Detected q2 instance, configuration will be set directly");
  }
}

void
gst_cool_playbin_set_q2_conf (GstElement * playbin, const gchar * firstfield,
    ...)
{
  va_list vaargs;
  g_return_if_fail (playbin != NULL);

  va_start (vaargs, firstfield);
  gst_cool_playbin_set_q2_conf_valist (playbin, firstfield, vaargs);
  va_end (vaargs);
}

static gboolean
dot_graph_timeout_cb (GstElement * playbin)
{
  GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (playbin),
      GST_DEBUG_GRAPH_SHOW_ALL, "timeout_dot_graph");
  g_message ("timout dot-graph created");
  return TRUE;
}

static void
gst_cool_playbin_load_configuration (GstElement * playbin)
{
  GError *err = NULL;
  GKeyFile *config = gst_cool_get_configuration ();
  gint dot_graph_timeout = 0;
  const gchar *debug_mode;

  debug_mode = g_key_file_get_string (config, "debug", "DEBUG_MODE", &err);
  if (err) {
    GST_WARNING ("Unable to read debug mode: %s", err->message);
    g_error_free (err);
    err = NULL;
  }

  /* if debug mode is not enable, do not set debug option */
  if (g_strcmp0 (debug_mode, "enable") != 0)
    return;

  dot_graph_timeout =
      g_key_file_get_integer (config, "debug", "DOT_GRAPH_TIMEOUT", &err);
  if (err) {
    GST_WARNING ("Unable to read debug mode: %s", err->message);
    g_error_free (err);
    err = NULL;
  }

  if (dot_graph_timeout) {
    g_timeout_add (dot_graph_timeout, (GSourceFunc) dot_graph_timeout_cb,
        playbin);
  }
}
