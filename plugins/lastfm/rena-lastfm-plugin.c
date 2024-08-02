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

#include <libpeas/peas.h>

#include "src/rena.h"
#include "src/rena-app-notification.h"
#include "src/rena-hig.h"
#include "src/rena-utils.h"
#include "src/rena-menubar.h"
#include "src/rena-musicobject.h"
#include "src/rena-favorites.h"
#include "src/rena-musicobject-mgmt.h"
#include "src/rena-plugins-engine.h"
#include "src/rena-statusicon.h"
#include "src/rena-tagger.h"
#include "src/rena-simple-async.h"
#include "src/rena-utils.h"
#include "src/rena-tags-dialog.h"
#include "src/rena-tags-mgmt.h"
#include "src/rena-window.h"
#include "src/xml_helper.h"

#include "rena-lastfm-menu-ui.h"

#include "plugins/rena-plugin-macros.h"

#include <clastfm.h>

#define RENA_TYPE_LASTFM_PLUGIN         (rena_lastfm_plugin_get_type ())
#define RENA_LASTFM_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), RENA_TYPE_LASTFM_PLUGIN, RenaLastfmPlugin))
#define RENA_LASTFM_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), RENA_TYPE_LASTFM_PLUGIN, RenaLastfmPlugin))
#define RENA_IS_LASTFM_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), RENA_TYPE_LASTFM_PLUGIN))
#define RENA_IS_LASTFM_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), RENA_TYPE_LASTFM_PLUGIN))
#define RENA_LASTFM_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), RENA_TYPE_LASTFM_PLUGIN, RenaLastfmPluginClass))

struct _RenaLastfmPluginPrivate {
	RenaApplication        *rena;

	RenaFavorites          *favorites;

	/* Last session status. */
	LASTFM_SESSION           *session_id;
	enum LASTFM_STATUS_CODES  status;
	gboolean                  has_user;
	gboolean                  has_pass;

	/* Settings widgets */
	GtkWidget                *setting_widget;
	GtkWidget                *enable_w;
	GtkWidget                *lastfm_uname_w;
	GtkWidget                *lastfm_pass_w;

	GtkWidget                *ntag_lastfm_button;

	/* Song status */
	GMutex                    data_mutex;
	time_t                    playback_started;
	RenaMusicobject        *current_mobj;
	RenaMusicobject        *updated_mobj;

	/* Menu options */
	GtkActionGroup           *action_group_main_menu;
	guint                     merge_id_main_menu;

	GtkActionGroup           *action_group_playlist;
	guint                     merge_id_playlist;

	guint                     update_timeout_id;
	guint                     scrobble_timeout_id;
};
typedef struct _RenaLastfmPluginPrivate RenaLastfmPluginPrivate;

RENA_PLUGIN_REGISTER (RENA_TYPE_LASTFM_PLUGIN,
                        RenaLastfmPlugin,
                        rena_lastfm_plugin)

/*
 * Some useful definitions
 */
#define LASTFM_API_KEY "ecdc2d21dbfe1139b1f0da35daca9309"
#define LASTFM_SECRET  "f3498ce387f30eeae8ea1b1023afb32b"

#define KEY_LASTFM_SCROBBLE "scrobble"
#define KEY_LASTFM_USER     "lastfm_user"
#define KEY_LASTFM_PASS     "lastfm_pass"

#define WAIT_UPDATE 5

typedef enum {
	LASTFM_NONE = 0,
	LASTFM_GET_SIMILAR,
	LASTFM_GET_LOVED
} LastfmQueryType;

/*
 * Some structs to handle threads.
 */

typedef struct {
	GList              *list;
	LastfmQueryType     query_type;
	guint               query_count;
	RenaLastfmPlugin *plugin;
} AddMusicObjectListData;

typedef struct {
	RenaLastfmPlugin *plugin;
	RenaMusicobject  *mobj;
} RenaLastfmAsyncData;

static RenaLastfmAsyncData *
rena_lastfm_async_data_new (RenaLastfmPlugin *plugin, RenaMusicobject *mobj)
{
	RenaBackend *backend;
	RenaLastfmAsyncData *data;

	RenaLastfmPluginPrivate *priv = plugin->priv;

	data = g_slice_new (RenaLastfmAsyncData);
	data->plugin = plugin;
	if (mobj)  {
		backend = rena_application_get_backend (priv->rena);
		data->mobj = rena_musicobject_dup (rena_backend_get_musicobject (backend));
	}
	else
		data->mobj = rena_musicobject_dup (mobj);

	return data;
}

static void
rena_lastfm_async_data_free (RenaLastfmAsyncData *data)
{
	g_object_unref (data->mobj);
	g_slice_free (RenaLastfmAsyncData, data);
}

/*
 * Menubar Prototypes
 */

static void lastfm_add_favorites_action                 (GtkAction *action, RenaLastfmPlugin *plugin);
static void lastfm_get_similar_action                   (GtkAction *action, RenaLastfmPlugin *plugin);
static void lastfm_import_xspf_action                   (GtkAction *action, RenaLastfmPlugin *plugin);

static const GtkActionEntry main_menu_actions [] = {
	{"Lastfm", NULL, N_("_Lastfm")},
	{"Import a XSPF playlist", NULL, N_("Import a XSPF playlist"),
	 "", "Import a XSPF playlist", G_CALLBACK(lastfm_import_xspf_action)},
	{"Add favorites", NULL, N_("Add favorites"),
	 "", "Add favorites", G_CALLBACK(lastfm_add_favorites_action)},
	{"Add similar", NULL, N_("Add similar"),
	 "", "Add similar", G_CALLBACK(lastfm_get_similar_action)},
};

static const gchar *main_menu_xml = "<ui>						\
	<menubar name=\"Menubar\">							\
		<menu action=\"ToolsMenu\">						\
			<placeholder name=\"rena-plugins-placeholder\">		\
				<menu action=\"Lastfm\">				\
					<menuitem action=\"Import a XSPF playlist\"/>	\
					<menuitem action=\"Add favorites\"/>		\
					<menuitem action=\"Add similar\"/>		\
				</menu>							\
				<separator/>						\
			</placeholder>							\
		</menu>									\
	</menubar>									\
</ui>";


/*
 * Playlist Prototypes.
 */

static void lastfm_get_similar_current_playlist_action  (GtkAction *action, RenaLastfmPlugin *plugin);

static const GtkActionEntry playlist_actions [] = {
	{"Add similar", NULL, N_("Add similar"),
	 "", "Add similar", G_CALLBACK(lastfm_get_similar_current_playlist_action)},
};

static const gchar *playlist_xml = "<ui>						\
	<popup name=\"SelectionPopup\">		   					\
	<menu action=\"ToolsMenu\">							\
		<placeholder name=\"rena-plugins-placeholder\">			\
			<menuitem action=\"Add similar\"/>				\
			<separator/>							\
		</placeholder>								\
	</menu>										\
	</popup>				    					\
</ui>";

/*
 * Gear Menu Prototypes
 */
static void rena_gmenu_lastfm_add_favorites_action    (GSimpleAction *action,
                                                         GVariant      *parameter,
                                                         gpointer       user_data);
static void rena_gmenu_lastfm_get_similar_action      (GSimpleAction *action,
                                                         GVariant      *parameter,
                                                         gpointer       user_data);
static void rena_gmenu_lastfm_import_xspf_action      (GSimpleAction *action,
                                                         GVariant      *parameter,
                                                         gpointer       user_data);

