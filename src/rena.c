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

#include "rena.h"

#if defined(GETTEXT_PACKAGE)
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#include <glib.h>
#include <locale.h> /* require LC_ALL */
#include <libintl.h>
#include <tag_c.h>

#include "rena-hig.h"
#include "rena-window.h"
#include "rena-playback.h"
#include "rena-musicobject-mgmt.h"
#include "rena-menubar.h"
#include "rena-file-utils.h"
#include "rena-utils.h"
#include "rena-music-enum.h"
#include "rena-playlists-mgmt.h"
#include "rena-database-provider.h"

#ifdef G_OS_WIN32
#include "win32/win32dep.h"
#endif

struct _RenaApplication {
	GtkApplication base_instance;

	/* Main window and icon */

	GtkWidget         *mainwindow;

	/* Main stuff */

	RenaBackend          *backend;
	RenaPreferences      *preferences;
	RenaDatabase         *cdbase;
	RenaDatabaseProvider *provider;
	RenaArtCache         *art_cache;
	RenaMusicEnum        *enum_map;

	RenaScanner     *scanner;

	RenaPreferencesDialog *setting_dialog;

	/* Main widgets */

	GtkUIManager      *menu_ui_manager;
	GtkBuilder        *menu_ui;
	RenaToolbar     *toolbar;
	GtkWidget         *infobox;
	GtkWidget         *overlay;
	GtkWidget         *pane1;
	GtkWidget         *pane2;
	RenaSidebar     *sidebar1;
	GtkWidget         *main_stack;
	RenaSidebar     *sidebar2;
	RenaLibraryPane *library;
	RenaPlaylist    *playlist;
	RenaStatusbar   *statusbar;

	RenaStatusIcon  *status_icon;

	GBinding          *sidebar2_binding;

#ifdef HAVE_LIBPEAS
	RenaPluginsEngine *plugins_engine;
#endif
};

G_DEFINE_TYPE (RenaApplication, rena_application, GTK_TYPE_APPLICATION);

/*
 * Some calbacks..
 */

/* Handler for the 'Open' item in the File menu */

static void
rena_open_files_dialog_close_button_cb (GtkWidget *widget, gpointer data)
{
    gtk_widget_destroy(GTK_WIDGET(data));
}

static void
rena_open_files_dialog_add_button_cb (GtkWidget *widget, gpointer data)
{
	RenaPlaylist *playlist;
	GSList *files = NULL, *l;
	gboolean add_recursively;
	GList *mlist = NULL;

	GtkWidget *window = g_object_get_data(data, "window");
	GtkWidget *chooser = g_object_get_data(data, "chooser");
	GtkWidget *toggle = g_object_get_data(data, "toggle-button");
	RenaApplication *rena = g_object_get_data(data, "rena");

	RenaPreferences *preferences = rena_application_get_preferences (rena);

	add_recursively = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle));
	rena_preferences_set_add_recursively (preferences, add_recursively);

	gchar *last_folder = gtk_file_chooser_get_current_folder ((GtkFileChooser *) chooser);
	rena_preferences_set_last_folder (preferences, last_folder);
	g_free (last_folder);

	files = gtk_file_chooser_get_filenames((GtkFileChooser *) chooser);

	gtk_widget_destroy(window);

	if (files) {
		for (l = files; l != NULL; l = l->next) {
			mlist = append_mobj_list_from_unknown_filename(mlist, l->data);
		}
		g_slist_free_full(files, g_free);

		playlist = rena_application_get_playlist (rena);
		rena_playlist_append_mobj_list (playlist, mlist);
		g_list_free (mlist);
	}
}

static gboolean
rena_open_files_dialog_keypress (GtkWidget   *dialog,
                                   GdkEventKey *event,
                                   gpointer     data)
{
    if (event->keyval == GDK_KEY_Escape) {
        gtk_widget_destroy(dialog);
        return TRUE;
    }
    return FALSE;
}

