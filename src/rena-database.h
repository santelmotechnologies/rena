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

#ifndef RENA_DATABASE_H
#define RENA_DATABASE_H

#include <glib-object.h>
#include "rena-prepared-statement.h"
#include "rena-musicobject.h"

G_BEGIN_DECLS

#define RENA_TYPE_DATABASE (rena_database_get_type())
#define RENA_DATABASE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_DATABASE, RenaDatabase))
#define RENA_DATABASE_CONST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_DATABASE, RenaDatabase const))
#define RENA_DATABASE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), RENA_TYPE_DATABASE, RenaDatabaseClass))
#define RENA_IS_DATABASE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_DATABASE))
#define RENA_IS_DATABASE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), RENA_TYPE_DATABASE))
#define RENA_DATABASE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), RENA_TYPE_DATABASE, RenaDatabaseClass))

typedef struct _RenaDatabase RenaDatabase;
typedef struct _RenaDatabaseClass RenaDatabaseClass;
typedef struct _RenaDatabasePrivate RenaDatabasePrivate;

struct _RenaDatabase
{
	GObject parent;

	/*< private >*/
	RenaDatabasePrivate *priv;
};

struct _RenaDatabaseClass
{
	GObjectClass parent_class;
	void (*playlists_change) (RenaDatabase *database);
	void (*tracks_change)    (RenaDatabase *database);
};

gboolean
rena_database_exec_query (RenaDatabase *database,
                            const gchar *query);

RenaPreparedStatement *
rena_database_create_statement (RenaDatabase *database, const gchar *sql);

void
rena_database_release_statement (RenaDatabase *database, RenaPreparedStatement *statement);

void
rena_database_begin_transaction (RenaDatabase *database);

void
rena_database_commit_transaction (RenaDatabase *database);

gint
rena_database_find_location (RenaDatabase *database, const gchar *location);

gint
rena_database_find_provider (RenaDatabase *database, const gchar *provider);

gint
rena_database_find_provider_type (RenaDatabase *database, const gchar *provider_type);

gint
rena_database_find_mime_type (RenaDatabase *database, const gchar *mime_type);

gint
rena_database_find_artist (RenaDatabase *database, const gchar *artist);

gint
rena_database_find_album (RenaDatabase *database, const gchar *album);

gint
rena_database_find_genre (RenaDatabase *database, const gchar *genre);

gint
rena_database_find_comment (RenaDatabase *database, const gchar *comment);

gint
rena_database_find_year (RenaDatabase *database, gint year);

gint
rena_database_find_playlist (RenaDatabase *database, const gchar *playlist);

gint
rena_database_find_radio (RenaDatabase *database, const gchar *radio);

gint
rena_database_add_new_location (RenaDatabase *database, const gchar *location);

gint
rena_database_add_new_provider_type (RenaDatabase *database, const gchar *provider_type);

gint
rena_database_add_new_mime_type (RenaDatabase *database, const gchar *mime_type);

gint
rena_database_add_new_artist (RenaDatabase *database, const gchar *artist);

gint
rena_database_add_new_album (RenaDatabase *database, const gchar *album);

gint
rena_database_add_new_genre (RenaDatabase *database, const gchar *genre);

gint
rena_database_add_new_comment (RenaDatabase *database, const gchar *comment);

gint
rena_database_add_new_year (RenaDatabase *database, guint year);

gint
rena_database_add_new_playlist (RenaDatabase *database, const gchar *playlist);

void
rena_database_add_playlist_track (RenaDatabase *database, gint playlist_id, const gchar *file);

gboolean
rena_database_playlist_has_track (RenaDatabase *database, gint playlist_id, const gchar *file);

void
rena_database_delete_playlist_track (RenaDatabase *database, gint playlist_id, const gchar *file);

gint
rena_database_add_new_radio (RenaDatabase *database, const gchar *radio);

void
rena_database_forget_location (RenaDatabase *database, gint location_id);

void
rena_database_forget_track (RenaDatabase *database, const gchar *file);

void
rena_database_add_radio_track (RenaDatabase *database, gint radio_id, const gchar *uri);

void
rena_database_update_playlist_name (RenaDatabase *database, const gchar *old_name, const gchar *new_name);

void
rena_database_update_radio_name (RenaDatabase *database, const gchar *old_name, const gchar *new_name);

void
rena_database_delete_dir (RenaDatabase *database, const gchar *dir_name);

gint
rena_database_get_playlist_count (RenaDatabase *database);

void
rena_database_flush_playlist (RenaDatabase *database, gint playlist_id);

void
rena_database_delete_playlist (RenaDatabase *database, const gchar *playlist);

void
rena_database_flush_radio (RenaDatabase *database, gint radio_id);

void
rena_database_delete_radio (RenaDatabase *database, const gchar *radio);

void
rena_database_add_new_musicobject (RenaDatabase *database, RenaMusicobject *mobj);

gchar *
rena_database_get_filename_from_location_id (RenaDatabase *database, gint location_id);

void
rena_database_update_local_files_change_tag (RenaDatabase *database, GArray *loc_arr, gint changed, RenaMusicobject *mobj);

gchar**
rena_database_get_playlist_names (RenaDatabase *database);

void
rena_database_flush (RenaDatabase *database);

void
rena_database_flush_stale_entries (RenaDatabase *database);

gint
rena_database_get_artist_count (RenaDatabase *database);

gint
rena_database_get_album_count (RenaDatabase *database);

gint
rena_database_get_track_count (RenaDatabase *database);

void
rena_database_change_playlists_done(RenaDatabase *database);

void
rena_database_compatibilize_version (RenaDatabase *database);

gint
rena_database_get_version (RenaDatabase *database);

gboolean
rena_database_start_successfully (RenaDatabase *database);

const gchar *
rena_database_get_last_error (RenaDatabase *database);

RenaDatabase* rena_database_get (void);

G_END_DECLS

#endif /* RENA_DATABASE_H */