static GActionEntry lastfm_entries[] = {
	{ "lastfm-import",     rena_gmenu_lastfm_import_xspf_action,   NULL, NULL, NULL },
	{ "lastfm-favorities", rena_gmenu_lastfm_add_favorites_action, NULL, NULL, NULL },
	{ "lastfm-similar",    rena_gmenu_lastfm_get_similar_action,   NULL, NULL, NULL }
};

/* Save a get the lastfm password.
 * TODO: Implement any basic crypto.
 */

static void
rena_lastfm_plugin_set_password (RenaPreferences *preferences, const gchar *pass)
{
	gchar *plugin_group = NULL;
	plugin_group = rena_preferences_get_plugin_group_name (preferences, "lastfm");

	if (string_is_not_empty(pass))
		rena_preferences_set_string (preferences,
		                               plugin_group,
		                               KEY_LASTFM_PASS,
		                               pass);
	else
 		rena_preferences_remove_key (preferences,
		                               plugin_group,
		                               KEY_LASTFM_PASS);

	g_free (plugin_group);
}

static gchar *
rena_lastfm_plugin_get_password (RenaPreferences *preferences)
{
	gchar *plugin_group = NULL, *string = NULL;
	plugin_group = rena_preferences_get_plugin_group_name (preferences, "lastfm");

	string = rena_preferences_get_string (preferences,
	                                        plugin_group,
	                                        KEY_LASTFM_PASS);

	g_free (plugin_group);

	return string;
}

static void
rena_lastfm_plugin_set_user (RenaPreferences *preferences, const gchar *user)
{
	gchar *plugin_group = NULL;
	plugin_group = rena_preferences_get_plugin_group_name (preferences, "lastfm");

	if (string_is_not_empty(user))
		rena_preferences_set_string (preferences,
		                               plugin_group,
		                               KEY_LASTFM_USER,
		                               user);
	else
 		rena_preferences_remove_key (preferences,
		                               plugin_group,
		                               KEY_LASTFM_USER);

	g_free (plugin_group);
}

static gchar *
rena_lastfm_plugin_get_user (RenaPreferences *preferences)
{
	gchar *plugin_group = NULL, *string = NULL;
	plugin_group = rena_preferences_get_plugin_group_name (preferences, "lastfm");

	string = rena_preferences_get_string (preferences,
	                                        plugin_group,
	                                        KEY_LASTFM_USER);

	g_free (plugin_group);

	return string;
}

static void
rena_lastfm_plugin_set_scrobble_support (RenaPreferences *preferences, gboolean supported)
{
	gchar *plugin_group = NULL;
	plugin_group = rena_preferences_get_plugin_group_name (preferences, "lastfm");

	rena_preferences_set_boolean (preferences,
		                            plugin_group,
		                            KEY_LASTFM_SCROBBLE,
		                            supported);

	g_free (plugin_group);
}

static gboolean
rena_lastfm_plugin_get_scrobble_support (RenaPreferences *preferences)
{
	gchar *plugin_group = NULL;
	gboolean scrobble = FALSE;

	plugin_group = rena_preferences_get_plugin_group_name (preferences, "lastfm");

	scrobble = rena_preferences_get_boolean (preferences,
	                                           plugin_group,
	                                           KEY_LASTFM_SCROBBLE);

	g_free (plugin_group);

	return scrobble;
}


/* Upadate lastfm menubar acording lastfm state */

static void
rena_action_group_set_sensitive (GtkActionGroup *group, const gchar *name, gboolean sensitive)
{
	GtkAction *action;
	action = gtk_action_group_get_action (group, name);
	gtk_action_set_sensitive (action, sensitive);
}

static void
rena_lastfm_update_menu_actions (RenaLastfmPlugin *plugin)
{
	RenaBackend *backend;
	RenaBackendState state = ST_STOPPED;
	GtkWindow *window;

	RenaLastfmPluginPrivate *priv = plugin->priv;

	backend = rena_application_get_backend (priv->rena);
	state = rena_backend_get_state (backend);

	gboolean playing    = (state != ST_STOPPED);
	gboolean lfm_inited = (priv->session_id != NULL);
	gboolean has_user   = (lfm_inited && priv->has_user);

	rena_action_group_set_sensitive (priv->action_group_main_menu, "Add favorites", has_user);
	rena_action_group_set_sensitive (priv->action_group_main_menu, "Add similar", playing && lfm_inited);

	rena_action_group_set_sensitive (priv->action_group_playlist, "Add similar", lfm_inited);

	window = GTK_WINDOW(rena_application_get_window(priv->rena));
	rena_menubar_set_enable_action (window, "lastfm-favorities", has_user);
	rena_menubar_set_enable_action (window, "lastfm-similar", playing && lfm_inited);
}

/*
 * Advise not connect with lastfm.
 */
static void rena_lastfm_no_connection_advice (void)
{
	RenaAppNotification *notification;
	notification = rena_app_notification_new ("Last.fm", _("Unable to establish conection with Last.fm"));
	rena_app_notification_show (notification);
}

/* Find a song with the artist and title independently of the album and adds it to the playlist */

static GList *
prepend_song_with_artist_and_title_to_mobj_list (RenaLastfmPlugin *plugin,
                                                 const gchar *artist,
                                                 const gchar *title,
                                                 GList *list)
{
	RenaPlaylist *playlist;
	RenaDatabase *cdbase;
	RenaMusicobject *mobj = NULL;
	gint location_id = 0;

	RenaLastfmPluginPrivate *priv = plugin->priv;

	playlist = rena_application_get_playlist (priv->rena);

	if (rena_mobj_list_already_has_title_of_artist (list, title, artist) ||
	    rena_playlist_already_has_title_of_artist (playlist, title, artist))
		return list;

	cdbase = rena_application_get_database (priv->rena);

	const gchar *sql =
		"SELECT LOCATION.id "
		"FROM TRACK, ARTIST, PROVIDER, LOCATION "
		"WHERE ARTIST.id = TRACK.artist "
		"AND LOCATION.id = TRACK.location "
		"AND TRACK.provider = PROVIDER.id AND PROVIDER.visible <> 0 "
		"AND TRACK.title = ? COLLATE NOCASE "
		"AND ARTIST.name = ? COLLATE NOCASE "
		"ORDER BY RANDOM() LIMIT 1;";

	RenaPreparedStatement *statement = rena_database_create_statement (cdbase, sql);
	rena_prepared_statement_bind_string (statement, 1, title);
	rena_prepared_statement_bind_string (statement, 2, artist);

	if (rena_prepared_statement_step (statement)) {
		location_id = rena_prepared_statement_get_int (statement, 0);
		mobj = new_musicobject_from_db (cdbase, location_id);
		list = g_list_prepend (list, mobj);
	}

	rena_prepared_statement_free (statement);

	return list;
}


/* Set correction basedm on lastfm now playing segestion.. */

