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

#ifndef RENA_APP_NOTIFICATION_CONTAINER_H
#define RENA_APP_NOTIFICATION_CONTAINER_H

#include <gtk/gtk.h>


G_BEGIN_DECLS

typedef struct _RenaAppNotificationContainerClass RenaAppNotificationContainerClass;
typedef struct _RenaAppNotificationContainer      RenaAppNotificationContainer;

#define RENA_TYPE_APP_NOTIFICATION_CONTAINER             (rena_app_notification_container_get_type ())
#define RENA_APP_NOTIFICATION_CONTAINER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_APP_NOTIFICATION_CONTAINER, RenaAppNotificationContainer))
#define RENA_APP_NOTIFICATION_CONTAINER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), RENA_TYPE_APP_NOTIFICATION_CONTAINER, RenaAppNotificationContainerClass))
#define RENA_IS_APP_NOTIFICATION_CONTAINER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_APP_NOTIFICATION_CONTAINER))
#define RENA_IS_APP_NOTIFICATION_CONTAINER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), RENA_TYPE_APP_NOTIFICATION_CONTAINER))
#define RENA_APP_NOTIFICATION_CONTAINER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), RENA_TYPE_APP_NOTIFICATION_CONTAINER, RenaAppNotificationContainerClass))

RenaAppNotificationContainer *
rena_app_notification_container_get_default      (void);

void
rena_app_notification_container_add_notification (RenaAppNotificationContainer *self,
                                                    GtkWidget                      *notification);

guint
rena_app_notification_container_get_num_children (RenaAppNotificationContainer *self);

G_END_DECLS

#endif /* RENA_APP_NOTIFICAION_CONTAINER_H */
