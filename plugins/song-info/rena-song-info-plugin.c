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

#include <glyr/glyr.h>
#include <glyr/cache.h>

#include <glib/gstdio.h>

#include <libpeas/peas.h>
#include <libpeas-gtk/peas-gtk.h>

#include "plugins/rena-plugin-macros.h"

#include "rena-song-info-plugin.h"
#include "rena-song-info-dialog.h"
#include "rena-song-info-pane.h"
#include "rena-song-info-thread-albumart.h"
#include "rena-song-info-thread-dialog.h"
#include "rena-song-info-thread-pane.h"

#include "src/rena.h"
#include "src/rena-hig.h"
#include "src/rena-playlist.h"
#include "src/rena-playback.h"
#include "src/rena-sidebar.h"
#include "src/rena-simple-async.h"
#include "src/rena-simple-widgets.h"
#include "src/rena-preferences-dialog.h"
#include "src/rena-utils.h"

struct _RenaSongInfoPluginPrivate {
	RenaApplication  *rena;
	GtkWidget          *setting_widget;

	RenaSonginfoPane *pane;

	RenaInfoCache    *cache_info;

	gboolean            download_album_art;
	GtkWidget          *download_album_art_w;
	GtkWidget          *proxy_w;

	GtkActionGroup     *action_group_playlist;
	guint               merge_id_playlist;

	GCancellable       *pane_search;
};

RENA_PLUGIN_REGISTER_PRIVATE_CODE (RENA_TYPE_SONG_INFO_PLUGIN,
                                     RenaSongInfoPlugin,
                                     rena_song_info_plugin)

/*
 * Popups
 */

static void get_lyric_current_playlist_action       (GtkAction *action, RenaSongInfoPlugin *plugin);
static void get_artist_info_current_playlist_action (GtkAction *action, RenaSongInfoPlugin *plugin);

static const GtkActionEntry playlist_actions [] = {
	{"Search lyric", NULL, N_("Search _lyric"),
	 "", "Search lyric", G_CALLBACK(get_lyric_current_playlist_action)},
	{"Search artist info", NULL, N_("Search _artist info"),
	 "", "Search artist info", G_CALLBACK(get_artist_info_current_playlist_action)},
};

static const gchar *playlist_xml = "<ui>						\
	<popup name=\"SelectionPopup\">		   				\
	<menu action=\"ToolsMenu\">							\
		<placeholder name=\"rena-glyr-placeholder\">			\
			<menuitem action=\"Search lyric\"/>				\
			<menuitem action=\"Search artist info\"/>			\
			<separator/>							\
		</placeholder>								\
	</menu>										\
	</popup>				    						\
</ui>";

/*
 * Action on playlist that show a dialog
 */

static void
get_artist_info_current_playlist_action (GtkAction *action, RenaSongInfoPlugin *plugin)
{
	RenaPlaylist *playlist;
	RenaMusicobject *mobj;
	const gchar *artist = NULL;

	RenaApplication *rena = NULL;

	rena = plugin->priv->rena;
	playlist = rena_application_get_playlist (rena);

	mobj = rena_playlist_get_selected_musicobject (playlist);

	artist = rena_musicobject_get_artist (mobj);

	CDEBUG(DBG_INFO, "Get Artist info Action of current playlist selection");

	if (string_is_empty(artist))
		return;

	rena_songinfo_plugin_get_info_to_dialog (plugin, GLYR_GET_ARTISTBIO, artist, NULL);
}

static void
get_lyric_current_playlist_action (GtkAction *action, RenaSongInfoPlugin *plugin)
{
	RenaPlaylist *playlist;
	RenaMusicobject *mobj;
	const gchar *artist = NULL;
	const gchar *title = NULL;

	RenaApplication *rena = NULL;
	rena = plugin->priv->rena;

	playlist = rena_application_get_playlist (rena);
	mobj = rena_playlist_get_selected_musicobject (playlist);

	artist = rena_musicobject_get_artist (mobj);
	title = rena_musicobject_get_title (mobj);

	CDEBUG(DBG_INFO, "Get lyrics Action of current playlist selection.");

	if (string_is_empty(artist) || string_is_empty(title))
		return;

	rena_songinfo_plugin_get_info_to_dialog (plugin, GLYR_GET_LYRICS, artist, title);
}

