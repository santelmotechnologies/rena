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

#ifndef RENA_STATUSICON_H
#define RENA_STATUSICON_H

#include <gtk/gtk.h>

/* rena.h */
typedef struct _RenaApplication RenaApplication;

#define RENA_TYPE_STATUS_ICON                  (rena_status_icon_get_type ())
#define RENA_STATUS_ICON(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_STATUS_ICON, RenaStatusIcon))
#define RENA_IS_STATUS_ICON(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_STATUS_ICON))
#define RENA_STATUS_ICON_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), RENA_TYPE_STATUS_ICON, RenaStatusIconClass))
#define RENA_IS_STATUS_ICON_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), RENA_TYPE_STATUS_ICON))
#define RENA_STATUS_ICON_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), RENA_TYPE_STATUS_ICON, RenaStatusIconClass))

typedef struct {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
	GtkStatusIconClass __parent__;
G_GNUC_END_IGNORE_DEPRECATIONS
} RenaStatusIconClass;

typedef struct _RenaStatusIcon RenaStatusIcon;

void
rena_systray_append_action (RenaStatusIcon *status_icon,
                              const gchar      *placeholder,
                              GSimpleAction    *action,
                              GMenuItem        *item);

void
rena_systray_remove_action (RenaStatusIcon *status_icon,
                              const gchar      *placeholder,
                              const gchar      *action_name);

RenaStatusIcon *rena_status_icon_new (RenaApplication *rena);

#endif /* RENA_STATUSICON_H */
