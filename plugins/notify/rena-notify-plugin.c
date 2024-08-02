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
#include <libpeas-gtk/peas-gtk.h>

#include <libnotify/notify.h>

#include "plugins/rena-plugin-macros.h"

#include "src/rena.h"
#include "src/rena-hig.h"
#include "src/rena-playback.h"
#include "src/rena-utils.h"
#include "src/rena-preferences-dialog.h"

#define RENA_TYPE_NOTIFY_PLUGIN         (rena_notify_plugin_get_type ())
#define RENA_NOTIFY_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), RENA_TYPE_NOTIFY_PLUGIN, RenaNotifyPlugin))
#define RENA_NOTIFY_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), RENA_TYPE_NOTIFY_PLUGIN, RenaNotifyPlugin))
#define RENA_IS_NOTIFY_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), RENA_TYPE_NOTIFY_PLUGIN))
#define RENA_IS_NOTIFY_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), RENA_TYPE_NOTIFY_PLUGIN))
#define RENA_NOTIFY_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), RENA_TYPE_NOTIFY_PLUGIN, RenaNotifyPluginClass))

typedef struct {
	RenaApplication  *rena;
	GtkWidget          *setting_widget;

	NotifyNotification *notify;

	GtkWidget          *album_art_in_osd_w;
	GtkWidget          *actions_in_osd_w;
	gboolean            album_art_in_osd;
	gboolean            actions_in_osd;
} RenaNotifyPluginPrivate;

RENA_PLUGIN_REGISTER (RENA_TYPE_NOTIFY_PLUGIN,
                        RenaNotifyPlugin,
                        rena_notify_plugin)

static gboolean
can_support_actions (void)
{
	static gboolean supported;
	static gboolean have_checked = FALSE;

	if( !have_checked ){
		GList * c;
		GList * caps = notify_get_server_caps( );

		have_checked = TRUE;

		for( c=caps; c && !supported; c=c->next )
			supported = !strcmp( "actions", (char*)c->data );

		g_list_free_full( caps, g_free );
	}

	return supported;
}

static void
notify_closed_cb (NotifyNotification *osd,
                  RenaNotifyPlugin *plugin)
{
	g_object_unref (G_OBJECT(osd));

	if (plugin->priv->notify == osd) {
		plugin->priv->notify = NULL;
	}
}

static void
notify_Prev_Callback (NotifyNotification *osd,
                      const char *action,
                      RenaNotifyPlugin *plugin)
{
	RenaBackend *backend;

	g_assert (action != NULL);

	RenaApplication *rena = plugin->priv->rena;

	backend = rena_application_get_backend (rena);
	if (rena_backend_emitted_error (backend) == FALSE)
		rena_playback_prev_track(rena);
}

static void
notify_Next_Callback (NotifyNotification *osd,
                      const char         *action,
                      RenaNotifyPlugin *plugin)
{
	RenaBackend *backend;

	g_assert (action != NULL);

	RenaApplication *rena = plugin->priv->rena;

	backend = rena_application_get_backend (rena);
	if (rena_backend_emitted_error (backend) == FALSE)
		rena_playback_next_track(rena);
}

void
rena_notify_plugin_show_new_track (RenaPlaylist     *playlist,
                                     RenaMusicobject  *mobj,
                                     RenaNotifyPlugin *plugin)
{
	RenaNotifyPluginPrivate *priv = NULL;
	RenaToolbar *toolbar;
	gchar *summary, *body, *slength;
	GError *error = NULL;

	priv = plugin->priv;

	if (NULL == mobj)
		return;

	if (gtk_window_is_active(GTK_WINDOW (rena_application_get_window(priv->rena))))
		return;

	const gchar *file = rena_musicobject_get_file (mobj);
	const gchar *title = rena_musicobject_get_title (mobj);
	const gchar *artist = rena_musicobject_get_artist (mobj);
	const gchar *album = rena_musicobject_get_album (mobj);
	gint length = rena_musicobject_get_length (mobj);

	if(string_is_not_empty(title))
		summary = g_strdup(title);
	else
		summary = g_path_get_basename(file);

	slength = convert_length_str(length);

	body = g_markup_printf_escaped(_("by <b>%s</b> in <b>%s</b> <b>(%s)</b>"),
	                               string_is_not_empty(artist) ? artist : _("Unknown Artist"),
	                               string_is_not_empty(album) ? album : _("Unknown Album"),
	                               slength);

	/* Create notification instance */

	if (priv->notify == NULL) {
		priv->notify = notify_notification_new(summary, body, NULL);

		if (can_support_actions() && priv->actions_in_osd) {
			notify_notification_add_action(
				priv->notify, "media-skip-backward", _("Previous track"),
				NOTIFY_ACTION_CALLBACK(notify_Prev_Callback), plugin,
				NULL);
			notify_notification_add_action(
				priv->notify, "media-skip-forward", _("Next track"),
				NOTIFY_ACTION_CALLBACK(notify_Next_Callback), plugin,
				NULL);
		}
		notify_notification_set_hint (priv->notify, "transient", g_variant_new_boolean (TRUE));
		g_signal_connect (priv->notify, "closed", G_CALLBACK (notify_closed_cb), plugin);
	}
	else {
		notify_notification_update (priv->notify, summary, body, NULL);

		if (!priv->actions_in_osd)
			notify_notification_clear_actions (priv->notify);
	}

	/* Add album art if set */
	if (priv->album_art_in_osd) {
		toolbar = rena_application_get_toolbar (priv->rena);
		notify_notification_set_image_from_pixbuf (priv->notify,
			rena_album_art_get_pixbuf (rena_toolbar_get_album_art(toolbar)));
	}

	/* Show OSD */
	if (!notify_notification_show (priv->notify, &error)) {
		g_warning("Unable to show OSD notification: %s", error->message);
		g_error_free (error);
	}

	/* Cleanup */

	g_free(summary);
	g_free(body);
	g_free(slength);
}

