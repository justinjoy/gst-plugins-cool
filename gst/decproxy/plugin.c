#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include "gstdecproxy.h"
#include "gstmediainfo.h"
#include "gstfakeadec.h"
#include "gstfakevdec.h"

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "decproxy", GST_RANK_NONE,
          GST_TYPE_DEC_PROXY))
    return FALSE;

  if (!gst_element_register (plugin, "mediainfo", GST_RANK_NONE,
          GST_TYPE_MEDIA_INFO))
    return FALSE;

  if (!gst_element_register (plugin, "fakeadec", GST_RANK_NONE,
          GST_TYPE_FAKEADEC))
    return FALSE;

  if (!gst_element_register (plugin, "fakevdec", GST_RANK_NONE,
          GST_TYPE_FAKEVDEC))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    decproxy,
    "Decoder Proxy",
    plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);
