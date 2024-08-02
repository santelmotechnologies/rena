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

#include "rena-subsonic-api.h"
#include "rena-subsonic-preferences.h"

#include "rena-subsonic-plugin.h"

#include "src/rena.h"
#include "src/rena-app-notification.h"
#include "src/rena-backend.h"
#include "src/rena-background-task-bar.h"
#include "src/rena-background-task-widget.h"
#include "src/rena-database-provider.h"
#include "src/rena-menubar.h"
#include "src/rena-music-enum.h"
#include "src/rena-musicobject.h"
#include "src/rena-song-cache.h"
#include "src/rena-temp-provider.h"
#include "src/rena-utils.h"
#include "src/rena-window.h"

#include "plugins/rena-plugin-macros.h"


typedef struct _RenaSubsonicPluginPrivate RenaSubsonicPluginPrivate;

struct _RenaSubsonicPluginPrivate {
	RenaApplication          *rena;

	RenaSubsonicApi          *subsonic;

	RenaSubsonicPreferences  *preferences;

	gchar                      *server;

	RenaBackgroundTaskWidget *task_widget;

	GtkActionGroup             *action_group_main_menu;
	guint                       merge_id_main_menu;
};

RENA_PLUGIN_REGISTER (RENA_TYPE_SUBSONIC_PLUGIN,
                        RenaSubsonicPlugin,
                        rena_subsonic_plugin)


/*
 * Menu actions
 */

static void
rena_subsonic_plugin_upgrade_database_action (GtkAction            *action,
                                                RenaSubsonicPlugin *plugin)
{
	RenaBackgroundTaskBar *taskbar;

	RenaSubsonicPluginPrivate *priv = plugin->priv;

	if (rena_subsonic_api_is_scanning(priv->subsonic))
		return;

	taskbar = rena_background_task_bar_get ();
	rena_background_task_widget_set_description (priv->task_widget, _("Getting albums from the server"));
	rena_background_task_bar_prepend_widget (taskbar, GTK_WIDGET(priv->task_widget));
	g_object_unref(G_OBJECT(taskbar));

	rena_subsonic_api_scan_server (priv->subsonic);
}

static void
rena_subsonic_plugin_upgrade_database_gmenu_action (GSimpleAction *action,
                                                      GVariant      *parameter,
                                                      gpointer       user_data)
{
	RenaBackgroundTaskBar *taskbar;

	RenaSubsonicPlugin *plugin = user_data;
	RenaSubsonicPluginPrivate *priv = plugin->priv;

	if (rena_subsonic_api_is_scanning(priv->subsonic))
		return;

	taskbar = rena_background_task_bar_get ();
	rena_background_task_widget_set_description (priv->task_widget, _("Getting albums from the server"));
	rena_background_task_bar_prepend_widget (taskbar, GTK_WIDGET(priv->task_widget));
	g_object_unref(G_OBJECT(taskbar));

	rena_subsonic_api_scan_server (priv->subsonic);
}

static const GtkActionEntry main_menu_actions [] = {
	{"Refresh the Subsonic library", NULL, N_("Refresh the Subsonic library"),
	 "", "Refresh the Subsonic library", G_CALLBACK(rena_subsonic_plugin_upgrade_database_action)}};

static const gchar *main_menu_xml = "<ui>								\
	<menubar name=\"Menubar\">											\
		<menu action=\"ToolsMenu\">										\
			<placeholder name=\"rena-plugins-placeholder\">			\
				<menuitem action=\"Refresh the Subsonic library\"/>		\
				<separator/>											\
			</placeholder>												\
		</menu>															\
	</menubar>															\
</ui>";


/*
 * Api responces
 */

