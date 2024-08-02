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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef RENA_SONGINFO_PLUGIN_H
#define RENA_SONGINFO_PLUGIN_H

#include <gtk/gtk.h>
#include <glib-object.h>

#include <glyr/cache.h>
#include "rena-song-info-cache.h"
#include "rena-song-info-pane.h"

#include "src/rena.h"

#include "plugins/rena-plugin-macros.h"

G_BEGIN_DECLS

#define RENA_TYPE_SONG_INFO_PLUGIN         (rena_song_info_plugin_get_type ())
#define RENA_SONG_INFO_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), RENA_TYPE_SONG_INFO_PLUGIN, RenaSongInfoPlugin))
#define RENA_SONG_INFO_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), RENA_TYPE_SONG_INFO_PLUGIN, RenaSongInfoPlugin))
#define RENA_IS_SONG_INFO_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), RENA_TYPE_SONG_INFO_PLUGIN))
#define RENA_IS_SONG_INFO_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), RENA_TYPE_SONG_INFO_PLUGIN))
#define RENA_SONG_INFO_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), RENA_TYPE_SONG_INFO_PLUGIN, RenaSongInfoPluginClass))

typedef struct _RenaSongInfoPluginPrivate RenaSongInfoPluginPrivate;

RENA_PLUGIN_REGISTER_PUBLIC_HEADER (RENA_TYPE_SONG_INFO_PLUGIN,
                                      RenaSongInfoPlugin,
                                      rena_song_info_plugin)

RenaApplication  *rena_songinfo_plugin_get_application (RenaSongInfoPlugin *plugin);

RenaInfoCache    *rena_songinfo_plugin_get_cache_info  (RenaSongInfoPlugin *plugin);
GlyrDatabase       *rena_songinfo_plugin_get_cache       (RenaSongInfoPlugin *plugin);
RenaSonginfoPane *rena_songinfo_plugin_get_pane        (RenaSongInfoPlugin *plugin);
void                rena_songinfo_plugin_init_glyr_query (gpointer data);

G_END_DECLS

#endif /* RENA_SONGINFO_PLUGIN_H */
