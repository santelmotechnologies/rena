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

#include "rena-preferences.h"

#include <stdio.h> /* TODO: Port this to glib!!. */
#include <errno.h>
#include <fcntl.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "rena-musicobject.h"
#include "rena-utils.h"
#include "rena-library-pane.h"
#include "rena-debug.h"

struct _RenaPreferencesPrivate
{
	GKeyFile  *rc_keyfile;
	gchar     *rc_filepath;

	/* Useful to rena. */
	gchar     *installed_version;
	/* Search preferences. */
	gboolean   instant_search;
	gboolean   approximate_search;
	/* LibraryPane preferences */
	gint       library_style;
	gboolean   sort_by_year;
	/* Playlist preferences. */
	gboolean   shuffle;
	gboolean   repeat;
	gboolean   restore_playlist;
	/* Audio preferences. */
	gchar     *audio_sink;
	gchar     *audio_device;
	gboolean   software_mixer;
	gdouble    software_volume;
	gboolean   ignore_errors;
	/* Window preferences. */
	gboolean   lateral_panel;
	gint       sidebar_size;
	gboolean   secondary_lateral_panel;
	gint       secondary_sidebar_size;
	gboolean   show_album_art;
	gint       album_art_size;
	gchar     *album_art_pattern;
	GtkIconSize toolbar_size;
	gboolean   show_status_icon;
	gboolean   show_menubar;
	gboolean   system_titlebar;
	gboolean   controls_below;
	gboolean   remember_state;
	gchar     *start_mode;
	/* Misc preferences. */
	gchar     *last_folder;
	gboolean   add_recursively;
	gboolean   timer_remaining_mode;
	gboolean   hide_instead_close;

	/* Properties without backup. */
	gboolean   lock_library;
};

G_DEFINE_TYPE_WITH_PRIVATE(RenaPreferences, rena_preferences, G_TYPE_OBJECT)

enum
{
	PROP_0,
	PROP_INSTANT_SEARCH,
	PROP_APPROXIMATE_SEARCH,
	PROP_LIBRARY_STYLE,
	PROP_LIBRARY_SORT_BY_YEAR,
	PROP_SHUFFLE,
	PROP_REPEAT,
	PROP_RESTORE_PLAYLIST,
	PROP_AUDIO_SINK,
	PROP_AUDIO_DEVICE,
	PROP_SOFTWARE_MIXER,
	PROP_SOFTWARE_VOLUME,
	PROP_IGNORE_ERRORS,
	PROP_LATERAL_PANEL,
	PROP_SIDEBAR_SIZE,
	PROP_SECONDARY_LATERAL_PANEL,
	PROP_SECONDARY_SIDEBAR_SIZE,
	PROP_SHOW_ALBUM_ART,
	PROP_ALBUM_ART_SIZE,
	PROP_ALBUM_ART_PATTERN,
	PROP_TOOLBAR_SIZE,
	PROP_SHOW_STATUS_ICON,
	PROP_SHOW_MENUBAR,
	PROP_SYSTEM_TITLEBAR,
	PROP_CONTROLS_BELOW,
	PROP_REMEMBER_STATE,
	PROP_START_MODE,
	PROP_LAST_FOLDER,
	PROP_ADD_RECURSIVELY,
	PROP_TIMER_REMAINING_MODE,
	PROP_HIDE_INSTEAD_CLOSE,
	PROP_LOCK_LIBRARY,
	LAST_PROP
};
static GParamSpec *gParamSpecs[LAST_PROP];

enum {
	SIGNAL_PLUGINS_CHANGED,
	SIGNAL_LIBRARY_CHANGED,
	SIGNAL_NEED_RESTART,
	LAST_SIGNAL
};
static int signals[LAST_SIGNAL] = { 0 };

gboolean
rena_preferences_get_boolean (RenaPreferences *preferences,
                                const gchar       *group_name,
                                const gchar       *key)
{
	g_return_val_if_fail (RENA_IS_PREFERENCES(preferences), FALSE);

	return g_key_file_get_boolean (preferences->priv->rc_keyfile,
	                               group_name,
	                               key,
	                               NULL);
}

void
rena_preferences_set_boolean (RenaPreferences *preferences,
                                const gchar       *group_name,
                                const gchar       *key,
                                gboolean           sbool)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	g_key_file_set_boolean (preferences->priv->rc_keyfile,
	                        group_name,
	                        key,
	                        sbool);
}

/**
 * rena_preferences_get_double_list:
 *
 */
gdouble *
rena_preferences_get_double_list (RenaPreferences *preferences,
                                    const gchar *group_name,
                                    const gchar *key)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), NULL);

	return g_key_file_get_double_list(preferences->priv->rc_keyfile,
	                                  group_name,
	                                  key,
	                                  NULL,
	                                  NULL);
}

/**
 * rena_preferences_set_double_list
 *
 */
void
rena_preferences_set_double_list (RenaPreferences *preferences,
                                    const gchar *group_name,
                                    const gchar *key,
                                    gdouble list[],
                                    gsize length)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	g_key_file_set_double_list(preferences->priv->rc_keyfile,
	                           group_name,
	                           key,
	                           list,
	                           length);
}

/**
 * rena_preferences_get_integer_list:
 *
 */
gint *
rena_preferences_get_integer_list (RenaPreferences *preferences,
                                     const gchar *group_name,
                                     const gchar *key,
                                     gsize *length)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), NULL);

	return g_key_file_get_integer_list(preferences->priv->rc_keyfile,
	                                   group_name,
	                                   key,
	                                   length,
	                                   NULL);
}

/**
 * rena_preferences_set_integer_list
 *
 */
void
rena_preferences_set_integer_list (RenaPreferences *preferences,
                                     const gchar *group_name,
                                     const gchar *key,
                                     gint list[],
                                     gsize length)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	g_key_file_set_integer_list(preferences->priv->rc_keyfile,
	                            group_name,
	                            key,
	                            list,
	                            length);
}

/**
 * rena_preferences_get_integer:
 *
 */
gint
rena_preferences_get_integer (RenaPreferences *preferences,
                                const gchar       *group_name,
                                const gchar       *key)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), 0);

	return g_key_file_get_integer (preferences->priv->rc_keyfile,
	                               group_name,
	                               key,
	                               NULL);
}

/**
 * rena_preferences_set_integer:
 *
 */
void
rena_preferences_set_integer (RenaPreferences *preferences,
                                const gchar       *group_name,
                                const gchar       *key,
                                gint               integer)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	g_key_file_set_integer (preferences->priv->rc_keyfile,
	                        group_name,
	                        key,
	                        integer);
}

/**
 * rena_preferences_get_string:
 *
 */
gchar *
rena_preferences_get_string (RenaPreferences *preferences,
                               const gchar *group_name,
                               const gchar *key)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), NULL);

	return g_key_file_get_string(preferences->priv->rc_keyfile,
	                             group_name,
	                             key,
	                             NULL);
}

/**
 * rena_preferences_set_string:
 *
 */
void
rena_preferences_set_string (RenaPreferences *preferences,
                               const gchar *group_name,
                               const gchar *key,
                               const gchar *string)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	g_key_file_set_string(preferences->priv->rc_keyfile,
	                      group_name,
	                      key,
	                      string);
}

/**
 * rena_preferences_get_string_list:
 *
 */
gchar **
rena_preferences_get_string_list (RenaPreferences *preferences,
                                    const gchar *group_name,
                                    const gchar *key,
                                    gsize *length)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), NULL);

	return g_key_file_get_string_list(preferences->priv->rc_keyfile,
	                                  group_name,
	                                  key,
	                                  length,
	                                  NULL);
}

/**
 * rena_preferences_set_string_list
 *
 */
void
rena_preferences_set_string_list (RenaPreferences *preferences,
                                     const gchar *group_name,
                                     const gchar *key,
                                     const gchar * const list[],
                                     gsize length)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	g_key_file_set_string_list(preferences->priv->rc_keyfile,
	                           group_name,
	                           key,
	                           list,
	                           length);
}

/**
 * rena_preferences_get_filename_list:
 *
 */
GSList *
rena_preferences_get_filename_list (RenaPreferences *preferences,
                                      const gchar *group_name,
                                      const gchar *key)
{
	gchar **clist;
	GSList *slist = NULL;
	gchar *filename = NULL;
	gsize i, length;
	GError *error = NULL;

	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), NULL);


	clist = g_key_file_get_string_list(preferences->priv->rc_keyfile,
	                                   group_name,
	                                   key,
	                                   &length,
	                                   NULL);

	if (clist) {
		for (i = 0 ; i < length ; i++) {
			filename = g_filename_from_utf8(clist[i], -1, NULL, NULL, &error);
			if (!filename) {
				g_warning("Unable to get filename from UTF-8 string: %s", clist[i]);
				error = NULL;
				continue;
			}
			else {
				slist = g_slist_append(slist, filename);
			}
		}
		g_strfreev(clist);
	}

	return slist;
}

