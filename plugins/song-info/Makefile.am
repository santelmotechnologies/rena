plugindir = $(libdir)/rena/plugins/song-info

AM_CPPFLAGS = \
	-I$(top_srcdir) 	\
	$(LIBPEAS_CFLAGS)		\
	$(LIBPEAS_GTK_CFLAGS)

plugin_LTLIBRARIES = libsong-info.la

libsong_info_la_SOURCES =						\
	rena-song-info-cache.c					\
	rena-song-info-cache.h					\
	rena-song-info-plugin.c					\
	rena-song-info-plugin.h					\
	rena-song-info-dialog.c					\
	rena-song-info-dialog.h					\
	rena-song-info-pane.c						\
	rena-song-info-pane.h						\
	rena-song-info-thread-albumart.c			\
	rena-song-info-thread-albumart.h			\
	rena-song-info-thread-artistart.c			\
	rena-song-info-thread-artistart.h			\
	rena-song-info-thread-dialog.c			\
	rena-song-info-thread-dialog.h			\
	rena-song-info-thread-pane.c				\
	rena-song-info-thread-pane.h

libsong_info_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libsong_info_la_CFLAGS = \
	$(RENA_CFLAGS) \
	$(LIBGLYR_CFLAGS)

libsong_info_la_LIBADD = \
	$(RENA_LIBS) \
	$(LIBGLYR_LIBS) \
	$(top_builddir)/src/librena.la


rena-song-info-ui.h: rena-song-info.ui Makefile
	$(AM_V_GEN) xdt-csource --strip-comments --strip-content --static --name=rena_song_info_ui $< > $@

plugin_DATA = song-info.plugin

EXTRA_DIST = \
	$(plugin_DATA) \
	rena-song-info.ui

DISTCLEANFILES = \
	rena-song-info-ui.h

BUILT_SOURCES = \
	rena-song-info-ui.h
