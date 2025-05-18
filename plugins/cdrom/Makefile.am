plugindir = $(libdir)/rena/plugins/cdrom

AM_CPPFLAGS = \
	-I$(top_srcdir) 	\
	$(LIBPEAS_CFLAGS)		\
	$(LIBPEAS_GTK_CFLAGS)

plugin_LTLIBRARIES = libcdrom.la

libcdrom_la_SOURCES =      \
	rena-cdrom-plugin.c  \
	rena-cdrom-plugin.h

libcdrom_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libcdrom_la_CFLAGS = \
	$(RENA_CFLAGS) \
	$(LIBCDIO_CFLAGS) \
	$(LIBCDDB_CFLAGS)

libcdrom_la_LIBADD = \
	$(RENA_LIBS) \
	$(LIBCDIO_LIBS) \
	$(LIBCDIO_PARANOIA_LIBS) \
	$(LIBCDDB_LIBS) \
	$(top_builddir)/src/librena.la

plugin_DATA = cdrom.plugin

EXTRA_DIST = $(plugin_DATA)
