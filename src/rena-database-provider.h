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

#ifndef RENA_DATABASE_PROVIDER_H
#define RENA_DATABASE_PROVIDER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define RENA_TYPE_DATABASE_PROVIDER            (rena_database_provider_get_type())
#define RENA_DATABASE_PROVIDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_DATABASE_PROVIDER, RenaDatabaseProvider))
#define RENA_DATABASE_PROVIDER_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_DATABASE_PROVIDER, RenaDatabaseProvider const))
#define RENA_DATABASE_PROVIDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  RENA_TYPE_DATABASE_PROVIDER, RenaDatabaseProviderClass))
#define RENA_IS_DATABASE_PROVIDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_DATABASE_PROVIDER))
#define RENA_IS_DATABASE_PROVIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  RENA_TYPE_DATABASE_PROVIDER))
#define RENA_DATABASE_PROVIDER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  RENA_TYPE_DATABASE_PROVIDER, RenaDatabaseProviderClass))

typedef struct _RenaDatabaseProvider        RenaDatabaseProvider;
typedef struct _RenaDatabaseProviderClass   RenaDatabaseProviderClass;
typedef struct _RenaDatabaseProviderPrivate RenaDatabaseProviderPrivate;

struct _RenaDatabaseProvider
{
	GObject parent;

	/*< private >*/
	RenaDatabaseProviderPrivate *priv;
};

struct _RenaDatabaseProviderClass
{
	GObjectClass parent_class;

	void (*want_upgrade)       (RenaDatabaseProvider *provider, gint provider_id);
	void (*want_update)        (RenaDatabaseProvider *provider, gint provider_id);
	void (*want_remove)        (RenaDatabaseProvider *provider, gint provider_id);
	void (*update_done)        (RenaDatabaseProvider *provider);
};

/*
 * Public api.
 */

void
rena_provider_add_new (RenaDatabaseProvider *provider,
                         const gchar            *name,
                         const gchar            *type,
                         const gchar            *friendly_name,
                         const gchar            *icon_name);

void
rena_provider_remove (RenaDatabaseProvider *provider,
                        const gchar            *name);

gboolean
rena_provider_exist (RenaDatabaseProvider *provider,
                       const gchar            *name);

void
rena_provider_forget_songs (RenaDatabaseProvider *provider,
                              const gchar            *name);

GSList *
rena_provider_get_list (RenaDatabaseProvider *provider);

GSList *
rena_provider_get_visible_list (RenaDatabaseProvider *provider, gboolean visibles);

GSList *
rena_provider_get_handled_list (RenaDatabaseProvider *provider);

GSList *
rena_database_provider_get_list_by_type (RenaDatabaseProvider *provider,
                                          const gchar            *provider_type);

GSList *
rena_provider_get_handled_list_by_type (RenaDatabaseProvider *provider,
                                          const gchar            *provider_type);

GSList *
rena_database_provider_get_list (RenaDatabaseProvider *database_provider);

void
rena_provider_set_visible (RenaDatabaseProvider *provider,
                             const gchar            *name,
                             gboolean                visible);

void
rena_provider_set_ignore (RenaDatabaseProvider *provider,
                            const gchar            *name,
                            gboolean                ignore);

gchar *
rena_database_provider_get_friendly_name (RenaDatabaseProvider *provider, const gchar *name);

gchar *
rena_database_provider_get_icon_name (RenaDatabaseProvider *provider, const gchar *name);

void
rena_provider_want_upgrade (RenaDatabaseProvider *provider, gint provider_id);

void
rena_provider_want_update (RenaDatabaseProvider *provider, gint provider_id);

void
rena_provider_want_remove (RenaDatabaseProvider *provider, gint provider_id);

void
rena_provider_update_done (RenaDatabaseProvider *provider);

RenaDatabaseProvider *
rena_database_provider_get (void);

G_END_DECLS

#endif /* RENA_DATABASE_PROVIDER_H */