static void
rena_subsonic_plugin_authenticated (RenaSubsonicApi    *subsonic,
                                      SubsonicStatusCode    code,
                                      RenaSubsonicPlugin *plugin)
{
	RenaBackgroundTaskBar *taskbar;
	RenaAppNotification *notification;
	RenaDatabaseProvider *provider;

	RenaSubsonicPluginPrivate *priv = plugin->priv;

	if (code != S_GENERIC_OK) {
		notification = rena_app_notification_new (string_is_empty(priv->server) ? priv->server : _("Subsonic"),
		                                            _("There was an error authenticating with the server"));
		rena_app_notification_show (notification);
		return;
	}

	/* Add a widget to see progress */

	priv->task_widget = rena_background_task_widget_new (_("Searching files to analyze"),
	                                                       "network-server",
	                                                       0,
	                                                       rena_subsonic_get_cancellable(priv->subsonic));
	g_object_ref(G_OBJECT(priv->task_widget));

	/* If the provider exists, we should not analyze it again. */

	provider = rena_database_provider_get ();
	if (rena_provider_exist (provider, priv->server)) {
		g_object_unref(provider);
		return;
	}

	taskbar = rena_background_task_bar_get ();
	rena_background_task_widget_set_description (priv->task_widget, _("Getting albums from the server"));
	rena_background_task_bar_prepend_widget (taskbar, GTK_WIDGET(priv->task_widget));
	g_object_unref(G_OBJECT(taskbar));

	/* Search songs in the provider. */

	rena_subsonic_api_scan_server (priv->subsonic);
}

static void
rena_subsonic_plugin_pinged (RenaSubsonicApi    *subsonic,
                               SubsonicStatusCode    code,
                               RenaSubsonicPlugin *plugin)
{
	RenaSubsonicPluginPrivate *priv = plugin->priv;

	if (code == S_GENERIC_OK) {
		g_warning ("Has connection with Subsonic server...");
	}
	else {
		g_warning ("Wrom connection with Subsonic server. Errro code: %i", code);
	}
}

static void
rena_subsonic_plugin_scan_total_albums (RenaSubsonicApi    *subsonic,
                                          guint                 total,
                                          RenaSubsonicPlugin *plugin)
{
	RenaSubsonicPluginPrivate *priv = plugin->priv;
	rena_background_task_widget_set_description (priv->task_widget, _("Getting the songs from the albums"));
	rena_background_task_widget_set_job_count (priv->task_widget, total);
}

static void
rena_subsonic_plugin_scan_progress_albums (RenaSubsonicApi    *subsonic,
                                             guint                 progress,
                                             RenaSubsonicPlugin *plugin)
{
	RenaSubsonicPluginPrivate *priv = plugin->priv;
	rena_background_task_widget_set_job_progress (priv->task_widget, progress);
}

static void
rena_subsonic_plugin_scan_finished (RenaSubsonicApi    *subsonic,
                                      SubsonicStatusCode    code,
                                      RenaSubsonicPlugin *plugin)
{
	RenaTempProvider *provider;
	RenaAppNotification *notification;
	RenaBackgroundTaskBar *taskbar;
	RenaPlaylist *playlist;
	GSList *songs_list, *l;

	RenaSubsonicPluginPrivate *priv = plugin->priv;

	/* Remove the progress widget */

	taskbar = rena_background_task_bar_get ();
	rena_background_task_bar_remove_widget (taskbar, GTK_WIDGET(priv->task_widget));
	g_object_unref(G_OBJECT(taskbar));

	g_clear_object(&priv->task_widget);

	/* If finish correctly, we must save the collection. */

	if (code == S_GENERIC_OK) {
		provider = rena_temp_provider_new (priv->server,
		                                     "SUBSONIC",
		                                     priv->server,
		                                     "folder-remote");

		songs_list = rena_subsonic_api_get_songs_list (priv->subsonic);
		for (l = songs_list ; l != NULL ; l = l->next) {
			rena_temp_provider_insert_track (provider, RENA_MUSICOBJECT(l->data));
		}
		rena_temp_provider_merge_database (provider);
		rena_temp_provider_commit_database (provider);
		rena_temp_provider_set_visible (provider, TRUE);
		g_object_unref (provider);
	}
	else if (code != S_USER_CANCELLED) {
		/* But if the user did not cancel, there was an error */
		notification = rena_app_notification_new (priv->server,
			_("There was an error searching the collection."));
		rena_app_notification_show (notification);
	}

}


/*
 * Authentication.
 */

static void
rena_subsonic_plugin_authenticate (RenaSubsonicPlugin *plugin)
{
	const gchar *server = NULL, *username = NULL, *password = NULL;

	RenaSubsonicPluginPrivate *priv = plugin->priv;

	/* Get settings */

	server = rena_subsonic_preferences_get_server_text(priv->preferences);
	username = rena_subsonic_preferences_get_username_text(priv->preferences);
	password = rena_subsonic_preferences_get_password_text(priv->preferences);

	if (string_is_empty (server))
		return;
	if (string_is_empty (username))
		return;
	if (string_is_empty (password))
		return;

	priv->server = g_strdup (server);

	rena_subsonic_api_authentication (priv->subsonic,
	                                    server, username, password);
}


