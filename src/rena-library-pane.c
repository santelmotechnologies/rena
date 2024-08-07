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

#include "rena-library-pane.h"

#if defined(GETTEXT_PACKAGE)
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#include <glib.h>
#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>

#include "rena-playback.h"

#include "rena-utils.h"
#include "rena-playlists-mgmt.h"
#include "rena-search-entry.h"
#include "rena-tagger.h"
#include "rena-tags-dialog.h"
#include "rena-tags-mgmt.h"
#include "rena-musicobject-mgmt.h"
#include "rena-database.h"
#include "rena-database-provider.h"
#include "rena-dnd.h"

#ifdef G_OS_WIN32
#include "win32/win32dep.h"
#endif

#include "rena-window-ui.h"

struct _RenaLibraryPane {
	GtkBox           __parent__;

	/* Global database and preferences instances */
	RenaDatabase    *cdbase;
	RenaPreferences *preferences;

	/* Tree view */
	GtkTreeStore      *library_store;
	GtkWidget         *library_tree;
	GtkWidget         *search_entry;
	GtkWidget         *pane_title;

	/* Tree view order. TODO: Rework and remove it. */
	GSList            *library_tree_nodes;

	/* Useful flags */
	gboolean           dragging;
	gboolean           view_change;

	/* Filter stuff */
	gchar             *filter_entry;
	guint              filter_id;
	gboolean           filter_active;
	guint              pulse_id;

	/* Fixbuf used on library tree. */
	GdkPixbuf         *pixbuf_artist;
	GdkPixbuf         *pixbuf_album;
	GdkPixbuf         *pixbuf_track;
	GdkPixbuf         *pixbuf_genre;
	GdkPixbuf         *pixbuf_dir;

	/* Menu */
	GtkBuilder        *builder;
	GSimpleActionGroup *actions;
};

enum
{
	LIBRARY_APPEND_PLAYLIST,
	LIBRARY_REPLACE_PLAYLIST,
	LIBRARY_REPLACE_PLAYLIST_AND_PLAY,
	LIBRARY_ADDTO_PLAYLIST_AND_PLAY,
	LAST_SIGNAL
};

static int signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE(RenaLibraryPane, rena_library_pane, GTK_TYPE_BOX)

/* Node types in library view */

typedef enum {
	NODE_CATEGORY_PLAYLIST,
	NODE_CATEGORY_RADIO,
	NODE_CATEGORY_PROVIDER,
	NODE_FOLDER,
	NODE_GENRE,
	NODE_ARTIST,
	NODE_ALBUM,
	NODE_TRACK,
	NODE_BASENAME,
	NODE_PLAYLIST,
	NODE_RADIO
} LibraryNodeType;

/* Columns in Library view */

enum library_columns {
	L_PIXBUF,
	L_NODE_DATA,
	L_NODE_BOLD,
	L_NODE_TYPE,
	L_DATABASE_ID,
	L_MACH,
	L_VISIBILE,
	N_L_COLUMNS
};

typedef enum {
	RENA_RESPONSE_SKIP,
	RENA_RESPONSE_SKIP_ALL,
	RENA_RESPONSE_DELETE_ALL
} RenaDeleteResponseType;

#define RENA_BUTTON_SKIP       _("_Skip")
#define RENA_BUTTON_SKIP_ALL   _("S_kip All")
#define RENA_BUTTON_DELETE_ALL _("Delete _All")

/*
 * Some prototypes
 */

static void rena_library_pane_rename_item     (RenaLibraryPane *library);
static void rena_library_pane_remove_item     (RenaLibraryPane *library);
static void rena_library_pane_export_playlist (RenaLibraryPane *library);
static void rena_library_pane_edit_tags       (RenaLibraryPane *library);
static void rena_library_pane_delete_from_hdd (RenaLibraryPane *library);
static void rena_library_pane_delete_from_db  (RenaLibraryPane *library);
static void rena_library_pane_upgrade_library (RenaLibraryPane *library);
static void rena_library_pane_update_library  (RenaLibraryPane *library);
static void rena_library_pane_remove_library  (RenaLibraryPane *library);

/*
 * Order menu callbacks
 */

static void
rena_library_pane_expand_action (GSimpleAction *action,
                                   GVariant      *parameter,
                                   gpointer       user_data)
{
	RenaLibraryPane *library = RENA_LIBRARY_PANE (user_data);
	gtk_tree_view_expand_all (GTK_TREE_VIEW(library->library_tree));
}

static void
rena_library_pane_collapse_action (GSimpleAction *action,
                                     GVariant      *parameter,
                                     gpointer       user_data)
{
	RenaLibraryPane *library = RENA_LIBRARY_PANE (user_data);
	gtk_tree_view_collapse_all(GTK_TREE_VIEW(library->library_tree));
}

static void
rena_library_pane_folder_action (GSimpleAction *action,
                                   GVariant      *parameter,
                                   gpointer       user_data)
{
	RenaLibraryPane *library = RENA_LIBRARY_PANE (user_data);
	rena_preferences_set_library_style(library->preferences, FOLDERS);
}

static void
rena_library_pane_artist_action (GSimpleAction *action,
                                   GVariant      *parameter,
                                   gpointer       user_data)
{
	RenaLibraryPane *library = RENA_LIBRARY_PANE (user_data);
	rena_preferences_set_library_style(library->preferences, ARTIST);
}

static void
rena_library_pane_album_action (GSimpleAction *action,
                                  GVariant      *parameter,
                                  gpointer       user_data)
{
	RenaLibraryPane *library = RENA_LIBRARY_PANE (user_data);
	rena_preferences_set_library_style(library->preferences, ALBUM);
}

static void
rena_library_pane_genre_action (GSimpleAction *action,
                                  GVariant      *parameter,
                                  gpointer       user_data)
{
	RenaLibraryPane *library = RENA_LIBRARY_PANE (user_data);
	rena_preferences_set_library_style(library->preferences, GENRE);
}

static void
rena_library_pane_artist_album_action (GSimpleAction *action,
                                         GVariant      *parameter,
                                         gpointer       user_data)
{
	RenaLibraryPane *library = RENA_LIBRARY_PANE (user_data);
	rena_preferences_set_library_style(library->preferences, ARTIST_ALBUM);
}

static void
rena_library_pane_genre_album_action (GSimpleAction *action,
                                        GVariant      *parameter,
                                        gpointer       user_data)
{
	RenaLibraryPane *library = RENA_LIBRARY_PANE (user_data);
	rena_preferences_set_library_style(library->preferences, GENRE_ALBUM);
}

static void
rena_library_pane_genre_artist_action (GSimpleAction *action,
                                         GVariant      *parameter,
                                         gpointer       user_data)
{
	RenaLibraryPane *library = RENA_LIBRARY_PANE (user_data);
	rena_preferences_set_library_style(library->preferences, GENRE_ARTIST);
}

static void
rena_library_pane_genre_artist_album_action (GSimpleAction *action,
                                               GVariant      *parameter,
                                               gpointer       user_data)
{
	RenaLibraryPane *library = RENA_LIBRARY_PANE (user_data);
	rena_preferences_set_library_style(library->preferences, GENRE_ARTIST_ALBUM);
}

/*
 * Tree menu callbacks
 */

static void
rena_library_pane_append_action (GSimpleAction *action,
                                   GVariant      *parameter,
                                   gpointer       user_data)
{
	RenaLibraryPane *library = RENA_LIBRARY_PANE (user_data);
	g_signal_emit (library, signals[LIBRARY_APPEND_PLAYLIST], 0);
}

static void
rena_library_pane_replace_action (GSimpleAction *action,
                                    GVariant      *parameter,
                                    gpointer       user_data)
{
	RenaLibraryPane *library = RENA_LIBRARY_PANE (user_data);
	g_signal_emit (library, signals[LIBRARY_REPLACE_PLAYLIST], 0);
}

static void
rena_library_pane_replace_and_play_action (GSimpleAction *action,
                                             GVariant      *parameter,
                                             gpointer       user_data)
{
	RenaLibraryPane *library = RENA_LIBRARY_PANE (user_data);
	g_signal_emit (library, signals[LIBRARY_ADDTO_PLAYLIST_AND_PLAY], 0);
}

static void
rena_library_pane_append_and_play_action (GSimpleAction *action,
                                            GVariant      *parameter,
                                            gpointer       user_data)
{
	RenaLibraryPane *library = RENA_LIBRARY_PANE (user_data);
	g_signal_emit (library, signals[LIBRARY_REPLACE_PLAYLIST_AND_PLAY], 0);
}

static void
rena_library_pane_rename_action (GSimpleAction *action,
                                   GVariant      *parameter,
                                   gpointer       user_data)
{
	RenaLibraryPane *library = RENA_LIBRARY_PANE (user_data);
	rena_library_pane_rename_item (library);
}

static void
rena_library_pane_delete_action (GSimpleAction *action,
                                   GVariant      *parameter,
                                   gpointer       user_data)
{
	RenaLibraryPane *library = RENA_LIBRARY_PANE (user_data);
	rena_library_pane_remove_item (library);
}

static void
rena_library_pane_export_action (GSimpleAction *action,
                                   GVariant      *parameter,
                                   gpointer       user_data)
{
	RenaLibraryPane *library = RENA_LIBRARY_PANE (user_data);
	rena_library_pane_export_playlist (library);
}

static void
rena_library_pane_edit_action (GSimpleAction *action,
                                 GVariant      *parameter,
                                 gpointer       user_data)
{
	RenaLibraryPane *library = RENA_LIBRARY_PANE (user_data);
	rena_library_pane_edit_tags (library);
}

static void
rena_library_pane_trash_action (GSimpleAction *action,
                                  GVariant      *parameter,
                                  gpointer       user_data)
{
	RenaLibraryPane *library = RENA_LIBRARY_PANE (user_data);
	rena_library_pane_delete_from_hdd (library);
}

static void
rena_library_pane_remove_action (GSimpleAction *action,
                                   GVariant      *parameter,
                                   gpointer       user_data)
{
	RenaLibraryPane *library = RENA_LIBRARY_PANE (user_data);
	rena_library_pane_delete_from_db  (library);
}

static void
rena_library_pane_rescan_action (GSimpleAction *action,
                                   GVariant      *parameter,
                                   gpointer       user_data)
{
	RenaLibraryPane *library = RENA_LIBRARY_PANE (user_data);
	rena_library_pane_upgrade_library (library);
}

static void
rena_library_pane_update_action (GSimpleAction *action,
                                   GVariant      *parameter,
                                   gpointer       user_data)
{
	RenaLibraryPane *library = RENA_LIBRARY_PANE (user_data);
	rena_library_pane_update_library (library);
}

static void
rena_library_pane_remove_provider_action (GSimpleAction *action,
                                            GVariant      *parameter,
                                            gpointer       user_data)
{
	RenaLibraryPane *library = RENA_LIBRARY_PANE (user_data);
	rena_library_pane_remove_library (library);
}

static const GActionEntry library_menu_aentries[] = {
	{ "expand",             rena_library_pane_expand_action,             NULL, NULL, NULL },
	{ "collapse",           rena_library_pane_collapse_action,           NULL, NULL, NULL },
	{ "folder",             rena_library_pane_folder_action,             NULL, NULL, NULL },
	{ "artist",             rena_library_pane_artist_action,             NULL, NULL, NULL },
	{ "album",              rena_library_pane_album_action,              NULL, NULL, NULL },
	{ "genre",              rena_library_pane_genre_action,              NULL, NULL, NULL },
	{ "artist_album",       rena_library_pane_artist_album_action,       NULL, NULL, NULL },
	{ "genre_album",        rena_library_pane_genre_album_action,        NULL, NULL, NULL },
	{ "genre_artist",       rena_library_pane_genre_artist_action,       NULL, NULL, NULL },
	{ "genre_artist_album", rena_library_pane_genre_artist_album_action, NULL, NULL, NULL },
	{ "append",             rena_library_pane_append_action,             NULL, NULL, NULL },
	{ "replace",            rena_library_pane_replace_action,            NULL, NULL, NULL },
	{ "replace_play",       rena_library_pane_replace_and_play_action,   NULL, NULL, NULL },
	{ "append_play",        rena_library_pane_append_and_play_action,    NULL, NULL, NULL },
	{ "rename",             rena_library_pane_rename_action,             NULL, NULL, NULL },
	{ "delete",             rena_library_pane_delete_action,             NULL, NULL, NULL },
	{ "export",             rena_library_pane_export_action,             NULL, NULL, NULL },
	{ "edit",               rena_library_pane_edit_action,               NULL, NULL, NULL },
	{ "trash",              rena_library_pane_trash_action,              NULL, NULL, NULL },
	{ "remove",             rena_library_pane_remove_action,             NULL, NULL, NULL },
	{ "rescan",             rena_library_pane_rescan_action,             NULL, NULL, NULL },
	{ "update",             rena_library_pane_update_action,             NULL, NULL, NULL },
	{ "remove_lib",         rena_library_pane_remove_provider_action,    NULL, NULL, NULL }
};


static void
rena_library_expand_categories(RenaLibraryPane *clibrary);

static gint
get_library_icon_size (void);


/* Returns TRUE if any of the childs of p_iter matches node_data. iter
 * and p_iter must be created outside this function */

