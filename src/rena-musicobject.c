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

#include "rena-musicobject.h"

struct _RenaMusicobjectPrivate
{
	gchar *file;
	RenaMusicSource source;
	gchar *provider;
	gchar *mime_type;
	gchar *title;
	gchar *artist;
	gchar *album;
	gchar *genre;
	gchar *comment;
	guint year;
	guint track_no;
	gint length;
	gint bitrate;
	gint channels;
	gint samplerate;
};

G_DEFINE_TYPE_WITH_PRIVATE(RenaMusicobject, rena_musicobject, G_TYPE_OBJECT)

enum
{
	PROP_0,
	PROP_FILE,
	PROP_SOURCE,
	PROP_PROVIDER,
	PROP_MIME_TYPE,
	PROP_TITLE,
	PROP_ARTIST,
	PROP_ALBUM,
	PROP_GENRE,
	PROP_COMMENT,
	PROP_YEAR,
	PROP_TRACK_NO,
	PROP_LENGTH,
	PROP_BITRATE,
	PROP_CHANNELS,
	PROP_SAMPLERATE,
	LAST_PROP
};

static GParamSpec *gParamSpecs[LAST_PROP];

/**
 * rena_musicobject_new:
 *
 */
RenaMusicobject *
rena_musicobject_new (void)
{
	return g_object_new (RENA_TYPE_MUSICOBJECT, NULL);
}

/**
 * rena_musicobject_dup:
 *
 */
RenaMusicobject *
rena_musicobject_dup (RenaMusicobject *musicobject)
{
	g_return_val_if_fail(RENA_IS_MUSICOBJECT(musicobject), NULL);

	return g_object_new (RENA_TYPE_MUSICOBJECT,
	                     "file", rena_musicobject_get_file(musicobject),
	                     "source", rena_musicobject_get_source (musicobject),
	                     "provider", rena_musicobject_get_provider (musicobject),
	                     "mime-type", rena_musicobject_get_mime_type(musicobject),
	                     "title", rena_musicobject_get_title(musicobject),
	                     "artist", rena_musicobject_get_artist(musicobject),
	                     "album", rena_musicobject_get_album(musicobject),
	                     "genre", rena_musicobject_get_genre(musicobject),
	                     "comment", rena_musicobject_get_comment(musicobject),
	                     "year", rena_musicobject_get_year(musicobject),
	                     "track-no", rena_musicobject_get_track_no(musicobject),
	                     "length", rena_musicobject_get_length(musicobject),
	                     "bitrate", rena_musicobject_get_bitrate(musicobject),
	                     "channels", rena_musicobject_get_channels(musicobject),
	                     "samplerate", rena_musicobject_get_samplerate(musicobject),
	                     NULL);
}

/**
 * rena_musicobject_clean:
 *
 */
void
rena_musicobject_clean (RenaMusicobject *musicobject)
{
	g_return_if_fail(RENA_IS_MUSICOBJECT(musicobject));

	g_object_set (musicobject,
	              "file", "",
	              "source", FILE_NONE,
	              "provider", "",
	              "mime-type", "",
	              "title", "",
	              "artist", "",
	              "album", "",
	              "genre", "",
	              "comment", "",
	              "year", 0,
	              "track-no", 0,
	              "length", 0,
	              "bitrate", 0,
	              "channels", 0,
	              "samplerate", 0,
	              NULL);
}

/**
 * rena_musicobject_compare:
 *
 */
gint
rena_musicobject_compare (RenaMusicobject *a, RenaMusicobject *b)
{
	/* First compare the pointers */
	if(a == b)
		return 0;

	/* Then compare filenames. */
	return g_strcmp0(rena_musicobject_get_file(a),
	                 rena_musicobject_get_file(b));
}

/**
 * rena_musicobject_compare_tags:
 *
 */