static void
rena_corrected_by_lastfm_dialog_response (GtkWidget    *dialog,
                                            gint          response_id,
                                            RenaLastfmPlugin *plugin)
{
	RenaBackend *backend;
	RenaPlaylist *playlist;
	RenaToolbar *toolbar;
	RenaMusicobject *nmobj, *current_mobj;
	RenaTagger *tagger;
	gint changed = 0;

	RenaLastfmPluginPrivate *priv = plugin->priv;

	if (response_id == GTK_RESPONSE_HELP) {
		nmobj = rena_tags_dialog_get_musicobject(RENA_TAGS_DIALOG(dialog));
		rena_track_properties_dialog(nmobj, rena_application_get_window(priv->rena));
		return;
	}

	if (response_id == GTK_RESPONSE_OK) {
		changed = rena_tags_dialog_get_changed(RENA_TAGS_DIALOG(dialog));
		if (changed) {
			backend = rena_application_get_backend (priv->rena);

			nmobj = rena_tags_dialog_get_musicobject(RENA_TAGS_DIALOG(dialog));

			if (rena_backend_get_state (backend) != ST_STOPPED) {
				current_mobj = rena_backend_get_musicobject (backend);
				if (rena_musicobject_compare(nmobj, current_mobj) == 0) {
					toolbar = rena_application_get_toolbar (priv->rena);

					/* Update public current song */
					rena_update_musicobject_change_tag(current_mobj, changed, nmobj);

					/* Update current song on playlist */
					playlist = rena_application_get_playlist (priv->rena);
					rena_playlist_update_current_track (playlist, changed, nmobj);

					rena_toolbar_set_title(toolbar, current_mobj);
				}
			}

			if (G_LIKELY(rena_musicobject_is_local_file (nmobj))) {
				tagger = rena_tagger_new();
				rena_tagger_add_file (tagger, rena_musicobject_get_file(nmobj));
				rena_tagger_set_changes(tagger, nmobj, changed);
				rena_tagger_apply_changes (tagger);
				g_object_unref(tagger);
			}
		}
	}

	gtk_widget_hide (priv->ntag_lastfm_button);
	gtk_widget_destroy (dialog);
}

static void
rena_lastfm_tags_corrected_dialog (GtkButton *button, RenaLastfmPlugin *plugin)
{
	RenaBackend *backend;
	RenaMusicobject *tmobj, *nmobj;
	gchar *otitle = NULL, *oartist = NULL, *oalbum = NULL;
	gchar *ntitle = NULL, *nartist = NULL, *nalbum = NULL;
	gint prechanged = 0;
	GtkWidget *dialog;

	RenaLastfmPluginPrivate *priv = plugin->priv;

	backend = rena_application_get_backend (priv->rena);

	if (rena_backend_get_state (backend) == ST_STOPPED)
		return;

	/* Get all info of current track */

	tmobj = rena_musicobject_dup (rena_backend_get_musicobject (backend));

	g_object_get(tmobj,
	             "title", &otitle,
	             "artist", &oartist,
	             "album", &oalbum,
	             NULL);

	/* Get all info of suggestions
	 * Temp Musicobject to not block tag edit dialog */
	g_mutex_lock (&priv->data_mutex);
	nmobj = rena_musicobject_dup(priv->updated_mobj);
	g_mutex_unlock (&priv->data_mutex);

	g_object_get(nmobj,
	             "title", &ntitle,
	             "artist", &nartist,
	             "album", &nalbum,
	             NULL);

	/* Compare original mobj and suggested from lastfm */
	if (g_ascii_strcasecmp(otitle, ntitle)) {
		rena_musicobject_set_title(tmobj, ntitle);
		prechanged |= TAG_TITLE_CHANGED;
	}
	if (g_ascii_strcasecmp(oartist, nartist)) {
		rena_musicobject_set_artist(tmobj, nartist);
		prechanged |= TAG_ARTIST_CHANGED;
	}
	if (g_ascii_strcasecmp(oalbum, nalbum)) {
		rena_musicobject_set_album(tmobj, nalbum);
		prechanged |= TAG_ALBUM_CHANGED;
	}

	dialog = rena_tags_dialog_new();
	gtk_window_set_transient_for (GTK_WINDOW(dialog),
		GTK_WINDOW(rena_application_get_window (priv->rena)));

	g_signal_connect (G_OBJECT (dialog), "response",
	                  G_CALLBACK (rena_corrected_by_lastfm_dialog_response), plugin);

	rena_tags_dialog_set_musicobject(RENA_TAGS_DIALOG(dialog), tmobj);
	rena_tags_dialog_set_changed(RENA_TAGS_DIALOG(dialog), prechanged);

	gtk_widget_show (dialog);
}

static GtkWidget*
rena_lastfm_tag_suggestion_button_new (RenaLastfmPlugin *plugin)
{
	GtkWidget* ntag_lastfm_button, *image;
	ntag_lastfm_button = gtk_button_new();
	gtk_button_set_relief(GTK_BUTTON(ntag_lastfm_button), GTK_RELIEF_NONE);

	image = gtk_image_new_from_icon_name ("tools-check-spelling", GTK_ICON_SIZE_MENU);
	gtk_button_set_image(GTK_BUTTON(ntag_lastfm_button), image);

	gtk_widget_set_tooltip_text(GTK_WIDGET(ntag_lastfm_button),
	                            _("Last.fm suggested a tag correction"));

	g_signal_connect(G_OBJECT(ntag_lastfm_button), "clicked",
	                 G_CALLBACK(rena_lastfm_tags_corrected_dialog), plugin);

	rena_hig_set_tiny_button (ntag_lastfm_button);
	gtk_image_set_pixel_size (GTK_IMAGE(image), 12);

	return ntag_lastfm_button;
}

/* Love and unlove music object */

gpointer
do_lastfm_love_mobj (RenaLastfmPlugin *plugin, const gchar *title, const gchar *artist)
{
	IdleMessage *id = NULL;
	gint rv;

	CDEBUG(DBG_PLUGIN, "Love mobj on thread");

	RenaLastfmPluginPrivate *priv = plugin->priv;
	rv = LASTFM_track_love (priv->session_id,
	                        title,
	                        artist);

	if (rv != LASTFM_STATUS_OK)
		id = rena_idle_message_new ("Last.fm", _("Love song on Last.fm failed."), FALSE);

	return id;
}

gpointer
do_lastfm_unlove_mobj (RenaLastfmPlugin *plugin, const gchar *title, const gchar *artist)
{
	IdleMessage *id = NULL;
	gint rv;

	CDEBUG(DBG_PLUGIN, "Unlove mobj on thread");

	RenaLastfmPluginPrivate *priv = plugin->priv;
	rv = LASTFM_track_unlove (priv->session_id,
	                          title,
	                          artist);

	if (rv != LASTFM_STATUS_OK)
		id = rena_idle_message_new ("Last.fm", _("Unlove song on Last.fm failed."), FALSE);

	return id;
}

static gboolean
append_mobj_list_current_playlist_idle(gpointer user_data)
{
	RenaAppNotification *notification;
	RenaPlaylist *playlist;
	gchar *summary = NULL;
	guint songs_added = 0;

	AddMusicObjectListData *data = user_data;

	GList *list = data->list;
	RenaLastfmPlugin *plugin = data->plugin;

	RenaLastfmPluginPrivate *priv = plugin->priv;

	if (list != NULL) {
		playlist = rena_application_get_playlist (priv->rena);
		rena_playlist_append_mobj_list (playlist, list);

		songs_added = g_list_length(list);
		g_list_free(list);
	}
	else {
		remove_watch_cursor (rena_application_get_window(priv->rena));
	}

	switch(data->query_type) {
		case LASTFM_GET_SIMILAR:
			if (data->query_count > 0)
				summary = g_strdup_printf(_("Added %d tracks of %d suggested from Last.fm"),
				                          songs_added, data->query_count);
			else
				summary = g_strdup_printf(_("Last.fm doesn't suggest any similar track"));
			break;
		case LASTFM_GET_LOVED:
			if (data->query_count > 0)
				summary = g_strdup_printf(_("Added %d songs of the last %d loved on Last.fm."),
							  songs_added, data->query_count);
			else
				summary = g_strdup_printf(_("You don't have favorite tracks on Last.fm"));
			break;
		case LASTFM_NONE:
		default:
			break;
	}

	if (summary != NULL) {
		notification = rena_app_notification_new ("Last.fm", summary);
		rena_app_notification_set_timeout (notification, 5);
		rena_app_notification_show (notification);
	}

	g_slice_free (AddMusicObjectListData, data);

	return FALSE;
}

