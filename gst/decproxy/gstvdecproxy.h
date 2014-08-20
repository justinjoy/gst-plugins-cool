/* GStreamer Plugins Cool
 * Copyright (C) 2014 LG Electronics, Inc.
 *	Author : Wonchul Lee <wonchul86.lee@lge.com>
 *	Author : HoonHee Lee <hoonhee.lee@lge.com>
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


#ifndef __GST_VDEC_PROXY_H__
#define __GST_VDEC_PROXY_H__

#include <gst/gst.h>
#include "gstdecproxy.h"

G_BEGIN_DECLS
#define GST_TYPE_VDEC_PROXY (gst_vdec_proxy_get_type())
#define GST_VDEC_PROXY(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VDEC_PROXY,GstVDecProxy))
#define GST_VDEC_PROXY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VDEC_PROXY,GstVDecProxyClass))
#define GST_IS_VDEC_PROXY(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VDEC_PROXY))
#define GST_IS_VDEC_PROXY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VDEC_PROXY))
typedef struct _GstVDecProxy GstVDecProxy;
typedef struct _GstVDecProxyClass GstVDecProxyClass;

struct _GstVDecProxy
{
  GstDecProxy parent;
};

struct _GstVDecProxyClass
{
  GstDecProxyClass parent_class;
};

GType gst_vdec_proxy_get_type (void);

G_END_DECLS
#endif // __GST_VDEC_PROXY_H__