void
rena_application_open_files (RenaApplication *rena)
{
	RenaPreferences *preferences;
	GtkWidget *window, *hbox, *vbox, *chooser, *bbox, *toggle, *close_button, *add_button;
	gpointer storage;
	gint i = 0;
	GtkFileFilter *media_filter, *playlist_filter, *all_filter;
	const gchar *last_folder = NULL;

	/* Create a file chooser dialog */

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_title(GTK_WINDOW(window), (_("Select a file to play")));
	gtk_window_set_default_size(GTK_WINDOW(window), 700, 450);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_widget_set_name (GTK_WIDGET(window), "GtkFileChooserDialog");
	gtk_container_set_border_width(GTK_CONTAINER(window), 0);

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_name (GTK_WIDGET(vbox), "dialog-vbox1");

	gtk_container_add(GTK_CONTAINER(window), vbox);

	chooser = gtk_file_chooser_widget_new(GTK_FILE_CHOOSER_ACTION_OPEN);

	/* Set various properties */

	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(chooser), TRUE);

	preferences = rena_application_get_preferences (rena);
	last_folder = rena_preferences_get_last_folder (preferences);
	if (string_is_not_empty(last_folder))
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), last_folder);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);

	toggle = gtk_check_button_new_with_label(_("Add files recursively"));
	if(rena_preferences_get_add_recursively (preferences))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), TRUE);

	bbox = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(bbox), 4);

	close_button = gtk_button_new_with_mnemonic (_("_Cancel"));
	add_button = gtk_button_new_with_mnemonic (_("_Add"));
	gtk_container_add(GTK_CONTAINER(bbox), close_button);
	gtk_container_add(GTK_CONTAINER(bbox), add_button);

	gtk_box_pack_start(GTK_BOX(hbox), toggle, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), bbox, FALSE, FALSE, 0);

	gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), chooser, TRUE, TRUE, 0);

	/* Create file filters  */

	media_filter = gtk_file_filter_new();
	gtk_file_filter_set_name(GTK_FILE_FILTER(media_filter), _("Supported media"));

	while (mime_wav[i])
		gtk_file_filter_add_mime_type(GTK_FILE_FILTER(media_filter),
		                              mime_wav[i++]);
	i = 0;
	while (mime_mpeg[i])
		gtk_file_filter_add_mime_type(GTK_FILE_FILTER(media_filter),
		                              mime_mpeg[i++]);
	i = 0;
	while (mime_flac[i])
		gtk_file_filter_add_mime_type(GTK_FILE_FILTER(media_filter),
		                              mime_flac[i++]);
	i = 0;
	while (mime_ogg[i])
		gtk_file_filter_add_mime_type(GTK_FILE_FILTER(media_filter),
		                              mime_ogg[i++]);

	i = 0;
	while (mime_asf[i])
		gtk_file_filter_add_mime_type(GTK_FILE_FILTER(media_filter),
		                              mime_asf[i++]);
	i = 0;
	while (mime_mp4[i])
		gtk_file_filter_add_mime_type(GTK_FILE_FILTER(media_filter),
		                              mime_mp4[i++]);
	i = 0;
	while (mime_ape[i])
		gtk_file_filter_add_mime_type(GTK_FILE_FILTER(media_filter),
		                              mime_ape[i++]);
	i = 0;
	while (mime_tracker[i])
	 gtk_file_filter_add_mime_type(GTK_FILE_FILTER(media_filter),
		                              mime_tracker[i++]);

	#ifdef HAVE_PLPARSER
	i = 0;
	while (mime_playlist[i])
		gtk_file_filter_add_mime_type(GTK_FILE_FILTER(media_filter),
		                              mime_playlist[i++]);
	i = 0;
	while (mime_dual[i])
		gtk_file_filter_add_mime_type(GTK_FILE_FILTER(media_filter),
		                              mime_dual[i++]);
	#else
	gtk_file_filter_add_pattern(GTK_FILE_FILTER(media_filter), "*.m3u");
	gtk_file_filter_add_pattern(GTK_FILE_FILTER(media_filter), "*.M3U");

	gtk_file_filter_add_pattern(GTK_FILE_FILTER(media_filter), "*.pls");
	gtk_file_filter_add_pattern(GTK_FILE_FILTER(media_filter), "*.PLS");

	gtk_file_filter_add_pattern(GTK_FILE_FILTER(media_filter), "*.xspf");
	gtk_file_filter_add_pattern(GTK_FILE_FILTER(media_filter), "*.XSPF");

	gtk_file_filter_add_pattern(GTK_FILE_FILTER(media_filter), "*.wax");
	gtk_file_filter_add_pattern(GTK_FILE_FILTER(media_filter), "*.WAX");
	#endif

	playlist_filter = gtk_file_filter_new();

	#ifdef HAVE_PLPARSER
	i = 0;
	while (mime_playlist[i])
		gtk_file_filter_add_mime_type(GTK_FILE_FILTER(playlist_filter),
		                              mime_playlist[i++]);
	i = 0;
	while (mime_dual[i])
		gtk_file_filter_add_mime_type(GTK_FILE_FILTER(playlist_filter),
		                              mime_dual[i++]);
	#else
	gtk_file_filter_add_pattern(GTK_FILE_FILTER(playlist_filter), "*.m3u");
	gtk_file_filter_add_pattern(GTK_FILE_FILTER(playlist_filter), "*.M3U");

	gtk_file_filter_add_pattern(GTK_FILE_FILTER(playlist_filter), "*.pls");
	gtk_file_filter_add_pattern(GTK_FILE_FILTER(playlist_filter), "*.PLS");

	gtk_file_filter_add_pattern(GTK_FILE_FILTER(playlist_filter), "*.xspf");
	gtk_file_filter_add_pattern(GTK_FILE_FILTER(playlist_filter), "*.XSPF");

	gtk_file_filter_add_pattern(GTK_FILE_FILTER(playlist_filter), "*.wax");
	gtk_file_filter_add_pattern(GTK_FILE_FILTER(playlist_filter), "*.WAX");
	#endif

	gtk_file_filter_set_name(GTK_FILE_FILTER(playlist_filter), _("Playlists"));

	all_filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (GTK_FILE_FILTER(all_filter), _("All files"));
	gtk_file_filter_add_pattern (GTK_FILE_FILTER(all_filter), "*");

	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER(chooser),
	                             GTK_FILE_FILTER(media_filter));
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER(chooser),
	                             GTK_FILE_FILTER(playlist_filter));
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER(chooser),
	                             GTK_FILE_FILTER(all_filter));

	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(chooser),
	                            GTK_FILE_FILTER(media_filter));

	storage = g_object_new(G_TYPE_OBJECT, NULL);
	g_object_set_data(storage, "window", window);
	g_object_set_data(storage, "chooser", chooser);
	g_object_set_data(storage, "toggle-button", toggle);
	g_object_set_data(storage, "rena", rena);

	g_signal_connect (add_button, "clicked",
	                  G_CALLBACK(rena_open_files_dialog_add_button_cb), storage);
	g_signal_connect (chooser, "file-activated",
	                  G_CALLBACK(rena_open_files_dialog_add_button_cb), storage);
	g_signal_connect (close_button, "clicked",
	                  G_CALLBACK(rena_open_files_dialog_close_button_cb), window);
	g_signal_connect (window, "destroy",
	                  G_CALLBACK(gtk_widget_destroy), window);
	g_signal_connect (window, "key-press-event",
	                  G_CALLBACK(rena_open_files_dialog_keypress), NULL);

	gtk_window_set_transient_for(GTK_WINDOW (window), GTK_WINDOW(rena_application_get_window(rena)));
	gtk_window_set_destroy_with_parent (GTK_WINDOW (window), TRUE);

	gtk_widget_show_all(window);
}

