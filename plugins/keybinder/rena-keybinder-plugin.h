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
#ifndef __RENA_KEYBINDER_PLUGIN_H__
#define __RENA_KEYBINDER_PLUGIN_H__

#include <gtk/gtk.h>
#include <libpeas/peas.h>

#include "src/rena.h"

G_BEGIN_DECLS

#define RENA_TYPE_KEYBINDER_PLUGIN         (rena_keybinder_plugin_get_type ())
#define RENA_KEYBINDER_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), RENA_TYPE_KEYBINDER_PLUGIN, RenaKeybinderPlugin))
#define RENA_KEYBINDER_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), RENA_TYPE_KEYBINDER_PLUGIN, RenaKeybinderPlugin))
#define RENA_IS_KEYBINDER_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), RENA_TYPE_KEYBINDER_PLUGIN))
#define RENA_IS_KEYBINDER_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), RENA_TYPE_KEYBINDER_PLUGIN))
#define RENA_KEYBINDER_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), RENA_TYPE_KEYBINDER_PLUGIN, RenaKeybinderPluginClass))

typedef struct _RenaKeybinderPluginPrivate RenaKeybinderPluginPrivate;

struct _RenaKeybinderPluginPrivate {
	RenaApplication *rena;
};

GType                 rena_keybinder_plugin_get_type        (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __RENA_KEYBINDER_PLUGIN_H__ */
