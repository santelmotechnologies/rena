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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(GETTEXT_PACKAGE)
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#ifdef HAVE_GRILO_NET3
#include <grilo-0.3/net/grl-net.h>
#endif
#ifdef HAVE_GRILO_NET2
#include <grilo-0.2/net/grl-net.h>
#endif

#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include "src/rena-musicobject.h"
#include "src/rena-utils.h"

#include "rena-subsonic-api.h"


/*
 * Forward declarations
 */

static void
rena_subsonic_api_get_album (RenaSubsonicApi *subsonic,
                               const gchar       *album_id);

static void
rena_subsonic_api_get_albums_queue (RenaSubsonicApi *subsonic);


/*
 * RenaSubsonicApi *
 */

struct _RenaSubsonicApi {
	GObject      _parent;

	GrlNetWc     *glrnet;
	GCancellable *cancellable;

	gchar        *server;
	gchar        *username;
	gchar        *password;

	GQueue       *albums_queue;
	guint         albums_count;
	guint         albums_offset;
	guint         albums_progress;

	guint         songs_count;
	GSList       *songs_list;

	gboolean      authenticated;
	gboolean      has_connection;
	gboolean      scanning;
};

enum {
	SIGNAL_AUTH_DONE,
	SIGNAL_PING_DONE,
	SIGNAL_SCAN_PROGRESS,
	SIGNAL_SCAN_TOTAL,
	SIGNAL_SCAN_DONE,
	LAST_SIGNAL
};

static int signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE(RenaSubsonicApi, rena_subsonic_api, G_TYPE_OBJECT)


/*
 *  Utils
 */
static gchar *
rena_subsonic_api_get_token (const gchar *raw_password, const gchar *salt)
{
	gchar *str, *token;

	str = g_strdup_printf ("%s%s", raw_password, salt);
	token = g_compute_checksum_for_string (G_CHECKSUM_MD5, str, strlen(str));
	g_free (str);

	return token;
}

static gchar *
rena_subsonic_api_get_random_salt (void)
{
	gdouble rand;
	gchar *salt, *str1, *str2;

	rand = g_random_double ();
	str1 = g_strdup_printf ("%g", rand);
	rand = g_random_double ();
	str2 = g_strdup_printf ("%g", rand);

	salt = rena_subsonic_api_get_token (str1, str2);

	g_free (str1);
	g_free (str2);

	return salt;
}

static void
rena_subsonic_api_add_query_item (GString     *url,
                                    const gchar *parameter,
                                    const gchar *value)
{
	g_string_append_printf (url, "%s=%s&", parameter, value);
}

static GString *
rena_subsonic_api_build_url (RenaSubsonicApi *subsonic,
                               const gchar       *method)
{
	gchar *salt, *token;
	GString *url = g_string_new (subsonic->server);

	g_string_append_printf (url, "/rest/%s.view?", method);

	rena_subsonic_api_add_query_item(url, "u", subsonic->username);

	if (TRUE) {
		salt = rena_subsonic_api_get_random_salt ();
		token = rena_subsonic_api_get_token (subsonic->password, salt);

		rena_subsonic_api_add_query_item(url, "t", token);
		rena_subsonic_api_add_query_item(url, "s", salt);

		g_free (salt);
		g_free (token);
	}
	else {
		rena_subsonic_api_add_query_item(url, "p", subsonic->password);
	}
	return url;
}

static gchar *
rena_subsonic_api_close_url (GString *url)
{
	g_string_append_printf (url, "%s=%s&", "v", "1.13.0");
	g_string_append_printf (url, "%s=%s", "c", "Rena");

	return g_string_free (url, FALSE);
}

static gchar *
rena_subsonic_api_get_friendly_url (const gchar *server,
                                      const gchar *artist,
                                      const gchar *album,
                                      const gchar *title,
                                      const gchar *song_id)
{
	gchar *url = NULL;
	url = g_strdup_printf("%s/%s/%s/%s - %s", server,
		string_is_not_empty(artist) ? artist : _("Unknown Artist"),
		string_is_not_empty(album)  ? album  : _("Unknown Album"),
		song_id,
		string_is_not_empty(title)  ? title  : _("Unknown"));
	return url;
}

