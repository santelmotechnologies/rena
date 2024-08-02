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

#include "rena-window.h"

#if defined(GETTEXT_PACKAGE)
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#include "rena-playback.h"
#include "rena-toolbar.h"
#include "rena-menubar.h"
#include "rena-playlists-mgmt.h"
#include "rena-session.h"
#include "rena-utils.h"
#include "rena-app-notification-container.h"

/********************************/
/* Externally visible functions */
/********************************/

gboolean
rena_close_window(GtkWidget *widget, GdkEvent *event, RenaApplication *rena)
{
	RenaStatusIcon *status_icon;
	RenaPreferences *preferences;

	preferences = rena_application_get_preferences (rena);
	if (rena_preferences_get_hide_instead_close (preferences)) {
		status_icon = rena_application_get_status_icon (rena);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
		if (rena_preferences_get_show_status_icon (preferences) &&
		    gtk_status_icon_is_embedded (GTK_STATUS_ICON(status_icon)))
			rena_window_toggle_state(rena, FALSE);
		else
			gtk_window_iconify (GTK_WINDOW(rena_application_get_window(rena)));
G_GNUC_END_IGNORE_DEPRECATIONS
	}
	else {
		rena_application_quit (rena);
	}
	return TRUE;
}

void
rena_window_toggle_state (RenaApplication *rena, gboolean ignoreActivity)
{
	GtkWidget *window;
	gint x = 0, y = 0;

	window = rena_application_get_window (rena);

	if (gtk_widget_get_visible (window)) {
		if (ignoreActivity || gtk_window_is_active (GTK_WINDOW(window))){
			gtk_window_get_position (GTK_WINDOW(window), &x, &y);
			gtk_widget_hide (GTK_WIDGET(window));
			gtk_window_move (GTK_WINDOW(window), x ,y);
		}
		else gtk_window_present (GTK_WINDOW(window));
	}
	else {
		gtk_widget_show (GTK_WIDGET(window));
	}
}


static void
toggle_check_ignore_button_cb (GtkToggleButton   *button,
                               RenaApplication *rena)
{
	RenaPreferences *preferences;
	preferences = rena_preferences_get();
	rena_preferences_set_ignore_errors (preferences,
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)));
	g_object_unref(preferences);
}

static void
backend_error_dialog_response_cb (GtkDialog         *dialog,
                                  gint               response,
                                  RenaApplication *rena)
{
	switch (response) {
		case GTK_RESPONSE_APPLY:
			rena_advance_playback (rena);
			break;
		case GTK_RESPONSE_CANCEL:
		case GTK_RESPONSE_DELETE_EVENT:
		default:
			rena_backend_stop (rena_application_get_backend (rena));
			break;
	}
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

void
rena_window_show_backend_error_dialog (RenaApplication *rena)
{
	RenaBackend *backend;
	const GError *error;
	GtkWidget *dialog, *check_ignore;

	backend = rena_application_get_backend (rena);
	error = rena_backend_get_error (backend);

	const gchar *file = rena_musicobject_get_file (rena_backend_get_musicobject (backend));

	dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW(rena_application_get_window(rena)),
	                                             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	                                             GTK_MESSAGE_QUESTION,
	                                             GTK_BUTTONS_NONE,
	                                             _("<b>Error playing current track.</b>\n(%s)\n<b>Reason:</b> %s"),
	                                             file, error->message);

	check_ignore = gtk_check_button_new_with_mnemonic (_("Ignore errors and continue playback"));
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
	                    check_ignore, FALSE, FALSE, 0);

	gtk_dialog_add_button (GTK_DIALOG (dialog), _("_Stop"), GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (dialog), _("_Next"), GTK_RESPONSE_APPLY);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_APPLY);

	g_signal_connect (G_OBJECT (check_ignore), "toggled",
	                  G_CALLBACK (toggle_check_ignore_button_cb), rena);

	g_signal_connect(G_OBJECT(dialog), "response",
	                 G_CALLBACK(backend_error_dialog_response_cb), rena);

	gtk_widget_show_all(dialog);
}

void
gui_backend_error_update_current_playlist_cb (RenaBackend *backend, const GError *error, RenaApplication *rena)
{
	RenaPlaylist *playlist;
	playlist = rena_application_get_playlist (rena);

	rena_playlist_set_track_error (playlist, rena_backend_get_error (backend));
}

