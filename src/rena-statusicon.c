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

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "rena-statusicon.h"

#if defined(GETTEXT_PACKAGE)
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#include "rena-playback.h"
#include "rena-menubar.h"
#include "rena-window.h"
#include "rena.h"

#include "rena-window-ui.h"

struct _RenaStatusIcon {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
	GtkStatusIcon     __parent__;
G_GNUC_END_IGNORE_DEPRECATIONS

	RenaApplication  *rena;

	GtkBuilder         *builder;
	GSimpleActionGroup *actions;
};

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
G_DEFINE_TYPE(RenaStatusIcon, rena_status_icon, GTK_TYPE_STATUS_ICON)
G_GNUC_END_IGNORE_DEPRECATIONS


static void
rena_systray_gmenu_about (GSimpleAction *action,
                            GVariant      *parameter,
                            gpointer       user_data);

static void
rena_systray_gmenu_open (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       user_data);

static void
rena_systray_gmenu_location (GSimpleAction *action,
                               GVariant      *parameter,
                               gpointer       user_data);

static void
rena_systray_gmenu_playpause (GSimpleAction *action,
                                GVariant      *parameter,
                                gpointer       user_data);

static void
rena_systray_gmenu_stop (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       user_data);

static void
rena_systray_gmenu_prev (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       user_data);

static void
rena_systray_gmenu_next (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       user_data);

static void
rena_systray_gmenu_edit (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       user_data);

static void
rena_systray_gmenu_quit (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       user_data);

static const GActionEntry systray_menu_aentries[] = {
	{ "about",    rena_systray_gmenu_about,     NULL, NULL, NULL },
	{ "open",     rena_systray_gmenu_open,      NULL, NULL, NULL },
	{ "location", rena_systray_gmenu_location,  NULL, NULL, NULL },
	{ "prev",     rena_systray_gmenu_prev,      NULL, NULL, NULL },
	{ "play",     rena_systray_gmenu_playpause, NULL, NULL, NULL },
	{ "stop",     rena_systray_gmenu_stop,      NULL, NULL, NULL },
	{ "next",     rena_systray_gmenu_next,      NULL, NULL, NULL },
	{ "edit",     rena_systray_gmenu_edit,      NULL, NULL, NULL },
	{ "quit",     rena_systray_gmenu_quit,      NULL, NULL, NULL }
};

static GMenu *
rena_systray_get_menu_section (RenaStatusIcon *status_icon,
                                 const char       *id)
{
	GObject *object;
	object = gtk_builder_get_object (status_icon->builder, id);

	if (object == NULL || !G_IS_MENU (object))
		return NULL;

	return G_MENU (object);
}

void
rena_systray_append_action (RenaStatusIcon *status_icon,
                              const gchar      *placeholder,
                              GSimpleAction    *action,
                              GMenuItem        *item)
{
	GMenu *place;

	place = rena_systray_get_menu_section (status_icon, placeholder);

	g_action_map_add_action (G_ACTION_MAP (status_icon->actions), G_ACTION (action));

	g_menu_append_item (G_MENU (place), item);
}


void
rena_systray_remove_action (RenaStatusIcon *status_icon,
                              const gchar      *placeholder,
                              const gchar      *action_name)
{
	GMenu *menu;
	gchar *action;
	gboolean found = FALSE;
	gint i;

	menu = G_MENU (gtk_builder_get_object (status_icon->builder, placeholder));

	for (i = 0; i < g_menu_model_get_n_items (G_MENU_MODEL(menu)); i++)
	{
		if (g_menu_model_get_item_attribute (G_MENU_MODEL(menu), i, G_MENU_ATTRIBUTE_ACTION, "s", &action))
		{
			if (g_strcmp0 (action + strlen ("syst."), action_name) == 0)
			{
				g_menu_remove (G_MENU (menu), i);
				g_action_map_remove_action (G_ACTION_MAP (status_icon->actions), action_name);
				found = TRUE;
			}
			g_free (action);
			if (found)
				break;
		}
	}
}

/*
 * Status Icon
 */

