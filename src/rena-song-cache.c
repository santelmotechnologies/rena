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

#include "rena-database.h"
#include "rena-song-cache.h"
#include "rena-preferences.h"

#include <glib/gstdio.h>
#include <gio/gio.h>

struct _RenaSongCache {
	GObject        _parent;
	RenaDatabase *cdbase;
	gchar          *cache_dir;
	gint            cache_size;
};

G_DEFINE_TYPE(RenaSongCache, rena_song_cache, G_TYPE_OBJECT)

static void
rena_song_cache_finalize (GObject *object)
{
	RenaSongCache *cache = RENA_SONG_CACHE(object);

	g_free (cache->cache_dir);

	G_OBJECT_CLASS(rena_song_cache_parent_class)->finalize(object);
}

static void
rena_song_cache_dispose (GObject *object)
{
	RenaSongCache *cache = RENA_SONG_CACHE(object);

	if (cache->cdbase) {
		g_object_unref (cache->cdbase);
		cache->cdbase = NULL;
	}
	G_OBJECT_CLASS(rena_song_cache_parent_class)->dispose(object);
}

static void
rena_song_cache_class_init (RenaSongCacheClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = rena_song_cache_finalize;
	object_class->dispose = rena_song_cache_dispose;
}

static void
rena_song_cache_init (RenaSongCache *cache)
{
	RenaPreferences *preferences;

	cache->cdbase = rena_database_get ();
	cache->cache_dir = g_build_path (G_DIR_SEPARATOR_S, g_get_user_cache_dir (), "rena", "songs", NULL);
	g_mkdir_with_parents (cache->cache_dir, S_IRWXU);

	preferences = rena_preferences_get ();
	cache->cache_size = rena_preferences_get_integer (preferences, GROUP_GENERAL, KEY_CACHE_SIZE);
	if (cache->cache_size == 0)
		cache->cache_size = 1*1024*1024*1024; /* 1GB by default */
	g_object_unref (G_OBJECT(preferences));
}

RenaSongCache *
rena_song_cache_get (void)
{
	static RenaSongCache *cache = NULL;

	if (G_UNLIKELY (cache == NULL)) {
		cache = g_object_new (RENA_TYPE_SONG_CACHE, NULL);
		g_object_add_weak_pointer (G_OBJECT (cache),
		                          (gpointer) &cache);
	}
	else {
		g_object_ref (G_OBJECT(cache));
	}

	return cache;
}

static gboolean
rena_song_cache_drop_song (RenaSongCache *cache, const gchar *basename)
{
	RenaPreparedStatement *statement;
	gchar *filename = NULL;
	GFile *file;
	gboolean done = FALSE;

	filename = g_strdup_printf ("%s%s%s", cache->cache_dir, G_DIR_SEPARATOR_S, basename);
	file = g_file_new_for_path (filename);

	done = g_file_delete (file, NULL, NULL);
	if (done) {
		statement = rena_database_create_statement (cache->cdbase, "DELETE FROM CACHE WHERE name = ?");
		rena_prepared_statement_bind_string (statement, 1, basename);
		rena_prepared_statement_step (statement);
		rena_prepared_statement_free (statement);
	}

	g_free (filename);
	g_object_unref (file);

	return done;
}

static void
rena_song_cache_purge (RenaSongCache *cache)
{
	RenaPreparedStatement *statement;
	gint cache_used = 0, song_size = 0;
	const gchar *basename = NULL;

	statement = rena_database_create_statement (cache->cdbase, "SELECT SUM (size) FROM CACHE");
	if (rena_prepared_statement_step (statement))
		cache_used = rena_prepared_statement_get_int (statement, 0);
	rena_prepared_statement_free (statement);

	if (cache->cache_size < cache_used)
	{
		statement = rena_database_create_statement (cache->cdbase, "SELECT name, size FROM CACHE ORDER BY timestamp");
		while (rena_prepared_statement_step (statement))
		{
			basename = rena_prepared_statement_get_string (statement, 0);
			song_size = rena_prepared_statement_get_int (statement, 1);
			if (rena_song_cache_drop_song(cache, basename))
			{
				cache_used -= song_size;
				if (cache->cache_size > cache_used)
					break;
			}
		}
		rena_prepared_statement_free (statement);
	}
}

