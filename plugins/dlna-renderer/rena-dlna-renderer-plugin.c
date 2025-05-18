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

#include <glib.h>
#include <glib-object.h>
#include <gmodule.h>
#include <gtk/gtk.h>

#include <gio/gio.h>

#include <libpeas/peas.h>
#include <libpeas-gtk/peas-gtk.h>

#include <grilo.h>

#include "rena-dlna-renderer-plugin.h"

#include "src/rena.h"
#include "src/rena-app-notification.h"
#include "src/rena-utils.h"
#include "src/rena-musicobject-mgmt.h"
#include "src/rena-playlist.h"
#include "src/rena-menubar.h"
#include "src/rena-musicobject.h"
#include "src/rena-musicobject-mgmt.h"
#include "src/rena-window.h"

#include "plugins/rena-plugin-macros.h"

typedef struct _RenaDlnaRendererPluginPrivate RenaDlnaRendererPluginPrivate;

struct _RenaDlnaRendererPluginPrivate {
	RenaApplication    *rena;

	GtkActionGroup       *action_group_main_menu;
	guint                 merge_id_main_menu;
};

RENA_PLUGIN_REGISTER (RENA_TYPE_DLNA_RENDERER_PLUGIN,
                        RenaDlnaRendererPlugin,
                        rena_dlna_renderer_plugin)
/*
 *
 */
static void
rena_dlna_renderer_plugin_search_music (RenaDlnaRendererPlugin *plugin);

/*
 * Popups
 */
static void
rena_dlna_renderer_plugin_search_music_action (GtkAction *action, RenaDlnaRendererPlugin *plugin)
{
	rena_dlna_renderer_plugin_search_music (plugin);
}

static void
rena_gmenu_dlna_renderer_plugin_search_music_action (GSimpleAction *action,
                                                       GVariant      *parameter,
                                                       gpointer       user_data)
{
	rena_dlna_renderer_plugin_search_music (RENA_DLNA_RENDERER_PLUGIN(user_data));
}

static const GtkActionEntry main_menu_actions [] = {
	{"Search dlna music", NULL, N_("Search music on DLNA server"),
	 "", "Search dlna music", G_CALLBACK(rena_dlna_renderer_plugin_search_music_action)}
};

static const gchar *main_menu_xml = "<ui>						\
	<menubar name=\"Menubar\">									\
		<menu action=\"ToolsMenu\">								\
			<placeholder name=\"rena-plugins-placeholder\">	\
				<menuitem action=\"Search dlna music\"/>		\
				<separator/>									\
			</placeholder>										\
		</menu>													\
	</menubar>													\
</ui>";

static GList *
rena_dlna_renderer_append_media (GList *list, GrlMedia *media)
{
	RenaMusicobject *mobj;
	const gchar *title = NULL, *url = NULL;
	guint seconds = 0;

	url = grl_media_get_url (media);
	title = grl_media_get_title (media);
	seconds = grl_media_get_duration (media);

	mobj = g_object_new (RENA_TYPE_MUSICOBJECT,
	                     "file", url,
	                     "source", FILE_HTTP,
	                     "title", title,
	                     "length", seconds,
	                     NULL);

	if (G_LIKELY(mobj))
		list = g_list_prepend (list, mobj);

	return list;
}

