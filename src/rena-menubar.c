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

#include "rena-menubar.h"

#if defined(GETTEXT_PACKAGE)
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#include <gdk/gdkkeysyms.h>

#include <stdlib.h>

#include "rena-playback.h"
#include "rena-file-utils.h"
#include "rena-utils.h"
#include "rena-filter-dialog.h"
#include "rena-playlists-mgmt.h"
#include "rena-tagger.h"
#include "rena-tags-dialog.h"
#include "rena-tags-mgmt.h"
#include "rena-preferences-dialog.h"
#include "rena-musicobject-mgmt.h"
#include "rena-equalizer-dialog.h"
#include "rena.h"

#include "rena-window-ui.h"

/*
 * Prototypes
 */

static void
rena_gear_menu_update_playlist_changes (RenaDatabase    *database,
                                          RenaApplication *rena);

/*
 * Menubar callbacks.
 */

/* Playback */

static void prev_action(GtkAction *action, RenaApplication *rena);
static void play_pause_action(GtkAction *action, RenaApplication *rena);
static void stop_action(GtkAction *action, RenaApplication *rena);
static void next_action (GtkAction *action, RenaApplication *rena);
static void edit_tags_playing_action(GtkAction *action, RenaApplication *rena);
static void quit_action(GtkAction *action, RenaApplication *rena);

/* Playlist */

static void open_file_action(GtkAction *action, RenaApplication *rena);
static void add_location_action(GtkAction *action, RenaApplication *rena);
static void add_libary_action(GtkAction *action, RenaApplication *rena);
static void rena_menubar_remove_playlist_action      (GtkAction *action, RenaApplication *rena);
static void rena_menubar_crop_playlist_action        (GtkAction *action, RenaApplication *rena);
static void rena_menubar_clear_playlist_action       (GtkAction *action, RenaApplication *rena);
static void rena_menubar_save_playlist_action        (GtkAction *action, RenaApplication *rena);
static void rena_menubar_export_playlist_action      (GtkAction *action, RenaApplication *rena);
static void rena_menubar_save_selection_action       (GtkAction *action, RenaApplication *rena);
static void rena_menubar_export_selection_action     (GtkAction *action, RenaApplication *rena);
static void rena_menu_action_save_playlist           (GtkAction *action, RenaApplication *rena);
static void rena_menu_action_save_selection          (GtkAction *action, RenaApplication *rena);
static void search_playlist_action(GtkAction *action, RenaApplication *rena);

/* View */

static void fullscreen_action (GtkAction *action, RenaApplication *rena);
static void show_controls_below_action (GtkAction *action, RenaApplication *rena);
static void jump_to_playing_song_action (GtkAction *action, RenaApplication *rena);

/* Tools */

static void show_equalizer_action(GtkAction *action, RenaApplication *rena);
static void rescan_library_action(GtkAction *action, RenaApplication *rena);
static void update_library_action(GtkAction *action, RenaApplication *rena);
static void statistics_action(GtkAction *action, RenaApplication *rena);
static void pref_action(GtkAction *action, RenaApplication *rena);

/* Help */

static void home_action(GtkAction *action, RenaApplication *rena);
static void community_action(GtkAction *action, RenaApplication *rena);
static void wiki_action(GtkAction *action, RenaApplication *rena);
static void translate_action(GtkAction *action, RenaApplication *rena);
static void about_action(GtkAction *action, RenaApplication *rena);

/*
 * Menu bar ui definition.
 */

static const gchar *main_menu_xml = "<ui>					\
	<menubar name=\"Menubar\">						\
		<menu action=\"PlaybackMenu\">					\
			<separator/>						\
			<menuitem action=\"Prev\"/>				\
			<menuitem action=\"Play_pause\"/>			\
			<menuitem action=\"Stop\"/>				\
			<menuitem action=\"Next\"/>				\
			<separator/>						\
			<menuitem action=\"Shuffle\"/>				\
			<menuitem action=\"Repeat\"/>				\
			<separator/>						\
			<menuitem action=\"Edit tags\"/>			\
			<separator/>						\
			<menuitem action=\"Quit\"/>				\
		</menu>								\
		<menu action=\"PlaylistMenu\">					\
			<menuitem action=\"Add files\"/>			\
			<menuitem action=\"Add location\"/>			\
			<placeholder name=\"rena-append-music-placeholder\"/>		\
			<separator/>				    			\
			<menuitem action=\"Add the library\"/>		\
			<separator/>				    		\
			<menuitem action=\"Remove from playlist\"/>		\
			<menuitem action=\"Crop playlist\"/>			\
			<menuitem action=\"Clear playlist\"/>			\
			<separator/>				    		\
			<menu action=\"SavePlaylist\">				\
				<menuitem action=\"New playlist1\"/>		\
				<menuitem action=\"Export1\"/>			\
				<separator/>				    	\
				<placeholder name=\"rena-save-playlist-placeholder\"/> 	\
			</menu>							\
			<menu action=\"SaveSelection\">				\
				<menuitem action=\"New playlist2\"/>		\
				<menuitem action=\"Export2\"/>			\
				<separator/>				    	\
				<placeholder name=\"rena-save-selection-placeholder\"/> 	\
			</menu>							\
			<separator/>						\
			<menuitem action=\"Search in playlist\"/>		\
		</menu>								\
		<menu action=\"ViewMenu\">					\
			<placeholder name=\"rena-view-placeholder\"/> 	\
			<separator/>						\
			<menuitem action=\"Fullscreen\"/>			\
			<separator/>						\
			<menuitem action=\"Lateral panel1\"/>		\
			<menuitem action=\"Lateral panel2\"/>		\
			<menuitem action=\"Playback controls below\"/>	\
			<menuitem action=\"Show menubar\"/>			\
			<separator/>						\
			<menuitem action=\"Jump to playing song\"/>	\
		</menu>								\
		<menu action=\"ToolsMenu\">					\
			<separator/>						\
			<menuitem action=\"Equalizer\"/>			\
			<separator/>						\
			<placeholder name=\"rena-plugins-placeholder\"/>		\
			<separator/>						\
			<menuitem action=\"Rescan library\"/>			\
			<menuitem action=\"Update library\"/>			\
			<separator/>						\
			<menuitem action=\"Statistics\"/>			\
			<separator/>						\
			<menuitem action=\"Preferences\"/>			\
		</menu>								\
		<menu action=\"HelpMenu\">					\
			<menuitem action=\"Home\"/>				\
			<menuitem action=\"Community\"/>			\
			<menuitem action=\"Wiki\"/>				\
			<separator/>						\
			<menuitem action=\"Translate Rena\"/>			\
			<separator/>						\
			<menuitem action=\"About\"/>				\
		</menu>								\
	</menubar>								\
</ui>";

