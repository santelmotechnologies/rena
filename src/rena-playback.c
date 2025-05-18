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

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "rena-playback.h"

#include "rena-tags-dialog.h"
#include "rena-tagger.h"
#include "rena-musicobject-mgmt.h"
#include "rena-favorites.h"
#include "rena-file-utils.h"
#include "rena-utils.h"
#include "rena-window.h"
#include "rena.h"

static void rena_playback_update_current_album_art (RenaApplication *rena, RenaMusicobject *mobj);

/**********************/
/* Playback functions */
/**********************/

/* Play prev track in current playlist */

void rena_playback_prev_track(RenaApplication *rena)
{
	RenaBackend *backend;
	RenaPlaylist *playlist;

	CDEBUG(DBG_BACKEND, "Want to play a song previously played");

	/* Are we playing right now ? */

	backend = rena_application_get_backend (rena);
	if (rena_backend_get_state (backend) == ST_STOPPED)
		return;

	playlist = rena_application_get_playlist (rena);
	rena_playlist_go_prev_track (playlist);
}

/* Start playback of a new track, or resume playback of current track */

void rena_playback_play_pause_resume(RenaApplication *rena)
{
	RenaBackend *backend;
	RenaPlaylist *playlist;

	CDEBUG(DBG_BACKEND, "Play pause or resume a track based on the current state");

	/* New action is based on the current state */

	/************************************/
	/* State     Action                 */
	/*                                  */
	/* Playing   Pause playback         */
	/* Paused    Resume playback        */
	/* Stopped   Start playback         */
	/************************************/

	backend = rena_application_get_backend (rena);

	switch (rena_backend_get_state (backend)) {
	case ST_PLAYING:
		rena_backend_pause (backend);
		break;
	case ST_PAUSED:
		rena_backend_resume (backend);
		break;
	case ST_STOPPED:
		playlist = rena_application_get_playlist (rena);
		rena_playlist_go_any_track (playlist);
		break;
	case ST_BUFFERING:
	default:
		break;
	}
}

/* Stop the playback */

void rena_playback_stop(RenaApplication *rena)
{
	RenaBackend *backend;
	RenaPlaylist *playlist;

	CDEBUG(DBG_BACKEND, "Stopping the current song");

	backend = rena_application_get_backend (rena);
	if (rena_backend_get_state (backend) == ST_STOPPED)
		return;

	rena_backend_stop (backend);

	playlist = rena_application_get_playlist (rena);
	rena_playlist_stopped_playback (playlist);
}

/* Play next song when terminate a song. */

void rena_advance_playback (RenaApplication *rena)
{
	RenaPlaylist *playlist;

	CDEBUG(DBG_BACKEND, "Advancing to next track");

	playlist = rena_application_get_playlist (rena);
	rena_playlist_go_next_track (playlist);
}

/* Play next track in current_playlist */

void rena_playback_next_track(RenaApplication *rena)
{
	RenaBackend *backend;

	CDEBUG(DBG_BACKEND, "Want to advancing to next track");

	/* Are we playing right now ? */

	backend = rena_application_get_backend (rena);
	if (rena_backend_get_state (backend) == ST_STOPPED)
		return;

	/* Play a new song */
	rena_advance_playback (rena);
}

gboolean
rena_playback_can_go_prev (RenaApplication *rena)
{
	RenaPlaylist *playlist;
	gboolean can_go_prev = FALSE;

	playlist = rena_application_get_playlist (rena);
	if (rena_playlist_get_no_unplayed_tracks(playlist) < rena_playlist_get_no_tracks(playlist))
		can_go_prev = TRUE;

	return can_go_prev;
}

gboolean
rena_playback_can_go_next (RenaApplication *rena)
{
	RenaPlaylist *playlist;
	gboolean can_go_next = FALSE;

	playlist = rena_application_get_playlist (rena);
	if (rena_playlist_get_no_unplayed_tracks(playlist) > 0)
		can_go_next = TRUE;

	return can_go_next;
}

gint
rena_playback_get_no_tracks (RenaApplication *rena)
{
	RenaPlaylist *playlist;
	playlist = rena_application_get_playlist (rena);
	return rena_playlist_get_no_tracks(playlist);
}


/******************************************/
/* Update playback state based on backend */
/******************************************/

