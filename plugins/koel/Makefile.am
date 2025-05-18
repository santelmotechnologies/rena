plugindir = $(libdir)/rena/plugins/koel

AM_CPPFLAGS = \
	-I$(top_srcdir) 	\
	$(LIBPEAS_CFLAGS)		\
	$(LIBPEAS_GTK_CFLAGS)

plugin_LTLIBRARIES = libpkoel.la

libpkoel_la_SOURCES =		\
	rena-koel-plugin.h 	\
	rena-koel-plugin.c

libpkoel_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libpkoel_la_CFLAGS = \
	$(LIBSOUP_CFLAGS) \
	$(JSON_GLIB_CFLAGS) \
	$(RENA_CFLAGS)

libpkoel_la_LIBADD = \
	$(RENA_LIBS) \
	$(LIBSOUP_LIBS) \
	$(JSON_GLIB_LIBS) \
	$(top_builddir)/src/librena.la

plugin_DATA = koel.plugin

EXTRA_DIST = $(plugin_DATA)
