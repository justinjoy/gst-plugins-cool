/*
 * GStreamer reformatter element
 *
 * Copyright 2015 LG Electronics, Inc.
 * Authors:
 *   SuHwang Kim <swhwang.kim@lge.com>
 *   HonHee Lee <hoonhee.lee@lge.com>
 *   YongJin Ohn <yongjin.ohn@lge.com>
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

#ifndef __GST_REFORMATTER_H__
#define __GST_REFORMATTER_H__

#include <gst/gst.h>
#include <gst/cool/gstcool.h>

G_BEGIN_DECLS
#define GST_TYPE_REFORMATTER (gst_reformatter_get_type())
#define GST_REFORMATTER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_REFORMATTER,GstReformatter))
#define GST_REFORMATTER_CAST(obj) ((GstReformatter *)obj)
#define GST_REFORMATTER_CLASS(obj) (G_TYPE_CHECK_CLASS_CAST((obj),GST_TYPE_REFORMATTER,GstReformatterClass))
#define GST_IS_REFORMATTER(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_REFORMATTER))
#define GST_IS_REFORMATTER_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((obj),GST_TYPE_REFORMATTER))
#define GST_REFORMATTER_GET_LOCK(reform) (&((GstReformatter*)(reform))->lock)
#define GST_REFORMATTER_LOCK(reform)   G_STMT_START { \
  GST_LOG_OBJECT (reform, "locking from thread %p", g_thread_self ()); \
  g_rec_mutex_lock (GST_REFORMATTER_GET_LOCK (reform)); \
  GST_LOG_OBJECT (reform, "locked from thread %p", g_thread_self ()); \
} G_STMT_END
#define GST_REFORMATTER_UNLOCK(reform)   G_STMT_START { \
  GST_LOG_OBJECT (reform, "unlocking from thread %p", g_thread_self ()); \
  g_rec_mutex_unlock (GST_REFORMATTER_GET_LOCK (reform)); \
  GST_LOG_OBJECT (reform, "unlocked from thread %p", g_thread_self ()); \
} G_STMT_END
typedef struct _GstReformatter GstReformatter;
typedef struct _GstReformatterClass GstReformatterClass;

struct _GstReformatter
{
  GstBin parent;

  GRecMutex lock;

  GstElement *aconverter;
  GstElement *aresampler;
};

struct _GstReformatterClass
{
  GstBinClass parent_class;
};

GType gst_reformatter_get_type (void);

G_END_DECLS
#endif /* __GST_REFORMATTER_H__ */
