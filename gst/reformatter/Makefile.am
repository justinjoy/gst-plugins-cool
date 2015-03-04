plugin_LTLIBRARIES = libgstreformatter.la

# sources used to compile this plug-in
libgstreformatter_la_SOURCES = \
	gstreformatter.c \
	plugin.c

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstreformatter_la_CFLAGS = $(GST_CFLAGS)
libgstreformatter_la_LIBADD = \
	$(top_builddir)/gst-libs/gst/cool/libgstcool-@GST_API_VERSION@.la \
	-lgstaudio-$(GST_API_VERSION) \
	-lgstvideo-$(GST_API_VERSION) \
	$(GST_BASE_LIBS) $(GST_LIBS)
libgstreformatter_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstreformatter_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = \
	gstreformatter.h