/* Build a dialog to get a new playlist name */

static char *
totem_open_location_set_from_clipboard (GtkWidget *open_location)
{
	GtkClipboard *clipboard;
	gchar *clipboard_content;

	/* Initialize the clipboard and get its content */
	clipboard = gtk_clipboard_get_for_display (gtk_widget_get_display (GTK_WIDGET (open_location)), GDK_SELECTION_CLIPBOARD);
	clipboard_content = gtk_clipboard_wait_for_text (clipboard);

	/* Check clipboard for "://". If it exists, return it */
	if (clipboard_content != NULL && strcmp (clipboard_content, "") != 0)
	{
		if (g_strrstr (clipboard_content, "://") != NULL)
			return clipboard_content;
	}

	g_free (clipboard_content);
	return NULL;
}

void
rena_application_add_location (RenaApplication *rena)
{
	RenaPlaylist *playlist;
	RenaDatabase *cdbase;
	RenaMusicobject *mobj;
	GtkWidget *dialog, *table, *uri_entry, *label_name, *name_entry;
	const gchar *uri = NULL, *name = NULL;
	gchar *clipboard_location = NULL, *real_name = NULL;
	GSList *list = NULL, *i = NULL;
	GList *mlist = NULL;
	guint row = 0;
	gint result;

	/* Create dialog window */

	table = rena_hig_workarea_table_new ();
	rena_hig_workarea_table_add_section_title(table, &row, _("Enter the URL of an internet radio stream"));

	uri_entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(uri_entry), 255);

	rena_hig_workarea_table_add_wide_control (table, &row, uri_entry);

	label_name = gtk_label_new_with_mnemonic(_("Give it a name to save"));
	name_entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(name_entry), 255);

	rena_hig_workarea_table_add_row (table, &row, label_name, name_entry);

	/* Get item from clipboard to fill GtkEntry */
	clipboard_location = totem_open_location_set_from_clipboard (uri_entry);
	if (clipboard_location != NULL && strcmp (clipboard_location, "") != 0) {
		gtk_entry_set_text (GTK_ENTRY(uri_entry), clipboard_location);
		g_free (clipboard_location);
	}

	dialog = gtk_dialog_new_with_buttons (_("Add a location"),
	                                      GTK_WINDOW(rena_application_get_window(rena)),
	                                      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	                                      _("_Cancel"), GTK_RESPONSE_CANCEL,
	                                      _("_Ok"), GTK_RESPONSE_ACCEPT,
	                                      NULL);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), table);

	gtk_window_set_default_size(GTK_WINDOW (dialog), 450, -1);

	gtk_entry_set_activates_default (GTK_ENTRY(uri_entry), TRUE);
	gtk_entry_set_activates_default (GTK_ENTRY(name_entry), TRUE);

	gtk_widget_show_all(dialog);

	result = gtk_dialog_run(GTK_DIALOG(dialog));
	switch(result) {
	case GTK_RESPONSE_ACCEPT:
		if (gtk_entry_get_text_length (GTK_ENTRY(uri_entry)))
			uri = gtk_entry_get_text(GTK_ENTRY(uri_entry));

		playlist = rena_application_get_playlist (rena);

		if (string_is_not_empty(uri)) {
			if (gtk_entry_get_text_length (GTK_ENTRY(name_entry)))
				name = gtk_entry_get_text(GTK_ENTRY(name_entry));

			#ifdef HAVE_PLPARSER
			list = rena_totem_pl_parser_parse_from_uri (uri);
			#else
			list = g_slist_append (list, g_strdup(uri));
			#endif

			for (i = list; i != NULL; i = i->next) {
				if (string_is_not_empty(name))
					real_name = new_radio (playlist, i->data, name);

				mobj = new_musicobject_from_location (i->data, real_name);
				mlist = g_list_append(mlist, mobj);

				if (real_name) {
					g_free (real_name);
					real_name = NULL;
				}
				g_free(i->data);
			}
			g_slist_free(list);

			/* Append playlist and save on database */

			rena_playlist_append_mobj_list (playlist, mlist);
			g_list_free(mlist);

			cdbase = rena_application_get_database (rena);
			rena_database_change_playlists_done (cdbase);
		}
		break;
	case GTK_RESPONSE_CANCEL:
		break;
	default:
		break;
	}
	gtk_widget_destroy(dialog);

	return;
}

/* Handler for 'Add All' action in the Tools menu */

void
rena_application_append_entery_libary (RenaApplication *rena)
{
	RenaPlaylist *playlist;
	RenaDatabase *cdbase;
	GList *list = NULL;
	RenaMusicobject *mobj;

	/* Query and insert entries */

	set_watch_cursor (rena_application_get_window(rena));

	cdbase = rena_application_get_database (rena);

	const gchar *sql = "SELECT id FROM LOCATION";
	RenaPreparedStatement *statement = rena_database_create_statement (cdbase, sql);

	while (rena_prepared_statement_step (statement)) {
		gint location_id = rena_prepared_statement_get_int (statement, 0);
		mobj = new_musicobject_from_db (cdbase, location_id);

		if (G_LIKELY(mobj))
			list = g_list_prepend (list, mobj);
		else
			g_warning ("Unable to retrieve details for"
			            " location_id : %d",
			            location_id);

		rena_process_gtk_events ();
	}

	rena_prepared_statement_free (statement);

	remove_watch_cursor (rena_application_get_window(rena));

	if (list) {
		list = g_list_reverse(list);
		playlist = rena_application_get_playlist (rena);
		rena_playlist_append_mobj_list (playlist, list);
		g_list_free(list);
	}
}

