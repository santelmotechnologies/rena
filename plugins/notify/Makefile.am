plugindir = $(libdir)/rena/plugins/notify

AM_CPPFLAGS = \
	-I$(top_srcdir) 	\
	$(LIBPEAS_CFLAGS)		\
	$(LIBPEAS_GTK_CFLAGS)

plugin_LTLIBRARIES = libpnotify.la

libpnotify_la_SOURCES =						\
	rena-notify-plugin.c

libpnotify_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libpnotify_la_CFLAGS = \
	$(RENA_CFLAGS) \
	$(LIBNOTIFY_CFLAGS)

libpnotify_la_LIBADD = \
	$(RENA_LIBS) \
	$(LIBNOTIFY_LIBS) \
	$(top_builddir)/src/librena.la

plugin_DATA = notify.plugin

EXTRA_DIST = $(plugin_DATA)
