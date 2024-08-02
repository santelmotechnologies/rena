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

#ifndef RENA_ALBUM_ART_H
#define RENA_ALBUM_ART_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define RENA_TYPE_ALBUM_ART (rena_album_art_get_type ())
G_DECLARE_FINAL_TYPE (RenaAlbumArt, rena_album_art, RENA, ALBUM_ART, GtkImage)


RenaAlbumArt *rena_album_art_new (void);

const gchar *
rena_album_art_get_path (RenaAlbumArt *albumart);
void
rena_album_art_set_path (RenaAlbumArt *albumart,
                           const char *path);

guint
rena_album_art_get_size (RenaAlbumArt *albumart);
void
rena_album_art_set_size (RenaAlbumArt *albumart,
                           guint size);

void
rena_album_art_set_pixbuf (RenaAlbumArt *albumart,
                             GdkPixbuf *pixbuf);
GdkPixbuf *
rena_album_art_get_pixbuf (RenaAlbumArt *albumart);

G_END_DECLS

#endif /* RENA_ALBUM_ART_H */