gint
rena_musicobject_compare_tags (RenaMusicobject *a, RenaMusicobject *b)
{
	gint diff = 0;

	diff = g_strcmp0(rena_musicobject_get_mime_type(a),
	                 rena_musicobject_get_mime_type(b));
	if (diff) return diff;

	diff = g_strcmp0(rena_musicobject_get_title(a),
	                 rena_musicobject_get_title(b));
	if (diff) return diff;

	diff = g_strcmp0(rena_musicobject_get_artist(a),
	                 rena_musicobject_get_artist(b));
	if (diff) return diff;

	diff = g_strcmp0(rena_musicobject_get_album(a),
	                 rena_musicobject_get_album(b));
	if (diff) return diff;

	diff = g_strcmp0(rena_musicobject_get_genre(a),
	                 rena_musicobject_get_genre(b));
	if (diff) return diff;

	diff = g_strcmp0(rena_musicobject_get_comment(a),
	                 rena_musicobject_get_comment(b));
	if (diff) return diff;

	diff = rena_musicobject_get_year(a) - rena_musicobject_get_year(b);
	if (diff) return diff;

	diff = rena_musicobject_get_track_no(a) - rena_musicobject_get_track_no(b);
	if (diff) return diff;

	diff = rena_musicobject_get_length(a) - rena_musicobject_get_length(b);
	if (diff) return diff;

	diff = rena_musicobject_get_bitrate(a) - rena_musicobject_get_bitrate(b);
	if (diff) return diff;

	diff = rena_musicobject_get_channels(a) - rena_musicobject_get_channels(b);
	if (diff) return diff;

	diff = rena_musicobject_get_samplerate(a) - rena_musicobject_get_samplerate(b);
	if (diff) return diff;

	return diff;
}

/**
 * rena_musicobject_get_file:
 *
 */
const gchar *
rena_musicobject_get_file (RenaMusicobject *musicobject)
{
	g_return_val_if_fail(RENA_IS_MUSICOBJECT(musicobject), NULL);

	return musicobject->priv->file;
}

/**
 * rena_musicobject_set_file:
 *
 */
void
rena_musicobject_set_file (RenaMusicobject *musicobject,
                             const gchar *file)
{
	RenaMusicobjectPrivate *priv;

	g_return_if_fail(RENA_IS_MUSICOBJECT(musicobject));

	priv = musicobject->priv;

	g_free(priv->file);
	priv->file = g_strdup(file);
}

/**
 * rena_musicobject_is_local_file:
 *
 */
gboolean
rena_musicobject_is_local_file (RenaMusicobject *musicobject)
{
	g_return_val_if_fail(RENA_IS_MUSICOBJECT(musicobject), FALSE);

	RenaMusicSource source = musicobject->priv->source;

	return (source == FILE_LOCAL);
}

/**
 * rena_musicobject_get_source:
 *
 */
RenaMusicSource
rena_musicobject_get_source (RenaMusicobject *musicobject)
{
	g_return_val_if_fail(RENA_IS_MUSICOBJECT(musicobject), FILE_NONE);

	return musicobject->priv->source;
}
/**
 * rena_musicobject_set_source:
 *
 */
void
rena_musicobject_set_source (RenaMusicobject *musicobject,
                               RenaMusicSource  source)
{
	RenaMusicobjectPrivate *priv;

	g_return_if_fail(RENA_IS_MUSICOBJECT(musicobject));

	priv = musicobject->priv;

	priv->source = source;
}

/**
 * rena_musicobject_get_provider:
 *
 */
const gchar *
rena_musicobject_get_provider (RenaMusicobject *musicobject)
{
	g_return_val_if_fail(RENA_IS_MUSICOBJECT(musicobject), NULL);

	return musicobject->priv->provider;
}
/**
 * rena_musicobject_set_provider:
 *
 */
void
rena_musicobject_set_provider (RenaMusicobject *musicobject,
                                 const gchar       *provider)
{
	RenaMusicobjectPrivate *priv;

	g_return_if_fail(RENA_IS_MUSICOBJECT(musicobject));

	priv = musicobject->priv;

	g_free(priv->provider);
	priv->provider = g_strdup(provider);
}


/**
 * rena_musicobject_get_mime_type:
 *
 */
const gchar *
rena_musicobject_get_mime_type (RenaMusicobject *musicobject)
{
	g_return_val_if_fail(RENA_IS_MUSICOBJECT(musicobject), NULL);

	return musicobject->priv->mime_type;
}
/**
 * rena_musicobject_set_mime_type:
 *
 */