/*
 * Handlers depending on backend status
 */

static void
related_get_album_art_handler (RenaSongInfoPlugin *plugin)
{
	RenaBackend *backend;
	RenaArtCache *art_cache;
	RenaMusicobject *mobj;
	const gchar *artist = NULL;
	const gchar *album = NULL;
	gchar *album_art_path = NULL;

	RenaSongInfoPluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_INFO, "Get album art handler");

	backend = rena_application_get_backend (priv->rena);
	if (rena_backend_get_state (backend) == ST_STOPPED)
		return;

	mobj = rena_backend_get_musicobject (backend);
	artist = rena_musicobject_get_artist (mobj);
	album = rena_musicobject_get_album (mobj);

	if (string_is_empty(artist) || string_is_empty(album))
		return;

	art_cache = rena_application_get_art_cache (priv->rena);
	album_art_path = rena_art_cache_get_album_uri (art_cache, artist, album);
	if (album_art_path)
		goto exists;

	rena_songinfo_plugin_get_album_art (plugin, artist, album);

exists:
	g_free (album_art_path);
}

static void
rena_song_info_cancel_pane_search (RenaSongInfoPlugin *plugin)
{
	RenaSongInfoPluginPrivate *priv = plugin->priv;

	if (priv->pane_search) {
		g_cancellable_cancel (priv->pane_search);
		g_object_unref (priv->pane_search);
		priv->pane_search = NULL;
	}
}

static void
related_get_song_info_pane_handler (RenaSongInfoPlugin *plugin)
{
	RenaBackend *backend;
	RenaMusicobject *mobj;
	GList *list = NULL, *l = NULL;
	const gchar *artist = NULL, *title = NULL, *filename = NULL;
	gchar *artist_bio = NULL, *lyrics = NULL, *provider = NULL;
	GLYR_GET_TYPE view_type = GLYR_GET_UNKNOWN;

	RenaSongInfoPluginPrivate *priv = plugin->priv;

	CDEBUG (DBG_INFO, "Get song info handler");

	backend = rena_application_get_backend (priv->rena);
	if (rena_backend_get_state (backend) == ST_STOPPED) {
		rena_songinfo_pane_clear_text (priv->pane);
		rena_songinfo_pane_clear_list (priv->pane);
		return;
	}

	mobj = rena_backend_get_musicobject (backend);
	artist = rena_musicobject_get_artist (mobj);
	title = rena_musicobject_get_title (mobj);
	filename = rena_musicobject_get_file (mobj);

	if (string_is_empty(artist) || string_is_empty(title))
		return;

	rena_song_info_cancel_pane_search (plugin);
	rena_songinfo_pane_clear_list (priv->pane);

	view_type = rena_songinfo_pane_get_default_view (priv->pane);
	switch (view_type) {
		case GLYR_GET_ARTIST_BIO:
			if (rena_info_cache_contains_artist_bio (priv->cache_info, artist))
			{
				artist_bio = rena_info_cache_get_artist_bio (priv->cache_info,
				                                               artist, &provider);

				rena_songinfo_pane_set_title (priv->pane, artist);
				rena_songinfo_pane_set_text (priv->pane, artist_bio, provider);
				g_free (artist_bio);
				g_free (provider);
				return;
			}
			break;
		case GLYR_GET_LYRICS:
			if (rena_info_cache_contains_song_lyrics (priv->cache_info, title, artist))
			{
				lyrics = rena_info_cache_get_song_lyrics (priv->cache_info,
				                                            title, artist,
				                                            &provider);
				rena_songinfo_pane_set_title (priv->pane, title);
				rena_songinfo_pane_set_text (priv->pane, lyrics, provider);
				g_free (lyrics);
				g_free (provider);
				return;
			}
			break;
		case GLYR_GET_SIMILAR_SONGS:
			if (rena_info_cache_contains_similar_songs (priv->cache_info, title, artist))
			{
				list = rena_info_cache_get_similar_songs (priv->cache_info,
				                                            title, artist,
				                                            &provider);
				for (l = list ; l != NULL ; l = l->next) {
					rena_songinfo_pane_append_song_row (priv->pane,
						rena_songinfo_pane_row_new ((RenaMusicobject *)l->data));
				}
				rena_songinfo_pane_set_title (priv->pane, title);
				rena_songinfo_pane_set_text (priv->pane, "", provider);
				g_list_free (list);
				g_free (provider);
				return;
			}
			break;
		default:
			break;
	}
	priv->pane_search = rena_songinfo_plugin_get_info_to_pane (plugin, view_type, artist, title, filename);
}