void
rena_playback_set_playlist_track (RenaPlaylist *playlist, RenaMusicobject *mobj, RenaApplication *rena)
{
	RenaBackend *backend;
	RenaToolbar *toolbar;
	RenaFavorites *favorites;

	CDEBUG(DBG_BACKEND, "Set track activated on playlist");

	/* Stop to set ready and clear all info */
	backend = rena_application_get_backend (rena);
	rena_backend_stop (backend);

	if (!mobj)
		return;

	/* Play new song. */
	rena_backend_set_musicobject (backend, mobj);
	rena_backend_play (backend);

	/* Update current song info */
	toolbar = rena_application_get_toolbar (rena);
	rena_toolbar_set_title (toolbar, mobj);
	rena_toolbar_update_progress (toolbar, rena_musicobject_get_length(mobj), 0);

	/* Update album art */
	rena_playback_update_current_album_art (rena, mobj);

	/* Set favorites icon */
	favorites = rena_favorites_get ();
	if (rena_favorites_contains_song(favorites, mobj))
		rena_toolbar_set_favorite_icon (toolbar, TRUE);
	g_object_unref (favorites);
}

void
rena_backend_finished_song (RenaBackend *backend, RenaApplication *rena)
{
	rena_advance_playback(rena);
}

void
rena_backend_finished_error (RenaBackend     *backend,
                               const GError      *error,
                               RenaApplication *rena)
{
	RenaPreferences *preferences;
	preferences = rena_preferences_get();

	if (!rena_preferences_get_ignore_errors (preferences)) {
		rena_window_show_backend_error_dialog (rena);
	}
	else {
		rena_advance_playback(rena);
	}

	g_object_unref(preferences);
}

void
rena_backend_tags_changed (RenaBackend *backend, gint changed, RenaApplication *rena)
{
	RenaPlaylist *playlist;
	RenaToolbar *toolbar;
	RenaMusicobject *nmobj;

	if(rena_backend_get_state (backend) != ST_PLAYING)
		return;

	nmobj = rena_backend_get_musicobject(backend);

	/* Update change on gui */
	toolbar = rena_application_get_toolbar (rena);
	rena_toolbar_set_title(toolbar, nmobj);

	/* Update the playlist */

	playlist = rena_application_get_playlist (rena);
	rena_playlist_update_current_track (playlist, changed, nmobj);
}

static void
rena_playback_update_current_album_art (RenaApplication *rena, RenaMusicobject *mobj)
{
	RenaToolbar *toolbar;
	RenaPreferences *preferences;
	RenaArtCache *art_cache;

	gchar *album_path = NULL, *path = NULL;

	CDEBUG(DBG_INFO, "Update album art");

	if (G_UNLIKELY(!mobj))
		return;

	preferences = rena_application_get_preferences (rena);
	if (!rena_preferences_get_show_album_art (preferences))
		return;

	art_cache = rena_application_get_art_cache (rena);
	album_path = rena_art_cache_get_album_uri (art_cache,
	                                             rena_musicobject_get_artist(mobj),
	                                             rena_musicobject_get_album(mobj));

	if (album_path == NULL) {
		if (!rena_musicobject_is_local_file(mobj))
			return;

		path = g_path_get_dirname(rena_musicobject_get_file(mobj));

		album_path = get_pref_image_path_dir (preferences, path);
		if (!album_path)
			album_path = get_image_path_from_dir(path);

		g_free(path);
	}

	toolbar = rena_application_get_toolbar (rena);
	rena_toolbar_set_image_album_art (toolbar, album_path);
	g_free(album_path);
}

void
rena_playback_show_current_album_art (GObject *object, RenaApplication *rena)
{
	RenaAlbumArt *albumart;
	gchar *uri = NULL;

	RenaBackend *backend = rena_application_get_backend (rena);

	if (rena_backend_get_state (backend) == ST_STOPPED)
		return;

	albumart = rena_toolbar_get_album_art (rena_application_get_toolbar (rena));

	const gchar *albumart_path = rena_album_art_get_path (albumart);

	if (!albumart_path)
		return;

	#ifdef G_OS_WIN32
	uri = g_strdup (albumart_path);
	#else
	uri = g_filename_to_uri (albumart_path, NULL, NULL);
	#endif

	open_url(uri, rena_application_get_window (rena));
	g_free (uri);
}