void
rena_song_cache_put_location (RenaSongCache *cache, const gchar *location, const gchar *filename)
{
	RenaPreparedStatement *statement;
	GFile *file, *destination;
	gchar *dest_filename = NULL, *file_basename = NULL;
	gint size = 0, timestamp = 0;
	gint location_id = 0;
	struct stat sbuf;

	location_id = rena_database_find_location (cache->cdbase, location);

	/* FIXME: Gstreamer 'download' even if it is a local file */
	statement = rena_database_create_statement (cache->cdbase, "SELECT name FROM CACHE WHERE id = ?");
	rena_prepared_statement_bind_int (statement, 1, location_id);
	if (rena_prepared_statement_step (statement)) {
		rena_prepared_statement_free (statement);
		return;
	}
	rena_prepared_statement_free (statement);

	/* TODO: Do it async... */
	file = g_file_new_for_path (filename);
	file_basename = g_file_get_basename(file);
	dest_filename = g_strdup_printf ("%s%s%s", cache->cache_dir, G_DIR_SEPARATOR_S, file_basename);
	destination = g_file_new_for_path (dest_filename);
	if (g_file_copy (file, destination, G_FILE_COPY_NONE, NULL, NULL, NULL, NULL)) {
		if (g_stat(dest_filename, &sbuf) == 0) {
			timestamp = sbuf.st_mtime;
			size = sbuf.st_size;
		}

		statement = rena_database_create_statement (cache->cdbase, "INSERT INTO CACHE (id, name, size, playcount, timestamp) VALUES (?, ?, ?, ?, ?)");
		rena_prepared_statement_bind_int (statement, 1, location_id);
		rena_prepared_statement_bind_string (statement, 2, file_basename);
		rena_prepared_statement_bind_int (statement, 3, size);
		rena_prepared_statement_bind_int (statement, 4, 1);
		rena_prepared_statement_bind_int (statement, 5, timestamp);
		rena_prepared_statement_step (statement);
		rena_prepared_statement_free (statement);
	}

	/* Clean cache if necessary. */
	rena_song_cache_purge (cache);

	g_object_unref(file);
	g_object_unref(destination);
	g_free (file_basename);
	g_free (dest_filename);
}

gchar *
rena_song_cache_get_from_location (RenaSongCache *cache, const gchar *location)
{
	RenaPreparedStatement *statement;
	gchar *basename = NULL, *filename = NULL;
	gint location_id = 0;
	GTimeVal tv;

	location_id = rena_database_find_location (cache->cdbase, location);

	statement = rena_database_create_statement (cache->cdbase, "SELECT name FROM CACHE WHERE id = ?");
	rena_prepared_statement_bind_int (statement, 1, location_id);
	if (rena_prepared_statement_step (statement))
		basename = g_strdup (rena_prepared_statement_get_string (statement, 0));
	rena_prepared_statement_free (statement);

	if (basename != NULL) {
		filename = g_strdup_printf ("%s%s%s", cache->cache_dir, G_DIR_SEPARATOR_S, basename);
		if (g_file_test(filename, G_FILE_TEST_EXISTS)) {
			statement = rena_database_create_statement (cache->cdbase, "UPDATE CACHE SET playcount = playcount + 1 WHERE id = ?");
			rena_prepared_statement_bind_int (statement, 1, location_id);
			rena_prepared_statement_step (statement);
			rena_prepared_statement_free (statement);

			g_get_current_time(&tv);
			statement = rena_database_create_statement (cache->cdbase, "UPDATE CACHE SET timestamp = ? WHERE id = ?");
			rena_prepared_statement_bind_int (statement, 1, tv.tv_sec);
			rena_prepared_statement_bind_int (statement, 2, location_id);
			rena_prepared_statement_step (statement);
			rena_prepared_statement_free (statement);
		}
		else {
			statement = rena_database_create_statement (cache->cdbase, "DELETE FROM CACHE WHERE id = ?");
			rena_prepared_statement_bind_int (statement, 1, location_id);
			rena_prepared_statement_step (statement);
			rena_prepared_statement_free (statement);
			g_free (filename);
			filename = NULL;
		}
		g_free (basename);
	}

	return filename;
}

