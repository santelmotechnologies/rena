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

#include "rena-song-info-cache.h"
#include "rena-song-info-plugin.h"
#include "rena-song-info-pane.h"

#include "src/rena-simple-async.h"
#include "src/rena-database.h"
#include "src/rena-utils.h"

typedef struct {
	PraghaSongInfoPlugin *plugin;
	GCancellable         *cancellable;
	gulong                cancel_id;
	gchar                *filename;
	GlyrQuery             query;
	GlyrMemCache         *head;
} glyr_struct;


/*
 * Utils.
 */

PraghaMusicobject *
glyr_mem_cache_get_raw_mobj (GlyrMemCache *it)
{
	PraghaMusicobject *mobj = NULL;
	gchar *title, *artist, *url;
	gchar *utitle = NULL, *uartist = NULL;
	gchar **tags;

	tags = g_strsplit (it->data, "\n", 4);
	title = tags[0];
	artist = tags[1];
	url = tags[3];

	if (string_is_empty(title) || string_is_empty(artist) || string_is_empty(url))
		return NULL;

	utitle = rena_unescape_html_utf75(title);
	uartist = rena_unescape_html_utf75(artist);

	mobj = rena_musicobject_new ();
	rena_musicobject_set_file (mobj, url);
	rena_musicobject_set_title (mobj, utitle);
	rena_musicobject_set_artist (mobj, uartist);

	g_free(utitle);
	g_free(uartist);

	g_strfreev (tags);

	return mobj;
}

/*
 * Function to check if has the last
 */

static gboolean
glyr_finished_thread_is_current_song (PraghaSongInfoPlugin *plugin, const gchar *filename)
{
	PraghaApplication *rena;
	PraghaBackend *backend;
	PraghaMusicobject *mobj;
	const gchar *current_filename = NULL;

	rena = rena_songinfo_plugin_get_application (plugin);

	backend = rena_application_get_backend (rena);
	if (rena_backend_get_state (backend) == ST_STOPPED)
		return FALSE;

	mobj = rena_backend_get_musicobject (backend);
	current_filename = rena_musicobject_get_file (mobj);

	if (g_ascii_strcasecmp(filename, current_filename))
		return FALSE;

	return TRUE;
}

/*
 * Threads
 */

static void
glyr_finished_successfully_pane (glyr_struct *glyr_info)
{
	PraghaSonginfoPane *pane;
	PraghaInfoCache *cache;
	PraghaDatabase *cdbase;
	GlyrMemCache *it = NULL;
	GList *list = NULL, *l = NULL;

	cache = rena_songinfo_plugin_get_cache_info (glyr_info->plugin);
	pane = rena_songinfo_plugin_get_pane (glyr_info->plugin);

	switch (glyr_info->head->type) {
		case GLYR_TYPE_LYRICS:
			rena_info_cache_save_song_lyrics (cache,
			                                    glyr_info->query.title,
			                                    glyr_info->query.artist,
			                                    glyr_info->head->prov,
			                                    glyr_info->head->data);
			rena_songinfo_pane_set_title (pane, glyr_info->query.title);
			rena_songinfo_pane_set_text (pane, glyr_info->head->data, glyr_info->head->prov);
			break;
		case GLYR_TYPE_ARTIST_BIO:
			rena_info_cache_save_artist_bio (cache,
			                                   glyr_info->query.artist,
			                                   glyr_info->head->prov,
			                                   glyr_info->head->data);
			rena_songinfo_pane_set_title (pane, glyr_info->query.artist);
			rena_songinfo_pane_set_text (pane, glyr_info->head->data, glyr_info->head->prov);
			break;
		case GLYR_TYPE_SIMILAR_SONG:
			cdbase = rena_database_get ();
			for (it = glyr_info->head ; it != NULL ; it = it->next) {
				list = g_list_append (list, glyr_mem_cache_get_raw_mobj(it));
			}
			g_object_unref (cdbase);

			rena_info_cache_save_similar_songs (cache,
			                                      glyr_info->query.title,
			                                      glyr_info->query.artist,
			                                      glyr_info->head->prov,
			                                      list);

			for (l = list ; l != NULL ; l = l->next) {
				rena_songinfo_pane_append_song_row (pane,
					rena_songinfo_pane_row_new (RENA_MUSICOBJECT(l->data)));
			}
			g_list_free (list);

			rena_songinfo_pane_set_title (pane, glyr_info->query.title);
			rena_songinfo_pane_set_text (pane, "", glyr_info->head->prov);
			break;
		case GLYR_TYPE_COVERART:
		default:
			break;
	}
}

