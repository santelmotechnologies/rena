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

#include <glib.h>
#include <glib-object.h>
#include <gmodule.h>
#include <gtk/gtk.h>

#include <gio/gio.h>

#include <libpeas/peas.h>
#include <libpeas-gtk/peas-gtk.h>

#ifdef HAVE_GRILO_NET3
#include <grilo-0.3/net/grl-net.h>
#endif
#ifdef HAVE_GRILO_NET2
#include <grilo-0.2/net/grl-net.h>
#endif

#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include "rena-ampache-plugin.h"

#include "src/rena.h"
#include "src/rena-app-notification.h"
#include "src/rena-favorites.h"
#include "src/rena-utils.h"
#include "src/rena-musicobject-mgmt.h"
#include "src/rena-music-enum.h"
#include "src/rena-playlist.h"
#include "src/rena-menubar.h"
#include "src/rena-musicobject.h"
#include "src/rena-musicobject-mgmt.h"
#include "src/rena-window.h"
#include "src/rena-hig.h"
#include "src/rena-database-provider.h"
#include "src/rena-background-task-bar.h"
#include "src/rena-background-task-widget.h"
#include "src/rena-song-cache.h"
#include "plugins/rena-plugin-macros.h"


typedef struct _RenaAmpachePluginPrivate RenaAmpachePluginPrivate;

struct _RenaAmpachePluginPrivate {
	RenaApplication          *rena;

	RenaFavorites            *favorites;
	RenaSongCache            *cache;
	RenaDatabaseProvider     *db_provider;

	GrlNetWc                   *glrnet;
	GCancellable               *cancellable;

	gchar                      *server;

	gchar                      *username;
	gchar                      *auth;
	gchar                      *version;
	gint                        songs_count;
	gboolean                    upgrade;
	gboolean                    implement_flags;

	guint                       ping_timer_id;

	gint                        pending_threads;
	GHashTable                 *tracks_table;
	gint                        songs_cache;
	GHashTable                 *favorites_table;

	RenaBackgroundTaskWidget *task_widget;

	GtkWidget                  *setting_widget;
	GtkWidget                  *server_entry;
	GtkWidget                  *user_entry;
	GtkWidget                  *pass_entry;

	GtkActionGroup             *action_group_main_menu;
	guint                       merge_id_main_menu;
};

RENA_PLUGIN_REGISTER (RENA_TYPE_AMPACHE_PLUGIN,
                        RenaAmpachePlugin,
                        rena_ampache_plugin)

/*
 * Definitions
 */

#define GROUP_KEY_AMPACHE  "ampache"
#define KEY_AMPACHE_SERVER "server"
#define KEY_AMPACHE_USER   "username"
#define KEY_AMPACHE_PASS   "password"

/*
 * Propotypes
 */

static gboolean
rena_musicobject_is_ampache_file (RenaMusicobject *mobj);

static void
rena_ampache_plugin_authenticate (RenaAmpachePlugin *plugin);
static void
rena_ampache_plugin_deauthenticate (RenaAmpachePlugin *plugin);

static void
rena_ampache_plugin_set_need_upgrade (RenaAmpachePlugin *plugin, gboolean upgrade);
static gboolean
rena_ampache_plugin_need_upgrade (RenaAmpachePlugin *plugin);

/*
 * Menu actions
 */

static void
rena_ampache_plugin_upgrade_database (RenaAmpachePlugin *plugin)
{
	rena_ampache_plugin_deauthenticate (plugin);
	rena_ampache_plugin_set_need_upgrade (plugin, TRUE);
	rena_ampache_plugin_authenticate (plugin);
}

static void
rena_ampache_plugin_upgrade_database_action (GtkAction *action, RenaAmpachePlugin *plugin)
{
	rena_ampache_plugin_upgrade_database (plugin);
}

static void
rena_ampache_plugin_upgrade_database_gmenu_action (GSimpleAction *action,
                                                     GVariant      *parameter,
                                                     gpointer       user_data)
{
	RenaAmpachePlugin *plugin = user_data;
	rena_ampache_plugin_upgrade_database (plugin);
}

static const GtkActionEntry main_menu_actions [] = {
	{"Refresh the Ampache library", NULL, N_("Refresh the Ampache library"),
	 "", "Refresh the Ampache library", G_CALLBACK(rena_ampache_plugin_upgrade_database_action)}};

static const gchar *main_menu_xml = "<ui>								\
	<menubar name=\"Menubar\">											\
		<menu action=\"ToolsMenu\">										\
			<placeholder name=\"rena-plugins-placeholder\">			\
				<menuitem action=\"Refresh the Ampache library\"/>		\
				<separator/>											\
			</placeholder>												\
		</menu>															\
	</menubar>															\
</ui>";

/* Helpers */

static gchar *
rena_ampache_raw_url_get_song_id (const gchar *raw_uri)
{
	GRegex *regex = NULL;
	GMatchInfo *match_info;
	gchar *song_id = NULL;

	regex = g_regex_new ("[\\?&]oid=([^&#]*)",
	                     G_REGEX_MULTILINE | G_REGEX_RAW, 0, NULL);
	if (g_regex_match (regex, raw_uri, 0, &match_info)) {
		song_id = g_match_info_fetch(match_info, 1);
	}
	g_regex_unref(regex);
	g_match_info_free (match_info);

	return song_id;
}

static gchar *
rena_ampache_raw_url_get_clean_url (const gchar *raw_uri)
{
	GRegex *regex = NULL;
	gchar *url = NULL;

	regex = g_regex_new ("ssid=(.[^&]*)&",
	                     G_REGEX_MULTILINE | G_REGEX_RAW, 0, NULL);
	url = g_regex_replace_literal (regex, raw_uri, -1, 0, "", 0, NULL);
	g_regex_unref(regex);

	return url;
}

static void
rena_ampache_plugin_add_track_db (gpointer key,
                                    gpointer value,
                                    gpointer user_data)
{
	RenaMusicobject *mobj = value;
	RenaDatabase *database = user_data;

	rena_database_add_new_musicobject (database, mobj);

	rena_process_gtk_events ();
}