static void
rena_song_info_get_info (gpointer data)
{
	RenaSongInfoPlugin *plugin = data;
	RenaSongInfoPluginPrivate *priv = plugin->priv;

	if (priv->download_album_art)
		related_get_album_art_handler (plugin);

	if (!gtk_widget_is_visible(GTK_WIDGET(priv->pane)))
		return;

	related_get_song_info_pane_handler (plugin);
}

static void
backend_changed_state_cb (RenaBackend *backend, GParamSpec *pspec, gpointer user_data)
{
	RenaMusicSource file_source = FILE_NONE;
	RenaBackendState state = ST_STOPPED;

	RenaSongInfoPlugin *plugin = user_data;
	RenaSongInfoPluginPrivate *priv = plugin->priv;

	rena_song_info_cancel_pane_search (plugin);

	state = rena_backend_get_state (backend);

	CDEBUG(DBG_INFO, "Configuring thread to get the cover art");

	if (state == ST_STOPPED) {
		rena_songinfo_pane_clear_text (priv->pane);
		rena_songinfo_pane_clear_list (priv->pane);
	}

	if (state != ST_PLAYING)
		return;

	file_source = rena_musicobject_get_source (rena_backend_get_musicobject (backend));

	if (file_source == FILE_NONE) {
		rena_songinfo_pane_clear_text (priv->pane);
		rena_songinfo_pane_clear_list (priv->pane);
		return;
	}

	rena_song_info_get_info (plugin);
}

static void
rena_songinfo_pane_append (RenaSonginfoPane *pane,
                             RenaMusicobject *mobj,
                             RenaSongInfoPlugin *plugin)
{
	RenaPlaylist *playlist;
	const gchar *provider = NULL, *title = NULL, *artist = NULL;

	RenaSongInfoPluginPrivate *priv = plugin->priv;

	provider = rena_musicobject_get_provider (mobj);
	if (string_is_empty(provider))
	{
		open_url (rena_musicobject_get_file(mobj), NULL);
	}
	else
	{
		title = rena_musicobject_get_title (mobj);
		artist = rena_musicobject_get_artist (mobj);
		playlist = rena_application_get_playlist (priv->rena);
		if (!rena_playlist_already_has_title_of_artist (playlist, title, artist))
			rena_playlist_append_single_song (playlist, g_object_ref(mobj));
		rena_playlist_select_title_of_artist (playlist, title, artist);
	}
}

static void
rena_songinfo_pane_queue (RenaSonginfoPane *pane,
                            RenaMusicobject *mobj,
                            RenaSongInfoPlugin *plugin)
{
	RenaPlaylist *playlist;
	const gchar *provider = NULL, *title = NULL, *artist = NULL;

	RenaSongInfoPluginPrivate *priv = plugin->priv;

	provider = rena_musicobject_get_provider (mobj);
	if (string_is_empty(provider))
		return;

	title = rena_musicobject_get_title (mobj);
	artist = rena_musicobject_get_artist (mobj);
	playlist = rena_application_get_playlist (priv->rena);
	if (!rena_playlist_already_has_title_of_artist (playlist, title, artist))
		rena_playlist_append_single_song (playlist, g_object_ref(mobj));
	rena_playlist_select_title_of_artist (playlist, title, artist);
	rena_playlist_toggle_queue_selected (playlist);
}

