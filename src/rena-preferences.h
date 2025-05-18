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

#ifndef RENA_PREFERENCES_H
#define RENA_PREFERENCES_H

#include <gtk/gtk.h>
#include <glib-object.h>

G_BEGIN_DECLS

GType rena_preferences_get_type (void) G_GNUC_CONST;
#define RENA_TYPE_PREFERENCES (rena_preferences_get_type())
#define RENA_PREFERENCES(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_PREFERENCES, RenaPreferences))
#define RENA_PREFERENCES_CONST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_PREFERENCES, RenaPreferences const))
#define RENA_PREFERENCES_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), RENA_TYPE_PREFERENCES, RenaPreferencesClass))
#define RENA_IS_PREFERENCES(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_PREFERENCES))
#define RENA_IS_PREFERENCES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), RENA_TYPE_PREFERENCES))
#define RENA_PREFERENCES_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), RENA_TYPE_PREFERENCES, RenaPreferencesClass))

typedef struct _RenaPreferences RenaPreferences;
typedef struct _RenaPreferencesClass RenaPreferencesClass;
typedef struct _RenaPreferencesPrivate RenaPreferencesPrivate;

struct _RenaPreferences
{
	GObject parent;

	/*< private >*/
	RenaPreferencesPrivate *priv;
};

struct _RenaPreferencesClass
{
	GObjectClass parent_class;
	void (*plugins_change) (RenaPreferences *preferences, const gchar *key);
  void (*library_change) (RenaPreferences *preferences);
  void (*need_restart) (RenaPreferences *preferences);
};

/* Defines to key preferences. */

#define GROUP_GENERAL  "General"
#define KEY_INSTALLED_VERSION      "installed_version"
#define KEY_LAST_FOLDER            "last_folder"
#define KEY_ADD_RECURSIVELY_FILES  "add_recursively_files"
#define KEY_ALBUM_ART_PATTERN      "album_art_pattern"
#define KEY_TIMER_REMAINING_MODE   "timer_remaining_mode"
#define KEY_SHOW_ICON_TRAY         "show_icon_tray"
#define KEY_CLOSE_TO_TRAY          "close_to_tray"
#define KEY_INSTANT_SEARCH         "instant_filter"
#define KEY_APPROXIMATE_SEARCH     "aproximate_search"
#define KEY_CACHE_SIZE             "cache_size"

#define GROUP_PLAYLIST "Playlist"
#define KEY_SAVE_PLAYLIST          "save_playlist"
#define KEY_CURRENT_REF		   "current_ref"
#define KEY_SHUFFLE                "shuffle"
#define KEY_REPEAT                 "repeat"
#define KEY_PLAYLIST_COLUMNS       "playlist_columns"
#define KEY_PLAYLIST_COLUMN_WIDTHS "playlist_column_widths"

#define GROUP_LIBRARY  "Library"
#define KEY_LIBRARY_DIR            "library_dir"
#define KEY_LIBRARY_SCANNED        "library_scanned"
#define KEY_LIBRARY_VIEW_ORDER     "library_view_order"
#define KEY_LIBRARY_LAST_SCANNED   "library_last_scanned"
#define KEY_SORT_BY_YEAR           "library_sort_by_year"

#define GROUP_AUDIO    "Audio"
#define KEY_AUDIO_SINK             "audio_sink"
#define KEY_AUDIO_DEVICE           "audio_device"
#define KEY_SOFTWARE_MIXER         "software_mixer"
#define KEY_SOFTWARE_VOLUME        "software_volume"
#define KEY_IGNORE_ERRORS          "ignore_errors"
#define KEY_EQ_10_BANDS            "equealizer_10_bands"
#define KEY_EQ_PRESET              "equalizer_preset"

#define GROUP_WINDOW   "Window"
#define KEY_REMEMBER_STATE          "remember_window_state"
#define KEY_START_MODE              "start_mode"
#define KEY_WINDOW_SIZE            "window_size"
#define KEY_WINDOW_POSITION        "window_position"
#define KEY_SIDEBAR                "sidebar"
#define KEY_SIDEBAR_SIZE           "sidebar_size"
#define KEY_SECONDARY_SIDEBAR      "secondary_sidebar"
#define KEY_SECONDARY_SIDEBAR_SIZE "secondary_sidebar_size"
#define KEY_SHOW_ALBUM_ART         "show_album_art"
#define KEY_ALBUM_ART_SIZE         "album_art_size"
#define KEY_TOOLBAR_SIZE           "toolbar_icon_size"
#define KEY_SYSTEM_TITLEBAR        "system_titlebar"
#define KEY_CONTROLS_BELOW         "controls_below"
#define KEY_SHOW_MENUBAR           "show_menubar"

/* Some default preferences. */

#define DEFAULT_SIDEBAR_SIZE       200
#define DEFAULT_ALBUM_ART_SIZE     32