static void
rena_ampache_plugin_add_favorites (gpointer key,
                                     gpointer value,
                                     gpointer user_data)
{
	guint playlist_id = 0;
	const gchar *filename = NULL;

	RenaMusicobject *mobj = value;
	RenaDatabase *database = user_data;

	playlist_id = rena_database_find_playlist (database, _("Favorites on Ampache"));
	if (!playlist_id)
		playlist_id = rena_database_add_new_playlist (database, _("Favorites on Ampache"));

	filename  = rena_musicobject_get_file (mobj);
	rena_database_add_playlist_track (database, playlist_id, filename);

	rena_process_gtk_events ();
}

/*
 * Basic Cache.
 */

static void
rena_ampache_tracks_cache_insert (RenaAmpachePlugin *plugin,
                                    RenaMusicobject   *mobj)
{
	RenaAmpachePluginPrivate *priv = plugin->priv;
	g_hash_table_insert (priv->tracks_table,
	                     g_strdup(rena_musicobject_get_file(mobj)),
	                     mobj);
}

static void
rena_ampache_favorites_cache_insert (RenaAmpachePlugin *plugin,
                                       RenaMusicobject   *mobj)
{
	RenaAmpachePluginPrivate *priv = plugin->priv;
	g_hash_table_insert (priv->favorites_table,
	                     g_strdup(rena_musicobject_get_file(mobj)),
	                     mobj);
}


/*
 * Settings.
 */

static gchar *
rena_ampache_plugin_get_server (RenaPreferences *preferences)
{
	gchar *plugin_group = NULL, *string = NULL;
	plugin_group = rena_preferences_get_plugin_group_name (preferences, GROUP_KEY_AMPACHE);

	string = rena_preferences_get_string (preferences,
	                                        plugin_group,
	                                        KEY_AMPACHE_SERVER);

	g_free (plugin_group);

	return string;
}

static void
rena_ampache_plugin_set_server (RenaPreferences *preferences, const gchar *server)
{
	gchar *plugin_group = NULL;
	plugin_group = rena_preferences_get_plugin_group_name (preferences, GROUP_KEY_AMPACHE);

	if (string_is_not_empty(server))
		rena_preferences_set_string (preferences,
		                               plugin_group,
		                               KEY_AMPACHE_SERVER,
		                               server);
	else
 		rena_preferences_remove_key (preferences,
		                               plugin_group,
		                               KEY_AMPACHE_SERVER);

	g_free (plugin_group);
}

static gchar *
rena_ampache_plugin_get_user (RenaPreferences *preferences)
{
	gchar *plugin_group = NULL, *string = NULL;
	plugin_group = rena_preferences_get_plugin_group_name (preferences, GROUP_KEY_AMPACHE);

	string = rena_preferences_get_string (preferences,
	                                        plugin_group,
	                                        KEY_AMPACHE_USER);

	g_free (plugin_group);

	return string;
}

static void
rena_ampache_plugin_set_user (RenaPreferences *preferences, const gchar *user)
{
	gchar *plugin_group = NULL;
	plugin_group = rena_preferences_get_plugin_group_name (preferences, GROUP_KEY_AMPACHE);

	if (string_is_not_empty(user))
		rena_preferences_set_string (preferences,
		                               plugin_group,
		                               KEY_AMPACHE_USER,
		                               user);
	else
		rena_preferences_remove_key (preferences,
		                               plugin_group,
		                               KEY_AMPACHE_USER);

	g_free (plugin_group);
}

static gchar *
rena_ampache_plugin_get_password (RenaPreferences *preferences)
{
	gchar *plugin_group = NULL, *string = NULL;
	plugin_group = rena_preferences_get_plugin_group_name (preferences, GROUP_KEY_AMPACHE);

	string = rena_preferences_get_string (preferences,
	                                        plugin_group,
	                                        KEY_AMPACHE_PASS);

	g_free (plugin_group);

	return string;
}

static void
rena_ampache_plugin_set_password (RenaPreferences *preferences, const gchar *pass)
{
	gchar *plugin_group = NULL;
	plugin_group = rena_preferences_get_plugin_group_name (preferences, GROUP_KEY_AMPACHE);

	if (string_is_not_empty(pass))
		rena_preferences_set_string (preferences,
		                               plugin_group,
		                               KEY_AMPACHE_PASS,
		                               pass);
	else
 		rena_preferences_remove_key (preferences,
		                               plugin_group,
		                               KEY_AMPACHE_PASS);

	g_free (plugin_group);
}


/*
 * Ampache plugin.
 */