/**
 * rena_preferences_set_filename_list:
 *
 */
void
rena_preferences_set_filename_list (RenaPreferences *preferences,
                                      const gchar *group_name,
                                      const gchar *key,
                                      GSList *list)
{
	gchar **clist;
	gchar *filename = NULL;
	gsize cnt = 0, i;
	GError *error = NULL;

	cnt = g_slist_length(list);
	clist = g_new0(gchar *, cnt);

	for (i = 0 ; i < cnt ; i++) {
		filename = g_filename_to_utf8(list->data, -1, NULL, NULL, &error);
		if (!filename) {
			g_warning("Unable to convert file to UTF-8: %s", (gchar *)list->data);
			g_error_free(error);
			error = NULL;
			list = list->next;
			continue;
		}
		clist[i] = filename;
		list = list->next;
	}

	g_key_file_set_string_list(preferences->priv->rc_keyfile,
	                           group_name,
	                           key,
	                           (const gchar **)clist,
	                           cnt);

	for(i = 0; i < cnt; i++) {
		g_free(clist[i]);
	}

	g_free(clist);
}

/**
 * rena_preferences_remove_key:
 *
 */
void
rena_preferences_remove_key (RenaPreferences *preferences,
                               const gchar *group_name,
                               const gchar *key)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	if (g_key_file_has_group(preferences->priv->rc_keyfile, group_name) &&
	    g_key_file_has_key(preferences->priv->rc_keyfile,
	                       group_name,
	                       key,
	                       NULL))
		g_key_file_remove_key(preferences->priv->rc_keyfile,
		                      group_name,
		                      key,
		                      NULL);
}

/**
 * rena_preferences_has_group:
 *
 */
gboolean
rena_preferences_has_group (RenaPreferences *preferences,
                              const gchar       *group_name)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), FALSE);

	return g_key_file_has_group(preferences->priv->rc_keyfile, group_name);
}


/**
 * rena_preferences_remove_group:
 *
 */
void
rena_preferences_remove_group (RenaPreferences *preferences,
                                 const gchar       *group_name)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	g_key_file_remove_group (preferences->priv->rc_keyfile,
	                         group_name,
	                         NULL);
}

/**
 * rena_preferences_get_plugin_group_name:
 *
 */
void
rena_preferences_plugin_changed (RenaPreferences *preferences,
                                   const gchar       *key)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	g_signal_emit (preferences, signals[SIGNAL_PLUGINS_CHANGED], 0, key);
}

/**
 * rena_preferences_get_plugin_group_name:
 *
 */
gchar *
rena_preferences_get_plugin_group_name (RenaPreferences *preferences,
                                          const gchar       *plugin_name)
{
	gchar *group_name = NULL, *name_upper = NULL;
	
	name_upper = g_ascii_strup (plugin_name, -1);
	group_name = g_strdup_printf("PLUGIN_%s",  name_upper);
	g_free (name_upper);

	return group_name;
}

/**
 * rena_preferences_get_installed_version:
 *
 */
void
rena_preferences_need_restart (RenaPreferences *preferences)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	g_signal_emit (preferences, signals[SIGNAL_NEED_RESTART], 0);
}

/**
 * rena_preferences_local_provider_changed:
 *
 */
void
rena_preferences_local_provider_changed (RenaPreferences *preferences)
{
	g_signal_emit (preferences, signals[SIGNAL_LIBRARY_CHANGED], 0);
}

/**
 * rena_preferences_get_installed_version:
 *
 */
const gchar *
rena_preferences_get_installed_version (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), NULL);

	return preferences->priv->installed_version;
}

/**
 * rena_preferences_set_installed_version:
 *
 */
static void
rena_preferences_set_installed_version (RenaPreferences *preferences,
                                          const gchar *installed_version)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	g_free(preferences->priv->installed_version);
	preferences->priv->installed_version = g_strdup(installed_version);
}

/**
 * rena_preferences_get_instant_search:
 *
 */
gboolean
rena_preferences_get_instant_search (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), FALSE);

	return preferences->priv->instant_search;
}

/**
 * rena_preferences_set_instant_search:
 *
 */
void
rena_preferences_set_instant_search (RenaPreferences *preferences,
                                       gboolean instant_search)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	preferences->priv->instant_search = instant_search;

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_INSTANT_SEARCH]);
}


/**
 * rena_preferences_get_approximate_search:
 *
 */
gboolean
rena_preferences_get_approximate_search (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), FALSE);

	return preferences->priv->approximate_search;
}

/**
 * rena_preferences_set_approximate_search:
 *
 */
void
rena_preferences_set_approximate_search (RenaPreferences *preferences,
                                           gboolean approximate_search)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	preferences->priv->approximate_search = approximate_search;

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_APPROXIMATE_SEARCH]);
}

/**
 * rena_preferences_get_library_style:
 *
 */
gint
rena_preferences_get_library_style (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), 0);

	return preferences->priv->library_style;
}

/**
 * rena_preferences_set_library_style:
 *
 */
void
rena_preferences_set_library_style (RenaPreferences *preferences,
                                      gint library_style)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	preferences->priv->library_style = library_style;

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_LIBRARY_STYLE]);
}

/**
 * rena_preferences_get_sort_by_year:
 *
 */
gboolean
rena_preferences_get_sort_by_year (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), FALSE);

	return preferences->priv->sort_by_year;
}

/**
 * rena_preferences_sort_by_year:
 *
 */
void
rena_preferences_set_sort_by_year (RenaPreferences *preferences,
                                     gboolean sort_by_year)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	preferences->priv->sort_by_year = sort_by_year;

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_LIBRARY_SORT_BY_YEAR]);
}

/**
 * rena_preferences_get_shuffle:
 *
 */
gboolean
rena_preferences_get_shuffle (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), FALSE);

	return preferences->priv->shuffle;
}

/**
 * rena_preferences_set_shuffle:
 *
 */
void
rena_preferences_set_shuffle (RenaPreferences *preferences,
                                gboolean shuffle)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	preferences->priv->shuffle = shuffle;

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_SHUFFLE]);
}

/**
 * rena_preferences_get_repeat:
 *
 */
gboolean
rena_preferences_get_repeat (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), FALSE);

	return preferences->priv->repeat;
}

/**
 * rena_preferences_set_repeat:
 *
 */
void
rena_preferences_set_repeat (RenaPreferences *preferences,
                               gboolean repeat)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	preferences->priv->repeat = repeat;

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_REPEAT]);
}

/**
 * rena_preferences_get_restore_playlist:
 *
 */
gboolean
rena_preferences_get_restore_playlist (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), FALSE);

	return preferences->priv->restore_playlist;
}

/**
 * rena_preferences_set_restore_playlist:
 *
 */
void
rena_preferences_set_restore_playlist (RenaPreferences *preferences,
                                         gboolean restore_playlist)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	preferences->priv->restore_playlist = restore_playlist;

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_RESTORE_PLAYLIST]);
}

/**
 * rena_preferences_get_audio_sink:
 *
 */
const gchar *
rena_preferences_get_audio_sink (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), NULL);

	return preferences->priv->audio_sink;
}

/**
 * rena_preferences_set_audio_sink:
 *
 */
void
rena_preferences_set_audio_sink (RenaPreferences *preferences,
                                   const gchar *audio_sink)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	g_free(preferences->priv->audio_sink);
	preferences->priv->audio_sink = g_strdup(audio_sink);

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_AUDIO_SINK]);
}

/**
 * rena_preferences_get_audio_device:
 *
 */
const gchar *
rena_preferences_get_audio_device (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), NULL);

	return preferences->priv->audio_device;
}

/**
 * rena_preferences_set_audio_device:
 *
 */
void
rena_preferences_set_audio_device (RenaPreferences *preferences,
                                     const gchar *audio_device)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	g_free(preferences->priv->audio_device);
	preferences->priv->audio_device = g_strdup(audio_device);

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_AUDIO_DEVICE]);
}

/**
 * rena_preferences_get_software_mixer:
 *
 */
gboolean
rena_preferences_get_software_mixer (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), FALSE);

	return preferences->priv->software_mixer;
}

/**
 * rena_preferences_set_software_mixer:
 *
 */
