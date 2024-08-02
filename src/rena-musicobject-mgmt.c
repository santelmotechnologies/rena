/*****************************************************************************/
/* Copyright (C) 2024 Santelmo Technologies <santelmotechnologies@gmail.com> */
/*                                                                           */
/* This program is free software: you can redistribute it and/or modify      */
/* it under the terms of the GNU General Public License as published by      */
/* the Free Software Foundation, either version 3 of the License, or         */
/* (at your option) any later version.                                       */
/*                                                                           */
/* This program is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* along with this program.  If not, see <http://www.gnu.org/licenses/>.     */
/*****************************************************************************/

#include "rena-musicobject-mgmt.h"

#include <glib.h>
#include <glib/gstdio.h>

#include "rena-file-utils.h"
#include "rena-playlists-mgmt.h"
#include "rena-tags-mgmt.h"
#include "rena-music-enum.h"

RenaMusicobject *
new_musicobject_from_file(const gchar *file, const gchar *provider)
{
	RenaMusicobject *mobj = NULL;
	gchar *mime_type = NULL;
	gboolean ret = FALSE;

	CDEBUG(DBG_MOBJ, "Creating new musicobject from file: %s", file);

	mime_type = rena_file_get_music_type(file);

	mobj = g_object_new (RENA_TYPE_MUSICOBJECT,
	                     "file", file,
	                     "source", FILE_LOCAL,
	                     "provider", provider,
	                     "mime-type", mime_type,
	                     NULL);

	g_free (mime_type);

	ret = rena_musicobject_set_tags_from_file (mobj, file);

	if (G_LIKELY(ret))
		return mobj;
	else {
		g_critical("Fail to create musicobject from file");
		g_object_unref(mobj);
	}

	return NULL;
}

RenaMusicobject *
new_musicobject_from_db(RenaDatabase *cdbase, gint location_id)
{
	RenaPreparedStatement *statement = NULL;
	RenaMusicEnum *enum_map = NULL;
	RenaMusicobject *mobj = NULL;

	CDEBUG(DBG_MOBJ, "Creating new musicobject with location id: %d", location_id);

	const gchar *sql =
		"SELECT LOCATION.name, PROVIDER_TYPE.name, PROVIDER.name, MIME_TYPE.name, TRACK.title, ARTIST.name, ALBUM.name, GENRE.name, COMMENT.name, YEAR.year, TRACK.track_no, TRACK.length, TRACK.bitrate, TRACK.channels, TRACK.samplerate \
		 FROM LOCATION, PROVIDER_TYPE, PROVIDER, MIME_TYPE, TRACK, ARTIST, ALBUM, GENRE, COMMENT, YEAR \
		 WHERE TRACK.location = ? AND PROVIDER.id = TRACK.provider AND PROVIDER_TYPE.id = PROVIDER.type AND MIME_TYPE.id = TRACK.file_type AND ARTIST.id = TRACK.artist AND ALBUM.id = TRACK.album AND GENRE.id = TRACK.genre AND COMMENT.id = TRACK.comment AND YEAR.id = TRACK.year \
		 AND LOCATION.id = ?";

	statement = rena_database_create_statement (cdbase, sql);
	rena_prepared_statement_bind_int (statement, 1, location_id);
	rena_prepared_statement_bind_int (statement, 2, location_id);

	if (rena_prepared_statement_step (statement))
	{
		mobj = g_object_new (RENA_TYPE_MUSICOBJECT,
		                     "file", rena_prepared_statement_get_string (statement, 0),
		                     "provider", rena_prepared_statement_get_string (statement, 2),
		                     "mime-type", rena_prepared_statement_get_string (statement, 3),
		                     "title", rena_prepared_statement_get_string (statement, 4),
		                     "artist", rena_prepared_statement_get_string (statement, 5),
		                     "album", rena_prepared_statement_get_string (statement, 6),
		                     "genre", rena_prepared_statement_get_string (statement, 7),
		                     "comment", rena_prepared_statement_get_string (statement, 8),
		                     "year", rena_prepared_statement_get_int (statement, 9),
		                     "track-no", rena_prepared_statement_get_int (statement, 10),
		                     "length", rena_prepared_statement_get_int (statement, 11),
		                     "bitrate", rena_prepared_statement_get_int (statement, 12),
		                     "channels", rena_prepared_statement_get_int (statement, 13),
		                     "samplerate", rena_prepared_statement_get_int (statement, 14),
		                     NULL);

		enum_map = rena_music_enum_get ();
		rena_musicobject_set_source (mobj,
			rena_music_enum_map_get(enum_map,
				rena_prepared_statement_get_string (statement, 1)));
		g_object_unref (enum_map);
	}
	else
	{
		g_critical("Track with location id : %d not found in DB", location_id);
	}

	rena_prepared_statement_free (statement);

	return mobj;
}

