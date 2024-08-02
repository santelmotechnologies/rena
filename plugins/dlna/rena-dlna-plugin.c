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

#if defined(GETTEXT_PACKAGE)
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif
#include <sys/types.h>
#include <ifaddrs.h>

#include <glib.h>
#include <glib-object.h>
#include <gmodule.h>
#include <gtk/gtk.h>

#include <gio/gio.h>
#include <rygel-server.h>
#include <rygel-core.h>

#include <libpeas/peas.h>
#include <libpeas-gtk/peas-gtk.h>

#include "rena-dlna-plugin.h"

#include "src/rena.h"
#include "src/rena-utils.h"
#include "src/rena-musicobject-mgmt.h"
#include "src/rena-playlist.h"
#include "src/rena-database-provider.h"

#include "plugins/rena-plugin-macros.h"

typedef struct _RenaDlnaPluginPrivate RenaDlnaPluginPrivate;

struct _RenaDlnaPluginPrivate {
	RenaApplication    *rena;

    RygelMediaServer     *server;
    RygelSimpleContainer *container;
};

RENA_PLUGIN_REGISTER (RENA_TYPE_DLNA_PLUGIN,
                        RenaDlnaPlugin,
                        rena_dlna_plugin)

static void
rena_dlna_plugin_append_track (RenaDlnaPlugin  *plugin,
                                 RenaMusicobject *mobj,
                                 gint               id)
{
	RygelMusicItem *item = NULL;
	gchar *uri = NULL, *u_title = NULL, *item_id = NULL, *content_type = NULL;
	const gchar *file = NULL, *title = NULL;
	gboolean uncertain;

	RenaDlnaPluginPrivate *priv = plugin->priv;

	title = rena_musicobject_get_title (mobj);
	u_title = string_is_not_empty(title) ? g_strdup(title) : get_display_name (mobj);

	item_id = g_strdup_printf ("%06d", id);
	item = rygel_music_item_new (item_id,
	                             RYGEL_MEDIA_CONTAINER(priv->container),
	                             u_title,
	                             RYGEL_MUSIC_ITEM_UPNP_CLASS);

	if (item != NULL) {
		file = rena_musicobject_get_file (mobj);

		uri = g_filename_to_uri (file, NULL, NULL);
		rygel_media_object_add_uri (RYGEL_MEDIA_OBJECT (item), uri);
		g_free (uri);

		content_type = g_content_type_guess (file, NULL, 0, &uncertain);
		rygel_media_file_item_set_mime_type (RYGEL_MEDIA_FILE_ITEM (item), content_type);
		g_free(content_type);

		rygel_music_item_set_track_number (item, rena_musicobject_get_track_no(mobj));

		rygel_audio_item_set_album (RYGEL_AUDIO_ITEM(item), rena_musicobject_get_album(mobj));
		rygel_audio_item_set_duration (RYGEL_AUDIO_ITEM(item), (glong)rena_musicobject_get_length(mobj));

		rygel_media_object_set_artist (RYGEL_MEDIA_OBJECT(item), rena_musicobject_get_artist(mobj));

		rygel_simple_container_add_child_item (priv->container, RYGEL_MEDIA_ITEM(item));

		g_object_unref (item);
	}

	g_free(u_title);
	g_free(item_id);
}

static void
rena_dlna_plugin_share_library (RenaDlnaPlugin *plugin)
{
	RenaDatabase *cdbase;
	RenaPreparedStatement *statement;
	RenaMusicobject *mobj;
	gint i = 0;

	const gchar *sql = NULL;

	RenaDlnaPluginPrivate *priv = plugin->priv;

	/* Query and insert entries */

	set_watch_cursor (rena_application_get_window(priv->rena));

	sql = "SELECT location FROM TRACK "
	      "INNER JOIN provider ON track.provider = provider.id "
	      "INNER JOIN provider_type ON provider.type = provider_type.id "
	      "WHERE provider_type.name  = ?";

	cdbase = rena_application_get_database (priv->rena);
	statement = rena_database_create_statement (cdbase, sql);
	rena_prepared_statement_bind_string (statement, 1, "local");

	while (rena_prepared_statement_step (statement)) {
		mobj = new_musicobject_from_db (cdbase,
			rena_prepared_statement_get_int (statement, 0));
		if (G_LIKELY(mobj)) {
			rena_dlna_plugin_append_track (plugin, mobj, i++);
			g_object_unref (mobj);
		}
		rena_process_gtk_events ();
	}
	rena_prepared_statement_free (statement);

	remove_watch_cursor (rena_application_get_window(priv->rena));
}