void
rena_preferences_set_software_mixer (RenaPreferences *preferences,
                                       gboolean software_mixer)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	preferences->priv->software_mixer = software_mixer;

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_SOFTWARE_MIXER]);
}

/**
 * rena_preferences_get_software_volume:
 *
 */
gdouble
rena_preferences_get_software_volume (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), -1.0);

	return preferences->priv->software_volume;
}

/**
 * rena_preferences_set_software_volume:
 *
 */
void
rena_preferences_set_software_volume (RenaPreferences *preferences,
                                        gdouble software_volume)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	preferences->priv->software_volume = software_volume;

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_SOFTWARE_VOLUME]);
}


/**
 * rena_preferences_get_ignore_errors:
 *
 */
gboolean
rena_preferences_get_ignore_errors (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), FALSE);

	return preferences->priv->ignore_errors;
}

/**
 * rena_preferences_set_ignore_errors:
 *
 */
void
rena_preferences_set_ignore_errors (RenaPreferences *preferences,
                                      gboolean           ignore_errors)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	preferences->priv->ignore_errors = ignore_errors;

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_IGNORE_ERRORS]);
}


/**
 * rena_preferences_get_lateral_panel:
 *
 */
gboolean
rena_preferences_get_lateral_panel (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), FALSE);

	return preferences->priv->lateral_panel;
}

/**
 * rena_preferences_set_lateral_panel:
 *
 */
void
rena_preferences_set_lateral_panel (RenaPreferences *preferences,
                                      gboolean lateral_panel)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	preferences->priv->lateral_panel = lateral_panel;

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_LATERAL_PANEL]);
}

/**
 * rena_preferences_get_sidebar_size:
 *
 */
gint
rena_preferences_get_sidebar_size (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), DEFAULT_SIDEBAR_SIZE);

	return preferences->priv->sidebar_size;
}

/**
 * rena_preferences_set_sidebar_size:
 *
 */
void
rena_preferences_set_sidebar_size (RenaPreferences *preferences,
                                     gint sidebar_size)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	preferences->priv->sidebar_size = sidebar_size;

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_SIDEBAR_SIZE]);
}

/**
 * rena_preferences_get_secondary_lateral_panel:
 *
 */
gboolean
rena_preferences_get_secondary_lateral_panel (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), FALSE);

	return preferences->priv->secondary_lateral_panel;
}

/**
 * rena_preferences_set_secondary_lateral_panel:
 *
 */
void
rena_preferences_set_secondary_lateral_panel (RenaPreferences *preferences,
                                                gboolean secondary_lateral_panel)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	preferences->priv->secondary_lateral_panel = secondary_lateral_panel;

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_SECONDARY_LATERAL_PANEL]);
}

/**
 * rena_preferences_get_secondary_sidebar_size:
 *
 */
gint
rena_preferences_get_secondary_sidebar_size (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), DEFAULT_SIDEBAR_SIZE);

	return preferences->priv->secondary_sidebar_size;
}

/**
 * rena_preferences_set_secondary_sidebar_size:
 *
 */
void
rena_preferences_set_secondary_sidebar_size (RenaPreferences *preferences,
                                               gint secondary_sidebar_size)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	preferences->priv->secondary_sidebar_size = secondary_sidebar_size;

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_SECONDARY_SIDEBAR_SIZE]);
}

/**
 * rena_preferences_get_show_album_art:
 *
 */
gboolean
rena_preferences_get_show_album_art (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), TRUE);

	return preferences->priv->show_album_art;
}

/**
 * rena_preferences_set_show_album_art:
 *
 */
void
rena_preferences_set_show_album_art (RenaPreferences *preferences,
                                       gboolean show_album_art)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	preferences->priv->show_album_art = show_album_art;

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_SHOW_ALBUM_ART]);
}

/**
 * rena_preferences_get_album_art_size:
 *
 */
gint
rena_preferences_get_album_art_size (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), DEFAULT_ALBUM_ART_SIZE);

	return preferences->priv->album_art_size;
}

/**
 * rena_preferences_set_album_art_size:
 *
 */
void
rena_preferences_set_album_art_size (RenaPreferences *preferences,
                                       gint album_art_size)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	preferences->priv->album_art_size = album_art_size;

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_ALBUM_ART_SIZE]);
}

/**
 * rena_preferences_get_album_art_pattern:
 *
 */
const gchar *
rena_preferences_get_album_art_pattern (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), NULL);

	return preferences->priv->album_art_pattern;
}

/**
 * rena_preferences_setalbum_art_pattern:
 *
 */
void
rena_preferences_set_album_art_pattern (RenaPreferences *preferences,
                                          const gchar *album_art_pattern)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	g_free(preferences->priv->album_art_pattern);
	preferences->priv->album_art_pattern = g_strdup(album_art_pattern);

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_ALBUM_ART_PATTERN]);
}

/**
 * rena_preferences_get_toolbar_size:
 *
 */
GtkIconSize
rena_preferences_get_toolbar_size (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), GTK_ICON_SIZE_LARGE_TOOLBAR);

	return preferences->priv->toolbar_size;
}

/**
 * rena_preferences_set_toolbar_size:
 *
 */
void
rena_preferences_set_toolbar_size (RenaPreferences *preferences,
                                     GtkIconSize        toolbar_size)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	preferences->priv->toolbar_size = toolbar_size;

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_TOOLBAR_SIZE]);
}

/**
 * rena_preferences_get_show_status_icon:
 *
 */
gboolean
rena_preferences_get_show_status_icon (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), FALSE);

	return preferences->priv->show_status_icon;
}

/**
 * rena_preferences_set_show_status_icon:
 *
 */
void
rena_preferences_set_show_status_icon (RenaPreferences *preferences,
                                         gboolean show_status_icon)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	preferences->priv->show_status_icon = show_status_icon;

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_SHOW_STATUS_ICON]);
}

/**
 * rena_preferences_get_show_menubar:
 *
 */
gboolean
rena_preferences_get_show_menubar (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), TRUE);

	return preferences->priv->show_menubar;
}

/**
 * rena_preferences_set_show_menubar:
 *
 */
void
rena_preferences_set_show_menubar (RenaPreferences *preferences,
                                     gboolean           show_menubar)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	preferences->priv->show_menubar = show_menubar;

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_SHOW_MENUBAR]);
}

/**
 * rena_preferences_get_system_titlebar:
 *
 */
gboolean
rena_preferences_get_system_titlebar (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), TRUE);

	return preferences->priv->system_titlebar;
}

/**
 * rena_preferences_set_system_titlebar:
 *
 */
void
rena_preferences_set_system_titlebar (RenaPreferences *preferences,
                                        gboolean           system_titlebar)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	preferences->priv->system_titlebar = system_titlebar;

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_SYSTEM_TITLEBAR]);
}

/**
 * rena_preferences_get_controls_below:
 *
 */
gboolean
rena_preferences_get_controls_below (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), TRUE);

	return preferences->priv->controls_below;
}

/**
 * rena_preferences_set_controls_below:
 *
 */
void
rena_preferences_set_controls_below (RenaPreferences *preferences,
                                       gboolean controls_below)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	preferences->priv->controls_below = controls_below;

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_CONTROLS_BELOW]);
}

/**
 * rena_preferences_get_remember_state:
 *
 */
gboolean
rena_preferences_get_remember_state (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), TRUE);

	return preferences->priv->remember_state;
}

/**
 * rena_preferences_set_remember_state:
 *
 */
void
rena_preferences_set_remember_state (RenaPreferences *preferences,
                                       gboolean remember_state)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	preferences->priv->remember_state = remember_state;

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_REMEMBER_STATE]);
}

/**
 * rena_preferences_get_start_mode:
 *
 */
const gchar *
rena_preferences_get_start_mode (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), NULL);

	return preferences->priv->start_mode;
}

/**
 * rena_preferences_set_start_mode:
 *
 */
void
rena_preferences_set_start_mode (RenaPreferences *preferences,
                                   const gchar *start_mode)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	g_free(preferences->priv->start_mode);
	preferences->priv->start_mode = g_strdup(start_mode);

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_START_MODE]);
}

/**
 * rena_preferences_get_last_folder:
 *
 */
const gchar *
rena_preferences_get_last_folder (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), NULL);

	return preferences->priv->last_folder;
}

/**
 * rena_preferences_set_last_folder:
 *
 */
void
rena_preferences_set_last_folder (RenaPreferences *preferences,
                                    const gchar *last_folder)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	g_free(preferences->priv->last_folder);
	preferences->priv->last_folder = g_strdup(last_folder);

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_LAST_FOLDER]);
}