static GtkActionEntry main_aentries[] = {
	{"PlaybackMenu", NULL, N_("_Playback")},
	{"PlaylistMenu", NULL, N_("Play_list")},
	{"ViewMenu", NULL, N_("_View")},
	{"ToolsMenu", NULL, N_("_Tools")},
	{"HelpMenu", NULL, N_("_Help")},
	{"Prev", NULL, N_("Previous track"),
	 "<Alt>Left", "Prev track", G_CALLBACK(prev_action)},
	{"Play_pause", NULL, N_("Play / Pause"),
	 "<Control>space", "Play / Pause", G_CALLBACK(play_pause_action)},
	{"Stop", NULL, N_("Stop"),
	 "", "Stop", G_CALLBACK(stop_action)},
	{"Next", NULL, N_("Next track"),
	 "<Alt>Right", "Next track", G_CALLBACK(next_action)},
	{"Edit tags", NULL, N_("Edit track information"),
	 "<Control>E", "Edit information of current track", G_CALLBACK(edit_tags_playing_action)},
	{"Quit", NULL, N_("_Quit"),
	 "<Control>Q", "Quit rena", G_CALLBACK(quit_action)},
	{"Add files", NULL, N_("_Add files"),
	 "<Control>O", N_("Open a media file"), G_CALLBACK(open_file_action)},
	{"Add location", NULL, N_("Add _location"),
	 "", "Add a no local stream", G_CALLBACK(add_location_action)},
	{"Add the library", NULL, N_("_Add the library"),
	"", "Add all the library", G_CALLBACK(add_libary_action)},
	{"Remove from playlist", NULL, N_("Remove selection from playlist"),
	 "", "Remove selection from playlist", G_CALLBACK(rena_menubar_remove_playlist_action)},
	{"Crop playlist", NULL, N_("Crop playlist"),
	 "<Control>C", "Crop playlist", G_CALLBACK(rena_menubar_crop_playlist_action)},
	{"Clear playlist", NULL, N_("Clear playlist"),
	 "<Control>L", "Clear the current playlist", G_CALLBACK(rena_menubar_clear_playlist_action)},
	{"SavePlaylist", NULL, N_("Save playlist")},
	{"New playlist1", NULL, N_("New playlist"),
	 "<Control>S", "Save new playlist", G_CALLBACK(rena_menubar_save_playlist_action)},
	{"Export1", NULL, N_("Export"),
	 "", "Export playlist", G_CALLBACK(rena_menubar_export_playlist_action)},
	{"SaveSelection", NULL, N_("Save selection")},
	{"New playlist2", NULL, N_("New playlist"),
	 "<Control><Shift>S", "Save new playlist", G_CALLBACK(rena_menubar_save_selection_action)},
	{"Export2", NULL, N_("Export"),
	 "", "Export playlist", G_CALLBACK(rena_menubar_export_selection_action)},
	{"Search in playlist", NULL, N_("_Search in playlist"),
	 "<Control>F", "Search in playlist", G_CALLBACK(search_playlist_action)},
	{"Preferences", NULL, N_("_Preferences"),
	 "<Control>P", "Set preferences", G_CALLBACK(pref_action)},
	{"Jump to playing song", NULL, N_("Jump to playing song"),
	 "<Control>J", "Jump to playing song", G_CALLBACK(jump_to_playing_song_action)},
	{"Equalizer", NULL, N_("E_qualizer"),
	 "", "Equalizer", G_CALLBACK(show_equalizer_action)},
	{"Rescan library", NULL, N_("_Rescan library"),
	 "", "Rescan library", G_CALLBACK(rescan_library_action)},
	{"Update library", NULL, N_("_Update library"),
	 "", "Update library", G_CALLBACK(update_library_action)},
	{"Statistics", NULL, N_("_Statistics"),
	 "", "Statistics", G_CALLBACK(statistics_action)},
	{"Home", NULL, N_("Homepage"),
	 "", "Homepage", G_CALLBACK(home_action)},
	{"Community", NULL, N_("Community"),
	 "", "Forum of rena", G_CALLBACK(community_action)},
	{"Wiki", NULL, N_("Wiki"),
	 "", "Wiki of rena", G_CALLBACK(wiki_action)},
	{"Translate Rena", NULL, N_("Translate Rena"),
	 "", "Translate Rena", G_CALLBACK(translate_action)},
	{"About", NULL, N_("About"),
	 "", "About rena", G_CALLBACK(about_action)},
};

static GtkToggleActionEntry toggles_entries[] = {
	{"Shuffle", NULL, N_("_Shuffle"),
	 "<Control>U", "Shuffle Songs", NULL,
	 FALSE},
	{"Repeat", NULL, N_("_Repeat"),
	 "<Control>R", "Repeat Songs", NULL,
	 FALSE},
	{"Fullscreen", NULL, N_("_Fullscreen"),
	 "F11", "Switch between full screen and windowed mode", G_CALLBACK(fullscreen_action),
	FALSE},
	{"Lateral panel1", NULL, N_("Lateral _panel"),
	 "F9", "Lateral panel", NULL,
	TRUE},
	{"Lateral panel2", NULL, N_("Secondary lateral panel"),
	 "<Shift>F9", "Secondary lateral panel", NULL,
	FALSE},
	{"Playback controls below", NULL, N_("Playback controls below"),
	 NULL, "Show playback controls below", G_CALLBACK(show_controls_below_action),
	FALSE},
	{"Show menubar", NULL, N_("Menubar"),
	 "<Control>M", "Show menubar", NULL,
	TRUE}
};

/* Sentitive menubar actions depending on the playback status. */

void
rena_menubar_update_playback_state_cb (RenaBackend *backend, GParamSpec *pspec, gpointer user_data)
{
	GtkAction *action;
	gboolean playing = FALSE;

	RenaApplication *rena = user_data;

	playing = (rena_backend_get_state (backend) != ST_STOPPED);

	action = rena_application_get_menu_action (rena, "/Menubar/PlaybackMenu/Prev");
	gtk_action_set_sensitive (GTK_ACTION (action), playing);

	action = rena_application_get_menu_action (rena, "/Menubar/PlaybackMenu/Stop");
	gtk_action_set_sensitive (GTK_ACTION (action), playing);

	action = rena_application_get_menu_action (rena, "/Menubar/PlaybackMenu/Next");
	gtk_action_set_sensitive (GTK_ACTION (action), playing);

	action = rena_application_get_menu_action (rena, "/Menubar/PlaybackMenu/Edit tags");
	gtk_action_set_sensitive (GTK_ACTION (action), playing);

	action = rena_application_get_menu_action (rena, "/Menubar/ViewMenu/Jump to playing song");
	gtk_action_set_sensitive (GTK_ACTION (action), playing);
}

