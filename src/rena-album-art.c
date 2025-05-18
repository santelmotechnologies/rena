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

#include "rena-album-art.h"

#include <glib.h>

#ifdef G_OS_WIN32
#include "win32/win32dep.h"
#endif

struct _RenaAlbumArt
{
   GtkImage parent;
   gchar *path;
   guint size;
};

G_DEFINE_TYPE (RenaAlbumArt, rena_album_art, GTK_TYPE_IMAGE)

enum
{
   PROP_0,
   PROP_PATH,
   PROP_SIZE,
   LAST_PROP
};

static GParamSpec *gParamSpecs[LAST_PROP];

RenaAlbumArt *
rena_album_art_new (void)
{
   return g_object_new(RENA_TYPE_ALBUM_ART, NULL);
}

/**
 * rena_album_art_update_image:
 *
 */

static void
rena_album_art_update_image (RenaAlbumArt *albumart)
{
   GdkPixbuf *pixbuf = NULL, *album_art = NULL, *frame;
   gchar *frame_uri = NULL;
   GError *error = NULL;

   g_return_if_fail(RENA_IS_ALBUM_ART(albumart));

   frame_uri = g_build_filename (PIXMAPDIR, "cover.png", NULL);
   frame = gdk_pixbuf_new_from_file (frame_uri, NULL);
   g_free (frame_uri);

   if (albumart->path != NULL) {
      #ifdef G_OS_WIN32
      GdkPixbuf *a_pixbuf = gdk_pixbuf_new_from_file (albumart->path, &error);
      if (a_pixbuf) {
         album_art = gdk_pixbuf_scale_simple (a_pixbuf, 112, 112, GDK_INTERP_BILINEAR);
         g_object_unref(G_OBJECT(a_pixbuf));
      }
      #else
      album_art = gdk_pixbuf_new_from_file_at_scale(albumart->path,
                                                    112,
                                                    112,
                                                    FALSE,
                                                    &error);
      #endif
      if (album_art) {
         gdk_pixbuf_copy_area (album_art, 0, 0, 112, 112, frame, 12, 8);
         g_object_unref(G_OBJECT(album_art));
      }
      else {
         g_critical("Unable to open image file: %s\n", albumart->path);
         g_error_free(error);
      }
   }

   pixbuf = gdk_pixbuf_scale_simple (frame,
                                     albumart->size,
                                     albumart->size,
                                     GDK_INTERP_BILINEAR);

   rena_album_art_set_pixbuf(albumart, pixbuf);

   g_object_unref(G_OBJECT(pixbuf));
   g_object_unref(G_OBJECT(frame));
}

/**
 * album_art_get_path:
 *
 */
const gchar *
rena_album_art_get_path (RenaAlbumArt *albumart)
{
   g_return_val_if_fail(RENA_IS_ALBUM_ART(albumart), NULL);
   return albumart->path;
}

/**
 * album_art_set_path:
 *
 */
void
rena_album_art_set_path (RenaAlbumArt *albumart,
                           const gchar    *path)
{
   g_return_if_fail(RENA_IS_ALBUM_ART(albumart));

   g_free(albumart->path);
   albumart->path = g_strdup(path);

   rena_album_art_update_image(albumart);

   g_object_notify_by_pspec(G_OBJECT(albumart), gParamSpecs[PROP_PATH]);
}

/**
 * album_art_get_size:
 *
 */
guint
rena_album_art_get_size (RenaAlbumArt *albumart)
{
   g_return_val_if_fail(RENA_IS_ALBUM_ART(albumart), 0);
   return albumart->size;
}

/**
 * album_art_set_size:
 *
 */
void
rena_album_art_set_size (RenaAlbumArt *albumart,
                           guint           size)
{
   g_return_if_fail(RENA_IS_ALBUM_ART(albumart));

   albumart->size = size;

   rena_album_art_update_image(albumart);

   g_object_notify_by_pspec(G_OBJECT(albumart), gParamSpecs[PROP_SIZE]);
}

/**
 * album_art_set_pixbuf:
 *
 */
void
rena_album_art_set_pixbuf (RenaAlbumArt *albumart, GdkPixbuf *pixbuf)
{
   g_return_if_fail(RENA_IS_ALBUM_ART(albumart));

   gtk_image_clear(GTK_IMAGE(albumart));
   gtk_image_set_from_pixbuf(GTK_IMAGE(albumart), pixbuf);
}

/**
 * album_art_get_pixbuf:
 *
 */
GdkPixbuf *
rena_album_art_get_pixbuf (RenaAlbumArt *albumart)
{
   GdkPixbuf *pixbuf = NULL;

   g_return_val_if_fail(RENA_IS_ALBUM_ART(albumart), NULL);

   if(gtk_image_get_storage_type(GTK_IMAGE(albumart)) == GTK_IMAGE_PIXBUF)
      pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(albumart));

   return pixbuf;
}

static void
rena_album_art_finalize (GObject *object)
{
   RenaAlbumArt *albumart = RENA_ALBUM_ART(object);

   g_free(albumart->path);

   G_OBJECT_CLASS(rena_album_art_parent_class)->finalize(object);
}

static void
rena_album_art_get_property (GObject *object,
                               guint prop_id,
                               GValue *value,
                               GParamSpec *pspec)
{
   RenaAlbumArt *albumart = RENA_ALBUM_ART(object);

   switch (prop_id) {
   case PROP_PATH:
      g_value_set_string(value, rena_album_art_get_path(albumart));
      break;
   case PROP_SIZE:
      g_value_set_uint (value, rena_album_art_get_size(albumart));
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
   }
}

static void
rena_album_art_set_property (GObject *object,
                               guint prop_id,
                               const GValue *value,
                               GParamSpec *pspec)
{
   RenaAlbumArt *albumart = RENA_ALBUM_ART(object);

   switch (prop_id) {
   case PROP_PATH:
      rena_album_art_set_path(albumart, g_value_get_string(value));
      break;
   case PROP_SIZE:
      rena_album_art_set_size(albumart, g_value_get_uint(value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
   }
}

static void
rena_album_art_class_init (RenaAlbumArtClass *klass)
{
   GObjectClass *object_class;

   object_class = G_OBJECT_CLASS(klass);
   object_class->finalize = rena_album_art_finalize;
   object_class->get_property = rena_album_art_get_property;
   object_class->set_property = rena_album_art_set_property;

   /**
    * RenaAlbumArt:path:
    *
    */
   gParamSpecs[PROP_PATH] =
      g_param_spec_string("path",
                          "Path",
                          "The album art path",
                          NULL,
                          G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS);

   /**
    * RenaAlbumArt:size:
    *
    */
   gParamSpecs[PROP_SIZE] =
      g_param_spec_uint("size",
                        "Size",
                        "The album art size",
                        24, 128,
                        36,
                        G_PARAM_READWRITE |
                        G_PARAM_STATIC_STRINGS);

   g_object_class_install_properties(object_class, LAST_PROP, gParamSpecs);
}

static void
rena_album_art_init (RenaAlbumArt *albumart)
{
}
