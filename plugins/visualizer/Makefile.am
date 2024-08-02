plugindir = $(libdir)/rena/plugins/visualizer

AM_CPPFLAGS = \
	-I$(top_srcdir) 	\
	$(LIBPEAS_CFLAGS)		\
	$(LIBPEAS_GTK_CFLAGS)

plugin_LTLIBRARIES = libvisualizer.la

libvisualizer_la_SOURCES =      \
	rena-visualizer-plugin.h \
	rena-visualizer-plugin.c \
	rena-visualizer.h \
	rena-visualizer.c \
	rena-visualizer-particle.h \
	rena-visualizer-particle.c

libvisualizer_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libvisualizer_la_CFLAGS = \
	$(RENA_CFLAGS)

libvisualizer_la_LIBADD = \
	$(RENA_LIBS) \
	$(top_builddir)/src/librena.la

plugin_DATA = visualizer.plugin

EXTRA_DIST = \
	$(plugin_DATA)
