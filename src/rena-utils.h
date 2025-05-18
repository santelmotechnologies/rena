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

#ifndef RENA_UTILS_H
#define RENA_UTILS_H

#include <gtk/gtk.h>

#include "rena-musicobject.h"
#include "rena-preferences.h"

#define string_is_empty(s) (!(s) || !(s)[0])
#define string_is_not_empty(s) (s && (s)[0])

gchar *rena_unescape_html_utf75 (const gchar *str);

gchar *e2_utf8_ndup (const gchar *str, glong num);
gsize levenshtein_strcmp(const gchar * s, const gchar * t);
gsize levenshtein_safe_strcmp(const gchar * s, const gchar * t);
gchar *g_strstr_lv (gchar *haystack, gchar *needle, gsize lv_distance);
gchar *rena_strstr_lv(gchar *haystack, gchar *needle, RenaPreferences *preferences);

void set_watch_cursor (GtkWidget *widget);
void remove_watch_cursor (GtkWidget *widget);

GdkPixbuf * rena_gdk_pixbuf_new_from_memory (gconstpointer data, gsize size);
gchar* convert_length_str(gint length);

gboolean
rena_string_list_is_present (GSList *list, const gchar *str);
gboolean
rena_string_list_is_not_present (GSList *list, const gchar *str);
GSList *
rena_string_list_get_added (GSList *list, GSList *new_list);
GSList *
rena_string_list_get_removed (GSList *list, GSList *new_list);

gboolean is_present_str_list(const gchar *str, GSList *list);
GSList* delete_from_str_list(const gchar *str, GSList *list);
gchar * path_get_dir_as_uri (const gchar *path);
gchar* get_display_filename(const gchar *filename, gboolean get_folder);
gchar* get_display_name(RenaMusicobject *mobj);
void free_str_list(GSList *list);
gint compare_utf8_str(const gchar *str1, const gchar *str2);
gchar * rena_escape_slashes (const gchar *str);
gboolean validate_album_art_pattern(const gchar *pattern);
void rena_process_gtk_events ();
void open_url(const gchar *url, GtkWidget *parent);

void
rena_utils_set_menu_position (GtkMenu  *menu,
                                gint     *x,
                                gint     *y,
                                gboolean *push_in,
                                gpointer  user_data);

#endif /* RENA_UTILS_H */
