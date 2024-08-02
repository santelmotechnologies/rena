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
#include "src/rena-window.h"
#include "src/rena-background-task-bar.h"
#include "src/rena-background-task-widget.h"
#include "src/xml_helper.h"

#include "plugins/rena-plugin-macros.h"

#define RENA_TYPE_TUNEIN_PLUGIN         (rena_tunein_plugin_get_type ())
#define RENA_TUNEIN_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), RENA_TYPE_TUNEIN_PLUGIN, RenaTuneinPlugin))
#define RENA_TUNEIN_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), RENA_TYPE_TUNEIN_PLUGIN, RenaTuneinPlugin))
#define RENA_IS_TUNEIN_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), RENA_TYPE_TUNEIN_PLUGIN))
#define RENA_IS_TUNEIN_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), RENA_TYPE_TUNEIN_PLUGIN))
#define RENA_TUNEIN_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), RENA_TYPE_TUNEIN_PLUGIN, RenaTuneinPluginClass))

struct _RenaTuneinPluginPrivate {
	RenaApplication          *rena;

	RenaBackgroundTaskWidget *task_widget;
	GtkWidget                  *name_entry;
	GtkActionGroup             *action_group_main_menu;
	guint                       merge_id_main_menu;
};

typedef struct _RenaTuneinPluginPrivate RenaTuneinPluginPrivate;

RENA_PLUGIN_REGISTER (RENA_TYPE_TUNEIN_PLUGIN,
                        RenaTuneinPlugin,
                        rena_tunein_plugin)

/*
 * Prototypes
 */
static void rena_tunein_get_radio_dialog        (RenaTuneinPlugin *plugin);

/*
 * Popups
 */

static void
rena_tunein_plugin_get_radio_action (GtkAction *action, RenaTuneinPlugin *plugin)
{
	rena_tunein_get_radio_dialog (plugin);
}

static void
rena_gmenu_tunein_plugin_get_radio_action (GSimpleAction *action,
                                             GVariant      *parameter,
                                             gpointer       user_data)
{
	rena_tunein_get_radio_dialog (RENA_TUNEIN_PLUGIN(user_data));
}

static const GtkActionEntry main_menu_actions [] = {
	{"Search tunein", NULL, N_("Search radio on TuneIn"),
	 "", "Search tunein", G_CALLBACK(rena_tunein_plugin_get_radio_action)}
};

static const gchar *main_menu_xml = "<ui>						\
	<menubar name=\"Menubar\">									\
		<menu action=\"ToolsMenu\">								\
			<placeholder name=\"rena-plugins-placeholder\">	\
				<menuitem action=\"Search tunein\"/>			\
				<separator/>									\
			</placeholder>										\
		</menu>													\
	</menubar>													\
</ui>";

/*
 * TuneIn Handlers
 */
static const gchar *
tunein_helper_get_atribute (XMLNode *xml, const gchar *atribute)
{
	XMLNode *xi;

	xi = xmlnode_get (xml,CCA {"outline", NULL}, atribute, NULL);

	if (xi)
		return xi->content;

	return NULL;
}

static void
rena_tunein_plugin_get_radio_done (SoupSession *session,
                                     SoupMessage *msg,
                                     gpointer     user_data)
{
	RenaAppNotification *notification;
	RenaPlaylist *playlist;
	RenaBackgroundTaskBar *taskbar;
	RenaDatabase *cdbase;
	RenaMusicobject *mobj = NULL;
	XMLNode *xml = NULL, *xi;
	const gchar *type = NULL, *name = NULL, *url = NULL;
	gchar *uri_parsed, *name_fixed = NULL;

	RenaTuneinPlugin *plugin = user_data;
	RenaTuneinPluginPrivate *priv = plugin->priv;

	taskbar = rena_background_task_bar_get ();
	rena_background_task_bar_remove_widget (taskbar, GTK_WIDGET(priv->task_widget));
	g_object_unref(G_OBJECT(taskbar));

	if (!SOUP_STATUS_IS_SUCCESSFUL (msg->status_code)) {
		notification = rena_app_notification_new ("TuneIn", _("There was an error when searching radio on TuneIn"));
		rena_app_notification_show (notification);
		return;
	}

	xml = tinycxml_parse ((gchar *)msg->response_body->data);
	xi = xmlnode_get (xml, CCA{"opml", "body", "outline", NULL }, NULL, NULL);
	for(;xi;xi= xi->next) {
		type = tunein_helper_get_atribute (xi, "type");
		if (g_ascii_strcasecmp(type, "audio") == 0)
			break;
	}

	if (xi == NULL) {
		notification = rena_app_notification_new ("TuneIn", _("Radio was not found"));
		rena_app_notification_show (notification);
		xmlnode_free(xml);
		return;
	}

	name = tunein_helper_get_atribute (xi, "text");
	url = tunein_helper_get_atribute (xi, "URL");

	if (string_is_empty(name) || string_is_empty(url)) {
		notification = rena_app_notification_new ("TuneIn", _("There was an error when searching radio on TuneIn"));
		rena_app_notification_show (notification);
		xmlnode_free(xml);
		return;
	}

	name_fixed = unescape_HTML (name);
	uri_parsed = rena_pl_get_first_playlist_item (url);

	mobj = new_musicobject_from_location (uri_parsed, name_fixed);

	playlist = rena_application_get_playlist (priv->rena);
	rena_playlist_append_single_song (playlist, mobj);
	new_radio (playlist, uri_parsed, name_fixed);

	cdbase = rena_application_get_database (priv->rena);
	rena_database_change_playlists_done (cdbase);

	xmlnode_free(xml);

	g_free (name_fixed);
	g_free (uri_parsed);
}

