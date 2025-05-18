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

#ifndef RENA_SONG_CACHE_H
#define RENA_SONG_CACHE_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define RENA_TYPE_SONG_CACHE (rena_song_cache_get_type())
#define RENA_SONG_CACHE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_SONG_CACHE, RenaSongCache))
#define RENA_SONG_CACHE_CONST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_SONG_CACHE, RenaSongCache const))
#define RENA_SONG_CACHE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), RENA_TYPE_SONG_CACHE, RenaSongCacheClass))
#define RENA_IS_SONG_CACHE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_SONG_CACHE))
#define RENA_IS_SONG_CACHE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), RENA_TYPE_SONG_CACHE))
#define RENA_SONG_CACHE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), RENA_TYPE_SONG_CACHE, RenaSongCacheClass))

typedef struct _RenaSongCache RenaSongCache;
typedef struct _RenaSongCacheClass RenaSongCacheClass;

struct _RenaSongCacheClass
{
	GObjectClass parent_class;
};

RenaSongCache *rena_song_cache_get                (void);

void             rena_song_cache_put_location      (RenaSongCache *cache, const gchar *location, const gchar *filename);
gchar           *rena_song_cache_get_from_location (RenaSongCache *cache, const gchar *location);


G_END_DECLS

#endif /* RENA_SONG_CACHE_H */