static gboolean find_child_node(const gchar *node_data, GtkTreeIter *iter,
	GtkTreeIter *p_iter, GtkTreeModel *model)
{
	gchar *data = NULL;
	gboolean valid;
	gint cmp;

	valid = gtk_tree_model_iter_children(model, iter, p_iter);

	while (valid) {
		gtk_tree_model_get(model, iter, L_NODE_DATA, &data, -1);
		if (data) {
			cmp = g_ascii_strcasecmp (data, node_data);
			if (cmp == 0) {
				g_free(data);
				return TRUE;
			}
			else if (cmp > 0) {
				g_free(data);
				return FALSE;
			}
		g_free(data);
		}
		valid = gtk_tree_model_iter_next(model, iter);
	}
	return FALSE;
}

/* Prepend a child (iter) to p_iter with given data. NOTE that iter
 * and p_iter must be created outside this function */

static void
library_store_prepend_node(GtkTreeModel *model,
                           GtkTreeIter *iter,
                           GtkTreeIter *p_iter,
                           GdkPixbuf *pixbuf,
                           const gchar *node_data,
                           int node_type,
                           int location_id)
{
	gtk_tree_store_prepend(GTK_TREE_STORE(model), iter, p_iter);

	gtk_tree_store_set (GTK_TREE_STORE(model), iter,
	                    L_PIXBUF, pixbuf,
	                    L_NODE_DATA, node_data,
	                    L_NODE_BOLD, PANGO_WEIGHT_NORMAL,
	                    L_NODE_TYPE, node_type,
	                    L_DATABASE_ID, location_id,
	                    L_MACH, FALSE,
	                    L_VISIBILE, TRUE,
	                    -1);
}

static void
add_child_node_folder(GtkTreeModel *model,
		      GtkTreeIter *iter,
		      GtkTreeIter *p_iter,
		      const gchar *node_data,
		      RenaLibraryPane *clibrary)
{
	GtkTreeIter l_iter;
	gchar *data = NULL;
	gint l_node_type;
	gboolean valid;

	/* Find position of the last directory that is a child of p_iter */
	valid = gtk_tree_model_iter_children(model, &l_iter, p_iter);
	while (valid) {
		gtk_tree_model_get(model, &l_iter, L_NODE_TYPE, &l_node_type, -1);
		if (l_node_type != NODE_FOLDER)
			break;
		gtk_tree_model_get(model, &l_iter, L_NODE_DATA, &data, -1);
		if (g_ascii_strcasecmp(data, node_data) >= 0) {
			g_free(data);
			break;
		}
		g_free(data);

		valid = gtk_tree_model_iter_next(model, &l_iter);
	}

	/* Insert the new folder after the last subdirectory by order */
	gtk_tree_store_insert_before (GTK_TREE_STORE(model), iter, p_iter, valid ? &l_iter : NULL);
	gtk_tree_store_set (GTK_TREE_STORE(model), iter, L_PIXBUF, clibrary->pixbuf_dir,
	                    L_NODE_DATA, node_data,
	                    L_NODE_BOLD, PANGO_WEIGHT_NORMAL,
	                    L_NODE_TYPE, NODE_FOLDER,
	                    L_DATABASE_ID, 0,
	                    L_MACH, FALSE,
	                    L_VISIBILE, TRUE,
	                    -1);
}

/* Appends a child (iter) to p_iter with given data. NOTE that iter
 * and p_iter must be created outside this function */

static void
add_child_node_file(GtkTreeModel *model,
		    GtkTreeIter *iter,
		    GtkTreeIter *p_iter,
		    const gchar *node_data,
		    int location_id,
		    RenaLibraryPane *clibrary)
{
	GtkTreeIter l_iter;
	gchar *data = NULL;
	gint l_node_type;
	gboolean valid;

	/* Find position of the last file that is a child of p_iter */
	valid = gtk_tree_model_iter_children(model, &l_iter, p_iter);
	while (valid) {
		gtk_tree_model_get(model, &l_iter, L_NODE_TYPE, &l_node_type, -1);
		gtk_tree_model_get(model, &l_iter, L_NODE_DATA, &data, -1);

		if ((l_node_type == NODE_BASENAME) && (g_ascii_strcasecmp(data, node_data) >= 0)) {
			g_free(data);
			break;
		}
		g_free(data);

		valid = gtk_tree_model_iter_next(model, &l_iter);
	}

	/* Insert the new file after the last file by order */
	gtk_tree_store_insert_before (GTK_TREE_STORE(model), iter, p_iter, valid ? &l_iter : NULL);
	gtk_tree_store_set (GTK_TREE_STORE(model), iter,
	                    L_PIXBUF, clibrary->pixbuf_track,
	                    L_NODE_DATA, node_data,
	                    L_NODE_BOLD, PANGO_WEIGHT_NORMAL,
	                    L_NODE_TYPE, NODE_BASENAME,
	                    L_DATABASE_ID, location_id,
	                    L_MACH, FALSE,
	                    L_VISIBILE, TRUE,
	                    -1);
}

/* Adds a file and its parent directories to the library tree */

static void
add_folder_file(GtkTreeModel *model,
                const gchar *filepath,
                int location_id,
                GtkTreeIter *p_iter,
                RenaLibraryPane *clibrary)
{
	gchar **subpaths = NULL;		/* To be freed */

	GtkTreeIter iter, iter2, search_iter;
	int i = 0 , len = 0;

	/* Point after library directory prefix */

	subpaths = g_strsplit(filepath, G_DIR_SEPARATOR_S, -1);

	len = g_strv_length (subpaths);
	len--;

	/* Add all subdirectories and filename to the tree */
	for (i = 0; subpaths[i]; i++) {
		if (!find_child_node(subpaths[i], &search_iter, p_iter, model)) {
			if(i < len)
				add_child_node_folder(model, &iter, p_iter, subpaths[i], clibrary);
			else
				add_child_node_file(model, &iter, p_iter, subpaths[i], location_id, clibrary);
			p_iter = &iter;
		}
		else {
			iter2 = search_iter;
			p_iter = &iter2;
		}
	}

	g_strfreev(subpaths);
}

/* Adds an entry to the library tree by tag (genre, artist...) */

static void
add_child_node_by_tags (GtkTreeModel *model,
                       GtkTreeIter *p_iter,
                       gint location_id,
                       const gchar *location,
                       const gchar *genre,
                       const gchar *album,
                       const gchar *year,
                       const gchar *artist,
                       const gchar *track,
                       RenaLibraryPane *clibrary)
{
	GtkTreeIter iter, iter2, search_iter;
	gchar *node_data = NULL;
	GdkPixbuf *node_pixbuf = NULL;
	LibraryNodeType node_type = 0;
	gint node_level = 0, tot_levels = 0;
	gboolean need_gfree = FALSE;

	/* Iterate through library tree node types */ 
	tot_levels = g_slist_length(clibrary->library_tree_nodes);
	while (node_level < tot_levels) {
		/* Set data to be added to the tree node depending on the type of node */
		node_type = GPOINTER_TO_INT(g_slist_nth_data(clibrary->library_tree_nodes, node_level));
		switch (node_type) {
			case NODE_TRACK:
				node_pixbuf = clibrary->pixbuf_track;
				if (string_is_not_empty(track)) {
					node_data = (gchar *)track;
				}
				else {
					node_data = get_display_filename(location, FALSE);
					need_gfree = TRUE;
				}
				break;
			case NODE_ARTIST:
				node_pixbuf = clibrary->pixbuf_artist;
				node_data = string_is_not_empty(artist) ? (gchar *)artist : _("Unknown Artist");
				break;
			case NODE_ALBUM:
				node_pixbuf = clibrary->pixbuf_album;
				if (rena_preferences_get_sort_by_year(clibrary->preferences)) {
					node_data = g_strconcat ((string_is_not_empty(year) && (atoi(year) > 0)) ? year : _("Unknown"),
					                          " - ",
					                          string_is_not_empty(album) ? album : _("Unknown Album"),
					                          NULL);
					need_gfree = TRUE;
				}
				else {
					node_data = string_is_not_empty(album) ? (gchar *)album : _("Unknown Album");
				}
				break;
			case NODE_GENRE:
				node_pixbuf = clibrary->pixbuf_genre;
				node_data = string_is_not_empty(genre) ? (gchar *)genre : _("Unknown Genre");
				break;
			case NODE_CATEGORY_PLAYLIST:
			case NODE_CATEGORY_RADIO:
			case NODE_CATEGORY_PROVIDER:
			case NODE_FOLDER:
			case NODE_PLAYLIST:
			case NODE_RADIO:
			case NODE_BASENAME:
			default:
				g_warning("add_by_tag: Bad node type.");
				break;
		}

		/* Find / add child node if it's not already added */
		if (node_type != NODE_TRACK) {
			if (!find_child_node(node_data, &search_iter, p_iter, model)) {
				library_store_prepend_node(model,
				                           &iter,
				                           p_iter,
				                           node_pixbuf,
				                           node_data,
				                           node_type,
				                           0);
				p_iter = &iter;
			}
			else {
				iter2 = search_iter;
				p_iter = &iter2;
			}
		}
		else {
			library_store_prepend_node(model,
			                           &iter,
			                           p_iter,
			                           node_pixbuf,
			                           node_data,
			                           NODE_TRACK,
			                           location_id);
		}

		/* Free node_data if needed */
		if (need_gfree) {
			need_gfree = FALSE;
			g_free(node_data);
		}
		node_level++;
	}
}

GString *
append_rena_uri_string_list(GtkTreeIter *r_iter,
                              GString *list,
                              GtkTreeModel *model)
{
	GtkTreeIter t_iter;
	LibraryNodeType node_type = 0;
	gint location_id;
	gchar *data, *uri = NULL;
	gboolean valid;

	gtk_tree_model_get(model, r_iter, L_NODE_TYPE, &node_type, -1);

	switch (node_type) {
		case NODE_CATEGORY_PLAYLIST:
		case NODE_CATEGORY_RADIO:
		case NODE_CATEGORY_PROVIDER:
		case NODE_FOLDER:
		case NODE_GENRE:
		case NODE_ARTIST:
		case NODE_ALBUM:
			valid = gtk_tree_model_iter_children(model, &t_iter, r_iter);
			while (valid) {
				list = append_rena_uri_string_list(&t_iter, list, model);

				valid = gtk_tree_model_iter_next(model, &t_iter);
			}
			rena_process_gtk_events ();
	 		break;
		case NODE_TRACK:
		case NODE_BASENAME:
			gtk_tree_model_get(model, r_iter, L_DATABASE_ID, &location_id, -1);
			uri = g_strdup_printf("Location:/%d", location_id);
			break;
		case NODE_PLAYLIST:
			gtk_tree_model_get(model, r_iter, L_NODE_DATA, &data, -1);
			uri = g_strdup_printf("Playlist:/%s", data);
			g_free(data);
			break;
		case NODE_RADIO:
			gtk_tree_model_get(model, r_iter, L_NODE_DATA, &data, -1);
			uri = g_strdup_printf("Radio:/%s", data);
			g_free(data);
			break;
		default:
			break;
	}

	if(uri) {
		g_string_append (list, uri);
		g_string_append (list, "\r\n");
		g_free(uri);
	}

	return list;
}

static GString *
append_uri_string_list(GtkTreeIter *r_iter,
                       GString *list,
                       GtkTreeModel *model,
                       RenaLibraryPane *clibrary)
{
	GtkTreeIter t_iter;
	LibraryNodeType node_type = 0;
	gint location_id;
	gchar *filename = NULL, *uri = NULL;
	gboolean valid;

	gtk_tree_model_get(model, r_iter, L_NODE_TYPE, &node_type, -1);

	switch (node_type) {
		case NODE_CATEGORY_PLAYLIST:
		case NODE_CATEGORY_RADIO:
		case NODE_CATEGORY_PROVIDER:
		case NODE_FOLDER:
		case NODE_GENRE:
		case NODE_ARTIST:
		case NODE_ALBUM:
			valid = gtk_tree_model_iter_children(model, &t_iter, r_iter);
			while (valid) {
				list = append_uri_string_list(&t_iter, list, model, clibrary);

				valid = gtk_tree_model_iter_next(model, &t_iter);
			}
			rena_process_gtk_events ();
			break;
		case NODE_TRACK:
		case NODE_BASENAME:
			gtk_tree_model_get(model, r_iter, L_DATABASE_ID, &location_id, -1);
			filename = rena_database_get_filename_from_location_id(clibrary->cdbase, location_id);
			break;
		case NODE_PLAYLIST:
		case NODE_RADIO:
			g_message("Drag Radios and Playlist not yet implemented");
			break;
		default:
			break;
	}

	if(filename) {
		uri = g_filename_to_uri(filename, NULL, NULL);
		if(uri) {
			g_string_append (list, uri);
			g_string_append (list, "\r\n");

			g_free(uri);
		}
		g_free(filename);
	}

	return list;
}

/* Append to the given array the location ids of
   all the nodes under the given path */