static void
rena_menubar_update_playlist_changes (RenaDatabase *database, RenaApplication *rena)
{
	GtkUIManager *ui_manager;
	GtkAction *action;
	RenaPreparedStatement *statement;
	const gchar *sql = NULL, *playlist = NULL;
	gchar *action_name = NULL;

	static gint playlist_ui_id = 0;
	static GtkActionGroup *playlist_action_group = NULL;

	ui_manager = rena_application_get_menu_ui_manager (rena);

	gtk_ui_manager_remove_ui (ui_manager, playlist_ui_id);
	gtk_ui_manager_ensure_update (ui_manager);

	if (playlist_action_group) {
		gtk_ui_manager_remove_action_group (ui_manager, playlist_action_group);
		g_object_unref (playlist_action_group);
	}

	playlist_action_group = gtk_action_group_new ("playlists-action-group");
	gtk_ui_manager_insert_action_group (ui_manager, playlist_action_group, -1);

	playlist_ui_id = gtk_ui_manager_new_merge_id (ui_manager);

	sql = "SELECT name FROM PLAYLIST WHERE name != ? ORDER BY name COLLATE NOCASE DESC";
	statement = rena_database_create_statement (database, sql);
	rena_prepared_statement_bind_string (statement, 1, SAVE_PLAYLIST_STATE);

	while (rena_prepared_statement_step (statement)) {
		playlist = rena_prepared_statement_get_string(statement, 0);

		/* Save playlist */
		action_name = g_strdup_printf ("playlist-to-%s", playlist);
		action = gtk_action_new (action_name, playlist, NULL, NULL);
		gtk_action_group_add_action (playlist_action_group, action);
		g_object_unref (action);

		g_signal_connect (G_OBJECT (action), "activate",
		                  G_CALLBACK (rena_menu_action_save_playlist), rena);

		gtk_ui_manager_add_ui (ui_manager, playlist_ui_id,
				       "/Menubar/PlaylistMenu/SavePlaylist/rena-save-playlist-placeholder",
				       playlist, action_name,
				       GTK_UI_MANAGER_MENUITEM, FALSE);
		g_free (action_name);

		/* Save selection */
		action_name = g_strdup_printf ("selection-to-%s", playlist);
		action = gtk_action_new (action_name, playlist, NULL, NULL);
		gtk_action_group_add_action (playlist_action_group, action);
		g_object_unref (action);

		g_signal_connect (G_OBJECT (action), "activate",
		                  G_CALLBACK (rena_menu_action_save_selection), rena);

		gtk_ui_manager_add_ui (ui_manager, playlist_ui_id,
				       "/Menubar/PlaylistMenu/SaveSelection/rena-save-selection-placeholder",
				       playlist, action_name,
				       GTK_UI_MANAGER_MENUITEM, FALSE);
		g_free (action_name);

		rena_process_gtk_events ();
	}
	rena_prepared_statement_free (statement);

}

/* Handler for the 'Open' item in the File menu */

void open_file_action(GtkAction *action, RenaApplication *rena)
{
	rena_application_open_files (rena);
}

/* Build a dialog to get a new playlist name */

void add_location_action(GtkAction *action, RenaApplication *rena)
{
	rena_application_add_location (rena);
}

/* Handler for 'Add All' action in the Tools menu */

static void add_libary_action(GtkAction *action, RenaApplication *rena)
{
	rena_application_append_entery_libary (rena);
}

/* Handler for the 'Prev' item in the rena menu */

static void prev_action(GtkAction *action, RenaApplication *rena)
{
	rena_playback_prev_track(rena);
}

/* Handler for the 'Play / Pause' item in the rena menu */

static void play_pause_action(GtkAction *action, RenaApplication *rena)
{
	rena_playback_play_pause_resume(rena);
}

/* Handler for the 'Stop' item in the rena menu */

static void stop_action(GtkAction *action, RenaApplication *rena)
{
	rena_playback_stop(rena);
}

/* Handler for the 'Next' item in the rena menu */

static void next_action (GtkAction *action, RenaApplication *rena)
{
	rena_playback_next_track(rena);
}

void edit_tags_playing_action(GtkAction *action, RenaApplication *rena)
{
	rena_playback_edit_current_track (rena);
}

/* Handler for the 'Quit' item in the rena menu */

static void quit_action(GtkAction *action, RenaApplication *rena)
{
	rena_application_quit (rena);
}

/* Handler for 'Search Playlist' option in the Edit menu */

static void search_playlist_action(GtkAction *action, RenaApplication *rena)
{
	rena_filter_dialog (rena_application_get_playlist (rena));
}

/* Handler for the 'Preferences' item in the Edit menu */

static void pref_action(GtkAction *action, RenaApplication *rena)
{
	RenaPreferencesDialog *dialog;
	dialog = rena_preferences_dialog_get ();
	rena_preferences_dialog_show (dialog);
	g_object_unref (dialog);
}

/* Handler for the 'Full screen' item in the Edit menu */

static void
fullscreen_action (GtkAction *action, RenaApplication *rena)
{
	GtkWidget *menu_bar;
	gboolean fullscreen;
	GdkWindowState state;

	menu_bar = rena_application_get_menubar (rena);

	fullscreen = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));

	if(fullscreen){
		gtk_window_fullscreen(GTK_WINDOW(rena_application_get_window(rena)));
		gtk_widget_hide(GTK_WIDGET(menu_bar));
	}
	else {
		state = gdk_window_get_state (gtk_widget_get_window (rena_application_get_window(rena)));
		if (state & GDK_WINDOW_STATE_FULLSCREEN)
			gtk_window_unfullscreen(GTK_WINDOW(rena_application_get_window(rena)));
		gtk_widget_show(GTK_WIDGET(menu_bar));
	}
}

/* Handler for the 'Show_controls_below_action' item in the view menu */

static void
show_controls_below_action (GtkAction *action, RenaApplication *rena)
{
	RenaPreferences *preferences;
	RenaToolbar *toolbar;
	GtkWidget *parent;

	preferences = rena_application_get_preferences (rena);

	rena_preferences_set_controls_below (preferences,
		gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action)));

	toolbar = rena_application_get_toolbar (rena);
	parent  = gtk_widget_get_parent (GTK_WIDGET(toolbar));

	gint position = rena_preferences_get_controls_below (preferences) ? 3 : 1;

	gtk_box_reorder_child(GTK_BOX(parent), GTK_WIDGET(toolbar), position);
}

static void
jump_to_playing_song_action (GtkAction *action, RenaApplication *rena)
{
	RenaPlaylist *playlist;
	playlist = rena_application_get_playlist (rena);

	rena_playlist_show_current_track (playlist);
}

/* Handler for the 'Equalizer' item in the Tools menu */

static void
show_equalizer_action(GtkAction *action, RenaApplication *rena)
{
	GtkWidget *parent = rena_application_get_window (rena);
	RenaBackend *backend = rena_application_get_backend(rena);

	rena_equalizer_dialog_show (backend, parent);
}