gchar *
rena_subsonic_api_get_playback_url (RenaSubsonicApi *subsonic,
                                      const gchar       *friendly_url)
{
	GString *url;
	gchar **url_split = NULL, *filename = NULL, *song_id = NULL;

	url_split = g_strsplit(friendly_url + strlen(subsonic->server), "/", -1);
	filename = g_strdup(url_split[3]);
	g_strfreev (url_split);

	url_split = g_strsplit(filename, " - ", -1);
	song_id = g_strdup(url_split[0]);
	g_strfreev (url_split);

	url = rena_subsonic_api_build_url (subsonic, "stream");
	rena_subsonic_api_add_query_item (url, "id", song_id);

	g_free (filename);
	g_free (song_id);

	return rena_subsonic_api_close_url (url);
}

/*
 * Subsonic API handlers
 */

static void
rena_subsonic_api_ping_done (GObject      *object,
                               GAsyncResult *res,
                               gpointer      user_data)
{
	GError *wc_error = NULL;
	xmlDocPtr doc;
	xmlNodePtr node;
	gboolean has_connection = TRUE;
	gchar *content = NULL;
	SubsonicStatusCode code = S_GENERIC_OK;

	RenaSubsonicApi *subsonic = RENA_SUBSONIC_API (user_data);

	if (!grl_net_wc_request_finish (GRL_NET_WC (object),
	                                res,
	                                &content,
	                                NULL,
	                                &wc_error))
	{

		if (g_cancellable_is_cancelled (subsonic->cancellable)) {
			code = S_USER_CANCELLED;
			g_cancellable_reset (subsonic->cancellable);
		}
		else {
			code = S_GENERIC_ERROR;
			has_connection = FALSE;
			g_warning ("Failed to connect to subsonic server: %s", wc_error->message);
		}
	}

	if (content)
	{
		doc = xmlReadMemory (content, strlen(content), NULL, NULL,
		                     XML_PARSE_RECOVER | XML_PARSE_NOBLANKS);

		node = xmlDocGetRootElement (doc);

		node = node->xmlChildrenNode;
		while(node)
		{
			if (!xmlStrcmp (node->name, (const xmlChar *) "error"))
			{
				code = atoi (xmlGetProp(node, "code"));
				has_connection = FALSE;
				g_warning("PING ERROR: %s %s", xmlGetProp(node, "code"), xmlGetProp(node, "message"));
			}
			node = node->next;
		}

		xmlFreeDoc (doc);
	}

	if (!subsonic->authenticated) {
		subsonic->authenticated = (code != S_GENERIC_OK);
		g_signal_emit (subsonic, signals[SIGNAL_AUTH_DONE], 0, code);
	}

	if (subsonic->has_connection != has_connection) {
		subsonic->has_connection = has_connection;
		g_signal_emit (subsonic, signals[SIGNAL_PING_DONE], 0, code);
	}
}

static void
rena_subsonic_api_get_albums_done (GObject      *object,
                                     GAsyncResult *res,
                                     gpointer      user_data)
{
	GError *wc_error = NULL;
	xmlDocPtr doc;
	xmlNodePtr node;
	guint albums_count = 0;
	gchar *content = NULL;
	gchar *album_id = NULL;
	SubsonicStatusCode code = S_GENERIC_OK;

	RenaSubsonicApi *subsonic = RENA_SUBSONIC_API (user_data);

	if (!grl_net_wc_request_finish (GRL_NET_WC (object),
	                                res,
	                                &content,
	                                NULL,
	                                &wc_error))
	{
		if (g_cancellable_is_cancelled (subsonic->cancellable)) {
			code = S_USER_CANCELLED;
			subsonic->scanning = FALSE;
			g_cancellable_reset (subsonic->cancellable);
		}
		else {
			code = S_GENERIC_ERROR;
			subsonic->scanning = FALSE;
			g_warning ("Failed to get albums from subsonic server: %s", wc_error->message);
		}
	}

	if (content)
	{
		doc = xmlReadMemory (content, strlen(content), NULL, NULL,
		                     XML_PARSE_RECOVER | XML_PARSE_NOBLANKS);

		node = xmlDocGetRootElement (doc);

		node = node->xmlChildrenNode;
		if (node)
			node = node->xmlChildrenNode;

		while(node)
		{
			if (!xmlStrcmp (node->name, (const xmlChar *) "album"))
			{
				const gchar *id = xmlGetProp(node, "id");
				if (string_is_not_empty (id)) {
					g_queue_push_head (subsonic->albums_queue, g_strdup(id));
					albums_count++;
				}
			}
			else {
				g_critical ("Remove these warning: Unknown node: %s", node->name);
			}
			node = node->next;
		}

		xmlFreeDoc (doc);
	}

	if (code != S_GENERIC_OK) {
		g_warning ("Remove these warning: Subsonic scan finished due error or user interaction.. ");
		g_signal_emit (subsonic, signals[SIGNAL_SCAN_DONE], 0, code);
		return;
	}

	if (albums_count > 0) {
		subsonic->albums_count += albums_count;
		g_warning ("Remove these warning: Subsonic response %i albums...", subsonic->albums_count);

		rena_subsonic_api_get_albums_queue (subsonic);
	}
	else if (albums_count == 0 && subsonic->albums_count > 0) {
		g_warning ("Remove these warning: Subsonic finish obtaining albums. Now look these songs.");

		g_signal_emit (subsonic, signals[SIGNAL_SCAN_TOTAL], 0, subsonic->albums_count);

		album_id = g_queue_pop_head(subsonic->albums_queue);
		rena_subsonic_api_get_album (subsonic, album_id);
	}
	else {
		g_warning ("Remove these warning: Subsonic dont reports any album...");

		subsonic->scanning = FALSE;
		g_signal_emit (subsonic, signals[SIGNAL_SCAN_DONE], 0, code);
	}
}

