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

/*
 * When caps event is coming, decproxy should trans caps's information to
 * media-info application message and post.
 * It should be confirmed by bus.
 */
GST_START_TEST (test_media_info)
{
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