static gboolean
status_icon_clicked (GtkWidget *widget, GdkEventButton *event, RenaStatusIcon *status_icon)
{
	GMenu *menu;
	GtkWidget *popup_menu;

	switch (event->button)
	{
		case 1:
			rena_window_toggle_state (status_icon->rena, FALSE);
			break;
		case 2:
			rena_playback_play_pause_resume (status_icon->rena);
			break;
		case 3:
			menu = G_MENU (gtk_builder_get_object (status_icon->builder, "statusicon-menu"));
			popup_menu = gtk_menu_new_from_model (G_MENU_MODEL(menu));
			gtk_widget_insert_action_group (popup_menu, "syst", G_ACTION_GROUP(status_icon->actions));
			gtk_menu_popup (GTK_MENU(popup_menu), NULL, NULL, NULL, NULL,
			                event->button, gtk_get_current_event_time ());
		default: break;
	}
	
	return TRUE;
}

static gboolean
status_get_tooltip_cb (GtkWidget        *widget,
                       gint              x,
                       gint              y,
                       gboolean          keyboard_mode,
                       GtkTooltip       *tooltip,
                       RenaStatusIcon *status_icon)
{
	RenaBackend *backend;
	RenaToolbar *toolbar;
	RenaMusicobject *mobj;
	gchar *markup_text;

	toolbar = rena_application_get_toolbar (status_icon->rena);

	backend = rena_application_get_backend (status_icon->rena);
	if (rena_backend_get_state (backend) == ST_STOPPED)
		markup_text = g_strdup_printf("%s", _("<b>Not playing</b>"));
	else {
		mobj = rena_backend_get_musicobject (backend);

		markup_text = g_markup_printf_escaped ("<b>%s</b>: %s\n<b>%s</b>: %s\n<b>%s</b>: %s\n<b>%s</b>: %s / %s",
		                                       _("Title"), rena_musicobject_get_title (mobj),
		                                       _("Artist"), rena_musicobject_get_artist (mobj),
		                                       _("Album"), rena_musicobject_get_album (mobj),
		                                       _("Length"), rena_toolbar_get_progress_text (toolbar),
		                                       rena_toolbar_get_length_text (toolbar));
	}

	gtk_tooltip_set_markup (tooltip, markup_text);
	gtk_tooltip_set_icon (tooltip,
		rena_album_art_get_pixbuf(rena_toolbar_get_album_art(toolbar)));

	g_free(markup_text);

	return TRUE;
}

static void
systray_volume_scroll (GtkWidget *widget, GdkEventScroll *event, RenaStatusIcon *status_icon)
{
	RenaBackend *backend;

	if (event->type != GDK_SCROLL)
		return;

	backend = rena_application_get_backend (status_icon->rena);

	switch (event->direction){
		case GDK_SCROLL_UP:
			rena_backend_set_delta_volume (backend, +0.02);
			break;
		case GDK_SCROLL_DOWN:
			rena_backend_set_delta_volume (backend, -0.02);
			break;
		default:
			break;
	}
}

static void
rena_systray_gmenu_about (GSimpleAction *action,
                            GVariant      *parameter,
                            gpointer       user_data)
{
	RenaStatusIcon *status_icon = user_data;
	rena_application_about_dialog (status_icon->rena);
}


static void
rena_systray_gmenu_open (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       user_data)
{
	RenaStatusIcon *status_icon = user_data;
	rena_application_open_files (status_icon->rena);
}

static void
rena_systray_gmenu_location (GSimpleAction *action,
                               GVariant      *parameter,
                               gpointer       user_data)
{
	RenaStatusIcon *status_icon = user_data;
	rena_application_add_location (status_icon->rena);
}

static void
rena_systray_gmenu_playpause (GSimpleAction *action,
                                GVariant      *parameter,
                                gpointer       user_data)
{
	RenaStatusIcon *status_icon = user_data;
	RenaBackend *backend = rena_application_get_backend (status_icon->rena);
	if (rena_backend_emitted_error (backend) == FALSE)
		rena_playback_play_pause_resume(status_icon->rena);
}

static void
rena_systray_gmenu_stop (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       user_data)
{
	RenaStatusIcon *status_icon = user_data;
	RenaBackend *backend = rena_application_get_backend (status_icon->rena);
	if (rena_backend_emitted_error (backend) == FALSE)
		rena_playback_stop(status_icon->rena);
}

static void
rena_systray_gmenu_prev (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       user_data)
{
	RenaStatusIcon *status_icon = user_data;
	RenaBackend *backend = rena_application_get_backend (status_icon->rena);
	if (rena_backend_emitted_error (backend) == FALSE)
		rena_playback_prev_track(status_icon->rena);
}