static gboolean
rena_window_state_event (GtkWidget *widget, GdkEventWindowState *event, RenaApplication *rena)
{
	GtkAction *action_fullscreen;

 	if (event->type == GDK_WINDOW_STATE && (event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN)) {
		action_fullscreen = rena_application_get_menu_action (rena, "/Menubar/ViewMenu/Fullscreen");

		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action_fullscreen),
		                              (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) != 0);
	}

	return FALSE;
}

void
rena_window_unfullscreen (GObject *object, RenaApplication *rena)
{
	GtkAction *action_fullscreen;

	action_fullscreen = rena_application_get_menu_action (rena, "/Menubar/ViewMenu/Fullscreen");

	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action_fullscreen), FALSE);
}

static void
rena_sidebar_children_changed (RenaSidebar *sidebar, RenaApplication *rena)
{
	GtkAction *action;
	GAction *baction;
	GActionMap *map;

	action = rena_application_get_menu_action (rena, "/Menubar/ViewMenu/Lateral panel2");

	map = G_ACTION_MAP (rena_application_get_window(rena));
	baction = g_action_map_lookup_action (map, "sidebar2");

	if (rena_sidebar_get_n_panes (sidebar)) {
		gtk_action_set_visible (action, TRUE);
		g_simple_action_set_enabled (G_SIMPLE_ACTION (baction), TRUE);
		gtk_widget_set_visible (GTK_WIDGET(sidebar), TRUE);
	}
	else {
		gtk_action_set_visible (action, FALSE);
		g_simple_action_set_enabled (G_SIMPLE_ACTION (baction), FALSE);
		gtk_widget_set_visible (GTK_WIDGET(sidebar), FALSE);
	}
}

static void
rena_playlist_edge_reache (GtkScrolledWindow *scrolled_window,
                             GtkPositionType    pos,
                             RenaApplication *rena)
{
	RenaStatusbar *statusbar;
	statusbar = rena_application_get_statusbar (rena);

	// TODO: Do it intelligently. Just hidden it when reach the end of the list.
	if (pos == GTK_POS_BOTTOM)
		gtk_widget_set_visible (GTK_WIDGET(statusbar), FALSE);
	else if (pos == GTK_POS_TOP)
		gtk_widget_set_visible (GTK_WIDGET(statusbar), TRUE);
}


/*
 * Public api.
 */

void
rena_window_add_widget_to_infobox (RenaApplication *rena, GtkWidget *widget)
{
	GtkWidget *infobox, *children;
	GList *list;

	infobox = rena_application_get_infobox_container (rena);
	list = gtk_container_get_children (GTK_CONTAINER(infobox));

	if(list) {
		children = list->data;
		gtk_container_remove (GTK_CONTAINER(infobox), children);
		gtk_widget_destroy(GTK_WIDGET(children));
		g_list_free(list);
	}
		
	gtk_container_add (GTK_CONTAINER(infobox), widget);
}


/*
 * Create and destroy the main window.
 */

void
rena_window_save_settings (RenaApplication *rena)
{
	RenaPreferences *preferences;
	GtkWidget *window, *pane;
	gint *window_size, *window_position;
	gint win_width, win_height, win_x, win_y;
	GdkWindowState state;
	const gchar *user_config_dir;
	gchar *rena_accels_path = NULL;

	preferences = rena_preferences_get();

	/* Save last window state */

	window = rena_application_get_window (rena);

	state = gdk_window_get_state (gtk_widget_get_window (window));

	if (rena_preferences_get_remember_state(preferences)) {
		if (state & GDK_WINDOW_STATE_FULLSCREEN)
			rena_preferences_set_start_mode(preferences, FULLSCREEN_STATE);
		else if(state & GDK_WINDOW_STATE_WITHDRAWN)
			rena_preferences_set_start_mode(preferences, ICONIFIED_STATE);
		else
			rena_preferences_set_start_mode(preferences, NORMAL_STATE);
	}

	/* Save geometry only if window is not maximized or fullscreened */

	if (!(state & GDK_WINDOW_STATE_MAXIMIZED) || !(state & GDK_WINDOW_STATE_FULLSCREEN)) {
		window_size = g_new0(gint, 2);
		gtk_window_get_size(GTK_WINDOW(window),
		                    &win_width, &win_height);
		window_size[0] = win_width;
		window_size[1] = win_height;

		window_position = g_new0(gint, 2);
		gtk_window_get_position(GTK_WINDOW(window),
		                        &win_x, &win_y);
		window_position[0] = win_x;
		window_position[1] = win_y;

		rena_preferences_set_integer_list (preferences,
		                                     GROUP_WINDOW,
		                                     KEY_WINDOW_SIZE,
		                                     window_size,
		                                     2);

		rena_preferences_set_integer_list (preferences,
		                                     GROUP_WINDOW,
		                                     KEY_WINDOW_POSITION,
		                                     window_position,
		                                     2);

		g_free(window_size);
		g_free(window_position);
	}

	/* Save sidebar size */

	pane = rena_application_get_first_pane (rena);
	rena_preferences_set_sidebar_size(preferences,
		gtk_paned_get_position(GTK_PANED(pane)));

	pane = rena_application_get_second_pane (rena);
	rena_preferences_set_secondary_sidebar_size (preferences,
		gtk_paned_get_position(GTK_PANED(pane)));

	/* Save menu accelerators edited */

	user_config_dir = g_get_user_config_dir();
	rena_accels_path = g_build_path(G_DIR_SEPARATOR_S, user_config_dir, "/rena/accels.scm", NULL);
	gtk_accel_map_save (rena_accels_path);

	/* Free memory */

	g_object_unref(preferences);
	g_free(rena_accels_path);
}