/* Handler for the 'About' action in the Help menu */

void
rena_application_about_dialog (RenaApplication *rena)
{
	GtkWidget *mainwindow;

	mainwindow = rena_application_get_window (rena);

	const gchar *authors[] = {
		"Santelmo Technologies <santelmotechnologies@gmail.com>",
		NULL
	};

	gtk_show_about_dialog(GTK_WINDOW(mainwindow),
	                      "logo-icon-name", "rena",
	                      "authors", authors,
	                      "comments", "A lightweight GTK+ music player",
	                      "copyright", "(C) 2024 Santelmo Technologies \n Pragha (C) 2009-2019 Matias \n Consonance (C) 2007-2009 Sujith",
	                      "license-type", GTK_LICENSE_GPL_3_0,
	                      "name", PACKAGE_NAME,
	                      "version", PACKAGE_VERSION,
	                      NULL);
}

static void
rena_library_pane_append_tracks (RenaLibraryPane *library, RenaApplication *rena)
{
	GList *list = NULL;
	list = rena_library_pane_get_mobj_list (library);
	if (list) {
		rena_playlist_append_mobj_list (rena->playlist,
			                              list);
		g_list_free(list);
	}
}

static void
rena_library_pane_replace_tracks (RenaLibraryPane *library, RenaApplication *rena)
{
	GList *list = NULL;
	list = rena_library_pane_get_mobj_list (library);
	if (list) {
		rena_playlist_remove_all (rena->playlist);

		rena_playlist_append_mobj_list (rena->playlist,
			                              list);
		g_list_free(list);
	}
}

static void
rena_library_pane_replace_tracks_and_play (RenaLibraryPane *library, RenaApplication *rena)
{
	GList *list = NULL;
	list = rena_library_pane_get_mobj_list (library);
	if (list) {
		rena_playlist_remove_all (rena->playlist);

		rena_playlist_append_mobj_list (rena->playlist,
			                              list);

		if (rena_backend_get_state (rena->backend) != ST_STOPPED)
			rena_playback_next_track(rena);
		else
			rena_playback_play_pause_resume(rena);

		g_list_free(list);
	}
}

static void
rena_library_pane_addto_playlist_and_play (RenaLibraryPane *library, RenaApplication *rena)
{
	GList *list = NULL;
	list = rena_library_pane_get_mobj_list (library);
	if (list) {
		rena_playlist_append_mobj_list(rena->playlist, list);
		rena_playlist_activate_unique_mobj(rena->playlist, g_list_first(list)->data);
		
		g_list_free(list);
	}
}

static void
rena_playlist_update_change_tags (RenaPlaylist *playlist, gint changed, RenaMusicobject *mobj, RenaApplication *rena)
{
	RenaBackend *backend;
	RenaToolbar *toolbar;
	RenaMusicobject *cmobj = NULL;

	backend = rena_application_get_backend (rena);

	if(rena_backend_get_state (backend) != ST_STOPPED) {
		cmobj = rena_backend_get_musicobject (backend);
		rena_update_musicobject_change_tag (cmobj, changed, mobj);

		toolbar = rena_application_get_toolbar (rena);
		rena_toolbar_set_title (toolbar, cmobj);
	}
}

static void
rena_playlist_update_statusbar_playtime (RenaPlaylist *playlist, RenaApplication *rena)
{
	RenaStatusbar *statusbar;
	gint total_playtime = 0, no_tracks = 0;
	gchar *str, *tot_str;

	if(rena_playlist_is_changing(playlist))
		return;

	total_playtime = rena_playlist_get_total_playtime (playlist);
	no_tracks = rena_playlist_get_no_tracks (playlist);

	tot_str = convert_length_str(total_playtime);
	str = g_strdup_printf("%i %s - %s",
	                      no_tracks,
	                      ngettext("Track", "Tracks", no_tracks),
	                      tot_str);

	CDEBUG(DBG_VERBOSE, "Updating status bar with new playtime: %s", tot_str);

	statusbar = rena_application_get_statusbar (rena);
	rena_statusbar_set_main_text(statusbar, str);

	g_free(tot_str);
	g_free(str);
}

static void
rena_art_cache_changed_handler (RenaArtCache *cache, RenaApplication *rena)
{
	RenaBackend *backend;
	RenaToolbar *toolbar;
	RenaMusicobject *mobj = NULL;
	gchar *album_art_path = NULL;
	const gchar *artist = NULL, *album = NULL;

	backend = rena_application_get_backend (rena);
	if (rena_backend_get_state (backend) != ST_STOPPED) {
		mobj = rena_backend_get_musicobject (backend);

		artist = rena_musicobject_get_artist (mobj);
		album = rena_musicobject_get_album (mobj);

		album_art_path = rena_art_cache_get_album_uri (cache, artist, album);

		if (album_art_path) {
			toolbar = rena_application_get_toolbar (rena);
			rena_toolbar_set_image_album_art (toolbar, album_art_path);
			g_free (album_art_path);
		}
	}
}

static void
rena_libary_list_changed_cb (RenaPreferences *preferences, RenaApplication *rena)
{
	GtkWidget *infobar = create_info_bar_update_music (rena);
	rena_window_add_widget_to_infobox (rena, infobar);
}