static void
rena_songinfo_pane_append_all (RenaSonginfoPane   *pane,
                                 RenaSongInfoPlugin *plugin)
{
	RenaPlaylist *playlist;
	RenaMusicobject *mobj;
	const gchar *title = NULL, *artist = NULL;
	GList *mlist, *list;

	RenaSongInfoPluginPrivate *priv = plugin->priv;

	mlist = rena_songinfo_get_mobj_list (priv->pane);
	list = mlist;
	while (list != NULL) {
		mobj = RENA_MUSICOBJECT(list->data);
		title = rena_musicobject_get_title (mobj);
		artist = rena_musicobject_get_artist (mobj);
		playlist = rena_application_get_playlist (priv->rena);
		if (!rena_playlist_already_has_title_of_artist (playlist, title, artist))
			rena_playlist_append_single_song (playlist, g_object_ref(mobj));
		list = g_list_next(list);
	}
	g_list_free (mlist);
}

static gchar *
rena_songinfo_plugin_get_proxy (RenaPreferences *preferences)
{
	gchar *plugin_group = NULL, *string = NULL;
	plugin_group = rena_preferences_get_plugin_group_name (preferences, "song-info");

	string = rena_preferences_get_string (preferences,
	                                        plugin_group,
	                                        "Proxy");

	g_free (plugin_group);

	return string;
}

/*
 * Update handlers
 */

static void
rena_songinfo_pane_type_changed (RenaSonginfoPane *pane, RenaSongInfoPlugin *plugin)
{
	related_get_song_info_pane_handler (plugin);
}

static void
rena_songinfo_pane_visibility_changed (RenaPreferences *preferences, GParamSpec *pspec, RenaSongInfoPlugin *plugin)
{
	if (rena_preferences_get_secondary_lateral_panel (preferences))
		related_get_song_info_pane_handler (plugin);
}

/*
 * Public api
 */

RenaInfoCache *
rena_songinfo_plugin_get_cache_info (RenaSongInfoPlugin *plugin)
{
	RenaSongInfoPluginPrivate *priv = plugin->priv;

	return priv->cache_info;
}

RenaSonginfoPane *
rena_songinfo_plugin_get_pane (RenaSongInfoPlugin *plugin)
{
	RenaSongInfoPluginPrivate *priv = plugin->priv;

	return priv->pane;
}

void
rena_songinfo_plugin_init_glyr_query (gpointer data)
{
	RenaPreferences *preferences;
	gchar *proxy = NULL;
	GlyrQuery *query = data;

	preferences = rena_preferences_get ();
	proxy = rena_songinfo_plugin_get_proxy (preferences);

	glyr_query_init (query);
	glyr_opt_proxy (query, proxy);

	g_object_unref (preferences);
	g_free (proxy);
}

RenaApplication *
rena_songinfo_plugin_get_application (RenaSongInfoPlugin *plugin)
{
	RenaSongInfoPluginPrivate *priv = plugin->priv;

	return priv->rena;
}

/*
 * Preferences plugin
 */

static void
rena_songinfo_preferences_dialog_response (GtkDialog            *dialog,
                                             gint                  response_id,
                                             RenaSongInfoPlugin *plugin)
{
	RenaPreferences *preferences;
	const gchar *entry_proxy = NULL;
	gchar *plugin_group = NULL, *test_proxy = NULL;

	RenaSongInfoPluginPrivate *priv = plugin->priv;

	preferences = rena_preferences_get ();
	plugin_group = rena_preferences_get_plugin_group_name (preferences, "song-info");

	test_proxy = rena_songinfo_plugin_get_proxy (preferences);

	switch(response_id) {
		case GTK_RESPONSE_CANCEL:
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(priv->download_album_art_w),
			                              priv->download_album_art);
			rena_gtk_entry_set_text (GTK_ENTRY(priv->proxy_w), test_proxy);
			break;
		case GTK_RESPONSE_OK:
			priv->download_album_art =
				gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->download_album_art_w));

			rena_preferences_set_boolean (preferences,
			                                plugin_group, "DownloadAlbumArt",
			                                priv->download_album_art);

			entry_proxy = gtk_entry_get_text (GTK_ENTRY(priv->proxy_w));

			if (g_strcmp0 (test_proxy, entry_proxy))
				rena_preferences_set_string (preferences, plugin_group, "Proxy", entry_proxy);

			break;
		default:
			break;
	}

	g_object_unref (preferences);
	g_free (plugin_group);
	g_free (test_proxy);
}