void
rena_subsonic_api_get_albums_queue (RenaSubsonicApi *subsonic)
{
	GString *url;
	gchar *urlc = NULL, *offsetc = NULL;

	subsonic->albums_offset += 250;
	offsetc = g_strdup_printf("%i",  subsonic->albums_offset);

	url = rena_subsonic_api_build_url (subsonic, "getAlbumList2");
	rena_subsonic_api_add_query_item (url, "type", "alphabeticalByName");
	rena_subsonic_api_add_query_item (url, "size", "250");
	rena_subsonic_api_add_query_item (url, "offset", offsetc);
	urlc = rena_subsonic_api_close_url (url);

	g_warning ("Remove these warning: Albums url: %s", urlc);

	grl_net_wc_request_async (subsonic->glrnet,
	                          urlc,
	                          subsonic->cancellable,
	                          rena_subsonic_api_get_albums_done,
	                          subsonic);

	g_free (offsetc);
	g_free(urlc);
}


static void
rena_subsonic_api_get_album_done (GObject      *object,
                                    GAsyncResult *res,
                                    gpointer      user_data)
{
	RenaMusicobject *mobj = NULL;
	GError *wc_error = NULL;
	xmlDocPtr doc;
	xmlNodePtr node;
	guint songs_count = 0;
	gchar *content = NULL;
	gchar *album = NULL, *artist = NULL, *albumArtist = NULL, *genre = NULL;
	gchar *url = NULL, *song_id = NULL, *title = NULL, *contentType = NULL;
	guint year = 0, track_no = 0, duration = 0;
	gchar *album_id = NULL;
	SubsonicStatusCode code = S_GENERIC_OK;

	RenaSubsonicApi *subsonic = RENA_SUBSONIC_API (user_data);

	if (!grl_net_wc_request_finish (GRL_NET_WC (object),
	                                res,
	                                &content,
	                                NULL,
	                                &wc_error))
	{
		if (g_cancellable_is_cancelled (subsonic->cancellable)) {
			code = S_USER_CANCELLED;
			subsonic->scanning = FALSE;
			g_cancellable_reset (subsonic->cancellable);
		}
		else {
			code = S_GENERIC_ERROR;
			subsonic->scanning = FALSE;
			g_warning ("Failed to get album from subsonic server: %s", wc_error->message);
		}
	}

	if (content)
	{
		doc = xmlReadMemory (content, strlen(content), NULL, NULL,
		                     XML_PARSE_RECOVER | XML_PARSE_NOBLANKS);

		node = xmlDocGetRootElement (doc);

		node = node->xmlChildrenNode;

		album = xmlGetProp(node, "name");
		albumArtist = xmlGetProp(node, "artist");
		year = xmlHasProp(node, "year") ? atoi(xmlGetProp(node, "year")) : 0;
		genre = xmlGetProp(node, "genre");

		node = node->xmlChildrenNode;
		while(node)
		{
			if (!xmlStrcmp (node->name, (const xmlChar *) "song"))
			{
				song_id = xmlGetProp(node, "id");
				title = xmlGetProp(node, "title");
				artist = xmlGetProp(node, "artist");
				contentType = xmlGetProp(node, "contentType");
				track_no = xmlHasProp(node, "track") ? atoi(xmlGetProp(node, "track")) : 0;
				duration = xmlHasProp(node, "duration") ? atoi(xmlGetProp(node, "duration")) : 0;

				// If doesn't return the artist of the song, try to use the album artist.
				artist = artist ? artist : albumArtist;

				url = rena_subsonic_api_get_friendly_url (subsonic->server, artist, album, title, song_id);

				mobj = g_object_new (RENA_TYPE_MUSICOBJECT,
				                     "file", url,
				                     "source", FILE_HTTP,
				                     "provider", subsonic->server,
				                     "mime-type", contentType,
				                     "track-no", track_no,
				                     "title", title != NULL ? title : "",
				                     "artist", artist != NULL ? artist : "",
				                     "album", album != NULL ? album : "",
				                     "year", year,
				                     "genre", genre != NULL ? genre : "",
				                     "length", duration,
				                     NULL);

				subsonic->songs_list = g_slist_prepend (subsonic->songs_list, mobj);
				songs_count++;
			}
			else {
				g_warning ("Remove these warning: Unknown album node: %s", node->name);
			}
			node = node->next;
		}

		xmlFreeDoc (doc);
	}

	if (code != S_GENERIC_OK) {
		g_warning ("Remove these warning: Subsonic scan finished due error or user interaction.. ");
		g_signal_emit (subsonic, signals[SIGNAL_SCAN_DONE], 0, code);
		return;
	}

	g_warning ("Remove these warning: Subsonic response %i songs...", songs_count);

	/* Report the progress of the albums */

	g_signal_emit (subsonic, signals[SIGNAL_SCAN_PROGRESS], 0, ++subsonic->albums_progress);

	/* If there are still albums, queue them.*/

	if (album_id = g_queue_pop_head(subsonic->albums_queue)) {
		g_warning ("Remove these warning: Queue new album to look: %s", album_id);
		rena_subsonic_api_get_album (subsonic, album_id);
	}
	else {
		g_warning ("Remove these warning: Subsonic import finished.");

		subsonic->scanning = FALSE;
		g_signal_emit (subsonic, signals[SIGNAL_SCAN_DONE], 0, code);
	}
}

