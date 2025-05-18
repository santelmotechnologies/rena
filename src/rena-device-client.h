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

#ifndef RENA_DEVICE_CLIENT_H
#define RENA_DEVICE_CLIENT_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include <gudev/gudev.h>

G_BEGIN_DECLS

/* Device types */

typedef enum {
	RENA_DEVICE_NONE = 0,
	RENA_DEVICE_MOUNTABLE,
	RENA_DEVICE_AUDIO_CD,
	RENA_DEVICE_EMPTY_AUDIO_CD,
	RENA_DEVICE_MTP,
	RENA_DEVICE_UNKNOWN
} RenaDeviceType;

#define RENA_TYPE_DEVICE_CLIENT (rena_device_client_get_type())
#define RENA_DEVICE_CLIENT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_DEVICE_CLIENT, RenaDeviceClient))
#define RENA_DEVICE_CLIENT_CONST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_DEVICE_CLIENT, RenaDeviceClient const))
#define RENA_DEVICE_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), RENA_TYPE_DEVICE_CLIENT, RenaDeviceClientClass))
#define RENA_IS_DEVICE_CLIENT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_DEVICE_CLIENT))
#define RENA_IS_DEVICE_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), RENA_TYPE_DEVICE_CLIENT))
#define RENA_DEVICE_CLIENT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), RENA_TYPE_DEVICE_CLIENT, RenaDeviceClientClass))

typedef struct _RenaDeviceClient RenaDeviceClient;
typedef struct _RenaDeviceClientClass RenaDeviceClientClass;

struct _RenaDeviceClientClass
{
	GObjectClass parent_class;
	void (*device_added)   (RenaDeviceClient *device_client,
	                        RenaDeviceType    device_type,
	                        GUdevDevice        *u_device);
	void (*device_removed) (RenaDeviceClient *device_client,
	                        RenaDeviceType    device_type,
	                        GUdevDevice        *u_device);
};

/* Dialog when add device */

enum
{
	RENA_DEVICE_RESPONSE_NONE,
	RENA_DEVICE_RESPONSE_PLAY,
	RENA_DEVICE_RESPONSE_BROWSE,
};

GtkWidget *
rena_gudev_dialog_new (GtkWidget   *parent,
                         const gchar *title,
                         const gchar *icon,
                         const gchar *primary_text,
                         const gchar *secondary_text,
                         const gchar *first_button_text,
                         gint         first_button_response);

gint
rena_gudev_get_property_as_int (GUdevDevice *device,
                                  const gchar *property,
                                  gint         base);

/* Create a new instance of RenaDeviceClient* */

RenaDeviceClient *rena_device_client_get          (void);

G_END_DECLS

#endif /* RENA_DEVICE_CLIENT_H */
