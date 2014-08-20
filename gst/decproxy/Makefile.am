plugin_LTLIBRARIES = libgstdecproxy.la

# sources used to compile this plug-in
libgstdecproxy_la_SOURCES = \
	gstdecproxy.c \
	gstvdecproxy.c \
	gstadecproxy.c \
	plugin.c

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstdecproxy_la_CFLAGS = $(GST_CFLAGS)
libgstdecproxy_la_LIBADD = $(GST_LIBS) -lgstvideo-@GST_API_VERSION@ -lgstaudio-@GST_API_VERSION@
libgstdecproxy_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstdecproxy_la_LIBTOOLFLAGS = --tag=disable-static

noinst_HEADERS = \
	gstvdecproxy.h \
	gstadecproxy.h \
	gstdecproxy.h