gpointer
do_lastfm_get_similar (RenaLastfmPlugin *plugin, const gchar *title, const gchar *artist)
{
	LFMList *results = NULL, *li;
	LASTFM_TRACK_INFO *track = NULL;
	guint query_count = 0;
	GList *list = NULL;
	gint rv;

	AddMusicObjectListData *data;
	RenaLastfmPluginPrivate *priv = plugin->priv;

	if (string_is_not_empty(title) && string_is_not_empty(artist)) {
		rv = LASTFM_track_get_similar (priv->session_id,
		                               title,
		                               artist,
		                               50, &results);

		for (li=results; li && rv == LASTFM_STATUS_OK; li=li->next) {
			track = li->data;
			list = prepend_song_with_artist_and_title_to_mobj_list (plugin, track->artist, track->name, list);
			query_count += 1;
		}
	}

	data = g_slice_new (AddMusicObjectListData);
	data->list = list;
	data->query_type = LASTFM_GET_SIMILAR;
	data->query_count = query_count;
	data->plugin = plugin;

	LASTFM_free_track_info_list (results);

	return data;
}

gpointer
do_lastfm_get_similar_current_playlist_action (gpointer user_data)
{
	RenaPlaylist *playlist;
	RenaMusicobject *mobj = NULL;
	const gchar *title, *artist;

	AddMusicObjectListData *data;

	RenaLastfmPlugin *plugin = user_data;
	RenaLastfmPluginPrivate *priv = plugin->priv;

	playlist = rena_application_get_playlist (priv->rena);
	mobj = rena_playlist_get_selected_musicobject (playlist);

	title = rena_musicobject_get_title(mobj);
	artist = rena_musicobject_get_artist(mobj);

	data = do_lastfm_get_similar (plugin, title, artist);

	return data;
}

static void
lastfm_get_similar_current_playlist_action (GtkAction *action, RenaLastfmPlugin *plugin)
{
	RenaLastfmPluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "Get similar action to current playlist");

	if (priv->session_id == NULL) {
		rena_lastfm_no_connection_advice ();
		return;
	}

	set_watch_cursor (rena_application_get_window(priv->rena));
	rena_async_launch (do_lastfm_get_similar_current_playlist_action,
	                     append_mobj_list_current_playlist_idle,
	                     plugin);
}

/* Functions that respond to menu options. */

static void
lastfm_import_xspf_response (GtkDialog          *dialog,
                             gint                response,
                             RenaLastfmPlugin *plugin)
{
	RenaAppNotification *notification;
	RenaPlaylist *playlist;
	XMLNode *xml = NULL, *xi, *xc, *xt;
	gchar *contents, *summary;
	gint try = 0, added = 0;
	GList *list = NULL;

	GFile *file;
	gsize size;

	RenaLastfmPluginPrivate *priv = plugin->priv;

	if (response != GTK_RESPONSE_ACCEPT)
		goto cancel;

	file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));

	if (!g_file_load_contents (file, NULL, &contents, &size, NULL, NULL)) {
		goto out;
    	}

	if (g_utf8_validate (contents, -1, NULL) == FALSE) {
		gchar *fixed;
		fixed = g_convert (contents, -1, "UTF-8", "ISO8859-1", NULL, NULL, NULL);
		if (fixed != NULL) {
			g_free (contents);
			contents = fixed;
		}
	}

	xml = tinycxml_parse(contents);

	xi = xmlnode_get(xml,CCA { "playlist","trackList","track",NULL},NULL,NULL);
	for(;xi;xi= xi->next) {
		try++;
		xt = xmlnode_get(xi,CCA {"track","title",NULL},NULL,NULL);
		xc = xmlnode_get(xi,CCA {"track","creator",NULL},NULL,NULL);

		if (xt && xc)
			list = prepend_song_with_artist_and_title_to_mobj_list (plugin, xc->content, xt->content, list);
	}

	if (list) {
		playlist = rena_application_get_playlist (priv->rena);
		rena_playlist_append_mobj_list (playlist, list);
		g_list_free (list);
	}

	summary = g_strdup_printf(_("Added %d songs from %d of the imported playlist."), added, try);

	notification = rena_app_notification_new ("Last.fm", summary);
	rena_app_notification_set_timeout(notification, 5);
	rena_app_notification_show (notification);

	g_free(summary);

	xmlnode_free(xml);
	g_free (contents);
out:
	g_object_unref (file);
cancel:
	gtk_widget_destroy (GTK_WIDGET(dialog));
}

static void
lastfm_import_xspf_action (GtkAction *action, RenaLastfmPlugin *plugin)
{
	GtkWidget *dialog;
	GtkFileFilter *media_filter;

	RenaLastfmPluginPrivate *priv = plugin->priv;

	dialog = gtk_file_chooser_dialog_new (_("Import a XSPF playlist"),
	                                      GTK_WINDOW(rena_application_get_window(priv->rena)),
	                                      GTK_FILE_CHOOSER_ACTION_OPEN,
	                                      _("_Cancel"), GTK_RESPONSE_CANCEL,
	                                      _("_Open"), GTK_RESPONSE_ACCEPT,
	                                      NULL);

	media_filter = gtk_file_filter_new();
	gtk_file_filter_set_name(GTK_FILE_FILTER(media_filter), _("Supported media"));
	gtk_file_filter_add_mime_type(GTK_FILE_FILTER(media_filter), "application/xspf+xml");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), GTK_FILE_FILTER(media_filter));

	g_signal_connect (G_OBJECT(dialog), "response",
	                  G_CALLBACK(lastfm_import_xspf_response), plugin);

	gtk_widget_show_all (dialog);
}

gpointer
do_lastfm_add_favorites_action (gpointer user_data)
{
	RenaPreferences *preferences;
	LFMList *results = NULL, *li;
	LASTFM_TRACK_INFO *track;
	gint rpages = 0, cpage = 0;
	AddMusicObjectListData *data;
	guint query_count = 0;
	GList *list = NULL;
	gchar *user = NULL;

	RenaLastfmPlugin *plugin = user_data;
	RenaLastfmPluginPrivate *priv = plugin->priv;

	preferences = rena_application_get_preferences (priv->rena);
	user = rena_lastfm_plugin_get_user (preferences);

	do {
		rpages = LASTFM_user_get_loved_tracks (priv->session_id,
		                                       user,
		                                       cpage,
		                                       &results);

		for (li=results; li; li=li->next) {
			track = li->data;
			list = prepend_song_with_artist_and_title_to_mobj_list (plugin, track->artist, track->name, list);
			query_count += 1;
		}
		LASTFM_free_track_info_list (results);
		cpage++;
	} while (rpages != 0);

	data = g_slice_new (AddMusicObjectListData);
	data->list = list;
	data->query_type = LASTFM_GET_LOVED;
	data->query_count = query_count;
	data->plugin = plugin;

	g_free (user);

	return data;
}

static void
lastfm_add_favorites_action (GtkAction *action, RenaLastfmPlugin *plugin)
{
	RenaLastfmPluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "Add Favorites action");

	if ((priv->session_id == NULL) || !priv->has_user) {
		rena_lastfm_no_connection_advice ();
		return;
	}

	set_watch_cursor (rena_application_get_window(priv->rena));
	rena_async_launch (do_lastfm_add_favorites_action,
	                     append_mobj_list_current_playlist_idle,
	                     plugin);
}

