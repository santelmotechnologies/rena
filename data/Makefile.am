SUBDIRS = icons ui

appstreamdir = $(datadir)/metainfo/
appstream_in_files = io.github.rena_music_player.metainfo.xml.in
appstream_DATA = $(appstream_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@

desktopdir=$(datadir)/applications
desktop_in_files = rena.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

imagedir = $(datadir)/pixmaps/rena
image_DATA = \
	album.png  \
	artist.png \
	track.png  \
	cover.png  \
	genre.png

man_MANS = rena.1

EXTRA_DIST = \
	$(appstream_in_files) \
	$(desktop_in_files) \
	$(image_DATA)       \
	$(man_MANS)

CLEANFILES = \
	$(appstream_DATA) \
	$(desktop_DATA)
