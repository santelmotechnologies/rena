SUBDIRS = win32

AM_CPPFLAGS =								\
	-DG_LOG_DOMAIN=\"rena\"					\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	-DPIXMAPDIR=\""$(datadir)/pixmaps/rena"\"			\
	-DDESKTOPENTRY=\""$(datadir)/applications/rena.desktop"\"	\
	-DLIBPLUGINDIR=\""$(libdir)/rena/plugins/"\"	\
	-DUSRPLUGINDIR=\""$(prefix)/share/rena/plugins"\"	\
	-DUSRSTYLEDIR=\""$(datadir)/rena/style"\"	\
	-I$(top_srcdir)

EXTRA_DIST =
DISTCLEANFILES =

#
# Rena library
#
pkglib_LTLIBRARIES = librena.la

librenadir = $(includedir)/librena
librena_HEADERS = \
	gtkcellrendererbubble.h \
	rena-album-art.h \
	rena-app-notification.h \
	rena-app-notification-container.h \
	rena-art-cache.h \
	rena-backend.h \
	rena-background-task-bar.h \
	rena-background-task-widget.h \
	rena-database-provider.h \
	rena-database.h \
	rena-debug.h \
	rena-dnd.h \
	rena-equalizer-dialog.h \
	rena-favorites.h \
	rena-file-utils.h \
	rena-filter-dialog.h \
	rena-library-pane.h \
	rena-hig.h \
	rena-menubar.h \
	rena-music-enum.h \
	rena-musicobject.h \
	rena-musicobject-mgmt.h \
	rena-playback.h \
	rena-playlist.h \
	rena-playlists-mgmt.h \
	rena-preferences.h \
	rena-preferences-dialog.h \
	rena-prepared-statement.h \
	rena-prepared-statement-private.h \
	rena-provider.h \
	rena-scanner.h \
	rena-search-entry.h \
	rena-session.h \
	rena-sidebar.h \
	rena-simple-async.h \
	rena-simple-widgets.h \
	rena-song-cache.h \
	rena-statusbar.h \
	rena-statusicon.h \
	rena-tagger.h \
	rena-tags-dialog.h \
	rena-tags-mgmt.h \
	rena-temp-provider.h \
	rena-toolbar.h \
	rena-utils.h \
	rena-window.h \
	rena.h \
	xml_helper.h

librena_la_SOURCES = \
	$(librena_HEADERS) \
	$(BUILT_SOURCES) \
	gtkcellrendererbubble.c \
	info-bar-import-music.c \
	rena-album-art.c \
	rena-app-notification.c \
	rena-app-notification-container.c \
	rena-art-cache.c \
	rena-backend.c \
	rena-background-task-bar.c \
	rena-background-task-widget.c \
	rena-cmdline.c \
	rena-database-provider.c \
	rena-database.c \
	rena-debug.c \
	rena-dnd.c \
	rena-equalizer-dialog.c \
	rena-favorites.c \
	rena-file-utils.c \
	rena-filter-dialog.c \
	rena-hig.c \
	rena-library-pane.c \
	rena-menubar.c \
	rena-music-enum.c \
	rena-musicobject.c \
	rena-musicobject-mgmt.c \
	rena-playback.c \
	rena-playlist.c \
	rena-playlists-mgmt.c \
	rena-preferences.c \
	rena-preferences-dialog.c \
	rena-prepared-statement.c \
	rena-provider.c \
	rena-scanner.c \
	rena-search-entry.c \
	rena-session.c \
	rena-sidebar.c \
	rena-simple-async.c \
	rena-simple-widgets.c \
	rena-song-cache.c \
	rena-statusbar.c \
	rena-statusicon.c \
	rena-tagger.c \
	rena-tags-dialog.c \
	rena-tags-mgmt.c \
	rena-temp-provider.c \
	rena-toolbar.c \
	rena-utils.c \
	rena-window.c \
	rena.c \
	xml_helper.c

librena_la_CFLAGS = \
	$(RENA_CFLAGS) \
	-I$(top_srcdir)/src/include \
	-Wall -lm

librena_la_LIBADD = \
	$(RENA_LIBS)

if HAVE_LIBPEAS
librena_la_SOURCES += \
	rena-plugins-engine.c
librena_HEADERS += \
	rena-plugins-engine.h
endif

if HAVE_GUDEV
librena_la_SOURCES += \
	rena-device-client.c
librena_HEADERS += \
	rena-device-client.h
endif


#
# WIN32
#
if WIN32
AM_CFLAGS = \
	-mms-bitfields

AM_LDFLAGS = \
	-Wl,-subsystem,windows

librena_la_LIBADD += \
	win32/libwin32.la
endif


librena_la_LDFLAGS = \
	-avoid-version \
	-export-dynamic \
	-no-undefined \
	-export-symbols-regex "^[^_].*"


#
# pkgconfig
#
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_in_files = librena.pc.in
pkgconfig_DATA = $(pkgconfig_in_files:.pc.in=.pc)

%.pc: %.pc.in
	sed "s,\@VERSION\@,$(VERSION),g; s,\@prefix\@,$(prefix),g; s,\@libdir\@,$(libdir),g; s,\@includedir\@,$(includedir),g" < $< > $@

EXTRA_DIST += \
	$(pkgconfig_in_files)

DISTCLEANFILES += \
	$(pkgconfig_DATA)


#
# Build sources
#
rena-window-ui.h: rena-window.ui Makefile
	$(AM_V_GEN) xdt-csource --strip-comments --strip-content --static --name=rena_window_ui $< > $@

BUILT_SOURCES = \
	rena-window-ui.h

DISTCLEANFILES += \
	rena-window-ui.h


#
# Rena binary
#
bin_PROGRAMS = rena

rena_SOURCES = \
	main.c

rena_CFLAGS = \
	$(librena_la_CFLAGS)

rena_LDADD = \
	librena.la

if WIN32
rena-res.o: win32/rena.rc
	$(WINDRES) -i win32/rena.rc --input-format=rc -o rena-res.o -O coff
rena_LDADD += rena-res.o
endif

EXTRA_DIST += \
	rena-window.ui