RenaMusicobject *
rena_ampache_xml_get_media (xmlDocPtr doc, xmlNodePtr node)
{
	RenaMusicobject *mobj = NULL;
	gchar *raw_url = NULL, *url = NULL, *title =  NULL, *artist = NULL, \
		*album = NULL, *genre = NULL, *comment = NULL;
	gchar *tracknoc = NULL, *yearc = NULL, *lenghtc = NULL;
	gint trackno = 0, year = 0, lenght = 0;

	node = node->xmlChildrenNode;
	while(node)
	{
		if (!xmlStrcmp (node->name, (const xmlChar *) "url"))
		{
			raw_url = (gchar *) xmlNodeListGetString (doc, node->xmlChildrenNode, 1);
			url = rena_ampache_raw_url_get_clean_url (raw_url);
			g_free (raw_url);
		}
		if (!xmlStrcmp (node->name, (const xmlChar *) "track"))
		{
			tracknoc = (gchar *) xmlNodeListGetString (doc, node->xmlChildrenNode, 1);
			trackno = string_is_not_empty(tracknoc) ? atoi (tracknoc) : 0;
			g_free (tracknoc);
		}
		if (!xmlStrcmp (node->name, (const xmlChar *) "title"))
		{
			 title = (gchar *) xmlNodeListGetString (doc, node->xmlChildrenNode, 1);
		}
		if (!xmlStrcmp (node->name, (const xmlChar *) "artist"))
		{
			 artist = (gchar *) xmlNodeListGetString (doc, node->xmlChildrenNode, 1);
		}
		if (!xmlStrcmp (node->name, (const xmlChar *) "album"))
		{
			 album = (gchar *) xmlNodeListGetString (doc, node->xmlChildrenNode, 1);
		}
		if (!xmlStrcmp (node->name, (const xmlChar *) "year"))
		{
			yearc = (gchar *) xmlNodeListGetString (doc, node->xmlChildrenNode, 1);
			year = string_is_not_empty(yearc) ? atoi (yearc) : 0;
			g_free (yearc);
		}
		if (!xmlStrcmp (node->name, (const xmlChar *) "genre"))
		{
			 genre = (gchar *) xmlNodeListGetString (doc, node->xmlChildrenNode, 1);
		}
		if (!xmlStrcmp (node->name, (const xmlChar *) "comment"))
		{
			 comment = (gchar *) xmlNodeListGetString (doc, node->xmlChildrenNode, 1);
		}
		if (!xmlStrcmp (node->name, (const xmlChar *) "time"))
		{
			lenghtc = (gchar *) xmlNodeListGetString (doc, node->xmlChildrenNode, 1);
			lenght = string_is_not_empty(lenghtc) ? atoi (lenghtc) : 0;
			g_free (lenghtc);
		}

		node = node->next;
	}

	mobj = g_object_new (RENA_TYPE_MUSICOBJECT,
	                     "file", url,
	                     "source", FILE_HTTP,
	                     "track-no", trackno,
	                     "title", title != NULL ? title : "",
	                     "artist", artist != NULL ? artist : "",
	                     "album", album != NULL ? album : "",
	                     "year", year,
	                     "genre", genre != NULL ? genre : "",
	                     "comment", comment != NULL ? comment : "",
	                     "length", lenght,
	                     NULL);

	g_free (url);
	g_free (title);
	g_free (artist);
	g_free (album);
	g_free (genre);
	g_free (comment);

	return mobj;
}

static void
rena_ampache_plugin_import_finished (RenaAmpachePlugin *plugin)
{
	RenaDatabase *database;
	RenaDatabaseProvider *provider;
	RenaBackgroundTaskBar *taskbar;

	RenaAmpachePluginPrivate *priv = plugin->priv;

	/* Remove background task widget. */

	taskbar = rena_background_task_bar_get ();
	rena_background_task_bar_remove_widget (taskbar, GTK_WIDGET(priv->task_widget));
	g_object_unref(G_OBJECT(taskbar));

	/* If cancelled just clean and return */

	if (g_cancellable_is_cancelled (priv->cancellable))
	{
		g_hash_table_remove_all (priv->tracks_table);
		g_hash_table_remove_all (priv->favorites_table);

		g_cancellable_reset (priv->cancellable);

		return;
	}

	/* Generate provider */

	database = rena_database_get ();
	provider = rena_database_provider_get ();
	if (rena_database_find_provider (database, priv->server))
	{
		rena_provider_forget_songs (provider, priv->server);
	}
	else
	{
		rena_provider_add_new (provider,
		                         priv->server,
		                         "AMPACHE",
		                         priv->server,
		                         "folder-remote");
		rena_provider_set_visible (provider, priv->server, TRUE);
	}

	/* Insert songs and favorites */

	g_hash_table_foreach (priv->tracks_table,
	                      rena_ampache_plugin_add_track_db,
	                      database);

	g_hash_table_foreach (priv->favorites_table,
	                      rena_ampache_plugin_add_favorites,
	                      database);

	/* Inform provides changes */

	rena_provider_update_done (provider);

	/* Clean */

	g_hash_table_remove_all (priv->tracks_table);
	g_hash_table_remove_all (priv->favorites_table);

	priv->songs_cache = 0;

	g_object_unref (provider);
	g_object_unref (database);
}

static void
rena_ampache_get_flagged_done (GObject      *object,
                                 GAsyncResult *res,
                                 gpointer      user_data)
{
	GError *wc_error = NULL;
	gchar *content = NULL;
	RenaMusicobject *mobj;
	xmlDocPtr doc;
	xmlNodePtr node;

	RenaAmpachePlugin *plugin = user_data;
	RenaAmpachePluginPrivate *priv = plugin->priv;

	/* Set thread as handled */

	priv->pending_threads--;

	/* Check error */

	if (!grl_net_wc_request_finish (GRL_NET_WC (object),
	                                res,
	                                &content,
	                                NULL,
	                                &wc_error))
	{
		if (!g_cancellable_is_cancelled (priv->cancellable))
			g_warning ("Failed to get flagged songs: %s", wc_error->message);
	}

	if (content)
	{
		doc = xmlReadMemory (content, strlen(content), NULL, NULL,
		                     XML_PARSE_RECOVER | XML_PARSE_NOBLANKS);

		node = xmlDocGetRootElement (doc);
		node = node->xmlChildrenNode;

		while(node)
		{
			if (!xmlStrcmp (node->name, (const xmlChar *) "song"))
			{
				mobj = rena_ampache_xml_get_media (doc, node);
				if (G_LIKELY(mobj)) {
					rena_musicobject_set_provider (mobj, priv->server);
					rena_ampache_favorites_cache_insert (plugin, mobj);
				}

				/* Have to give control to GTK periodically ... */
				rena_process_gtk_events ();
			}
			node = node->next;
 		}

		xmlFreeDoc (doc);

	}

	/* If it is last response finishing import */

	if (priv->pending_threads == 0)
	{
		rena_ampache_plugin_import_finished (plugin);
	}

}

