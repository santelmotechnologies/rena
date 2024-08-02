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

#if defined(GETTEXT_PACKAGE)
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#include "rena-favorites.h"

#include <glib/gstdio.h>

#include "rena-database.h"
#include "rena-musicobject.h"

struct _RenaFavorites {
	GObject        _parent;
	RenaDatabase *cdbase;
};

enum {
	SIGNAL_SONG_ADDED,
	SIGNAL_SONG_REMOVED,
	LAST_SIGNAL
};

static int signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE(RenaFavorites, rena_favorites, G_TYPE_OBJECT)

static void
rena_favorites_dispose (GObject *object)
{
	RenaFavorites *favorites = RENA_FAVORITES(object);
	if (favorites->cdbase) {
		g_object_unref (favorites->cdbase);
		favorites->cdbase = NULL;
	}
	G_OBJECT_CLASS(rena_favorites_parent_class)->dispose(object);
}

static void
rena_favorites_class_init (RenaFavoritesClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->dispose = rena_favorites_dispose;

	signals[SIGNAL_SONG_ADDED] =
		g_signal_new ("song-added",
		              G_TYPE_FROM_CLASS (object_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (RenaFavoritesClass, song_added),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__POINTER,
		              G_TYPE_NONE, 1, G_TYPE_POINTER);
	signals[SIGNAL_SONG_REMOVED] =
		g_signal_new ("song-removed",
		              G_TYPE_FROM_CLASS (object_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (RenaFavoritesClass, song_removed),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__POINTER,
		              G_TYPE_NONE, 1, G_TYPE_POINTER);
}

static void
rena_favorites_init (RenaFavorites *favorites)
{
	favorites->cdbase = rena_database_get();
}

RenaFavorites *
rena_favorites_get (void)
{
	static RenaFavorites *favorites = NULL;

	if (G_UNLIKELY (favorites == NULL)) {
		favorites = g_object_new (RENA_TYPE_FAVORITES, NULL);
		g_object_add_weak_pointer (G_OBJECT (favorites),
		                          (gpointer) &favorites);
	}
	else {
		g_object_ref (G_OBJECT(favorites));
	}

	return favorites;
}

void
rena_favorites_put_song (RenaFavorites *favorites, RenaMusicobject *mobj)
{
	gint playlist_id = 0;
	playlist_id = rena_database_find_playlist (favorites->cdbase, _("Favorites"));
	rena_database_add_playlist_track (favorites->cdbase, playlist_id, rena_musicobject_get_file(mobj));
	g_signal_emit (favorites, signals[SIGNAL_SONG_ADDED], 0, mobj);
	return;
}

void
rena_favorites_remove_song (RenaFavorites *favorites, RenaMusicobject *mobj)
{
	gint playlist_id = 0;
	playlist_id = rena_database_find_playlist (favorites->cdbase, _("Favorites"));
	rena_database_delete_playlist_track (favorites->cdbase, playlist_id, rena_musicobject_get_file(mobj));
	g_signal_emit (favorites, signals[SIGNAL_SONG_REMOVED], 0, mobj);
	return;
}

gboolean
rena_favorites_contains_song (RenaFavorites *favorites, RenaMusicobject *mobj)
{
	gint playlist_id = 0;
	playlist_id = rena_database_find_playlist (favorites->cdbase, _("Favorites"));
	if (!playlist_id) {
		rena_database_add_new_playlist (favorites->cdbase, _("Favorites"));
		return FALSE;
	}
	return rena_database_playlist_has_track (favorites->cdbase, playlist_id, rena_musicobject_get_file(mobj));
}