static void get_location_ids(GtkTreePath *path,
			     GArray *loc_arr,
			     GtkTreeModel *model,
			     RenaLibraryPane *clibrary)
{
	GtkTreeIter t_iter, r_iter;
	LibraryNodeType node_type = 0;
	gint location_id;
	gint j = 0;

	clibrary->view_change = TRUE;

	gtk_tree_model_get_iter(model, &r_iter, path);

	/* If this path is a track, just append it to the array */

	gtk_tree_model_get(model, &r_iter, L_NODE_TYPE, &node_type, -1);
	if ((node_type == NODE_TRACK) || (node_type == NODE_BASENAME)) {
		gtk_tree_model_get(model, &r_iter, L_DATABASE_ID, &location_id, -1);
		g_array_append_val(loc_arr, location_id);
	}

	/* For all other node types do a recursive add */

	while (gtk_tree_model_iter_nth_child(model, &t_iter, &r_iter, j++)) {
		gtk_tree_model_get(model, &t_iter, L_NODE_TYPE, &node_type, -1);
		if ((node_type == NODE_TRACK) || (node_type == NODE_BASENAME)) {
			gtk_tree_model_get(model, &t_iter,
					   L_DATABASE_ID, &location_id, -1);
			g_array_append_val(loc_arr, location_id);
		}
		else {
			path = gtk_tree_model_get_path(model, &t_iter);
			get_location_ids(path, loc_arr, model, clibrary);
			gtk_tree_path_free(path);
		}
	}

	clibrary->view_change = FALSE;
}

/* Add all the tracks under the given path to the current playlist */

GList *
append_library_row_to_mobj_list(RenaDatabase *cdbase,
                                GtkTreePath *path,
                                GtkTreeModel *row_model,
                                GList *list)
{
	GtkTreeIter t_iter, r_iter;
	LibraryNodeType node_type = 0;
	gint location_id;
	RenaMusicobject *mobj = NULL;
	gchar *data = NULL;
	gint j = 0;

	/* If this path is a track, just append it to the current playlist */

	gtk_tree_model_get_iter(row_model, &r_iter, path);

	gtk_tree_model_get(row_model, &r_iter, L_NODE_TYPE, &node_type, -1);
	gtk_tree_model_get(row_model, &r_iter, L_DATABASE_ID, &location_id, -1);
	gtk_tree_model_get(row_model, &r_iter, L_NODE_DATA, &data, -1);

	switch (node_type) {
		case NODE_CATEGORY_PLAYLIST:
		case NODE_CATEGORY_RADIO:
		case NODE_CATEGORY_PROVIDER:
		case NODE_FOLDER:
		case NODE_GENRE:
		case NODE_ARTIST:
		case NODE_ALBUM:
			/* For all other node types do a recursive add */
			while (gtk_tree_model_iter_nth_child(row_model, &t_iter, &r_iter, j++)) {
				path = gtk_tree_model_get_path(row_model, &t_iter);
				list = append_library_row_to_mobj_list(cdbase, path, row_model, list);
				gtk_tree_path_free(path);
			}
			break;
		case NODE_TRACK:
		case NODE_BASENAME:
			mobj = new_musicobject_from_db(cdbase, location_id);
			if (G_LIKELY(mobj))
				list = g_list_append(list, mobj);
			break;
		case NODE_PLAYLIST:
			list = add_playlist_to_mobj_list(cdbase, data, list);
			break;
		case NODE_RADIO:
			list = add_radio_to_mobj_list(cdbase, data, list);
			break;
		default:
			break;
	}

	g_free(data);

	return list;
}

static void
delete_row_from_db(RenaDatabase *cdbase,
                   GtkTreePath *path,
                   GtkTreeModel *model)
{
	GtkTreeIter t_iter, r_iter;
	LibraryNodeType node_type = 0;
	gboolean valid;
	gint location_id;

	/* If this path is a track, delete it immediately */

	gtk_tree_model_get_iter(model, &r_iter, path);
	gtk_tree_model_get(model, &r_iter, L_NODE_TYPE, &node_type, -1);
	if ((node_type == NODE_TRACK) || (node_type == NODE_BASENAME)) {
		gtk_tree_model_get(model, &r_iter, L_DATABASE_ID, &location_id, -1);
		rena_database_forget_location(cdbase, location_id);
	}

	/* For all other node types do a recursive deletion */

	valid = gtk_tree_model_iter_children(model, &t_iter, &r_iter);
	while (valid) {
		gtk_tree_model_get(model, &t_iter, L_NODE_TYPE, &node_type, -1);
		if ((node_type == NODE_TRACK) || (node_type == NODE_BASENAME)) {
			gtk_tree_model_get(model, &t_iter,
					   L_DATABASE_ID, &location_id, -1);
			rena_database_forget_location(cdbase, location_id);
		}
		else {
			path = gtk_tree_model_get_path(model, &t_iter);
			delete_row_from_db(cdbase, path, model);
			gtk_tree_path_free(path);
		}

		valid = gtk_tree_model_iter_next(model, &t_iter);
	}
}

static void
trash_or_unlink_row (GArray *loc_arr, gboolean unlink, RenaLibraryPane *library)
{
	GtkWidget *question_dialog;
	gchar *primary, *secondary, *filename = NULL;
	gint response, location_id = 0;
	guint i;
	gboolean deleted = FALSE;
	GError *error = NULL;
	GFile *file = NULL;

	if (!loc_arr)
		return;

	for(i = 0; i < loc_arr->len; i++) {
		location_id = g_array_index(loc_arr, gint, i);
		if (location_id) {
			filename = rena_database_get_filename_from_location_id (library->cdbase, location_id);
			if (filename && g_file_test(filename, G_FILE_TEST_EXISTS)) {
				file = g_file_new_for_path(filename);

				if(!unlink && !(deleted = g_file_trash(file, NULL, &error))) {
					primary = g_strdup (_("File can't be moved to trash. Delete permanently?"));
					secondary = g_strdup_printf (_("The file \"%s\" cannot be moved to the trash. Details: %s"),
									g_file_get_basename (file), error->message);

					question_dialog = gtk_message_dialog_new (GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(library))),
				                                                GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				                                                GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, "%s", primary);
					gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (question_dialog), "%s", secondary);

					gtk_dialog_add_button (GTK_DIALOG (question_dialog), _("_Cancel"), GTK_RESPONSE_CANCEL);
					if (loc_arr->len > 1) {
					        gtk_dialog_add_button (GTK_DIALOG (question_dialog), RENA_BUTTON_SKIP, RENA_RESPONSE_SKIP);
					        gtk_dialog_add_button (GTK_DIALOG (question_dialog), RENA_BUTTON_SKIP_ALL, RENA_RESPONSE_SKIP_ALL);
			        		gtk_dialog_add_button (GTK_DIALOG (question_dialog), RENA_BUTTON_DELETE_ALL, RENA_RESPONSE_DELETE_ALL);
					}
					gtk_dialog_add_button (GTK_DIALOG (question_dialog), _("_Delete"), GTK_RESPONSE_ACCEPT);

					response = gtk_dialog_run (GTK_DIALOG (question_dialog));
					gtk_widget_destroy (question_dialog);
					g_free (primary);
					g_free (secondary);
					g_error_free (error);
					error = NULL;

					switch (response)
					{
						case RENA_RESPONSE_DELETE_ALL:
							unlink = TRUE;
							break;
						case GTK_RESPONSE_ACCEPT:
							g_unlink(filename);
							deleted = TRUE;
							break;
						case RENA_RESPONSE_SKIP:
							break;
						case RENA_RESPONSE_SKIP_ALL:
						case GTK_RESPONSE_CANCEL:
						case GTK_RESPONSE_DELETE_EVENT:
						default:
							return;
					}
				}
				if(unlink) {
					g_unlink(filename);
					deleted = TRUE;
				}
				g_object_unref(G_OBJECT(file));
			}
			if (deleted) {
				rena_database_forget_location (library->cdbase, location_id);
			}
		}
	}
}

/******************/
/* Event handlers */
/******************/

static void
library_tree_row_activated_cb (GtkTreeView *library_tree,
                               GtkTreePath *path,
                               GtkTreeViewColumn *column,
                               RenaLibraryPane *library)
{
	GtkTreeIter iter;
	GtkTreeModel *filter_model;
	LibraryNodeType node_type;

	filter_model = gtk_tree_view_get_model (GTK_TREE_VIEW(library->library_tree));
	gtk_tree_model_get_iter (filter_model, &iter, path);
	gtk_tree_model_get (filter_model, &iter, L_NODE_TYPE, &node_type, -1);

	switch(node_type) {
	case NODE_CATEGORY_PLAYLIST:
	case NODE_CATEGORY_RADIO:
	case NODE_CATEGORY_PROVIDER:
	case NODE_ARTIST:
	case NODE_ALBUM:
	case NODE_GENRE:
	case NODE_FOLDER:
		if (!gtk_tree_view_row_expanded (GTK_TREE_VIEW(library->library_tree), path))
			gtk_tree_view_expand_row (GTK_TREE_VIEW(library->library_tree), path, TRUE);
		else
			gtk_tree_view_collapse_row (GTK_TREE_VIEW(library->library_tree), path);
		break;
	case NODE_TRACK:
	case NODE_BASENAME:
	case NODE_PLAYLIST:
	case NODE_RADIO:
		g_signal_emit (library, signals[LIBRARY_APPEND_PLAYLIST], 0);
		break;
	default:
		break;
	}
}

static int
rena_library_pane_tree_key_press (GtkWidget *win, GdkEventKey *event, RenaLibraryPane *library)
{
	if (event->state != 0
			&& ((event->state & GDK_CONTROL_MASK)
			|| (event->state & GDK_MOD1_MASK)
			|| (event->state & GDK_MOD3_MASK)
			|| (event->state & GDK_MOD4_MASK)
			|| (event->state & GDK_MOD5_MASK)))
		return FALSE;
	if (event->keyval == GDK_KEY_Delete){
		rena_library_pane_delete_from_db (library);
		return TRUE;
	}
	return FALSE;
}

/*****************/
/* DnD functions */
/*****************/
/* These two functions are only callbacks that must be passed to
gtk_tree_selection_set_select_function() to chose if GTK is allowed
to change selection itself or if we handle it ourselves */

static gboolean
rena_library_pane_selection_func_true(GtkTreeSelection *selection,
                                        GtkTreeModel *model,
                                        GtkTreePath *path,
                                        gboolean path_currently_selected,
                                        gpointer data)
{
	return TRUE;
}

static gboolean
rena_library_pane_selection_func_false(GtkTreeSelection *selection,
                                         GtkTreeModel *model,
                                         GtkTreePath *path,
                                         gboolean path_currently_selected,
                                         gpointer data)
{
	return FALSE;
}


static gboolean
rena_library_pane_tree_button_press_cb (GtkWidget         *widget,
                                          GdkEventButton    *event,
                                          RenaLibraryPane *library)
{
	GtkWidget *popup_menu;
	GMenu *menu;
	GAction *action;
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	gboolean many_selected = FALSE, sensitive = TRUE;
	gchar *node_data = NULL;
	LibraryNodeType node_type;
	gint n_select = 0;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW(library->library_tree));
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(library->library_tree));

	if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW(widget),
	                                   (gint) event->x,
	                                   (gint) event->y,
	                                   &path,
	                                   NULL, NULL, NULL))
	{
		switch(event->button) {
		case 1:
			if (gtk_tree_selection_path_is_selected(selection, path) &&
			    !(event->state & GDK_CONTROL_MASK) &&
			    !(event->state & GDK_SHIFT_MASK))
			{
				gtk_tree_selection_set_select_function(selection, &rena_library_pane_selection_func_false, NULL, NULL);
			}
			else
			{
				gtk_tree_selection_set_select_function(selection, &rena_library_pane_selection_func_true, NULL, NULL);
			}
			break;
		case 2:
			if (!gtk_tree_selection_path_is_selected(selection, path))
			{
				gtk_tree_selection_unselect_all(selection);
				gtk_tree_selection_select_path(selection, path);
			}
			g_signal_emit (library, signals[LIBRARY_APPEND_PLAYLIST], 0);
			break;
		case 3:
			if (!gtk_tree_selection_path_is_selected(selection, path))
			{
				gtk_tree_selection_unselect_all(selection);
				gtk_tree_selection_select_path(selection, path);
			}

			gtk_tree_model_get_iter(model, &iter, path);
			gtk_tree_model_get(model, &iter, L_NODE_TYPE, &node_type, -1);

			n_select = gtk_tree_selection_count_selected_rows(selection);
			if (n_select > 1)
			{
				menu = G_MENU (gtk_builder_get_object (library->builder, "library-categories-menu"));
				popup_menu = gtk_menu_new_from_model (G_MENU_MODEL(menu));
			}
			else if (node_type == NODE_CATEGORY_PLAYLIST)
			{
				menu = G_MENU (gtk_builder_get_object (library->builder, "library-categories-menu"));
				popup_menu = gtk_menu_new_from_model (G_MENU_MODEL(menu));
			}
			else if (node_type == NODE_CATEGORY_RADIO)
			{
				menu = G_MENU (gtk_builder_get_object (library->builder, "library-categories-menu"));
				popup_menu = gtk_menu_new_from_model (G_MENU_MODEL(menu));
			}
			else if (node_type == NODE_CATEGORY_PROVIDER)
			{
				menu = G_MENU (gtk_builder_get_object (library->builder, "library-provider-menu"));
				popup_menu = gtk_menu_new_from_model (G_MENU_MODEL(menu));
			}
			else if (node_type == NODE_PLAYLIST)
			{
				menu = G_MENU (gtk_builder_get_object (library->builder, "library-playlist-menu"));
				popup_menu = gtk_menu_new_from_model (G_MENU_MODEL(menu));

				gtk_tree_model_get (model, &iter, L_NODE_DATA, &node_data, -1);
				if (g_ascii_strcasecmp(_("Favorites"), node_data) == 0)
					sensitive = FALSE;
				g_free (node_data);
			}
			else if (node_type == NODE_RADIO)
			{
				menu = G_MENU (gtk_builder_get_object (library->builder, "library-radio-menu"));
				popup_menu = gtk_menu_new_from_model (G_MENU_MODEL(menu));
			}
			else
			{
				menu = G_MENU (gtk_builder_get_object (library->builder, "library-music-menu"));
				popup_menu = gtk_menu_new_from_model (G_MENU_MODEL(menu));
			}

			action = g_action_map_lookup_action (G_ACTION_MAP (library->actions), "rename");
			g_object_set (action, "enabled", sensitive, NULL);

			action = g_action_map_lookup_action (G_ACTION_MAP (library->actions), "delete");
			g_object_set (action, "enabled", sensitive, NULL);

			gtk_widget_insert_action_group (popup_menu, "lib", G_ACTION_GROUP(library->actions));
			gtk_menu_popup (GTK_MENU(popup_menu),
			                NULL, NULL, NULL, NULL,
			                event->button, event->time);

			/* If more than one track is selected, don't propagate event */

			if (n_select > 1)
				many_selected = TRUE;
			else
				many_selected = FALSE;
			break;
		default:
			many_selected = FALSE;
			break;
		}
		gtk_tree_path_free(path);
	}
	else gtk_tree_selection_unselect_all(selection);

	return many_selected;
}

