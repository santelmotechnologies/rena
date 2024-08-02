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

#include "rena-song-info-cache.h"

#include <glib/gstdio.h>

#include "src/rena-utils.h"
#include "src/rena-musicobject-mgmt.h"

struct _RenaInfoCache {
	GObject        _parent;

	RenaDatabase *cdbase;
	gchar          *cache_dir;
};

enum {
	SIGNAL_CACHE_CHANGED,
	LAST_SIGNAL
};

static int signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE(RenaInfoCache, rena_info_cache, G_TYPE_OBJECT)

static void
rena_info_cache_finalize (GObject *object)
{
	RenaInfoCache *cache = RENA_INFO_CACHE(object);

	g_free (cache->cache_dir);

	G_OBJECT_CLASS(rena_info_cache_parent_class)->finalize(object);
}

static void
rena_info_cache_dispose (GObject *object)
{
	RenaInfoCache *cache = RENA_INFO_CACHE(object);

	if (cache->cdbase) {
		g_object_unref (cache->cdbase);
		cache->cdbase = NULL;
	}
	G_OBJECT_CLASS(rena_info_cache_parent_class)->dispose(object);
}


static void
rena_info_cache_class_init (RenaInfoCacheClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = rena_info_cache_finalize;
	object_class->dispose = rena_info_cache_dispose;

	signals[SIGNAL_CACHE_CHANGED] =
		g_signal_new ("cache-changed",
		              G_TYPE_FROM_CLASS (object_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (RenaInfoCacheClass, cache_changed),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);
}

static void
rena_info_cache_init (RenaInfoCache *cache)
{
	cache->cache_dir = g_build_path (G_DIR_SEPARATOR_S, g_get_user_cache_dir (), "rena", "info", NULL);
	g_mkdir_with_parents (cache->cache_dir, S_IRWXU);
	cache->cdbase = rena_database_get ();
}

RenaInfoCache *
rena_info_cache_get (void)
{
	static RenaInfoCache *cache = NULL;

	if (G_UNLIKELY (cache == NULL)) {
		cache = g_object_new (RENA_TYPE_INFO_CACHE, NULL);
		g_object_add_weak_pointer (G_OBJECT (cache),
		                          (gpointer) &cache);
	}
	else {
		g_object_ref (G_OBJECT(cache));
	}

	return cache;
}

/*
 * Similar songs cache.
 */

static gchar *
rena_info_cache_build_similar_songs_path (RenaInfoCache *cache, const gchar *title, const gchar *artist)
{
	gchar *title_escaped = rena_escape_slashes (title);
	gchar *artist_escaped = rena_escape_slashes (artist);
	gchar *result = g_strdup_printf ("%s%s%s-%s.similar", cache->cache_dir, G_DIR_SEPARATOR_S, artist_escaped, title_escaped);
	g_free (title_escaped);
	g_free (artist_escaped);
	return result;
}

static gchar *
rena_info_cache_get_similar_songs_uri (RenaInfoCache *cache, const gchar *title, const gchar *artist)
{
	gchar *path = rena_info_cache_build_similar_songs_path (cache, title, artist);

	if (g_file_test (path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR) == FALSE) {
		g_free (path);
		return NULL;
	}

	return path;
}

gboolean
rena_info_cache_contains_similar_songs (RenaInfoCache *cache, const gchar *title, const gchar *artist)
{
	gchar *path = rena_info_cache_get_similar_songs_uri (cache, title, artist);

	if (path) {
		g_free (path);
		return TRUE;
	}

	return FALSE;
}

GList *
rena_info_cache_get_similar_songs (RenaInfoCache *cache,
                                     const gchar     *title,
                                     const gchar     *artist,
                                     gchar          **provider)
{
	RenaMusicobject *mobj;
	GKeyFile *key_file;
	GError *error = NULL;
	GList *list = NULL;
	guint length = 0, i = 0;
	gchar *path = NULL, *key = NULL;
	gchar *ifile = NULL, *ititle = NULL, *iartist = NULL;

	path = rena_info_cache_get_similar_songs_uri(cache, title, artist);
	if (!path)
		return NULL;

	key_file = g_key_file_new ();

	if (!g_key_file_load_from_file (key_file, path, G_KEY_FILE_NONE, &error)) {
		if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
			g_warning ("Error loading key file: %s", error->message);
		g_error_free (error);
		g_free (path);
		return NULL;
	}

	length = g_key_file_get_integer (key_file, "Similar-Songs", "NumberOfEntries", NULL);

	for (i = 1 ; i <= length ; i++)
	{
		key = g_strdup_printf("File%d", i);
		ifile = g_key_file_get_string (key_file, "Similar-Songs", key, NULL);
		g_free (key);

		key = g_strdup_printf("Title%d", i);
		ititle = g_key_file_get_string (key_file, "Similar-Songs", key, NULL);
		g_free (key);

		key = g_strdup_printf("Artist%d", i);
		iartist = g_key_file_get_string (key_file, "Similar-Songs", key, NULL);
		g_free (key);

		mobj = rena_musicobject_new ();

		rena_musicobject_set_file (mobj, ifile);
		rena_musicobject_set_title (mobj, ititle);
		rena_musicobject_set_artist (mobj, iartist);

		list = g_list_prepend (list, mobj);

		g_free (ifile);
		g_free (ititle);
		g_free (iartist);
	}

	*provider = g_key_file_get_string (key_file, "Similar-Songs", "Provider", NULL);

	g_key_file_free (key_file);
	g_free (path);

	return g_list_reverse (list);
}

void
rena_info_cache_save_similar_songs (RenaInfoCache *cache,
                                      const gchar     *title,
                                      const gchar     *artist,
                                      const gchar     *provider,
                                      GList           *mlist)
{
	RenaMusicobject *mobj;
	GKeyFile *key_file = NULL;
	GError *error = NULL;
	GList *list = NULL;
	const gchar *file = NULL, *ititle = NULL, *iartist = NULL;
	gchar *file_entry, *key = NULL;
	guint length = 0;
	gint64 time = 0;
	gint i = 0;

	key_file = g_key_file_new ();

	g_key_file_set_string (key_file, "Songs", "Title", title);
	g_key_file_set_string (key_file, "Songs", "Artist", artist);

	time = g_get_real_time ();
	g_key_file_set_int64 (key_file, "Similar-Songs", "SavedTime", time);

	length = g_list_length (mlist);
	g_key_file_set_integer (key_file, "Similar-Songs", "NumberOfEntries", length);

	g_key_file_set_string (key_file, "Similar-Songs", "Provider", provider);

	for (list = mlist; list != NULL; list = list->next) {
		mobj = RENA_MUSICOBJECT(list->data);
		i++;

		key = g_strdup_printf("File%d", i);
		file = rena_musicobject_get_file (mobj);
		g_key_file_set_string (key_file, "Similar-Songs", key, file);
		g_free (key);

		key = g_strdup_printf("Title%d", i);
		ititle = rena_musicobject_get_title (mobj);
		g_key_file_set_string (key_file, "Similar-Songs", key, ititle);
		g_free (key);

		key = g_strdup_printf("Artist%d", i);
		iartist = rena_musicobject_get_artist (mobj);
		g_key_file_set_string (key_file, "Similar-Songs", key, iartist);
		g_free (key);
	}

	file_entry = rena_info_cache_build_similar_songs_path(cache, title, artist);
	if (!g_key_file_save_to_file (key_file, file_entry, &error))
		g_warning ("Error saving key file: %s", error->message);
	g_free(file_entry);

	g_key_file_free (key_file);
}

/*
 * Lyrics cache.
 */

static gchar *
rena_info_cache_build_lyrics_path (RenaInfoCache *cache, const gchar *title, const gchar *artist)
{
	gchar *title_escaped = rena_escape_slashes (title);
	gchar *artist_escaped = rena_escape_slashes (artist);
	gchar *result = g_strdup_printf ("%s%s%s-%s.lyrics.txt", cache->cache_dir, G_DIR_SEPARATOR_S, artist_escaped, title_escaped);
	g_free (title_escaped);
	g_free (artist_escaped);
	return result;
}

static gchar *
rena_info_cache_get_lyrics_uri (RenaInfoCache *cache, const gchar *title, const gchar *artist)
{
	gchar *path = rena_info_cache_build_lyrics_path (cache, title, artist);

	if (g_file_test (path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR) == FALSE) {
		g_free (path);
		return NULL;
	}

	return path;
}

gboolean
rena_info_cache_contains_song_lyrics (RenaInfoCache *cache, const gchar *title, const gchar *artist)
{
	gchar *path = rena_info_cache_get_lyrics_uri (cache, title, artist);

	if (path) {
		g_free (path);
		return TRUE;
	}

	return FALSE;
}

static gchar *
rena_info_cache_build_ini_lyrics_path (RenaInfoCache *cache, const gchar *title, const gchar *artist)
{
	gchar *title_escaped = rena_escape_slashes (title);
	gchar *artist_escaped = rena_escape_slashes (artist);
	gchar *result = g_strdup_printf ("%s%s%s-%s.lyrics", cache->cache_dir, G_DIR_SEPARATOR_S, artist_escaped, title_escaped);
	g_free (title_escaped);
	g_free (artist_escaped);
	return result;
}

static gchar *
rena_info_cache_get_ini_lyrics_uri (RenaInfoCache *cache, const gchar *title, const gchar *artist)
{
	gchar *path = rena_info_cache_build_ini_lyrics_path (cache, title, artist);

	if (g_file_test (path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR) == FALSE) {
		g_free (path);
		return NULL;
	}

	return path;
}

gboolean
rena_info_cache_contains_ini_song_lyrics (RenaInfoCache *cache, const gchar *title, const gchar *artist)
{
	gchar *path = rena_info_cache_get_ini_lyrics_uri (cache, title, artist);

	if (path) {
		g_free (path);
		return TRUE;
	}

	return FALSE;
}

gchar *
rena_info_cache_get_song_lyrics (RenaInfoCache *cache,
                                   const gchar     *title,
                                   const gchar     *artist,
                                   gchar          **provider)
{
	GKeyFile *key_file;
	GError *error = NULL;
	gchar *path = NULL, *ini_path = NULL, *lyrics = NULL;

	path = rena_info_cache_get_lyrics_uri (cache, title, artist);
	if (!path)
		return NULL;

	if (!g_file_get_contents (path, &lyrics, NULL, &error)) {
		g_warning ("Error loading lyrics file: %s", error->message);
		g_free (path);
		return NULL;
	}

	ini_path = rena_info_cache_get_ini_lyrics_uri (cache, title, artist);
	if (ini_path) {
		key_file = g_key_file_new ();
		if (!g_key_file_load_from_file (key_file, ini_path, G_KEY_FILE_NONE, &error)) {
			if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
				g_warning ("Error loading key file: %s", error->message);
			g_error_free (error);
		}
		else {
			*provider = g_key_file_get_string (key_file, "Lyrics", "Provider", NULL);
		}
		g_key_file_free (key_file);
	}

	g_free (path);
	g_free (ini_path);

	return lyrics;
}

void
rena_info_cache_save_song_lyrics (RenaInfoCache *cache,
                                    const gchar     *title,
                                    const gchar     *artist,
                                    const gchar     *provider,
                                    const gchar     *lyrics)
{
	GKeyFile *key_file = NULL;
	GError *error = NULL;
	gchar *key_path = NULL, *lyrics_path = NULL;
	gint64 time = 0;

	lyrics_path = rena_info_cache_build_lyrics_path (cache, title, artist);
	if (!g_file_set_contents (lyrics_path, lyrics, -1, &error)) {
		g_warning ("Error saving lyrics file: %s", error->message);
		g_free (lyrics_path);
		return;
	}

	key_file = g_key_file_new ();

	g_key_file_set_string (key_file, "Song", "Title", title);
	g_key_file_set_string (key_file, "Song", "Artist", artist);

	time = g_get_real_time ();
	g_key_file_set_int64 (key_file, "Lyrics", "SavedTime", time);
	g_key_file_set_string (key_file, "Lyrics", "Provider", provider);

	key_path = rena_info_cache_build_ini_lyrics_path (cache, title, artist);
	if (!g_key_file_save_to_file (key_file, key_path, &error))
		g_warning ("Error saving key file: %s", error->message);
	g_free (key_path);

	g_key_file_free (key_file);
	g_free (lyrics_path);
}

/*
 * Artist bio cache.
 */

static gchar *
rena_info_cache_build_artist_bio_path (RenaInfoCache *cache, const gchar *artist)
{
	gchar *artist_escaped = rena_escape_slashes (artist);
	gchar *result = g_strdup_printf ("%s%s%s.bio.txt", cache->cache_dir, G_DIR_SEPARATOR_S, artist_escaped);
	g_free (artist_escaped);
	return result;
}

static gchar *
rena_info_cache_get_artist_bio_uri (RenaInfoCache *cache, const gchar *artist)
{
	gchar *path = rena_info_cache_build_artist_bio_path (cache, artist);
	if (g_file_test (path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR) == FALSE) {
		g_free (path);
		return NULL;
	}
	return path;
}

gboolean
rena_info_cache_contains_artist_bio (RenaInfoCache *cache, const gchar *artist)
{
	gchar *path = rena_info_cache_get_artist_bio_uri (cache, artist);
	if (path) {
		g_free (path);
		return TRUE;
	}
	return FALSE;
}

static gchar *
rena_info_cache_build_ini_artist_bio_path (RenaInfoCache *cache, const gchar *artist)
{
	gchar *artist_escaped = rena_escape_slashes (artist);
	gchar *result = g_strdup_printf ("%s%s%s.bio", cache->cache_dir, G_DIR_SEPARATOR_S, artist_escaped);
	g_free (artist_escaped);
	return result;
}

static gchar *
rena_info_cache_get_ini_artist_bio_uri (RenaInfoCache *cache, const gchar *artist)
{
	gchar *path = rena_info_cache_build_ini_artist_bio_path (cache, artist);
	if (g_file_test (path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR) == FALSE) {
		g_free (path);
		return NULL;
	}
	return path;
}

gboolean
rena_info_cache_contains_ini_artist_bio (RenaInfoCache *cache, const gchar *artist)
{
	gchar *path = rena_info_cache_get_ini_artist_bio_uri (cache, artist);
	if (path) {
		g_free (path);
		return TRUE;
	}
	return FALSE;
}

gchar *
rena_info_cache_get_artist_bio (RenaInfoCache *cache,
                                  const gchar     *artist,
                                  gchar          **provider)
{
	GKeyFile *key_file;
	GError *error = NULL;
	gchar *path = NULL, *bio = NULL;

	path = rena_info_cache_get_artist_bio_uri (cache, artist);
	if (!path)
		return NULL;

	if (!g_file_get_contents (path, &bio, NULL, &error)) {
		g_warning ("Error loading artist bio file: %s", error->message);
		g_free (path);
		return NULL;
	}

	path = rena_info_cache_get_ini_artist_bio_uri (cache, artist);
	if (path) {
		key_file = g_key_file_new ();

		if (!g_key_file_load_from_file (key_file, path, G_KEY_FILE_NONE, &error)) {
			if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
				g_warning ("Error loading key file: %s", error->message);
			g_error_free (error);
			g_free (path);
			return NULL;
		}

		*provider = g_key_file_get_string (key_file, "Artist-Bio", "Provider", NULL);

		g_key_file_free (key_file);
		g_free (path);
	}

	return bio;
}

void
rena_info_cache_save_artist_bio (RenaInfoCache *cache,
                                   const gchar     *artist,
                                   const gchar     *provider,
                                   const gchar     *bio)
{
	GKeyFile *key_file = NULL;
	GError *error = NULL;
	gchar *key_path = NULL, *bio_path = NULL;
	gint64 time = 0;

	bio_path = rena_info_cache_build_artist_bio_path (cache, artist);
	if (!g_file_set_contents (bio_path, bio, -1, &error)) {
		g_warning ("Error saving artist bio file: %s", error->message);
		g_free (bio_path);
		return;
	}

	key_file = g_key_file_new ();
	g_key_file_set_string (key_file, "Song", "Artist", artist);

	time = g_get_real_time ();
	g_key_file_set_int64 (key_file, "Artist-Bio", "SavedTime", time);
	g_key_file_set_string (key_file, "Artist-Bio", "Provider", provider);

	key_path = rena_info_cache_build_ini_artist_bio_path (cache, artist);
	if (!g_key_file_save_to_file (key_file, key_path, &error))
		g_warning ("Error saving key file: %s", error->message);

	g_free (key_path);
	g_free (bio_path);

	g_key_file_free (key_file);
}