void
rena_musicobject_set_mime_type (RenaMusicobject *musicobject,
                                  const gchar       *mime_type)
{
	RenaMusicobjectPrivate *priv;

	g_return_if_fail(RENA_IS_MUSICOBJECT(musicobject));

	priv = musicobject->priv;

	g_free (priv->mime_type);
	priv->mime_type = g_strdup(mime_type);
}

/**
 * rena_musicobject_get_title:
 *
 */
const gchar *
rena_musicobject_get_title (RenaMusicobject *musicobject)
{
	g_return_val_if_fail(RENA_IS_MUSICOBJECT(musicobject), NULL);

	return musicobject->priv->title;
}
/**
 * rena_musicobject_set_title:
 *
 */
void
rena_musicobject_set_title (RenaMusicobject *musicobject,
                              const gchar *title)
{
	RenaMusicobjectPrivate *priv;

	g_return_if_fail(RENA_IS_MUSICOBJECT(musicobject));

	priv = musicobject->priv;

	g_free(priv->title);
	priv->title = g_strdup(title);
}

/**
 * rena_musicobject_get_artist:
 *
 */
const gchar *
rena_musicobject_get_artist (RenaMusicobject *musicobject)
{
	g_return_val_if_fail(RENA_IS_MUSICOBJECT(musicobject), NULL);

	return musicobject->priv->artist;
}
/**
 * rena_musicobject_set_artist:
 *
 */
void
rena_musicobject_set_artist (RenaMusicobject *musicobject,
                               const gchar *artist)
{
	RenaMusicobjectPrivate *priv;

	g_return_if_fail(RENA_IS_MUSICOBJECT(musicobject));

	priv = musicobject->priv;

	g_free(priv->artist);
	priv->artist = g_strdup(artist);
}

/**
 * rena_musicobject_get_album:
 *
 */
const gchar *
rena_musicobject_get_album (RenaMusicobject *musicobject)
{
	g_return_val_if_fail(RENA_IS_MUSICOBJECT(musicobject), NULL);

	return musicobject->priv->album;
}
/**
 * rena_musicobject_set_album:
 *
 */
void
rena_musicobject_set_album (RenaMusicobject *musicobject,
                              const gchar *album)
{
	RenaMusicobjectPrivate *priv;

	g_return_if_fail(RENA_IS_MUSICOBJECT(musicobject));

	priv = musicobject->priv;

	g_free(priv->album);
	priv->album = g_strdup(album);
}

/**
 * rena_musicobject_get_genre:
 *
 */
const gchar *
rena_musicobject_get_genre (RenaMusicobject *musicobject)
{
	g_return_val_if_fail(RENA_IS_MUSICOBJECT(musicobject), NULL);

	return musicobject->priv->genre;
}
/**
 * rena_musicobject_set_genre:
 *
 */
void
rena_musicobject_set_genre (RenaMusicobject *musicobject,
                              const gchar *genre)
{
	RenaMusicobjectPrivate *priv;

	g_return_if_fail(RENA_IS_MUSICOBJECT(musicobject));

	priv = musicobject->priv;

	g_free(priv->genre);
	priv->genre = g_strdup(genre);
}

/**
 * rena_musicobject_get_comment:
 *
 */
const gchar *
rena_musicobject_get_comment (RenaMusicobject *musicobject)
{
	g_return_val_if_fail(RENA_IS_MUSICOBJECT(musicobject), NULL);

	return musicobject->priv->comment;
}
/**
 * rena_musicobject_set_comment:
 *
 */
void
rena_musicobject_set_comment (RenaMusicobject *musicobject,
                                const gchar *comment)
{
	RenaMusicobjectPrivate *priv;

	g_return_if_fail(RENA_IS_MUSICOBJECT(musicobject));

	priv = musicobject->priv;

	g_free(priv->comment);
	priv->comment = g_strdup(comment);
}

/**
 * rena_musicobject_get_year:
 *
 */
guint
rena_musicobject_get_year (RenaMusicobject *musicobject)
{
	g_return_val_if_fail(RENA_IS_MUSICOBJECT(musicobject), 0);

	return musicobject->priv->year;
}
/**
 * rena_musicobject_set_year:
 *
 */
void
rena_musicobject_set_year (RenaMusicobject *musicobject,
                             guint year)
{
	RenaMusicobjectPrivate *priv;

	g_return_if_fail(RENA_IS_MUSICOBJECT(musicobject));

	priv = musicobject->priv;

	priv->year = year;
}

