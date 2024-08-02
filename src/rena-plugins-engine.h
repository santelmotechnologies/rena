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

#ifndef RENA_PLUGINS_ENGINE_H
#define RENA_PLUGINS_ENGINE_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define RENA_TYPE_PLUGINS_ENGINE (rena_plugins_engine_get_type())
#define RENA_PLUGINS_ENGINE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_PLUGINS_ENGINE, RenaPluginsEngine))
#define RENA_PLUGINS_ENGINE_CONST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_PLUGINS_ENGINE, RenaPluginsEngine const))
#define RENA_PLUGINS_ENGINE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), RENA_TYPE_PLUGINS_ENGINE, RenaPluginsEngineClass))
#define RENA_IS_PLUGINS_ENGINE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_PLUGINS_ENGINE))
#define RENA_IS_PLUGINS_ENGINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), RENA_TYPE_PLUGINS_ENGINE))
#define RENA_PLUGINS_ENGINE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), RENA_TYPE_PLUGINS_ENGINE, RenaPluginsEngineClass))

typedef struct _RenaPluginsEngine RenaPluginsEngine;
typedef struct _RenaPluginsEngineClass RenaPluginsEngineClass;

struct _RenaPluginsEngineClass
{
	GObjectClass parent_class;
};

gboolean             rena_plugins_engine_is_starting (RenaPluginsEngine *engine);
gboolean             rena_plugins_engine_is_shutdown (RenaPluginsEngine *engine);

void                 rena_plugins_engine_shutdown    (RenaPluginsEngine *engine);
void                 rena_plugins_engine_startup     (RenaPluginsEngine *engine);

RenaPluginsEngine *rena_plugins_engine_new         (GObject             *object);

G_END_DECLS

#endif /* RENA_PLUGINS_ENGINE_H */