/*
 * RenaSubsonicPreferences signals
 */

static void
rena_subsonic_preferences_server_changed (RenaSubsonicPreferences *preferences,
                                            RenaSubsonicPlugin      *plugin)
{
	RenaDatabaseProvider *provider;

	RenaSubsonicPluginPrivate *priv = plugin->priv;

	rena_subsonic_api_deauthentication(priv->subsonic);

	if (string_is_not_empty(priv->server)) {
		provider = rena_database_provider_get ();
		rena_provider_remove (provider,
		                        priv->server);
		rena_provider_update_done (provider);
		g_object_unref (provider);

		g_free (priv->server);
		priv->server = NULL;
	}
}

static void
rena_subsonic_preferences_credentials_changed (RenaSubsonicPreferences *preferences,
                                                 RenaSubsonicPlugin      *plugin)
{
	RenaSubsonicPluginPrivate *priv = plugin->priv;

	rena_subsonic_api_deauthentication (priv->subsonic);

	rena_subsonic_plugin_authenticate (plugin);
}


/*
 * Gstreamer.source.
 */

static gboolean
rena_musicobject_is_subsonic_file (RenaMusicobject *mobj)
{
	RenaMusicEnum *enum_map = NULL;
	RenaMusicSource file_source = FILE_NONE;

	enum_map = rena_music_enum_get ();
	file_source = rena_music_enum_map_get(enum_map, "SUBSONIC");
	g_object_unref (enum_map);

	return (file_source == rena_musicobject_get_source (mobj));
}

static void
rena_subsonic_plugin_prepare_source (RenaBackend        *backend,
                                       RenaSubsonicPlugin *plugin)
{
	RenaSongCache *cache;
	RenaMusicobject *mobj;
	const gchar *location = NULL;
	gchar *filename = NULL, *song_id = NULL, *uri = NULL;

	RenaSubsonicPluginPrivate *priv = plugin->priv;

	mobj = rena_backend_get_musicobject (backend);
	if (!rena_musicobject_is_subsonic_file (mobj))
		return;

	location =  rena_musicobject_get_file (mobj);

	cache = rena_song_cache_get ();
	filename = rena_song_cache_get_from_location (cache, location);
	g_object_unref(cache);

	if (filename != NULL) {
		uri = g_filename_to_uri (filename, NULL, NULL);
		g_free (filename);
	}
	else {
		uri = rena_subsonic_api_get_playback_url (priv->subsonic, location);
	}

	rena_backend_set_playback_uri (backend, uri);
	g_free (uri);
}

static void
rena_subsonic_plugin_download_done (RenaBackend        *backend,
                                      gchar                *filename,
                                      RenaSubsonicPlugin *plugin)
{
	RenaSongCache *cache;
	RenaMusicobject *mobj;
	const gchar *location = NULL;

	RenaSubsonicPluginPrivate *priv = plugin->priv;

	mobj = rena_backend_get_musicobject (backend);
	if (!rena_musicobject_is_subsonic_file (mobj))
		return;

	location = rena_musicobject_get_file (mobj);

	cache = rena_song_cache_get ();
	rena_song_cache_put_location (cache, location, filename);
	g_object_unref(cache);
}


/*
 * Plugin.
 */

