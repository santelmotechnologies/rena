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
#ifndef RENA_PLAYLIST_H
#define RENA_PLAYLIST_H

#include <gtk/gtk.h>
#include "rena-backend.h"
#include "rena-database.h"

#define RENA_TYPE_PLAYLIST                  (rena_playlist_get_type ())
#define RENA_PLAYLIST(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_PLAYLIST, RenaPlaylist))
#define RENA_IS_PLAYLIST(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_PLAYLIST))
#define RENA_PLAYLIST_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), RENA_TYPE_PLAYLIST, RenaPlaylistClass))
#define RENA_IS_PLAYLIST_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), RENA_TYPE_PLAYLIST))
#define RENA_PLAYLIST_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), RENA_TYPE_PLAYLIST, RenaPlaylistClass))

typedef struct _RenaPlaylist RenaPlaylist;

typedef struct {
	GtkScrolledWindowClass __parent__;
	void (*playlist_set_track) (RenaPlaylist *playlist, RenaMusicobject *mobj);
	void (*playlist_change_tags) (RenaPlaylist *playlist, gint changes, RenaMusicobject *mobj);
	void (*playlist_changed) (RenaPlaylist *playlist);
} RenaPlaylistClass;

/* Columns in current playlist view */

enum curplaylist_columns {
	P_MOBJ_PTR,
	P_QUEUE,
	P_BUBBLE,
	P_STATUS_PIXBUF,
	P_TRACK_NO,
	P_TITLE,
	P_ARTIST,
	P_ALBUM,
	P_GENRE,
	P_BITRATE,
	P_YEAR,
	P_COMMENT,
	P_LENGTH,
	P_FILENAME,
	P_MIMETYPE,
	P_PLAYED,
	N_P_COLUMNS
};

/* Current playlist movement */

typedef enum {
	PLAYLIST_NONE,
	PLAYLIST_CURR,
	PLAYLIST_NEXT,
	PLAYLIST_PREV
} RenaUpdateAction;

void save_selected_playlist   (GtkAction *action, RenaPlaylist *cplaylist);
void save_current_playlist    (GtkAction *action, RenaPlaylist *cplaylist);
void export_current_playlist  (GtkAction *action, RenaPlaylist *cplaylist);
void export_selected_playlist (GtkAction *action, RenaPlaylist *cplaylist);

void rena_playlist_remove_selection (RenaPlaylist *playlist);
void rena_playlist_crop_selection   (RenaPlaylist *playlist);
void rena_playlist_crop_music_type  (RenaPlaylist *playlist, RenaMusicSource music_type);
void rena_playlist_remove_all       (RenaPlaylist *playlist);

void rena_playlist_go_prev_track    (RenaPlaylist *playlist);
void rena_playlist_go_any_track     (RenaPlaylist *playlist);
void rena_playlist_go_next_track    (RenaPlaylist *playlist);
void rena_playlist_stopped_playback (RenaPlaylist *playlist);

void               rena_playlist_show_current_track (RenaPlaylist *playlist);
void               rena_playlist_set_track_error    (RenaPlaylist *playlist, GError *error);

void select_numered_path_of_current_playlist(RenaPlaylist *cplaylist, gint path_number, gboolean center);

void update_current_playlist_view_playback_state_cb (RenaBackend *backend, GParamSpec *pspec, RenaPlaylist *cplaylist);

RenaMusicobject * current_playlist_mobj_at_path(GtkTreePath *path,
						  RenaPlaylist *cplaylist);

void rena_playlist_toggle_queue_selected (RenaPlaylist *cplaylist);

void rena_playlist_update_current_track(RenaPlaylist *cplaylist, gint changed, RenaMusicobject *nmobj);
void
rena_playlist_append_single_song(RenaPlaylist *cplaylist, RenaMusicobject *mobj);
void
rena_playlist_append_mobj_and_play(RenaPlaylist *cplaylist, RenaMusicobject *mobj);
void
rena_playlist_append_mobj_list(RenaPlaylist *cplaylist, GList *list);

gboolean
rena_mobj_list_already_has_title_of_artist (GList       *list,
                                              const gchar *title,
                                              const gchar *artist);

gboolean
rena_playlist_already_has_title_of_artist (RenaPlaylist *cplaylist,
                                             const gchar    *title,
                                             const gchar    *artist);

gboolean
rena_playlist_select_title_of_artist (RenaPlaylist *cplaylist,
                                        const gchar    *title,
                                        const gchar    *artist);

GList *rena_playlist_get_mobj_list(RenaPlaylist* cplaylist);
GList *rena_playlist_get_selection_mobj_list(RenaPlaylist* cplaylist);
GList *rena_playlist_get_selection_ref_list(RenaPlaylist *cplaylist);

void rena_playlist_save_playlist_state (RenaPlaylist* cplaylist);
void rena_playlist_init_playlist_state (RenaPlaylist* cplaylist);
RenaMusicobject *rena_playlist_get_selected_musicobject(RenaPlaylist* cplaylist);
gboolean rena_playlist_propagate_event(RenaPlaylist* cplaylist, GdkEventKey *event);

void rena_playlist_activate_path        (RenaPlaylist* cplaylist, GtkTreePath *path);
void rena_playlist_activate_unique_mobj (RenaPlaylist* cplaylist, RenaMusicobject *mobj);

gint rena_playlist_get_no_tracks      (RenaPlaylist *playlist);
gint rena_playlist_get_no_unplayed_tracks (RenaPlaylist *playlist);

gint rena_playlist_get_total_playtime (RenaPlaylist *playlist);

gboolean rena_playlist_has_queue(RenaPlaylist* cplaylist);

gboolean rena_playlist_is_changing (RenaPlaylist* cplaylist);
void     rena_playlist_set_changing (RenaPlaylist* cplaylist, gboolean changing);

GtkWidget    *rena_playlist_get_view  (RenaPlaylist* cplaylist);
GtkTreeModel *rena_playlist_get_model (RenaPlaylist* cplaylist);

GtkUIManager   *rena_playlist_get_context_menu(RenaPlaylist* cplaylist);

gint            rena_playlist_append_plugin_action (RenaPlaylist *cplaylist, GtkActionGroup *action_group, const gchar *menu_xml);
void            rena_playlist_remove_plugin_action (RenaPlaylist *cplaylist, GtkActionGroup *action_group, gint merge_id);

RenaDatabase *rena_playlist_get_database(RenaPlaylist* cplaylist);

RenaPlaylist *rena_playlist_new  (void);


#endif /* RENA_PLAYLIST_H */
