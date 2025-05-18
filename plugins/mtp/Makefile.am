plugindir = $(libdir)/rena/plugins/mtp

AM_CPPFLAGS = \
	-I$(top_srcdir) 	\
	$(LIBPEAS_CFLAGS)		\
	$(LIBPEAS_GTK_CFLAGS)

plugin_LTLIBRARIES = libpmtp.la

libpmtp_la_SOURCES = \
	rena-devices-mtp.c \
	rena-mtp-musicobject.c \
	rena-mtp-musicobject.h \
	rena-mtp-thread.c \
	rena-mtp-thread.h \
	rena-mtp-thread-data.c \
	rena-mtp-thread-data.h

libpmtp_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS)

libpmtp_la_CFLAGS = \
	$(RENA_CFLAGS) \
	$(LIBMTP_CFLAGS)

libpmtp_la_LIBADD = \
	$(RENA_LIBS) \
	$(LIBMTP_LIBS) \
	$(top_builddir)/src/librena.la

plugin_DATA = mtp.plugin

EXTRA_DIST = $(plugin_DATA)
