/* GStreamer Plugins Cool
 * Copyright (C) 2014 LG Electronics, Inc.
 *    Author : Wonchul Lee <wonchul86.lee@lge.com>
 *             HoonHee Lee <hoonhee.lee@lge.com>
 *             Jeongseok Kim <jeongseok.kim@lge.com>
 *             Myoungsun Lee <mysunny.lee@lge.com>
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


#ifndef __GST_DECPROXY2_H__
#define __GST_DECPROXY2_H__

#include <gst/gst.h>
#include <gst/cool/gstcool.h>

G_BEGIN_DECLS

#define DECODE_VIDEO_CAPS \
    "video/x-divx;" \
    "video/x-h265;" \
    "video/x-h264;" \
    "video/x-intel-h263;" \
    "video/x-h263;" \
    "video/mpeg;" \
    "video/x-wmv;" \
    "video/x-msmpeg;" \
    "video/x-pn-realvideo;" \
    "video/x-pn-realvideo;" \
    "video/x-svq;" \
    "video/x-ffv;" \
    "video/x-3ivx;" \
    "video/x-vp8;" \
    "video/x-vp9;" \
    "video/x-xvid;" \
    "video/x-flash-video;" \
    "image/jpeg"

#define DECODE_AUDIO_CAPS \
    "audio/mpeg;"\
    "audio/x-dts;" \
    "audio/x-dtsh;" \
    "audio/x-dtsl;" \
    "audio/x-dtse;" \
    "audio/x-ac3;" \
    "audio/x-eac3;" \
    "audio/x-private1-ac3;" \
    "audio/x-wma;" \
    "audio/x-pn-realaudio;" \
    "audio/x-lpcm-1;" \
    "audio/x-lpcm;" \
    "audio/x-private-lg-lpcm;" \
    "audio/x-private1-lpcm;" \
    "audio/x-private-ts-lpcm;" \
    "audio/x-adpcm;" \
    "audio/x-vorbis;" \
    "audio/AMR;" \
    "audio/AMR-WB;" \
    "audio/x-flac;" \
    "audio/x-mulaw;" \
    "audio/x-alaw;" \
    "audio/x-private1-dts"

#define GST_TYPE_DECPROXY (gst_decproxy_get_type())
#define GST_DECPROXY(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_DECPROXY,GstDecProxy))
#define GST_DECPROXY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_DECPROXY,GstDecProxyClass))
#define GST_IS_DECPROXY(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_DECPROXY))
#define GST_IS_DECPROXY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_DECPROXY))
#define GST_DECPROXY_GET_LOCK(dec) (&((GstDecProxy*)(dec))->lock)

#define GST_DECPROXY_LOCK(dec)   G_STMT_START { \
  GST_LOG_OBJECT (dec, "locking from thread %p", g_thread_self ()); \
  g_rec_mutex_lock (GST_DECPROXY_GET_LOCK (dec)); \
  GST_LOG_OBJECT (dec, "locked from thread %p", g_thread_self ()); \
} G_STMT_END 

#define GST_DECPROXY_UNLOCK(dec)   G_STMT_START { \
  GST_LOG_OBJECT (dec, "unlocking from thread %p", g_thread_self ()); \
    g_rec_mutex_unlock (GST_DECPROXY_GET_LOCK (dec)); \
} G_STMT_END 

typedef struct _GstDecProxy GstDecProxy;
typedef struct _GstDecProxyClass GstDecProxyClass;

typedef enum {
  GST_DECPROXY_STATE_UNKNOWN = -1,
  GST_DECPROXY_STATE_PUPPET = 0,
  GST_DECPROXY_STATE_DECODER
} GstDecProxyState;

struct _GstDecProxy
{
  GstBin parent;

  GRecMutex lock;

  GstElement *front;
  GstElement *back;

  GstElement *decoder;
  GstElement *puppet;

  GstDecProxyState current_decoder_state;
  GstDecProxyState next_decoder_state;
  GstDecProxyState pending_decoder_state;
  GstStructure *resource_info;

  GstCoolStreamType stream_type;
  gulong blocked_id;

  gboolean pending_switch_decoder;
  gboolean dts_seamless;
};

struct _GstDecProxyClass
{
  GstBinClass parent_class;
};

GType gst_decproxy_get_type (void);

G_END_DECLS

#endif // __GST_DECPROXY2_H__
