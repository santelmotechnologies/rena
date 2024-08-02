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

#include <glib.h>
#include <glib-object.h>
#include <gmodule.h>
#include <gtk/gtk.h>
#include <keybinder.h>
#include <gdk/gdkx.h>

#include <libpeas/peas.h>
#include <libpeas-gtk/peas-gtk.h>

#include "rena-keybinder-plugin.h"

#include "src/rena.h"
#include "src/rena-playback.h"
#include "src/rena-window.h"

#include "plugins/rena-plugin-macros.h"

RENA_PLUGIN_REGISTER (RENA_TYPE_KEYBINDER_PLUGIN,
                        RenaKeybinderPlugin,
                        rena_keybinder_plugin)

static void
keybind_prev_handler (const char *keystring, gpointer data)
{
	RenaBackend *backend;
	RenaApplication *rena = data;

	backend = rena_application_get_backend (rena);

	if (rena_backend_emitted_error (backend) == FALSE)
		rena_playback_prev_track(rena);
}

static void
keybind_play_handler (const char *keystring, gpointer data)
{
	RenaBackend *backend;
	RenaApplication *rena = data;

	backend = rena_application_get_backend (rena);

	if (rena_backend_emitted_error (backend) == FALSE)
		rena_playback_play_pause_resume(rena);
}

static void
keybind_stop_handler (const char *keystring, gpointer data)
{
	RenaBackend *backend;
	RenaApplication *rena = data;

	backend = rena_application_get_backend (rena);

	if (rena_backend_emitted_error (backend) == FALSE)
		rena_playback_stop(rena);
}

static void
keybind_next_handler (const char *keystring, gpointer data)
{
	RenaBackend *backend;
	RenaApplication *rena = data;

	backend = rena_application_get_backend (rena);

	if (rena_backend_emitted_error (backend) == FALSE)
		rena_playback_next_track(rena);
}

static void
keybind_media_handler (const char *keystring, gpointer data)
{
	RenaApplication *rena = data;

	rena_window_toggle_state (rena, FALSE);
}

static void
rena_plugin_activate (PeasActivatable *activatable)
{
	RenaKeybinderPlugin *plugin = RENA_KEYBINDER_PLUGIN (activatable);

	if (!GDK_IS_X11_DISPLAY (gdk_display_get_default ()))
		return;

	RenaKeybinderPluginPrivate *priv = plugin->priv;
	priv->rena = g_object_get_data (G_OBJECT (plugin), "object");

	keybinder_init ();

	CDEBUG(DBG_PLUGIN, "Keybinder plugin %s", G_STRFUNC);

	keybinder_bind("XF86AudioPlay", (KeybinderHandler) keybind_play_handler, priv->rena);
	keybinder_bind("XF86AudioStop", (KeybinderHandler) keybind_stop_handler, priv->rena);
	keybinder_bind("XF86AudioPrev", (KeybinderHandler) keybind_prev_handler, priv->rena);
	keybinder_bind("XF86AudioNext", (KeybinderHandler) keybind_next_handler, priv->rena);
	keybinder_bind("XF86AudioMedia", (KeybinderHandler) keybind_media_handler, priv->rena);
}

static void
rena_plugin_deactivate (PeasActivatable *activatable)
{
	CDEBUG(DBG_PLUGIN, "Keybinder plugin %s", G_STRFUNC);

	if (!GDK_IS_X11_DISPLAY (gdk_display_get_default ()))
		return;

	keybinder_unbind("XF86AudioPlay", (KeybinderHandler) keybind_play_handler);
	keybinder_unbind("XF86AudioStop", (KeybinderHandler) keybind_stop_handler);
	keybinder_unbind("XF86AudioPrev", (KeybinderHandler) keybind_prev_handler);
	keybinder_unbind("XF86AudioNext", (KeybinderHandler) keybind_next_handler);
	keybinder_unbind("XF86AudioMedia", (KeybinderHandler) keybind_media_handler);
}