/* Handler for the 'Rescan Library' item in the Tools menu */

static void rescan_library_action(GtkAction *action, RenaApplication *rena)
{
	RenaScanner *scanner;
	scanner = rena_application_get_scanner (rena);

	rena_scanner_scan_library (scanner);
}

/* Handler for the 'Update Library' item in the Tools menu */

static void update_library_action(GtkAction *action, RenaApplication *rena)
{
	RenaScanner *scanner;
	scanner = rena_application_get_scanner (rena);

	rena_scanner_update_library (scanner);
}

/* Handler for remove, crop and clear action in the Tools menu */

static void
rena_menubar_remove_playlist_action (GtkAction *action, RenaApplication *rena)
{
	RenaPlaylist *playlist;

	playlist = rena_application_get_playlist (rena);
	rena_playlist_remove_selection (playlist);
}

static void
rena_menubar_crop_playlist_action (GtkAction *action, RenaApplication *rena)
{
	RenaPlaylist *playlist;

	playlist = rena_application_get_playlist (rena);
	rena_playlist_crop_selection (playlist);
}

static void
rena_menubar_clear_playlist_action (GtkAction *action, RenaApplication *rena)
{
	RenaPlaylist *playlist;

	playlist = rena_application_get_playlist (rena);
	rena_playlist_remove_all (playlist);
}

static void
rena_menubar_save_playlist_action (GtkAction *action, RenaApplication *rena)
{
	RenaPlaylist *playlist = rena_application_get_playlist (rena);
	save_current_playlist (NULL, playlist);
}

static void
rena_menubar_export_playlist_action (GtkAction *action, RenaApplication *rena)
{
	RenaPlaylist *playlist = rena_application_get_playlist (rena);
	export_current_playlist (NULL, playlist);
}

static void
rena_menubar_save_selection_action (GtkAction *action, RenaApplication *rena)
{
	RenaPlaylist *playlist = rena_application_get_playlist (rena);
	save_selected_playlist (NULL, playlist);
}

static void
rena_menu_action_save_playlist (GtkAction *action, RenaApplication *rena)
{
	RenaPlaylist *playlist = rena_application_get_playlist (rena);;
	const gchar *name = gtk_action_get_label (action);

	rena_playlist_save_playlist (playlist, name);
}

static void
rena_menu_action_save_selection (GtkAction *action, RenaApplication *rena)
{
	RenaPlaylist *playlist = rena_application_get_playlist (rena);;
	const gchar *name = gtk_action_get_label (action);

	rena_playlist_save_selection (playlist, name);
}

static void
rena_menubar_export_selection_action (GtkAction *action, RenaApplication *rena)
{
	RenaPlaylist *playlist = rena_application_get_playlist (rena);
	export_selected_playlist (NULL, playlist);
}

/* Handler for 'Statistics' action in the Tools menu */

static void statistics_action(GtkAction *action, RenaApplication *rena)
{
	RenaDatabase *cdbase;
	gint n_artists, n_albums, n_tracks;
	GtkWidget *dialog;

	cdbase = rena_application_get_database (rena);

	n_artists = rena_database_get_artist_count (cdbase);
	n_albums = rena_database_get_album_count (cdbase);
	n_tracks = rena_database_get_track_count (cdbase);

	dialog = gtk_message_dialog_new(GTK_WINDOW(rena_application_get_window(rena)),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_INFO,
					GTK_BUTTONS_OK,
					"%s %d\n%s %d\n%s %d",
					_("Total Tracks:"),
					n_tracks,
					_("Total Artists:"),
					n_artists,
					_("Total Albums:"),
					n_albums);

	gtk_window_set_title(GTK_WINDOW(dialog), _("Statistics"));

	g_signal_connect (dialog, "response",
	                  G_CALLBACK (gtk_widget_destroy), NULL);

	gtk_widget_show_all(dialog);
}

/* Handler for the 'About' action in the Help menu */

static void home_action(GtkAction *action, RenaApplication *rena)
{
	const gchar *uri = "http://santelmotechnologies.github.io/rena";
	open_url(uri, rena_application_get_window(rena));
}

static void wiki_action(GtkAction *action, RenaApplication *rena)
{
	const gchar *uri = "https://github.com/santelmotechnologies/rena/wiki/Welcome-to-the-Rena-Wiki";
	open_url(uri, rena_application_get_window(rena));
}

static void community_action(GtkAction *action, RenaApplication *rena)
{
	const gchar *uri = "http://bbs.archlinux.org/viewtopic.php?id=46171";
	open_url(uri, rena_application_get_window(rena));
}

static void translate_action(GtkAction *action, RenaApplication *rena)
{
	const gchar *uri = "http://www.transifex.com/projects/p/Rena/";
	open_url(uri, rena_application_get_window(rena));
}

void about_action(GtkAction *action, RenaApplication *rena)
{
	rena_application_about_dialog(rena);
}

void
rena_menubar_connect_signals (GtkUIManager *menu_ui_manager, RenaApplication *rena)
{
	RenaPreferences *preferences;
	GtkActionGroup *main_actions;

	const GBindingFlags binding_flags =
		G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL;
 
	main_actions = gtk_action_group_new("Main Actions");

	gtk_action_group_set_translation_domain (main_actions, GETTEXT_PACKAGE);

	gtk_action_group_add_actions (main_actions,
	                              main_aentries,
	                              G_N_ELEMENTS(main_aentries),
	                              (gpointer)rena);
	gtk_action_group_add_toggle_actions (main_actions,
	                                     toggles_entries,
	                                     G_N_ELEMENTS(toggles_entries),
	                                     rena);

	gtk_window_add_accel_group (GTK_WINDOW(rena_application_get_window(rena)),
	                            gtk_ui_manager_get_accel_group(menu_ui_manager));

	gtk_ui_manager_insert_action_group (menu_ui_manager, main_actions, 0);

	/* Hide second sidebar */
	GtkAction *action_sidebar = gtk_ui_manager_get_action(menu_ui_manager, "/Menubar/ViewMenu/Lateral panel2");
	gtk_action_set_visible (action_sidebar, FALSE);

	/* Binding properties to Actions. */

	preferences = rena_application_get_preferences (rena);

	if (!rena_preferences_get_system_titlebar(preferences)) {
		GtkAction *fullscreen_action = rena_application_get_menu_action (rena, "/Menubar/ViewMenu/Fullscreen");
		gtk_action_set_sensitive (GTK_ACTION (fullscreen_action), FALSE);

		GtkAction *below_action = rena_application_get_menu_action (rena, "/Menubar/ViewMenu/Playback controls below");
		gtk_action_set_sensitive (GTK_ACTION (below_action), FALSE);
	}

	GtkAction *action_shuffle = gtk_ui_manager_get_action(menu_ui_manager, "/Menubar/PlaybackMenu/Shuffle");
	g_object_bind_property (preferences, "shuffle", action_shuffle, "active", binding_flags);

	GtkAction *action_repeat = gtk_ui_manager_get_action(menu_ui_manager,"/Menubar/PlaybackMenu/Repeat");
	g_object_bind_property (preferences, "repeat", action_repeat, "active", binding_flags);

	GtkAction *action_lateral1 = gtk_ui_manager_get_action(menu_ui_manager, "/Menubar/ViewMenu/Lateral panel1");
	g_object_bind_property (preferences, "lateral-panel", action_lateral1, "active", binding_flags);

	GtkAction *action_lateral2 = gtk_ui_manager_get_action(menu_ui_manager, "/Menubar/ViewMenu/Lateral panel2");
	g_object_bind_property (preferences, "secondary-lateral-panel", action_lateral2, "active", binding_flags);

	GtkAction *action_show_menubar = gtk_ui_manager_get_action(menu_ui_manager, "/Menubar/ViewMenu/Show menubar");
	g_object_bind_property (preferences, "show-menubar", action_show_menubar, "active", binding_flags);

	g_signal_connect (rena_application_get_database(rena), "PlaylistsChanged",
	                  G_CALLBACK(rena_menubar_update_playlist_changes), rena);
	rena_menubar_update_playlist_changes (rena_application_get_database(rena), rena);

	g_signal_connect (rena_application_get_database(rena), "PlaylistsChanged",
	                  G_CALLBACK(rena_gear_menu_update_playlist_changes), rena);
	rena_gear_menu_update_playlist_changes (rena_application_get_database(rena), rena);

	g_object_unref (main_actions);
}

