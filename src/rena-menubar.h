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

#ifndef RENA_MENU_H
#define RENA_MENU_H

#include <gtk/gtk.h>
#include "rena-backend.h"

#include "rena.h"

void rena_menubar_update_playback_state_cb (RenaBackend *backend, GParamSpec *pspec, gpointer user_data);

/*
 * Public api..
 */

void rena_menubar_connect_signals (GtkUIManager *menu_ui_manager, RenaApplication *rena);

void
rena_menubar_set_enable_action (GtkWindow  *window,
                                  const char *action_name,
                                  gboolean    enabled);

void
rena_menubar_append_action (RenaApplication *rena,
                              const gchar       *placeholder,
                              GSimpleAction     *action,
                              GMenuItem         *item);
void
rena_menubar_remove_action (RenaApplication *rena,
                              const gchar       *placeholder,
                              const gchar       *action_name);

void
rena_menubar_append_submenu (RenaApplication  *rena,
                               const gchar        *placeholder,
                               const gchar        *xml_ui,
                               const gchar        *menu_id,
                               const gchar        *label,
                               gpointer            user_data);
void
rena_menubar_remove_by_id (RenaApplication *rena,
                             const gchar       *placeholder,
                             const gchar       *item_id);

gint
rena_menubar_append_plugin_action (RenaApplication *rena,
                                     GtkActionGroup    *action_group,
                                     const gchar       *menu_xml);

void
rena_menubar_remove_plugin_action (RenaApplication *rena,
                                     GtkActionGroup    *action_group,
                                     gint               merge_id);

GtkActionGroup *
rena_menubar_plugin_action_new (const gchar                *name,
                                  const GtkActionEntry       *action_entries,
                                  guint                       n_action_entries,
                                  const GtkToggleActionEntry *toggle_entries,
                                  guint                       n_toggle_entries,
                                  gpointer                    user_data);


GtkUIManager *rena_menubar_new       (void);
GtkBuilder   *rena_gmenu_toolbar_new (RenaApplication *rena);

#endif /* RENA_MENU_H */