static void
rena_tunein_plugin_get_radio (RenaTuneinPlugin *plugin, const gchar *field)
{
	RenaBackgroundTaskBar *taskbar;
	SoupSession *session;
	SoupMessage *msg;
	gchar *escaped_field = NULL, *query = NULL;

	RenaTuneinPluginPrivate *priv = plugin->priv;

	priv->task_widget = rena_background_task_widget_new (_("Searching radio on TuneIn"),
	                                                       "edit-find",
	                                                       0,
	                                                       NULL);

	taskbar = rena_background_task_bar_get ();
	rena_background_task_bar_prepend_widget (taskbar, GTK_WIDGET(priv->task_widget));
	g_object_unref(G_OBJECT(taskbar));

	escaped_field = g_uri_escape_string (field, NULL, TRUE);
	query = g_strdup_printf ("%s%s", "http://opml.radiotime.com/Search.aspx?query=", escaped_field);

	session = soup_session_new ();

	msg = soup_message_new ("GET", query);
	soup_session_queue_message (session, msg,
	                            rena_tunein_plugin_get_radio_done, plugin);

	g_free (escaped_field);
	g_free (query);
}

/*
 * TuneIn dialog
 */

static void
rena_tunein_dialog_response (GtkWidget          *dialog,
                               gint                response_id,
                               RenaTuneinPlugin *plugin)
{
	RenaTuneinPluginPrivate *priv = plugin->priv;

	switch (response_id) {
		case GTK_RESPONSE_ACCEPT:
			rena_tunein_plugin_get_radio (plugin, gtk_entry_get_text(GTK_ENTRY(priv->name_entry)));
			break;
		case GTK_RESPONSE_CANCEL:
		default:
			break;
	}

	priv->name_entry = NULL;
	gtk_widget_destroy (dialog);
}

static void
rena_tunein_get_radio_dialog (RenaTuneinPlugin *plugin)
{
	GtkWidget *dialog, *parent;
	GtkWidget *table, *entry;
	guint row = 0;

	RenaTuneinPluginPrivate *priv = plugin->priv;

	parent = rena_application_get_window (priv->rena);
	dialog = gtk_dialog_new_with_buttons (_("Search in TuneIn"),
	                                      GTK_WINDOW(parent),
	                                      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	                                      _("_Cancel"), GTK_RESPONSE_CANCEL,
	                                      _("_Ok"), GTK_RESPONSE_ACCEPT,
	                                      NULL);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

	table = rena_hig_workarea_table_new ();

	rena_hig_workarea_table_add_section_title (table, &row, _("Search in TuneIn"));

	entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY(entry), 255);
	gtk_entry_set_activates_default (GTK_ENTRY(entry), TRUE);
	priv->name_entry = entry;

	rena_hig_workarea_table_add_wide_control (table, &row, entry);

	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), table);

	g_signal_connect (G_OBJECT (dialog), "response",
	                  G_CALLBACK (rena_tunein_dialog_response), plugin);
	gtk_widget_show_all (dialog);
}

/*
 * TuneIn plugin
 */
static void
rena_plugin_activate (PeasActivatable *activatable)
{
	GMenuItem *item;
	GSimpleAction *action;

	RenaTuneinPlugin *plugin = RENA_TUNEIN_PLUGIN (activatable);

	RenaTuneinPluginPrivate *priv = plugin->priv;
	priv->rena = g_object_get_data (G_OBJECT (plugin), "object");

	CDEBUG(DBG_PLUGIN, "TuneIn plugin %s", G_STRFUNC);

	/* Attach main menu */

	priv->action_group_main_menu = rena_menubar_plugin_action_new ("RenaTuneinPlugin",
	                                                                 main_menu_actions,
	                                                                 G_N_ELEMENTS (main_menu_actions),
	                                                                 NULL,
	                                                                 0,
	                                                                 plugin);

	priv->merge_id_main_menu = rena_menubar_append_plugin_action (priv->rena,
	                                                                priv->action_group_main_menu,
	                                                                main_menu_xml);

	/* Gear Menu */

	action = g_simple_action_new ("search-tunein", NULL);
	g_signal_connect (G_OBJECT (action), "activate",
	                  G_CALLBACK (rena_gmenu_tunein_plugin_get_radio_action), plugin);

	item = g_menu_item_new (_("Search radio on TuneIn"), "win.search-tunein");
	rena_menubar_append_action (priv->rena, "rena-plugins-placeholder", action, item);
	g_object_unref (item);
}

static void
rena_plugin_deactivate (PeasActivatable *activatable)
{
	RenaTuneinPlugin *plugin = RENA_TUNEIN_PLUGIN (activatable);
	RenaTuneinPluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "TuneIn plugin %s", G_STRFUNC);

	rena_menubar_remove_plugin_action (priv->rena,
	                                     priv->action_group_main_menu,
	                                     priv->merge_id_main_menu);
	priv->merge_id_main_menu = 0;

	rena_menubar_remove_action (priv->rena, "rena-plugins-placeholder", "search-tunein");
}