/**
 * rena_preferences_get_add_recursively:
 *
 */
gboolean
rena_preferences_get_add_recursively (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), FALSE);

	return preferences->priv->add_recursively;
}

/**
 * rena_preferences_set_add_recursively:
 *
 */
void
rena_preferences_set_add_recursively(RenaPreferences *preferences,
                                       gboolean add_recursively)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	preferences->priv->add_recursively = add_recursively;

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_ADD_RECURSIVELY]);
}

/**
 * rena_preferences_get_timer_remaining_mode:
 *
 */
gboolean
rena_preferences_get_timer_remaining_mode (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), FALSE);

	return preferences->priv->timer_remaining_mode;
}

/**
 * rena_preferences_set_timer_remaining_mode:
 *
 */
void
rena_preferences_set_timer_remaining_mode(RenaPreferences *preferences,
                                            gboolean timer_remaining_mode)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	preferences->priv->timer_remaining_mode = timer_remaining_mode;

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_TIMER_REMAINING_MODE]);
}

/**
 * rena_preferences_get_hide_instead_close:
 *
 */
gboolean
rena_preferences_get_hide_instead_close (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), TRUE);

	return preferences->priv->hide_instead_close;
}

/**
 * rena_preferences_set_hide_instead_close:
 *
 */
void
rena_preferences_set_hide_instead_close (RenaPreferences *preferences,
                                           gboolean hide_instead_close)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	preferences->priv->hide_instead_close = hide_instead_close;

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_HIDE_INSTEAD_CLOSE]);
}

/**
 * rena_preferences_get_lock_library:
 *
 */
gboolean
rena_preferences_get_lock_library (RenaPreferences *preferences)
{
	g_return_val_if_fail(RENA_IS_PREFERENCES(preferences), TRUE);

	return preferences->priv->lock_library;
}

/**
 * rena_preferences_set_lock_library:
 *
 */
void
rena_preferences_set_lock_library (RenaPreferences *preferences,
                                     gboolean           lock_library)
{
	g_return_if_fail(RENA_IS_PREFERENCES(preferences));

	preferences->priv->lock_library = lock_library;

	g_object_notify_by_pspec(G_OBJECT(preferences), gParamSpecs[PROP_LOCK_LIBRARY]);
}

static void
rena_preferences_load_from_file(RenaPreferences *preferences)
{
	gchar *installed_version;
	gboolean approximate_search, instant_search;
	gboolean shuffle, repeat, restore_playlist, software_mixer, ignore_errors;
	gboolean lateral_panel, secondary_lateral_panel, show_album_art, \
		show_status_icon, show_menubar, system_titlebar, controls_below, remember_state;
	gchar *album_art_pattern;
	gchar *start_mode, *last_folder, *last_folder_converted = NULL;
	gboolean add_recursively, timer_remaining_mode, hide_instead_close;
	gchar *audio_sink, *audio_device;
	gdouble software_volume;
	gint library_style, sidebar_size, secondary_sidebar_size, album_art_size;
	GtkIconSize toolbar_size;
	gboolean sort_by_year;
	const gchar *user_config_dir;
	gchar *rena_config_dir = NULL;
	GError *error = NULL;

	RenaPreferencesPrivate *priv = preferences->priv;

	/* First check preferences folder or create it */

	user_config_dir = g_get_user_config_dir();
	rena_config_dir = g_build_path(G_DIR_SEPARATOR_S, user_config_dir, "/rena", NULL);

	if (g_file_test(rena_config_dir, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR) == FALSE) {
		if (g_mkdir_with_parents (rena_config_dir, S_IRWXU) == -1) {
			g_free(rena_config_dir);
			g_critical("Unable to create preferences directory, err: %s", g_strerror(errno));
			return;
		}
		CDEBUG(DBG_INFO, "Created .config/rena folder");
	}
	g_free(rena_config_dir);

	/* Open the preferences storage file */

	priv->rc_keyfile = g_key_file_new();
	priv->rc_filepath = g_build_path(G_DIR_SEPARATOR_S, user_config_dir, "/rena/config", NULL);

	/* Does /rena/config exist ? */

	if (g_file_test(priv->rc_filepath, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR) == FALSE) {
		CDEBUG(DBG_INFO, "First init of rena. Use default settings.");
		return;
	}

	/* Open the preferences key file */

	if (!g_key_file_load_from_file(priv->rc_keyfile,
	                               priv->rc_filepath,
	                               G_KEY_FILE_NONE,
	                               &error)) {
		g_critical("Unable to load config file (Possible first start), err: %s", error->message);
		g_error_free(error);
		return;
	}

	/* Installed version used as flag to detect the first run. */

	installed_version = g_key_file_get_string(priv->rc_keyfile,
	                                          GROUP_GENERAL,
	                                          KEY_INSTALLED_VERSION,
	                                          &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_installed_version(preferences, installed_version);
	}


	/* Open last preferences */

	approximate_search = g_key_file_get_boolean(priv->rc_keyfile,
	                                            GROUP_GENERAL,
	                                            KEY_APPROXIMATE_SEARCH,
	                                            &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_approximate_search(preferences, approximate_search);
	}

	instant_search = g_key_file_get_boolean(priv->rc_keyfile,
	                                        GROUP_GENERAL,
	                                        KEY_INSTANT_SEARCH,
	                                        &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_instant_search(preferences, instant_search);
	}

	shuffle = g_key_file_get_boolean(priv->rc_keyfile,
	                                 GROUP_PLAYLIST,
	                                 KEY_SHUFFLE,
	                                 &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_shuffle(preferences, shuffle);
	}

	repeat = g_key_file_get_boolean(priv->rc_keyfile,
	                                GROUP_PLAYLIST,
	                                KEY_REPEAT,
	                                &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_repeat(preferences, repeat);
	}

	library_style = g_key_file_get_integer(priv->rc_keyfile,
	                                       GROUP_LIBRARY,
	                                       KEY_LIBRARY_VIEW_ORDER,
	                                       &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_library_style(preferences, library_style);
	}

	sort_by_year = g_key_file_get_boolean(priv->rc_keyfile,
	                                      GROUP_LIBRARY,
	                                      KEY_SORT_BY_YEAR,
	                                      &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_sort_by_year(preferences, sort_by_year);
	}

	restore_playlist = g_key_file_get_boolean(priv->rc_keyfile,
	                                          GROUP_PLAYLIST,
	                                          KEY_SAVE_PLAYLIST,
	                                          &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_restore_playlist(preferences, restore_playlist);
	}

	audio_sink = g_key_file_get_string(priv->rc_keyfile,
	                                   GROUP_AUDIO,
	                                   KEY_AUDIO_SINK,
	                                   &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_audio_sink(preferences, audio_sink);
	}

	audio_device = g_key_file_get_string(priv->rc_keyfile,
	                                     GROUP_AUDIO,
	                                     KEY_AUDIO_DEVICE,
	                                     &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_audio_device(preferences, audio_device);
	}

	software_mixer = g_key_file_get_boolean(priv->rc_keyfile,
	                                        GROUP_AUDIO,
	                                        KEY_SOFTWARE_MIXER,
	                                        &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_software_mixer(preferences, software_mixer);
	}

	software_volume = g_key_file_get_double(priv->rc_keyfile,
	                                        GROUP_AUDIO,
	                                        KEY_SOFTWARE_VOLUME,
	                                        &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_software_volume(preferences, software_volume);
	}

	ignore_errors = g_key_file_get_boolean(priv->rc_keyfile,
	                                       GROUP_AUDIO,
	                                       KEY_IGNORE_ERRORS,
	                                       &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_ignore_errors(preferences, ignore_errors);
	}

	lateral_panel = g_key_file_get_boolean(priv->rc_keyfile,
	                                       GROUP_WINDOW,
	                                       KEY_SIDEBAR,
	                                       &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_lateral_panel(preferences, lateral_panel);
	}

	sidebar_size = g_key_file_get_integer(priv->rc_keyfile,
	                                      GROUP_WINDOW,
	                                      KEY_SIDEBAR_SIZE,
	                                      &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_sidebar_size(preferences, sidebar_size);
	}

	secondary_lateral_panel = g_key_file_get_boolean(priv->rc_keyfile,
	                                                 GROUP_WINDOW,
	                                                 KEY_SECONDARY_SIDEBAR,
	                                                 &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_secondary_lateral_panel(preferences, secondary_lateral_panel);
	}

	secondary_sidebar_size = g_key_file_get_integer(priv->rc_keyfile,
	                                                GROUP_WINDOW,
	                                                KEY_SECONDARY_SIDEBAR_SIZE,
	                                                &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_secondary_sidebar_size(preferences, secondary_sidebar_size);
	}

	show_album_art = g_key_file_get_boolean(priv->rc_keyfile,
	                                        GROUP_WINDOW,
	                                        KEY_SHOW_ALBUM_ART,
	                                        &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_show_album_art(preferences, show_album_art);
	}

	album_art_size = g_key_file_get_integer(priv->rc_keyfile,
	                                        GROUP_WINDOW,
	                                        KEY_ALBUM_ART_SIZE,
	                                        &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_album_art_size(preferences, album_art_size);
	}

	album_art_pattern = g_key_file_get_string(priv->rc_keyfile,
	                                         GROUP_GENERAL,
	                                         KEY_ALBUM_ART_PATTERN,
	                                         &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_album_art_pattern(preferences, album_art_pattern);
	}

	toolbar_size = g_key_file_get_integer(priv->rc_keyfile,
	                                      GROUP_WINDOW,
	                                      KEY_TOOLBAR_SIZE,
	                                      &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_toolbar_size(preferences, toolbar_size);
	}

	show_status_icon = g_key_file_get_boolean(priv->rc_keyfile,
	                                          GROUP_GENERAL,
	                                          KEY_SHOW_ICON_TRAY,
	                                          &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_show_status_icon(preferences, show_status_icon);
	}

	show_menubar = g_key_file_get_boolean(priv->rc_keyfile,
	                                      GROUP_WINDOW,
	                                      KEY_SHOW_MENUBAR,
	                                      &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_show_menubar(preferences, show_menubar);
	}

	system_titlebar = g_key_file_get_boolean(priv->rc_keyfile,
	                                         GROUP_WINDOW,
	                                         KEY_SYSTEM_TITLEBAR,
	                                         &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_system_titlebar(preferences, system_titlebar);
	}

	controls_below = g_key_file_get_boolean(priv->rc_keyfile,
	                                        GROUP_WINDOW,
	                                        KEY_CONTROLS_BELOW,
	                                        &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_controls_below(preferences, controls_below);
	}

	remember_state = g_key_file_get_boolean(priv->rc_keyfile,
	                                        GROUP_WINDOW,
	                                        KEY_REMEMBER_STATE,
	                                        &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_remember_state(preferences, remember_state);
	}

	start_mode = g_key_file_get_string(priv->rc_keyfile,
	                                   GROUP_WINDOW,
	                                   KEY_START_MODE,
	                                   &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_start_mode(preferences, NORMAL_STATE);
	}

	last_folder = g_key_file_get_string(priv->rc_keyfile,
	                                    GROUP_GENERAL,
	                                    KEY_LAST_FOLDER,
	                                    &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		last_folder_converted = g_filename_from_utf8(last_folder, -1, NULL, NULL, &error);
		if (error) {
			g_warning("Unable to get filename from UTF-8 string: %s", error->message);
			g_error_free(error);
			error = NULL;
		}
		else {
			rena_preferences_set_last_folder(preferences, last_folder_converted);
		}
	}

	add_recursively = g_key_file_get_boolean(priv->rc_keyfile,
	                                         GROUP_GENERAL,
	                                         KEY_ADD_RECURSIVELY_FILES,
	                                         &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_add_recursively(preferences, add_recursively);
	}

	timer_remaining_mode = g_key_file_get_boolean(priv->rc_keyfile,
	                                              GROUP_GENERAL,
	                                              KEY_TIMER_REMAINING_MODE,
	                                              &error);
	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_timer_remaining_mode(preferences, timer_remaining_mode);
	}

	hide_instead_close = g_key_file_get_boolean(priv->rc_keyfile,
	                                            GROUP_GENERAL,
	                                            KEY_CLOSE_TO_TRAY,
	                                            &error);

	if (error) {
		g_error_free(error);
		error = NULL;
	}
	else {
		rena_preferences_set_hide_instead_close(preferences, hide_instead_close);
	}

	g_free(installed_version);
	g_free(audio_sink);
	g_free(audio_device);
	g_free(album_art_pattern);
	g_free(start_mode);
	g_free(last_folder);
	g_free(last_folder_converted);
}

