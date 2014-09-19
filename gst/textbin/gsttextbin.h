/*
 * GStreamer textbin element
 *
 * Copyright 2014 LG Electronics, Inc.
 *  @author: HoonHee Lee <hoonhee.lee@lge.com>
 *
 * gsttextbin.h: Deserializable element for serialized text-stream and posting text data
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef __GST_TEXT_BIN_H__
#define __GST_TEXT_BIN_H__

#include <gst/gst.h>

G_BEGIN_DECLS
#define GST_TYPE_TEXT_BIN (gst_text_bin_get_type())
#define GST_TEXT_BIN(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_TEXT_BIN,GstTextBin))
#define GST_TEXT_BIN_CLASS(obj) (G_TYPE_CHECK_CLASS_CAST((obj),GST_TYPE_TEXT_BIN,GstTextBinClass))
#define GST_IS_TEXT_BIN(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_TEXT_BIN))
#define GST_IS_TEXT_BIN_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((obj),GST_TYPE_TEXT_BIN))
#define GST_TEXT_BIN_GET_LOCK(bin) (&((GstTextBin*)(bin))->lock)
#define GST_TEXT_BIN_LOCK(bin) (g_rec_mutex_lock (GST_TEXT_BIN_GET_LOCK(bin)))
#define GST_TEXT_BIN_UNLOCK(bin) (g_rec_mutex_unlock (GST_TEXT_BIN_GET_LOCK(bin)))
typedef struct _GstTextBin GstTextBin;
typedef struct _GstTextBinClass GstTextBinClass;

struct _GstTextBin
{
  GstBin parent;

  GRecMutex lock;

  GstPad *sinkpad;

  GstElement *streamiddemux;
  GstElement *mq;
  GstElement *tsinkbin;
};

struct _GstTextBinClass
{
  GstBinClass parent_class;
};

GType gst_text_bin_get_type (void);

G_END_DECLS
#endif /* __GST_TEXT_BIN_H__ */