/**
 * rena_musicobject_get_track_no:
 *
 */
guint
rena_musicobject_get_track_no (RenaMusicobject *musicobject)
{
	g_return_val_if_fail(RENA_IS_MUSICOBJECT(musicobject), 0);

	return musicobject->priv->track_no;
}
/**
 * rena_musicobject_set_track_no:
 *
 */
void
rena_musicobject_set_track_no (RenaMusicobject *musicobject,
                                 guint track_no)
{
	RenaMusicobjectPrivate *priv;

	g_return_if_fail(RENA_IS_MUSICOBJECT(musicobject));

	priv = musicobject->priv;

	priv->track_no = track_no;
}

/**
 * rena_musicobject_get_length:
 *
 */
gint
rena_musicobject_get_length (RenaMusicobject *musicobject)
{
	g_return_val_if_fail(RENA_IS_MUSICOBJECT(musicobject), 0);

	return musicobject->priv->length;
}
/**
 * rena_musicobject_set_length:
 *
 */
void
rena_musicobject_set_length (RenaMusicobject *musicobject,
                               gint length)
{
	RenaMusicobjectPrivate *priv;

	g_return_if_fail(RENA_IS_MUSICOBJECT(musicobject));

	priv = musicobject->priv;

	priv->length = length;
}

/**
 * rena_musicobject_get_bitrate:
 *
 */
gint
rena_musicobject_get_bitrate (RenaMusicobject *musicobject)
{
	g_return_val_if_fail(RENA_IS_MUSICOBJECT(musicobject), 0);

	return musicobject->priv->bitrate;
}
/**
 * rena_musicobject_set_bitrate:
 *
 */
void
rena_musicobject_set_bitrate (RenaMusicobject *musicobject,
                                gint bitrate)
{
	RenaMusicobjectPrivate *priv;

	g_return_if_fail(RENA_IS_MUSICOBJECT(musicobject));

	priv = musicobject->priv;

	priv->bitrate = bitrate;
}

/**
 * rena_musicobject_get_channels:
 *
 */
gint
rena_musicobject_get_channels (RenaMusicobject *musicobject)
{
	g_return_val_if_fail(RENA_IS_MUSICOBJECT(musicobject), 0);

	return musicobject->priv->channels;
}
/**
 * rena_musicobject_set_channels:
 *
 */
void
rena_musicobject_set_channels (RenaMusicobject *musicobject,
                                 gint channels)
{
	RenaMusicobjectPrivate *priv;

	g_return_if_fail(RENA_IS_MUSICOBJECT(musicobject));

	priv = musicobject->priv;

	priv->channels = channels;
}

/**
 * rena_musicobject_get_samplerate:
 *
 */
gint
rena_musicobject_get_samplerate (RenaMusicobject *musicobject)
{
	g_return_val_if_fail(RENA_IS_MUSICOBJECT(musicobject), 0);

	return musicobject->priv->samplerate;
}
/**
 * rena_musicobject_set_samplerate:
 *
 */
void
rena_musicobject_set_samplerate (RenaMusicobject *musicobject,
                                   gint samplerate)
{
	RenaMusicobjectPrivate *priv;

	g_return_if_fail(RENA_IS_MUSICOBJECT(musicobject));

	priv = musicobject->priv;

	priv->samplerate = samplerate;
}

static void
rena_musicobject_finalize (GObject *object)
{
	RenaMusicobjectPrivate *priv;

	priv = RENA_MUSICOBJECT(object)->priv;

	g_free(priv->file);
	g_free(priv->mime_type);
	g_free(priv->provider);
	g_free(priv->title);
	g_free(priv->artist);
	g_free(priv->album);
	g_free(priv->genre);
	g_free(priv->comment);

	G_OBJECT_CLASS(rena_musicobject_parent_class)->finalize(object);
}

