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

#ifndef RENA_FAVORITES_H
#define RENA_FAVORITES_H

#include <glib.h>
#include <glib-object.h>

#include "rena-musicobject.h"

G_BEGIN_DECLS

#define RENA_TYPE_FAVORITES (rena_favorites_get_type())
#define RENA_FAVORITES(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_FAVORITES, RenaFavorites))
#define RENA_FAVORITES_CONST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_FAVORITES, RenaFavorites const))
#define RENA_FAVORITES_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), RENA_TYPE_FAVORITES, RenaFavoritesClass))
#define RENA_IS_FAVORITES(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_FAVORITES))
#define RENA_IS_FAVORITES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), RENA_TYPE_FAVORITES))
#define RENA_FAVORITES_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), RENA_TYPE_FAVORITES, RenaFavoritesClass))

typedef struct _RenaFavorites RenaFavorites;
typedef struct _RenaFavoritesClass RenaFavoritesClass;

struct _RenaFavoritesClass
{
	GObjectClass parent_class;
	void (*song_added)    (RenaFavorites *favorites, RenaMusicobject *mobj);
	void (*song_removed)  (RenaFavorites *favorites, RenaMusicobject *mobj);
};

RenaFavorites *
rena_favorites_get      (void);

void             rena_favorites_put_song      (RenaFavorites *favorites, RenaMusicobject *mobj);
void             rena_favorites_remove_song   (RenaFavorites *favorites, RenaMusicobject *mobj);
gboolean         rena_favorites_contains_song (RenaFavorites *favorites, RenaMusicobject *mobj);

G_END_DECLS

#endif /* RENA_FAVORITES_H */
