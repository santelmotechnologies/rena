plugindir = $(libdir)/rena/plugins/gnome-media-keys

AM_CPPFLAGS = \
	-I$(top_srcdir) 	\
	$(LIBPEAS_CFLAGS)		\
	$(LIBPEAS_GTK_CFLAGS)

plugin_LTLIBRARIES = libgnome-media-keys.la

libgnome_media_keys_la_SOURCES =       \
	rena-gnome-media-keys-plugin.c \
	rena-gnome-media-keys-plugin.h

libgnome_media_keys_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libgnome_media_keys_la_CFLAGS = \
	$(RENA_CFLAGS)

libgnome_media_keys_la_LIBADD = \
	$(RENA_LIBS) \
	$(top_builddir)/src/librena.la

plugin_DATA = gnome-media-keys.plugin

EXTRA_DIST = $(plugin_DATA)
