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

#ifndef RENA_APPLICATION_H
#define RENA_APPLICATION_H

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <gtk/gtk.h>

#include "rena-album-art.h"
#include "rena-art-cache.h"
#include "rena-backend.h"
#include "rena-database.h"
#include "rena-preferences.h"
#include "rena-preferences-dialog.h"
#include "rena-playlist.h"
#include "rena-library-pane.h"
#include "rena-toolbar.h"
#include "rena-scanner.h"
#include "rena-sidebar.h"
#include "rena-statusbar.h"
#include "rena-statusicon.h"
#include "rena-debug.h"

#ifdef HAVE_LIBPEAS
#include "rena-plugins-engine.h"
#endif

G_BEGIN_DECLS

/* Some default preferences. */

#define MIN_WINDOW_WIDTH           (gdk_screen_width() * 3 / 4)
#define MIN_WINDOW_HEIGHT          (gdk_screen_height() * 3 / 4)
#define COL_WIDTH_THRESH           30
#define DEFAULT_PLAYLIST_COL_WIDTH ((MIN_WINDOW_WIDTH - DEFAULT_SIDEBAR_SIZE) / 4)

typedef struct _RenaApplication RenaApplication;

void               rena_application_open_files          (RenaApplication *rena);
void               rena_application_add_location        (RenaApplication *rena);
void               rena_application_append_entery_libary(RenaApplication *rena);
void               rena_application_about_dialog        (RenaApplication *rena);

/* Functions to access private members */

RenaPreferences *rena_application_get_preferences     (RenaApplication *rena);
RenaDatabase    *rena_application_get_database        (RenaApplication *rena);
RenaArtCache    *rena_application_get_art_cache       (RenaApplication *rena);

RenaBackend     *rena_application_get_backend         (RenaApplication *rena);

#ifdef HAVE_LIBPEAS
RenaPluginsEngine *rena_application_get_plugins_engine (RenaApplication *rena);
#endif

RenaScanner     *rena_application_get_scanner         (RenaApplication *rena);

GtkWidget         *rena_application_get_window          (RenaApplication *rena);
RenaPlaylist    *rena_application_get_playlist        (RenaApplication *rena);
RenaLibraryPane *rena_application_get_library         (RenaApplication *rena);

RenaPreferencesDialog *rena_application_get_preferences_dialog (RenaApplication *rena);

RenaToolbar     *rena_application_get_toolbar         (RenaApplication *rena);
GtkWidget         *rena_application_get_overlay         (RenaApplication *rena);
RenaSidebar     *rena_application_get_first_sidebar   (RenaApplication *rena);
GtkWidget         *rena_application_get_main_stack      (RenaApplication *rena);
RenaSidebar     *rena_application_get_second_sidebar  (RenaApplication *rena);
RenaStatusbar   *rena_application_get_statusbar       (RenaApplication *rena);
RenaStatusIcon  *rena_application_get_status_icon     (RenaApplication *rena);

GtkBuilder        *rena_application_get_menu_ui            (RenaApplication *rena);
GtkUIManager      *rena_application_get_menu_ui_manager    (RenaApplication *rena);
GtkAction         *rena_application_get_menu_action        (RenaApplication *rena, const gchar *path);
GtkWidget         *rena_application_get_menu_action_widget (RenaApplication *rena, const gchar *path);
GtkWidget         *rena_application_get_menubar            (RenaApplication *rena);
GtkWidget         *rena_application_get_infobox_container  (RenaApplication *rena);
GtkWidget         *rena_application_get_first_pane         (RenaApplication *rena);
GtkWidget         *rena_application_get_second_pane        (RenaApplication *rena);

gboolean           rena_application_is_first_run           (RenaApplication *rena);

gint handle_command_line (RenaApplication *rena, GApplicationCommandLine *command_line, gint argc, gchar **args);

/* Info bar import music */

gboolean info_bar_import_music_will_be_useful(RenaApplication *rena);
GtkWidget* create_info_bar_import_music(RenaApplication *rena);
GtkWidget* create_info_bar_update_music(RenaApplication *rena);
GtkWidget *rena_info_bar_need_restart (RenaApplication *rena);

/* Rena app */

#define RENA_TYPE_APPLICATION            (rena_application_get_type ())
#define RENA_APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), RENA_TYPE_APPLICATION, RenaApplication))
#define RENA_APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  RENA_TYPE_APPLICATION, RenaApplicationClass))
#define RENA_IS_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), RENA_TYPE_APPLICATION))
#define RENA_IS_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  RENA_TYPE_APPLICATION))
#define RENA_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  RENA_TYPE_APPLICATION, RenaApplicationClass))

GType    rena_application_get_type        (void) G_GNUC_CONST;

typedef struct {
	GtkApplicationClass parent_class;
} RenaApplicationClass;

void                rena_application_quit (RenaApplication *rena);
RenaApplication * rena_application_new  ();

G_END_DECLS

#endif /* RENA_APPLICATION_H */
