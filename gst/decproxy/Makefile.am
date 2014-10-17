plugin_LTLIBRARIES = libgstdecproxy.la

# sources used to compile this plug-in
libgstdecproxy_la_SOURCES = \
	gstdecproxy.c \
	gstmediainfo.c \
	gstfakeadec.c \
	gstfakevdec.c \
	plugin.c

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstdecproxy_la_CFLAGS = $(GST_CFLAGS)
libgstdecproxy_la_LIBADD = \
	$(top_builddir)/gst-libs/gst/cool/libgstcool-@GST_API_VERSION@.la \
	-lgstaudio-$(GST_API_VERSION) \
	-lgstvideo-$(GST_API_VERSION) \
	$(GST_BASE_LIBS) $(GST_LIBS)
libgstdecproxy_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstdecproxy_la_LIBTOOLFLAGS = --tag=disable-static

noinst_HEADERS = \
	gstdecproxy.h \
	gstfakeadec.h \
	gstfakevdec.h \
	gstmediainfo.h
