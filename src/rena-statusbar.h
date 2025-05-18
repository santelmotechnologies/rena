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


#ifndef __RENA_STATUSBAR_H__
#define __RENA_STATUSBAR_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS;

typedef struct _RenaStatusbarClass RenaStatusbarClass;
typedef struct _RenaStatusbar      RenaStatusbar;

#define RENA_TYPE_STATUSBAR             (rena_statusbar_get_type ())
#define RENA_STATUSBAR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_STATUSBAR, RenaStatusbar))
#define RENA_STATUSBAR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), RENA_TYPE_STATUSBAR, RenaStatusbarClass))
#define RENA_IS_STATUSBAR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_STATUSBAR))
#define RENA_IS_STATUSBAR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), RENA_TYPE_STATUSBAR))
#define RENA_STATUSBAR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), RENA_TYPE_STATUSBAR, RenaStatusbarClass))

void
rena_statusbar_set_main_text (RenaStatusbar *statusbar,
                                const gchar     *text);


GType      rena_statusbar_get_type    (void) G_GNUC_CONST;

RenaStatusbar *
rena_statusbar_get (void);

G_END_DECLS;

#endif /* !__RENA_STATUSBAR_H__ */

