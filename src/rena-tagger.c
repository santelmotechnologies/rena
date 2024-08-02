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

#include "rena-tagger.h"

#include "rena-musicobject.h"
#include "rena-database.h"
#include "rena-database-provider.h"
#include "rena-library-pane.h"
#include "rena-tags-mgmt.h"

struct _RenaTaggerPrivate
{
	RenaMusicobject *mobj;
	gint               changed;

	GArray            *loc_arr;
	GPtrArray         *file_arr;

	RenaDatabase    *cdbase;
};

G_DEFINE_TYPE_WITH_PRIVATE(RenaTagger, rena_tagger, G_TYPE_OBJECT)

void
rena_tagger_set_changes(RenaTagger *tagger, RenaMusicobject *mobj, gint changed)
{
	RenaTaggerPrivate *priv = tagger->priv;

	priv->mobj = rena_musicobject_dup(mobj);
	priv->changed = changed;
}

void
rena_tagger_add_file(RenaTagger *tagger, const gchar *file)
{
	gint location_id = 0;
	RenaTaggerPrivate *priv = tagger->priv;

	location_id = rena_database_find_location(priv->cdbase, file);
	if (G_LIKELY(location_id))
		g_array_append_val(priv->loc_arr, location_id);

	g_ptr_array_add(priv->file_arr, g_strdup(file));
}

void
rena_tagger_add_location_id(RenaTagger *tagger, gint location_id)
{
	gchar *file = NULL;

	RenaTaggerPrivate *priv = tagger->priv;

	g_array_append_val(priv->loc_arr, location_id);

	file = rena_database_get_filename_from_location_id(priv->cdbase, location_id);
	if (G_LIKELY(file))
		g_ptr_array_add(priv->file_arr, file);
}

void
rena_tagger_apply_changes(RenaTagger *tagger)
{
	RenaDatabaseProvider *provider;

	RenaTaggerPrivate *priv = tagger->priv;

	if(priv->file_arr->len)
		rena_update_local_files_change_tag(priv->file_arr, priv->changed, priv->mobj);

	if(priv->loc_arr->len) {
		rena_database_update_local_files_change_tag(priv->cdbase, priv->loc_arr, priv->changed, priv->mobj);

		provider = rena_database_provider_get ();
		rena_provider_update_done (provider);
		g_object_unref (provider);
	}
}

static void
rena_tagger_dispose (GObject *object)
{
	RenaTagger *tagger = RENA_TAGGER (object);
	RenaTaggerPrivate *priv = tagger->priv;

	if (priv->mobj) {
		g_object_unref (priv->mobj);
		priv->mobj = NULL;
	}
	if (priv->cdbase) {
		g_object_unref (priv->cdbase);
		priv->cdbase = NULL;
	}

	G_OBJECT_CLASS (rena_tagger_parent_class)->dispose (object);
}

static void
rena_tagger_finalize (GObject *object)
{
	RenaTagger *tagger = RENA_TAGGER(object);
	RenaTaggerPrivate *priv = tagger->priv;

	g_array_free(priv->loc_arr, TRUE);
	g_ptr_array_free(priv->file_arr, TRUE);

	G_OBJECT_CLASS(rena_tagger_parent_class)->finalize(object);
}

static void
rena_tagger_class_init (RenaTaggerClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->dispose = rena_tagger_dispose;
	object_class->finalize = rena_tagger_finalize;
}

static void
rena_tagger_init (RenaTagger *tagger)
{
	tagger->priv = G_TYPE_INSTANCE_GET_PRIVATE(tagger,
	                                           RENA_TYPE_TAGGER,
	                                           RenaTaggerPrivate);

	RenaTaggerPrivate *priv = tagger->priv;

	priv->mobj = NULL;
	priv->changed = 0;

	priv->loc_arr = g_array_new(TRUE, TRUE, sizeof(gint));
	priv->file_arr = g_ptr_array_new_with_free_func(g_free);

	priv->cdbase = rena_database_get();
}

/**
 * rena_tagger_new:
 *
 * Return value: a new #RenaTagger instance.
 **/
RenaTagger*
rena_tagger_new (void)
{
	RenaTagger *tagger = NULL;

	tagger = g_object_new(RENA_TYPE_TAGGER, NULL);

	return tagger;
}
