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

#ifndef RENA_TAGS_DIALOG_H
#define RENA_TAGS_DIALOG_H

#include <glib-object.h>
#include <gtk/gtk.h>

#include "rena-musicobject.h"

G_BEGIN_DECLS

#define RENA_TYPE_TAGS_DIALOG            (rena_tags_dialog_get_type ())
#define RENA_TAGS_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_TAGS_DIALOG, RenaTagsDialog))
#define RENA_TAGS_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), RENA_TYPE_TAGS_DIALOG, RenaTagsDialogClass))
#define RENA_IS_TAGS_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_TAGS_DIALOG))
#define RENA_IS_TAGS_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), RENAL_TYPE_TAGS_DIALOG))
#define RENA_TAGS_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), RENA_TYPE_TAGS_DIALOG, RenaTagsDialogClass))

#define TAG_MAX_LEN                256

typedef struct _RenaTagsDialogClass RenaTagsDialogClass;
typedef struct _RenaTagsDialog      RenaTagsDialog;

GType              rena_tags_dialog_get_type        (void) G_GNUC_CONST;

RenaMusicobject *rena_tags_dialog_get_musicobject (RenaTagsDialog *dialog);
void               rena_tags_dialog_set_musicobject (RenaTagsDialog *dialog, RenaMusicobject *mobj);

void               rena_tags_dialog_set_changed     (RenaTagsDialog *dialog, gint changed);
gint               rena_tags_dialog_get_changed     (RenaTagsDialog *dialog);

GtkWidget          *rena_tags_dialog_new            (void);

void                rena_track_properties_dialog    (RenaMusicobject *mobj, GtkWidget *parent);

G_END_DECLS

#endif /* RENA_TAGS_DIALOG_H */