GtkUIManager*
rena_menubar_new (void)
{
	GtkUIManager *main_menu = NULL;
	gchar *rena_accels_path = NULL;
	GError *error = NULL;

	main_menu = gtk_ui_manager_new();

	if (!gtk_ui_manager_add_ui_from_string(main_menu, main_menu_xml, -1, &error)) {
		g_critical("Unable to create main menu, err : %s", error->message);
	}

	/* Load menu accelerators edited */

	rena_accels_path = g_build_path(G_DIR_SEPARATOR_S, g_get_user_config_dir(), "/rena/accels.scm", NULL);
	gtk_accel_map_load (rena_accels_path);
	g_free (rena_accels_path);

	return main_menu;
}

/*
 * Menu on toolbar.
 */

/* Playback submenu. */

static void
rena_gmenu_prev (GSimpleAction *action,
                   GVariant      *parameter,
                   gpointer       user_data)
{
	RenaApplication *rena = user_data;
	rena_playback_prev_track (rena);
}

static void
rena_gmenu_playpause (GSimpleAction *action,
                        GVariant      *parameter,
                        gpointer       user_data)
{
	RenaApplication *rena = user_data;
	rena_playback_play_pause_resume (rena);
}

static void
rena_gmenu_stop (GSimpleAction *action,
                   GVariant      *parameter,
                   gpointer       user_data)
{
	RenaApplication *rena = user_data;
	rena_playback_stop (rena);
}

static void
rena_gmenu_next (GSimpleAction *action,
                        GVariant      *parameter,
                        gpointer       user_data)
{
	RenaApplication *rena = user_data;
	rena_playback_next_track (rena);
}

static void
rena_gmenu_edit (GSimpleAction *action,
                   GVariant      *parameter,
                   gpointer       user_data)
{
	RenaApplication *rena = user_data;
	rena_playback_edit_current_track (rena);
}

static void
rena_gmenu_quit (GSimpleAction *action,
                   GVariant      *parameter,
                   gpointer       user_data)
{
	RenaApplication *rena = user_data;

	rena_application_quit (rena);
}

/* Playlist submenu. */
static void
rena_gmenu_open (GSimpleAction *action,
                   GVariant      *parameter,
                   gpointer       user_data)
{
	RenaApplication *rena = user_data;
	rena_application_open_files (rena);
}

static void
rena_gmenu_location (GSimpleAction *action,
                       GVariant      *parameter,
                       gpointer       user_data)
{
	RenaApplication *rena = user_data;
	rena_application_add_location (rena);
}

static void
rena_gmenu_library (GSimpleAction *action,
                      GVariant      *parameter,
                      gpointer       user_data)
{
	RenaApplication *rena = user_data;
	rena_application_append_entery_libary (rena);
}

static void
rena_gmenu_remove (GSimpleAction *action,
                     GVariant      *parameter,
                     gpointer       user_data)
{
	RenaPlaylist *playlist;
	RenaApplication *rena = user_data;

	playlist = rena_application_get_playlist (rena);
	rena_playlist_remove_selection (playlist);
}

static void
rena_gmenu_crop (GSimpleAction *action,
                   GVariant      *parameter,
                   gpointer       user_data)
{
	RenaPlaylist *playlist;
	RenaApplication *rena = user_data;

	playlist = rena_application_get_playlist (rena);
	rena_playlist_crop_selection (playlist);
}

static void
rena_gmenu_playlist_export (GSimpleAction *action,
                              GVariant      *parameter,
                              gpointer       user_data)
{
	RenaPlaylist *playlist;
	RenaApplication *rena = user_data;

	playlist = rena_application_get_playlist (rena);

	export_current_playlist (NULL, playlist);
}

static void
rena_gmenu_playlist_save (GSimpleAction *action,
                            GVariant      *parameter,
                            gpointer       user_data)
{
	RenaPlaylist *playlist;
	RenaApplication *rena = user_data;

	playlist = rena_application_get_playlist (rena);

	save_current_playlist (NULL, playlist);
}


static gchar *
rena_database_get_playlist_by_order (RenaDatabase *cdbase, gint id)
{
	gchar *name = NULL;
	gint i = 0;

	const gchar *sql = "SELECT name FROM PLAYLIST WHERE name != ? ORDER BY name COLLATE NOCASE";

	RenaPreparedStatement *statement = rena_database_create_statement (cdbase, sql);
	rena_prepared_statement_bind_string (statement, 1, SAVE_PLAYLIST_STATE);
	while (rena_prepared_statement_step (statement)) {
		if (i++ == id)
			break;
	}
	name = g_strdup(rena_prepared_statement_get_string (statement, 0));
	rena_prepared_statement_free (statement);

	return name;
}

static void
rena_gmenu_playlist_append (GSimpleAction *action,
                              GVariant      *parameter,
                              gpointer       user_data)
{
	RenaApplication *rena = user_data;
	RenaPlaylist *playlist  = rena_application_get_playlist (rena);
	RenaDatabase *cdbase    = rena_playlist_get_database (playlist);

	const gchar *name         = g_action_get_name (G_ACTION(action));
	gchar *title              = rena_database_get_playlist_by_order(cdbase, atoi(name + strlen("playlist")));

	rena_playlist_save_playlist (playlist, title);

	g_free(title);
}

