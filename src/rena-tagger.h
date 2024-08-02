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

#ifndef RENA_TAGGER_H
#define RENA_TAGGER_H

#include <glib-object.h>
#include "rena-musicobject.h"

G_BEGIN_DECLS

#define RENA_TYPE_TAGGER (rena_tagger_get_type())
#define RENA_TAGGER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_TAGGER, RenaTagger))
#define RENA_TAGGER_CONST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_TAGGER, RenaTagger const))
#define RENA_TAGGER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), RENA_TYPE_TAGGER, RenaTaggerClass))
#define RENA_IS_TAGGER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_TAGGER))
#define RENA_IS_TAGGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), RENA_TYPE_TAGGER))
#define RENA_TAGGER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), RENA_TYPE_TAGGER, RenaTaggerClass))

typedef struct _RenaTagger RenaTagger;
typedef struct _RenaTaggerClass RenaTaggerClass;
typedef struct _RenaTaggerPrivate RenaTaggerPrivate;

struct _RenaTagger
{
	GObject parent;

	/*< private >*/
	RenaTaggerPrivate *priv;
};

struct _RenaTaggerClass
{
	GObjectClass parent_class;
};

void rena_tagger_set_changes     (RenaTagger *tagger, RenaMusicobject *mobj, gint changed);
void rena_tagger_add_file        (RenaTagger *tagger, const gchar *file);
void rena_tagger_add_location_id (RenaTagger *tagger, gint location_id);
void rena_tagger_apply_changes   (RenaTagger *tagger);

RenaTagger *rena_tagger_new (void);

G_END_DECLS

#endif /* RENA_TAGGER_H */
