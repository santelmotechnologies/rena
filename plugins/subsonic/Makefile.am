plugindir = $(libdir)/rena/plugins/subsonic

AM_CPPFLAGS = \
	-I$(top_srcdir) 	\
	$(LIBPEAS_CFLAGS)		\
	$(LIBPEAS_GTK_CFLAGS)

plugin_LTLIBRARIES = libpsubsonic.la

libpsubsonic_la_SOURCES = \
	rena-subsonic-api.c \
	rena-subsonic-api.h \
	rena-subsonic-preferences.c \
	rena-subsonic-preferences.h \
	rena-subsonic-plugin.c \
	rena-subsonic-plugin.h

libpsubsonic_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libpsubsonic_la_CFLAGS = \
	$(RENA_CFLAGS) \
	$(GRILO_NET_CFLAGS)

libpsubsonic_la_LIBADD = \
	$(RENA_LIBS) \
	$(GRILO_NET_LIBS) \
	$(top_builddir)/src/librena.la

if HAVE_GRILO3
libpsubsonic_la_CFLAGS += $(GRILO_NET3_CFLAGS)
libpsubsonic_la_LIBADD += $(GRILO_NET3_LIBS)
endif
if HAVE_GRILO2
libpsubsonic_la_CFLAGS += $(GRILO_NET2_CFLAGS)
libpsubsonic_la_LIBADD += $(GRILO_NET2_LIBS)
endif

plugin_DATA = subsonic.plugin

EXTRA_DIST = $(plugin_DATA)
