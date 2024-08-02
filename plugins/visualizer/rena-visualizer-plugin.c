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

#include <libpeas/peas.h>
#include <libpeas-gtk/peas-gtk.h>

#include "rena-visualizer-plugin.h"

#include "src/rena.h"
#include "src/rena-menubar.h"
#include "src/rena-playback.h"
#include "src/rena-window.h"

#include "plugins/rena-plugin-macros.h"

RENA_PLUGIN_REGISTER (RENA_TYPE_VISUALIZER_PLUGIN,
                        RenaVisualizerPlugin,
                        rena_visualizer_plugin)


/*
 * Menubar Prototypes
 */

static void
visualizer_action (GtkAction *action, RenaVisualizerPlugin *plugin)
{
	GtkWidget *main_stack;
	gboolean visualizer;

	RenaVisualizerPluginPrivate *priv = plugin->priv;

	main_stack = rena_application_get_main_stack (priv->rena);

	visualizer = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));
	if(visualizer) {
		gtk_stack_set_visible_child_name (GTK_STACK(main_stack), "visualizer");
	}
	else {
		gtk_stack_set_visible_child_name (GTK_STACK(main_stack), "playlist");
	}

	/* Sink gear menu and menubar */
	g_simple_action_set_state (priv->gear_action, g_variant_new_boolean (visualizer));
}

static const
GtkToggleActionEntry main_menu_actions [] = {
	{"Visualizer", NULL, N_("_Visualizer"),
	 "<Control>T", "Switch between playlist and visualizer", G_CALLBACK(visualizer_action),
	FALSE}
};

static const
gchar *main_menu_xml = "<ui>						\
	<menubar name=\"Menubar\">							\
		<menu action=\"ViewMenu\">						\
			<placeholder name=\"rena-view-placeholder\">			\
				<menuitem action=\"Visualizer\"/>			\
			</placeholder>							\
		</menu>									\
	</menubar>									\
</ui>";


static void
rena_gmenu_visualizer (GSimpleAction *action,
                         GVariant      *parameter,
                         gpointer       user_data)
{
	GtkAction *gtkaction;

	RenaVisualizerPlugin *plugin = user_data;
	RenaVisualizerPluginPrivate *priv = plugin->priv;

	/* Sink gear menu and menubar. Logic there. */
	gtkaction = gtk_action_group_get_action (priv->action_group_main_menu, "Visualizer");
	gtk_action_activate (gtkaction);
}


/*
 *  Visualizer plugin.
 */
static void
rena_visualizer_plugin_update_spectrum (RenaBackend *backend, gpointer value, gpointer user_data)
{
	GValue *magnitudes = value;

	RenaVisualizerPlugin *plugin = user_data;
	RenaVisualizerPluginPrivate *priv = plugin->priv;

	rena_visualizer_set_magnitudes (priv->visualizer, magnitudes);
}

static void
rena_visualizer_plugin_append_menues (RenaVisualizerPlugin *plugin)
{
	GMenuItem *item;
	GSimpleAction *action;

	RenaVisualizerPluginPrivate *priv = plugin->priv;

	/*
	 * Menubar
	 */
	priv->action_group_main_menu = rena_menubar_plugin_action_new ("RenaVisualizerMainMenuActions",
	                                                                 NULL,
	                                                                 0,
	                                                                 main_menu_actions,
	                                                                 G_N_ELEMENTS (main_menu_actions),
	                                                                 plugin);

	priv->merge_id_main_menu = rena_menubar_append_plugin_action (priv->rena,
	                                                                priv->action_group_main_menu,
	                                                                main_menu_xml);

	/*
	 * Gear Menu
	 */

	action = g_simple_action_new_stateful("visualizer", NULL, g_variant_new_boolean(FALSE));
	g_signal_connect (G_OBJECT (action), "activate",
	                  G_CALLBACK (rena_gmenu_visualizer), plugin);

	item = g_menu_item_new (_("Show Visualizer"), "win.visualizer");
	rena_menubar_append_action (priv->rena, "rena-view-placeholder", action, item);
	g_object_unref (item);

	priv->gear_action = action;
}

static void
rena_visualizer_plugin_remove_menues (RenaVisualizerPlugin *plugin)
{
	RenaVisualizerPluginPrivate *priv = plugin->priv;

	if (!priv->merge_id_main_menu)
		return;

	rena_menubar_remove_plugin_action (priv->rena,
	                                     priv->action_group_main_menu,
	                                     priv->merge_id_main_menu);

	priv->merge_id_main_menu = 0;

	rena_menubar_remove_action (priv->rena, "rena-view-placeholder", "visualizer");
}


static void
rena_plugin_activate (PeasActivatable *activatable)
{
	RenaBackend *backend;
	GtkWidget *main_stack;

	RenaVisualizerPlugin *plugin = RENA_VISUALIZER_PLUGIN (activatable);
	RenaVisualizerPluginPrivate *priv = plugin->priv;
	priv->rena = g_object_get_data (G_OBJECT (plugin), "object");

	CDEBUG(DBG_PLUGIN, "Visualizer plugin %s", G_STRFUNC);

	priv->visualizer = rena_visualizer_new ();

	main_stack = rena_application_get_main_stack (priv->rena);
	gtk_stack_add_named (GTK_STACK(main_stack), GTK_WIDGET(priv->visualizer), "visualizer");

	rena_visualizer_plugin_append_menues (plugin);

	/* Connect signals */
	backend = rena_application_get_backend (priv->rena);
	rena_backend_enable_spectrum (backend);
	g_signal_connect (backend, "spectrum",
	                 G_CALLBACK(rena_visualizer_plugin_update_spectrum), plugin);

	gtk_widget_show_all (GTK_WIDGET(priv->visualizer));
}

static void
rena_plugin_deactivate (PeasActivatable *activatable)
{
	RenaBackend *backend;
	GtkWidget *main_stack;

	RenaVisualizerPlugin *plugin = RENA_VISUALIZER_PLUGIN (activatable);
	RenaVisualizerPluginPrivate *priv = plugin->priv;

	priv->rena = g_object_get_data (G_OBJECT (plugin), "object");

	CDEBUG(DBG_PLUGIN, "Visualizer plugin %s", G_STRFUNC);

	/* Disconnect signals */
	backend = rena_application_get_backend (priv->rena);
	rena_backend_disable_spectrum (backend);
	g_signal_handlers_disconnect_by_func (backend,
	                                      rena_visualizer_plugin_update_spectrum, plugin);

	rena_visualizer_plugin_remove_menues (plugin);

	/* Free Memory */

	main_stack = rena_application_get_main_stack (priv->rena);
	gtk_container_remove (GTK_CONTAINER(main_stack), GTK_WIDGET(priv->visualizer));
}
