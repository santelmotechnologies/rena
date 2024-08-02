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

#ifndef RENA_APP_NOTIFICATION_H
#define RENA_APP_NOTIFICATION_H

#include <glib-object.h>
#include <gtk/gtk.h>


G_BEGIN_DECLS

typedef struct _RenaAppNotificationClass RenaAppNotificationClass;
typedef struct _RenaAppNotification      RenaAppNotification;

#define RENA_TYPE_APP_NOTIFICATION             (rena_app_notification_get_type ())
#define RENA_APP_NOTIFICATION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_APP_NOTIFICATION, RenaAppNotification))
#define RENA_APP_NOTIFICATION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), RENA_TYPE_APP_NOTIFICATION, RenaAppNotificationClass))
#define RENA_IS_APP_NOTIFICATION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_APP_NOTIFICATION))
#define RENA_IS_APP_NOTIFICATION_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), RENA_TYPE_APP_NOTIFICATION))
#define RENA_APP_NOTIFICATION_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), RENA_TYPE_APP_NOTIFICATION, RenaAppNotificationClass))

RenaAppNotification *
rena_app_notification_new         (const char            *head,
                                     const char            *body);

void
rena_app_notification_show        (RenaAppNotification *self);

void
rena_app_notification_set_timeout (RenaAppNotification *self,
                                     guint                  timeout);

G_END_DECLS

#endif /* RENA_APP_NOTIFICATION_H */