static gpointer
do_lastfm_get_similar_action (gpointer user_data)
{
	AddMusicObjectListData *data;

	RenaLastfmAsyncData *lastfm_async_data = user_data;

	RenaLastfmPlugin *plugin   = lastfm_async_data->plugin;
	RenaMusicobject *mobj = lastfm_async_data->mobj;

	const gchar *title = rena_musicobject_get_title (mobj);
	const gchar *artist = rena_musicobject_get_artist (mobj);

	data = do_lastfm_get_similar (plugin, title, artist);

	rena_lastfm_async_data_free(lastfm_async_data);

	return data;
}

static void
lastfm_get_similar_action (GtkAction *action, RenaLastfmPlugin *plugin)
{
	RenaBackend *backend;

	RenaLastfmPluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "Get similar action");

	backend = rena_application_get_backend (priv->rena);
	if (rena_backend_get_state (backend) == ST_STOPPED)
		return;

	if (priv->session_id == NULL) {
		rena_lastfm_no_connection_advice ();
		return;
	}

	set_watch_cursor (rena_application_get_window(priv->rena));
	rena_async_launch (do_lastfm_get_similar_action,
	                     append_mobj_list_current_playlist_idle,
	                     rena_lastfm_async_data_new (plugin, NULL));
}

static gpointer
do_lastfm_current_song_love (gpointer data)
{
	gpointer msg_data = NULL;

	RenaLastfmAsyncData *lastfm_async_data = data;

	RenaLastfmPlugin *plugin = lastfm_async_data->plugin;
	RenaMusicobject *mobj = lastfm_async_data->mobj;

	const gchar *title = rena_musicobject_get_title (mobj);
	const gchar *artist = rena_musicobject_get_artist (mobj);

	msg_data = do_lastfm_love_mobj (plugin, title, artist);

	rena_lastfm_async_data_free(lastfm_async_data);

	return msg_data;
}

static gpointer
do_lastfm_current_song_unlove (gpointer data)
{
	gpointer msg_data = NULL;

	RenaLastfmAsyncData *lastfm_async_data = data;

	RenaLastfmPlugin *plugin = lastfm_async_data->plugin;
	RenaMusicobject *mobj = lastfm_async_data->mobj;

	const gchar *title = rena_musicobject_get_title (mobj);
	const gchar *artist = rena_musicobject_get_artist (mobj);

	msg_data = do_lastfm_unlove_mobj (plugin, title, artist);

	rena_lastfm_async_data_free(lastfm_async_data);

	return msg_data;
}

static void
rena_lastfm_plugin_favorites_song_added (RenaFavorites    *favorites,
                                           RenaMusicobject  *mobj,
                                           RenaLastfmPlugin *plugin)
{
	RenaLastfmPluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "Lastfm: Love Handler");

	if (!priv->has_user || !priv->has_pass)
		return;

	if (priv->status != LASTFM_STATUS_OK) {
		rena_lastfm_no_connection_advice ();
		return;
	}

	rena_async_launch (do_lastfm_current_song_love,
	                     rena_async_set_idle_message,
	                     rena_lastfm_async_data_new (plugin, mobj));

}

static void
rena_lastfm_plugin_favorites_song_removed (RenaFavorites    *favorites,
                                             RenaMusicobject  *mobj,
                                             RenaLastfmPlugin *plugin)
{
	RenaLastfmPluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "Lastfm: Unlove Handler");

	if (!priv->has_user || !priv->has_pass)
		return;

	if (priv->status != LASTFM_STATUS_OK) {
		rena_lastfm_no_connection_advice ();
		return;
	}

	rena_async_launch (do_lastfm_current_song_unlove,
	                     rena_async_set_idle_message,
	                     rena_lastfm_async_data_new (plugin, mobj));
}

/*
 * Gear menu actions.
 */

static void
rena_gmenu_lastfm_add_favorites_action (GSimpleAction *action,
                                          GVariant      *parameter,
                                          gpointer       user_data)
{
	lastfm_add_favorites_action (NULL, RENA_LASTFM_PLUGIN(user_data));
}

static void
rena_gmenu_lastfm_get_similar_action (GSimpleAction *action,
                                        GVariant      *parameter,
                                        gpointer       user_data)
{
	lastfm_get_similar_action (NULL, RENA_LASTFM_PLUGIN(user_data));
}

static void
rena_gmenu_lastfm_import_xspf_action (GSimpleAction *action,
                                        GVariant      *parameter,
                                        gpointer       user_data)
{
	lastfm_import_xspf_action (NULL, RENA_LASTFM_PLUGIN(user_data));
}

/*
 * Handlers
 */

static gpointer
rena_lastfm_scrobble_thread (gpointer data)
{
	IdleMessage *id = NULL;
	gchar *title = NULL, *artist = NULL, *album = NULL;
	gint track_no, length, rv;
	time_t last_time;

	RenaLastfmPlugin *plugin = data;
	RenaLastfmPluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "Scrobbler thread");

	g_mutex_lock (&priv->data_mutex);
	if (priv->playback_started == 0) {
		g_mutex_unlock (&priv->data_mutex);
		return rena_idle_message_new ("Last.fm", _("Last.fm submission failed"), FALSE);
	}

	g_object_get (priv->current_mobj,
	              "title",    &title,
	              "artist",   &artist,
	              "album",    &album,
	              "track-no", &track_no,
	              "length",   &length,
	              NULL);
	last_time = priv->playback_started;
	g_mutex_unlock (&priv->data_mutex);

	rv = LASTFM_track_scrobble (priv->session_id,
	                            title,
	                            album,
	                            artist,
	                            last_time,
	                            length,
	                            track_no,
	                            0, NULL);

	g_free(title);
	g_free(artist);
	g_free(album);

	g_mutex_lock (&priv->data_mutex);
	priv->playback_started = 0;
	g_mutex_unlock (&priv->data_mutex);

	if (rv != LASTFM_STATUS_OK)
		id = rena_idle_message_new ("Last.fm", _("Last.fm submission failed"), FALSE);
	else {
		id = rena_idle_message_new ("Last.fm", _("Track scrobbled on Last.fm"), TRUE);
	}

	return id;
}

static gboolean
rena_lastfm_show_corrrection_button (gpointer user_data)
{
	RenaToolbar *toolbar;
	RenaBackend *backend;
	gchar *cfile = NULL, *nfile = NULL;

	RenaLastfmPlugin *plugin = user_data;
	RenaLastfmPluginPrivate *priv = plugin->priv;

	/* Hack to safe!.*/
	if (!priv->ntag_lastfm_button) {
		toolbar = rena_application_get_toolbar (priv->rena);

		priv->ntag_lastfm_button =
			rena_lastfm_tag_suggestion_button_new (plugin);

		rena_toolbar_add_extention_widget (toolbar, priv->ntag_lastfm_button);
	}

	backend = rena_application_get_backend (priv->rena);
	g_object_get(rena_backend_get_musicobject (backend),
	             "file", &cfile,
	             NULL);

	g_mutex_lock (&priv->data_mutex);
	g_object_get (priv->updated_mobj,
	              "file", &nfile,
	              NULL);
	g_mutex_unlock (&priv->data_mutex);

	if (g_ascii_strcasecmp(cfile, nfile) == 0)
		gtk_widget_show (priv->ntag_lastfm_button);

	g_free(cfile);
	g_free(nfile);

	return FALSE;
}

