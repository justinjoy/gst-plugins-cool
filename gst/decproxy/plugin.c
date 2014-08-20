#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include "gstvdecproxy.h"
#include "gstadecproxy.h"

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "vdecproxy", GST_RANK_NONE,
          GST_TYPE_VDEC_PROXY))
    return FALSE;

  if (!gst_element_register (plugin, "adecproxy", GST_RANK_NONE,
          GST_TYPE_ADEC_PROXY))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    decproxy,
    "Decoder Proxy",
    plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);
