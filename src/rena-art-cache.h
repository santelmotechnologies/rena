/*
 * Copyright (C) 2024 Santelmo Technologies <santelmotechnologies@gmail.com> 
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef RENA_ART_CACHE_H
#define RENA_ART_CACHE_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define RENA_TYPE_ART_CACHE (rena_art_cache_get_type())
#define RENA_ART_CACHE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_ART_CACHE, RenaArtCache))
#define RENA_ART_CACHE_CONST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_ART_CACHE, RenaArtCache const))
#define RENA_ART_CACHE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), RENA_TYPE_ART_CACHE, RenaArtCacheClass))
#define RENA_IS_ART_CACHE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_ART_CACHE))
#define RENA_IS_ART_CACHE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), RENA_TYPE_ART_CACHE))
#define RENA_ART_CACHE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), RENA_TYPE_ART_CACHE, RenaArtCacheClass))

typedef struct _RenaArtCache RenaArtCache;
typedef struct _RenaArtCacheClass RenaArtCacheClass;

struct _RenaArtCacheClass
{
	GObjectClass parent_class;
	void (*cache_changed)    (RenaArtCache *cache);
};

RenaArtCache * rena_art_cache_get      (void);

gchar *          rena_art_cache_get_album_uri   (RenaArtCache *cache, const gchar *artist, const gchar *album);
gboolean         rena_art_cache_contains_album  (RenaArtCache *cache, const gchar *artist, const gchar *album);
void             rena_art_cache_put_album       (RenaArtCache *cache, const gchar *artist, const gchar *album, gconstpointer data, gsize size);

gchar *          rena_art_cache_get_artist_uri  (RenaArtCache *cache, const gchar *artist);
gboolean         rena_art_cache_contains_artist (RenaArtCache *cache, const gchar *artist);
void             rena_art_cache_put_artist      (RenaArtCache *cache, const gchar *artist, gconstpointer data, gsize size);

G_END_DECLS

#endif /* RENA_ART_CACHE_H */
