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

#include "config.h"
#include "rena-app-notification-container.h"


struct _RenaAppNotificationContainer {
	GtkRevealer  parent_instance;

	GtkWidget   *grid;
};

struct _RenaAppNotificationContainerClass {
	GtkRevealerClass parent_class;
};

G_DEFINE_TYPE (RenaAppNotificationContainer, rena_app_notification_container, GTK_TYPE_REVEALER);

static RenaAppNotificationContainer *notification_container = NULL;

static void
rena_app_notification_container_init (RenaAppNotificationContainer *self)
{
	/* Globally accessible singleton */
	g_assert (notification_container == NULL);
	notification_container = self;
	g_object_add_weak_pointer (G_OBJECT (notification_container),
	                           (gpointer *)&notification_container);

	gtk_widget_set_halign (GTK_WIDGET (self), GTK_ALIGN_CENTER);
	gtk_widget_set_valign (GTK_WIDGET (self), GTK_ALIGN_START);

	self->grid = gtk_grid_new ();
	gtk_orientable_set_orientation (GTK_ORIENTABLE (self->grid), GTK_ORIENTATION_VERTICAL);
	gtk_grid_set_row_spacing (GTK_GRID (self->grid), 6);
	gtk_container_add (GTK_CONTAINER (self), self->grid);
}

static void
rena_app_notification_container_class_init (RenaAppNotificationContainerClass *klass)
{
}

RenaAppNotificationContainer *
rena_app_notification_container_get_default (void)
{
	if (notification_container != NULL)
		return notification_container;

	return g_object_new (RENA_TYPE_APP_NOTIFICATION_CONTAINER,
	                     NULL);
}

void
rena_app_notification_container_add_notification (RenaAppNotificationContainer *self,
                                                    GtkWidget                      *notification)
{
	g_assert (RENA_IS_APP_NOTIFICATION_CONTAINER (self));
	g_assert (GTK_IS_WIDGET (notification));

	gtk_container_add (GTK_CONTAINER (self->grid), notification);

	gtk_widget_show (GTK_WIDGET (self));
	gtk_widget_show (GTK_WIDGET (self->grid));
	gtk_widget_show (GTK_WIDGET (notification));

	gtk_revealer_set_reveal_child (GTK_REVEALER (self), TRUE);
}

guint
rena_app_notification_container_get_num_children (RenaAppNotificationContainer *self)
{
	GList *children;
	guint retval;

	g_assert (RENA_IS_APP_NOTIFICATION_CONTAINER (self));

	children = gtk_container_get_children (GTK_CONTAINER (self->grid));
	retval = g_list_length (children);
	g_list_free (children);

	return retval;
}