static void
rena_subsonic_api_get_album (RenaSubsonicApi *subsonic,
                               const gchar       *album_id)
{
	GString *url;
	gchar *urlc = NULL;

	url = rena_subsonic_api_build_url (subsonic, "getAlbum");
	rena_subsonic_api_add_query_item (url, "id", album_id);
	urlc = rena_subsonic_api_close_url (url);

	g_warning ("Remove these warning: Album url: %s", urlc);

	grl_net_wc_request_async (subsonic->glrnet,
	                          urlc,
	                          subsonic->cancellable,
	                          rena_subsonic_api_get_album_done,
	                          subsonic);

	g_free(urlc);
}


/*
 *  Public api.
 */

void
rena_subsonic_api_authentication (RenaSubsonicApi *subsonic,
                                    const gchar       *server,
                                    const gchar       *username,
                                    const gchar       *password)
{
	/* Save credentials */

	g_free (subsonic->server);
	g_free (subsonic->username);
	g_free(subsonic->password);

	subsonic->server = g_strdup (server);
	subsonic->username = g_strdup (username);
	subsonic->password = g_strdup (password);

	/* Ping to check connection */

	rena_subsonic_api_ping_server (subsonic);
}

void
rena_subsonic_api_deauthentication (RenaSubsonicApi *subsonic)
{
	if (subsonic->scanning)
		g_cancellable_cancel (subsonic->cancellable);

	subsonic->authenticated = FALSE;

	if (subsonic->server) {
		g_free(subsonic->server);
		subsonic->server = NULL;
	}
	if (subsonic->username) {
		g_free(subsonic->username);
		subsonic->username = NULL;
	}
	if (subsonic->password) {
		g_free(subsonic->password);
		subsonic->password = NULL;
	}
}

void
rena_subsonic_api_ping_server (RenaSubsonicApi *subsonic)
{
	GString *url;
	gchar *urlc = NULL;

	url = rena_subsonic_api_build_url (subsonic, "ping");
	urlc = rena_subsonic_api_close_url (url);
	grl_net_wc_request_async (subsonic->glrnet,
	                          urlc,
	                          subsonic->cancellable,
	                          rena_subsonic_api_ping_done,
	                          subsonic);

	g_warning ("Remove these warning: Ping url: %s", urlc);

	g_free(urlc);
}

void
rena_subsonic_api_scan_server (RenaSubsonicApi *subsonic)
{
	if (subsonic->scanning)
		return;

	subsonic->scanning = TRUE;

	subsonic->albums_count = 0;
	subsonic->albums_offset = 0;
	subsonic->songs_count = 0;

	rena_subsonic_api_get_albums_queue (subsonic);
}

