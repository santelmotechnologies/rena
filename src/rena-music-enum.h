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

#ifndef RENA_MUSIC_ENUM_H
#define RENA_MUSIC_ENUM_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define MAX_ENUM_SIZE 10

#define RENA_TYPE_MUSIC_ENUM (rena_music_enum_get_type())
#define RENA_MUSIC_ENUM(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_MUSIC_ENUM, RenaMusicEnum))
#define RENA_MUSIC_ENUM_CONST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_MUSIC_ENUM, RenaMusicEnum const))
#define RENA_MUSIC_ENUM_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), RENA_TYPE_MUSIC_ENUM, RenaMusicEnumClass))
#define RENA_IS_MUSIC_ENUM(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_MUSIC_ENUM))
#define RENA_IS_MUSIC_ENUM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), RENA_TYPE_MUSIC_ENUM))
#define RENA_MUSIC_ENUM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), RENA_TYPE_MUSIC_ENUM, RenaMusicEnumClass))

typedef struct _RenaMusicEnum RenaMusicEnum;
typedef struct _RenaMusicEnumClass RenaMusicEnumClass;

struct _RenaMusicEnumClass
{
	GObjectClass parent_class;
	void (*enum_removed)    (RenaMusicEnum *enum_map, gint enum_removed);
};

RenaMusicEnum *rena_music_enum_get          (void);

const gchar     *rena_music_enum_map_get_name (RenaMusicEnum *enum_map, gint enum_code);
gint             rena_music_enum_map_get      (RenaMusicEnum *enum_map, const gchar *name);
gint             rena_music_enum_map_remove   (RenaMusicEnum *enum_map, const gchar *name);

G_END_DECLS

#endif /* RENA_MUSIC_ENUM_H */
