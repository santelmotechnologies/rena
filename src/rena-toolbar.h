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

#ifndef RENA_TOOLBAR_H
#define RENA_TOOLBAR_H

#include <gtk/gtk.h>

#include "rena-musicobject.h"
#include "rena-album-art.h"
#include "rena-backend.h"

#define RENA_TYPE_TOOLBAR                  (rena_toolbar_get_type ())
#define RENA_TOOLBAR(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_TOOLBAR, RenaToolbar))
#define RENA_IS_TOOLBAR(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_TOOLBAR))
#define RENA_TOOLBAR_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), RENA_TYPE_TOOLBAR, RenaToolbarClass))
#define RENA_IS_TOOLBAR_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), RENA_TYPE_TOOLBAR))
#define RENA_TOOLBAR_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), RENA_TYPE_TOOLBAR, RenaToolbarClass))

typedef struct _RenaToolbar RenaToolbar;

typedef struct {
	GtkHeaderBarClass __parent__;

	void (*prev) (RenaToolbar *toolbar);
	void (*play) (RenaToolbar *toolbar);
	void (*stop) (RenaToolbar *toolbar);
	void (*next) (RenaToolbar *toolbar);
	void (*album_art_activated) (RenaToolbar *toolbar);
	void (*track_info_activated) (RenaToolbar *toolbar);
	void (*track_progress_activated) (RenaToolbar *toolbar, gdouble fraction);
	void (*favorite_toggle) (RenaToolbar *toolbar);
	void (*unfull) (RenaToolbar *toolbar);
} RenaToolbarClass;

void rena_toolbar_set_title               (RenaToolbar *toolbar, RenaMusicobject *mobj);
void rena_toolbar_set_image_album_art     (RenaToolbar *toolbar, const gchar *uri);
void rena_toolbar_set_favorite_icon       (RenaToolbar *toolbar, gboolean love);
void rena_toolbar_update_progress         (RenaToolbar *toolbar, gint length, gint progress);

void rena_toolbar_update_buffering_cb      (RenaBackend *backend, gint percent, gpointer user_data);
void rena_toolbar_update_playback_progress (RenaBackend *backend, gpointer user_data);
void rena_toolbar_playback_state_cb        (RenaBackend *backend, GParamSpec *pspec, gpointer user_data);
void rena_toolbar_show_ramaning_time_cb    (RenaToolbar *toolbar, GParamSpec *pspec, gpointer user_data);

gboolean rena_toolbar_window_state_event   (GtkWidget *widget, GdkEventWindowState *event, RenaToolbar *toolbar);

void     rena_toolbar_set_style            (RenaToolbar *toolbar, gboolean gnome_style);
void     rena_toolbar_add_extention_widget (RenaToolbar *toolbar, GtkWidget *widget);

void     rena_toolbar_add_extra_button     (RenaToolbar *toolbar, GtkWidget *widget);
void     rena_toolbar_remove_extra_button  (RenaToolbar *toolbar, GtkWidget *widget);

const gchar    *rena_toolbar_get_progress_text (RenaToolbar *toolbar);
const gchar    *rena_toolbar_get_length_text   (RenaToolbar *toolbar);

GtkWidget      *rena_toolbar_get_song_box      (RenaToolbar *toolbar);
RenaAlbumArt *rena_toolbar_get_album_art     (RenaToolbar *toolbar);

RenaToolbar *rena_toolbar_new        (void);

#endif /* RENA_TOOLBAR_H */
