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

#ifndef RENA_SONGINFO_PANE_H
#define RENA_SONGINFO_PANE_H

#include <gtk/gtk.h>
#include <glib-object.h>

#include "src/rena-musicobject.h"

G_BEGIN_DECLS

#define RENA_TYPE_SONGINFO_PANE            (rena_songinfo_pane_get_type ())
#define RENA_SONGINFO_PANE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_SONGINFO_PANE, RenaSonginfoPane))
#define RENA_IS_SONGINFO_PANE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_SONGINFO_PANE))
#define RENA_SONGINFO_PANE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  RENA_TYPE_SONGINFO_PANE, RenaSonginfoPaneClass))
#define RENA_IS_SONGINFO_PANE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  RENA_TYPE_SONGINFO_PANE))
#define RENA_SONGINFO_PANE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  RENA_TYPE_SONGINFO_PANE, RenaSonginfoPaneClass))

typedef struct _RenaSonginfoPane RenaSonginfoPane;

typedef struct {
	GtkScrolledWindowClass __parent__;
	void (*type_changed)   (RenaSonginfoPane *pane);
	void (*append)         (RenaSonginfoPane *pane, RenaMusicobject *mobj);
	void (*append_all)     (RenaSonginfoPane *pane);
	void (*queue)          (RenaSonginfoPane *pane, RenaMusicobject *mobj);
} RenaSonginfoPaneClass;

GtkWidget *         rena_songinfo_pane_row_new               (RenaMusicobject *mobj);

void                rena_songinfo_pane_set_title             (RenaSonginfoPane *pane,
                                                                const gchar        *title);

void                rena_songinfo_pane_set_text              (RenaSonginfoPane *pane,
                                                                const gchar        *text,
                                                                const gchar        *provider);

void                rena_songinfo_pane_append_song_row       (RenaSonginfoPane *pane,
                                                                GtkWidget          *row);

void                rena_songinfo_pane_clear_text            (RenaSonginfoPane *pane);
void                rena_songinfo_pane_clear_list            (RenaSonginfoPane *pane);

GList              *rena_songinfo_get_mobj_list              (RenaSonginfoPane *pane);

GtkWidget          *rena_songinfo_pane_get_pane_title        (RenaSonginfoPane *pane);
GtkWidget          *rena_songinfo_pane_get_popover           (RenaSonginfoPane *pane);
GLYR_GET_TYPE       rena_songinfo_pane_get_default_view      (RenaSonginfoPane *pane);
void                rena_songinfo_pane_set_default_view      (RenaSonginfoPane *pane, GLYR_GET_TYPE view_type);

RenaSonginfoPane *rena_songinfo_pane_new                   (void);

G_END_DECLS

#endif /* RENA_SONGINFO_H */