static void
rena_gmenu_selection_export (GSimpleAction *action,
                               GVariant      *parameter,
                               gpointer       user_data)
{
	RenaPlaylist *playlist;
	RenaApplication *rena = user_data;

	playlist = rena_application_get_playlist (rena);
	export_selected_playlist (NULL, playlist);
}

static void
rena_gmenu_selection_save (GSimpleAction *action,
                             GVariant      *parameter,
                             gpointer       user_data)
{
	RenaPlaylist *playlist;
	RenaApplication *rena = user_data;

	playlist = rena_application_get_playlist (rena);
	save_selected_playlist (NULL, playlist);
}

static void
rena_gmenu_selection_append (GSimpleAction *action,
                               GVariant      *parameter,
                               gpointer       user_data)
{
	RenaApplication *rena = user_data;
	RenaPlaylist *playlist  = rena_application_get_playlist (rena);
	RenaDatabase *cdbase    = rena_playlist_get_database (playlist);

	const gchar *name         = g_action_get_name (G_ACTION(action));
	gchar *title              = rena_database_get_playlist_by_order(cdbase, atoi(name + strlen("selection")));

	rena_playlist_save_selection (playlist, title);

	g_free(title);
}

static void
rena_gmenu_clear (GSimpleAction *action,
                    GVariant      *parameter,
                    gpointer       user_data)
{
	RenaPlaylist *playlist;
	RenaApplication *rena = user_data;

	playlist = rena_application_get_playlist (rena);
	rena_playlist_remove_all (playlist);
}

static void
rena_gmenu_search (GSimpleAction *action,
                     GVariant      *parameter,
                     gpointer       user_data)
{
	RenaApplication *rena = user_data;
	RenaPlaylist *playlist = rena_application_get_playlist (rena);
	rena_filter_dialog(playlist);
}

/* View submenu */

static void
rena_gmenu_jump_to_song (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       user_data)
{
	RenaPlaylist *playlist;

	RenaApplication *rena = user_data;
	playlist = rena_application_get_playlist (rena);

	rena_playlist_show_current_track (playlist);
}

/* Tools Submenu */

static void
rena_gmenu_equalizer (GSimpleAction *action,
                        GVariant      *parameter,
                        gpointer       user_data)
{
	RenaApplication *rena = user_data;
	GtkWidget *parent = rena_application_get_window (rena);
	RenaBackend *backend = rena_application_get_backend(rena);

	rena_equalizer_dialog_show (backend, parent);
}

static void
rena_gmenu_rescan_library (GSimpleAction *action,
                            GVariant      *parameter,
                            gpointer       user_data)
{
	RenaScanner *scanner;
	RenaApplication *rena = user_data;

	scanner = rena_application_get_scanner (rena);
	rena_scanner_scan_library (scanner);
}

static void
rena_gmenu_update_library (GSimpleAction *action,
                            GVariant      *parameter,
                            gpointer       user_data)
{
	RenaScanner *scanner;
	RenaApplication *rena = user_data;

	scanner = rena_application_get_scanner (rena);
	rena_scanner_update_library (scanner);
}

static void
rena_gmenu_show_statistic (GSimpleAction *action,
                             GVariant      *parameter,
                             gpointer       user_data)
{
	RenaApplication *rena = user_data;
	statistics_action (NULL, rena);
}

static void
rena_gmenu_show_preferences (GSimpleAction *action,
                               GVariant      *parameter,
                               gpointer       user_data)
{
	RenaPreferencesDialog *dialog;
	dialog = rena_preferences_dialog_get ();
	rena_preferences_dialog_show (dialog);
	g_object_unref (dialog);
}

/* Help Submenu */

static void
rena_gmenu_show_homepage (GSimpleAction *action,
                            GVariant      *parameter,
                            gpointer       user_data)
{
	RenaApplication *rena = user_data;
	const gchar *uri = "http://santelmotechnologies.github.io/rena";

	open_url (uri, rena_application_get_window(rena));
}

static void
rena_gmenu_show_wiki (GSimpleAction *action,
                        GVariant      *parameter,
                        gpointer       user_data)
{
	RenaApplication *rena = user_data;
	const gchar *uri = "https://github.com/santelmotechnologies/rena/wiki/Welcome-to-the-Rena-Wiki";

	open_url (uri, rena_application_get_window(rena));
}

static void
rena_gmenu_translate (GSimpleAction *action,
                        GVariant      *parameter,
                        gpointer       user_data)
{
	RenaApplication *rena = user_data;
	const gchar *uri = "http://www.transifex.net/projects/p/Rena/";
	open_url (uri, rena_application_get_window(rena));
}


static void
rena_gmenu_about (GSimpleAction *action,
                    GVariant      *parameter,
                    gpointer       user_data)
{
	RenaApplication *rena = user_data;
	rena_application_about_dialog (rena);
}

/*
 * Useful functions.
 */

void
rena_menubar_set_enable_action (GtkWindow  *window,
                                  const char *action_name,
                                  gboolean    enabled)
{
	GAction *action;
	action = g_action_map_lookup_action (G_ACTION_MAP (window), action_name);
	g_object_set (action, "enabled", enabled, NULL);
}

GMenu *
rena_menubar_get_menu_section (RenaApplication *rena,
                                 const char        *id)
{
	GObject *object;
	GtkBuilder *builder;

	builder = rena_application_get_menu_ui (rena);
	object = gtk_builder_get_object (builder, id);

	if (object == NULL || !G_IS_MENU (object))
		return NULL;

	return G_MENU (object);
}

static void
rena_menubar_emthy_menu_section (RenaApplication *rena,
                                   const char        *id)
{
	GMenu *menu;
	GtkBuilder *builder;

	builder = rena_application_get_menu_ui (rena);
	menu = G_MENU (gtk_builder_get_object (builder, id));

	while (g_menu_model_get_n_items (G_MENU_MODEL (menu)) > 0) {
		const char *action;
		g_menu_model_get_item_attribute (G_MENU_MODEL (menu), 0, G_MENU_ATTRIBUTE_ACTION, "s", &action);
		if (g_str_has_prefix (action, "win.")) {
			GVariant *target;

			target = g_menu_model_get_item_attribute_value (G_MENU_MODEL (menu), 0, G_MENU_ATTRIBUTE_TARGET, NULL);

			/* Don't remove actions that have a specific target */
			if (target == NULL) {
				GtkWindow *window;
				window = GTK_WINDOW(rena_application_get_window(rena));
				g_action_map_remove_action (G_ACTION_MAP (window), action + strlen ("win."));
			}
			else
				g_variant_unref (target);
		}
		g_menu_remove (G_MENU (menu), 0);
	}
}

