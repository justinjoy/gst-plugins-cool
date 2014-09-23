lib_LTLIBRARIES = libgstcool-@GST_API_VERSION@.la

include $(top_srcdir)/common/gst-glib-gen.mak

libgstcool_@GST_API_VERSION@_la_SOURCES = gstcool.c gstcoolplaybin.c gstcoolutil.c
libgstcool_@GST_API_VERSION@_la_CFLAGS = $(GST_PLUGINS_BASE_CFLAGS) \
	$(GST_BASE_CFLAGS) $(GST_CFLAGS)
libgstcool_@GST_API_VERSION@_la_LIBADD = $(GST_BASE_LIBS)
libgstcool_@GST_API_VERSION@_la_LDFLAGS = $(GST_LIB_LDFLAGS) $(GST_ALL_LDFLAGS) $(GST_LT_LDFLAGS)

libgstcool_@GST_API_VERSION@includedir = $(includedir)/gstreamer-@GST_API_VERSION@/gst/cool
libgstcool_@GST_API_VERSION@include_HEADERS = \
	gstcool.h \
	gstcoolutil.h \
	gstcoolplaybin.h \
	gstcoolrawcaps.h

CLEANFILES = $(BUILT_SOURCES)