static gboolean
library_tree_button_release_cb (GtkWidget *widget,
                                GdkEventButton *event,
                                RenaLibraryPane *library)
{
	GtkTreeSelection *selection;
	GtkTreePath *path;
	
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(library->library_tree));

	if((event->state & GDK_CONTROL_MASK) || (event->state & GDK_SHIFT_MASK) || (library->dragging == TRUE) || (event->button!=1)){
		gtk_tree_selection_set_select_function(selection, &rena_library_pane_selection_func_true, NULL, NULL);
		library->dragging = FALSE;
		return FALSE;
	}

	gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), (gint) event->x,(gint) event->y, &path, NULL, NULL, NULL);

	if (path){
		gtk_tree_selection_set_select_function(selection, &rena_library_pane_selection_func_true, NULL, NULL);
		gtk_tree_selection_unselect_all(selection);
		gtk_tree_selection_select_path(selection, path);
		gtk_tree_path_free(path);
	}
	return FALSE;
}

/*******/
/* DnD */
/*******/

static gboolean
dnd_library_tree_begin(GtkWidget *widget,
                       GdkDragContext *context,
                       RenaLibraryPane *clibrary)
{
	clibrary->dragging = TRUE;
	return FALSE;
}

gboolean
gtk_selection_data_set_rena_uris (GtkSelectionData  *selection_data,
                                    GString *list)
{
	gchar *result;
	gsize length;

	result = g_convert (list->str, list->len,
	                    "ASCII", "UTF-8",
	                    NULL, &length, NULL);

	if (result) {
		gtk_selection_data_set (selection_data,
		                        gtk_selection_data_get_target(selection_data),
		                        8, (guchar *) result, length);
		g_free (result);

		return TRUE;
	}

	return FALSE;
}

/* Callback for DnD signal 'drag-data-get' */

static void
dnd_library_tree_get(GtkWidget *widget,
                     GdkDragContext *context,
                     GtkSelectionData *data,
                     RenaDndTarget info,
                     guint time,
                     RenaLibraryPane *clibrary)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *list = NULL, *l;
	GString *rlist;
	GtkTreeIter s_iter;

	switch(info) {
	case TARGET_REF_LIBRARY:
		rlist = g_string_new (NULL);

		set_watch_cursor (GTK_WIDGET(clibrary));
		clibrary->view_change = TRUE;

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(
							clibrary->library_tree));
		list = gtk_tree_selection_get_selected_rows(selection, &model);

		l = list;
		while(l) {
			if(gtk_tree_model_get_iter(model, &s_iter, l->data))
				rlist = append_rena_uri_string_list(&s_iter, rlist, model);
			gtk_tree_path_free(l->data);
			l = l->next;
		}

		clibrary->view_change = FALSE;
		remove_watch_cursor (GTK_WIDGET(clibrary));

		gtk_selection_data_set_rena_uris(data, rlist);

		g_list_free(list);
		g_string_free (rlist, TRUE);
 		break;
 	case TARGET_URI_LIST:
		rlist = g_string_new (NULL);

		set_watch_cursor (GTK_WIDGET(clibrary));
		clibrary->view_change = TRUE;

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(
							clibrary->library_tree));
		list = gtk_tree_selection_get_selected_rows(selection, &model);

		l = list;
		while(l) {
			if(gtk_tree_model_get_iter(model, &s_iter, l->data))
				rlist = append_uri_string_list(&s_iter, rlist, model, clibrary);
			l = l->next;
		}

		clibrary->view_change = FALSE;
		remove_watch_cursor (GTK_WIDGET(clibrary));

		gtk_selection_data_set_rena_uris(data, rlist);

		g_list_free(list);
		g_string_free (rlist, TRUE);
		break;
	case TARGET_PLAIN_TEXT:
	default:
		g_warning("Unknown DND type");
		break;
	}
}

static const GtkTargetEntry lentries[] = {
	{"REF_LIBRARY", GTK_TARGET_SAME_APP, TARGET_REF_LIBRARY},
	{"text/uri-list", GTK_TARGET_OTHER_APP, TARGET_URI_LIST},
	{"text/plain", GTK_TARGET_OTHER_APP, TARGET_PLAIN_TEXT}
};

static void
library_pane_init_dnd(RenaLibraryPane *clibrary)
{
	/* Source: Library View */

	gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(clibrary->library_tree),
					       GDK_BUTTON1_MASK,
					       lentries,
					       G_N_ELEMENTS(lentries),
					       GDK_ACTION_COPY);

	g_signal_connect(G_OBJECT(GTK_WIDGET(clibrary->library_tree)),
	                 "drag-begin",
	                 G_CALLBACK(dnd_library_tree_begin),
	                 clibrary);
	g_signal_connect(G_OBJECT(clibrary->library_tree),
	                 "drag-data-get",
	                 G_CALLBACK(dnd_library_tree_get),
	                 clibrary);
}


/**********/
/* Search */
/**********/

static gboolean
rena_library_pane_set_all_visible_func (GtkTreeModel *model,
                                          GtkTreePath  *path,
                                          GtkTreeIter  *iter,
                                          gpointer      data)
{
	RenaLibraryPane *library = data;
	if (library->filter_entry != NULL)
		return TRUE;

	/* Have to give control to GTK periodically ... */
	if (gtk_tree_path_get_depth (path) == 2)
		rena_process_gtk_events ();

	gtk_tree_store_set (GTK_TREE_STORE(model), iter,
	                    L_MACH, FALSE, L_VISIBILE, TRUE, -1);
	return FALSE;
}

static void
rena_library_pane_expand_filtered_tree_func(GtkTreeView *view,
                                              GtkTreePath *path,
                                              gpointer     data)
{
	GtkTreeIter iter;
	LibraryNodeType node_type;
	gboolean node_mach;

	GtkTreeModel *filter_model = data;

	/* Collapse any non-leaf node that matches the seach entry */

	gtk_tree_model_get_iter(filter_model, &iter, path);
	gtk_tree_model_get(filter_model, &iter, L_MACH, &node_mach, -1);

	if (node_mach) {
		gtk_tree_model_get(filter_model, &iter, L_NODE_TYPE, &node_type, -1);
		if ((node_type != NODE_TRACK) && (node_type != NODE_BASENAME))
			gtk_tree_view_collapse_row(view, path);
	}
}

static void
rena_library_pane_set_visible_parents_nodes (GtkTreeModel *model, GtkTreeIter *c_iter)
{
	GtkTreeIter t_iter, parent;

	t_iter = *c_iter;

	while(gtk_tree_model_iter_parent(model, &parent, &t_iter)) {
		gtk_tree_store_set (GTK_TREE_STORE(model), &parent,
		                   L_VISIBILE, TRUE, -1);
		t_iter = parent;
	}
}

static gboolean
rena_libary_pane_any_parent_node_mach (GtkTreeModel *model, GtkTreeIter *iter)
{
	GtkTreeIter t_iter, parent;
	gboolean p_mach = FALSE;

	t_iter = *iter;
	while(gtk_tree_model_iter_parent(model, &parent, &t_iter)) {
		gtk_tree_model_get(model, &parent, L_MACH, &p_mach, -1);
		if (p_mach)
			return TRUE;

		t_iter = parent;
	}

	return FALSE;
}

static gboolean
rena_libary_pane_filter_tree_func (GtkTreeModel *model,
                                     GtkTreePath  *path,
                                     GtkTreeIter  *iter,
                                     gpointer      data)
{
	gchar *node_data = NULL, *u_str;
	gboolean p_mach;

	RenaLibraryPane *library = data;

	if (library->filter_entry == NULL)
		return TRUE;

	/* Have to give control to GTK periodically ... */
	if (gtk_tree_path_get_depth (path) == 2)
		rena_process_gtk_events ();

	/* Mark node and its parents visible if search entry matches.
	   If search entry doesn't match, check if _any_ ancestor has
	   been marked as visible and if so, mark current node as visible too. */

	gtk_tree_model_get(model, iter, L_NODE_DATA, &node_data, -1);
	u_str = g_utf8_strdown(node_data, -1);
	if (rena_strstr_lv(u_str, library->filter_entry, library->preferences))
	{
		/* Set visible the match row */
		gtk_tree_store_set (GTK_TREE_STORE(model), iter,
		                    L_MACH, TRUE, L_VISIBILE, TRUE, -1);

		/* Also set visible the parents */
		rena_library_pane_set_visible_parents_nodes (model, iter);
	}
	else
	{
		/* Check parents. If any node is visible due it mach,
		 * also shows. So, show the children of coincidences. */
		p_mach = rena_libary_pane_any_parent_node_mach (model, iter);
		gtk_tree_store_set (GTK_TREE_STORE(model), iter,
		                    L_MACH, FALSE, L_VISIBILE, p_mach, -1);
	}
	g_free(u_str);
	g_free(node_data);

	return FALSE;
}

static void
rena_library_pane_do_filter (RenaLibraryPane *library)
{
	GtkTreeModel *filter_model;

	/* Have to give control to GTK periodically ... */
	rena_process_gtk_events ();

	/* Remove the model of widget. */
	filter_model = gtk_tree_view_get_model(GTK_TREE_VIEW(library->library_tree));
	g_object_ref(filter_model);
	gtk_tree_view_set_model(GTK_TREE_VIEW(library->library_tree), NULL);

	/* Have to give control to GTK periodically ... */
	rena_process_gtk_events ();

	/* Set visibility of rows in the library store. */
	gtk_tree_model_foreach (GTK_TREE_MODEL(library->library_store),
	                        rena_libary_pane_filter_tree_func, library);

	/* Have to give control to GTK periodically ... */
	rena_process_gtk_events ();

	/* Set the model again.*/
	gtk_tree_view_set_model(GTK_TREE_VIEW(library->library_tree), filter_model);
	g_object_unref(filter_model);

	/* Have to give control to GTK periodically ... */
	rena_process_gtk_events ();

	/* Expand all and then reduce properly. */
	gtk_tree_view_expand_all(GTK_TREE_VIEW(library->library_tree));
	gtk_tree_view_map_expanded_rows(GTK_TREE_VIEW(library->library_tree),
	                                rena_library_pane_expand_filtered_tree_func, filter_model);

	/* Have to give control to GTK periodically ... */
	rena_process_gtk_events ();

}

static void
rena_library_pane_show_all (RenaLibraryPane *library)
{
	GtkTreeModel *filter_model;

	/* Have to give control to GTK periodically ... */
	rena_process_gtk_events ();

	/* Remove the model of widget. */
	filter_model = gtk_tree_view_get_model(GTK_TREE_VIEW(library->library_tree));
	g_object_ref(filter_model);
	gtk_tree_view_set_model(GTK_TREE_VIEW(library->library_tree), NULL);

	/* Have to give control to GTK periodically ... */
	rena_process_gtk_events ();

	/* Set all nodes visibles. */
	gtk_tree_model_foreach (GTK_TREE_MODEL(library->library_store),
	                        rena_library_pane_set_all_visible_func, library);

	/* Have to give control to GTK periodically ... */
	rena_process_gtk_events ();

	/* Set the model again. */
	gtk_tree_view_set_model(GTK_TREE_VIEW(library->library_tree), filter_model);
	g_object_unref(filter_model);

	/* Have to give control to GTK periodically ... */
	rena_process_gtk_events ();

	/* Expand the categories. */

	rena_library_expand_categories(library);
}

static gboolean
rena_search_entry_pulse_it (RenaLibraryPane *library)
{
	gtk_entry_progress_pulse (GTK_ENTRY(library->search_entry));
	return G_SOURCE_CONTINUE;
}