static void
rena_application_provider_want_update (RenaDatabaseProvider *provider,
                                         gint                    provider_id,
                                         RenaApplication      *rena)
{
	RenaDatabase *database;
	RenaScanner *scanner;
	RenaPreparedStatement *statement;
	const gchar *sql, *provider_type = NULL;

	sql = "SELECT name FROM provider_type WHERE id IN (SELECT type FROM provider WHERE id = ?)";

	database = rena_application_get_database (rena);
	statement = rena_database_create_statement (database, sql);
	rena_prepared_statement_bind_int (statement, 1, provider_id);
	if (rena_prepared_statement_step (statement))
		provider_type = rena_prepared_statement_get_string (statement, 0);

	if (g_ascii_strcasecmp (provider_type, "local") == 0)
	{
		scanner = rena_application_get_scanner (rena);
		rena_scanner_update_library (scanner);
	}
	rena_prepared_statement_free (statement);
}

static void
rena_application_provider_want_upgrade (RenaDatabaseProvider *provider,
                                          gint                    provider_id,
                                          RenaApplication      *rena)
{
	RenaDatabase *database;
	RenaScanner *scanner;
	RenaPreparedStatement *statement;
	const gchar *sql, *provider_type = NULL;

	sql = "SELECT name FROM provider_type WHERE id IN (SELECT type FROM provider WHERE id = ?)";

	database = rena_application_get_database (rena);
	statement = rena_database_create_statement (database, sql);
	rena_prepared_statement_bind_int (statement, 1, provider_id);
	if (rena_prepared_statement_step (statement))
		provider_type = rena_prepared_statement_get_string (statement, 0);

	if (g_ascii_strcasecmp (provider_type, "local") == 0)
	{
		scanner = rena_application_get_scanner (rena);
		rena_scanner_scan_library (scanner);
	}
	rena_prepared_statement_free (statement);
}

static void
rena_need_restart_cb (RenaPreferences *preferences, RenaApplication *rena)
{
	GtkWidget *infobar = rena_info_bar_need_restart (rena);
	rena_window_add_widget_to_infobox (rena, infobar);
}

static void
rena_system_titlebar_changed_cb (RenaPreferences *preferences, GParamSpec *pspec, RenaApplication *rena)
{
	RenaToolbar *toolbar;
	GtkWidget *window, *parent, *menubar;
	GtkAction *action;

	window = rena_application_get_window (rena);
	toolbar = rena_application_get_toolbar (rena);
	menubar = rena_application_get_menubar (rena);
	g_object_ref(toolbar);

	parent  = gtk_widget_get_parent (GTK_WIDGET(menubar));

	if (rena_preferences_get_system_titlebar (preferences)) {
		gtk_widget_hide(GTK_WIDGET(window));

		action = rena_application_get_menu_action (rena,
			"/Menubar/ViewMenu/Fullscreen");
		gtk_action_set_sensitive (GTK_ACTION (action), TRUE);

		action = rena_application_get_menu_action (rena,
			"/Menubar/ViewMenu/Playback controls below");
		gtk_action_set_sensitive (GTK_ACTION (action), TRUE);

		gtk_window_set_titlebar (GTK_WINDOW (window), NULL);
		gtk_window_set_title (GTK_WINDOW(window), _("Rena Music Player"));

		gtk_box_pack_start (GTK_BOX(parent), GTK_WIDGET(toolbar),
		                    FALSE, FALSE, 0);
		gtk_box_reorder_child(GTK_BOX(parent), GTK_WIDGET(toolbar), 1);

		rena_toolbar_set_style(toolbar, TRUE);

		gtk_widget_show(GTK_WIDGET(window));

	}
	else {
		gtk_widget_hide(GTK_WIDGET(window));

		rena_preferences_set_controls_below(preferences, FALSE);

		action = rena_application_get_menu_action (rena,
			"/Menubar/ViewMenu/Fullscreen");
		gtk_action_set_sensitive (GTK_ACTION (action), FALSE);

		action = rena_application_get_menu_action (rena,
			"/Menubar/ViewMenu/Playback controls below");
		gtk_action_set_sensitive (GTK_ACTION (action), FALSE);

		gtk_container_remove (GTK_CONTAINER(parent), GTK_WIDGET(toolbar));
		gtk_window_set_titlebar (GTK_WINDOW (window), GTK_WIDGET(toolbar));

		rena_toolbar_set_style(toolbar, FALSE);

		gtk_widget_show(GTK_WIDGET(window));
	}
	g_object_unref(toolbar);
}


static void
rena_enum_map_removed_handler (RenaMusicEnum *enum_map, gint enum_removed, RenaApplication *rena)
{
	rena_playlist_crop_music_type (rena->playlist, enum_removed);
}

/*
 * Some public actions.
 */

RenaPreferences *
rena_application_get_preferences (RenaApplication *rena)
{
	return rena->preferences;
}

RenaDatabase *
rena_application_get_database (RenaApplication *rena)
{
	return rena->cdbase;
}

RenaArtCache *
rena_application_get_art_cache (RenaApplication *rena)
{
	return rena->art_cache;
}

RenaBackend *
rena_application_get_backend (RenaApplication *rena)
{
	return rena->backend;
}

#ifdef HAVE_LIBPEAS
RenaPluginsEngine *
rena_application_get_plugins_engine (RenaApplication *rena)
{
	return rena->plugins_engine;
}
#endif

RenaScanner *
rena_application_get_scanner (RenaApplication *rena)
{
	return rena->scanner;
}

GtkWidget *
rena_application_get_window (RenaApplication *rena)
{
	return rena->mainwindow;
}

RenaPlaylist *
rena_application_get_playlist (RenaApplication *rena)
{
	return rena->playlist;
}

RenaLibraryPane *
rena_application_get_library (RenaApplication *rena)
{
	return rena->library;
}

RenaPreferencesDialog *
rena_application_get_preferences_dialog (RenaApplication *rena)
{
	return rena->setting_dialog;
}

RenaToolbar *
rena_application_get_toolbar (RenaApplication *rena)
{
	return rena->toolbar;
}

GtkWidget *
rena_application_get_overlay (RenaApplication *rena)
{
	return rena->overlay;
}