static void
rena_notify_preferences_dialog_response (GtkDialog          *dialog,
                                           gint                response_id,
                                           RenaNotifyPlugin *plugin)
{
	RenaPreferences *preferences;
	gchar *plugin_group = NULL;

	RenaNotifyPluginPrivate *priv = plugin->priv;

	switch(response_id) {
		case GTK_RESPONSE_CANCEL:
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(priv->album_art_in_osd_w),
			                              priv->album_art_in_osd);
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(priv->actions_in_osd_w),
			                              priv->actions_in_osd);
			break;
		case GTK_RESPONSE_OK:
			priv->album_art_in_osd =
				gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->album_art_in_osd_w));
			priv->actions_in_osd =
				gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->actions_in_osd_w));

			preferences = rena_preferences_get ();
			plugin_group = rena_preferences_get_plugin_group_name(preferences, "notify");

			rena_preferences_set_boolean (preferences,
			                                plugin_group, "album_art_in_osd",
			                                priv->album_art_in_osd);
			rena_preferences_set_boolean (preferences,
			                                plugin_group, "actions_in_osd",
			                                priv->actions_in_osd);

			g_object_unref (preferences);
			g_free (plugin_group);
			break;
		default:
			break;
	}
}

static void
rena_notify_plugin_append_setting (RenaNotifyPlugin *plugin)
{
	RenaPreferencesDialog *dialog;
	GtkWidget *table, *albumart_in_osd, *actions_in_osd;
	guint row = 0;

	RenaNotifyPluginPrivate *priv = plugin->priv;

	table = rena_hig_workarea_table_new ();

	rena_hig_workarea_table_add_section_title(table, &row, _("Notifications"));

	albumart_in_osd = gtk_check_button_new_with_label(_("Show Album art in notifications"));
	rena_hig_workarea_table_add_wide_control(table, &row, albumart_in_osd);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(albumart_in_osd), priv->album_art_in_osd);

	actions_in_osd = gtk_check_button_new_with_label(_("Add actions to change track in notifications"));
	rena_hig_workarea_table_add_wide_control(table, &row, actions_in_osd);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(actions_in_osd), priv->actions_in_osd);

	if (!can_support_actions())
		gtk_widget_set_sensitive (actions_in_osd, FALSE);

	priv->setting_widget = table;
	priv->album_art_in_osd_w = albumart_in_osd;
	priv->actions_in_osd_w = actions_in_osd;

	dialog = rena_application_get_preferences_dialog (priv->rena);
	rena_preferences_append_desktop_setting (dialog, table, FALSE);

	/* Configure handler and settings */
	rena_preferences_dialog_connect_handler (dialog,
	                                           G_CALLBACK(rena_notify_preferences_dialog_response),
	                                           plugin);
}

static void
rena_notify_plugin_remove_setting (RenaNotifyPlugin *plugin)
{
	RenaPreferencesDialog *dialog;
	RenaNotifyPluginPrivate *priv = plugin->priv;

	dialog = rena_application_get_preferences_dialog (priv->rena);

	rena_preferences_dialog_disconnect_handler (dialog,
	                                              G_CALLBACK(rena_notify_preferences_dialog_response),
	                                              plugin);
	rena_preferences_remove_desktop_setting (dialog, priv->setting_widget);
}

static void
rena_plugin_activate (PeasActivatable *activatable)
{
	RenaPreferences *preferences;
	RenaPlaylist *playlist;
	gchar *plugin_group = NULL;

	RenaNotifyPlugin *plugin = RENA_NOTIFY_PLUGIN (activatable);
	RenaNotifyPluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "Notify plugin %s", G_STRFUNC);

	priv->rena = g_object_get_data (G_OBJECT (plugin), "object");

	notify_init (PACKAGE_NAME);

	preferences = rena_application_get_preferences (priv->rena);
	plugin_group = rena_preferences_get_plugin_group_name (preferences, "notify");
	if (rena_preferences_has_group (preferences, plugin_group)) {
		priv->actions_in_osd =
			rena_preferences_get_boolean (preferences,
			                                plugin_group,
			                                "actions_in_osd");
		priv->album_art_in_osd =
			rena_preferences_get_boolean (preferences,
			                                plugin_group,
			                                "album_art_in_osd");
	}
	else {
		priv->actions_in_osd = TRUE;
		priv->album_art_in_osd = TRUE;
	}

	/* Fix for nofify-osd users */
	if (!can_support_actions())
		priv->actions_in_osd = FALSE;

	playlist = rena_application_get_playlist (priv->rena);
	g_signal_connect (playlist, "playlist-set-track",
	                  G_CALLBACK(rena_notify_plugin_show_new_track), plugin);

	rena_notify_plugin_append_setting (plugin);

	g_free (plugin_group);
}

static void
rena_plugin_deactivate (PeasActivatable *activatable)
{
	RenaPlaylist *playlist;

	RenaNotifyPlugin *plugin = RENA_NOTIFY_PLUGIN (activatable);
	RenaNotifyPluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "Notify plugin %s", G_STRFUNC);

	notify_uninit ();

	playlist = rena_application_get_playlist (priv->rena);
	g_signal_handlers_disconnect_by_func (playlist,
	                                      rena_notify_plugin_show_new_track,
	                                      plugin);

	rena_notify_plugin_remove_setting (plugin);

	priv->rena= NULL;
}
