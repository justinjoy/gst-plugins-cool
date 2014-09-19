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

#include <gst/check/gstcheck.h>
#include <gst/cool/gstcool.h>

GST_START_TEST (test_caps_to_info)
{
  GstCaps *caps;
  GstStructure *info1;
  const gchar *fieldname;
  const gchar *value;

  caps = gst_caps_new_simple ("video/x-h264",
      "stream-format", G_TYPE_STRING, "byte-stream",
      "alignment", G_TYPE_STRING, "nal", "format", G_TYPE_STRING, "h264", NULL);

  info1 = gst_cool_caps_to_info (caps, "xxx-stream-id-xxx");
  fail_unless (info1 != NULL, "Cannot create structure from CAPS");

  /* TEST1 : compare structure name */
  fail_unless_equals_string ("media-info", gst_structure_get_name (info1));

  /* TEST2-1 : compare field name about second field */
  fieldname = gst_structure_nth_field_name (info1, 2);
  value = gst_structure_get_string (info1, fieldname);
  fail_unless_equals_string ("video/x-h264", value);

  /* TEST2-2 : compare field name about third field */
  fieldname = gst_structure_nth_field_name (info1, 3);
  value = gst_structure_get_string (info1, fieldname);
  fail_unless_equals_string ("byte-stream", value);

  gst_caps_unref (caps);
}

GST_END_TEST;

GST_START_TEST (test_taglist_to_info)
{
  GstTagList *taglist;
  GstStructure *info1;
  const gchar *fieldname;
  const gchar *value;

  taglist = gst_tag_list_new (GST_TAG_DEVICE_MANUFACTURER, "MyFavoriteBrand",
      GST_TAG_DEVICE_MODEL, "123v42.1",
      GST_TAG_DESCRIPTION, "some description",
      GST_TAG_APPLICATION_NAME, "camerabin test", NULL);

  info1 = gst_cool_taglist_to_info (taglist, "xxx-stream-id-xxx", "video/mpeg");
  fail_unless (info1 != NULL, "Cannot create structure from TagList");

  /* TEST1 : compare structure name */
  fail_unless_equals_string ("media-info", gst_structure_get_name (info1));

  /* TEST2-1 : compare field name about second field */
  fieldname = gst_structure_nth_field_name (info1, 2);
  value = gst_structure_get_string (info1, fieldname);
  fail_unless_equals_string ("video/mpeg", value);

  /* TEST2-2 : compare field name about third field */
  fieldname = gst_structure_nth_field_name (info1, 3);
  value = gst_structure_get_string (info1, fieldname);
  fail_unless_equals_string ("MyFavoriteBrand", value);

  gst_structure_free (info1);
  gst_tag_list_unref (taglist);
}

GST_END_TEST;

static Suite *
gstcoolutil_suite (void)
{
  Suite *s = suite_create ("GstCoolUtil");
  TCase *tc_chain = tcase_create ("gst cool util tests");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_caps_to_info);
  tcase_add_test (tc_chain, test_taglist_to_info);
  return s;
}

GST_CHECK_MAIN (gstcoolutil);