RenaSidebar *
rena_application_get_first_sidebar (RenaApplication *rena)
{
	return rena->sidebar1;
}

GtkWidget *
rena_application_get_main_stack (RenaApplication *rena)
{
	return rena->main_stack;
}

RenaSidebar *
rena_application_get_second_sidebar (RenaApplication *rena)
{
	return rena->sidebar2;
}

RenaStatusbar *
rena_application_get_statusbar (RenaApplication *rena)
{
	return rena->statusbar;
}

RenaStatusIcon *
rena_application_get_status_icon (RenaApplication *rena)
{
	return rena->status_icon;
}

GtkBuilder *
rena_application_get_menu_ui (RenaApplication *rena)
{
	return rena->menu_ui;
}

GtkUIManager *
rena_application_get_menu_ui_manager (RenaApplication *rena)
{
	return rena->menu_ui_manager;
}

GtkAction *
rena_application_get_menu_action (RenaApplication *rena, const gchar *path)
{
	GtkUIManager *ui_manager = rena_application_get_menu_ui_manager (rena);

	return gtk_ui_manager_get_action (ui_manager, path);
}

GtkWidget *
rena_application_get_menu_action_widget (RenaApplication *rena, const gchar *path)
{
	GtkUIManager *ui_manager = rena_application_get_menu_ui_manager (rena);

	return gtk_ui_manager_get_widget (ui_manager, path);
}

GtkWidget *
rena_application_get_menubar (RenaApplication *rena)
{
	GtkUIManager *ui_manager = rena_application_get_menu_ui_manager (rena);

	return gtk_ui_manager_get_widget (ui_manager, "/Menubar");
}

GtkWidget *
rena_application_get_infobox_container (RenaApplication *rena)
{
	return rena->infobox;
}

GtkWidget *
rena_application_get_first_pane (RenaApplication *rena)
{
	return rena->pane1;
}

GtkWidget *
rena_application_get_second_pane (RenaApplication *rena)
{
	return rena->pane2;
}

gboolean
rena_application_is_first_run (RenaApplication *rena)
{
	return string_is_empty (rena_preferences_get_installed_version (rena->preferences));
}

static void
rena_application_construct_window (RenaApplication *rena)
{
	/* Main window */

	rena->mainwindow = gtk_application_window_new (GTK_APPLICATION (rena));

	gtk_window_set_icon_name (GTK_WINDOW(rena->mainwindow), "rena");


	/* Get all widgets instances */

	rena->menu_ui_manager = rena_menubar_new ();
	rena->menu_ui = rena_gmenu_toolbar_new (rena);
	rena->toolbar = rena_toolbar_new ();
	rena->infobox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	rena->overlay = gtk_overlay_new ();
	rena->pane1 = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
	rena->main_stack = gtk_stack_new ();
	rena->pane2 = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
	rena->sidebar1 = rena_sidebar_new ();
	rena->sidebar2 = rena_sidebar_new ();
	rena->library = rena_library_pane_new ();
	rena->playlist = rena_playlist_new ();
	rena->statusbar = rena_statusbar_get ();
	rena->scanner = rena_scanner_new();

	rena->status_icon = rena_status_icon_new (rena);

	rena_menubar_connect_signals (rena->menu_ui_manager, rena);

	/* Contruct the window. */

	rena_window_new (rena);

	gtk_window_set_title (GTK_WINDOW(rena->mainwindow),
	                      _("Rena Music Player"));
}

static void
rena_application_dispose (GObject *object)
{
	RenaApplication *rena = RENA_APPLICATION (object);

	CDEBUG(DBG_INFO, "Cleaning up");

#ifdef HAVE_LIBPEAS
	if (rena->plugins_engine) {
		g_object_unref (rena->plugins_engine);
		rena->plugins_engine = NULL;
	}
#endif

	if (rena->setting_dialog) {
		// Explicit destroy dialog.
		// TODO: Evaluate if needed.
		gtk_widget_destroy (GTK_WIDGET(rena->setting_dialog));
		rena->setting_dialog = NULL;
	}

	if (rena->backend) {
		g_object_unref (rena->backend);
		rena->backend = NULL;
	}
	if (rena->art_cache) {
		g_object_unref (rena->art_cache);
		rena->art_cache = NULL;
	}
	if (rena->enum_map) {
		g_object_unref (rena->enum_map);
		rena->enum_map = NULL;
	}
	if (rena->scanner) {
		rena_scanner_free (rena->scanner);
		rena->scanner = NULL;
	}
	if (rena->menu_ui_manager) {
		g_object_unref (rena->menu_ui_manager);
		rena->menu_ui_manager = NULL;
	}
	if (rena->menu_ui) {
		g_object_unref (rena->menu_ui);
		rena->menu_ui = NULL;
	}

	/* Save Preferences and database. */

	if (rena->preferences) {
		g_object_unref (rena->preferences);
		rena->preferences = NULL;
	}
	if (rena->provider) {
		g_object_unref (rena->provider);
		rena->provider = NULL;
	}
	if (rena->cdbase) {
		g_object_unref (rena->cdbase);
		rena->cdbase = NULL;
	}

	G_OBJECT_CLASS (rena_application_parent_class)->dispose (object);
}