static gboolean
rena_library_pane_do_refilter (RenaLibraryPane *clibrary)
{
	gchar *needle = NULL;
	gboolean ret = FALSE;

	gtk_entry_set_progress_pulse_step (GTK_ENTRY(clibrary->search_entry), 0.1);
	clibrary->pulse_id = g_timeout_add (250, (GSourceFunc)rena_search_entry_pulse_it, clibrary);

	if (clibrary->filter_active == TRUE)
		return TRUE;

	clibrary->filter_active = TRUE;
	needle = g_strdup(clibrary->filter_entry);

	rena_process_gtk_events ();

	if (clibrary->filter_entry != NULL)
		rena_library_pane_do_filter (clibrary);
	else
		rena_library_pane_show_all (clibrary);

	/* Have to give control to GTK periodically ... */
	rena_process_gtk_events ();

	clibrary->filter_id = 0;
	clibrary->filter_active = FALSE;

	/* If changed the needle search again. */
	if (needle && g_ascii_strcasecmp(needle, clibrary->filter_entry))
		ret = TRUE;

	g_free(needle);

	gtk_entry_set_progress_pulse_step (GTK_ENTRY(clibrary->search_entry), 0.0);
	gtk_entry_set_progress_fraction (GTK_ENTRY(clibrary->search_entry), 0.0);
	g_source_remove (clibrary->pulse_id);
	clibrary->pulse_id = 0;

	return ret;
}

void
rena_library_panel_queue_refilter (RenaLibraryPane *clibrary)
{
	if (clibrary->filter_id != 0 && clibrary->filter_active == FALSE) {
		g_source_remove(clibrary->filter_id);
		clibrary->filter_id = 0;
	}

	if (clibrary->filter_id == 0)
		clibrary->filter_id = g_timeout_add(500, (GSourceFunc)rena_library_pane_do_refilter, clibrary);
}

static void
simple_library_search_keyrelease_handler (GtkEntry          *entry,
                                          RenaLibraryPane *clibrary)
{
	const gchar *filter_entry = NULL;

	if (!rena_preferences_get_instant_search(clibrary->preferences))
		return;

	if (clibrary->filter_entry != NULL) {
		g_free (clibrary->filter_entry);
		clibrary->filter_entry = NULL;
	}

	filter_entry = gtk_entry_get_text (entry);
	if (string_is_not_empty(filter_entry))
		clibrary->filter_entry = g_utf8_strdown (filter_entry, -1);

	rena_library_panel_queue_refilter(clibrary);
}

gboolean
simple_library_search_activate_handler (GtkEntry          *entry,
                                        RenaLibraryPane *clibrary)
{
	const gchar *filter_entry = NULL;

	if (!rena_preferences_get_instant_search(clibrary->preferences))
		return FALSE;

	if (clibrary->filter_entry != NULL) {
		g_free (clibrary->filter_entry);
		clibrary->filter_entry = NULL;
	}
	filter_entry = gtk_entry_get_text (entry);
	if (string_is_not_empty(filter_entry))
		clibrary->filter_entry = g_utf8_strdown (filter_entry, -1);

	rena_library_panel_queue_refilter (clibrary);

	return FALSE;
}

static void
rena_library_expand_categories(RenaLibraryPane *clibrary)
{
	GtkTreeModel *filter_model, *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	gboolean valid, visible;

	filter_model = gtk_tree_view_get_model(GTK_TREE_VIEW(clibrary->library_tree));
	model = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(filter_model));

	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid) {
		visible = gtk_tree_model_iter_has_child(model, &iter);
		gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
		                   L_VISIBILE, visible, -1);

		path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_view_expand_row (GTK_TREE_VIEW(clibrary->library_tree), path, FALSE);
		gtk_tree_path_free(path);

		valid = gtk_tree_model_iter_next(model, &iter);
	}
}

/*
 * Return if you must update the library according to the changes, and the current view.
 */

gboolean
rena_library_need_update_view(RenaPreferences *preferences, gint changed)
{
	gboolean need_update = FALSE;

	switch (rena_preferences_get_library_style(preferences)) {
		case FOLDERS:
			break;
		case ARTIST:
			need_update = ((changed & TAG_ARTIST_CHANGED) ||
			               (changed & TAG_TITLE_CHANGED));

			break;
		case ALBUM:
			need_update = ((changed & TAG_ALBUM_CHANGED) ||
			               (rena_preferences_get_sort_by_year(preferences) && (changed & TAG_YEAR_CHANGED)) ||
			               (changed & TAG_TITLE_CHANGED));
			break;
		case GENRE:
			need_update = ((changed & TAG_GENRE_CHANGED) ||
			               (changed & TAG_TITLE_CHANGED));
			break;
		case ARTIST_ALBUM:
			need_update = ((changed & TAG_ARTIST_CHANGED) ||
			               (changed & TAG_ALBUM_CHANGED) ||
			               (rena_preferences_get_sort_by_year(preferences) && (changed & TAG_YEAR_CHANGED)) ||
			               (changed & TAG_TITLE_CHANGED));
			break;
		case GENRE_ARTIST:
			need_update = ((changed & TAG_GENRE_CHANGED) ||
			               (changed & TAG_ARTIST_CHANGED) ||
			               (changed & TAG_TITLE_CHANGED));
			break;
		case GENRE_ALBUM:
			need_update = ((changed & TAG_GENRE_CHANGED) ||
			               (changed & TAG_ALBUM_CHANGED) ||
			               (rena_preferences_get_sort_by_year(preferences) && (changed & TAG_YEAR_CHANGED)) ||
			               (changed & TAG_TITLE_CHANGED));
			break;
		case GENRE_ARTIST_ALBUM:
			need_update = ((changed & TAG_GENRE_CHANGED) ||
			               (changed & TAG_ARTIST_CHANGED) ||
			               (changed & TAG_ALBUM_CHANGED) ||
			               (rena_preferences_get_sort_by_year(preferences) && (changed & TAG_YEAR_CHANGED)) ||
			               (changed & TAG_TITLE_CHANGED));
			break;
		default:
			break;
	}

	return need_update;
}

gboolean
rena_library_need_update(RenaLibraryPane *clibrary, gint changed)
{
	return rena_library_need_update_view(clibrary->preferences, changed);
}

/********************************/
/* Library view order selection */
/********************************/

static void
library_pane_update_style (RenaLibraryPane *library)
{
	g_slist_free (library->library_tree_nodes);
	library->library_tree_nodes = NULL;

	switch (rena_preferences_get_library_style(library->preferences)) {
		case FOLDERS:
			library->library_tree_nodes =
				g_slist_append(library->library_tree_nodes,
					       GINT_TO_POINTER(NODE_FOLDER));
			library->library_tree_nodes =
				g_slist_append(library->library_tree_nodes,
				              GINT_TO_POINTER(NODE_BASENAME));
			gtk_label_set_text (GTK_LABEL(library->pane_title), _("Folders structure"));
			break;
		case ARTIST:
			library->library_tree_nodes =
				g_slist_append(library->library_tree_nodes,
				               GINT_TO_POINTER(NODE_ARTIST));
			library->library_tree_nodes =
				g_slist_append(library->library_tree_nodes,
				               GINT_TO_POINTER(NODE_TRACK));
			gtk_label_set_text (GTK_LABEL(library->pane_title), _("Artist"));
			break;
		case ALBUM:
			library->library_tree_nodes =
				g_slist_append(library->library_tree_nodes,
				               GINT_TO_POINTER(NODE_ALBUM));
			library->library_tree_nodes =
				g_slist_append(library->library_tree_nodes,
				               GINT_TO_POINTER(NODE_TRACK));
			gtk_label_set_text (GTK_LABEL(library->pane_title), _("Album"));
			break;
		case GENRE:
			library->library_tree_nodes =
				g_slist_append(library->library_tree_nodes,
				               GINT_TO_POINTER(NODE_GENRE));
			library->library_tree_nodes =
				g_slist_append(library->library_tree_nodes,
				               GINT_TO_POINTER(NODE_TRACK));
			gtk_label_set_text (GTK_LABEL(library->pane_title), _("Genre"));
			break;
		case ARTIST_ALBUM:
			library->library_tree_nodes =
				g_slist_append(library->library_tree_nodes,
				               GINT_TO_POINTER(NODE_ARTIST));
			library->library_tree_nodes =
				g_slist_append(library->library_tree_nodes,
				               GINT_TO_POINTER(NODE_ALBUM));
			library->library_tree_nodes =
				g_slist_append(library->library_tree_nodes,
				               GINT_TO_POINTER(NODE_TRACK));
			gtk_label_set_text (GTK_LABEL(library->pane_title), _("Artist / Album"));
			break;
		case GENRE_ARTIST:
			library->library_tree_nodes =
				g_slist_append(library->library_tree_nodes,
				               GINT_TO_POINTER(NODE_GENRE));
			library->library_tree_nodes =
				g_slist_append(library->library_tree_nodes,
				               GINT_TO_POINTER(NODE_ARTIST));
			library->library_tree_nodes =
				g_slist_append(library->library_tree_nodes,
				               GINT_TO_POINTER(NODE_TRACK));
			gtk_label_set_text (GTK_LABEL(library->pane_title), _("Genre / Artist"));
			break;
		case GENRE_ALBUM:
			library->library_tree_nodes =
				g_slist_append(library->library_tree_nodes,
				               GINT_TO_POINTER(NODE_GENRE));
			library->library_tree_nodes =
				g_slist_append(library->library_tree_nodes,
				               GINT_TO_POINTER(NODE_ALBUM));
			library->library_tree_nodes =
				g_slist_append(library->library_tree_nodes,
				               GINT_TO_POINTER(NODE_TRACK));
			gtk_label_set_text (GTK_LABEL(library->pane_title), _("Genre / Album"));
			break;
		case GENRE_ARTIST_ALBUM:
			library->library_tree_nodes =
				g_slist_append(library->library_tree_nodes,
				               GINT_TO_POINTER(NODE_GENRE));
			library->library_tree_nodes =
				g_slist_append(library->library_tree_nodes,
				               GINT_TO_POINTER(NODE_ARTIST));
			library->library_tree_nodes =
				g_slist_append(library->library_tree_nodes,
				               GINT_TO_POINTER(NODE_ALBUM));
			library->library_tree_nodes =
				g_slist_append(library->library_tree_nodes,
				               GINT_TO_POINTER(NODE_TRACK));
			gtk_label_set_text (GTK_LABEL(library->pane_title), _("Genre / Artist / Album"));
			break;
		default:
			break;
	}
}

static void
library_pane_change_style (GObject *gobject, GParamSpec *pspec, RenaLibraryPane *library)
{
	library_pane_update_style (library);
	library_pane_view_reload (library);
}

/*********************************/
/* Functions to reload playlist. */
/*********************************/

static void
library_view_append_playlists(GtkTreeModel *model,
                              GtkTreeIter *p_iter,
                              RenaLibraryPane *clibrary)
{
	RenaPreparedStatement *statement;
	const gchar *sql = NULL, *playlist = NULL;
	GtkTreeIter iter;

	sql = "SELECT name FROM PLAYLIST WHERE name != ? ORDER BY name COLLATE NOCASE DESC";
	statement = rena_database_create_statement (clibrary->cdbase, sql);
	rena_prepared_statement_bind_string (statement, 1, SAVE_PLAYLIST_STATE);

	while (rena_prepared_statement_step (statement)) {
		playlist = rena_prepared_statement_get_string(statement, 0);

		library_store_prepend_node(model,
		                           &iter,
		                           p_iter,
		                           clibrary->pixbuf_track,
		                           playlist,
		                           NODE_PLAYLIST,
		                           0);

		rena_process_gtk_events ();
	}
	rena_prepared_statement_free (statement);
}

static void
library_view_append_radios(GtkTreeModel *model,
                           GtkTreeIter *p_iter,
                           RenaLibraryPane *clibrary)
{
	RenaPreparedStatement *statement;
	const gchar *sql = NULL, *radio = NULL;
	GtkTreeIter iter;

	sql = "SELECT name FROM RADIO ORDER BY name COLLATE NOCASE DESC";
	statement = rena_database_create_statement (clibrary->cdbase, sql);
	while (rena_prepared_statement_step (statement)) {
		radio = rena_prepared_statement_get_string(statement, 0);

		library_store_prepend_node(model,
		                           &iter,
		                           p_iter,
		                           clibrary->pixbuf_track,
		                           radio,
		                           NODE_RADIO,
		                           0);

		rena_process_gtk_events ();
	}
	rena_prepared_statement_free (statement);
}

static void
rena_library_view_append_provider_by_folder (RenaLibraryPane *clibrary,
                                               GtkTreeModel      *model,
                                               GtkTreeIter       *p_iter,
                                               const gchar       *provider)

{
	RenaPreparedStatement *statement;
	const gchar *sql = NULL, *filepath = NULL, *filename = NULL;
	gint provider_id = 0;

	sql = "SELECT name, id FROM LOCATION WHERE id IN (SELECT location FROM TRACK WHERE PROVIDER = ?) ORDER BY name DESC";

	statement = rena_database_create_statement (clibrary->cdbase, sql);

	provider_id = rena_database_find_provider (clibrary->cdbase, provider);
	rena_prepared_statement_bind_int (statement, 1, provider_id);

	while (rena_prepared_statement_step (statement)) {
		filepath = rena_prepared_statement_get_string(statement, 0);

		/* FIXME: Handle uris like cdda:// */
		filename = g_strrstr (filepath, "://");
		if (filename) {
			filename += strlen("://");
		}
		else {
			filename = filepath + strlen(provider) + 1;
		}

		add_folder_file(model,
		                filename,
		                rena_prepared_statement_get_int(statement, 1),
		                p_iter,
		                clibrary);
		rena_process_gtk_events ();
	}

	rena_prepared_statement_free (statement);
}

