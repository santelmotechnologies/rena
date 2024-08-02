plugindir = $(libdir)/rena/plugins/tunein

AM_CPPFLAGS = \
	-I$(top_srcdir) 	\
	$(LIBPEAS_CFLAGS)		\
	$(LIBPEAS_GTK_CFLAGS)

plugin_LTLIBRARIES = libtunein.la

libtunein_la_SOURCES =      \
	rena-tunein-plugin.c

libtunein_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libtunein_la_CFLAGS = \
	$(RENA_CFLAGS) \
	$(LIBSOUP_CFLAGS)


libtunein_la_LIBADD = \
	$(RENA_LIBS) \
	$(LIBSOUP_LIBS) \
	$(top_builddir)/src/librena.la

plugin_DATA = tunein.plugin

EXTRA_DIST = $(plugin_DATA)