static void
rena_songinfo_plugin_append_setting (RenaSongInfoPlugin *plugin)
{
	RenaPreferencesDialog *dialog;
	RenaPreferences *preferences = NULL;
	gchar *plugin_group = NULL, *proxy = NULL;
	GtkWidget *table, *download_album_art_w, *proxy_label, *proxy_w;
	guint row = 0;

	RenaSongInfoPluginPrivate *priv = plugin->priv;

	table = rena_hig_workarea_table_new ();

	rena_hig_workarea_table_add_section_title(table, &row, _("Song Information"));

	download_album_art_w = gtk_check_button_new_with_label (_("Download the album art while playing their songs."));
	rena_hig_workarea_table_add_wide_control (table, &row, download_album_art_w);

	preferences = rena_preferences_get ();
	proxy_label = gtk_label_new (_("Proxy"));
	proxy_w = gtk_entry_new ();
	proxy = rena_songinfo_plugin_get_proxy (preferences);
	if (proxy)
		gtk_entry_set_text (GTK_ENTRY(proxy_w), proxy);
	gtk_entry_set_placeholder_text (GTK_ENTRY(proxy_w), "[protocol://][user:pass@]yourproxy.domain[:port]");
	gtk_entry_set_activates_default (GTK_ENTRY(proxy_w), TRUE);

	rena_hig_workarea_table_add_row (table, &row, proxy_label, proxy_w);

	plugin_group = rena_preferences_get_plugin_group_name(preferences, "song-info");

	priv->download_album_art =
		rena_preferences_get_boolean (preferences, plugin_group, "DownloadAlbumArt");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(download_album_art_w),
	                              priv->download_album_art);

	priv->setting_widget = table;
	priv->download_album_art_w = download_album_art_w;
	priv->proxy_w = proxy_w;

	dialog = rena_application_get_preferences_dialog (priv->rena);
	rena_preferences_append_services_setting (dialog, table, FALSE);

	rena_preferences_dialog_connect_handler (dialog,
	                                           G_CALLBACK(rena_songinfo_preferences_dialog_response),
	                                           plugin);

	g_object_unref (G_OBJECT (preferences));
	g_free (plugin_group);
	g_free (proxy);
}

static void
rena_songinfo_plugin_remove_setting (RenaSongInfoPlugin *plugin)
{
	RenaPreferencesDialog *dialog;
	RenaSongInfoPluginPrivate *priv = plugin->priv;

	dialog = rena_application_get_preferences_dialog (priv->rena);

	rena_preferences_dialog_disconnect_handler (dialog,
	                                              G_CALLBACK(rena_songinfo_preferences_dialog_response),
	                                              plugin);

	rena_preferences_remove_services_setting (dialog, priv->setting_widget);
}

/*
 * Plugin
 */
