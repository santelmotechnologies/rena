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

#ifndef RENA_PLAYLISTS_MGMT_H
#define RENA_PLAYLISTS_MGMT_H

#include "rena-database.h"
#include "rena-playlist.h"
#include "rena.h"

/* Playlist management */

typedef enum {
	SAVE_COMPLETE,
	SAVE_SELECTED
} RenaPlaylistActionRange;

#define SAVE_PLAYLIST_STATE         "con_playlist"

gchar *get_playlist_name(RenaPlaylistActionRange type, GtkWidget *parent);
void add_playlist_current_playlist(gchar *splaylist, RenaApplication *rena);
GList * add_playlist_to_mobj_list(RenaDatabase *cdbase, const gchar *playlist, GList *list);
GList *add_radio_to_mobj_list(RenaDatabase *cdbase, const gchar *playlist, GList *list);
gboolean delete_existing_item_dialog(const gchar *item, GtkWidget *parent);
gchar* rename_playlist_dialog(const gchar *oplaylist, GtkWidget *parent);
GIOChannel *create_m3u_playlist(gchar *file);
gint save_m3u_playlist(GIOChannel *chan, gchar *playlist, gchar *filename, RenaDatabase *cdbase);
gchar *playlist_export_dialog_get_filename(const gchar *prefix, GtkWidget *parent);

void export_playlist (RenaPlaylist* cplaylist, RenaPlaylistActionRange choice);
void save_playlist(RenaPlaylist* cplaylist, gint playlist_id, RenaPlaylistActionRange type);
void new_playlist(RenaPlaylist* cplaylist, const gchar *playlist, RenaPlaylistActionRange type);
void append_playlist(RenaPlaylist* cplaylist, const gchar *playlist, RenaPlaylistActionRange type);

void
rena_playlist_database_update_playlist (RenaDatabase *cdbase, const gchar *playlist, GList *mlist);
void
rena_playlist_database_insert_playlist (RenaDatabase *cdbase, const gchar *playlist, GList *mlist);

void rena_playlist_save_selection (RenaPlaylist *playlist, const gchar *name);
void rena_playlist_save_playlist  (RenaPlaylist *playlist, const gchar *name);

GList *
rena_pl_parser_append_mobj_list_by_extension (GList *mlist, const gchar *file);
GSList *rena_pl_parser_parse_from_file_by_extension (const gchar *filename);
GSList *rena_totem_pl_parser_parse_from_uri(const gchar *uri);
void rena_pl_parser_open_from_file_by_extension(const gchar *file, RenaApplication *rena);
gchar * rena_pl_get_first_playlist_item (const gchar *uri);

gchar *
new_radio (RenaPlaylist *playlist, const gchar *uri, const gchar *basename);

void update_playlist_changes_on_menu (RenaPlaylist *playlist);

#endif /* RENA_PLAYLISTS_MGMT_H */