void
rena_subsonic_api_cancel (RenaSubsonicApi *subsonic)
{
	if (!g_cancellable_is_cancelled (subsonic->cancellable))
		g_cancellable_cancel (subsonic->cancellable);

	if (subsonic->scanning == TRUE) {
		while (g_cancellable_is_cancelled(subsonic->cancellable)) {
			// When canceling always resets it.
			rena_process_gtk_events ();
		}

		g_queue_clear_full (subsonic->albums_queue,
		                    (GDestroyNotify) g_free);

		g_slist_free_full (subsonic->songs_list,
		                   (GDestroyNotify) g_object_unref);

		subsonic->albums_count = 0;
		subsonic->albums_offset = 0;
		subsonic->songs_count = 0;
	}
}

gboolean
rena_subsonic_api_is_authtenticated (RenaSubsonicApi *subsonic)
{
	return subsonic->authenticated;
}

gboolean
rena_subsonic_api_is_connected (RenaSubsonicApi *subsonic)
{
	return subsonic->has_connection;
}

gboolean
rena_subsonic_api_is_scanning (RenaSubsonicApi *subsonic)
{
	return subsonic->scanning;
}


GCancellable *
rena_subsonic_get_cancellable (RenaSubsonicApi *subsonic)
{
	return subsonic->cancellable;
}

GSList *
rena_subsonic_api_get_songs_list (RenaSubsonicApi *subsonic)
{
	if (subsonic->scanning == TRUE)
		return NULL;

	return subsonic->songs_list;
}

/*
 * RenaSubsonicApi
 */
static void
rena_subsonic_api_finalize (GObject *object)
{
	RenaSubsonicApi *subsonic = RENA_SUBSONIC_API(object);

	if (subsonic->scanning == TRUE)
		rena_subsonic_api_cancel (subsonic);

	g_queue_free_full (subsonic->albums_queue,
	                   (GDestroyNotify) g_free);

	g_slist_free_full (subsonic->songs_list,
	                   (GDestroyNotify) g_object_unref);

	g_object_unref(subsonic->cancellable);

	g_free (subsonic->server);
	g_free (subsonic->username);
	g_free (subsonic->password);

	G_OBJECT_CLASS(rena_subsonic_api_parent_class)->finalize(object);
}

static void
rena_subsonic_api_class_init (RenaSubsonicApiClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = rena_subsonic_api_finalize;

	signals[SIGNAL_AUTH_DONE] =
		g_signal_new ("authenticated",
		              G_TYPE_FROM_CLASS (object_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (RenaSubsonicApiClass, authenticated),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__INT,
		              G_TYPE_NONE, 1, G_TYPE_INT);
	signals[SIGNAL_PING_DONE] =
		g_signal_new ("pong",
		              G_TYPE_FROM_CLASS (object_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (RenaSubsonicApiClass, pong),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__INT,
		              G_TYPE_NONE, 1, G_TYPE_INT);
	signals[SIGNAL_SCAN_PROGRESS] =
		g_signal_new ("scan-progress",
		              G_TYPE_FROM_CLASS (object_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (RenaSubsonicApiClass, scan_progress),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__INT,
		              G_TYPE_NONE, 1, G_TYPE_INT);
	signals[SIGNAL_SCAN_TOTAL] =
		g_signal_new ("scan-total",
		              G_TYPE_FROM_CLASS (object_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (RenaSubsonicApiClass, scan_total),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__INT,
		              G_TYPE_NONE, 1, G_TYPE_INT);
	signals[SIGNAL_SCAN_DONE] =
		g_signal_new ("scan-finished",
		              G_TYPE_FROM_CLASS (object_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (RenaSubsonicApiClass, scan_finished),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__INT,
		              G_TYPE_NONE, 1, G_TYPE_INT);
}

static void
rena_subsonic_api_init (RenaSubsonicApi *subsonic)
{
	subsonic->server = NULL;
	subsonic->username = NULL;
	subsonic->password = NULL;

	subsonic->glrnet = grl_net_wc_new ();
	//grl_net_wc_set_throttling (subsonic->glrnet, 1);
	subsonic->cancellable = g_cancellable_new ();

	subsonic->albums_offset = 0;
	subsonic->albums_queue = g_queue_new ();
	subsonic->albums_count = 0;
	subsonic->albums_progress = 0;

	subsonic->songs_list = NULL;

	subsonic->authenticated = FALSE;
	subsonic->has_connection = FALSE;
	subsonic->scanning = FALSE;
}

RenaSubsonicApi *
rena_subsonic_api_new (void)
{
	return RENA_SUBSONIC_API(g_object_new (RENA_TYPE_SUBSONIC_API, NULL));
}

