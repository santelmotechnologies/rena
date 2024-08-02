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

#ifndef __RENA_MPRIS2_PLUGIN_H__
#define __RENA_MPRIS2_PLUGIN_H__

#include <gtk/gtk.h>
#include <libpeas/peas.h>

#include "src/rena.h"

G_BEGIN_DECLS

#define RENA_TYPE_MPRIS2_PLUGIN         (rena_mpris2_plugin_get_type ())
#define RENA_MPRIS2_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), RENA_TYPE_MPRIS2_PLUGIN, RenaMpris2Plugin))
#define RENA_MPRIS2_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), RENA_TYPE_MPRIS2_PLUGIN, RenaMpris2Plugin))
#define RENA_IS_MPRIS2_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), RENA_TYPE_MPRIS2_PLUGIN))
#define RENA_IS_MPRIS2_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), RENA_TYPE_MPRIS2_PLUGIN))
#define RENA_MPRIS2_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), RENA_TYPE_MPRIS2_PLUGIN, RenaMpris2PluginClass))

#define N_OBJECTS  4
#define MPRIS_NAME "org.mpris.MediaPlayer2.rena"
#define MPRIS_PATH "/org/mpris/MediaPlayer2"

typedef struct _RenaMpris2PluginPrivate RenaMpris2PluginPrivate;

struct _RenaMpris2PluginPrivate {
	RenaApplication *rena;

	guint              owner_id;
	GDBusNodeInfo     *introspection_data;
	GDBusConnection   *dbus_connection;
	GQuark             interface_quarks[N_OBJECTS];
	guint              registration_object_ids[N_OBJECTS];

	gboolean           saved_playbackstatus;
	gboolean           saved_shuffle;
	gchar             *saved_title;
	gdouble            volume;
	gboolean           saved_can_next;
	gboolean           saved_can_prev;
	gboolean           saved_can_play;
	gboolean           saved_can_pause;
	gboolean           saved_can_seek;

	RenaBackendState state;
};

GType                 rena_mpris2_plugin_get_type           (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __RENA_MPRIS2_PLUGIN_H__ */
