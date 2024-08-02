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

#ifndef RENA_MUSICOBJECT_H
#define RENA_MUSICOBJECT_H

#include <glib-object.h>

G_BEGIN_DECLS

GType rena_musicobject_get_type (void) G_GNUC_CONST;
#define RENA_TYPE_MUSICOBJECT (rena_musicobject_get_type())
#define RENA_MUSICOBJECT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_MUSICOBJECT, RenaMusicobject))
#define RENA_MUSICOBJECT_CONST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_MUSICOBJECT, RenaMusicobject const))
#define RENA_MUSICOBJECT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), RENA_TYPE_MUSICOBJECT, RenaMusicobjectClass))
#define RENA_IS_MUSICOBJECT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_MUSICOBJECT))
#define RENA_IS_MUSICOBJECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), RENA_TYPE_MUSICOBJECT))
#define RENA_MUSICOBJECT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), RENA_TYPE_MUSICOBJECT, RenaMusicobjectClass))

typedef struct _RenaMusicobject RenaMusicobject;
typedef struct _RenaMusicobjectClass RenaMusicobjectClass;
typedef struct _RenaMusicobjectPrivate RenaMusicobjectPrivate;

struct _RenaMusicobject
{
   GObject parent;

   /*< private >*/
   RenaMusicobjectPrivate *priv;
};

struct _RenaMusicobjectClass
{
   GObjectClass parent_class;
};

/* File music types */

typedef enum {
	FILE_USER_L    =  4,
	FILE_USER_3    =  3,
	FILE_USER_2    =  2,
	FILE_USER_1    =  1,
	FILE_USER_0    =  0,
	FILE_NONE      = -1,
	FILE_LOCAL     = -2,
	FILE_HTTP      = -3
} RenaMusicSource;

#define RENA_MUSICOBJECT_PARAM_STRING G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS

RenaMusicobject *
rena_musicobject_new (void);

RenaMusicobject *
rena_musicobject_dup (RenaMusicobject *musicobject);
void
rena_musicobject_clean (RenaMusicobject *musicobject);
gint
rena_musicobject_compare (RenaMusicobject *a, RenaMusicobject *b);
gint
rena_musicobject_compare_tags (RenaMusicobject *a, RenaMusicobject *b);

const gchar *
rena_musicobject_get_file (RenaMusicobject *musicobject);
void
rena_musicobject_set_file (RenaMusicobject *musicobject,
                             const gchar *file);

gboolean
rena_musicobject_is_local_file (RenaMusicobject *musicobject);

RenaMusicSource
rena_musicobject_get_source (RenaMusicobject *musicobject);
void
rena_musicobject_set_source (RenaMusicobject *musicobject,
                               RenaMusicSource  source);

const gchar *
rena_musicobject_get_provider (RenaMusicobject *musicobject);
void
rena_musicobject_set_provider (RenaMusicobject *musicobject,
                                 const gchar       *provider);

const gchar *
rena_musicobject_get_mime_type (RenaMusicobject *musicobject);
void
rena_musicobject_set_mime_type (RenaMusicobject *musicobject,
                                  const gchar       *mime_type);

const gchar *
rena_musicobject_get_title (RenaMusicobject *musicobject);
void
rena_musicobject_set_title (RenaMusicobject *musicobject,
                              const gchar *title);

const gchar *
rena_musicobject_get_artist (RenaMusicobject *musicobject);
void
rena_musicobject_set_artist (RenaMusicobject *musicobject,
                               const gchar *artist);

const gchar *
rena_musicobject_get_album (RenaMusicobject *musicobject);
void
rena_musicobject_set_album (RenaMusicobject *musicobject,
                              const gchar *album);

const gchar *
rena_musicobject_get_genre (RenaMusicobject *musicobject);
void
rena_musicobject_set_genre (RenaMusicobject *musicobject,
                              const gchar *genre);

const gchar *
rena_musicobject_get_comment (RenaMusicobject *musicobject);
void
rena_musicobject_set_comment (RenaMusicobject *musicobject,
                                const gchar *comment);

guint
rena_musicobject_get_year (RenaMusicobject *musicobject);
void
rena_musicobject_set_year (RenaMusicobject *musicobject,
                             guint year);

guint
rena_musicobject_get_track_no (RenaMusicobject *musicobject);
void
rena_musicobject_set_track_no (RenaMusicobject *musicobject,
                                 guint track_no);

gint
rena_musicobject_get_length (RenaMusicobject *musicobject);
void
rena_musicobject_set_length (RenaMusicobject *musicobject,
                               gint length);

gint
rena_musicobject_get_bitrate (RenaMusicobject *musicobject);
void
rena_musicobject_set_bitrate (RenaMusicobject *musicobject,
                                gint bitrate);

gint
rena_musicobject_get_channels (RenaMusicobject *musicobject);
void
rena_musicobject_set_channels (RenaMusicobject *musicobject,
                                 gint channels);

gint
rena_musicobject_get_samplerate (RenaMusicobject *musicobject);
void
rena_musicobject_set_samplerate (RenaMusicobject *musicobject,
                                   gint samplerate);
G_END_DECLS

#endif /* RENA_MUSICOBJECT_H */
