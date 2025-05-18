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

#ifndef RENA_FILE_UTILS_H
#define RENA_FILE_UTILS_H

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <gio/gio.h>
#include "rena-musicobject.h"
#include "rena-preferences.h"

/* Playlist type formats */

typedef enum {
	PL_FORMAT_UNKNOWN,
	PL_FORMAT_M3U,
	PL_FORMAT_PLS,
	PL_FORMAT_ASX,
	PL_FORMAT_XSPF
} RenaPlaylistType;

typedef enum {
	MEDIA_TYPE_UNKNOWN,
	MEDIA_TYPE_AUDIO,
	MEDIA_TYPE_PLAYLIST,
	MEDIA_TYPE_IMAGE
} RenaMediaType;

extern const gchar *mime_mpeg[];
extern const gchar *mime_wav[];
extern const gchar *mime_flac[];
extern const gchar *mime_ogg[];
extern const gchar *mime_asf[];
extern const gchar *mime_mp4[];
extern const gchar *mime_ape[];
extern const gchar *mime_tracker[];

extern const gchar *mime_image[];

#ifdef HAVE_PLPARSER
extern const gchar *mime_playlist[];
extern const gchar *mime_dual[];
#endif

gboolean is_playable_file(const gchar *file);

RenaMediaType    rena_file_get_media_type                   (const gchar *filename);
gchar             *rena_file_get_music_type                   (const gchar *filename);
RenaPlaylistType rena_pl_parser_guess_format_from_extension (const gchar *filename);

gboolean is_dir_and_accessible(const gchar *dir);

gchar    *get_image_path_from_dir (const gchar *path);
gchar    *get_pref_image_path_dir (RenaPreferences *preferences, const gchar *path);

gint rena_get_dir_count(const gchar *dir_name, GCancellable *cancellable);

GList *append_mobj_list_from_folder(GList *list, gchar *dir_name);
GList *append_mobj_list_from_unknown_filename(GList *list, gchar *filename);

#endif /* RENA_FILE_UTILS_H */
