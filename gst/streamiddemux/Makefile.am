plugin_LTLIBRARIES = libgststreamiddemux.la

# sources used to compile this plug-in
libgststreamiddemux_la_SOURCES = \
	gststreamiddemux.c \
	plugin.c

# compiler and linker flags used to compile this plugin, set in configure.ac
libgststreamiddemux_la_CFLAGS = $(GST_CFLAGS)
libgststreamiddemux_la_LIBADD = $(GST_LIBS)
libgststreamiddemux_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgststreamiddemux_la_LIBTOOLFLAGS = --tag=disable-static

noinst_HEADERS = \
	gststreamiddemux.h