void
rena_menubar_append_action (RenaApplication *rena,
                              const gchar       *placeholder,
                              GSimpleAction     *action,
                              GMenuItem         *item)
{
	GActionMap *map;
	GMenu *place;

	place = rena_menubar_get_menu_section (rena, placeholder);

	map = G_ACTION_MAP (rena_application_get_window(rena));

	g_action_map_add_action (map, G_ACTION (action));
	g_menu_append_item (G_MENU (place), item);
}

void
rena_menubar_remove_action (RenaApplication *rena,
                              const gchar       *placeholder,
                              const gchar       *action_name)
{
	GtkBuilder *builder;
	GActionMap *map;
	GMenu *menu;
	gchar *action;
	gboolean found = FALSE;
	gint i;

	builder = rena_application_get_menu_ui (rena);
	menu = G_MENU (gtk_builder_get_object (builder, placeholder));

	for (i = 0; i < g_menu_model_get_n_items (G_MENU_MODEL(menu)); i++)
	{
		if (g_menu_model_get_item_attribute (G_MENU_MODEL(menu), i, G_MENU_ATTRIBUTE_ACTION, "s", &action))
		{
			if (g_strcmp0 (action + strlen ("win."), action_name) == 0)
			{
				g_menu_remove (G_MENU (menu), i);

				map = G_ACTION_MAP (rena_application_get_window(rena));
				g_action_map_remove_action (map, action_name);
				found = TRUE;
			}
			g_free (action);
			if (found)
				break;
		}
	}
}

void
rena_menubar_append_submenu (RenaApplication  *rena,
                               const gchar        *placeholder,
                               const gchar        *xml_ui,
                               const gchar        *menu_id,
                               const gchar        *label,
                               gpointer            user_data)
{
	GtkBuilder *builder;
	GError *error = NULL;
	GMenuModel *menu;
	GMenu *section;
	GMenuItem *menu_item;

	builder = gtk_builder_new ();
	gtk_builder_add_from_string (builder, xml_ui, -1, &error);

	if (error) {
		g_print ("GtkBuilder error: %s", error->message);
		g_error_free (error);
		error = NULL;
	}

	section = rena_menubar_get_menu_section (rena, placeholder);
	menu = G_MENU_MODEL (gtk_builder_get_object (builder, menu_id));

	menu_item = g_menu_item_new_submenu (label, menu);
	g_menu_item_set_attribute (menu_item, "rena-merge-id", "s", menu_id);
	g_menu_insert_item (section, -1, menu_item);

	g_object_unref (menu_item);
	g_object_unref (builder);
}

void
rena_menubar_remove_by_id (RenaApplication *rena,
                             const gchar       *placeholder,
                             const gchar       *item_id)
{
	GtkBuilder *builder;
	GMenu *menu;
	gchar *id = NULL;
	gint i;

	builder = rena_application_get_menu_ui (rena);
	menu = G_MENU (gtk_builder_get_object (builder, placeholder));

	for (i = 0; i < g_menu_model_get_n_items (G_MENU_MODEL(menu)); i++)
	{
		if (g_menu_model_get_item_attribute (G_MENU_MODEL(menu), i, "rena-merge-id", "s", &id))
		{
			if (g_strcmp0 (id, item_id) == 0)
				g_menu_remove (G_MENU (menu), i);
			g_free (id);
		}
	}
}

gint
rena_menubar_append_plugin_action (RenaApplication *rena,
                                     GtkActionGroup    *action_group,
                                     const gchar       *menu_xml)
{
	GtkUIManager *ui_manager;
	GError *error = NULL;
	gint merge_id;

	ui_manager = rena_application_get_menu_ui_manager (rena);
	gtk_ui_manager_insert_action_group (ui_manager, action_group, -1);

	merge_id = gtk_ui_manager_add_ui_from_string (ui_manager,
	                                              menu_xml,
	                                              -1,
	                                              &error);

	if (error) {
		g_warning ("Adding plugin to menubar: %s", error->message);
		g_error_free (error);
	}

	return merge_id;
}

void
rena_menubar_remove_plugin_action (RenaApplication *rena,
                                     GtkActionGroup    *action_group,
                                     gint               merge_id)
{
	GtkUIManager * ui_manager = rena_application_get_menu_ui_manager (rena);

	gtk_ui_manager_remove_ui (ui_manager, merge_id);
	gtk_ui_manager_remove_action_group (ui_manager, action_group);
	g_object_unref (action_group);
}

GtkActionGroup *
rena_menubar_plugin_action_new (const gchar                *name,
                                  const GtkActionEntry       *action_entries,
                                  guint                       n_action_entries,
                                  const GtkToggleActionEntry *toggle_entries,
                                  guint                       n_toggle_entries,
                                  gpointer                    user_data)
{
	GtkActionGroup *action_group = gtk_action_group_new (name);
	gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);

	if (action_entries) {
		gtk_action_group_add_actions (action_group,
		                              action_entries,
		                              n_action_entries,
		                              user_data);
	}

	if (toggle_entries) {
		gtk_action_group_add_toggle_actions (action_group,
		                                     toggle_entries,
		                                     n_toggle_entries,
		                                     user_data);
	}

	return action_group;
}

static void
rena_gear_menu_update_playlist_changes (RenaDatabase *database, RenaApplication *rena)
{
	RenaDatabase *cdbase;
	GSimpleAction *action;
	GMenuItem *item;
	gchar *selection_name = NULL, *action_name = NULL;
	gint i = 0;

	rena_menubar_emthy_menu_section (rena, "selection-submenu");
	rena_menubar_emthy_menu_section (rena, "playlist-submenu");

	cdbase = rena_application_get_database (rena);

	const gchar *sql = "SELECT name FROM PLAYLIST WHERE name != ? ORDER BY name COLLATE NOCASE";
	RenaPreparedStatement *statement = rena_database_create_statement (cdbase, sql);

	rena_prepared_statement_bind_string (statement, 1, SAVE_PLAYLIST_STATE);
	while (rena_prepared_statement_step (statement)) {
		const gchar *name = rena_prepared_statement_get_string (statement, 0);

		/* Playlist */
		selection_name = g_strdup_printf ("playlist%d", i);

		action = g_simple_action_new (selection_name, NULL);
		g_signal_connect (G_OBJECT (action), "activate",
		                  G_CALLBACK (rena_gmenu_playlist_append), rena);
		action_name = g_strdup_printf ("win.%s", selection_name);

		item = g_menu_item_new (name, action_name);
		rena_menubar_append_action (rena, "playlist-submenu", action, item);
		g_object_unref (item);

		g_free(selection_name);
		g_free(action_name);

		/* Selection */
		selection_name = g_strdup_printf ("selection%d", i);

		action = g_simple_action_new (selection_name, NULL);
		g_signal_connect (G_OBJECT (action), "activate",
		                  G_CALLBACK (rena_gmenu_selection_append), rena);

		action_name = g_strdup_printf ("win.%s", selection_name);

		item = g_menu_item_new (name, action_name);
		rena_menubar_append_action (rena, "selection-submenu", action, item);
		g_object_unref (item);

		g_free(selection_name);
		g_free(action_name);

		i++;
		rena_process_gtk_events ();
	}
	rena_prepared_statement_free (statement);
}