static gpointer
rena_lastfm_now_playing_thread (gpointer data)
{
	IdleMessage *id = NULL;
	LFMList *list = NULL;
	LASTFM_TRACK_INFO *ntrack = NULL;
	gchar *title = NULL, *artist = NULL, *album = NULL;
	gchar *utitle = NULL, *uartist = NULL, *ualbum = NULL;
	gint track_no, length, changed = 0, rv;

	RenaLastfmPlugin *plugin = data;
	RenaLastfmPluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "Update now playing thread");

	g_mutex_lock (&priv->data_mutex);
	g_object_get (priv->current_mobj,
	              "title",    &title,
	              "artist",   &artist,
	              "album",    &album,
	              "track-no", &track_no,
	              "length",   &length,
	              NULL);
	g_mutex_unlock (&priv->data_mutex);

	rv = LASTFM_track_update_now_playing (priv->session_id,
	                                      title,
	                                      album,
	                                      artist,
	                                      length,
	                                      track_no,
	                                      0,
	                                      &list);

	if (rv == LASTFM_STATUS_OK) {
		/* Fist check lastfm response, and compare tags. */
		if (list != NULL) {
			ntrack = list->data;
			utitle = rena_unescape_html_utf75(ntrack->name);
			uartist = rena_unescape_html_utf75(ntrack->artist);
			ualbum = rena_unescape_html_utf75(ntrack->album);
			if (utitle && g_ascii_strcasecmp(title, utitle))
				changed |= TAG_TITLE_CHANGED;
			if (uartist && g_ascii_strcasecmp(artist, uartist))
				changed |= TAG_ARTIST_CHANGED;
			if (ualbum && g_ascii_strcasecmp(album, ualbum))
				changed |= TAG_ALBUM_CHANGED;
		}

		if (changed) {
			g_mutex_lock (&priv->data_mutex);
			if (priv->updated_mobj) 
				g_object_unref (priv->updated_mobj);
			priv->updated_mobj = rena_musicobject_dup (priv->current_mobj);
			if (changed & TAG_TITLE_CHANGED)
				rena_musicobject_set_title (priv->updated_mobj, utitle);
			if (changed & TAG_ARTIST_CHANGED)
				rena_musicobject_set_artist (priv->updated_mobj, uartist);
			if (changed & TAG_ALBUM_CHANGED)
				rena_musicobject_set_album (priv->updated_mobj, ualbum);
			g_mutex_unlock (&priv->data_mutex);

			g_idle_add (rena_lastfm_show_corrrection_button, plugin);
		}
	}
	LASTFM_free_track_info_list(list);

	g_free(title);
	g_free(artist);
	g_free(album);

	g_free(utitle);
	g_free(uartist);
	g_free(ualbum);

	if (rv != LASTFM_STATUS_OK)
		id = rena_idle_message_new ("Last.fm", _("Update current song on Last.fm failed."), FALSE);

	return id;
}

static gboolean
rena_lastfm_now_playing_handler (gpointer data)
{
	RenaBackend *backend;
	RenaMusicobject *mobj = NULL;

	RenaLastfmPlugin *plugin = data;
	RenaLastfmPluginPrivate *priv = plugin->priv;

	priv->update_timeout_id = 0;

	CDEBUG(DBG_PLUGIN, "Update now playing Handler");

	/* Set current song info */
	backend = rena_application_get_backend (priv->rena);
	mobj = rena_backend_get_musicobject (backend);

	g_mutex_lock (&priv->data_mutex);
	if (priv->current_mobj) {
		g_object_unref (priv->current_mobj);
		priv->current_mobj = NULL;
	}
	if (priv->updated_mobj) {
		g_object_unref (priv->updated_mobj);
		priv->updated_mobj = NULL;
	}

	if (priv->playback_started) {
		g_critical ("Postposed update now_playing since a scrobble is pending");
		g_mutex_unlock (&priv->data_mutex);
		return G_SOURCE_CONTINUE;
	}

	priv->current_mobj = rena_musicobject_dup (mobj);
	time(&priv->playback_started);
	g_mutex_unlock (&priv->data_mutex);

	/* Launch tread */
	rena_async_launch (rena_lastfm_now_playing_thread,
	                     rena_async_set_idle_message,
	                     plugin);

	return G_SOURCE_REMOVE;
}

static gboolean
rena_lastfm_scrobble_handler (gpointer data)
{
	RenaLastfmPlugin *plugin = data;
	RenaLastfmPluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "Scrobbler Handler");

	priv->scrobble_timeout_id = 0;

	if (priv->status != LASTFM_STATUS_OK) {
		rena_lastfm_no_connection_advice ();
		return G_SOURCE_REMOVE;
	}

	rena_async_launch (rena_lastfm_scrobble_thread,
	                     rena_async_set_idle_message,
	                     plugin);

	return G_SOURCE_REMOVE;
}

static void
backend_changed_state_cb (RenaBackend *backend, GParamSpec *pspec, gpointer user_data)
{
	RenaPreferences *preferences;
	RenaMusicobject *mobj = NULL;
	RenaMusicSource file_source = FILE_NONE;
	RenaBackendState state = 0;
	gint length = 0, dalay_time = 0;

	RenaLastfmPlugin *plugin = user_data;
	RenaLastfmPluginPrivate *priv = plugin->priv;

	state = rena_backend_get_state (backend);

	CDEBUG(DBG_PLUGIN, "Configuring thread to update Lastfm");

	/* Update gui. */

	rena_lastfm_update_menu_actions (plugin);

	/* Update thread. */

	if (priv->update_timeout_id) {
		g_source_remove (priv->update_timeout_id);
		priv->update_timeout_id = 0;
	}

	if (priv->scrobble_timeout_id) {
		g_source_remove (priv->scrobble_timeout_id);
		priv->scrobble_timeout_id = 0;
	}

	g_mutex_lock (&priv->data_mutex);
	if (priv->playback_started != 0) {
		priv->playback_started = 0;
	}
	g_mutex_unlock (&priv->data_mutex);

	if (state != ST_PLAYING) {
		if (priv->ntag_lastfm_button)
			gtk_widget_hide (priv->ntag_lastfm_button);
		return;
	}

	/*
	 * Check settings, status, file-type, title, artist and length before update.
	 */

	preferences = rena_application_get_preferences (priv->rena);
	if (!rena_lastfm_plugin_get_scrobble_support (preferences))
		return;

	if (!priv->has_user || !priv->has_pass)
		return;

	if (priv->status != LASTFM_STATUS_OK)
		return;

	mobj = rena_backend_get_musicobject (backend);

	file_source = rena_musicobject_get_source (mobj);
	if (file_source == FILE_NONE || file_source == FILE_HTTP)
		return;

	length = rena_musicobject_get_length (mobj);
	if (length < 30)
		return;

	if (string_is_empty(rena_musicobject_get_title(mobj)))
		return;
	if (string_is_empty(rena_musicobject_get_artist(mobj)))
		return;

	/* Now playing delayed handler */
	priv->update_timeout_id =
		g_timeout_add_seconds_full (G_PRIORITY_DEFAULT_IDLE, WAIT_UPDATE,
	                                rena_lastfm_now_playing_handler, plugin, NULL);

	/* Scrobble delayed handler */
	dalay_time = ((length / 2) > 240) ? 240 : (length / 2);
	priv->scrobble_timeout_id =
		g_timeout_add_seconds_full (G_PRIORITY_DEFAULT_IDLE, dalay_time,
		                            rena_lastfm_scrobble_handler, plugin, NULL);
}