static void
rena_preferences_finalize (GObject *object)
{
	gchar *data = NULL;
	gsize length;
	GError *error = NULL;

	RenaPreferences *preferences = RENA_PREFERENCES(object);
	RenaPreferencesPrivate *priv = preferences->priv;

	/* Store new preferences */

	g_key_file_set_string(priv->rc_keyfile,
	                      GROUP_GENERAL,
	                      KEY_INSTALLED_VERSION,
	                      PACKAGE_VERSION);

	g_key_file_set_boolean(priv->rc_keyfile,
	                       GROUP_GENERAL,
	                       KEY_INSTANT_SEARCH,
	                       priv->instant_search);
	g_key_file_set_boolean(priv->rc_keyfile,
	                       GROUP_GENERAL,
	                       KEY_APPROXIMATE_SEARCH,
	                       priv->approximate_search);

	g_key_file_set_integer(priv->rc_keyfile,
	                       GROUP_LIBRARY,
	                       KEY_LIBRARY_VIEW_ORDER,
	                       priv->library_style);
	g_key_file_set_boolean(priv->rc_keyfile,
	                       GROUP_LIBRARY,
	                       KEY_SORT_BY_YEAR,
	                       priv->sort_by_year);

	g_key_file_set_boolean(priv->rc_keyfile,
	                       GROUP_PLAYLIST,
	                       KEY_SHUFFLE,
	                       priv->shuffle);
	g_key_file_set_boolean(priv->rc_keyfile,
	                       GROUP_PLAYLIST,
	                       KEY_REPEAT,
	                       priv->repeat);
	g_key_file_set_boolean(priv->rc_keyfile,
	                       GROUP_PLAYLIST,
	                       KEY_SAVE_PLAYLIST,
	                       priv->restore_playlist);
	g_key_file_set_string(priv->rc_keyfile,
	                      GROUP_AUDIO,
	                      KEY_AUDIO_SINK,
	                      priv->audio_sink);
	g_key_file_set_string(priv->rc_keyfile,
	                      GROUP_AUDIO,
	                      KEY_AUDIO_DEVICE,
	                      priv->audio_device);
	g_key_file_set_boolean(priv->rc_keyfile,
	                       GROUP_AUDIO,
	                       KEY_SOFTWARE_MIXER,
	                       priv->software_mixer);
	g_key_file_set_double(priv->rc_keyfile,
	                      GROUP_AUDIO,
	                      KEY_SOFTWARE_VOLUME,
	                      priv->software_volume);
	g_key_file_set_boolean(priv->rc_keyfile,
	                       GROUP_AUDIO,
	                       KEY_IGNORE_ERRORS,
	                       priv->ignore_errors);

	g_key_file_set_boolean(priv->rc_keyfile,
	                       GROUP_WINDOW,
	                       KEY_SIDEBAR,
	                       priv->lateral_panel);
	g_key_file_set_integer(priv->rc_keyfile,
	                       GROUP_WINDOW,
	                       KEY_SIDEBAR_SIZE,
	                       priv->sidebar_size);
	g_key_file_set_boolean(priv->rc_keyfile,
	                       GROUP_WINDOW,
	                       KEY_SECONDARY_SIDEBAR,
	                       priv->secondary_lateral_panel);
	g_key_file_set_integer(priv->rc_keyfile,
	                       GROUP_WINDOW,
	                       KEY_SECONDARY_SIDEBAR_SIZE,
	                       priv->secondary_sidebar_size);
	g_key_file_set_boolean(priv->rc_keyfile,
	                       GROUP_WINDOW,
	                       KEY_SHOW_ALBUM_ART,
	                       priv->show_album_art);
	g_key_file_set_integer(priv->rc_keyfile,
	                       GROUP_WINDOW,
	                       KEY_ALBUM_ART_SIZE,
	                       priv->album_art_size);
	if (string_is_not_empty(priv->album_art_pattern))
		g_key_file_set_string(priv->rc_keyfile,
		                      GROUP_GENERAL,
		                      KEY_ALBUM_ART_PATTERN,
		                      priv->album_art_pattern);
	else
		rena_preferences_remove_key(preferences,
		                              GROUP_GENERAL,
		                              KEY_ALBUM_ART_PATTERN);
	g_key_file_set_integer(priv->rc_keyfile,
	                       GROUP_WINDOW,
	                       KEY_TOOLBAR_SIZE,
	                       priv->toolbar_size);
	g_key_file_set_boolean(priv->rc_keyfile,
	                       GROUP_GENERAL,
	                       KEY_SHOW_ICON_TRAY,
	                       priv->show_status_icon);
	g_key_file_set_boolean(priv->rc_keyfile,
	                       GROUP_WINDOW,
	                       KEY_SHOW_MENUBAR,
	                       priv->show_menubar);
	g_key_file_set_boolean(priv->rc_keyfile,
	                       GROUP_WINDOW,
	                       KEY_SYSTEM_TITLEBAR,
	                       priv->system_titlebar);
	g_key_file_set_boolean(priv->rc_keyfile,
	                       GROUP_WINDOW,
	                       KEY_CONTROLS_BELOW,
	                       priv->controls_below);
	g_key_file_set_boolean(priv->rc_keyfile,
	                       GROUP_WINDOW,
	                       KEY_REMEMBER_STATE,
	                       priv->remember_state);
	g_key_file_set_string(priv->rc_keyfile,
	                      GROUP_WINDOW,
	                      KEY_START_MODE,
	                      priv->start_mode);

	gchar *last_folder_converted = g_filename_to_utf8(priv->last_folder, -1, NULL, NULL, &error);
	if (error) {
		g_warning("Unable to convert filename to UTF-8: %s", error->message);
		g_error_free(error);
		error = NULL;
	}
	if (string_is_not_empty(last_folder_converted))
		g_key_file_set_string(priv->rc_keyfile,
		                      GROUP_GENERAL,
		                      KEY_LAST_FOLDER,
		                      last_folder_converted);
	else
		rena_preferences_remove_key(preferences,
		                              GROUP_GENERAL,
		                              KEY_LAST_FOLDER);
	g_free(last_folder_converted);

	g_key_file_set_boolean(priv->rc_keyfile,
	                       GROUP_GENERAL,
	                       KEY_ADD_RECURSIVELY_FILES,
	                       priv->add_recursively);
	g_key_file_set_boolean(priv->rc_keyfile,
	                       GROUP_GENERAL,
	                       KEY_TIMER_REMAINING_MODE,
	                       priv->timer_remaining_mode);

	g_key_file_set_boolean(priv->rc_keyfile,
	                       GROUP_GENERAL,
	                       KEY_CLOSE_TO_TRAY,
	                       priv->hide_instead_close);

	/* Save to key file */

	data = g_key_file_to_data(priv->rc_keyfile, &length, NULL);
	if(!g_file_set_contents(priv->rc_filepath, data, length, &error))
		g_critical("Unable to write preferences file : %s", error->message);

	g_free(data);
	g_key_file_free(priv->rc_keyfile);
	g_free(priv->installed_version);
	g_free(priv->rc_filepath);
	g_free(priv->audio_sink);
	g_free(priv->audio_device);
	g_free(priv->album_art_pattern);
	g_free(priv->start_mode);
	g_free(priv->last_folder);

	G_OBJECT_CLASS(rena_preferences_parent_class)->finalize(object);
}

