plugindir = $(libdir)/rena/plugins/ampache

AM_CPPFLAGS = \
	-I$(top_srcdir) 	\
	$(LIBPEAS_CFLAGS)		\
	$(LIBPEAS_GTK_CFLAGS)

plugin_LTLIBRARIES = libpampache.la

libpampache_la_SOURCES =		\
	rena-ampache-plugin.h 	\
	rena-ampache-plugin.c

libpampache_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libpampache_la_CFLAGS = \
	$(RENA_CFLAGS) \
	$(GRILO_NET_CFLAGS)

libpampache_la_LIBADD = \
	$(RENA_LIBS) \
	$(GRILO_NET_LIBS) \
	$(top_builddir)/src/librena.la

if HAVE_GRILO3
libpampache_la_CFLAGS += $(GRILO_NET3_CFLAGS)
libpampache_la_LIBADD += $(GRILO_NET3_LIBS)
endif
if HAVE_GRILO2
libpampache_la_CFLAGS += $(GRILO_NET2_CFLAGS)
libpampache_la_LIBADD += $(GRILO_NET2_LIBS)
endif

plugin_DATA = ampache.plugin

EXTRA_DIST = $(plugin_DATA)
