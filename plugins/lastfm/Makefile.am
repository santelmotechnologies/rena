plugindir = $(libdir)/rena/plugins/lastfm

AM_CPPFLAGS = \
	-I$(top_srcdir) 	\
	$(LIBPEAS_CFLAGS)		\
	$(LIBPEAS_GTK_CFLAGS)

plugin_LTLIBRARIES = libplastfm.la

libplastfm_la_SOURCES =      \
	rena-lastfm-plugin.c

libplastfm_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libplastfm_la_CFLAGS = \
	$(RENA_CFLAGS) \
	$(LIBCLASTFM_CFLAGS)

libplastfm_la_LIBADD = \
	$(RENA_LIBS) \
	$(LIBCLASTFM_LIBS) \
	$(top_builddir)/src/librena.la

rena-lastfm-menu-ui.h: rena-lastfm-menu.ui Makefile
	$(AM_V_GEN) xdt-csource --strip-comments --strip-content --static --name=lastfm_menu_ui $< > $@

plugin_DATA = lastfm.plugin

EXTRA_DIST = \
	$(plugin_DATA) \
	rena-lastfm-menu.ui

DISTCLEANFILES = \
	rena-lastfm-menu-ui.h

BUILT_SOURCES = \
	rena-lastfm-menu-ui.h