static void
rena_application_startup (GApplication *application)
{
	RenaToolbar *toolbar;
	RenaPlaylist *playlist;
	const gchar *version = NULL;
	const gchar *desktop = NULL;
	gint playlist_id = 0;

	const GBindingFlags binding_flags =
		G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL;

	RenaApplication *rena = RENA_APPLICATION (application);

	G_APPLICATION_CLASS (rena_application_parent_class)->startup (application);

	/* Allocate memory for simple structures */

	rena->preferences = rena_preferences_get();

	rena->cdbase = rena_database_get();
	if (rena_database_start_successfully(rena->cdbase) == FALSE) {
		g_error("Unable to init music dbase");
	}

	version = rena_preferences_get_installed_version (rena->preferences);
	if (string_is_not_empty (version) && (g_ascii_strcasecmp (version, "0.1") < 0)) {
		CDEBUG(DBG_INFO, "Compatibilize database to new version.");
		rena_database_compatibilize_version (rena->cdbase);
	}

	playlist_id = rena_database_find_playlist (rena->cdbase, _("Favorites"));
	if (playlist_id == 0)
		rena_database_add_new_playlist (rena->cdbase, _("Favorites"));

	rena->provider = rena_database_provider_get ();
	g_signal_connect (rena->provider, "want-upgrade",
	                  G_CALLBACK(rena_application_provider_want_upgrade), rena);
	g_signal_connect (rena->provider, "want-update",
	                  G_CALLBACK(rena_application_provider_want_update), rena);

	rena->enum_map = rena_music_enum_get ();
	g_signal_connect (rena->enum_map, "enum-removed",
	                  G_CALLBACK(rena_enum_map_removed_handler), rena);

#ifdef HAVE_LIBPEAS
	rena->plugins_engine = rena_plugins_engine_new (G_OBJECT(rena));
#endif

	rena->art_cache = rena_art_cache_get ();
	g_signal_connect (rena->art_cache, "cache-changed",
	                  G_CALLBACK(rena_art_cache_changed_handler), rena);

	rena->backend = rena_backend_new ();

	g_signal_connect (rena->backend, "finished",
	                  G_CALLBACK(rena_backend_finished_song), rena);
	g_signal_connect (rena->backend, "tags-changed",
	                  G_CALLBACK(rena_backend_tags_changed), rena);

	g_signal_connect (rena->backend, "error",
	                  G_CALLBACK(gui_backend_error_update_current_playlist_cb), rena);
	g_signal_connect (rena->backend, "error",
	                 G_CALLBACK(rena_backend_finished_error), rena);
	g_signal_connect (rena->backend, "notify::state",
	                  G_CALLBACK (rena_menubar_update_playback_state_cb), rena);

	/*
	 * Collect widgets and construct the window.
	 */

	rena_application_construct_window (rena);

	/* Connect Signals and Bindings. */

	toolbar = rena->toolbar;
	g_signal_connect_swapped (toolbar, "prev",
	                          G_CALLBACK(rena_playback_prev_track), rena);
	g_signal_connect_swapped (toolbar, "play",
	                          G_CALLBACK(rena_playback_play_pause_resume), rena);
	g_signal_connect_swapped (toolbar, "stop",
	                          G_CALLBACK(rena_playback_stop), rena);
	g_signal_connect_swapped (toolbar, "next",
	                          G_CALLBACK(rena_playback_next_track), rena);
	g_signal_connect (toolbar, "unfull-activated",
	                  G_CALLBACK(rena_window_unfullscreen), rena);
	g_signal_connect (toolbar, "album-art-activated",
	                  G_CALLBACK(rena_playback_show_current_album_art), rena);
	g_signal_connect_swapped (toolbar, "track-info-activated",
	                          G_CALLBACK(rena_playback_edit_current_track), rena);
	g_signal_connect (toolbar, "track-progress-activated",
	                  G_CALLBACK(rena_playback_seek_fraction), rena);
	g_signal_connect (toolbar, "favorite-toggle",
	                  G_CALLBACK(rena_playback_toogle_favorite), rena);

	playlist = rena->playlist;
	g_signal_connect (playlist, "playlist-set-track",
	                  G_CALLBACK(rena_playback_set_playlist_track), rena);
	g_signal_connect (playlist, "playlist-change-tags",
	                  G_CALLBACK(rena_playlist_update_change_tags), rena);
	g_signal_connect (playlist, "playlist-changed",
	                  G_CALLBACK(rena_playlist_update_statusbar_playtime), rena);
	rena_playlist_update_statusbar_playtime (playlist, rena);

	g_signal_connect (rena->library, "library-append-playlist",
	                  G_CALLBACK(rena_library_pane_append_tracks), rena);
	g_signal_connect (rena->library, "library-replace-playlist",
	                  G_CALLBACK(rena_library_pane_replace_tracks), rena);
	g_signal_connect (rena->library, "library-replace-playlist-and-play",
	                  G_CALLBACK(rena_library_pane_replace_tracks_and_play), rena);
	g_signal_connect (rena->library, "library-addto-playlist-and-play",
	                  G_CALLBACK(rena_library_pane_addto_playlist_and_play), rena);
	                  
	g_signal_connect (G_OBJECT(rena->mainwindow), "window-state-event",
	                  G_CALLBACK(rena_toolbar_window_state_event), toolbar);
	g_signal_connect (G_OBJECT(toolbar), "notify::timer-remaining-mode",
	                  G_CALLBACK(rena_toolbar_show_ramaning_time_cb), rena->backend);

	g_signal_connect (rena->backend, "notify::state",
	                  G_CALLBACK(rena_toolbar_playback_state_cb), toolbar);
	g_signal_connect (rena->backend, "tick",
	                 G_CALLBACK(rena_toolbar_update_playback_progress), toolbar);
	g_signal_connect (rena->backend, "buffering",
	                  G_CALLBACK(rena_toolbar_update_buffering_cb), toolbar);

	g_signal_connect (rena->backend, "notify::state",
	                  G_CALLBACK (update_current_playlist_view_playback_state_cb), rena->playlist);

	g_object_bind_property (rena->backend, "volume",
	                        toolbar, "volume",
	                        binding_flags);

	g_object_bind_property (rena->preferences, "timer-remaining-mode",
	                        toolbar, "timer-remaining-mode",
	                        binding_flags);

	g_signal_connect (rena->preferences, "LibraryChanged",
	                  G_CALLBACK (rena_libary_list_changed_cb), rena);
	g_signal_connect (rena->preferences, "NeedRestart",
	                  G_CALLBACK (rena_need_restart_cb), rena);

	g_signal_connect (rena->preferences, "notify::system-titlebar",
	                  G_CALLBACK (rena_system_titlebar_changed_cb), rena);

	rena->sidebar2_binding =
		g_object_bind_property (rena->preferences, "secondary-lateral-panel",
		                        rena->sidebar2, "visible",
		                        binding_flags);

	rena->setting_dialog = rena_preferences_dialog_get ();
	rena_preferences_dialog_set_parent (rena->setting_dialog, GTK_WIDGET (rena->mainwindow));

	#ifdef HAVE_LIBPEAS
	gboolean sidebar2_visible = // FIXME: Hack to allow hide sidebar when init.
		rena_preferences_get_secondary_lateral_panel(rena->preferences);

	rena_plugins_engine_startup (rena->plugins_engine);

	rena_preferences_set_secondary_lateral_panel(rena->preferences,
	                                               sidebar2_visible);
	#endif

	/* If first run and the desktop is gnome adapts style. */

	if (rena_application_is_first_run (rena)) {
		desktop = g_getenv ("XDG_CURRENT_DESKTOP");
		if (desktop && (g_strcmp0(desktop, "GNOME") == 0) &&
			gdk_screen_is_composited (gdk_screen_get_default())) {
			rena_preferences_set_system_titlebar (rena->preferences, FALSE);
			rena_preferences_set_toolbar_size (rena->preferences, GTK_ICON_SIZE_SMALL_TOOLBAR);
			rena_preferences_set_show_menubar (rena->preferences, FALSE);
		}
	}

	/* Forse update menubar and toolbar playback actions */

	rena_menubar_update_playback_state_cb (rena->backend, NULL, rena);
	rena_toolbar_playback_state_cb (rena->backend, NULL, rena->toolbar);

	/* Finally fill the library and the playlist */

	rena_init_gui_state (rena);
}