static void
rena_ampache_get_songs_done (GObject      *object,
                               GAsyncResult *res,
                               gpointer      user_data)
{
	RenaMusicobject *mobj;
	GError *wc_error = NULL;
	xmlDocPtr doc;
	xmlNodePtr node;
	gchar *content = NULL, *summary = NULL;

	RenaAmpachePlugin *plugin = user_data;
	RenaAmpachePluginPrivate *priv = plugin->priv;

	/* Set thread as handled */

	priv->pending_threads--;

	/* Check error */

	if (!grl_net_wc_request_finish (GRL_NET_WC (object),
	                                res,
	                                &content,
	                                NULL,
	                                &wc_error))
	{
		if (!g_cancellable_is_cancelled (priv->cancellable))
			g_warning ("Failed to get songs: %s", wc_error->message);
	}

	if (content)
	{
		doc = xmlReadMemory (content, strlen(content), NULL, NULL,
		                     XML_PARSE_RECOVER | XML_PARSE_NOBLANKS);

		node = xmlDocGetRootElement (doc);
		node = node->xmlChildrenNode;

		while(node)
		{
			if (!xmlStrcmp (node->name, (const xmlChar *) "song"))
			{
				mobj = rena_ampache_xml_get_media (doc, node);
				if (G_LIKELY(mobj)) {
					rena_musicobject_set_provider (mobj, priv->server);
					rena_ampache_tracks_cache_insert (plugin, mobj);
				}
				priv->songs_cache++;

				/* Have to give control to GTK periodically ... */
				rena_process_gtk_events ();
			}
			node = node->next;
 		}

		xmlFreeDoc (doc);
	}

	/* Show status update to user. */

	if (priv->pending_threads > 0)
	{
		if (priv->songs_count > 0)
		{
			rena_background_task_widget_set_job_progress (priv->task_widget,
			                                                100*priv->songs_cache/priv->songs_count);
		}
		summary = g_strdup_printf(_("%i files analyzed of %i detected"),
		                          priv->songs_cache, priv->songs_count);
		rena_background_task_widget_set_description(priv->task_widget, summary);
		g_free (summary);
	}

	/* If last thread save on database. */

	if (priv->pending_threads == 0)
	{
		rena_ampache_plugin_import_finished (plugin);
	}
}

static void
rena_ampache_plugin_cache_music (RenaAmpachePlugin *plugin)
{
	RenaAppNotification *notification;
	RenaBackgroundTaskBar *taskbar;
	gchar *url = NULL;
	const guint limit = 250;
	guint i = 0;

	RenaAmpachePluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "Ampache server plugin %s", G_STRFUNC);

	if (!priv->auth)
		return;

	if (!priv->songs_count) {
		notification = rena_app_notification_new ("Ampache", _("The server has no songs to play"));
		rena_app_notification_show (notification);
		return;
	}

	/* Add the taks manager */

	taskbar = rena_background_task_bar_get ();
	rena_background_task_bar_prepend_widget (taskbar, GTK_WIDGET(priv->task_widget));
	g_object_unref(G_OBJECT(taskbar));

	/* Launch threads to get music */

	priv->pending_threads = priv->songs_count/limit + 1;
	for (i = 0 ; i < priv->pending_threads; i++)
	{
		url = g_strdup_printf("%s/server/xml.server.php?action=songs&offset=%i&limit=%i&auth=%s",
		                      priv->server, i*limit, limit, priv->auth);

		grl_net_wc_request_async (priv->glrnet,
		                          url,
		                          priv->cancellable,
		                          rena_ampache_get_songs_done,
		                          plugin);
		g_free (url);
	}

	/* Finally launch get favorites as last thread */

	if (priv->implement_flags) {
		priv->pending_threads++;

		url = g_strdup_printf("%s/server/xml.server.php?action=stats&type=song&filter=flagged&auth=%s",
		                      priv->server, priv->auth);

		grl_net_wc_request_async (priv->glrnet,
		                          url,
		                          priv->cancellable,
		                          rena_ampache_get_flagged_done,
		                          plugin);
		g_free (url);
	}
}

static void
rena_ampache_provider_want_upgrade (RenaDatabaseProvider *provider,
                                      gint                    provider_id,
                                      RenaAmpachePlugin    *plugin)
{
	RenaDatabase *database;
	RenaPreparedStatement *statement;
	const gchar *sql, *provider_type = NULL;

	sql = "SELECT name FROM provider_type WHERE id IN (SELECT type FROM provider WHERE id = ?)";

	database = rena_database_get ();
	statement = rena_database_create_statement (database, sql);
	rena_prepared_statement_bind_int (statement, 1, provider_id);
	if (rena_prepared_statement_step (statement))
		provider_type = rena_prepared_statement_get_string (statement, 0);

	if (g_ascii_strcasecmp (provider_type, "ampache") == 0)
	{
		rena_ampache_plugin_upgrade_database (plugin);
	}
	rena_prepared_statement_free (statement);
	g_object_unref (database);
}


/*
 * Ampache Settings
 */