#define DEFAULT_SINK               "default"
#define ALSA_SINK                  "alsa"
#define OSS4_SINK                  "oss4"
#define OSS_SINK                   "oss"
#define PULSE_SINK                 "pulse"

#define ALSA_DEFAULT_DEVICE        "default"
#define OSS_DEFAULT_DEVICE         "/dev/dsp"

#define ALBUM_ART_NO_PATTERNS      6

/* Some useful macros. */

#define NORMAL_STATE               "normal"
#define FULLSCREEN_STATE           "fullscreen"
#define ICONIFIED_STATE            "iconified"


RenaPreferences *
rena_preferences_get (void);

/*
 * Generic api to accessing other preferences.
 */

gboolean
rena_preferences_get_boolean (RenaPreferences *preferences,
                                const gchar       *group_name,
                                const gchar       *key);
void
rena_preferences_set_boolean (RenaPreferences *preferences,
                                const gchar       *group_name,
                                const gchar       *key,
                                gboolean           sbool);

gint *
rena_preferences_get_integer_list (RenaPreferences *preferences,
                                     const gchar *group_name,
                                     const gchar *key,
                                     gsize *length);
void
rena_preferences_set_integer_list (RenaPreferences *preferences,
                                     const gchar *group_name,
                                     const gchar *key,
                                     gint list[],
                                     gsize length);

gdouble *
rena_preferences_get_double_list (RenaPreferences *preferences,
                                    const gchar *group_name,
                                    const gchar *key);
void
rena_preferences_set_double_list (RenaPreferences *preferences,
                                    const gchar *group_name,
                                    const gchar *key,
                                    gdouble list[],
                                    gsize length);

gint
rena_preferences_get_integer (RenaPreferences *preferences,
                                const gchar       *group_name,
                                const gchar       *key);

void
rena_preferences_set_integer (RenaPreferences *preferences,
                                const gchar       *group_name,
                                const gchar       *key,
                                      gint         integer);

gchar *
rena_preferences_get_string (RenaPreferences *preferences,
                               const gchar *group_name,
                               const gchar *key);

void
rena_preferences_set_string (RenaPreferences *preferences,
                               const gchar *group_name,
                               const gchar *key,
                               const gchar *string);

gchar **
rena_preferences_get_string_list (RenaPreferences *preferences,
                                    const gchar *group_name,
                                    const gchar *key,
                                    gsize *length);
void
rena_preferences_set_string_list (RenaPreferences *preferences,
                                    const gchar *group_name,
                                    const gchar *key,
                                    const gchar * const list[],
                                    gsize length);

GSList *
rena_preferences_get_filename_list (RenaPreferences *preferences,
                                      const gchar *group_name,
                                      const gchar *key);
void
rena_preferences_set_filename_list (RenaPreferences *preferences,
                                      const gchar *group_name,
                                      const gchar *key,
                                      GSList *list);

/**
 * rena_preferences_remove_key:
 *
 */
void
rena_preferences_remove_key (RenaPreferences *preferences,
                               const gchar       *group_name,
                               const gchar       *key);

gboolean
rena_preferences_has_group (RenaPreferences *preferences,
                              const gchar       *group_name);

void
rena_preferences_remove_group (RenaPreferences *preferences,
                                 const gchar       *group_name);

/*
 * Specific plugin api.
 */

void
rena_preferences_plugin_changed (RenaPreferences *preferences,
                                   const gchar       *key);

gchar *
rena_preferences_get_plugin_group_name (RenaPreferences *preferences,
                                          const gchar       *plugin_name);

/*
 * Public api.
 */

void
rena_preferences_need_restart (RenaPreferences *preferences);

void
rena_preferences_local_provider_changed (RenaPreferences *preferences);

const gchar *
rena_preferences_get_installed_version (RenaPreferences *preferences);

void
rena_preferences_set_approximate_search (RenaPreferences *prefernces,
                                           gboolean approximate_search);
gboolean
rena_preferences_get_approximate_search (RenaPreferences *preferences);

void
rena_preferences_set_instant_search (RenaPreferences *preferences,
                                       gboolean instant_search);
gboolean
rena_preferences_get_instant_search (RenaPreferences *preferences);

void
rena_preferences_set_library_style (RenaPreferences *preferences,
                                      gint library_style);
gint
rena_preferences_get_library_style (RenaPreferences *preferences);

void
rena_preferences_set_sort_by_year (RenaPreferences *preferences,
                                     gboolean sort_by_year);
gboolean
rena_preferences_get_sort_by_year (RenaPreferences *preferences);

void
rena_preferences_set_shuffle (RenaPreferences *preferences,
                                gboolean shuffle);
gboolean
rena_preferences_get_shuffle (RenaPreferences *preferences);

void
rena_preferences_set_repeat (RenaPreferences *preferences,
                               gboolean repeat);
gboolean
rena_preferences_get_repeat (RenaPreferences *preferences);

void
rena_preferences_set_restore_playlist (RenaPreferences *preferences,
                                         gboolean restore_playlist);
