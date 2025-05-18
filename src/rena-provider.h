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

#ifndef RENA_PROVIDER_H
#define RENA_PROVIDER_H

#include <glib-object.h>

G_BEGIN_DECLS

GType rena_provider_get_type (void);

#define RENA_TYPE_PROVIDER (rena_provider_get_type())
#define RENA_PROVIDER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_PROVIDER, RenaProvider))
#define RENA_PROVIDER_CONST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_PROVIDER, RenaProvider const))
#define RENA_PROVIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), RENA_TYPE_PROVIDER, RenaProviderClass))
#define RENA_IS_PROVIDER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_PROVIDER))
#define RENA_IS_PROVIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), RENA_TYPE_PROVIDER))
#define RENA_PROVIDER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), RENA_TYPE_PROVIDER, RenaProviderClass))

typedef struct _RenaProvider RenaProvider;
typedef struct _RenaProviderClass RenaProviderClass;

struct _RenaProviderClass {
	GObjectClass parent_class;
};

/*
 * Public api.
 */

RenaProvider *
rena_provider_new (const gchar *name,
                     const gchar *kind,
                     const gchar *friendly_name,
                     const gchar *icon_name,
                     gboolean     visible,
                     gboolean     ignored);

const gchar *
rena_provider_get_name          (RenaProvider *provider);
const gchar *
rena_provider_get_kind          (RenaProvider *provider);
const gchar *
rena_provider_get_friendly_name (RenaProvider *provider);
const gchar *
rena_provider_get_icon_name     (RenaProvider *provider);
gboolean
rena_provider_get_visible       (RenaProvider *provider);
gboolean
rena_provider_get_ignored       (RenaProvider *provider);

G_END_DECLS

#endif /* RENA_PROVIDER_H */

