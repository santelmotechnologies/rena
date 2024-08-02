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

#ifndef RENA_WINDOW_H
#define RENA_WINDOW_H

#include <gtk/gtk.h>

#include "rena-backend.h"
#include "rena.h"

gboolean rena_close_window        (GtkWidget *widget, GdkEvent *event, RenaApplication *rena);
void     rena_destroy_window      (GtkWidget *widget, RenaApplication *rena);
void     rena_window_toggle_state (RenaApplication *rena, gboolean ignoreActivity);

void     rena_window_show_backend_error_dialog (RenaApplication *rena);

void     gui_backend_error_update_current_playlist_cb (RenaBackend *backend, const GError *error, RenaApplication *rena);

void     rena_window_unfullscreen          (GObject *object, RenaApplication *rena);

void     rena_window_add_widget_to_infobox (RenaApplication *rena, GtkWidget *widget);

void     rena_init_gui_state (RenaApplication *rena);

void     rena_window_save_settings (RenaApplication *rena);

void     rena_window_new  (RenaApplication *rena);

#endif /* RENA_WINDOW_H */