static void
rena_plugin_activate (PeasActivatable *activatable)
{
	RenaBackend *backend;
	GtkWidget *settings_widget;
	GMenuItem *item;
	GSimpleAction *action;

	RenaSubsonicPlugin *plugin = RENA_SUBSONIC_PLUGIN (activatable);

	RenaSubsonicPluginPrivate *priv = plugin->priv;
	priv->rena = g_object_get_data (G_OBJECT (plugin), "object");

	CDEBUG(DBG_PLUGIN, "Subsonic Server plugin %s", G_STRFUNC);

	/* New subsonic client api */

	priv->subsonic = rena_subsonic_api_new();
	g_signal_connect (priv->subsonic, "authenticated",
	                  G_CALLBACK (rena_subsonic_plugin_authenticated), plugin);
	g_signal_connect (priv->subsonic, "pong",
	                  G_CALLBACK (rena_subsonic_plugin_pinged), plugin);
	g_signal_connect (priv->subsonic, "scan-total",
	                  G_CALLBACK (rena_subsonic_plugin_scan_total_albums), plugin);
	g_signal_connect (priv->subsonic, "scan-progress",
	                  G_CALLBACK (rena_subsonic_plugin_scan_progress_albums), plugin);
	g_signal_connect (priv->subsonic, "scan-finished",
	                  G_CALLBACK (rena_subsonic_plugin_scan_finished), plugin);

	/* Settings */

	priv->preferences = rena_subsonic_preferences_new ();
	g_signal_connect (priv->preferences, "server-changed",
	                  G_CALLBACK (rena_subsonic_preferences_server_changed), plugin);
	g_signal_connect (priv->preferences, "credentials-changed",
	                  G_CALLBACK (rena_subsonic_preferences_credentials_changed), plugin);

	/* Attach main menu */

	priv->action_group_main_menu = rena_menubar_plugin_action_new ("RenaSubsonicPlugin",
	                                                                 main_menu_actions,
	                                                                 G_N_ELEMENTS (main_menu_actions),
	                                                                 NULL,
	                                                                 0,
	                                                                 plugin);

	priv->merge_id_main_menu = rena_menubar_append_plugin_action (priv->rena,
	                                                                priv->action_group_main_menu,
	                                                                main_menu_xml);

	/* Attach gear menu */

	action = g_simple_action_new ("refresh-subsonic", NULL);
	g_signal_connect (G_OBJECT (action), "activate",
	                  G_CALLBACK (rena_subsonic_plugin_upgrade_database_gmenu_action), plugin);

	item = g_menu_item_new (_("Refresh the Subsonic library"), "win.refresh-subsonic");
	rena_menubar_append_action (priv->rena, "rena-plugins-placeholder", action, item);
	g_object_unref (item);

	/* Backend signals */

	backend = rena_application_get_backend (priv->rena);
	rena_backend_set_local_storage (backend, TRUE);
	g_signal_connect (backend, "prepare-source",
	                  G_CALLBACK(rena_subsonic_plugin_prepare_source), plugin);
	g_signal_connect (backend, "download-done",
	                  G_CALLBACK(rena_subsonic_plugin_download_done), plugin);

	/* Authenticate */

	rena_subsonic_plugin_authenticate (plugin);
}

static void
rena_plugin_deactivate (PeasActivatable *activatable)
{
	RenaBackend *backend;
	RenaBackgroundTaskBar *taskbar;
	RenaDatabaseProvider *provider;
	RenaPreferences *preferences;
	GtkWidget *settings_widget;

	RenaSubsonicPlugin *plugin = RENA_SUBSONIC_PLUGIN (activatable);
	RenaSubsonicPluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "Subsonic Server plugin %s", G_STRFUNC);

	/* Subsonic client */

	g_object_unref (priv->subsonic);

	if (priv->task_widget) {
		/* Remove background task widget. */
		taskbar = rena_background_task_bar_get ();
		rena_background_task_bar_remove_widget (taskbar, GTK_WIDGET(priv->task_widget));
		g_object_unref(G_OBJECT(taskbar));

		/* Drop task widget */
		g_object_unref (G_OBJECT(priv->task_widget));
	}


	/* Remove menu actions */

	rena_menubar_remove_plugin_action (priv->rena,
	                                     priv->action_group_main_menu,
	                                     priv->merge_id_main_menu);
	rena_menubar_remove_action (priv->rena,
	                              "rena-plugins-placeholder",
	                              "refresh-subsonic");

	/* If user disable the plugin (Rena not shutdown) */

	if (!rena_plugins_engine_is_shutdown(rena_application_get_plugins_engine(priv->rena)))
	{
		rena_subsonic_preferences_forget_settings (priv->preferences);
		if (string_is_not_empty(priv->server)) {
			provider = rena_database_provider_get ();
			rena_provider_remove (provider,
			                        priv->server);
			rena_provider_update_done (provider);
			g_object_unref (provider);
		}
	}

	/* Remove settings */

	g_object_unref (priv->preferences);

	/* Clean memory */

	if (priv->server) {
		g_free (priv->server);
		priv->server = NULL;
	}
}