static void
rena_preferences_get_property (GObject *object,
                                 guint prop_id,
                                 GValue *value,
                                 GParamSpec *pspec)
{
	RenaPreferences *preferences = RENA_PREFERENCES(object);

	switch (prop_id) {
		case PROP_INSTANT_SEARCH:
			g_value_set_boolean (value, rena_preferences_get_instant_search(preferences));
			break;
		case PROP_APPROXIMATE_SEARCH:
			g_value_set_boolean (value, rena_preferences_get_approximate_search(preferences));
			break;
		case PROP_LIBRARY_STYLE:
			g_value_set_int (value, rena_preferences_get_library_style(preferences));
			break;
		case PROP_LIBRARY_SORT_BY_YEAR:
			g_value_set_boolean (value, rena_preferences_get_sort_by_year(preferences));
			break;
		case PROP_SHUFFLE:
			g_value_set_boolean (value, rena_preferences_get_shuffle(preferences));
			break;
		case PROP_REPEAT:
			g_value_set_boolean (value, rena_preferences_get_repeat(preferences));
			break;
		case PROP_RESTORE_PLAYLIST:
			g_value_set_boolean (value, rena_preferences_get_restore_playlist(preferences));
			break;
		case PROP_AUDIO_SINK:
			g_value_set_string (value, rena_preferences_get_audio_sink(preferences));
			break;
		case PROP_AUDIO_DEVICE:
			g_value_set_string (value, rena_preferences_get_audio_device(preferences));
			break;
		case PROP_SOFTWARE_MIXER:
			g_value_set_boolean (value, rena_preferences_get_software_mixer(preferences));
			break;
		case PROP_SOFTWARE_VOLUME:
			g_value_set_double (value, rena_preferences_get_software_volume(preferences));
			break;
		case PROP_IGNORE_ERRORS:
			g_value_set_boolean (value, rena_preferences_get_ignore_errors(preferences));
			break;
		case PROP_LATERAL_PANEL:
			g_value_set_boolean (value, rena_preferences_get_lateral_panel(preferences));
			break;
		case PROP_SIDEBAR_SIZE:
			g_value_set_int (value, rena_preferences_get_sidebar_size(preferences));
			break;
		case PROP_SECONDARY_LATERAL_PANEL:
			g_value_set_boolean (value, rena_preferences_get_secondary_lateral_panel(preferences));
			break;
		case PROP_SECONDARY_SIDEBAR_SIZE:
			g_value_set_int (value, rena_preferences_get_secondary_sidebar_size(preferences));
			break;
		case PROP_SHOW_ALBUM_ART:
			g_value_set_boolean (value, rena_preferences_get_show_album_art(preferences));
			break;
		case PROP_ALBUM_ART_SIZE:
			g_value_set_int (value, rena_preferences_get_album_art_size(preferences));
			break;
		case PROP_ALBUM_ART_PATTERN:
			g_value_set_string (value, rena_preferences_get_album_art_pattern(preferences));
			break;
		case PROP_TOOLBAR_SIZE:
			g_value_set_enum (value, rena_preferences_get_toolbar_size(preferences));
			break;
		case PROP_SHOW_STATUS_ICON:
			g_value_set_boolean (value, rena_preferences_get_show_status_icon(preferences));
			break;
		case PROP_SHOW_MENUBAR:
			g_value_set_boolean (value, rena_preferences_get_show_menubar(preferences));
			break;
		case PROP_SYSTEM_TITLEBAR:
			g_value_set_boolean (value, rena_preferences_get_system_titlebar(preferences));
			break;
		case PROP_CONTROLS_BELOW:
			g_value_set_boolean (value, rena_preferences_get_controls_below(preferences));
			break;
		case PROP_REMEMBER_STATE:
			g_value_set_boolean (value, rena_preferences_get_remember_state(preferences));
			break;
		case PROP_START_MODE:
			g_value_set_string (value, rena_preferences_get_start_mode(preferences));
			break;
		case PROP_LAST_FOLDER:
			g_value_set_string (value, rena_preferences_get_last_folder(preferences));
			break;
		case PROP_ADD_RECURSIVELY:
			g_value_set_boolean (value, rena_preferences_get_add_recursively(preferences));
			break;
		case PROP_TIMER_REMAINING_MODE:
			g_value_set_boolean (value, rena_preferences_get_timer_remaining_mode(preferences));
			break;
		case PROP_HIDE_INSTEAD_CLOSE:
			g_value_set_boolean (value, rena_preferences_get_hide_instead_close(preferences));
			break;
		case PROP_LOCK_LIBRARY:
			g_value_set_boolean (value, rena_preferences_get_lock_library(preferences));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
rena_preferences_set_property (GObject *object,
                                 guint prop_id,
                                 const GValue *value,
                                 GParamSpec *pspec)
{
	RenaPreferences *preferences = RENA_PREFERENCES(object);

	switch (prop_id) {
		case PROP_INSTANT_SEARCH:
			rena_preferences_set_instant_search(preferences, g_value_get_boolean(value));
			break;
		case PROP_APPROXIMATE_SEARCH:
			rena_preferences_set_approximate_search(preferences, g_value_get_boolean(value));
			break;
		case PROP_LIBRARY_STYLE:
			rena_preferences_set_library_style(preferences, g_value_get_int(value));
			break;
		case PROP_LIBRARY_SORT_BY_YEAR:
			rena_preferences_set_sort_by_year(preferences, g_value_get_boolean(value));
			break;
		case PROP_SHUFFLE:
			rena_preferences_set_shuffle(preferences, g_value_get_boolean(value));
			break;
		case PROP_REPEAT:
			rena_preferences_set_repeat(preferences, g_value_get_boolean(value));
			break;
		case PROP_RESTORE_PLAYLIST:
			rena_preferences_set_restore_playlist(preferences, g_value_get_boolean(value));
			break;
		case PROP_AUDIO_SINK:
			rena_preferences_set_audio_sink(preferences, g_value_get_string(value));
			break;
		case PROP_AUDIO_DEVICE:
			rena_preferences_set_audio_device(preferences, g_value_get_string(value));
			break;
		case PROP_SOFTWARE_MIXER:
			rena_preferences_set_software_mixer(preferences, g_value_get_boolean(value));
			break;
		case PROP_SOFTWARE_VOLUME:
			rena_preferences_set_software_volume(preferences, g_value_get_double(value));
			break;
		case PROP_IGNORE_ERRORS:
			rena_preferences_set_ignore_errors(preferences, g_value_get_boolean(value));
			break;
		case PROP_LATERAL_PANEL:
			rena_preferences_set_lateral_panel(preferences, g_value_get_boolean(value));
			break;
		case PROP_SIDEBAR_SIZE:
			rena_preferences_set_sidebar_size(preferences, g_value_get_int(value));
			break;
		case PROP_SECONDARY_LATERAL_PANEL:
			rena_preferences_set_secondary_lateral_panel(preferences, g_value_get_boolean(value));
			break;
		case PROP_SECONDARY_SIDEBAR_SIZE:
			rena_preferences_set_secondary_sidebar_size(preferences, g_value_get_int(value));
			break;
		case PROP_SHOW_ALBUM_ART:
			rena_preferences_set_show_album_art(preferences, g_value_get_boolean(value));
			break;
		case PROP_ALBUM_ART_SIZE:
			rena_preferences_set_album_art_size(preferences, g_value_get_int(value));
			break;
		case PROP_ALBUM_ART_PATTERN:
			rena_preferences_set_album_art_pattern(preferences, g_value_get_string(value));
			break;
		case PROP_TOOLBAR_SIZE:
			rena_preferences_set_toolbar_size(preferences, g_value_get_enum(value));
			break;
		case PROP_SHOW_STATUS_ICON:
			rena_preferences_set_show_status_icon(preferences, g_value_get_boolean(value));
			break;
		case PROP_SHOW_MENUBAR:
			rena_preferences_set_show_menubar(preferences, g_value_get_boolean(value));
			break;
		case PROP_SYSTEM_TITLEBAR:
			rena_preferences_set_system_titlebar(preferences, g_value_get_boolean(value));
			break;
		case PROP_CONTROLS_BELOW:
			rena_preferences_set_controls_below(preferences, g_value_get_boolean(value));
			break;
		case PROP_REMEMBER_STATE:
			rena_preferences_set_remember_state(preferences, g_value_get_boolean(value));
			break;
		case PROP_START_MODE:
			rena_preferences_set_start_mode(preferences, g_value_get_string(value));
			break;
		case PROP_LAST_FOLDER:
			rena_preferences_set_last_folder(preferences, g_value_get_string(value));
			break;
		case PROP_ADD_RECURSIVELY:
			rena_preferences_set_add_recursively(preferences, g_value_get_boolean(value));
			break;
		case PROP_TIMER_REMAINING_MODE:
			rena_preferences_set_timer_remaining_mode(preferences, g_value_get_boolean(value));
			break;
		case PROP_HIDE_INSTEAD_CLOSE:
			rena_preferences_set_hide_instead_close(preferences, g_value_get_boolean(value));
			break;
		case PROP_LOCK_LIBRARY:
			rena_preferences_set_lock_library(preferences, g_value_get_boolean(value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static GObject *
rena_preferences_constructor (GType type, guint n_construct_properties, GObjectConstructParam *construct_properties)
{
	GObject *object = G_OBJECT_CLASS (rena_preferences_parent_class)->
		constructor (type, n_construct_properties, construct_properties);

	RenaPreferences *preferences = RENA_PREFERENCES (object);
	rena_preferences_load_from_file (preferences);

	return object;
}

static void
rena_preferences_class_init (RenaPreferencesClass *klass)
{
	GObjectClass *object_class;
	const GParamFlags RENA_PREF_PARAMS = G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS;

	object_class = G_OBJECT_CLASS(klass);
	object_class->constructor = rena_preferences_constructor;
	object_class->finalize = rena_preferences_finalize;
	object_class->get_property = rena_preferences_get_property;
	object_class->set_property = rena_preferences_set_property;

	/**
	  * RenaPreferences:instant_search:
	  *
	  */
	gParamSpecs[PROP_INSTANT_SEARCH] =
		g_param_spec_boolean("instant-search",
		                     "InstantSearch",
		                     "Instant Search Preference",
		                      TRUE,
		                      RENA_PREF_PARAMS);

	/**
	  * RenaPreferences:approximate_searches:
	  *
	  */
	gParamSpecs[PROP_APPROXIMATE_SEARCH] =
		g_param_spec_boolean("approximate-searches",
		                     "ApproximateSearches",
		                     "Approximate Searches Preference",
		                      FALSE,
		                      RENA_PREF_PARAMS);

	/**
	  * RenaPreferences:library_style:
	  *
	  */
	gParamSpecs[PROP_LIBRARY_STYLE] =
		g_param_spec_int ("library-style",
		                  "LibraryStyle",
		                  "Library Style Preferences",
		                  0,
		                  LAST_LIBRARY_STYLE,
		                  FOLDERS,
		                  RENA_PREF_PARAMS);

	/**
	 * RenaPreferences:sort_by_year:
	 *
	 */
	gParamSpecs[PROP_LIBRARY_SORT_BY_YEAR] =
		g_param_spec_boolean("sort-by-year",
		                     "SortByYear",
		                     "Sort By Year Preference",
		                     TRUE,
		                     RENA_PREF_PARAMS);

	/**
	  * RenaPreferences:shuffle:
	  *
	  */
	gParamSpecs[PROP_SHUFFLE] =
		g_param_spec_boolean("shuffle",
		                     "Shuffle",
		                     "Shuffle Preference",
		                     FALSE,
		                     RENA_PREF_PARAMS);

	/**
	  * RenaPreferences:repeat:
	  *
	  */
	gParamSpecs[PROP_REPEAT] =
		g_param_spec_boolean("repeat",
		                     "Repeat",
		                     "Repeat Preference",
		                     FALSE,
		                     RENA_PREF_PARAMS);

	/**
	  * RenaPreferences:restore_playlist:
	  *
	  */
	gParamSpecs[PROP_RESTORE_PLAYLIST] =
		g_param_spec_boolean("restore-playlist",
		                     "RestorePlaylist",
		                     "Restore Playlist Preference",
		                     TRUE,
		                     RENA_PREF_PARAMS);

	/**
	  * RenaPreferences:audio_sink:
	  *
	  */
	gParamSpecs[PROP_AUDIO_SINK] =
		g_param_spec_string("audio-sink",
		                    "AudioSink",
		                    "Audio Sink",
		                    DEFAULT_SINK,
		                    RENA_PREF_PARAMS);

	/**
	  * RenaPreferences:audio_device:
	  *
	 */
	gParamSpecs[PROP_AUDIO_DEVICE] =
		g_param_spec_string("audio-device",
		                    "AudioDevice",
		                    "Audio Device",
		                    ALSA_DEFAULT_DEVICE,
		                    RENA_PREF_PARAMS);

	/**
	  * RenaPreferences:software_mixer:
	  *
	  */
	gParamSpecs[PROP_SOFTWARE_MIXER] =
		g_param_spec_boolean("software-mixer",
		                     "SoftwareMixer",
		                     "Use Software Mixer",
		                     FALSE,
		                     RENA_PREF_PARAMS);

	/**
	  * RenaPreferences:software_volume:
	  *
	  */
	gParamSpecs[PROP_SOFTWARE_VOLUME] =
		g_param_spec_double ("software-volume",
		                     "SoftwareVolume",
		                     "Software Volume Preferences",
		                     -1.0,
		                      1.0,
		                     -1.0,
		                     RENA_PREF_PARAMS);

	/**
	  * RenaPreferences:ignore_errors:
	  *
	  */
	gParamSpecs[PROP_IGNORE_ERRORS] =
		g_param_spec_boolean("ignore-errors",
		                     "IgnoreErrors",
		                     "Ignore Errors on Playback",
		                     FALSE,
		                     RENA_PREF_PARAMS);

	/**
	  * RenaPreferences:lateral_panel:
	  *
	  */
	gParamSpecs[PROP_LATERAL_PANEL] =
		g_param_spec_boolean("lateral-panel",
		                     "LateralPanel",
		                     "Show Lateral Panel Preference",
		                     TRUE,
		                     RENA_PREF_PARAMS);

	/**
	  * RenaPreferences:sidebar_size:
	  *
	  */
	gParamSpecs[PROP_SIDEBAR_SIZE] =
		g_param_spec_int ("sidebar-size",
		                  "SidebarSize",
		                  "Sidebar Size Preferences",
		                  0,
		                  G_MAXINT,
		                  DEFAULT_SIDEBAR_SIZE,
		                  RENA_PREF_PARAMS);

	/**
	  * RenaPreferences:secondary_lateral_panel:
	  *
	  */
	gParamSpecs[PROP_SECONDARY_LATERAL_PANEL] =
		g_param_spec_boolean("secondary-lateral-panel",
		                     "SecondaryLateralPanel",
		                     "Show Secondary Lateral Panel Preference",
		                     FALSE,
		                     RENA_PREF_PARAMS);

	/**
	  * RenaPreferences:secondary_sidebar_size:
	  *
	  */
	gParamSpecs[PROP_SECONDARY_SIDEBAR_SIZE] =
		g_param_spec_int ("secondary-sidebar-size",
		                  "SecondarySidebarSize",
		                  "Secondary Sidebar Size Preferences",
		                  0,
		                  G_MAXINT,
		                  DEFAULT_SIDEBAR_SIZE*4,
		                  RENA_PREF_PARAMS);

	/**
	  * RenaPreferences:show_album_art:
	  *
	  */
	gParamSpecs[PROP_SHOW_ALBUM_ART] =
		g_param_spec_boolean("show-album-art",
		                     "ShowAlbumArt",
		                     "show Album Art Preference",
		                      TRUE,
		                      RENA_PREF_PARAMS);

	/**
	  * RenaPreferences:album_art_size:
	  *
	  */
	gParamSpecs[PROP_ALBUM_ART_SIZE] =
		g_param_spec_int ("album-art-size",
		                  "AlbumArtSize",
		                  "Album Art Size Preferences",
		                  24,
		                  128,
		                  DEFAULT_ALBUM_ART_SIZE,
		                  RENA_PREF_PARAMS);

	/**
	  * RenaPreferences:album_art_pattern:
	  *
	  */
	gParamSpecs[PROP_ALBUM_ART_PATTERN] =
		g_param_spec_string("album-art-pattern",
		                    "AlbumArtPattern",
		                    "Album Art Pattern Preferences",
		                    "",
		                    RENA_PREF_PARAMS);

	/**
	  * RenaPreferences:toolbar_size:
	  *
	  */
	gParamSpecs[PROP_TOOLBAR_SIZE] =
		g_param_spec_enum ("toolbar-size",
		                   "ToolbarSize",
		                   "Toolbar Size Preferences",
		                   GTK_TYPE_ICON_SIZE,
		                   GTK_ICON_SIZE_LARGE_TOOLBAR,
		                   RENA_PREF_PARAMS);

	/**
	  * RenaPreferences:show_status_icon:
	  *
	  */
	gParamSpecs[PROP_SHOW_STATUS_ICON] =
		g_param_spec_boolean("show-status-icon",
		                     "ShowStatusIcon",
		                     "Show Status Icon Preference",
		                      FALSE,
		                      RENA_PREF_PARAMS);

	/**
	  * RenaPreferences:show_menubar:
	  *
	  */
	gParamSpecs[PROP_SHOW_MENUBAR] =
		g_param_spec_boolean("show-menubar",
		                     "ShowMenubar",
		                     "Show Menubar Preference",
		                      TRUE,
		                      RENA_PREF_PARAMS);

	/**
	  * RenaPreferences:system_titlebar:
	  *
	  */
	gParamSpecs[PROP_SYSTEM_TITLEBAR] =
		g_param_spec_boolean("system-titlebar",
		                     "SystemTitlebar",
		                     "System Titlebar Preference",
		                      TRUE,
		                      RENA_PREF_PARAMS);

	/**
	  * RenaPreferences:controls_below:
	  *
	  */
	gParamSpecs[PROP_CONTROLS_BELOW] =
		g_param_spec_boolean("controls-below",
		                     "ControlsBelow",
		                     "Controls Below Preference",
		                      FALSE,
		                      RENA_PREF_PARAMS);

	/**
	  * RenaPreferences:remember_state:
	  *
	  */
	gParamSpecs[PROP_REMEMBER_STATE] =
		g_param_spec_boolean("remember-state",
		                     "RememberState",
		                     "Remember State Preference",
		                      TRUE,
		                      RENA_PREF_PARAMS);

	/**
	  * RenaPreferences:start_mode:
	  *
	  */
	gParamSpecs[PROP_START_MODE] =
		g_param_spec_string("start-mode",
		                    "StartMode",
		                    "Start Mode Preference",
		                    NORMAL_STATE,
		                    RENA_PREF_PARAMS);

	/**
	  * RenaPreferences:last_folder:
	  *
	  */
	gParamSpecs[PROP_LAST_FOLDER] =
		g_param_spec_string("last-folder",
		                    "LastFolder",
		                    "Last folder used in file chooser",
		                    g_get_home_dir(),
		                    RENA_PREF_PARAMS);

	/**
	  * RenaPreferences:add_recursively:
	  *
	  */
	gParamSpecs[PROP_ADD_RECURSIVELY] =
		g_param_spec_boolean("add-recursively",
		                     "AddRecursively",
		                     "Add Recursively Preference",
		                     FALSE,
		                     RENA_PREF_PARAMS);

	/**
	 * RenaPreferences:timer_remaining_mode:
	 *
	 */
	gParamSpecs[PROP_TIMER_REMAINING_MODE] =
		g_param_spec_boolean("timer-remaining-mode",
		                     "TimerRemainingMode",
		                     "Timer Remaining Mode Preference",
		                      FALSE,
		                      RENA_PREF_PARAMS);

	/**
	  * RenaPreferences:hide_instead_close:
	  *
	  */
	gParamSpecs[PROP_HIDE_INSTEAD_CLOSE] =
		g_param_spec_boolean("hide-instead-close",
		                     "HideInsteadClose",
		                     "Hide Instead Close Preference",
		                      TRUE,
		                      RENA_PREF_PARAMS);

	/**
	  * RenaPreferences:lock_library:
	  *
	  */
	gParamSpecs[PROP_LOCK_LIBRARY] =
		g_param_spec_boolean("lock-library",
		                     "LockLibrary",
		                     "Lock Library Changes",
		                      FALSE,
		                      RENA_PREF_PARAMS);

	g_object_class_install_properties(object_class, LAST_PROP, gParamSpecs);

	signals[SIGNAL_PLUGINS_CHANGED] =
		g_signal_new ("PluginsChanged",
		              G_TYPE_FROM_CLASS (object_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (RenaPreferencesClass, plugins_change),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__STRING,
		              G_TYPE_NONE, 1, G_TYPE_STRING);

	signals[SIGNAL_LIBRARY_CHANGED] =
		g_signal_new ("LibraryChanged",
		              G_TYPE_FROM_CLASS (object_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (RenaPreferencesClass, library_change),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);

	signals[SIGNAL_NEED_RESTART] =
		g_signal_new ("NeedRestart",
		              G_TYPE_FROM_CLASS (object_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (RenaPreferencesClass, need_restart),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);
}

static void
rena_preferences_init (RenaPreferences *preferences)
{
	preferences->priv = G_TYPE_INSTANCE_GET_PRIVATE(preferences,
	                                                RENA_TYPE_PREFERENCES,
	                                                RenaPreferencesPrivate);
}

/**
 * rena_preferences_get:
 *
 * Queries the global #RenaPreferences instance, which is shared
 * by all modules. The function automatically takes a reference
 * for the caller, so you'll need to call g_object_unref() when
 * you're done with it.
 *
 * Return value: the global #RenaPreferences instance.
 **/
RenaPreferences*
rena_preferences_get (void)
{
	static RenaPreferences *preferences = NULL;

	if (G_UNLIKELY (preferences == NULL)) {

		CDEBUG(DBG_INFO, "Creating a new RenaPreferences instance");

		preferences = g_object_new(RENA_TYPE_PREFERENCES, NULL);
		g_object_add_weak_pointer(G_OBJECT (preferences),
		                          (gpointer) &preferences);
	}
	else {
		g_object_ref (G_OBJECT (preferences));
	}

	return preferences;
}