static GList *
rena_dlna_renderer_append_source (GList     *list,
                                    GrlSource *source,
                                    GrlMedia  *container)
{
	GrlOperationOptions *options;
	GrlCaps *caps;
	GrlMedia *media;
	GList *keys = NULL;
	GList *medias = NULL, *media_iter;

	keys = grl_metadata_key_list_new (GRL_METADATA_KEY_TITLE,
	                                  GRL_METADATA_KEY_DURATION,
	                                  GRL_METADATA_KEY_URL,
	                                  GRL_METADATA_KEY_CHILDCOUNT,
	                                  GRL_METADATA_KEY_INVALID);

	caps = grl_source_get_caps (source, GRL_OP_BROWSE);
	options = grl_operation_options_new (caps);

#ifdef HAVE_GRILO3
	grl_operation_options_set_resolution_flags (options, GRL_RESOLVE_IDLE_RELAY);
#endif
#ifdef HAVE_GRILO2
	grl_operation_options_set_flags (options, GRL_RESOLVE_IDLE_RELAY);
#endif

	medias = grl_source_browse_sync (source, container, keys, options, NULL);
	for (media_iter = medias; media_iter; media_iter = g_list_next (media_iter)) {
		if (media_iter->data == NULL)
			continue;

		media = GRL_MEDIA (media_iter->data);

#ifdef HAVE_GRILO3
		if (grl_media_is_container (media)) {
#endif
#ifdef HAVE_GRILO2
		if (GRL_IS_MEDIA_BOX (media)) {
#endif
			list = rena_dlna_renderer_append_source (list, source, media);
		}
#ifdef HAVE_GRILO3
		else if (grl_media_is_audio (media)) {
#endif
#ifdef HAVE_GRILO2
		else if (GRL_IS_MEDIA_AUDIO (media)) {
#endif
			list = rena_dlna_renderer_append_media (list, media);
		}
		rena_process_gtk_events ();

		g_object_unref (media);
	}

	g_object_unref (options);

	g_list_free (keys);
	g_list_free (medias);

	return list;
}

static void
rena_dlna_renderer_plugin_search_music (RenaDlnaRendererPlugin *plugin)
{
	RenaAppNotification *notification;
	RenaPlaylist *playlist;
	GList *sources = NULL, *sources_iter;
	GrlRegistry *registry;
	GList *list = NULL;

	CDEBUG(DBG_PLUGIN, "DLNA Renderer plugin %s", G_STRFUNC);

	registry = grl_registry_get_default ();

	sources = grl_registry_get_sources_by_operations (registry, GRL_OP_BROWSE, FALSE);
	for (sources_iter = sources; sources_iter; sources_iter = g_list_next (sources_iter)) {
		list = rena_dlna_renderer_append_source (list, GRL_SOURCE(sources_iter->data), NULL);
		if (list)
			break;
	}

	if (list) {
		playlist = rena_application_get_playlist (plugin->priv->rena);

		rena_playlist_append_mobj_list (playlist, list);
		g_list_free (list);

		const gchar *server = grl_source_get_name (GRL_SOURCE(sources_iter->data));
		gchar *msge = g_strdup_printf (_("Music of the %s server was added."), server);
		notification = rena_app_notification_new (_("Search music on DLNA server"), msge);
		rena_app_notification_show (notification);
		g_free (msge);
	}
	else {
		notification = rena_app_notification_new (_("Search music on DLNA server"), _("Could not find any DLNA server."));
		rena_app_notification_show (notification);
	}

	g_list_free (sources);
}

/*
 * Plugin.
 */
static void
rena_plugin_activate (PeasActivatable *activatable)
{
	GrlRegistry *registry;
	GMenuItem *item;
	GSimpleAction *action;
	GError *error = NULL;

	RenaDlnaRendererPlugin *plugin = RENA_DLNA_RENDERER_PLUGIN (activatable);

	RenaDlnaRendererPluginPrivate *priv = plugin->priv;
	priv->rena = g_object_get_data (G_OBJECT (plugin), "object");

	CDEBUG(DBG_PLUGIN, "DLNA Renderer plugin %s", G_STRFUNC);

	grl_init (NULL, NULL);

	registry = grl_registry_get_default ();
#ifdef HAVE_GRILO3
	if (!grl_registry_load_all_plugins (registry, FALSE, &error)) {
		g_warning ("Failed to load plugins: %s\n\n", error->message);
		g_clear_error (&error);
	}
	if (!grl_registry_activate_plugin_by_id (registry, "grl-dleyna", &error)) {
		g_warning ("Failed to activate dleyna plugin: %s\n\n", error->message);
		g_clear_error (&error);
	}
#endif
#ifdef HAVE_GRILO2
	if (!grl_registry_load_plugin_by_id (registry, "grl-dleyna", &error)) {
		g_warning ("Failed to load plugins: %s\n\n", error->message);
		g_clear_error (&error);
	}
#endif

	/* Attach main menu */

	priv->action_group_main_menu = gtk_action_group_new ("RenaDlnaPlugin");
	gtk_action_group_set_translation_domain (priv->action_group_main_menu, GETTEXT_PACKAGE);
	gtk_action_group_add_actions (priv->action_group_main_menu,
	                              main_menu_actions,
	                              G_N_ELEMENTS (main_menu_actions),
	                              plugin);

	priv->merge_id_main_menu = rena_menubar_append_plugin_action (priv->rena,
	                                                                priv->action_group_main_menu,
	                                                                main_menu_xml);

	/* Gear Menu */

	action = g_simple_action_new ("search-dlna", NULL);
	g_signal_connect (G_OBJECT (action), "activate",
	                  G_CALLBACK (rena_gmenu_dlna_renderer_plugin_search_music_action), plugin);

	item = g_menu_item_new (_("Search music on DLNA server"), "win.search-dlna");
	rena_menubar_append_action (priv->rena, "rena-plugins-placeholder", action, item);
	g_object_unref (item);
}

static void
rena_plugin_deactivate (PeasActivatable *activatable)
{
	RenaDlnaRendererPlugin *plugin = RENA_DLNA_RENDERER_PLUGIN (activatable);

	RenaDlnaRendererPluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "DLNA Renderer plugin %s", G_STRFUNC);

	rena_menubar_remove_plugin_action (priv->rena,
	                                     priv->action_group_main_menu,
	                                     priv->merge_id_main_menu);
	priv->merge_id_main_menu = 0;

	rena_menubar_remove_action (priv->rena, "rena-plugins-placeholder", "search-dlna");

	grl_deinit ();
}