void rena_init_gui_state (RenaApplication *rena)
{
	RenaPlaylist *playlist;
	RenaLibraryPane *library;
	RenaPreferences *preferences;

	library = rena_application_get_library (rena);
	rena_library_pane_init_view (library);

	preferences = rena_application_get_preferences (rena);
	if (rena_preferences_get_restore_playlist (preferences)) {
		playlist = rena_application_get_playlist (rena);
		rena_playlist_init_playlist_state (playlist);
	}

	if (info_bar_import_music_will_be_useful(rena)) {
		GtkWidget* info_bar = create_info_bar_import_music(rena);
		rena_window_add_widget_to_infobox(rena, info_bar);
	}
}

static void
rena_window_init_menu_actions (RenaApplication *rena)
{
	RenaPreferences *preferences;
	GtkAction *action = NULL;
	const gchar *start_mode;

	preferences = rena_application_get_preferences (rena);

	action = rena_application_get_menu_action (rena, "/Menubar/ViewMenu/Fullscreen");

	start_mode = rena_preferences_get_start_mode (preferences);
	if(!g_ascii_strcasecmp(start_mode, FULLSCREEN_STATE))
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION(action), TRUE);
	else
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION(action), FALSE);

	action = rena_application_get_menu_action (rena, "/Menubar/ViewMenu/Playback controls below");
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION(action), rena_preferences_get_controls_below (preferences));
}

static void
rena_window_init (RenaApplication *rena)
{
	RenaStatusIcon *status_icon;
	RenaPreferences *preferences;
	GtkWidget *window;
	const gchar *start_mode;

	/* Init window state */

	preferences = rena_application_get_preferences (rena);
	window = rena_application_get_window (rena);

	start_mode = rena_preferences_get_start_mode (preferences);
	if(!g_ascii_strcasecmp(start_mode, FULLSCREEN_STATE)) {
		gtk_widget_show(window);
	}
	else if(!g_ascii_strcasecmp(start_mode, ICONIFIED_STATE)) {
		status_icon = rena_application_get_status_icon (rena);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
		if(gtk_status_icon_is_embedded (GTK_STATUS_ICON(status_icon))) {
			gtk_widget_hide(GTK_WIDGET(window));
		}
		else {
			g_warning("(%s): No embedded status_icon.", __func__);
			gtk_window_iconify (GTK_WINDOW(window));
			gtk_widget_show(window);
		}
G_GNUC_END_IGNORE_DEPRECATIONS
	}
	else {
		gtk_widget_show(window);
	}

	rena_window_init_menu_actions(rena);

	rena_init_session_support(rena);
}

static void
prefrences_change_icon_size (RenaPreferences *preferences,
                             GParamSpec        *pspec,
                             GtkWidget         *button)
{
	GIcon *icon = NULL;

	const gchar *fallbacks_icon_menu[] = {
		"open-menu",
		"emblem-system",
		"open-menu-symbolic",
		"emblem-system-symbolic",
		NULL,
	};

  	icon = g_themed_icon_new_from_names ((gchar **)fallbacks_icon_menu, -1);
	gtk_button_set_image (GTK_BUTTON (button),
		gtk_image_new_from_gicon(icon, rena_preferences_get_toolbar_size(preferences)));
	g_object_unref (icon);
}

