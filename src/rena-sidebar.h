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

#ifndef RENA_SIDEBAR_H
#define RENA_SIDEBAR_H

#include <gtk/gtk.h>

#define RENA_TYPE_SIDEBAR            (rena_sidebar_get_type ())
#define RENA_SIDEBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_SIDEBAR, RenaSidebar))
#define RENA_IS_SIDEBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_SIDEBAR))
#define RENA_SIDEBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  RENA_TYPE_SIDEBAR, RenaSidebarClass))
#define RENA_IS_SIDEBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  RENA_TYPE_SIDEBAR))
#define RENA_SIDEBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  RENA_TYPE_SIDEBAR, RenaSidebarClass))

typedef struct _RenaSidebar RenaSidebar;

typedef struct {
	GtkScrolledWindowClass    __parent__;
	void (*children_changed) (RenaSidebar *sidebar);
} RenaSidebarClass;

void
rena_sidebar_attach_plugin (RenaSidebar *sidebar,
                              GtkWidget     *widget,
                              GtkWidget     *title,
                              GtkWidget     *popover);

void
rena_sidebar_remove_plugin (RenaSidebar *sidebar,
                              GtkWidget     *widget);

gint
rena_sidebar_get_n_panes (RenaSidebar *sidebar);

void
rena_sidebar_style_position (RenaSidebar  *sidebar,
                               GtkPositionType position);

RenaSidebar *rena_sidebar_new (void);

#endif /* RENA_SIDEBAR_H */