static void
rena_application_shutdown (GApplication *application)
{
	RenaApplication *rena = RENA_APPLICATION (application);

	CDEBUG(DBG_INFO, "Rena shutdown: Saving curret state.");

	if (rena_preferences_get_restore_playlist (rena->preferences))
		rena_playlist_save_playlist_state (rena->playlist);

	rena_window_save_settings (rena);

	rena_playback_stop (rena);

	/* Shutdown plugins can hide sidebar before save settings. */
	if (rena->sidebar2_binding) {
		g_object_unref (rena->sidebar2_binding);
		rena->sidebar2_binding = NULL;
	}

#ifdef HAVE_LIBPEAS
	rena_plugins_engine_shutdown (rena->plugins_engine);
#endif

	gtk_widget_destroy (rena->mainwindow);

	G_APPLICATION_CLASS (rena_application_parent_class)->shutdown (application);
}

static void
rena_application_activate (GApplication *application)
{
	RenaApplication *rena = RENA_APPLICATION (application);

	CDEBUG(DBG_INFO, G_STRFUNC);

	gtk_window_present (GTK_WINDOW (rena->mainwindow));
}

static void
rena_application_open (GApplication *application, GFile **files, gint n_files, const gchar *hint)
{
	RenaApplication *rena = RENA_APPLICATION (application);
	gint i;
	GList *mlist = NULL;

	for (i = 0; i < n_files; i++) {
		gchar *path = g_file_get_path (files[i]);
		mlist = append_mobj_list_from_unknown_filename (mlist, path);
		g_free (path);
	}

	if (mlist) {
		rena_playlist_append_mobj_list (rena->playlist, mlist);
		g_list_free (mlist);
	}

	gtk_window_present (GTK_WINDOW (rena->mainwindow));
}

static int
rena_application_command_line (GApplication *application, GApplicationCommandLine *command_line)
{
	RenaApplication *rena = RENA_APPLICATION (application);
	int ret = 0;
	gint argc;

	gchar **argv = g_application_command_line_get_arguments (command_line, &argc);

	if (argc <= 1) {
		rena_application_activate (application);
		goto exit;
	}

	ret = handle_command_line (rena, command_line, argc, argv);

exit:
	g_strfreev (argv);

	return ret;
}

//it's used for --help and --version
static gboolean
rena_application_local_command_line (GApplication *application, gchar ***arguments, int *exit_status)
{
	RenaApplication *rena = RENA_APPLICATION (application);

	gchar **argv = *arguments;
	gint argc = g_strv_length (argv);

	*exit_status = handle_command_line (rena, NULL, argc, argv);

	return FALSE;
}

void
rena_application_quit (RenaApplication *rena)
{
	g_application_quit (G_APPLICATION (rena));
}

static void
rena_application_class_init (RenaApplicationClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GApplicationClass *application_class = G_APPLICATION_CLASS (class);

	object_class->dispose = rena_application_dispose;

	application_class->startup = rena_application_startup;
	application_class->shutdown = rena_application_shutdown;
	application_class->activate = rena_application_activate;
	application_class->open = rena_application_open;
	application_class->command_line = rena_application_command_line;
	application_class->local_command_line = rena_application_local_command_line;
}

static void
rena_application_init (RenaApplication *rena)
{
}

RenaApplication *
rena_application_new ()
{
	return g_object_new (RENA_TYPE_APPLICATION,
	                     "application-id", "io.github.rena_music_player",
	                     "flags", G_APPLICATION_HANDLES_COMMAND_LINE | G_APPLICATION_HANDLES_OPEN,
	                     NULL);
}
