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
#ifndef RENA_PREFERENCES_DIALOG_H
#define RENA_PREFERENCES_DIALOG_H

#include <gtk/gtk.h>

#define LASTFM_UNAME_LEN           256
#define LASTFM_PASS_LEN            512
#define ALBUM_ART_PATTERN_LEN      1024
#define AUDIO_CD_DEVICE_ENTRY_LEN  32

#define RENA_TYPE_PREFERENCES_DIALOG (rena_preferences_dialog_get_type())
#define RENA_PREFERENCES_DIALOG(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_PREFERENCES_DIALOG, RenaPreferencesDialog))
#define RENA_PREFERENCES_DIALOG_CONST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_PREFERENCES_DIALOG, RenaPreferencesDialog const))
#define RENA_PREFERENCES_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), RENA_TYPE_PREFERENCES_DIALOG, RenaPreferencesDialogClass))
#define RENA_IS_PREFERENCES_DIALOG(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_PREFERENCES_DIALOG))
#define RENA_IS_PREFERENCES_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), RENA_TYPE_PREFERENCES_DIALOG))
#define RENA_PREFERENCES_DIALOG_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), RENA_TYPE_PREFERENCES_DIALOG, RenaPreferencesDialogClass))

GType rena_preferences_dialog_get_type (void);

typedef struct _RenaPreferencesDialog RenaPreferencesDialog;

void               rena_gtk_entry_set_text                  (GtkEntry *entry, const gchar *text);

void               rena_preferences_append_audio_setting    (RenaPreferencesDialog *dialog, GtkWidget *widget, gboolean expand);
void               rena_preferences_remove_audio_setting    (RenaPreferencesDialog *dialog, GtkWidget *widget);

void               rena_preferences_append_desktop_setting  (RenaPreferencesDialog *dialog, GtkWidget *widget, gboolean expand);
void               rena_preferences_remove_desktop_setting  (RenaPreferencesDialog *dialog, GtkWidget *widget);

void               rena_preferences_append_services_setting (RenaPreferencesDialog *dialog, GtkWidget *widget, gboolean expand);
void               rena_preferences_remove_services_setting (RenaPreferencesDialog *dialog, GtkWidget *widget);

void               rena_preferences_dialog_connect_handler    (RenaPreferencesDialog *dialog,
                                                                 GCallback          callback,
                                                                 gpointer           user_data);
void               rena_preferences_dialog_disconnect_handler (RenaPreferencesDialog *rena,
                                                                 GCallback          callback,
                                                                 gpointer           user_data);

void               rena_preferences_dialog_show            (RenaPreferencesDialog *dialog);

void               rena_preferences_dialog_set_parent         (RenaPreferencesDialog *dialog, GtkWidget *parent);

RenaPreferencesDialog *
rena_preferences_dialog_get (void);

#endif /* RENA_PREFERENCES_DIALOG_H */