static void
rena_plugin_activate (PeasActivatable *activatable)
{
	RenaPreferences *preferences;
	RenaPlaylist *playlist;
	RenaSidebar *sidebar;
	GLYR_GET_TYPE view_type = GLYR_GET_LYRICS;
	gchar *plugin_group = NULL;

	RenaSongInfoPlugin *plugin = RENA_SONG_INFO_PLUGIN (activatable);
	RenaSongInfoPluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "Song-info plugin %s", G_STRFUNC);

	priv->rena = g_object_get_data (G_OBJECT (plugin), "object");

	glyr_init ();

	priv->cache_info = rena_info_cache_get();

	/* Attach Playlist popup menu*/

	priv->action_group_playlist = rena_menubar_plugin_action_new ("RenaGlyrPlaylistActions",
	                                                                playlist_actions,
	                                                                G_N_ELEMENTS (playlist_actions),
	                                                                NULL,
	                                                                0,
	                                                                plugin);

	playlist = rena_application_get_playlist (priv->rena);
	priv->merge_id_playlist = rena_playlist_append_plugin_action (playlist,
	                                                                priv->action_group_playlist,
	                                                                playlist_xml);

	/* Create the pane and attach it */
	priv->pane = rena_songinfo_pane_new ();
	sidebar = rena_application_get_second_sidebar (priv->rena);
	rena_sidebar_attach_plugin (sidebar,
		                          GTK_WIDGET (priv->pane),
		                          rena_songinfo_pane_get_pane_title (priv->pane),
		                          rena_songinfo_pane_get_popover (priv->pane));

	/* Connect signals */

	g_signal_connect (rena_application_get_backend (priv->rena), "notify::state",
	                  G_CALLBACK (backend_changed_state_cb), plugin);
	backend_changed_state_cb (rena_application_get_backend (priv->rena), NULL, plugin);

	preferences = rena_application_get_preferences (priv->rena);

	plugin_group = rena_preferences_get_plugin_group_name (preferences, "song-info");
	view_type = rena_preferences_get_integer (preferences, plugin_group, "default-view");
	rena_songinfo_pane_set_default_view (priv->pane, view_type);
	g_free (plugin_group);

	g_signal_connect (G_OBJECT(preferences), "notify::secondary-lateral-panel",
	                  G_CALLBACK(rena_songinfo_pane_visibility_changed), plugin);

	g_signal_connect (G_OBJECT(priv->pane), "type-changed",
	                  G_CALLBACK(rena_songinfo_pane_type_changed), plugin);
	g_signal_connect (G_OBJECT(priv->pane), "append",
	                  G_CALLBACK(rena_songinfo_pane_append), plugin);
	g_signal_connect (G_OBJECT(priv->pane), "append-all",
	                  G_CALLBACK(rena_songinfo_pane_append_all), plugin);
	g_signal_connect (G_OBJECT(priv->pane), "queue",
	                  G_CALLBACK(rena_songinfo_pane_queue), plugin);

	/* Default values */

	rena_songinfo_plugin_append_setting (plugin);
}

static void
rena_plugin_deactivate (PeasActivatable *activatable)
{
	RenaApplication *rena = NULL;
	RenaPreferences *preferences;
	RenaPlaylist *playlist;
	RenaSidebar *sidebar;
	gchar *plugin_group = NULL;

	RenaSongInfoPlugin *plugin = RENA_SONG_INFO_PLUGIN (activatable);
	RenaSongInfoPluginPrivate *priv = plugin->priv;

	rena = plugin->priv->rena;

	CDEBUG(DBG_PLUGIN, "SongInfo plugin %s", G_STRFUNC);

	g_signal_handlers_disconnect_by_func (rena_application_get_backend (rena),
	                                      backend_changed_state_cb, plugin);

	playlist = rena_application_get_playlist (rena);
	rena_playlist_remove_plugin_action (playlist,
	                                      priv->action_group_playlist,
	                                      priv->merge_id_playlist);

	priv->merge_id_playlist = 0;

	preferences = rena_application_get_preferences (rena);

	g_signal_handlers_disconnect_by_func (G_OBJECT(preferences),
	                                      rena_songinfo_pane_visibility_changed,
	                                      plugin);

	g_signal_handlers_disconnect_by_func (G_OBJECT(preferences),
	                                      rena_songinfo_pane_type_changed,
	                                      plugin);

	plugin_group = rena_preferences_get_plugin_group_name (preferences, "song-info");
	rena_preferences_set_integer (preferences, plugin_group, "default-view",
			rena_songinfo_pane_get_default_view (priv->pane));

	if (!rena_plugins_engine_is_shutdown(rena_application_get_plugins_engine(priv->rena))) {
		rena_preferences_remove_group (preferences, plugin_group);
	}
	g_free (plugin_group);

	sidebar = rena_application_get_second_sidebar (priv->rena);
	rena_sidebar_remove_plugin (sidebar, GTK_WIDGET(priv->pane));

	rena_songinfo_plugin_remove_setting (plugin);

	g_object_unref (priv->cache_info);

	glyr_cleanup ();

	priv->rena = NULL;
}