static void
rena_systray_gmenu_next (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       user_data)
{
	RenaStatusIcon *status_icon = user_data;
	RenaBackend *backend = rena_application_get_backend (status_icon->rena);
	if (rena_backend_emitted_error (backend) == FALSE)
		rena_playback_next_track(status_icon->rena);
}

static void
rena_systray_gmenu_edit (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       user_data)
{
	RenaStatusIcon *status_icon = user_data;
	RenaBackend *backend = rena_application_get_backend (status_icon->rena);
	if (rena_backend_emitted_error (backend) == FALSE)
		rena_playback_edit_current_track (status_icon->rena);
}

static void
rena_systray_gmenu_quit (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       user_data)
{
	RenaStatusIcon *status_icon = user_data;
	rena_application_quit (status_icon->rena);
}

static void
rena_status_icon_update_state (RenaBackend *backend, GParamSpec *pspec, RenaStatusIcon *status_icon)
{
	GAction *action;
	RenaBackendState state = ST_STOPPED;

	state = rena_backend_get_state (backend);

	gboolean playing = (state != ST_STOPPED);

	action = g_action_map_lookup_action (G_ACTION_MAP (status_icon->actions), "prev");
	g_object_set (action, "enabled", playing, NULL);

	action = g_action_map_lookup_action (G_ACTION_MAP (status_icon->actions), "stop");
	g_object_set (action, "enabled", playing, NULL);

	action = g_action_map_lookup_action (G_ACTION_MAP (status_icon->actions), "next");
	g_object_set (action, "enabled", playing, NULL);

	action = g_action_map_lookup_action (G_ACTION_MAP (status_icon->actions), "edit");
	g_object_set (action, "enabled", playing, NULL);
}

static void
rena_status_icon_set_application (RenaStatusIcon *status_icon, RenaApplication *rena)
{
	RenaPreferences *preferences;

	const GBindingFlags binding_flags =
		G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL;

	status_icon->rena = rena;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
	gtk_status_icon_set_from_icon_name (GTK_STATUS_ICON(status_icon), "rena-panel");
G_GNUC_END_IGNORE_DEPRECATIONS

	status_icon->actions =  g_simple_action_group_new ();
	g_action_map_add_action_entries (G_ACTION_MAP(status_icon->actions),
	                                 systray_menu_aentries,
	                                 G_N_ELEMENTS(systray_menu_aentries),
	                                 (gpointer)status_icon);

	preferences = rena_application_get_preferences (rena);
	g_object_bind_property (preferences, "show-status-icon",
	                        status_icon, "visible", binding_flags);

	g_signal_connect (rena_application_get_backend (rena), "notify::state",
	                  G_CALLBACK (rena_status_icon_update_state), status_icon);

	rena_status_icon_update_state (rena_application_get_backend (rena), NULL, status_icon);
}

static void
rena_status_icon_dispose (GObject *object)
{
	RenaStatusIcon *status_icon = RENA_STATUS_ICON(object);

	if (status_icon->builder) {
		g_object_unref (status_icon->builder);
		status_icon->builder = NULL;
	}

	(*G_OBJECT_CLASS (rena_status_icon_parent_class)->dispose) (object);
}

static void
rena_status_icon_class_init (RenaStatusIconClass *klass)
{
	GObjectClass  *gobject_class;

	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->dispose = rena_status_icon_dispose;
}

static void
rena_status_icon_init (RenaStatusIcon *status_icon)
{
	GError *error = NULL;

	status_icon->builder = gtk_builder_new ();
	gtk_builder_add_from_string (status_icon->builder, rena_window_ui, -1, &error);
	if (error) {
		g_print ("GtkBuilder error: %s", error->message);
		g_error_free (error);
		error = NULL;
	}

	g_signal_connect (status_icon, "button-press-event",
	                  G_CALLBACK (status_icon_clicked), status_icon);
	g_signal_connect (status_icon, "scroll_event",
	                  G_CALLBACK(systray_volume_scroll), status_icon);

	g_object_set (G_OBJECT(status_icon), "has-tooltip", TRUE, NULL);
	g_signal_connect (G_OBJECT(status_icon), "query-tooltip",
	                  G_CALLBACK(status_get_tooltip_cb), status_icon);
}

RenaStatusIcon *
rena_status_icon_new (RenaApplication *rena)
{
	RenaStatusIcon *status_icon;

	status_icon = g_object_new (RENA_TYPE_STATUS_ICON, NULL);

	rena_status_icon_set_application (status_icon, rena);

	return status_icon;
}
