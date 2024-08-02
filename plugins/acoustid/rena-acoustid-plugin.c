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

#include <gst/gst.h>

#include <libsoup/soup.h>

#include <libpeas/peas.h>

#include "src/rena.h"
#include "src/rena-app-notification.h"
#include "src/rena-menubar.h"
#include "src/rena-playlist.h"
#include "src/rena-playlists-mgmt.h"
#include "src/rena-musicobject-mgmt.h"
#include "src/rena-hig.h"
#include "src/rena-utils.h"
#include "src/xml_helper.h"
#include "src/rena-window.h"
#include "src/rena-tagger.h"
#include "src/rena-tags-dialog.h"
#include "src/rena-background-task-bar.h"
#include "src/rena-background-task-widget.h"

#include "plugins/rena-plugin-macros.h"

#define RENA_TYPE_ACOUSTID_PLUGIN         (rena_acoustid_plugin_get_type ())
#define RENA_ACOUSTID_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), RENA_TYPE_ACOUSTID_PLUGIN, RenaAcoustidPlugin))
#define RENA_ACOUSTID_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), RENA_TYPE_ACOUSTID_PLUGIN, RenaAcoustidPlugin))
#define RENA_IS_ACOUSTID_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), RENA_TYPE_ACOUSTID_PLUGIN))
#define RENA_IS_ACOUSTID_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), RENA_TYPE_ACOUSTID_PLUGIN))
#define RENA_ACOUSTID_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), RENA_TYPE_ACOUSTID_PLUGIN, RenaAcoustidPluginClass))

struct _RenaAcoustidPluginPrivate {
	RenaApplication          *rena;

	RenaMusicobject          *mobj;

	RenaBackgroundTaskWidget *task_widget;

	GtkActionGroup             *action_group_main_menu;
	guint                       merge_id_main_menu;
};
typedef struct _RenaAcoustidPluginPrivate RenaAcoustidPluginPrivate;

RENA_PLUGIN_REGISTER (RENA_TYPE_ACOUSTID_PLUGIN,
                        RenaAcoustidPlugin,
                        rena_acoustid_plugin)

/*
 * Prototypes
 */
static void rena_acoustid_get_metadata_dialog (RenaAcoustidPlugin *plugin);

/*
 * Popups
 */
static void
rena_acoustid_plugin_get_metadata_action (GtkAction *action, RenaAcoustidPlugin *plugin)
{
	RenaBackend *backend;

	RenaAcoustidPluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "Get Metadata action");

	backend = rena_application_get_backend (priv->rena);
	if (rena_backend_get_state (backend) == ST_STOPPED)
		return;

	rena_acoustid_get_metadata_dialog (plugin);
}

static const GtkActionEntry main_menu_actions [] = {
	{"Search metadata", NULL, N_("Search tags on AcoustID"),
	 "", "Search metadata", G_CALLBACK(rena_acoustid_plugin_get_metadata_action)}
};

static const gchar *main_menu_xml = "<ui>						\
	<menubar name=\"Menubar\">									\
		<menu action=\"ToolsMenu\">								\
			<placeholder name=\"rena-plugins-placeholder\">	\
				<menuitem action=\"Search metadata\"/>			\
				<separator/>									\
			</placeholder>										\
		</menu>													\
	</menubar>													\
</ui>";

/*
 * Gear menu.
 */

static void
rena_gmenu_search_metadata_action (GSimpleAction *action,
                                     GVariant      *parameter,
                                     gpointer       user_data)
{
	rena_acoustid_plugin_get_metadata_action (NULL, RENA_ACOUSTID_PLUGIN(user_data));
}

/*
 * AcoustID Handlers
 */

