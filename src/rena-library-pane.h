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

#ifndef RENA_LIBRARY_PANE_H
#define RENA_LIBRARY_PANE_H

#include <gtk/gtk.h>
#include "rena-preferences.h"

#define RENA_TYPE_LIBRARY_PANE                  (rena_library_pane_get_type ())
#define RENA_LIBRARY_PANE(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_LIBRARY_PANE, RenaLibraryPane))
#define RENA_IS_LIBRARY_PANE(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_LIBRARY_PANE))
#define RENA_LIBRARY_PANE_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), RENA_TYPE_LIBRARY_PANE, RenaLibraryPaneClass))
#define RENA_IS_LIBRARY_PANE_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), RENA_TYPE_LIBRARY_PANE))
#define RENA_LIBRARY_PANE_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), RENA_TYPE_LIBRARY_PANE, RenaLibraryPaneClass))

typedef struct _RenaLibraryPane RenaLibraryPane;

typedef struct {
	GtkBoxClass __parent__;
	void (*library_append_playlist) (RenaLibraryPane *toolbar);
	void (*library_replace_playlist) (RenaLibraryPane *toolbar);
	void (*library_replace_playlist_and_play) (RenaLibraryPane *toolbar);
	void (*library_addto_playlist_and_play) (RenaLibraryPane *toolbar);
} RenaLibraryPaneClass;

/* Library Views */

typedef enum {
	FOLDERS,
	ARTIST,
	ALBUM,
	GENRE,
	ARTIST_ALBUM,
	GENRE_ARTIST,
	GENRE_ALBUM,
	GENRE_ARTIST_ALBUM,
	LAST_LIBRARY_STYLE
} RenaLibraryStyle;

/* Functions */

GList * rena_library_pane_get_mobj_list (RenaLibraryPane *library);

gboolean simple_library_search_activate_handler   (GtkEntry *entry, RenaLibraryPane *clibrary);
void     clear_library_search                     (RenaLibraryPane *clibrary);

gboolean rena_library_need_update_view (RenaPreferences *preferences, gint changed);
gboolean rena_library_need_update    (RenaLibraryPane *clibrary, gint changed);
void     library_pane_view_reload      (RenaLibraryPane *clibrary);
void     rena_library_pane_init_view (RenaLibraryPane *clibrary);

GtkWidget         *rena_library_pane_get_widget     (RenaLibraryPane *librarypane);
GtkWidget         *rena_library_pane_get_pane_title (RenaLibraryPane *library);
GtkWidget         *rena_library_pane_get_popover    (RenaLibraryPane *library);

GtkUIManager      *rena_library_pane_get_pane_context_menu(RenaLibraryPane *clibrary);

RenaLibraryPane *rena_library_pane_new        (void);

#endif /* RENA_LIBRARY_PANE_H */