static void
glyr_finished_incorrectly_pane (glyr_struct *glyr_info)
{
	PraghaSonginfoPane *pane;

	switch (glyr_info->query.type) {
		case GLYR_GET_LYRICS:
			pane = rena_songinfo_plugin_get_pane (glyr_info->plugin);
			rena_songinfo_pane_set_title (pane, glyr_info->query.title);
			rena_songinfo_pane_set_text (pane, _("Lyrics not found."), "");
			break;
		case GLYR_GET_ARTIST_BIO:
			pane = rena_songinfo_plugin_get_pane (glyr_info->plugin);
			rena_songinfo_pane_set_title (pane, glyr_info->query.artist);
			rena_songinfo_pane_set_text (pane, _("Artist information not found."), "");
			break;
		case GLYR_GET_SIMILAR_SONGS:
			pane = rena_songinfo_plugin_get_pane (glyr_info->plugin);
			rena_songinfo_pane_set_title (pane, glyr_info->query.title);
			rena_songinfo_pane_set_text (pane, _("No recommended songs."), "");
			break;
		case GLYR_GET_COVERART:
		default:
			break;
	}
}

static gboolean
glyr_finished_thread_update_pane (gpointer data)
{
	glyr_struct *glyr_info = data;

	if (g_cancellable_is_cancelled (glyr_info->cancellable))
		goto old_thread;

	if (!glyr_finished_thread_is_current_song(glyr_info->plugin, glyr_info->filename))
		goto old_thread;

	if (glyr_info->head != NULL)
		glyr_finished_successfully_pane (glyr_info);
	else
		glyr_finished_incorrectly_pane (glyr_info);

old_thread:
	g_cancellable_disconnect (glyr_info->cancellable, glyr_info->cancel_id);
	g_object_unref (glyr_info->cancellable);

	if (glyr_info->head != NULL)
		glyr_free_list (glyr_info->head);

	glyr_query_destroy (&glyr_info->query);
	g_free (glyr_info->filename);

	g_slice_free (glyr_struct, glyr_info);

	return FALSE;
}

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

static void
search_cancelled (GCancellable *cancellable, gpointer user_data)
{
	GlyrQuery *query = user_data;
	glyr_signal_exit (query);
}

GCancellable *
rena_songinfo_plugin_get_info_to_pane (PraghaSongInfoPlugin *plugin,
                                         GLYR_GET_TYPE        type,
                                         const gchar          *artist,
                                         const gchar          *title,
                                         const gchar          *filename)
{
	PraghaSonginfoPane *pane;
	glyr_struct *glyr_info;
	GCancellable *cancellable;

	glyr_info = g_slice_new0 (glyr_struct);

	rena_songinfo_plugin_init_glyr_query (&glyr_info->query);
	glyr_opt_type (&glyr_info->query, type);

	pane = rena_songinfo_plugin_get_pane (plugin);
	rena_songinfo_pane_clear_text (pane);
	rena_songinfo_pane_clear_list (pane);

	switch (type) {
		case GLYR_GET_ARTIST_BIO:
			rena_songinfo_pane_set_title (pane, artist);
			rena_songinfo_pane_set_text (pane, _("Searching..."), "");

			glyr_opt_artist(&glyr_info->query, artist);

			glyr_opt_lang (&glyr_info->query, "auto");
			glyr_opt_lang_aware_only (&glyr_info->query, TRUE);
			break;
		case GLYR_GET_SIMILAR_SONGS:
			rena_songinfo_pane_set_title (pane, title);
			rena_songinfo_pane_set_text (pane, _("Searching..."), "");

			glyr_opt_number (&glyr_info->query, 50);
			glyr_opt_artist(&glyr_info->query, artist);
			glyr_opt_title(&glyr_info->query, title);
			break;
		case GLYR_GET_LYRICS:
			rena_songinfo_pane_set_title (pane, title);
			rena_songinfo_pane_set_text (pane, _("Searching..."), "");

			glyr_opt_artist(&glyr_info->query, artist);
			glyr_opt_title(&glyr_info->query, title);
			break;
		default:
			break;
	}

	glyr_info->filename = g_strdup(filename);
	glyr_info->plugin = plugin;

	cancellable = g_cancellable_new ();
	glyr_info->cancellable = g_object_ref (cancellable);
	glyr_info->cancel_id = g_cancellable_connect (glyr_info->cancellable,
	                                              G_CALLBACK (search_cancelled),
	                                              &glyr_info->query,
	                                              NULL);

	rena_async_launch (get_related_info_idle_func,
	                     glyr_finished_thread_update_pane,
	                     glyr_info);

	return cancellable;
}