static void
rena_library_view_append_provider_by_tags (RenaLibraryPane *clibrary,
                                             GtkTreeModel      *model,
                                             GtkTreeIter       *p_iter,
                                             const gchar       *provider)
{
	RenaPreparedStatement *statement;
	gchar *order_str = NULL, *sql = NULL;
	gint provider_id = 0;

	/* Get order needed to sqlite query. */
	switch(rena_preferences_get_library_style(clibrary->preferences)) {
		case FOLDERS:
			break;
		case ARTIST:
			order_str = g_strdup("ARTIST.name COLLATE NOCASE DESC, TRACK.title COLLATE NOCASE DESC");
			break;
		case ALBUM:
			if (rena_preferences_get_sort_by_year(clibrary->preferences))
				order_str = g_strdup("YEAR.year COLLATE NOCASE DESC, ALBUM.name COLLATE NOCASE DESC, TRACK.title COLLATE NOCASE DESC");
			else
				order_str = g_strdup("ALBUM.name COLLATE NOCASE DESC, TRACK.title COLLATE NOCASE DESC");
			break;
		case GENRE:
			order_str = g_strdup("GENRE.name COLLATE NOCASE DESC, TRACK.title COLLATE NOCASE DESC");
			break;
		case ARTIST_ALBUM:
			if (rena_preferences_get_sort_by_year(clibrary->preferences))
				order_str = g_strdup("ARTIST.name COLLATE NOCASE DESC, YEAR.year COLLATE NOCASE DESC, ALBUM.name COLLATE NOCASE DESC, TRACK.track_no COLLATE NOCASE DESC");
			else
				order_str = g_strdup("ARTIST.name COLLATE NOCASE DESC, ALBUM.name COLLATE NOCASE DESC, TRACK.track_no COLLATE NOCASE DESC");
			break;
		case GENRE_ARTIST:
			order_str = g_strdup("GENRE.name COLLATE NOCASE DESC, ARTIST.name COLLATE NOCASE DESC, TRACK.title COLLATE NOCASE DESC");
			break;
		case GENRE_ALBUM:
			if (rena_preferences_get_sort_by_year(clibrary->preferences))
				order_str = g_strdup("GENRE.name COLLATE NOCASE DESC, YEAR.year COLLATE NOCASE DESC, ALBUM.name COLLATE NOCASE DESC, TRACK.track_no COLLATE NOCASE DESC");
			else
				order_str = g_strdup("GENRE.name COLLATE NOCASE DESC, ALBUM.name COLLATE NOCASE DESC, TRACK.track_no COLLATE NOCASE DESC");
			break;
		case GENRE_ARTIST_ALBUM:
			if (rena_preferences_get_sort_by_year(clibrary->preferences))
				order_str = g_strdup("GENRE.name COLLATE NOCASE DESC, ARTIST.name COLLATE NOCASE DESC, YEAR.year COLLATE NOCASE DESC, ALBUM.name COLLATE NOCASE DESC, TRACK.track_no COLLATE NOCASE DESC");
			else
				order_str = g_strdup("GENRE.name COLLATE NOCASE DESC, ARTIST.name COLLATE NOCASE DESC, ALBUM.name COLLATE NOCASE DESC, TRACK.track_no COLLATE NOCASE DESC");
			break;
		default:
			break;
	}

	/* Common query for all tag based library views */
	sql = g_strdup_printf("SELECT TRACK.title, ARTIST.name, YEAR.year, ALBUM.name, GENRE.name, LOCATION.name, LOCATION.id "
	                        "FROM TRACK, ARTIST, YEAR, ALBUM, GENRE, LOCATION "
	                        "WHERE PROVIDER = ? AND ARTIST.id = TRACK.artist AND TRACK.year = YEAR.id AND ALBUM.id = TRACK.album AND GENRE.id = TRACK.genre AND LOCATION.id = TRACK.location "
	                        "ORDER BY %s;", order_str);

	statement = rena_database_create_statement (clibrary->cdbase, sql);
	provider_id = rena_database_find_provider (clibrary->cdbase, provider);
	rena_prepared_statement_bind_int (statement, 1, provider_id);

	while (rena_prepared_statement_step (statement)) {
		add_child_node_by_tags(model,
		                       p_iter,
		                       rena_prepared_statement_get_int(statement, 6),
		                       rena_prepared_statement_get_string(statement, 5),
		                       rena_prepared_statement_get_string(statement, 4),
		                       rena_prepared_statement_get_string(statement, 3),
		                       rena_prepared_statement_get_string(statement, 2),
		                       rena_prepared_statement_get_string(statement, 1),
		                       rena_prepared_statement_get_string(statement, 0),
		                       clibrary);

		/* Have to give control to GTK periodically ... */
		rena_process_gtk_events ();
	}
	rena_prepared_statement_free (statement);

	g_free(order_str);
	g_free(sql);
}

void
library_pane_view_reload(RenaLibraryPane *clibrary)
{
	RenaDatabaseProvider *provider;
	GdkPixbuf *pixbuf = NULL;
	GtkTreeModel *model, *filter_model;
	GtkTreeIter iter;
	GSList *provider_list, *l;
	gchar *icon_name, *friendly_name = NULL;

	clibrary->view_change = TRUE;

	set_watch_cursor (GTK_WIDGET(clibrary));

	filter_model = gtk_tree_view_get_model(GTK_TREE_VIEW(clibrary->library_tree));
	model = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(filter_model));

	g_object_ref(filter_model);

	gtk_widget_set_sensitive(GTK_WIDGET(GTK_WIDGET(clibrary)), FALSE);
	gtk_tree_view_set_model(GTK_TREE_VIEW(clibrary->library_tree), NULL);

	gtk_tree_store_clear(GTK_TREE_STORE(model));

	/* Playlists.*/

	gtk_tree_store_append(GTK_TREE_STORE(model),
			      &iter,
			      NULL);
	gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
			   L_PIXBUF, clibrary->pixbuf_dir,
			   L_NODE_DATA, _("Playlists"),
			   L_NODE_BOLD, PANGO_WEIGHT_BOLD,
			   L_NODE_TYPE, NODE_CATEGORY_PLAYLIST,
			   L_MACH, FALSE,
			   L_VISIBILE, TRUE,
			   -1);

	library_view_append_playlists(model, &iter, clibrary);

	/* Radios. */

	gtk_tree_store_append(GTK_TREE_STORE(model),
			      &iter,
			      NULL);
	gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
			   L_PIXBUF, clibrary->pixbuf_dir,
			   L_NODE_DATA, _("Radios"),
			   L_NODE_BOLD, PANGO_WEIGHT_BOLD,
			   L_NODE_TYPE, NODE_CATEGORY_RADIO,
			   L_MACH, FALSE,
			   L_VISIBILE, TRUE,
			   -1);

	library_view_append_radios(model, &iter, clibrary);

	/* Add library header */

	provider = rena_database_provider_get ();
	provider_list = rena_provider_get_visible_list (provider, TRUE);

	for (l = provider_list; l != NULL; l = l->next) {
		gtk_tree_store_append (GTK_TREE_STORE(model),
		                       &iter,
		                       NULL);

		icon_name = rena_database_provider_get_icon_name (provider, l->data);

		pixbuf  = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
		                                    icon_name,
		                                    get_library_icon_size(), GTK_ICON_LOOKUP_FORCE_SIZE,
		                                    NULL);

		friendly_name = rena_database_provider_get_friendly_name (provider, l->data);

		gtk_tree_store_set (GTK_TREE_STORE(model), &iter,
		                    L_PIXBUF, pixbuf ? pixbuf : clibrary->pixbuf_dir,
		                    L_NODE_DATA, friendly_name,
		                    L_NODE_BOLD, PANGO_WEIGHT_BOLD,
		                    L_NODE_TYPE, NODE_CATEGORY_PROVIDER,
		                    L_DATABASE_ID, rena_database_find_provider (clibrary->cdbase, l->data),
		                    L_MACH, FALSE,
		                    L_VISIBILE, TRUE,
		                    -1);

		if (pixbuf) {
			g_object_unref (pixbuf);
			pixbuf = NULL;
		}
		if (icon_name) {
			g_free (icon_name);
			icon_name = NULL;
		}
		if (friendly_name) {
			g_free (friendly_name);
			friendly_name = NULL;
		}

		if (rena_preferences_get_library_style(clibrary->preferences) == FOLDERS) {
			rena_library_view_append_provider_by_folder (clibrary, model, &iter, l->data);
		}
		else {
			rena_library_view_append_provider_by_tags (clibrary, model, &iter, l->data);
		}
	}

	/* Sensitive, set model and filter */

	gtk_widget_set_sensitive(GTK_WIDGET(GTK_WIDGET(clibrary)), TRUE);

	gtk_tree_view_set_model(GTK_TREE_VIEW(clibrary->library_tree), filter_model);
	g_object_unref(filter_model);
	
	if(gtk_entry_get_text_length (GTK_ENTRY(clibrary->search_entry)))
		g_signal_emit_by_name (G_OBJECT (clibrary->search_entry), "activate");
	else
		rena_library_expand_categories(clibrary);

	remove_watch_cursor (GTK_WIDGET(clibrary));

	clibrary->view_change = FALSE;

	g_slist_free_full (provider_list, g_free);
	g_object_unref (provider);
}

static void
update_library_playlist_changes (RenaDatabase *database,
                                 RenaLibraryPane *clibrary)
{
	GtkTreeModel *model, *filter_model;
	GtkTreeIter c_iter, iter;

	clibrary->view_change = TRUE;

	set_watch_cursor (GTK_WIDGET(clibrary));

	filter_model = gtk_tree_view_get_model(GTK_TREE_VIEW(clibrary->library_tree));
	model = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(filter_model));

	g_object_ref(filter_model);

	gtk_widget_set_sensitive(GTK_WIDGET(GTK_WIDGET(clibrary)), FALSE);
	gtk_tree_view_set_model(GTK_TREE_VIEW(clibrary->library_tree), NULL);

	if(find_child_node(_("Playlists"), &c_iter, NULL, model)) {
		while (gtk_tree_model_iter_nth_child(model, &iter, &c_iter, 0)) {
			gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);
		}
		library_view_append_playlists(model,
				              &c_iter,
				              clibrary);
	}

	if(find_child_node(_("Radios"), &c_iter, NULL, model)) {
		while (gtk_tree_model_iter_nth_child(model, &iter, &c_iter, 0)) {
			gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);
		}
		library_view_append_radios(model,
				           &c_iter,
				           clibrary);
	}

	gtk_widget_set_sensitive(GTK_WIDGET(GTK_WIDGET(clibrary)), TRUE);

	gtk_tree_view_set_model(GTK_TREE_VIEW(clibrary->library_tree), filter_model);
	g_object_unref(filter_model);

	if(gtk_entry_get_text_length (GTK_ENTRY(clibrary->search_entry)))
		g_signal_emit_by_name (G_OBJECT (clibrary->search_entry), "activate");
	else
		rena_library_expand_categories(clibrary);

	remove_watch_cursor (GTK_WIDGET(clibrary));

	clibrary->view_change = FALSE;
}

static void
update_library_tracks_changes(RenaDatabaseProvider *provider, RenaLibraryPane *library)
{
	/*
	 * Rework to olny update library tree!!!.
	 **/
	library_pane_view_reload(library);
}


/*
 * library_tree_context_menu calbacks
 */
GList *
rena_library_pane_get_mobj_list (RenaLibraryPane *library)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreePath *path;
	GList *mlist = NULL, *list, *i;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(library->library_tree));
	list = gtk_tree_selection_get_selected_rows (selection, &model);

	if (list) {
		/* Add all the rows to the current playlist */

		for (i = list; i != NULL; i = i->next) {
			path = i->data;
			mlist = append_library_row_to_mobj_list (library->cdbase, path, model, mlist);
			gtk_tree_path_free (path);

			/* Have to give control to GTK periodically ... */
			rena_process_gtk_events ();
		}
		g_list_free (list);
	}

	return mlist;
}

static void
rena_library_pane_rename_item (RenaLibraryPane *library)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreePath *path;
	GtkTreeIter iter;
	GList *list;
	gchar *playlist = NULL, *n_playlist = NULL;
	gint node_type;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(library->library_tree));
	list = gtk_tree_selection_get_selected_rows(selection, &model);

	if (list) {
		path = list->data;
		if (gtk_tree_path_get_depth(path) > 1) {
			gtk_tree_model_get_iter(model, &iter, path);
			gtk_tree_model_get(model, &iter, L_NODE_DATA, &playlist, -1);

			n_playlist = rename_playlist_dialog (playlist,
			                                     gtk_widget_get_toplevel(GTK_WIDGET(library)));
			if(n_playlist != NULL) {
				gtk_tree_model_get(model, &iter, L_NODE_TYPE, &node_type, -1);

				if(node_type == NODE_PLAYLIST)
					rena_database_update_playlist_name (library->cdbase, playlist, n_playlist);
				else if (node_type == NODE_RADIO)
					rena_database_update_radio_name (library->cdbase, playlist, n_playlist);

				rena_database_change_playlists_done(library->cdbase);

				g_free(n_playlist);
			}
			g_free(playlist);
		}
		gtk_tree_path_free(path);
	}
	g_list_free(list);
}

