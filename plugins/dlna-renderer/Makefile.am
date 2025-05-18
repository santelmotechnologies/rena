plugindir = $(libdir)/rena/plugins/dlna-renderer

AM_CPPFLAGS = \
	-I$(top_srcdir) 	\
	$(LIBPEAS_CFLAGS)		\
	$(LIBPEAS_GTK_CFLAGS)

plugin_LTLIBRARIES = libpdlnarenderer.la

libpdlnarenderer_la_SOURCES =		\
	rena-dlna-renderer-plugin.h 	\
	rena-dlna-renderer-plugin.c

libpdlnarenderer_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libpdlnarenderer_la_CFLAGS = \
	$(RENA_CFLAGS)

libpdlnarenderer_la_LIBADD = \
	$(RENA_LIBS) \
	$(top_builddir)/src/librena.la

if HAVE_GRILO3
libpdlnarenderer_la_CFLAGS += $(GRILO3_CFLAGS)
libpdlnarenderer_la_LIBADD += $(GRILO3_LIBS)
endif
if HAVE_GRILO2
libpdlnarenderer_la_CFLAGS += $(GRILO2_CFLAGS)
libpdlnarenderer_la_LIBADD += $(GRILO2_LIBS)
endif

plugin_DATA = dlna-renderer.plugin

EXTRA_DIST = $(plugin_DATA)