static void
Rena_acoustid_dialog_response (GtkWidget            *dialog,
                                 gint                  response_id,
                                 RenaAcoustidPlugin *plugin)
{
	RenaBackend *backend;
	RenaPlaylist *playlist;
	RenaToolbar *toolbar;
	RenaMusicobject *nmobj, *current_mobj;
	RenaTagger *tagger;
	gint changed = 0;

	RenaAcoustidPluginPrivate *priv = plugin->priv;

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
				if (rena_musicobject_compare (nmobj, current_mobj) == 0) {
					toolbar = rena_application_get_toolbar (priv->rena);

					/* Update public current song */
					rena_update_musicobject_change_tag (current_mobj, changed, nmobj);

					/* Update current song on playlist */
					playlist = rena_application_get_playlist (priv->rena);
					rena_playlist_update_current_track (playlist, changed, nmobj);

					rena_toolbar_set_title(toolbar, current_mobj);
				}
			}

			if (G_LIKELY(rena_musicobject_is_local_file (nmobj))) {
				tagger = rena_tagger_new();
				rena_tagger_add_file (tagger, rena_musicobject_get_file(nmobj));
				rena_tagger_set_changes (tagger, nmobj, changed);
				rena_tagger_apply_changes (tagger);
				g_object_unref(tagger);
			}
		}
	}

	gtk_widget_destroy (dialog);
}

static void
rena_acoustid_plugin_get_metadata_done (SoupSession *session,
                                          SoupMessage *msg,
                                          gpointer     user_data)
{
	GtkWidget *dialog;
	RenaAppNotification *notification;
	RenaBackgroundTaskBar *taskbar;
	XMLNode *xml = NULL, *xi;
	gchar *otitle = NULL, *oartist = NULL, *oalbum = NULL;
	gchar *ntitle = NULL, *nartist = NULL, *nalbum = NULL;
	gint prechanged = 0;

	RenaAcoustidPlugin *plugin = user_data;
	RenaAcoustidPluginPrivate *priv = plugin->priv;

	taskbar = rena_background_task_bar_get ();
	rena_background_task_bar_remove_widget (taskbar, GTK_WIDGET(priv->task_widget));
	g_object_unref(G_OBJECT(taskbar));

	if (!SOUP_STATUS_IS_SUCCESSFUL (msg->status_code)) {
		notification = rena_app_notification_new ("AcoustID", _("There was an error when searching your tags on AcoustID"));
		rena_app_notification_show (notification);
		return;
	}

	g_object_get (priv->mobj,
	              "title", &otitle,
	              "artist", &oartist,
	              "album", &oalbum,
	              NULL);

	xml = tinycxml_parse ((gchar *)msg->response_body->data);

	xi = xmlnode_get (xml, CCA{"response", "results", "result", "recordings", "recording", "title", NULL }, NULL, NULL);
	if (xi && string_is_not_empty(xi->content)) {
		ntitle = unescape_HTML (xi->content);
		if (g_strcmp0(otitle, ntitle)) {
			rena_musicobject_set_title (priv->mobj, ntitle);
			prechanged |= TAG_TITLE_CHANGED;
		}
		g_free (ntitle);
	}

	xi = xmlnode_get (xml, CCA{"response", "results", "result", "recordings", "recording", "artists", "artist", "name", NULL }, NULL, NULL);
	if (xi && string_is_not_empty(xi->content)) {
		nartist = unescape_HTML (xi->content);
		if (g_strcmp0(oartist, nartist)) {
			rena_musicobject_set_artist (priv->mobj, nartist);
			prechanged |= TAG_ARTIST_CHANGED;
		}
		g_free (nartist);
	}

	xi = xmlnode_get (xml, CCA{"response", "results", "result", "recordings", "recording", "releasegroups", "releasegroup", "title", NULL }, NULL, NULL);
	if (xi && string_is_not_empty(xi->content)) {
		nalbum = unescape_HTML (xi->content);
		if (g_strcmp0(oalbum, nalbum)) {
			rena_musicobject_set_album (priv->mobj, nalbum);
			prechanged |= TAG_ALBUM_CHANGED;
		}
		g_free (nalbum);
	}

	if (prechanged)	{
		dialog = rena_tags_dialog_new ();
		gtk_window_set_transient_for (GTK_WINDOW(dialog),
			GTK_WINDOW(rena_application_get_window(priv->rena)));

		g_signal_connect (G_OBJECT (dialog), "response",
		                  G_CALLBACK (rena_acoustid_dialog_response), plugin);

		rena_tags_dialog_set_musicobject (RENA_TAGS_DIALOG(dialog), priv->mobj);
		rena_tags_dialog_set_changed (RENA_TAGS_DIALOG(dialog), prechanged);

		gtk_widget_show (dialog);
	}
	else {
		notification = rena_app_notification_new ("AcoustID", _("AcoustID not found any similar song"));
		rena_app_notification_show (notification);
	}

	g_free (otitle);
	g_free (oartist);
	g_free (oalbum);

	g_object_unref (priv->mobj);
	xmlnode_free (xml);
}

