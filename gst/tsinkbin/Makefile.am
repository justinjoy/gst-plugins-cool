plugin_LTLIBRARIES = libgsttsinkbin.la

# sources used to compile this plug-in
libgsttsinkbin_la_SOURCES = \
	gsttsinkbin.c \
	plugin.c

# compiler and linker flags used to compile this plugin, set in configure.ac
libgsttsinkbin_la_CFLAGS = $(GST_CFLAGS)
libgsttsinkbin_la_LIBADD = $(GST_LIBS)
libgsttsinkbin_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgsttsinkbin_la_LIBTOOLFLAGS = --tag=disable-static

noinst_HEADERS = \
	gsttsinkbin.h