static void
rena_library_pane_remove_item (RenaLibraryPane *library)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreePath *path;
	GtkTreeIter iter;
	GList *list, *i;
	gchar *playlist;
	gint node_type;
	gboolean removed = FALSE;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(library->library_tree));
	list = gtk_tree_selection_get_selected_rows (selection, &model);

	if (list) {
		/* Delete selected playlists */

		for (i = list; i != NULL; i = i->next) {
			path = i->data;
			if (gtk_tree_path_get_depth(path) > 1) {
				gtk_tree_model_get_iter(model, &iter, path);
				gtk_tree_model_get(model, &iter, L_NODE_TYPE, &node_type, -1);
				gtk_tree_model_get(model, &iter, L_NODE_DATA, &playlist, -1);

				if (delete_existing_item_dialog(playlist, gtk_widget_get_toplevel(GTK_WIDGET(library)))) {
					if(node_type == NODE_PLAYLIST) {
						rena_database_delete_playlist (library->cdbase, playlist);
					}
					else if (node_type == NODE_RADIO) {
						rena_database_delete_radio (library->cdbase, playlist);
					}
					removed = TRUE;
				}
				g_free (playlist);
			}
			gtk_tree_path_free (path);
		}
		g_list_free (list);
	}

	if (removed)
		rena_database_change_playlists_done (library->cdbase);
}

static void
rena_library_pane_export_playlist (RenaLibraryPane *library)
{
	GtkWidget *toplevel;
	GIOChannel *chan = NULL;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreePath *path;
	GtkTreeIter iter;
	GList *list, *i;
	GError *err = NULL;
	gint cnt;
	gchar *filename = NULL, *playlist = NULL, *playlistpath = NULL;
	gint node_type;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW(library->library_tree));
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(library->library_tree));
	cnt = (gtk_tree_selection_count_selected_rows(selection));

	list = gtk_tree_selection_get_selected_rows(selection, NULL);
	path = list->data;

	/* If only is 'Playlist' node, just return, else get playlistname. */
	if ((cnt == 1) && (gtk_tree_path_get_depth(path) == 1)) {
		gtk_tree_path_free(path);
		g_list_free(list);
		return;
	}
	else {
		gtk_tree_model_get_iter(model, &iter, path);
		gtk_tree_model_get(model, &iter, L_NODE_DATA, &playlistpath, -1);

		gtk_tree_model_get(model, &iter, L_NODE_TYPE, &node_type, -1);
		if(node_type != NODE_PLAYLIST) {
			gtk_tree_path_free(path);
			g_list_free(list);
			return;
		}
	}

	toplevel = gtk_widget_get_toplevel(GTK_WIDGET(library));

	filename = playlist_export_dialog_get_filename(playlistpath, toplevel);

	if (!filename)
		goto exit;

	chan = create_m3u_playlist(filename);
	if (!chan) {
		g_warning("Unable to create M3U playlist file: %s", filename);
		goto exit;
	}

	set_watch_cursor (toplevel);

	list = gtk_tree_selection_get_selected_rows(selection, NULL);

	if (list) {
		/* Export all the playlists to the given file */

		for (i=list; i != NULL; i = i->next) {
			path = i->data;
			if (gtk_tree_path_get_depth(path) > 1) {
				gtk_tree_model_get_iter(model, &iter, path);
				gtk_tree_model_get(model, &iter, L_NODE_DATA,
						   &playlist, -1);
				if (save_m3u_playlist(chan, playlist,
						      filename, library->cdbase) < 0) {
					g_warning("Unable to save M3U playlist: %s",
						  filename);
					g_free(playlist);
					goto exit_list;
				}
				g_free(playlist);
			}
			gtk_tree_path_free(path);

			/* Have to give control to GTK periodically ... */
			rena_process_gtk_events ();
		}
	}

	if (chan) {
		if (g_io_channel_shutdown(chan, TRUE, &err) != G_IO_STATUS_NORMAL) {
			g_critical("Unable to save M3U playlist: %s", filename);
			g_error_free(err);
			err = NULL;
		} else {
			CDEBUG(DBG_INFO, "Saved M3U playlist: %s", filename);
		}
		g_io_channel_unref(chan);
	}

exit_list:
	remove_watch_cursor (toplevel);

	if (list)
		g_list_free(list);
exit:
	g_free(playlistpath);
	g_free(filename);
}

/*
 * library_tree_context_menu_xml calbacks
 */

static void
rena_library_pane_edit_tags_dialog_response (GtkWidget      *dialog,
                                               gint            response_id,
                                               RenaLibraryPane *library)
{
	RenaMusicobject *nmobj;
	RenaTagger *tagger;
	GArray *loc_arr = NULL;
	gint changed = 0, elem = 0, ielem;
	GtkWidget  *toplevel;

	toplevel = gtk_widget_get_toplevel (GTK_WIDGET(library));

	if (response_id == GTK_RESPONSE_HELP) {
		nmobj = rena_tags_dialog_get_musicobject(RENA_TAGS_DIALOG(dialog));
		rena_track_properties_dialog(nmobj, toplevel);
		return;
	}

	loc_arr = g_object_get_data (G_OBJECT (dialog), "local-array");

	if (response_id == GTK_RESPONSE_OK) {
		changed = rena_tags_dialog_get_changed(RENA_TAGS_DIALOG(dialog));
		if(!changed)
			goto no_change;

		nmobj = rena_tags_dialog_get_musicobject(RENA_TAGS_DIALOG(dialog));

		/* Updata the db changes */
		if(loc_arr) {
			if (changed & TAG_TNO_CHANGED) {
				if (loc_arr->len > 1) {
					if (!confirm_tno_multiple_tracks(rena_musicobject_get_track_no(nmobj), toplevel))
						return;
				}
			}
			if (changed & TAG_TITLE_CHANGED) {
				if (loc_arr->len > 1) {
					if (!confirm_title_multiple_tracks(rena_musicobject_get_title(nmobj), toplevel))
						return;
				}
			}

			tagger = rena_tagger_new();
			/* Get a array of files and update it */
			for(ielem = 0; ielem < loc_arr->len; ielem++) {
				elem = g_array_index(loc_arr, gint, ielem);
				if (G_LIKELY(elem))
					rena_tagger_add_location_id(tagger, elem);
			}
			rena_tagger_set_changes(tagger, nmobj, changed);
			rena_tagger_apply_changes (tagger);
			g_object_unref(tagger);
		}
	}

no_change:
	g_array_free (loc_arr, TRUE);
	gtk_widget_destroy (dialog);
}

static void
rena_library_pane_edit_tags (RenaLibraryPane *library)
{
	GtkWidget *dialog;
	LibraryNodeType node_type = 0;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreePath *path;
	GtkTreeIter iter;
	GList *list, *i;
	GArray *loc_arr = NULL;
	gint sel, location_id;
	gchar *node_data = NULL, **split_album = NULL;

	RenaMusicobject *omobj = NULL;

	dialog = rena_tags_dialog_new();
	gtk_window_set_transient_for (GTK_WINDOW(dialog),
		GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(library))));

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(library->library_tree));
	sel = gtk_tree_selection_count_selected_rows(selection);
	list = gtk_tree_selection_get_selected_rows(selection, &model);

	/* Setup initial entries */

	if (sel == 1) {
		path = list->data;

		if (!gtk_tree_model_get_iter(model, &iter, path))
			goto exit;

		gtk_tree_model_get(model, &iter, L_NODE_TYPE, &node_type, -1);

		if (node_type == NODE_TRACK || node_type == NODE_BASENAME) {
			gtk_tree_model_get(model, &iter,
					   L_DATABASE_ID, &location_id, -1);

			omobj = new_musicobject_from_db(library->cdbase, location_id);
		}
		else {
			omobj = rena_musicobject_new();
			gtk_tree_model_get(model, &iter, L_NODE_DATA, &node_data, -1);

			switch(node_type) {
			case NODE_ARTIST:
				rena_musicobject_set_artist(omobj, node_data);
				break;
			case NODE_ALBUM:
				if (rena_preferences_get_sort_by_year(library->preferences)) {
					split_album = g_strsplit(node_data, " - ", 2);
					rena_musicobject_set_year(omobj, atoi (split_album[0]));
					rena_musicobject_set_album(omobj, split_album[1]);
				}
				else {
					rena_musicobject_set_album(omobj, node_data);
				}
				break;
			case NODE_GENRE:
				rena_musicobject_set_genre(omobj, node_data);
				break;
			default:
				break;
			}
		}
	}

	if (omobj)
		rena_tags_dialog_set_musicobject(RENA_TAGS_DIALOG(dialog), omobj);

	loc_arr = g_array_new(TRUE, TRUE, sizeof(gint));
	for (i=list; i != NULL; i = i->next) {
		path = i->data;
		/* Form an array of location ids */
		get_location_ids(path, loc_arr, model, library);
	}
	g_object_set_data (G_OBJECT (dialog), "local-array", loc_arr);

	g_signal_connect (G_OBJECT (dialog), "response",
	                  G_CALLBACK (rena_library_pane_edit_tags_dialog_response), library);

	gtk_widget_show (dialog);

exit:
	g_free(node_data);
	g_strfreev (split_album);
	if (omobj)
		g_object_unref (omobj);
	g_list_free_full(list, (GDestroyNotify) gtk_tree_path_free);
}

static void
rena_library_pane_delete_from_hdd (RenaLibraryPane *library)
{
	RenaDatabaseProvider *provider;
	GtkWidget *dialog;
	GtkWidget *toggle_unlink;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreePath *path;
	GList *list, *i;
	gint result;
	GArray *loc_arr;
	gboolean unlink = FALSE;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(library->library_tree));
	list = gtk_tree_selection_get_selected_rows(selection, &model);

	if (list) {
		dialog = gtk_message_dialog_new (GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(library))),
		                                 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		                                 GTK_MESSAGE_QUESTION,
		                                 GTK_BUTTONS_YES_NO,
		                                 _("Really want to move the files to trash?"));

		toggle_unlink = gtk_check_button_new_with_label(_("Delete permanently instead of moving to trash"));
		gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), toggle_unlink, TRUE, TRUE, 0);

		gtk_widget_show_all(dialog);
		result = gtk_dialog_run(GTK_DIALOG(dialog));
		unlink = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle_unlink));
		gtk_widget_destroy(dialog);

		if(result == GTK_RESPONSE_YES){
			loc_arr = g_array_new(TRUE, TRUE, sizeof(gint));

			rena_database_begin_transaction(library->cdbase);
			for (i=list; i != NULL; i = i->next) {
				path = i->data;
				get_location_ids(path, loc_arr, model, library);
				trash_or_unlink_row(loc_arr, unlink, library);

				/* Have to give control to GTK periodically ... */
				rena_process_gtk_events ();
			}
			rena_database_commit_transaction(library->cdbase);

			g_array_free(loc_arr, TRUE);

			rena_database_flush_stale_entries (library->cdbase);

			provider = rena_database_provider_get ();
			rena_provider_update_done (provider);
			g_object_unref (provider);
		}

		g_list_free_full(list, (GDestroyNotify) gtk_tree_path_free);
	}
}

static void
rena_library_pane_delete_from_db (RenaLibraryPane *library)
{
	RenaDatabaseProvider *provider;
	GtkWidget *dialog;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreePath *path;
	GList *list, *i;
	gint result;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(library->library_tree));
	list = gtk_tree_selection_get_selected_rows(selection, &model);

	if (list) {
		dialog = gtk_message_dialog_new (GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(library))),
		                                 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		                                 GTK_MESSAGE_QUESTION,
		                                 GTK_BUTTONS_YES_NO,
		                                 _("Are you sure you want to delete current file from library?\n\n"
		                                 "Warning: To recover we must rescan the entire library."));

		result = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);

		if( result == GTK_RESPONSE_YES ){
			/* Delete all the rows */

			rena_database_begin_transaction (library->cdbase);

			for (i=list; i != NULL; i = i->next) {
				path = i->data;
				delete_row_from_db(library->cdbase, path, model);

				/* Have to give control to GTK periodically ... */
				rena_process_gtk_events ();
			}

			rena_database_commit_transaction (library->cdbase);

			rena_database_flush_stale_entries (library->cdbase);

			provider = rena_database_provider_get ();
			rena_provider_update_done (provider);
			g_object_unref (provider);
		}

		g_list_free_full(list, (GDestroyNotify) gtk_tree_path_free);
	}
}

static void
rena_library_pane_upgrade_library (RenaLibraryPane *library)
{
	RenaDatabaseProvider *provider;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreePath *path;
	GtkTreeIter iter;
	GList *list;
	LibraryNodeType node_type = 0;
	gint provider_id = 0;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(library->library_tree));
	list = gtk_tree_selection_get_selected_rows (selection, &model);

	if (list != NULL)
	{
		path = list->data;
		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_model_get (model, &iter, L_NODE_TYPE, &node_type, -1);
		if (node_type == NODE_CATEGORY_PROVIDER)
			gtk_tree_model_get (model, &iter, L_DATABASE_ID, &provider_id, -1);
	}

	if (provider_id > 0)
	{
		provider = rena_database_provider_get ();
		rena_provider_want_upgrade (provider, provider_id);
		g_object_unref (provider);
	}
}

static void
rena_library_pane_update_library (RenaLibraryPane *library)
{
	RenaDatabaseProvider *provider;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreePath *path;
	GtkTreeIter iter;
	GList *list;
	LibraryNodeType node_type = 0;
	gint provider_id = 0;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(library->library_tree));
	list = gtk_tree_selection_get_selected_rows (selection, &model);

	if (list != NULL)
	{
		path = list->data;
		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_model_get (model, &iter, L_NODE_TYPE, &node_type, -1);
		if (node_type == NODE_CATEGORY_PROVIDER)
			gtk_tree_model_get (model, &iter, L_DATABASE_ID, &provider_id, -1);
	}

	if (provider_id > 0)
	{
		provider = rena_database_provider_get ();
		rena_provider_want_update (provider, provider_id);
		g_object_unref (provider);
	}
}