static void
rena_dlna_plugin_database_changed (RenaDatabaseProvider *provider,
                                     RenaDlnaPlugin       *plugin)
{
	RenaDlnaPluginPrivate *priv = plugin->priv;

	if (TRUE)
		return;

	rygel_simple_container_clear (priv->container);
	rena_dlna_plugin_share_library (plugin);
}

static void
rena_dlna_plugin_share_playlist (RenaDlnaPlugin *plugin)
{
	RenaPlaylist *playlist;
	GList *list = NULL, *i;
	RenaMusicobject *mobj;
	gint id = 0;

	RenaDlnaPluginPrivate *priv = plugin->priv;

	playlist = rena_application_get_playlist (priv->rena);

	set_watch_cursor (rena_application_get_window(priv->rena));

	list = rena_playlist_get_mobj_list (playlist);
	for (i = list; i != NULL; i = i->next) {
		mobj = i->data;

		if (mobj == NULL)
			continue;

		if (rena_musicobject_is_local_file(mobj))
			rena_dlna_plugin_append_track (plugin, mobj, id++);

		rena_process_gtk_events ();
	}
	g_list_free(list);

	remove_watch_cursor (rena_application_get_window(priv->rena));
}

static void
rena_dlna_plugin_playlist_changed (RenaPlaylist   *playlist,
                                     RenaDlnaPlugin *plugin)
{
	RenaDlnaPluginPrivate *priv = plugin->priv;

	if (FALSE)
		return;

	rygel_simple_container_clear (priv->container);
	rena_dlna_plugin_share_playlist (plugin);
}


static void
rena_plugin_activate (PeasActivatable *activatable)
{
	RenaDatabaseProvider *provider;
	RenaPlaylist *playlist;
    GError *error = NULL;
	struct ifaddrs *addrs,*tmp;

	RenaDlnaPlugin *plugin = RENA_DLNA_PLUGIN (activatable);

	RenaDlnaPluginPrivate *priv = plugin->priv;
	priv->rena = g_object_get_data (G_OBJECT (plugin), "object");

	CDEBUG(DBG_PLUGIN, "DLNA plugin %s", G_STRFUNC);

	/* Init rygel */

	rygel_media_engine_init (&error);
	if (error != NULL) {
		g_critical ("Could not initialize media engine: %s\n", error->message);
		g_error_free (error);
	}

	/* Create root container */

	priv->container = rygel_simple_container_new_root (_("Local Music"));

	/* Put initial music on container */

	if (FALSE)
		rena_dlna_plugin_share_library (plugin);
	else
		rena_dlna_plugin_share_playlist (plugin);

	/* Launch server */

	priv->server = rygel_media_server_new (_("Rena Music Player"),
	                                       RYGEL_MEDIA_CONTAINER(priv->container),
	                                       RYGEL_PLUGIN_CAPABILITIES_NONE);

	getifaddrs (&addrs);
	tmp = addrs;
	while (tmp) {
		if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_PACKET)
			rygel_media_device_add_interface (RYGEL_MEDIA_DEVICE (priv->server), tmp->ifa_name);
		tmp = tmp->ifa_next;
	}
	freeifaddrs (addrs);

	/* Connect signals to update music */

	provider = rena_database_provider_get ();
	g_signal_connect (provider, "update-done",
	                  G_CALLBACK(rena_dlna_plugin_database_changed), plugin);
	g_object_unref (provider);

	playlist = rena_application_get_playlist (priv->rena);
	g_signal_connect (playlist, "playlist-changed",
	                  G_CALLBACK(rena_dlna_plugin_playlist_changed), plugin);

}

static void
rena_plugin_deactivate (PeasActivatable *activatable)
{
	RenaDatabaseProvider *provider;
	RenaPlaylist *playlist;

	RenaDlnaPlugin *plugin = RENA_DLNA_PLUGIN (activatable);

	RenaDlnaPluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "DLNA plugin %s", G_STRFUNC);

	provider = rena_database_provider_get ();
	g_signal_handlers_disconnect_by_func (provider,
	                                      rena_dlna_plugin_database_changed,
	                                      plugin);
	g_object_unref (provider);

	playlist = rena_application_get_playlist (priv->rena);
	g_signal_handlers_disconnect_by_func (playlist,
	                                      rena_dlna_plugin_playlist_changed,
	                                      plugin);
	                                      
	rygel_simple_container_clear (priv->container);

	g_object_unref (priv->container);
	g_object_unref (priv->server);
}
