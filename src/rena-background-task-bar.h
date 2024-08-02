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

#ifndef RENA_BACKGROUND_TASK_BAR_H
#define RENA_BACKGROUND_TASK_BAR_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define RENA_TYPE_BACKGROUND_TASK_BAR (rena_background_task_bar_get_type())
#define RENA_BACKGROUND_TASK_BAR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_BACKGROUND_TASK_BAR, RenaBackgroundTaskBar))
#define RENA_BACKGROUND_TASK_BAR_CONST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_BACKGROUND_TASK_BAR, RenaBackgroundTaskBar const))
#define RENA_BACKGROUND_TASK_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), RENA_TYPE_BACKGROUND_TASK_BAR, RenaBackgroundTaskBarClass))
#define RENA_IS_BACKGROUND_TASK_BAR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_BACKGROUND_TASK_BAR))
#define RENA_IS_BACKGROUND_TASK_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), RENA_TYPE_BACKGROUND_TASK_BAR))
#define RENA_BACKGROUND_TASK_BAR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), RENA_TYPE_BACKGROUND_TASK_BAR, RenaBackgroundTaskBarClass))

GType rena_background_task_bar_get_type (void);

typedef struct _RenaBackgroundTaskBar RenaBackgroundTaskBar;

void
rena_background_task_bar_prepend_widget (RenaBackgroundTaskBar *taskbar,
                                           GtkWidget               *widget);

void
rena_background_task_bar_remove_widget  (RenaBackgroundTaskBar *taskbar,
                                           GtkWidget               *widget);

RenaBackgroundTaskBar *
rena_background_task_bar_new            (void);

RenaBackgroundTaskBar *
rena_background_task_bar_get            (void);

G_END_DECLS

#endif /* RENA_BACKGROUND_TASK_BAR_H */