static void
rena_menubar_append_lastfm (RenaLastfmPlugin *plugin)
{
	RenaPlaylist *playlist;
	GtkWindow *window;
	GActionMap *map;

	RenaLastfmPluginPrivate *priv = plugin->priv;

	/*
	 * Menubar
	 */
	priv->action_group_main_menu = rena_menubar_plugin_action_new ("RenaLastfmMainMenuActions",
	                                                                 main_menu_actions,
	                                                                 G_N_ELEMENTS (main_menu_actions),
	                                                                 NULL,
	                                                                 0,
	                                                                 plugin);

	priv->merge_id_main_menu = rena_menubar_append_plugin_action (priv->rena,
	                                                                priv->action_group_main_menu,
	                                                                main_menu_xml);

	rena_action_group_set_sensitive (priv->action_group_main_menu, "Add favorites", FALSE);
	rena_action_group_set_sensitive (priv->action_group_main_menu, "Add similar", FALSE);

	/*
	 * Playlist
	 */
	priv->action_group_playlist = rena_menubar_plugin_action_new ("RenaLastfmPlaylistActions",
	                                                                playlist_actions,
	                                                                G_N_ELEMENTS (playlist_actions),
	                                                                NULL,
	                                                                0,
	                                                                plugin);

	playlist = rena_application_get_playlist (priv->rena);
	priv->merge_id_playlist = rena_playlist_append_plugin_action (playlist,
	                                                                priv->action_group_playlist,
	                                                                playlist_xml);

	/*
	 * Gear Menu
	 */
	rena_menubar_append_submenu (priv->rena, "rena-plugins-placeholder",
	                               lastfm_menu_ui,
	                               "lastfm-sudmenu",
	                               _("_Lastfm"),
	                               plugin);

	map = G_ACTION_MAP (rena_application_get_window(priv->rena));
	g_action_map_add_action_entries (G_ACTION_MAP (map),
	                                 lastfm_entries, G_N_ELEMENTS(lastfm_entries),
	                                 plugin);

	window = GTK_WINDOW(rena_application_get_window(priv->rena));
	rena_menubar_set_enable_action (window, "lastfm-favorities", FALSE);
	rena_menubar_set_enable_action (window, "lastfm-similar", FALSE);
}

static void
rena_menubar_remove_lastfm (RenaLastfmPlugin *plugin)
{
	RenaPlaylist *playlist;

	RenaLastfmPluginPrivate *priv = plugin->priv;

	if (!priv->merge_id_main_menu)
		return;

	rena_menubar_remove_plugin_action (priv->rena,
	                                     priv->action_group_main_menu,
	                                     priv->merge_id_main_menu);

	priv->merge_id_main_menu = 0;

	if (!priv->merge_id_playlist)
		return;

	playlist = rena_application_get_playlist (priv->rena);
	rena_playlist_remove_plugin_action (playlist,
	                                      priv->action_group_playlist,
	                                      priv->merge_id_playlist);

	priv->merge_id_playlist = 0;

	rena_menubar_remove_by_id (priv->rena,
	                             "rena-plugins-placeholder",
	                             "lastfm-sudmenu");
}

static gboolean
rena_lastfm_connect_idle(gpointer data)
{
	RenaPreferences *preferences;
	gchar *user = NULL, *pass = NULL;
	gboolean scrobble = FALSE;

	RenaLastfmPlugin *plugin = data;
	RenaLastfmPluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "Connecting LASTFM");

	priv->session_id = LASTFM_init (LASTFM_API_KEY, LASTFM_SECRET);

	preferences = rena_application_get_preferences (priv->rena);

	scrobble = rena_lastfm_plugin_get_scrobble_support (preferences);
	user = rena_lastfm_plugin_get_user (preferences);
	pass = rena_lastfm_plugin_get_password (preferences);

	priv->has_user = string_is_not_empty(user);
	priv->has_pass = string_is_not_empty(pass);

	if (scrobble && priv->has_user && priv->has_pass) {
		priv->status = LASTFM_login (priv->session_id, user, pass);

		if (priv->status != LASTFM_STATUS_OK) {
			rena_lastfm_no_connection_advice ();
			CDEBUG(DBG_PLUGIN, "Failure to login on lastfm");
		}
	}

	rena_lastfm_update_menu_actions (plugin);

	g_free(user);
	g_free(pass);

	return FALSE;
}

/* Init lastfm with a simple thread when change preferences and show error messages. */

static void
rena_lastfm_connect (RenaLastfmPlugin *plugin)
{
	g_idle_add (rena_lastfm_connect_idle, plugin);
}

static void
rena_lastfm_disconnect (RenaLastfmPlugin *plugin)
{
	RenaLastfmPluginPrivate *priv = plugin->priv;

	if (priv->session_id != NULL) {
		CDEBUG(DBG_PLUGIN, "Disconnecting LASTFM");

		LASTFM_dinit(priv->session_id);

		priv->session_id = NULL;
		priv->status = LASTFM_STATUS_INVALID;
		priv->has_user = FALSE;
		priv->has_pass = FALSE;
	}
	rena_lastfm_update_menu_actions (plugin);
}

/*
 * Lastfm Settings
 */
static void
rena_lastfm_preferences_dialog_response (GtkDialog    *dialog,
                                           gint          response_id,
                                           RenaLastfmPlugin *plugin)
{
	RenaPreferences *preferences;
	const gchar *entry_user = NULL, *entry_pass = NULL;
	gchar *test_user = NULL, *test_pass = NULL;
	gboolean changed = FALSE, test_scrobble = FALSE, toggle_scrobble = FALSE;

	RenaLastfmPluginPrivate *priv = plugin->priv;

	preferences = rena_preferences_get ();

	test_user = rena_lastfm_plugin_get_user (preferences);
	test_pass = rena_lastfm_plugin_get_password (preferences);

	switch(response_id) {
	case GTK_RESPONSE_CANCEL:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(priv->enable_w),
			rena_lastfm_plugin_get_scrobble_support (preferences));

		rena_gtk_entry_set_text (GTK_ENTRY(priv->lastfm_uname_w), test_user);
		rena_gtk_entry_set_text (GTK_ENTRY(priv->lastfm_pass_w), test_pass);
		break;
	case GTK_RESPONSE_OK:
		toggle_scrobble = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(priv->enable_w));
		entry_user = gtk_entry_get_text (GTK_ENTRY(priv->lastfm_uname_w));
		entry_pass = gtk_entry_get_text (GTK_ENTRY(priv->lastfm_pass_w));

		test_scrobble = rena_lastfm_plugin_get_scrobble_support (preferences);

		if (test_scrobble != toggle_scrobble) {
			rena_lastfm_plugin_set_scrobble_support (preferences, toggle_scrobble);
			changed = TRUE;
		}

		if (g_strcmp0 (test_user, entry_user)) {
			rena_lastfm_plugin_set_user (preferences, entry_user);
			changed = TRUE;
		}

		if (g_strcmp0 (test_pass, entry_pass)) {
			rena_lastfm_plugin_set_password (preferences, entry_pass);
			changed = TRUE;
		}

		if (changed) {
			rena_lastfm_disconnect (plugin);
			rena_lastfm_connect (plugin);
		}
		break;
	default:
		break;
	}
	g_object_unref (preferences);
	g_free (test_user);
	g_free (test_pass);
}

