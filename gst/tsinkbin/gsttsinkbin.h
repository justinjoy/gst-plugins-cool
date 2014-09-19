/* GStreamer Lightweight Playback Plugins
 *
 * Copyright (C) 2013-2014 LG Electronics, Inc.
 *	Author : Wonchul Lee <wonchul86.lee@lge.com>
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


#ifndef __GST_TSINK_BIN_H__
#define __GST_TSINK_BIN_H__

#include <gst/gst.h>

G_BEGIN_DECLS
#define GST_TYPE_TSINK_BIN (gst_tsink_bin_get_type())
#define GST_TSINK_BIN(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_TSINK_BIN,GstTSinkBin))
#define GST_TSINK_BIN_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_TSINK_BIN,GstLpTSinkBinClass))
#define GST_IS_TSINK_BIN(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_TSINK_BIN))
#define GST_IS_TSINK_BIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_TSINK_BIN))
#define GST_TSINK_BIN_CAST(obj) ((GstTSinkBin*)(obj))
#define GST_TSINK_BIN_GET_LOCK(bin) (&((GstTSinkBin*)(bin))->lock)
#define GST_TSINK_BIN_LOCK(bin) (g_rec_mutex_lock (GST_TSINK_BIN_GET_LOCK(bin)))
#define GST_TSINK_BIN_UNLOCK(bin) (g_rec_mutex_unlock (GST_TSINK_BIN_GET_LOCK(bin)))
typedef struct _GstTextGroup GstTextGroup;
typedef struct _GstTSinkBinClass GstTSinkBinClass;
typedef struct _GstTSinkBin GstTSinkBin;
typedef struct _GstTSinkBinClass GstTSinkBinClass;

struct _GstTextGroup
{
  GstElement *appsink;
  GstElement *queue;
};

struct _GstTSinkBin
{
  GstBin parent;

  GRecMutex lock;               /* to protect group switching */

  GList *sink_list;
};

struct _GstTSinkBinClass
{
  GstBinClass parent_class;
};

GType gst_tsink_bin_get_type (void);

G_END_DECLS
#endif // __GST_TSINK_BIN_H__