static void
rena_ampache_preferences_dialog_response (GtkDialog           *dialog,
                                            gint                 response_id,
                                            RenaAmpachePlugin *plugin)
{
	RenaDatabase *database;
	RenaDatabaseProvider *provider;
	RenaPreferences *preferences;
	const gchar *entry_server = NULL, *entry_user = NULL, *entry_pass = NULL;
	gchar *test_server = NULL, *test_user = NULL, *test_pass = NULL;
	gboolean changed = FALSE, changed_server = FALSE;

	RenaAmpachePluginPrivate *priv = plugin->priv;

	preferences = rena_preferences_get ();

	test_server = rena_ampache_plugin_get_server (preferences);
	test_user = rena_ampache_plugin_get_user (preferences);
	test_pass = rena_ampache_plugin_get_password (preferences);

	switch(response_id)
	{
		case GTK_RESPONSE_CANCEL:
			rena_gtk_entry_set_text (GTK_ENTRY(priv->server_entry), test_server);
			rena_gtk_entry_set_text (GTK_ENTRY(priv->user_entry), test_user);
			rena_gtk_entry_set_text (GTK_ENTRY(priv->pass_entry), test_pass);
			break;
		case GTK_RESPONSE_OK:
			entry_server = gtk_entry_get_text (GTK_ENTRY(priv->server_entry));
			entry_user = gtk_entry_get_text (GTK_ENTRY(priv->user_entry));
			entry_pass = gtk_entry_get_text (GTK_ENTRY(priv->pass_entry));

			if (g_strcmp0 (test_server, entry_server)) {
				rena_ampache_plugin_set_server (preferences, entry_server);
				changed = TRUE;
				changed_server = TRUE;
			}
			if (g_strcmp0 (test_user, entry_user)) {
				rena_ampache_plugin_set_user (preferences, entry_user);
				changed = TRUE;
			}
			if (g_strcmp0 (test_pass, entry_pass)) {
				rena_ampache_plugin_set_password (preferences, entry_pass);
				changed = TRUE;
			}

			if (changed)
			{
				/* Deauthenticate connection */

				rena_ampache_plugin_deauthenticate (plugin);

				/* Remove old provider if exist */

				if (changed_server)
				{
					database = rena_database_get ();
					if (rena_database_find_provider (database, test_server)) {
						provider = rena_database_provider_get ();
						rena_provider_remove (provider, test_server);
						rena_provider_update_done (provider);
						g_object_unref (provider);
					}
					g_object_unref (database);
				}

				/* With all mandatory fields updates the collection.*/

				if (string_is_not_empty(entry_server) &&
				    string_is_not_empty(entry_user) &&
				    string_is_not_empty(entry_pass))
				{
					rena_ampache_plugin_set_need_upgrade (plugin, TRUE);
					rena_ampache_plugin_authenticate (plugin);
				}
			}
			break;
		default:
			break;
	}

	g_object_unref (preferences);

	g_free (test_server);
	g_free (test_user);
	g_free (test_pass);
}

static void
rena_ampache_plugin_append_setting (RenaAmpachePlugin *plugin)
{
	RenaPreferences *preferences;
	RenaPreferencesDialog *dialog;
	GtkWidget *table, *label, *ampache_server, *ampache_uname, *ampache_pass;
	gchar *server = NULL, *user = NULL, *pass = NULL;
	guint row = 0;

	RenaAmpachePluginPrivate *priv = plugin->priv;

	preferences = rena_preferences_get ();

	table = rena_hig_workarea_table_new ();

	rena_hig_workarea_table_add_section_title (table, &row, "Ampache");

	label = gtk_label_new (_("Server"));
	ampache_server = gtk_entry_new ();

	server = rena_ampache_plugin_get_server (preferences);
	rena_gtk_entry_set_text (GTK_ENTRY(ampache_server), server);

	gtk_entry_set_icon_from_icon_name (GTK_ENTRY(ampache_server), GTK_ENTRY_ICON_PRIMARY, "network-server");
	gtk_entry_set_activates_default (GTK_ENTRY(ampache_server), TRUE);

	rena_hig_workarea_table_add_row (table, &row, label, ampache_server);


	label = gtk_label_new (_("Username"));
	ampache_uname = gtk_entry_new ();

	user = rena_ampache_plugin_get_user (preferences);
	rena_gtk_entry_set_text (GTK_ENTRY(ampache_uname), user);

	gtk_entry_set_icon_from_icon_name (GTK_ENTRY(ampache_uname), GTK_ENTRY_ICON_PRIMARY, "system-users");
	gtk_entry_set_max_length (GTK_ENTRY(ampache_uname), LASTFM_UNAME_LEN);
	gtk_entry_set_activates_default (GTK_ENTRY(ampache_uname), TRUE);

	rena_hig_workarea_table_add_row (table, &row, label, ampache_uname);

	label = gtk_label_new (_("Password"));
	ampache_pass = gtk_entry_new ();

	pass = rena_ampache_plugin_get_password (preferences);
	rena_gtk_entry_set_text (GTK_ENTRY(ampache_pass), pass);

	gtk_entry_set_icon_from_icon_name (GTK_ENTRY(ampache_pass), GTK_ENTRY_ICON_PRIMARY, "changes-prevent");
	gtk_entry_set_max_length (GTK_ENTRY(ampache_pass), LASTFM_PASS_LEN);
	gtk_entry_set_visibility (GTK_ENTRY(ampache_pass), FALSE);
	gtk_entry_set_activates_default (GTK_ENTRY(ampache_pass), TRUE);

	rena_hig_workarea_table_add_row (table, &row, label, ampache_pass);

	/* Append panes */

	priv->setting_widget = table;
	priv->server_entry = ampache_server;
	priv->user_entry = ampache_uname;
	priv->pass_entry = ampache_pass;

	dialog = rena_application_get_preferences_dialog (priv->rena);
	rena_preferences_append_services_setting (dialog,
	                                            priv->setting_widget, FALSE);

	/* Configure handler and settings */
	rena_preferences_dialog_connect_handler (dialog,
	                                           G_CALLBACK(rena_ampache_preferences_dialog_response),
	                                           plugin);

	g_object_unref (preferences);

	g_free (server);
	g_free (user);
	g_free (pass);
}

static void
rena_ampache_plugin_remove_setting (RenaAmpachePlugin *plugin)
{
	RenaPreferencesDialog *dialog;
	RenaAmpachePluginPrivate *priv = plugin->priv;

	dialog = rena_application_get_preferences_dialog (priv->rena);
	rena_preferences_remove_services_setting (dialog,
	                                            priv->setting_widget);

	rena_preferences_dialog_disconnect_handler (dialog,
	                                              G_CALLBACK(rena_ampache_preferences_dialog_response),
	                                              plugin);
}


/*
 * Interactions.
 */
static void
rena_ampache_plugin_flag_done (GObject      *object,
                                 GAsyncResult *res,
                                 gpointer user_data)
{
	GError *wc_error = NULL;
	gchar *content = NULL;

	RenaAmpachePlugin *plugin = RENA_AMPACHE_PLUGIN(user_data);
	RenaAmpachePluginPrivate *priv = plugin->priv;

	if (!grl_net_wc_request_finish (GRL_NET_WC (object),
	                                res,
	                                &content,
	                                NULL,
	                                &wc_error))
	{
		if (!g_cancellable_is_cancelled (priv->cancellable))
			g_critical("Ampache ERROR Response: %s", content);
	}
}