static void
toggle_lastfm (GtkToggleButton *button, RenaLastfmPlugin *plugin)
{
	gboolean is_active;

	RenaLastfmPluginPrivate *priv = plugin->priv;

	is_active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->enable_w));

	gtk_widget_set_sensitive (priv->lastfm_uname_w, is_active);
	gtk_widget_set_sensitive (priv->lastfm_pass_w, is_active);

	if (!is_active) {
		gtk_entry_set_text (GTK_ENTRY(priv->lastfm_uname_w), "");
		gtk_entry_set_text (GTK_ENTRY(priv->lastfm_pass_w), "");
	}
}

static void
rena_lastfm_init_settings (RenaLastfmPlugin *plugin)
{
	RenaPreferences *preferences;
	gchar *user = NULL, *pass = NULL;

	RenaLastfmPluginPrivate *priv = plugin->priv;

	preferences = rena_preferences_get ();

	if (rena_lastfm_plugin_get_scrobble_support (preferences)) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(priv->enable_w), TRUE);

		user = rena_lastfm_plugin_get_user (preferences);
		if (string_is_not_empty(user))
			gtk_entry_set_text (GTK_ENTRY(priv->lastfm_uname_w), user);
		g_free(user);

		pass = rena_lastfm_plugin_get_password (preferences);
		if (string_is_not_empty(pass))
			gtk_entry_set_text (GTK_ENTRY(priv->lastfm_pass_w), pass);
		g_free(pass);
	}
	else {
		gtk_widget_set_sensitive (priv->lastfm_uname_w, FALSE);
		gtk_widget_set_sensitive (priv->lastfm_pass_w, FALSE);
	}

	g_object_unref (preferences);
}

static void
rena_lastfm_plugin_append_setting (RenaLastfmPlugin *plugin)
{
	RenaPreferencesDialog *dialog;
	GtkWidget *table;
	GtkWidget *lastfm_check, *lastfm_uname, *lastfm_pass, *lastfm_ulabel, *lastfm_plabel;
	guint row = 0;

	RenaLastfmPluginPrivate *priv = plugin->priv;

	table = rena_hig_workarea_table_new ();

	rena_hig_workarea_table_add_section_title (table, &row, "Last.fm");

	lastfm_check = gtk_check_button_new_with_label (_("Scrobble on Last.fm"));
	rena_hig_workarea_table_add_wide_control (table, &row, lastfm_check);

	lastfm_ulabel = gtk_label_new (_("Username"));
	lastfm_uname = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY(lastfm_uname), LASTFM_UNAME_LEN);
	gtk_entry_set_activates_default (GTK_ENTRY(lastfm_uname), TRUE);

	rena_hig_workarea_table_add_row (table, &row, lastfm_ulabel, lastfm_uname);

	lastfm_plabel = gtk_label_new (_("Password"));
	lastfm_pass = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY(lastfm_pass), LASTFM_PASS_LEN);
	gtk_entry_set_visibility (GTK_ENTRY(lastfm_pass), FALSE);
	gtk_entry_set_activates_default (GTK_ENTRY(lastfm_pass), TRUE);

	rena_hig_workarea_table_add_row (table, &row, lastfm_plabel, lastfm_pass);

	/* Store references. */

	priv->enable_w = lastfm_check;
	priv->lastfm_uname_w = lastfm_uname;
	priv->lastfm_pass_w = lastfm_pass;
	priv->setting_widget = table;

	/* Append panes */

	dialog = rena_application_get_preferences_dialog (priv->rena);
	rena_preferences_append_services_setting (dialog,
	                                            priv->setting_widget, FALSE);

	/* Configure handler and settings */
	rena_preferences_dialog_connect_handler (dialog,
	                                           G_CALLBACK(rena_lastfm_preferences_dialog_response),
	                                           plugin);

	g_signal_connect (G_OBJECT(lastfm_check), "toggled",
	                  G_CALLBACK(toggle_lastfm), plugin);

	rena_lastfm_init_settings (plugin);
}

static void
rena_lastfm_plugin_remove_setting (RenaLastfmPlugin *plugin)
{
	RenaPreferencesDialog *dialog;
	RenaLastfmPluginPrivate *priv = plugin->priv;

	dialog = rena_application_get_preferences_dialog (priv->rena);
	rena_preferences_remove_services_setting (dialog,
	                                            priv->setting_widget);

	rena_preferences_dialog_disconnect_handler (dialog,
	                                              G_CALLBACK(rena_lastfm_preferences_dialog_response),
	                                              plugin);
}

/*
 * Lastfm plugin
 */
static void
rena_plugin_activate (PeasActivatable *activatable)
{
	RenaLastfmPlugin *plugin = RENA_LASTFM_PLUGIN (activatable);
	RenaLastfmPluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "Lastfm plugin %s", G_STRFUNC);

	priv->rena = g_object_get_data (G_OBJECT (plugin), "object");

	/* Favororites */

	priv->favorites = rena_favorites_get ();

	/* Init plugin flags */

	priv->session_id = NULL;
	priv->status = LASTFM_STATUS_INVALID;

	g_mutex_init (&priv->data_mutex);
	priv->updated_mobj = NULL;
	priv->current_mobj = NULL;

	priv->ntag_lastfm_button = NULL;

	priv->has_user = FALSE;
	priv->has_pass = FALSE;

	priv->update_timeout_id = 0;
	priv->scrobble_timeout_id = 0;

	/* Append menu and settings */

	rena_menubar_append_lastfm (plugin);
	rena_lastfm_plugin_append_setting (plugin);

	/* Test internet and launch threads.*/

	if (g_network_monitor_get_network_available (g_network_monitor_get_default ())) {
		g_idle_add (rena_lastfm_connect_idle, plugin);
	}
	else {
		g_timeout_add_seconds_full (G_PRIORITY_DEFAULT_IDLE, 30,
		                            rena_lastfm_connect_idle, plugin, NULL);
	}

	/* Connect playback signals */

	g_signal_connect (rena_application_get_backend (priv->rena), "notify::state",
	                  G_CALLBACK (backend_changed_state_cb), plugin);

	/* Favorites handler */

	g_signal_connect (priv->favorites, "song-added",
	                  G_CALLBACK(rena_lastfm_plugin_favorites_song_added), plugin);
	g_signal_connect (priv->favorites, "song-removed",
	                  G_CALLBACK(rena_lastfm_plugin_favorites_song_removed), plugin);
}

static void
rena_plugin_deactivate (PeasActivatable *activatable)
{
	RenaPreferences *preferences;
	RenaLastfmPlugin *plugin = RENA_LASTFM_PLUGIN (activatable);
	RenaLastfmPluginPrivate *priv = plugin->priv;
	gchar *plugin_group = NULL;

	CDEBUG(DBG_PLUGIN, "Lastfm plugin %s", G_STRFUNC);

	/* Favorites */

	g_object_unref (priv->favorites);

	/* Disconnect playback signals */

	g_signal_handlers_disconnect_by_func (rena_application_get_backend (priv->rena),
	                                      backend_changed_state_cb, plugin);

	rena_lastfm_disconnect (plugin);

	/* Settings */

	preferences = rena_application_get_preferences (priv->rena);
	plugin_group = rena_preferences_get_plugin_group_name (preferences, "lastfm");
	if (!rena_plugins_engine_is_shutdown(rena_application_get_plugins_engine(priv->rena))) {
		rena_preferences_remove_group (preferences, plugin_group);
	}
	g_free (plugin_group);

	/* Remove menu and settings */

	rena_menubar_remove_lastfm (plugin);
	rena_lastfm_plugin_remove_setting (plugin);

	/* Clean */

	if (priv->updated_mobj)
		g_object_unref (priv->updated_mobj);
	if (priv->current_mobj)
		g_object_unref (priv->current_mobj);
	g_mutex_clear (&priv->data_mutex);
}