static void
rena_acoustid_plugin_get_metadata (RenaAcoustidPlugin *plugin, gint duration, const gchar *fingerprint)
{
	SoupSession *session;
	SoupMessage *msg;
	gchar *query = NULL;

	query = g_strdup_printf ("http://api.acoustid.org/v2/lookup?client=%s&meta=%s&format=%s&duration=%d&fingerprint=%s",
	                         "yPvUXBmO", "recordings+releasegroups+compress", "xml", duration, fingerprint);

	session = soup_session_new ();

	msg = soup_message_new ("GET", query);
	soup_session_queue_message (session, msg,
	                            rena_acoustid_plugin_get_metadata_done, plugin);

	g_free (query);
}

static void
error_cb (GstBus *bus, GstMessage *msg, void *data)
{
	GError *err;
	gchar *debug_info;

	/* Print error details on the screen */
	gst_message_parse_error (msg, &err, &debug_info);
	g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
	g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
	g_clear_error (&err);
	g_free (debug_info);
}

static gboolean
rena_acoustid_get_fingerprint (const gchar *filename, gchar **fingerprint)
{
	GstElement *pipeline, *chromaprint;
	GstBus *bus;
	GstMessage *msg;
	gchar *uri, *pipestring = NULL;

	uri = g_filename_to_uri(filename, NULL, NULL);
	pipestring = g_strdup_printf("uridecodebin uri=%s ! audioconvert ! chromaprint name=chromaprint0 ! fakesink", uri);
	g_free (uri);

	pipeline = gst_parse_launch (pipestring, NULL);

	bus = gst_element_get_bus (pipeline);
	g_signal_connect (G_OBJECT (bus), "message::error", (GCallback)error_cb, NULL);

	gst_element_set_state (pipeline, GST_STATE_PLAYING);

	msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
	if (msg != NULL)
		gst_message_unref (msg);
	gst_object_unref (bus);

	gst_element_set_state (pipeline, GST_STATE_NULL);

	chromaprint = gst_bin_get_by_name (GST_BIN(pipeline), "chromaprint0");
	g_object_get (chromaprint, "fingerprint", fingerprint, NULL);

	gst_object_unref (pipeline);
	g_free (pipestring);

	return TRUE;
}

/*
 * AcoustID dialog
 */
static void
rena_acoustid_get_metadata_dialog (RenaAcoustidPlugin *plugin)
{
	RenaAppNotification *notification;
	RenaBackend *backend = NULL;
	RenaMusicobject *mobj = NULL;
	RenaBackgroundTaskBar *taskbar;
	const gchar *file = NULL;
	gchar *fingerprint = NULL;
	gint duration = 0;

	RenaAcoustidPluginPrivate *priv = plugin->priv;

	backend = rena_application_get_backend (priv->rena);
	mobj = rena_backend_get_musicobject (backend);

	priv->mobj = rena_musicobject_dup (mobj);

	file = rena_musicobject_get_file (mobj);
	duration = rena_musicobject_get_length (mobj);

	priv->task_widget = rena_background_task_widget_new (_("Searching tags on AcoustID"),
	                                                      "edit-find",
	                                                      0,
	                                                      NULL);
	g_object_unref (G_OBJECT(priv->task_widget));

	taskbar = rena_background_task_bar_get ();
	rena_background_task_bar_prepend_widget (taskbar, GTK_WIDGET(priv->task_widget));
	g_object_unref(G_OBJECT(taskbar));

	if (rena_acoustid_get_fingerprint (file, &fingerprint))
		rena_acoustid_plugin_get_metadata (plugin, duration, fingerprint);
	else {
		taskbar = rena_background_task_bar_get ();
		rena_background_task_bar_remove_widget (taskbar, GTK_WIDGET(priv->task_widget));
		g_object_unref(G_OBJECT(taskbar));
		priv->task_widget = NULL;

		notification = rena_app_notification_new ("AcoustID", _("There was an error when searching your tags on AcoustID"));
		rena_app_notification_show (notification);
	}

	g_free (fingerprint);
}