static void
rena_library_pane_remove_library (RenaLibraryPane *library)
{
	RenaDatabaseProvider *provider;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreePath *path;
	GtkTreeIter iter;
	GList *list;
	LibraryNodeType node_type = 0;
	gint provider_id = 0;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(library->library_tree));
	list = gtk_tree_selection_get_selected_rows (selection, &model);

	if (list != NULL)
	{
		path = list->data;
		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_model_get (model, &iter, L_NODE_TYPE, &node_type, -1);
		if (node_type == NODE_CATEGORY_PROVIDER)
			gtk_tree_model_get (model, &iter, L_DATABASE_ID, &provider_id, -1);
	}

	if (provider_id > 0)
	{
		provider = rena_database_provider_get ();
		rena_provider_want_remove (provider, provider_id);
		g_object_unref (provider);
	}
}

/**************************************/
/* Construction menus of library pane */
/**************************************/

static void
rena_library_tree_context_menu_new (RenaLibraryPane *library)
{
	GError *error = NULL;

	library->builder = gtk_builder_new ();
	gtk_builder_add_from_string (library->builder, rena_window_ui, -1, &error);
	if (error) {
		g_print ("GtkBuilder error: %s", error->message);
		g_error_free (error);
		error = NULL;
	}

	library->actions =  g_simple_action_group_new ();
	g_action_map_add_action_entries (G_ACTION_MAP(library->actions),
	                                 library_menu_aentries,
	                                 G_N_ELEMENTS(library_menu_aentries),
	                                 (gpointer)library);
}

/********************************/
/* Construction of library pane */
/********************************/

static GtkTreeStore *
rena_library_pane_store_new()
{
	GtkTreeStore *store;
	store = gtk_tree_store_new(N_L_COLUMNS,
	                           GDK_TYPE_PIXBUF, /* Pixbuf */
	                           G_TYPE_STRING,   /* Node */
	                           G_TYPE_INT,      /* Bold */
	                           G_TYPE_INT,      /* Node type : Artist / Album / Track */
	                           G_TYPE_INT,      /* Location id (valid only for Track) */
	                           G_TYPE_BOOLEAN,  /* Flag to save mach when filtering */
	                           G_TYPE_BOOLEAN); /* Row visibility */
	return store;
}

static GtkWidget*
rena_library_pane_tree_new(RenaLibraryPane *clibrary)
{
	GtkWidget *library_tree;
	GtkTreeModel *library_filter_tree;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;

	/* Create the filter model */

	library_filter_tree = gtk_tree_model_filter_new(GTK_TREE_MODEL(clibrary->library_store), NULL);
	gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(library_filter_tree),
	                                         L_VISIBILE);

	/* Create the tree view */

	library_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(library_filter_tree));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(library_tree), FALSE);
	gtk_tree_view_set_show_expanders(GTK_TREE_VIEW(library_tree), TRUE);
	gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW(library_tree), TRUE);

	/* Set sidebar style */

	gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(library_tree)),
	                             GTK_STYLE_CLASS_SIDEBAR);

	/* Selection mode is multiple */

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(library_tree));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

	/* Create column and cell renderers */

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_attributes(column, renderer,
					    "pixbuf", L_PIXBUF,
					    NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_attributes(column, renderer,
					    "text", L_NODE_DATA,
					    "weight", L_NODE_BOLD,
					    NULL);
	g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(library_tree), column);

	g_object_unref(library_filter_tree);

	return library_tree;
}

static GtkWidget*
rena_library_pane_search_entry_new(RenaLibraryPane *clibrary)
{
	GtkWidget *search_entry;

	search_entry = rena_search_entry_new(clibrary->preferences);

	g_signal_connect (G_OBJECT(search_entry),
	                  "changed",
	                  G_CALLBACK(simple_library_search_keyrelease_handler),
	                  clibrary);
	g_signal_connect (G_OBJECT(search_entry),
	                  "activate",
	                  G_CALLBACK(simple_library_search_activate_handler),
	                  clibrary);

	return search_entry;
}

static void
rena_library_pane_create_widget (RenaLibraryPane *library)
{
	GtkWidget *library_tree_scroll;

	library_tree_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(library_tree_scroll),
	                                GTK_POLICY_AUTOMATIC,
	                                GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(library_tree_scroll),
	                                     GTK_SHADOW_IN);

	/* Package all */

	gtk_box_pack_start (GTK_BOX(library), library->search_entry,
	                    FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(library), library_tree_scroll,
	                    TRUE, TRUE, 0);

	gtk_container_add (GTK_CONTAINER(library_tree_scroll),
	                   library->library_tree);
}

static gint
get_library_icon_size (void)
{
  gint width, height;

  if (gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &width, &height))
    return MAX (width, height);
  else
    return 16;
}

static void
rena_library_pane_init_pixbufs(RenaLibraryPane *librarypane)
{
	gchar *pix_uri = NULL;
	GtkIconTheme *icontheme = gtk_icon_theme_get_default();
	gint icon_size = get_library_icon_size();

	pix_uri = g_build_filename (PIXMAPDIR, "artist.png", NULL);
	librarypane->pixbuf_artist =
		gdk_pixbuf_new_from_file_at_scale(pix_uri,
		                                  icon_size, icon_size,
		                                  TRUE,
		                                  NULL);
	if (!librarypane->pixbuf_artist)
		g_warning("Unable to load artist png");
	g_free (pix_uri);

	librarypane->pixbuf_album =
		gtk_icon_theme_load_icon(icontheme,
		                         "media-optical",
		                         icon_size, GTK_ICON_LOOKUP_FORCE_SIZE,
		                         NULL);

	if (!librarypane->pixbuf_album) {
		pix_uri = g_build_filename (PIXMAPDIR, "album.png", NULL);
		librarypane->pixbuf_album =
			gdk_pixbuf_new_from_file_at_scale(pix_uri,
			                                  icon_size, icon_size,
			                                  TRUE, NULL);
		g_free (pix_uri);
	}
	if (!librarypane->pixbuf_album)
		g_warning("Unable to load album png");

	librarypane->pixbuf_track =
		gtk_icon_theme_load_icon(icontheme,
		                         "audio-x-generic",
		                         icon_size, GTK_ICON_LOOKUP_FORCE_SIZE,
		                         NULL);
	if (!librarypane->pixbuf_track) {
		pix_uri = g_build_filename (PIXMAPDIR, "track.png", NULL);
		librarypane->pixbuf_track =
			gdk_pixbuf_new_from_file_at_scale(pix_uri,
			                                  icon_size, icon_size,
			                                  TRUE, NULL);
		g_free (pix_uri);
	}
	if (!librarypane->pixbuf_track)
		g_warning("Unable to load track png");

	pix_uri = g_build_filename (PIXMAPDIR, "genre.png", NULL);
	librarypane->pixbuf_genre =
		gdk_pixbuf_new_from_file_at_scale(pix_uri,
		                                  icon_size, icon_size,
		                                  TRUE, NULL);
	if (!librarypane->pixbuf_genre)
		g_warning("Unable to load genre png");
	g_free (pix_uri);

	librarypane->pixbuf_dir =
		gtk_icon_theme_load_icon(icontheme,
		                         "folder-music",
		                         icon_size, GTK_ICON_LOOKUP_FORCE_SIZE,
		                         NULL);
	if (!librarypane->pixbuf_dir)
		librarypane->pixbuf_dir =
			gtk_icon_theme_load_icon(icontheme,
			                         "folder",
			                         icon_size, GTK_ICON_LOOKUP_FORCE_SIZE,
			                         NULL);
	if (!librarypane->pixbuf_dir)
		g_warning("Unable to load folder png");
}

void
rena_library_pane_init_view (RenaLibraryPane *clibrary)
{
	library_pane_update_style(clibrary);
	library_pane_view_reload(clibrary);
}

GtkWidget *
rena_library_pane_get_widget(RenaLibraryPane *librarypane)
{
	return GTK_WIDGET(librarypane);
}

GtkWidget *
rena_library_pane_get_pane_title (RenaLibraryPane *library)
{
	return library->pane_title;
}

GtkWidget *
rena_library_pane_get_popover (RenaLibraryPane *library)
{
	GMenuModel *model;
	GtkWidget *popover;

	model = G_MENU_MODEL(gtk_builder_get_object (library->builder, "library-order-menu"));
	popover = gtk_popover_new_from_model (library->pane_title, model);
	gtk_widget_insert_action_group (popover, "lib", G_ACTION_GROUP(library->actions));

	return popover;
}

static void
rena_library_pane_init (RenaLibraryPane *library)
{
	RenaDatabaseProvider *provider;

	gtk_orientable_set_orientation (GTK_ORIENTABLE (library), GTK_ORIENTATION_VERTICAL);
	g_object_set (G_OBJECT(library), "spacing", 2, NULL);

	/* Get usefuls instances */

	library->cdbase = rena_database_get ();
	library->preferences = rena_preferences_get ();

	/* Create the store */

	library->library_store = rena_library_pane_store_new();

	/* Create the widgets */

	library->search_entry = rena_library_pane_search_entry_new (library);
	library->library_tree = rena_library_pane_tree_new (library);
	library->pane_title = gtk_label_new("");
	gtk_widget_set_halign (GTK_WIDGET(library->pane_title), GTK_ALIGN_START);
	gtk_widget_set_valign (GTK_WIDGET(library->pane_title), GTK_ALIGN_CENTER);

	/* Create main widget */

	rena_library_pane_create_widget (library);

	/* Create context menus */

	rena_library_tree_context_menu_new (library);

	/* Init the rest of flags */

	library->filter_entry = NULL;
	library->dragging = FALSE;
	library->view_change = FALSE;
	library->library_tree_nodes = NULL;

	/* Init drag and drop */

	library_pane_init_dnd (library);

	/* Init pixbufs */

	rena_library_pane_init_pixbufs (library);

	/* Conect signals */

	g_signal_connect (G_OBJECT(library->library_tree), "row-activated",
	                  G_CALLBACK(library_tree_row_activated_cb), library);
	g_signal_connect (G_OBJECT(library->library_tree), "button-press-event",
	                  G_CALLBACK(rena_library_pane_tree_button_press_cb), library);
	g_signal_connect (G_OBJECT(library->library_tree), "button-release-event",
	                  G_CALLBACK(library_tree_button_release_cb), library);
	g_signal_connect (G_OBJECT (library->library_tree), "key-press-event",
	                  G_CALLBACK(rena_library_pane_tree_key_press), library);

	g_signal_connect (library->cdbase, "PlaylistsChanged",
	                  G_CALLBACK (update_library_playlist_changes), library);

	g_signal_connect (library->preferences, "notify::library-style",
	                  G_CALLBACK (library_pane_change_style), library);

	provider = rena_database_provider_get ();
	g_signal_connect (provider, "update-done",
	                  G_CALLBACK (update_library_tracks_changes), library);
	g_object_unref (provider);

	gtk_widget_show_all (GTK_WIDGET(library));
}

static void
rena_library_pane_finalize (GObject *object)
{
	RenaLibraryPane *library = RENA_LIBRARY_PANE (object);

	if (library->pixbuf_dir)
		g_object_unref (library->pixbuf_dir);
	if (library->pixbuf_artist)
		g_object_unref (library->pixbuf_artist);
	if (library->pixbuf_album)
		g_object_unref (library->pixbuf_album);
	if (library->pixbuf_track)
		g_object_unref (library->pixbuf_track);
	if (library->pixbuf_genre)
		g_object_unref (library->pixbuf_genre);

	if (library->filter_entry != NULL) {
		g_free (library->filter_entry);
		library->filter_entry = NULL;
	}

	g_object_unref (library->cdbase);
	g_object_unref (library->preferences);
	g_object_unref (library->library_store);

	g_slist_free (library->library_tree_nodes);

	g_object_unref (library->builder);
	g_object_unref (library->actions);

	(*G_OBJECT_CLASS (rena_library_pane_parent_class)->finalize) (object);
}

static void
rena_library_pane_class_init (RenaLibraryPaneClass *klass)
{
	GObjectClass  *gobject_class;

	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = rena_library_pane_finalize;

	/*
	 * Signals:
	 */
	signals[LIBRARY_APPEND_PLAYLIST] =
		g_signal_new ("library-append-playlist",
		              G_TYPE_FROM_CLASS (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (RenaLibraryPaneClass, library_append_playlist),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);
	signals[LIBRARY_REPLACE_PLAYLIST] =
		g_signal_new ("library-replace-playlist",
		              G_TYPE_FROM_CLASS (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (RenaLibraryPaneClass, library_replace_playlist),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);
	signals[LIBRARY_REPLACE_PLAYLIST_AND_PLAY] =
		g_signal_new ("library-replace-playlist-and-play",
		              G_TYPE_FROM_CLASS (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (RenaLibraryPaneClass, library_replace_playlist_and_play),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);
	signals[LIBRARY_ADDTO_PLAYLIST_AND_PLAY] =
		g_signal_new ("library-addto-playlist-and-play",
		              G_TYPE_FROM_CLASS (gobject_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (RenaLibraryPaneClass, library_addto_playlist_and_play),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);
}

RenaLibraryPane *
rena_library_pane_new (void)
{
	return g_object_new (RENA_TYPE_LIBRARY_PANE, NULL);
}