RenaMusicobject *
new_musicobject_from_location(const gchar *uri, const gchar *name)
{
	RenaMusicobject *mobj = NULL;

	CDEBUG(DBG_MOBJ, "Creating new musicobject to location: %s", uri);

	mobj = g_object_new (RENA_TYPE_MUSICOBJECT,
	                     "file",      uri,
	                     "source", FILE_HTTP,
	                     NULL);
	if (name)
		rena_musicobject_set_title(mobj, name);

	return mobj;
}

void
rena_update_musicobject_change_tag(RenaMusicobject *mobj, gint changed, RenaMusicobject *nmobj)
{
	if (!changed)
		return;

	CDEBUG(DBG_VERBOSE, "Tags Updates: 0x%x", changed);

	if (changed & TAG_TNO_CHANGED) {
		rena_musicobject_set_track_no(mobj, rena_musicobject_get_track_no(nmobj));
	}
	if (changed & TAG_TITLE_CHANGED) {
		rena_musicobject_set_title(mobj, rena_musicobject_get_title(nmobj));
	}
	if (changed & TAG_ARTIST_CHANGED) {
		rena_musicobject_set_artist (mobj, rena_musicobject_get_artist(nmobj));
	}
	if (changed & TAG_ALBUM_CHANGED) {
		rena_musicobject_set_album(mobj, rena_musicobject_get_album(nmobj));
	}
	if (changed & TAG_GENRE_CHANGED) {
		rena_musicobject_set_genre(mobj, rena_musicobject_get_genre(nmobj));
	}
	if (changed & TAG_YEAR_CHANGED) {
		rena_musicobject_set_year(mobj, rena_musicobject_get_year(nmobj));
	}
	if (changed & TAG_COMMENT_CHANGED) {
		rena_musicobject_set_comment(mobj, rena_musicobject_get_comment(nmobj));
	}
}

RenaMusicobject *
rena_database_get_artist_and_title_song (RenaDatabase *cdbase,
                                           const gchar    *artist,
                                           const gchar    *title)
{
	RenaMusicobject *mobj = NULL;
	gint location_id = 0;

	const gchar *sql =
		"SELECT LOCATION.id "
		"FROM TRACK, ARTIST, PROVIDER, LOCATION "
		"WHERE ARTIST.id = TRACK.artist "
		"AND LOCATION.id = TRACK.location "
		"AND TRACK.provider = PROVIDER.id AND PROVIDER.visible <> 0 "
		"AND TRACK.title = ? COLLATE NOCASE "
		"AND ARTIST.name = ? COLLATE NOCASE "
		"ORDER BY RANDOM() LIMIT 1;";

	RenaPreparedStatement *statement = rena_database_create_statement (cdbase, sql);
	rena_prepared_statement_bind_string (statement, 1, title);
	rena_prepared_statement_bind_string (statement, 2, artist);

	if (rena_prepared_statement_step (statement)) {
		location_id = rena_prepared_statement_get_int (statement, 0);
		mobj = new_musicobject_from_db (cdbase, location_id);
	}

	rena_prepared_statement_free (statement);

	return mobj;
}
