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

  caps =
      gst_caps_new_simple ("video/x-h264", "stream-id", G_TYPE_STRING,
      "xxx-stream-id-xxx", "type", G_TYPE_INT, 1, NULL);

  // Should not be crashed even NULL ptr given.
  info1 = gst_cool_caps_to_info (caps);
  fail_unless (info1 != NULL, "Cannot create structure from CAPS");
  fail_unless_equals_string ("video/x-h264", gst_structure_get_name (info1));

  gst_structure_free (info1);
}

GST_END_TEST;

GST_START_TEST (test_taglist_to_info)
{
  GstTagList *taglist = NULL;
  GstStructure *info1;


  info1 = gst_cool_taglist_to_info (taglist);
  fail_unless (info1 == NULL, "Unexpected Structure");

  taglist = gst_tag_list_new ();
  info1 = gst_cool_taglist_to_info (taglist);
  fail_unless (info1 != NULL, "Cannot create structure from TagList");

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