static void
rena_ampache_plugin_flag_launch (RenaAmpachePlugin *plugin,
                                   const gchar         *filename,
                                   gboolean             flag)
{
	gchar *url = NULL, *song_id = NULL;

	RenaAmpachePluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "Ampache server plugin %s", G_STRFUNC);

	if (!priv->auth)
		return;

	song_id = rena_ampache_raw_url_get_song_id (filename);

	url = g_strdup_printf("%s/server/xml.server.php?action=flag&type=song&id=%s&flag=%i&auth=%s",
	                      priv->server,
	                      song_id,
	                      flag ? 1 : 0,
	                      priv->auth);

	grl_net_wc_request_async (priv->glrnet,
	                          url,
	                          priv->cancellable,
	                          rena_ampache_plugin_flag_done,
	                          plugin);

	g_free (song_id);
	g_free (url);
}

static void
rena_ampache_plugin_favorites_song_added (RenaFavorites     *favorites,
                                            RenaMusicobject   *mobj,
                                            RenaAmpachePlugin *plugin)
{
	RenaDatabase *database;
	const gchar *file = NULL;
	gint playlist_id = 0;

	RenaAmpachePluginPrivate *priv = plugin->priv;

	if (!priv->implement_flags)
		return;

	if (!rena_musicobject_is_ampache_file (mobj))
		return;

	file  = rena_musicobject_get_file (mobj);
	rena_ampache_plugin_flag_launch (plugin, file, TRUE);

	database = rena_database_get ();
	playlist_id = rena_database_find_playlist (database, _("Favorites on Ampache"));
	if (!playlist_id)
		playlist_id = rena_database_add_new_playlist (database, _("Favorites on Ampache"));
	rena_database_add_playlist_track (database, playlist_id, file);
	g_object_unref (database);
}

static void
rena_ampache_plugin_favorites_song_removed (RenaFavorites     *favorites,
                                              RenaMusicobject   *mobj,
                                              RenaAmpachePlugin *plugin)
{
	RenaDatabase *database;
	const gchar *file = NULL;
	gint playlist_id = 0;

	RenaAmpachePluginPrivate *priv = plugin->priv;

	if (!priv->implement_flags)
		return;

	if (!rena_musicobject_is_ampache_file (mobj))
		return;

	file  = rena_musicobject_get_file (mobj);
	rena_ampache_plugin_flag_launch (plugin, file, FALSE);

	database = rena_database_get ();
	playlist_id = rena_database_find_playlist (database, _("Favorites on Ampache"));
	rena_database_delete_playlist_track (database, playlist_id, file);
	g_object_unref (database);
}

/*
 * Authentication.
 */

static void
rena_ampache_ping_server_done (GObject      *object,
                                 GAsyncResult *res,
                                 gpointer      user_data)
{
	GError *wc_error = NULL;
	gchar *content = NULL;

	RenaAmpachePlugin *plugin = user_data;
	RenaAmpachePluginPrivate *priv = plugin->priv;

	if (!grl_net_wc_request_finish (GRL_NET_WC (object),
	                                res,
	                                &content,
	                                NULL,
	                                &wc_error))
	{
		if (!g_cancellable_is_cancelled (priv->cancellable))
			g_warning ("Failed to ping server: %s", wc_error->message);
		return;
	}

	if (content)
	{
		/* Just debug.. The documentation does not specify the result. */
		CDEBUG(DBG_PLUGIN, "Ampache Server plugin %s", G_STRFUNC);
	}
}

static gboolean
rena_ampache_plugin_ping_server (gpointer user_data)
{
	gchar *url = NULL;
	const gchar *server = NULL;

	RenaAmpachePlugin *plugin = user_data;
	RenaAmpachePluginPrivate *priv = plugin->priv;

	server = gtk_entry_get_text (GTK_ENTRY(priv->server_entry));

	url = g_strdup_printf ("%s/server/xml.server.php?action=ping&auth=%s",
	                      server, priv->auth);

	grl_net_wc_request_async (priv->glrnet,
	                          url,
	                          priv->cancellable,
	                          rena_ampache_ping_server_done,
	                          plugin);

	g_free (url);

	return TRUE;
}

static void
rena_ampache_get_auth_done (GObject      *object,
                              GAsyncResult *res,
                              gpointer      user_data)
{
	RenaAppNotification *notification;
	GError *wc_error = NULL;
	gchar *content = NULL, *songs_count = NULL;
	xmlDocPtr doc;
	xmlNodePtr node;

	RenaAmpachePlugin *plugin = user_data;
	RenaAmpachePluginPrivate *priv = plugin->priv;

	if (!grl_net_wc_request_finish (GRL_NET_WC (object),
	                                res,
	                                &content,
	                                NULL,
	                                &wc_error))
	{
		if (!g_cancellable_is_cancelled (priv->cancellable)) {
			notification = rena_app_notification_new ("Ampache", _("Unable to establish conection with Ampache"));
			rena_app_notification_show (notification);

			g_warning ("Failed to connect: %s", wc_error->message);
		}
		return;
	}

	if (content)
	{
		doc = xmlReadMemory (content, strlen(content), NULL, NULL,
		                     XML_PARSE_RECOVER | XML_PARSE_NOBLANKS);

		node = xmlDocGetRootElement (doc);

		node = node->xmlChildrenNode;
		while(node)
		{
			if (!xmlStrcmp (node->name, (const xmlChar *) "auth"))
			{
				priv->auth = (gchar *) xmlNodeListGetString (doc, node->xmlChildrenNode, 1);
			}
			if (!xmlStrcmp (node->name, (const xmlChar *) "api"))
			{
				priv->version = (gchar *) xmlNodeListGetString (doc, node->xmlChildrenNode, 1);
			}
			if (!xmlStrcmp (node->name, (const xmlChar *) "songs"))
			{
				songs_count = (gchar *) xmlNodeListGetString (doc, node->xmlChildrenNode, 1);
				priv->songs_count = atoi (songs_count);
				g_free (songs_count);
			}
			node = node->next;
		}

		xmlFreeDoc (doc);
	}

	if (priv->auth != NULL)
	{
		/* Disabled until ampache solves several problems */
		//priv->implement_flags = g_ascii_strcasecmp (priv->version, "400001") >= 0;

		priv->ping_timer_id = g_timeout_add_seconds (10*60,
		                                             rena_ampache_plugin_ping_server,
		                                             plugin);

		if (rena_ampache_plugin_need_upgrade (plugin))
		{
			rena_ampache_plugin_cache_music (plugin);
		}
	}
}

