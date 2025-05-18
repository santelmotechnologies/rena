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

#ifndef RENA_BACKGROUND_TASK_WIDGET_H
#define RENA_BACKGROUND_TASK_WIDGET_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define RENA_TYPE_BACKGROUND_TASK_WIDGET (rena_background_task_widget_get_type())
#define RENA_BACKGROUND_TASK_WIDGET(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_BACKGROUND_TASK_WIDGET, RenaBackgroundTaskWidget))
#define RENA_BACKGROUND_TASK_WIDGET_CONST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_BACKGROUND_TASK_WIDGET, RenaBackgroundTaskWidget const))
#define RENA_BACKGROUND_TASK_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), RENA_TYPE_BACKGROUND_TASK_WIDGET, RenaBackgroundTaskWidgetClass))
#define RENA_IS_BACKGROUND_TASK_WIDGET(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_BACKGROUND_TASK_WIDGET))
#define RENA_IS_BACKGROUND_TASK_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), RENA_TYPE_BACKGROUND_TASK_WIDGET))
#define RENA_BACKGROUND_TASK_WIDGET_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), RENA_TYPE_BACKGROUND_TASK_WIDGET, RenaBackgroundTaskWidgetClass))

GType rena_background_task_widget_get_type (void);

typedef struct _RenaBackgroundTaskWidget RenaBackgroundTaskWidget;

void
rena_background_task_widget_set_description (RenaBackgroundTaskWidget *taskwidget,
                                               const gchar                *description);

void
rena_background_task_widget_set_job_count (RenaBackgroundTaskWidget *taskwidget,
                                             gint                        job_count);

void
rena_background_task_widget_set_job_progress (RenaBackgroundTaskWidget *taskwidget,
                                                gint                        job_progress);

RenaBackgroundTaskWidget *
rena_background_task_widget_new (const gchar  *description,
                                   const gchar  *icon_name,
                                   gint          job_count,
                                   GCancellable *cancellable);

G_END_DECLS

#endif /* RENA_BACKGROUND_TASK_WIDGET_H */