gboolean
rena_preferences_get_restore_playlist (RenaPreferences *preferences);

const gchar *
rena_preferences_get_audio_sink (RenaPreferences *preferences);

void
rena_preferences_set_audio_sink (RenaPreferences *preferences,
                                   const gchar *audio_sink);

const gchar *
rena_preferences_get_audio_device (RenaPreferences *preferences);

void
rena_preferences_set_audio_device (RenaPreferences *preferences,
                                     const gchar *audio_device);

gboolean
rena_preferences_get_software_mixer (RenaPreferences *preferences);

void
rena_preferences_set_software_mixer (RenaPreferences *preferences,
                                       gboolean software_mixer);

gdouble
rena_preferences_get_software_volume (RenaPreferences *preferences);

void
rena_preferences_set_software_volume (RenaPreferences *preferences,
                                        gdouble software_volume);

gboolean
rena_preferences_get_ignore_errors (RenaPreferences *preferences);

void
rena_preferences_set_ignore_errors (RenaPreferences *preferences,
                                      gboolean           ignore_errors);


gboolean
rena_preferences_get_lateral_panel (RenaPreferences *preferences);

void
rena_preferences_set_lateral_panel (RenaPreferences *preferences,
                                      gboolean lateral_panel);

gboolean
rena_preferences_get_secondary_lateral_panel (RenaPreferences *preferences);

void
rena_preferences_set_secondary_lateral_panel (RenaPreferences *preferences,
                                                gboolean secondary_lateral_panel);

gboolean
rena_preferences_get_show_album_art (RenaPreferences *preferences);

void
rena_preferences_set_show_album_art (RenaPreferences *preferences,
                                       gboolean show_album_art);

gint
rena_preferences_get_album_art_size (RenaPreferences *preferences);

void
rena_preferences_set_album_art_size (RenaPreferences *preferences,
                                       gint album_art_size);

const gchar *
rena_preferences_get_album_art_pattern (RenaPreferences *preferences);

void
rena_preferences_set_album_art_pattern (RenaPreferences *preferences,
                                          const gchar *album_art_pattern);

GtkIconSize
rena_preferences_get_toolbar_size (RenaPreferences *preferences);

void
rena_preferences_set_toolbar_size (RenaPreferences *preferences,
                                     GtkIconSize        toolbar_size);

gboolean
rena_preferences_get_show_status_icon (RenaPreferences *preferences);

void
rena_preferences_set_show_status_icon (RenaPreferences *preferences,
                                         gboolean show_status_icon);

gboolean
rena_preferences_get_show_menubar (RenaPreferences *preferences);

void
rena_preferences_set_show_menubar (RenaPreferences *preferences,
                                     gboolean           show_menubar);

gboolean
rena_preferences_get_system_titlebar (RenaPreferences *preferences);

void
rena_preferences_set_system_titlebar (RenaPreferences *preferences,
                                        gboolean           system_titlebar);

gboolean
rena_preferences_get_controls_below (RenaPreferences *preferences);

void
rena_preferences_set_controls_below (RenaPreferences *preferences,
                                       gboolean controls_below);

gboolean
rena_preferences_get_remember_state (RenaPreferences *preferences);

void
rena_preferences_set_remember_state (RenaPreferences *preferences,
                                       gboolean remember_state);

gint
rena_preferences_get_sidebar_size (RenaPreferences *preferences);

void
rena_preferences_set_sidebar_size (RenaPreferences *preferences,
                                     gint sidebar_size);

gint
rena_preferences_get_secondary_sidebar_size (RenaPreferences *preferences);

void
rena_preferences_set_secondary_sidebar_size (RenaPreferences *preferences,
                                               gint secondary_sidebar_size);

const gchar *
rena_preferences_get_start_mode (RenaPreferences *preferences);

void
rena_preferences_set_start_mode (RenaPreferences *preferences,
                                   const gchar *start_mode);

const gchar *
rena_preferences_get_last_folder (RenaPreferences *preferences);

void
rena_preferences_set_last_folder (RenaPreferences *preferences,
                                    const gchar *last_folder);

gboolean
rena_preferences_get_add_recursively (RenaPreferences *preferences);

void
rena_preferences_set_add_recursively(RenaPreferences *preferences,
                                       gboolean add_recursively);

gboolean
rena_preferences_get_timer_remaining_mode (RenaPreferences *preferences);

void
rena_preferences_set_timer_remaining_mode(RenaPreferences *preferences,
                                            gboolean add_recursively);

gboolean
rena_preferences_get_hide_instead_close (RenaPreferences *preferences);

void
rena_preferences_set_hide_instead_close (RenaPreferences *preferences,
                                           gboolean hide_instead_close);

gboolean
rena_preferences_get_lock_library (RenaPreferences *preferences);

void
rena_preferences_set_lock_library (RenaPreferences *preferences,
                                     gboolean           lock_library);

G_END_DECLS

#endif /* RENA_PREFERENCES_H */
