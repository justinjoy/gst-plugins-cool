plugin_LTLIBRARIES = libgsttextbin.la

# sources used to compile this plug-in
libgsttextbin_la_SOURCES = \
	gsttextbin.c \
	plugin.c

# compiler and linker flags used to compile this plugin, set in configure.ac
libgsttextbin_la_CFLAGS = $(GST_CFLAGS)
libgsttextbin_la_LIBADD = $(GST_LIBS)
libgsttextbin_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgsttextbin_la_LIBTOOLFLAGS = --tag=disable-static

noinst_HEADERS = \
	gsttextbin.h
