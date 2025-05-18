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

#ifndef RENA_SIMPLE_WIDGETS_H
#define RENA_SIMPLE_WIDGETS_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _RenaHeader        RenaHeader;
typedef struct _RenaTrackProgress RenaTrackProgress;
typedef struct _RenaContainer     RenaContainer;
typedef struct _RenaToolbarButton RenaToolbarButton;
typedef struct _RenaToggleButton  RenaToggleButton;

RenaHeader *rena_header_new (void);
void
rena_header_set_icon_name (RenaHeader *header,
                             const gchar  *icon_name);
void
rena_header_set_title (RenaHeader *header,
                         const gchar  *title);
void
rena_header_set_subtitle (RenaHeader *header,
                            const gchar  *subtitle);


void rena_toolbar_button_set_icon_name (RenaToolbarButton *button, const gchar *icon_name);
void rena_toolbar_button_set_icon_size (RenaToolbarButton *button, GtkIconSize  icon_size);
RenaToolbarButton *rena_toolbar_button_new (const gchar *icon_name);

void rena_toggle_button_set_icon_name (RenaToggleButton *button, const gchar *icon_name);
void rena_toggle_button_set_icon_size (RenaToggleButton *button, GtkIconSize  icon_size);
RenaToggleButton *rena_toggle_button_new (const gchar *icon_name);


G_END_DECLS

#endif /* RENA_SIMPLE_WIDGETS_H */
