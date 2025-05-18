/*************************************************************************/
/* Copyright (C) 2007 Jan Arne Petersen <jap@gnome.org>                  */
/* Copyright (C) 2012-2013 Pavel Vasin                                   */
/* This program is free software: you can redistribute it and/or modify  */
/* it under the terms of the GNU General Public License as published by  */
/* the Free Software Foundation, either version 3 of the License, or     */
/* (at your option) any later version.                                   */
/*                                                                       */
/* This program is distributed in the hope that it will be useful,       */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         */
/* GNU General Public License for more details.                          */
/*                                                                       */
/* You should have received a copy of the GNU General Public License     */
/* along with this program.  If not, see <http://www.gnu.org/licenses/>. */
/*************************************************************************/

#ifndef __RENA_GNOME_MEDIA_KEYS_PLUGIN_H__
#define __RENA_GNOME_MEDIA_KEYS_PLUGIN_H__

#include <gtk/gtk.h>
#include <libpeas/peas.h>

#include "src/rena.h"

G_BEGIN_DECLS

#define RENA_TYPE_GNOME_MEDIA_KEYS_PLUGIN         (rena_gnome_media_keys_plugin_get_type ())
#define RENA_GNOME_MEDIA_KEYS_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), RENA_TYPE_GNOME_MEDIA_KEYS_PLUGIN, RenaGnomeMediaKeysPlugin))
#define RENA_GNOME_MEDIA_KEYS_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), RENA_TYPE_GNOME_MEDIA_KEYS_PLUGIN, RenaGnomeMediaKeysPlugin))
#define RENA_IS_GNOME_MEDIA_KEYS_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), RENA_TYPE_GNOME_MEDIA_KEYS_PLUGIN))
#define RENA_IS_GNOME_MEDIA_KEYS_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), RENA_TYPE_GNOME_MEDIA_KEYS_PLUGIN))
#define RENA_GNOME_MEDIA_KEYS_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), RENA_TYPE_GNOME_MEDIA_KEYS_PLUGIN, RenaGnomeMediaKeysPluginClass))

typedef struct _RenaGnomeMediaKeysPluginPrivate RenaGnomeMediaKeysPluginPrivate;

struct _RenaGnomeMediaKeysPluginPrivate {
	RenaApplication *rena;

	gint               watch_id;
	guint              handler_id;
	GDBusProxy        *proxy;
};

GType                 rena_gnome_media_keys_plugin_get_type        (void) G_GNUC_CONST;
G_MODULE_EXPORT void  peas_register_types                            (PeasObjectModule *module);

G_END_DECLS

#endif /* __RENA_GNOME_MEDIA_KEYS_PLUGIN_H__ */
