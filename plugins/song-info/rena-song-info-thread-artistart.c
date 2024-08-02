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

#include <glyr/glyr.h>

#include "rena-song-info-plugin.h"
#include "rena-song-info-pane.h"

#include "src/rena-art-cache.h"
#include "src/rena-simple-async.h"

typedef struct {
	RenaSongInfoPlugin *plugin;
	GlyrQuery             query;
	GlyrMemCache         *head;
} glyr_struct;

/* Save the downloaded artist art in cache, and updates the gui.*/

static void
glyr_finished_successfully (glyr_struct *glyr_info)
{
	RenaApplication *rena;
	RenaArtCache *art_cache;
	const gchar *artist = NULL;

	rena = rena_songinfo_plugin_get_application (glyr_info->plugin);

	artist = glyr_info->query.artist;

	art_cache = rena_application_get_art_cache (rena);

	if (glyr_info->head->data)
		rena_art_cache_put_artist (art_cache, artist, glyr_info->head->data, glyr_info->head->size);

	glyr_free_list(glyr_info->head);
}

/*
 * Final threads
 */

static gboolean
glyr_finished_thread_update (gpointer data)
{
	glyr_struct *glyr_info = data;

	if (glyr_info->head != NULL)
		glyr_finished_successfully (glyr_info);

	glyr_query_destroy (&glyr_info->query);
	g_slice_free (glyr_struct, glyr_info);

	return FALSE;
}

/* Get artist bio or lyric on a thread. */

static gpointer
get_related_info_idle_func (gpointer data)
{
	GlyrMemCache *head;
	GLYR_ERROR error;

	glyr_struct *glyr_info = data;

	head = glyr_get (&glyr_info->query, &error, NULL);

	glyr_info->head = head;

	return glyr_info;
}

void
rena_songinfo_plugin_get_artist_art (RenaSongInfoPlugin *plugin,
                                       const gchar          *artist)
{
	glyr_struct *glyr_info;

	CDEBUG(DBG_INFO, "Get artist art handler");

	glyr_info = g_slice_new0 (glyr_struct);

	rena_songinfo_plugin_init_glyr_query(&glyr_info->query);

	glyr_opt_type (&glyr_info->query, GLYR_GET_ARTIST_PHOTOS);
	glyr_opt_from (&glyr_info->query, "lastfm");

	glyr_opt_artist (&glyr_info->query, artist);

	glyr_info->plugin = plugin;

	rena_async_launch (get_related_info_idle_func,
	                     glyr_finished_thread_update,
	                     glyr_info);
}