static void
rena_ampache_plugin_authenticate (RenaAmpachePlugin *plugin)
{
	GChecksum *hash = NULL;
	const gchar *server = NULL, *username = NULL, *password = NULL;
	gchar *url = NULL, *timestampc = NULL, *key = NULL, *passphraseh = NULL, *passphrasec = NULL;
	time_t timestamp = 0;

	RenaAmpachePluginPrivate *priv = plugin->priv;

	/* Get settings */

	server = gtk_entry_get_text (GTK_ENTRY(priv->server_entry));
	username = gtk_entry_get_text (GTK_ENTRY(priv->user_entry));
	password = gtk_entry_get_text (GTK_ENTRY(priv->pass_entry));

	if (string_is_empty (server))
		return;
	if (string_is_empty (username))
		return;
	if (string_is_empty (password))
		return;

	/* Set new server as alias of provider */

	priv->server = g_strdup (server);
	priv->username = g_strdup(username);

	/* Authenticate */

	time(&timestamp);
	timestampc = g_strdup_printf("%i", (gint)timestamp);

	hash = g_checksum_new (G_CHECKSUM_SHA256);
	g_checksum_update (hash, (guchar *)password, strlen(password));
	key = g_strdup(g_checksum_get_string(hash));

	g_checksum_reset (hash);

	passphrasec = g_strdup_printf("%s%s", timestampc, key);
	g_checksum_update(hash, (guchar *)passphrasec, -1);
	passphraseh = g_strdup(g_checksum_get_string(hash));

	url = g_strdup_printf("%s/server/xml.server.php?action=handshake&auth=%s&timestamp=%s&version=350001&user=%s",
	                      server, passphraseh, timestampc, username);

	grl_net_wc_request_async (priv->glrnet,
	                          url,
	                          priv->cancellable,
	                          rena_ampache_get_auth_done,
	                          plugin);

	g_checksum_free(hash);

	g_free (timestampc);
	g_free (key);
	g_free (passphrasec);
	g_free (passphraseh);
	g_free (url);
}

static void
rena_ampache_plugin_deauthenticate (RenaAmpachePlugin *plugin)
{
	RenaAmpachePluginPrivate *priv = plugin->priv;

	if (priv->server) {
		g_free (priv->server);
		priv->server = NULL;
	}
	if (priv->username) {
		g_free (priv->username);
		priv->username = NULL;
	}
	if (priv->auth) {
		g_free (priv->auth);
		priv->auth = NULL;
	}
	if (priv->songs_count > 0)
		priv->songs_count = 0;
	if (priv->songs_cache > 0)
		priv->songs_cache = 0;

	if (priv->ping_timer_id > 0) {
		g_source_remove (priv->ping_timer_id);
		priv->ping_timer_id = 0;
	}

	priv->upgrade = FALSE;
}

static void
rena_ampache_plugin_set_need_upgrade (RenaAmpachePlugin *plugin, gboolean upgrade)
{
	RenaAmpachePluginPrivate *priv = plugin->priv;

	priv->upgrade = upgrade;
}

static gboolean
rena_ampache_plugin_need_upgrade (RenaAmpachePlugin *plugin)
{
	RenaAmpachePluginPrivate *priv = plugin->priv;

	//TODO: Verify the update date on auth.
	return priv->upgrade;
}

/*
 * Gstreamer.source.
 */

static gboolean
rena_musicobject_is_ampache_file (RenaMusicobject *mobj)
{
	RenaMusicEnum *enum_map = NULL;
	RenaMusicSource file_source = FILE_NONE;

	enum_map = rena_music_enum_get ();
	file_source = rena_music_enum_map_get(enum_map, "AMPACHE");
	g_object_unref (enum_map);

	return (file_source == rena_musicobject_get_source (mobj));
}

static void
rena_ampache_plugin_prepare_source (RenaBackend       *backend,
                                      RenaAmpachePlugin *plugin)
{
	RenaMusicobject *mobj;
	const gchar *location = NULL;
	gchar *filename = NULL, *uri = NULL;

	RenaAmpachePluginPrivate *priv = plugin->priv;

	mobj = rena_backend_get_musicobject (backend);
	if (!rena_musicobject_is_ampache_file (mobj))
		return;

	location =  rena_musicobject_get_file (mobj);
	filename = rena_song_cache_get_from_location (priv->cache, location);
	if (filename != NULL) {
		uri = g_filename_to_uri (filename, NULL, NULL);
		g_free (filename);
	}
	else {
		uri = g_strdup_printf ("%s&ssid=%s", location, priv->auth);
	}
	rena_backend_set_playback_uri (backend, uri);
	g_free (uri);
}

static void
rena_ampache_plugin_download_done (RenaBackend       *backend,
                                     gchar               *filename,
                                     RenaAmpachePlugin *plugin)
{
	RenaMusicobject *mobj;
	const gchar *location = NULL;

	RenaAmpachePluginPrivate *priv = plugin->priv;

	mobj = rena_backend_get_musicobject (backend);
	if (!rena_musicobject_is_ampache_file (mobj))
		return;

	location = rena_musicobject_get_file (mobj);
	rena_song_cache_put_location (priv->cache, location, filename);
}

/*
 * Plugin.
 */

