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

#ifndef RENA_TEMP_PROVIDER_H
#define RENA_TEMP_PROVIDER_H

#include <glib.h>
#include <glib-object.h>

#include "rena-musicobject.h"

G_BEGIN_DECLS

#define RENA_TYPE_TEMP_PROVIDER (rena_temp_provider_get_type())
#define RENA_TEMP_PROVIDER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_TEMP_PROVIDER, RenaTempProvider))
#define RENA_TEMP_PROVIDER_CONST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_TEMP_PROVIDER, RenaTempProvider const))
#define RENA_TEMP_PROVIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), RENA_TYPE_TEMP_PROVIDER, RenaTempProviderClass))
#define RENA_IS_TEMP_PROVIDER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_TEMP_PROVIDER))
#define RENA_IS_TEMP_PROVIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), RENA_TYPE_TEMP_PROVIDER))
#define RENA_TEMP_PROVIDER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), RENA_TYPE_TEMP_PROVIDER, RenaTempProviderClass))

typedef struct _RenaTempProvider RenaTempProvider;
typedef struct _RenaTempProviderClass RenaTempProviderClass;

struct _RenaTempProviderClass
{
	GObjectClass parent_class;
};


void
rena_temp_provider_insert_track    (RenaTempProvider *provider,
                                      RenaMusicobject  *mobj);

void
rena_temp_provider_merge_database  (RenaTempProvider *provider);

void
rena_temp_provider_commit_database (RenaTempProvider *provider);

void
rena_temp_provider_set_visible     (RenaTempProvider *provider,
                                      gboolean            visible);

RenaTempProvider *
rena_temp_provider_new             (const gchar        *name,
                                      const gchar        *type,
                                      const gchar        *friendly_name,
                                      const gchar        *icon_name);

G_END_DECLS

#endif /* RENA_TEMP_PROVIDER_H */