void
rena_window_new (RenaApplication *rena)
{
	RenaPreferences *preferences;
	GtkWidget *window;
	RenaAppNotificationContainer *container;
	RenaPlaylist *playlist;
	RenaLibraryPane *library;
	RenaSidebar *sidebar1, *sidebar2;
	RenaStatusbar *statusbar;
	RenaToolbar *toolbar;
	GtkWidget *menubar, *overlay, *pane1, *pane2, *infobox;
	GtkWidget *main_stack, *playlist_overlay, *vbox_main;
	GtkWidget *song_box, *playlist_menu_button, *menu_button;
	GtkBuilder *menu_ui;
	GtkCssProvider *css_provider;
	GIcon *icon = NULL;
	GError *error = NULL;
	gint *win_size, *win_position;
	gchar *css_filename = NULL;
	gsize cnt = 0;

	const GBindingFlags binding_flags =
		G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL;

	const gchar *fallbacks_icon_menu[] = {
		"open-menu",
		"emblem-system",
		"open-menu-symbolic",
		"emblem-system-symbolic",
		NULL,
	};

	const gchar *fallbacks_playlist_icon[] = {
		"view-list",
		"view-list-symbolic",
		NULL,
	};

	CDEBUG(DBG_INFO, "Packaging widgets, and initiating the window");

	preferences = rena_application_get_preferences (rena);

	/* Collect widgets. */

	window    = rena_application_get_window (rena);
	playlist  = rena_application_get_playlist (rena);
	library   = rena_application_get_library (rena);
	overlay   = rena_application_get_overlay (rena);
	sidebar1  = rena_application_get_first_sidebar (rena);
	main_stack= rena_application_get_main_stack (rena);
	sidebar2  = rena_application_get_second_sidebar (rena);
	statusbar = rena_application_get_statusbar (rena);
	toolbar   = rena_application_get_toolbar (rena);
	menubar   = rena_application_get_menubar (rena);
	pane1     = rena_application_get_first_pane (rena);
	pane2     = rena_application_get_second_pane (rena);
	infobox   = rena_application_get_infobox_container (rena);

	/* Main window */

	g_signal_connect (G_OBJECT(window), "window-state-event",
	                  G_CALLBACK(rena_window_state_event), rena);
	g_signal_connect (G_OBJECT(window), "delete_event",
	                  G_CALLBACK(rena_close_window), rena);

	/* Set Default Size */

	win_size = rena_preferences_get_integer_list (preferences,
	                                                GROUP_WINDOW,
	                                                KEY_WINDOW_SIZE,
	                                                &cnt);
	if (win_size) {
		gtk_window_set_default_size(GTK_WINDOW(window),
		                            win_size[0], win_size[1]);
		g_free(win_size);
	}
	else {
		gtk_window_set_default_size(GTK_WINDOW(window),
		                            MIN_WINDOW_WIDTH, MIN_WINDOW_HEIGHT);
	}

	/* Set Position */

	win_position = rena_preferences_get_integer_list (preferences,
	                                                    GROUP_WINDOW,
	                                                    KEY_WINDOW_POSITION,
	                                                    &cnt);

	if (win_position) {
		gtk_window_move(GTK_WINDOW(window),
		                win_position[0], win_position[1]);
		g_free(win_position);
	}
	else {
		gtk_window_set_position(GTK_WINDOW(window),
		                        GTK_WIN_POS_CENTER);
	}

	/* Pack widgets: [      MainStack      ]
	 *               [ Playlist/Status Bar ]
	 */

	playlist_overlay = gtk_overlay_new();
	gtk_container_add (GTK_CONTAINER(playlist_overlay),
	                   GTK_WIDGET(playlist));

	gtk_widget_set_halign (GTK_WIDGET(statusbar), GTK_ALIGN_START);
	gtk_widget_set_valign (GTK_WIDGET(statusbar), GTK_ALIGN_END);
	gtk_overlay_add_overlay (GTK_OVERLAY(playlist_overlay),
	                         GTK_WIDGET(statusbar));

	gtk_stack_add_named (GTK_STACK(main_stack),
	                     GTK_WIDGET(playlist_overlay), "playlist");

	/*
	 *  Show and hide the status bar automatically.
	 */
	g_signal_connect (G_OBJECT (playlist), "edge-reached",
	                  G_CALLBACK(rena_playlist_edge_reache), rena);

	/* Pack widgets: [Sidebar1][Main Stack]
	 *               [        ][Status Bar]
	 */

	gtk_paned_pack1 (GTK_PANED (pane1), GTK_WIDGET(sidebar1), FALSE, TRUE);
	gtk_paned_pack2 (GTK_PANED (pane1), main_stack, TRUE, FALSE);

	gtk_paned_set_position (GTK_PANED (pane1),
		rena_preferences_get_sidebar_size (preferences));

	/* Pack widgets: [Sidebar1][ Playlist ][Sidebar2]
	 *               [        ][Status Bar][        ]
	 */

	gtk_paned_pack1 (GTK_PANED (pane2), pane1, TRUE, FALSE);
	gtk_paned_pack2 (GTK_PANED (pane2), GTK_WIDGET(sidebar2), FALSE, TRUE);

	gtk_paned_set_position (GTK_PANED (pane2),
		rena_preferences_get_secondary_sidebar_size (preferences));

	gtk_container_add (GTK_CONTAINER (overlay), GTK_WIDGET (pane2));

	/* Pack widgets: [            Menubar           ]
	 *               [            Toolbar           ]
	 *               [            Infobox           ]
	 *               [Sidebar1][ Playlist ][Sidebar2]
	 *               [Sidebar1][Status Bar][Sidebar2]
	 */

	vbox_main = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

	/* Add menubar */
	gtk_box_pack_start (GTK_BOX(vbox_main), menubar,
	                    FALSE, FALSE, 0);

	/* If not CSD add toolbar */
	if (rena_preferences_get_system_titlebar (preferences))
		gtk_box_pack_start (GTK_BOX(vbox_main), GTK_WIDGET(toolbar),
		                    FALSE, FALSE, 0);

	/* Add infobox */
	gtk_box_pack_start (GTK_BOX(vbox_main), infobox,
	                    FALSE, FALSE, 0);

	/* Append overlay that is the main widget. */
	gtk_box_pack_start (GTK_BOX(vbox_main), overlay,
	                    TRUE, TRUE, 0);

	/* Add notification container within these overlay */
	container = rena_app_notification_container_get_default ();
	gtk_overlay_add_overlay (GTK_OVERLAY (overlay), GTK_WIDGET (container));

	/* Configure menubar visibility */

	g_object_bind_property (preferences, "show-menubar",
	                        menubar, "visible",
	                        binding_flags);

	/* Add playlist menu */

	playlist_menu_button =  gtk_menu_button_new ();
	gtk_button_set_relief(GTK_BUTTON(playlist_menu_button), GTK_RELIEF_NONE);

	icon = g_themed_icon_new_from_names ((gchar **)fallbacks_playlist_icon, -1);
	gtk_button_set_image (GTK_BUTTON (playlist_menu_button),
		gtk_image_new_from_gicon(icon, rena_preferences_get_toolbar_size(preferences)));
	g_object_unref (icon);

	menu_ui = rena_application_get_menu_ui(rena);
	gtk_menu_button_set_menu_model (GTK_MENU_BUTTON(playlist_menu_button),
		G_MENU_MODEL (gtk_builder_get_object (menu_ui, "playlist-button-menu")));

	g_object_bind_property (preferences, "show-menubar",
	                        playlist_menu_button, "visible",
	                        binding_flags | G_BINDING_INVERT_BOOLEAN);

	g_signal_connect (preferences, "notify::toolbar-size",
	                  G_CALLBACK (prefrences_change_icon_size), playlist_menu_button);

	rena_toolbar_add_extra_button (toolbar, playlist_menu_button);

	/* Add menu-button to toolbar */

	menu_button =  gtk_menu_button_new ();
	gtk_button_set_relief(GTK_BUTTON(menu_button), GTK_RELIEF_NONE);

	icon = g_themed_icon_new_from_names ((gchar **)fallbacks_icon_menu, -1);
	gtk_button_set_image (GTK_BUTTON (menu_button),
		gtk_image_new_from_gicon(icon, rena_preferences_get_toolbar_size(preferences)));
	g_object_unref (icon);

	menu_ui = rena_application_get_menu_ui(rena);
	gtk_menu_button_set_menu_model (GTK_MENU_BUTTON(menu_button),
		G_MENU_MODEL (gtk_builder_get_object (menu_ui, "button-menu")));

	g_object_bind_property (preferences, "show-menubar",
	                        menu_button, "visible",
	                        binding_flags | G_BINDING_INVERT_BOOLEAN);

	g_signal_connect (preferences, "notify::toolbar-size",
	                  G_CALLBACK (prefrences_change_icon_size), menu_button);

	rena_toolbar_add_extra_button (toolbar, menu_button);

	/* Add library pane to first sidebar. */

	rena_sidebar_attach_plugin (sidebar1,
		                          rena_library_pane_get_widget (library),
		                          rena_library_pane_get_pane_title (library),
		                          rena_library_pane_get_popover (library));

	g_object_bind_property (preferences, "lateral-panel",
	                        sidebar1, "visible",
	                        binding_flags);

	/* Second sidebar visibility depend on their children */

	g_signal_connect (G_OBJECT(sidebar2), "children-changed",
	                  G_CALLBACK(rena_sidebar_children_changed), rena);
	rena_sidebar_style_position (sidebar2, GTK_POS_RIGHT);

	/* Show the widgets individually.
	 *  NOTE: the rest of the widgets, depends on the preferences.
	 */
	gtk_widget_show(vbox_main);
	gtk_widget_show (GTK_WIDGET(toolbar));
	gtk_widget_show (infobox);
	gtk_widget_show (pane1);
	gtk_widget_show (main_stack);
	gtk_widget_show (pane2);
	gtk_widget_show (overlay);

	gtk_widget_show(playlist_overlay);
	gtk_widget_show_all (GTK_WIDGET(playlist));

	/* Pack everyting on the main window. */

	gtk_container_add(GTK_CONTAINER(window), vbox_main);

	/* Attach CSS to main window */

	css_provider = gtk_css_provider_new ();

	css_filename = g_build_path(G_DIR_SEPARATOR_S, USRSTYLEDIR, "rena.css", NULL);
	gtk_css_provider_load_from_path (css_provider, css_filename, &error);

	if (error == NULL)
	{
		gtk_style_context_add_provider_for_screen (gtk_widget_get_screen (GTK_WIDGET (window)),
		                                           GTK_STYLE_PROVIDER (css_provider),
		                                           GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	}
	else
	{
		g_warning ("Could not attach rena css style: %s", error->message);
		g_error_free (error);
	}
	g_free (css_filename);

	/* Attach the custum CSS to main window */

	css_filename = g_build_path(G_DIR_SEPARATOR_S, USRSTYLEDIR, "custom.css", NULL);
	if (g_file_test(css_filename, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
	{
		gtk_css_provider_load_from_path (css_provider, css_filename, &error);

		if (error == NULL)
		{
			gtk_style_context_add_provider_for_screen (gtk_widget_get_screen (GTK_WIDGET (window)),
			                                           GTK_STYLE_PROVIDER (css_provider),
			                                           GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		}
		else
		{
			g_warning ("Could not attach distro css style: %s", error->message);
			g_error_free (error);
		}
	}
	g_free (css_filename);

	css_filename = g_build_path(G_DIR_SEPARATOR_S, g_get_user_config_dir(), "/rena/custom.css", NULL);
	if (g_file_test(css_filename, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
	{
		gtk_css_provider_load_from_path (css_provider, css_filename, &error);

		if (error == NULL)
		{
			gtk_style_context_add_provider_for_screen (gtk_widget_get_screen (GTK_WIDGET (window)),
			                                           GTK_STYLE_PROVIDER (css_provider),
			                                           GTK_STYLE_PROVIDER_PRIORITY_USER);
		}
		else
		{
		g_warning ("Could not attach user css style: %s", error->message);
		g_error_free (error);
		}
	}
	g_free (css_filename);

	g_object_unref (css_provider);

	if (!rena_preferences_get_system_titlebar (preferences))
		gtk_window_set_titlebar (GTK_WINDOW (window), GTK_WIDGET(toolbar));

	song_box = rena_toolbar_get_song_box(toolbar);
	gtk_header_bar_set_custom_title(GTK_HEADER_BAR(toolbar), GTK_WIDGET(song_box));

	gtk_widget_show (GTK_WIDGET(toolbar));

	rena_window_init (rena);
}