static void
rena_plugin_activate (PeasActivatable *activatable)
{
	RenaBackend *backend;
	GMenuItem *item;
	GSimpleAction *action;

	RenaAmpachePlugin *plugin = RENA_AMPACHE_PLUGIN (activatable);

	RenaAmpachePluginPrivate *priv = plugin->priv;
	priv->rena = g_object_get_data (G_OBJECT (plugin), "object");

	CDEBUG(DBG_PLUGIN, "Ampache Server plugin %s", G_STRFUNC);

	/* Default settings */

	priv->implement_flags = FALSE;

	/* New grilo network helper */

	priv->glrnet = grl_net_wc_new ();
	grl_net_wc_set_throttling (priv->glrnet, 1);

	priv->cancellable = g_cancellable_new ();

	/* New cache */

	priv->cache = rena_song_cache_get ();

	/* Temp tables.*/

	priv->tracks_table = g_hash_table_new_full (g_str_hash,
	                                            g_str_equal,
	                                            g_free,
	                                            g_object_unref);

	priv->favorites_table = g_hash_table_new_full (g_str_hash,
	                                               g_str_equal,
	                                               g_free,
	                                               g_object_unref);

	/* Updrade signals */

	priv->db_provider = rena_database_provider_get ();
	g_signal_connect (priv->db_provider, "want-upgrade",
	                  G_CALLBACK(rena_ampache_provider_want_upgrade), plugin);
	g_signal_connect (priv->db_provider, "want-update",
	                  G_CALLBACK(rena_ampache_provider_want_upgrade), plugin);
	g_object_ref (G_OBJECT(priv->db_provider));

	/* New Task widget */

	priv->task_widget = rena_background_task_widget_new (_("Searching files to analyze"),
	                                                       "network-server",
	                                                       100,
	                                                       priv->cancellable);
	g_object_ref (G_OBJECT(priv->task_widget));

	/* Attach main menu */

	priv->action_group_main_menu = rena_menubar_plugin_action_new ("RenaAmpachePlugin",
	                                                                 main_menu_actions,
	                                                                 G_N_ELEMENTS (main_menu_actions),
	                                                                 NULL,
	                                                                 0,
	                                                                 plugin);

	priv->merge_id_main_menu = rena_menubar_append_plugin_action (priv->rena,
	                                                                priv->action_group_main_menu,
	                                                                main_menu_xml);

	/* Gear Menu */

	action = g_simple_action_new ("refresh-ampache", NULL);
	g_signal_connect (G_OBJECT (action), "activate",
	                  G_CALLBACK (rena_ampache_plugin_upgrade_database_gmenu_action), plugin);

	item = g_menu_item_new (_("Refresh the Ampache library"), "win.refresh-ampache");
	rena_menubar_append_action (priv->rena, "rena-plugins-placeholder", action, item);
	g_object_unref (item);

	/* Backend signals */

	backend = rena_application_get_backend (priv->rena);
	rena_backend_set_local_storage (backend, TRUE);
	g_signal_connect (backend, "prepare-source",
	                  G_CALLBACK(rena_ampache_plugin_prepare_source), plugin);
	g_signal_connect (backend, "download-done",
	                  G_CALLBACK(rena_ampache_plugin_download_done), plugin);

	/* Favorites handler */

	priv->favorites = rena_favorites_get ();
	g_signal_connect (priv->favorites, "song-added",
	                  G_CALLBACK(rena_ampache_plugin_favorites_song_added), plugin);
	g_signal_connect (priv->favorites, "song-removed",
	                  G_CALLBACK(rena_ampache_plugin_favorites_song_removed), plugin);

	/* Append setting */

	rena_ampache_plugin_append_setting (plugin);

	/* Authenticate */

	rena_ampache_plugin_authenticate (plugin);
}

static void
rena_plugin_deactivate (PeasActivatable *activatable)
{
	RenaBackend *backend;
	RenaDatabaseProvider *provider;
	RenaPreferences *preferences;
	gchar *plugin_group = NULL;

	RenaAmpachePlugin *plugin = RENA_AMPACHE_PLUGIN (activatable);
	RenaAmpachePluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "Ampache Server plugin %s", G_STRFUNC);

	/* Cache */

	g_hash_table_destroy (priv->tracks_table);
	g_object_unref (priv->cache);

	/* Favorites */

	g_hash_table_destroy (priv->favorites_table);
	g_object_unref (priv->favorites);

	/* Network staff */

	g_object_unref (priv->glrnet);

	/* Provider signals */

	g_signal_handlers_disconnect_by_func (priv->db_provider, rena_ampache_provider_want_upgrade, plugin);
	g_object_unref (G_OBJECT(priv->db_provider));

	/* If user disable the plugin (Rena not shutdown) */

	if (!rena_plugins_engine_is_shutdown(rena_application_get_plugins_engine(priv->rena)))
	{
		/* Remove provider from gui. */

		if (priv->server) {
			provider = rena_database_provider_get ();
			rena_provider_remove (provider,
			                        priv->server);
			rena_provider_update_done (provider);
			g_object_unref (provider);
		}

		/* Remove settings */

		preferences = rena_application_get_preferences (priv->rena);
		plugin_group = rena_preferences_get_plugin_group_name (preferences, GROUP_KEY_AMPACHE);
		rena_preferences_remove_group (preferences, plugin_group);
		g_free (plugin_group);
	}

	backend = rena_application_get_backend (priv->rena);
	rena_backend_set_local_storage (backend, FALSE);
	g_signal_handlers_disconnect_by_func (backend, rena_ampache_plugin_prepare_source, plugin);

	/* Menu Action */

	rena_menubar_remove_plugin_action (priv->rena,
	                                     priv->action_group_main_menu,
	                                     priv->merge_id_main_menu);
	priv->merge_id_main_menu = 0;

	rena_menubar_remove_action (priv->rena, "rena-plugins-placeholder", "refresh-ampache");

	/* Close */

	rena_ampache_plugin_deauthenticate (plugin);

	/* Setting dialog widget */

	rena_ampache_plugin_remove_setting (plugin);
}