static void
rena_musicobject_get_property (GObject *object,
                                 guint prop_id,
                                 GValue *value,
                                 GParamSpec *pspec)
{
	RenaMusicobject *musicobject = RENA_MUSICOBJECT(object);

	switch (prop_id) {
	case PROP_FILE:
		g_value_set_string (value, rena_musicobject_get_file(musicobject));
		break;
	case PROP_SOURCE:
		g_value_set_int(value, rena_musicobject_get_source(musicobject));
		break;
	case PROP_PROVIDER:
		g_value_set_string (value, rena_musicobject_get_provider(musicobject));
		break;
	case PROP_MIME_TYPE:
		g_value_set_string (value, rena_musicobject_get_mime_type(musicobject));
		break;
	case PROP_TITLE:
		g_value_set_string (value, rena_musicobject_get_title(musicobject));
		break;
	case PROP_ARTIST:
		g_value_set_string (value, rena_musicobject_get_artist(musicobject));
		break;
	case PROP_ALBUM:
		g_value_set_string (value, rena_musicobject_get_album(musicobject));
		break;
	case PROP_GENRE:
		g_value_set_string (value, rena_musicobject_get_genre(musicobject));
		break;
	case PROP_COMMENT:
		g_value_set_string (value, rena_musicobject_get_comment(musicobject));
		break;
	case PROP_YEAR:
		g_value_set_uint (value, rena_musicobject_get_year(musicobject));
		break;
	case PROP_TRACK_NO:
		g_value_set_uint (value, rena_musicobject_get_track_no(musicobject));
		break;
	case PROP_LENGTH:
		g_value_set_int(value, rena_musicobject_get_length(musicobject));
		break;
	case PROP_BITRATE:
		g_value_set_int(value, rena_musicobject_get_bitrate(musicobject));
		break;
	case PROP_CHANNELS:
		g_value_set_int(value, rena_musicobject_get_channels(musicobject));
		break;
	case PROP_SAMPLERATE:
		g_value_set_int(value, rena_musicobject_get_samplerate(musicobject));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
rena_musicobject_set_property (GObject *object,
                                 guint prop_id,
                                 const GValue *value,
                                 GParamSpec *pspec)
{
	RenaMusicobject *musicobject = RENA_MUSICOBJECT(object);

	switch (prop_id) {
	case PROP_FILE:
		rena_musicobject_set_file(musicobject, g_value_get_string(value));
		break;
	case PROP_SOURCE:
		rena_musicobject_set_source(musicobject, g_value_get_int(value));
		break;
	case PROP_PROVIDER:
		rena_musicobject_set_provider(musicobject, g_value_get_string(value));
		break;
	case PROP_MIME_TYPE:
		rena_musicobject_set_mime_type(musicobject, g_value_get_string(value));
		break;
	case PROP_TITLE:
		rena_musicobject_set_title(musicobject, g_value_get_string(value));
		break;
	case PROP_ARTIST:
		rena_musicobject_set_artist(musicobject, g_value_get_string(value));
		break;
	case PROP_ALBUM:
		rena_musicobject_set_album(musicobject, g_value_get_string(value));
		break;
	case PROP_GENRE:
		rena_musicobject_set_genre(musicobject, g_value_get_string(value));
		break;
	case PROP_COMMENT:
		rena_musicobject_set_comment(musicobject, g_value_get_string(value));
		break;
	case PROP_YEAR:
		rena_musicobject_set_year(musicobject, g_value_get_uint(value));
		break;
	case PROP_TRACK_NO:
		rena_musicobject_set_track_no(musicobject, g_value_get_uint(value));
		break;
	case PROP_LENGTH:
		rena_musicobject_set_length(musicobject, g_value_get_int(value));
		break;
	case PROP_BITRATE:
		rena_musicobject_set_bitrate(musicobject, g_value_get_int(value));
		break;
	case PROP_CHANNELS:
		rena_musicobject_set_channels(musicobject, g_value_get_int(value));
		break;
	case PROP_SAMPLERATE:
		rena_musicobject_set_samplerate(musicobject, g_value_get_int(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
   }
}

static void
rena_musicobject_class_init (RenaMusicobjectClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = rena_musicobject_finalize;
	object_class->get_property = rena_musicobject_get_property;
	object_class->set_property = rena_musicobject_set_property;

	/**
	  * RenaMusicobject:file:
	  *
	  */
	gParamSpecs[PROP_FILE] =
		g_param_spec_string("file",
		                    "File",
		                    "The File",
		                    "",
		                    RENA_MUSICOBJECT_PARAM_STRING);

	/**
	  * RenaMusicobject:source:
	  *
	  */
	gParamSpecs[PROP_SOURCE] =
		g_param_spec_int ("source",
		                  "Source",
		                  "Source of file",
		                  FILE_HTTP,
		                  FILE_USER_L,
		                  FILE_NONE,
		                  G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	  * RenaMusicobject:provider:
	  *
	  */
	gParamSpecs[PROP_PROVIDER] =
		g_param_spec_string("provider",
		                    "Provider",
		                    "The Provider",
		                    "",
		                    RENA_MUSICOBJECT_PARAM_STRING);

	/**
	  * RenaMusicobject:mime_type:
	  *
	  */
	gParamSpecs[PROP_MIME_TYPE] =
		g_param_spec_string("mime-type",
		                    "MimeType",
		                    "The MimeType",
		                    "",
		                    RENA_MUSICOBJECT_PARAM_STRING);

	/**
	  * RenaMusicobject:title:
	  *
	  */
	gParamSpecs[PROP_TITLE] =
		g_param_spec_string("title",
		                    "Title",
		                    "The Title",
		                    "",
		                    RENA_MUSICOBJECT_PARAM_STRING);

	/**
	  * RenaMusicobject:artist:
	  *
	  */
	gParamSpecs[PROP_ARTIST] =
		g_param_spec_string("artist",
		                    "Artist",
		                    "The Artist",
		                    "",
		                    RENA_MUSICOBJECT_PARAM_STRING);

	/**
	  * RenaMusicobject:album:
	  *
	  */
	gParamSpecs[PROP_ALBUM] =
		g_param_spec_string("album",
		                    "Album",
		                    "The Album",
		                    "",
		                    RENA_MUSICOBJECT_PARAM_STRING);

	/**
	  * RenaMusicobject:genre:
	  *
	  */
	gParamSpecs[PROP_GENRE] =
		g_param_spec_string("genre",
		                    "Genre",
		                    "The Genre",
		                    "",
		                    RENA_MUSICOBJECT_PARAM_STRING);

	/**
	  * RenaMusicobject:comment:
	  *
	  */
	gParamSpecs[PROP_COMMENT] =
		g_param_spec_string("comment",
		                    "Comment",
		                    "The Comment",
		                    "",
		                    RENA_MUSICOBJECT_PARAM_STRING);

	/**
	  * RenaMusicobject:year:
	  *
	  */
	gParamSpecs[PROP_YEAR] =
		g_param_spec_uint ("year",
		                   "Year",
		                   "The Year",
		                   0,
		                   G_MAXUINT,
		                   0,
		                   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	  * RenaMusicobject:track_no:
	  *
	  */
	gParamSpecs[PROP_TRACK_NO] =
		g_param_spec_uint ("track-no",
		                   "TrackNo",
		                   "The Track No",
		                   0,
		                   G_MAXUINT,
		                   0,
		                   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	  * RenaMusicobject:length:
	  *
	  */
	gParamSpecs[PROP_LENGTH] =
		g_param_spec_int ("length",
		                  "Length",
		                  "The Length",
		                  0,
		                  G_MAXINT,
		                  0,
		                  G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	  * RenaMusicobject:bitrate:
	  *
	  */
	gParamSpecs[PROP_BITRATE] =
		g_param_spec_int ("bitrate",
		                  "Bitrate",
		                  "The Bitrate",
		                  0,
		                  G_MAXINT,
		                  0,
		                  G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	  * RenaMusicobject:channels:
	  *
	  */
	gParamSpecs[PROP_CHANNELS] =
		g_param_spec_int ("channels",
		                  "Channels",
		                  "The Channels",
		                  0,
		                  G_MAXINT,
		                  0,
		                  G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	  * RenaMusicobject:samplerate:
	  *
	  */
	gParamSpecs[PROP_SAMPLERATE] =
		g_param_spec_int ("samplerate",
		                  "Samplerate",
		                  "The Samplerate",
		                  0,
		                  G_MAXINT,
		                  0,
		                  G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(object_class, LAST_PROP, gParamSpecs);
}

static void
rena_musicobject_init (RenaMusicobject *musicobject)
{
   musicobject->priv = G_TYPE_INSTANCE_GET_PRIVATE(musicobject,
                                                   RENA_TYPE_MUSICOBJECT,
                                                   RenaMusicobjectPrivate);
}