/*
 * Bindigns Functions
 */

static void
activate_toggle (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
	GVariant *state;

	state = g_action_get_state (G_ACTION (action));
	g_action_change_state (G_ACTION (action), g_variant_new_boolean (!g_variant_get_boolean (state)));
	g_variant_unref (state);
}

static gboolean
binding_gboolean_to_variant (GBinding     *binding,
                             const GValue *from_value,
                             GValue       *to_value,
                             gpointer      user_data)
{
	GVariant *vvalue = g_variant_new_boolean (g_value_get_boolean (from_value));
	g_value_set_variant (to_value, vvalue);
	return TRUE;
}

static gboolean
binding_variant_to_gboolean (GBinding     *binding,
                             const GValue *from_value,
                             GValue       *to_value,
                             gpointer      user_data)
{
	gboolean vbool = g_variant_get_boolean(g_value_get_variant(from_value));
	g_value_set_boolean (to_value, vbool);
	return TRUE;
}

/*
 * Menu definitions.
 */

static GActionEntry win_entries[] = {
	/* Playback submenu. */
	{ "prev",             rena_gmenu_prev,             NULL, NULL,    NULL },
	{ "play",             rena_gmenu_playpause,        NULL, NULL,    NULL },
	{ "stop",             rena_gmenu_stop,             NULL, NULL,    NULL },
	{ "next",             rena_gmenu_next,             NULL, NULL,    NULL },
	{ "shuffle",          activate_toggle,               NULL, "false", NULL },
	{ "repeat",           activate_toggle,               NULL, "false", NULL },
	{ "edit",             rena_gmenu_edit,             NULL, NULL,    NULL },
	{ "quit",             rena_gmenu_quit,             NULL, NULL,    NULL },
	/* Playlist submenu. */
	{ "open",             rena_gmenu_open,             NULL, NULL,    NULL },
	{ "location",         rena_gmenu_location,         NULL, NULL,    NULL },
	{ "library",          rena_gmenu_library,          NULL, NULL,    NULL },
	{ "remove",           rena_gmenu_remove,           NULL, NULL,    NULL },
	{ "crop",             rena_gmenu_crop,             NULL, NULL,    NULL },
	{ "clear",            rena_gmenu_clear,            NULL, NULL,    NULL },
	{ "export_playlist",  rena_gmenu_playlist_export,  NULL, NULL,    NULL },
	{ "new_playlist",     rena_gmenu_playlist_save,    NULL, NULL,    NULL },
	{ "export_selection", rena_gmenu_selection_export, NULL, NULL,    NULL },
	{ "new_selection",    rena_gmenu_selection_save,   NULL, NULL,    NULL },
	{ "search",           rena_gmenu_search,           NULL, NULL,    NULL },
	/* View Submenu */
	{ "sidebar1",         activate_toggle,               NULL, "false", NULL },
	{ "sidebar2",         activate_toggle,               NULL, "false", NULL },
	{ "menubar",          activate_toggle,               NULL, "true",  NULL },
	{ "jump-song",        rena_gmenu_jump_to_song,     NULL, NULL,    NULL },
	/* Tools submenu */
	{ "equalizer",        rena_gmenu_equalizer,        NULL, NULL,    NULL },
	{ "lib-rescan",       rena_gmenu_rescan_library,   NULL, NULL,    NULL },
	{ "lib-update",       rena_gmenu_update_library,   NULL, NULL,    NULL },
	{ "statistics",       rena_gmenu_show_statistic,   NULL, NULL,    NULL },
	{ "preferences",      rena_gmenu_show_preferences, NULL, NULL,    NULL },
	/* Help submenu */
	{ "homepage",         rena_gmenu_show_homepage,    NULL, NULL,    NULL },
	{ "wiki",             rena_gmenu_show_wiki,        NULL, NULL,    NULL },
	{ "translate",        rena_gmenu_translate,        NULL, NULL,    NULL },
	{ "about",            rena_gmenu_about,            NULL, NULL,    NULL }
};

GtkBuilder *
rena_gmenu_toolbar_new (RenaApplication *rena)
{
	RenaPreferences *preferences;
	GtkBuilder *builder;
	GActionMap *map;
	GAction *action;
	GError *error = NULL;

	const GBindingFlags binding_flags =
		G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL;

	builder = gtk_builder_new ();
	gtk_builder_add_from_string (builder, rena_window_ui, -1, &error);
	if (error) {
		g_print ("GtkBuilder error: %s", error->message);
		g_error_free (error);
		error = NULL;
	}

	/* Get the action map */

	map = G_ACTION_MAP (rena_application_get_window(rena));

	/* Add the menu */

	g_action_map_add_action_entries (G_ACTION_MAP (map),
	                                 win_entries, G_N_ELEMENTS (win_entries), rena);

	/* Insensitive second sidebar */

	action = g_action_map_lookup_action (map, "sidebar2");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);

	/* Binding properties to Actions. */

	preferences = rena_application_get_preferences (rena);

	action = g_action_map_lookup_action (map, "shuffle");
	g_object_bind_property_full (preferences, "shuffle",
	                             action, "state",
	                             binding_flags,
	                             binding_gboolean_to_variant,
	                             binding_variant_to_gboolean,
	                             NULL,
	                             NULL);

	action = g_action_map_lookup_action (map, "repeat");
	g_object_bind_property_full (preferences, "repeat",
	                             action, "state",
	                             binding_flags,
	                             binding_gboolean_to_variant,
	                             binding_variant_to_gboolean,
	                             NULL,
	                             NULL);

	action = g_action_map_lookup_action (map, "sidebar1");
	g_object_bind_property_full (preferences, "lateral-panel",
	                             action, "state",
	                             binding_flags,
	                             binding_gboolean_to_variant,
	                             binding_variant_to_gboolean,
	                             NULL,
	                             NULL);
	action = g_action_map_lookup_action (map, "sidebar2");
	g_object_bind_property_full (preferences, "secondary-lateral-panel",
	                             action, "state",
	                             binding_flags,
	                             binding_gboolean_to_variant,
	                             binding_variant_to_gboolean,
	                             NULL,
	                             NULL);

	action = g_action_map_lookup_action (map, "menubar");
	g_object_bind_property_full (preferences, "show-menubar",
	                             action, "state",
	                             binding_flags,
	                             binding_gboolean_to_variant,
	                             binding_variant_to_gboolean,
	                             NULL,
	                             NULL);

	return builder;
}
