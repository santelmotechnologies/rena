plugindir = $(libdir)/rena/plugins/removable

AM_CPPFLAGS = \
	-I$(top_srcdir) 	\
	$(LIBPEAS_CFLAGS)		\
	$(LIBPEAS_GTK_CFLAGS)

plugin_LTLIBRARIES = libremovable.la

libremovable_la_SOURCES = \
	rena-devices-removable.c

libremovable_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libremovable_la_CFLAGS = \
	$(RENA_CFLAGS)

libremovable_la_LIBADD = \
	$(RENA_LIBS) \
	$(top_builddir)/src/librena.la

plugin_DATA = removable.plugin

EXTRA_DIST = $(plugin_DATA)
