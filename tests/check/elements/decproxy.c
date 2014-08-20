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

static gboolean
compare_media_field (GQuark field_id, const GValue * value, gpointer user_data)
{
  const GstStructure *s2;
  gchar *value_str1, *value_str2;

  /* No need to compare stream-id and type */
  if ((g_strcmp0 (g_quark_to_string (field_id), "stream-id") == 0)
      || (g_strcmp0 (g_quark_to_string (field_id), "type") == 0))
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

  decproxy = gst_element_factory_make ("vdecproxy", "vdecproxy");
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

/*
 * When active event is coming, decproxy should deploy a decode element
 * and connect.
 */
GST_START_TEST (test_active_decodable_element)
{
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

  return s;
}

GST_CHECK_MAIN (decproxy);