static void
backend_changed_state_cb (RenaBackend *backend, GParamSpec *pspec, gpointer user_data)
{
	GtkWindow *window;
	GtkAction *action;
	RenaBackendState state = ST_STOPPED;

	RenaAcoustidPlugin *plugin = user_data;
	RenaAcoustidPluginPrivate *priv = plugin->priv;

	state = rena_backend_get_state (backend);

	action = gtk_action_group_get_action (priv->action_group_main_menu, "Search metadata");
	gtk_action_set_sensitive (action, state != ST_STOPPED);

	window = GTK_WINDOW(rena_application_get_window(priv->rena));
	rena_menubar_set_enable_action (window, "search-metadata", state != ST_STOPPED);
}

/*
 * AcoustID plugin
 */
static void
rena_plugin_activate (PeasActivatable *activatable)
{
	GMenuItem *item;
	GSimpleAction *action;

	RenaAcoustidPlugin *plugin = RENA_ACOUSTID_PLUGIN (activatable);

	RenaAcoustidPluginPrivate *priv = plugin->priv;
	priv->rena = g_object_get_data (G_OBJECT (plugin), "object");

	CDEBUG(DBG_PLUGIN, "AcustId plugin %s", G_STRFUNC);

	/* Attach main menu */

	priv->action_group_main_menu = rena_menubar_plugin_action_new ("RenaAcoustidPlugin",
	                                                                 main_menu_actions,
	                                                                 G_N_ELEMENTS (main_menu_actions),
	                                                                 NULL,
	                                                                 0,
	                                                                 plugin);

	priv->merge_id_main_menu = rena_menubar_append_plugin_action (priv->rena,
	                                                                priv->action_group_main_menu,
	                                                                main_menu_xml);
	/* Gear Menu */

	action = g_simple_action_new ("search-metadata", NULL);
	g_signal_connect (G_OBJECT (action), "activate",
	                  G_CALLBACK (rena_gmenu_search_metadata_action), plugin);

	item = g_menu_item_new (_("Search tags on AcoustID"), "win.search-metadata");
	rena_menubar_append_action (priv->rena, "rena-plugins-placeholder", action, item);
	g_object_unref (item);

	/* Connect playback signals */

	g_signal_connect (rena_application_get_backend (priv->rena), "notify::state",
	                  G_CALLBACK (backend_changed_state_cb), plugin);
	backend_changed_state_cb (rena_application_get_backend (priv->rena), NULL, plugin);
}

static void
rena_plugin_deactivate (PeasActivatable *activatable)
{
	RenaAcoustidPlugin *plugin = RENA_ACOUSTID_PLUGIN (activatable);
	RenaAcoustidPluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "AcustID plugin %s", G_STRFUNC);

	/* Disconnect playback signals */

	g_signal_handlers_disconnect_by_func (rena_application_get_backend (priv->rena),
	                                      backend_changed_state_cb, plugin);

	/* Remove menu actions */

	rena_menubar_remove_plugin_action (priv->rena,
	                                     priv->action_group_main_menu,
	                                     priv->merge_id_main_menu);
	priv->merge_id_main_menu = 0;

	rena_menubar_remove_action (priv->rena, "rena-plugins-placeholder", "search-metadata");
}