static void
rena_edit_tags_dialog_response (GtkWidget         *dialog,
                                  gint               response_id,
                                  RenaApplication *rena)
{
	RenaBackend *backend;
	RenaToolbar *toolbar;
	RenaPlaylist *playlist;
	RenaMusicobject *nmobj, *bmobj;
	RenaTagger *tagger;
	gint changed = 0;

	if (response_id == GTK_RESPONSE_HELP) {
		nmobj = rena_tags_dialog_get_musicobject(RENA_TAGS_DIALOG(dialog));
		rena_track_properties_dialog(nmobj, rena_application_get_window(rena));
		return;
	}

	if (response_id == GTK_RESPONSE_OK) {
		changed = rena_tags_dialog_get_changed(RENA_TAGS_DIALOG(dialog));
		if(changed) {
			nmobj = rena_tags_dialog_get_musicobject(RENA_TAGS_DIALOG(dialog));

			backend = rena_application_get_backend (rena);

			if(rena_backend_get_state (backend) != ST_STOPPED) {
				RenaMusicobject *current_mobj = rena_backend_get_musicobject (backend);
				if (rena_musicobject_compare (nmobj, current_mobj) == 0) {
					toolbar = rena_application_get_toolbar (rena);
					playlist = rena_application_get_playlist (rena);

					/* Update public current song */
					rena_update_musicobject_change_tag (current_mobj, changed, nmobj);

					/* Update current song on playlist */
					rena_playlist_update_current_track(playlist, changed, nmobj);

					/* Update current song on backend */
					bmobj = g_object_ref(rena_backend_get_musicobject(backend));
					rena_update_musicobject_change_tag(bmobj, changed, nmobj);
					g_object_unref(bmobj);

					rena_toolbar_set_title(toolbar, current_mobj);
				}
			}

			if(G_LIKELY(rena_musicobject_is_local_file (nmobj))) {
				tagger = rena_tagger_new();
				rena_tagger_add_file (tagger, rena_musicobject_get_file(nmobj));
				rena_tagger_set_changes(tagger, nmobj, changed);
				rena_tagger_apply_changes (tagger);
				g_object_unref(tagger);
			}
		}
	}
	gtk_widget_destroy (dialog);
}

void
rena_playback_edit_current_track (RenaApplication *rena)
{
	RenaBackend *backend;
	GtkWidget *dialog;

	backend = rena_application_get_backend (rena);

	if(rena_backend_get_state (backend) == ST_STOPPED)
		return;

	dialog = rena_tags_dialog_new();
	gtk_window_set_transient_for (GTK_WINDOW(dialog),
		GTK_WINDOW(rena_application_get_window (rena)));

	g_signal_connect (G_OBJECT (dialog), "response",
	                  G_CALLBACK (rena_edit_tags_dialog_response), rena);

	rena_tags_dialog_set_musicobject (RENA_TAGS_DIALOG(dialog),
	                                    rena_backend_get_musicobject (backend));
	
	gtk_widget_show (dialog);
}

void
rena_playback_seek_fraction (GObject *object, gdouble fraction, RenaApplication *rena)
{
	gint seek = 0, length = 0;

	RenaBackend *backend = rena_application_get_backend (rena);

	if (rena_backend_get_state (backend) != ST_PLAYING)
		return;

	length = rena_musicobject_get_length (rena_backend_get_musicobject (backend));

	if (length == 0)
		return;

	seek = (length * fraction);

	if (seek >= length)
		seek = length;

	rena_backend_seek (backend, seek);
}

void
rena_playback_toogle_favorite (GObject *object, RenaApplication *rena)
{
	RenaBackend *backend = NULL;
	RenaToolbar *toolbar = NULL;
	RenaFavorites *favorites = NULL;
	RenaMusicobject *mobj = NULL;

	backend = rena_application_get_backend (rena);
	if (rena_backend_get_state (backend) != ST_PLAYING)
		return;

	toolbar = rena_application_get_toolbar (rena);

	favorites = rena_favorites_get ();
	mobj = rena_backend_get_musicobject (backend);
	if (rena_favorites_contains_song(favorites, mobj)) {
		rena_favorites_remove_song (favorites, mobj);
		rena_toolbar_set_favorite_icon (toolbar, FALSE);
	}
	else {
		rena_favorites_put_song (favorites, mobj);
		rena_toolbar_set_favorite_icon (toolbar, TRUE);
	}
	g_object_unref (favorites);
}
