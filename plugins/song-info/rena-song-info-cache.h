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

#ifndef RENA_INFO_CACHE_H
#define RENA_INFO_CACHE_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define RENA_TYPE_INFO_CACHE (rena_info_cache_get_type())
#define RENA_INFO_CACHE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_INFO_CACHE, RenaInfoCache))
#define RENA_INFO_CACHE_CONST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_INFO_CACHE, RenaInfoCache const))
#define RENA_INFO_CACHE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), RENA_TYPE_INFO_CACHE, RenaInfoCacheClass))
#define RENA_IS_INFO_CACHE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_INFO_CACHE))
#define RENA_IS_INFO_CACHE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), RENA_TYPE_INFO_CACHE))
#define RENA_INFO_CACHE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), RENA_TYPE_INFO_CACHE, RenaInfoCacheClass))

typedef struct _RenaInfoCache RenaInfoCache;
typedef struct _RenaInfoCacheClass RenaInfoCacheClass;

struct _RenaInfoCacheClass
{
	GObjectClass parent_class;
	void (*cache_changed)    (RenaInfoCache *cache);
};

RenaInfoCache *
rena_info_cache_get                (void);

gboolean
rena_info_cache_contains_similar_songs (RenaInfoCache *cache,
                                          const gchar     *title,
                                          const gchar     *artist);


GList *
rena_info_cache_get_similar_songs      (RenaInfoCache *cache,
                                          const gchar     *title,
                                          const gchar     *artist,
                                          gchar          **provider);


void
rena_info_cache_save_similar_songs     (RenaInfoCache *cache,
                                          const gchar     *title,
                                          const gchar     *artist,
                                          const gchar     *provider,
                                          GList           *mlist);

gboolean
rena_info_cache_contains_song_lyrics   (RenaInfoCache *cache,
                                          const gchar     *title,
                                          const gchar     *artist);

void
rena_info_cache_save_song_lyrics       (RenaInfoCache *cache,
                                          const gchar     *title,
                                          const gchar     *artist,
                                          const gchar     *provider,
                                          const gchar     *lyrics);

gchar *
rena_info_cache_get_song_lyrics        (RenaInfoCache *cache,
                                          const gchar     *title,
                                          const gchar     *artist,
                                          gchar          **provider);
gboolean
rena_info_cache_contains_artist_bio    (RenaInfoCache *cache,
                                          const gchar     *artist);

gboolean
rena_info_cache_contains_ini_artist_bio(RenaInfoCache *cache,
                                          const gchar     *artist);

gchar *
rena_info_cache_get_artist_bio         (RenaInfoCache *cache,
                                          const gchar     *artist,
                                          gchar          **provider);

void
rena_info_cache_save_artist_bio        (RenaInfoCache *cache,
                                          const gchar     *artist,
                                          const gchar     *provider,
                                          const gchar     *bio);

G_END_DECLS

#endif /* RENA_INFO_CACHE_H */
