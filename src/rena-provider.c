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

#include "rena-provider.h"

struct _RenaProvider
{
	GObject     parent_instance;
};

typedef struct _RenaProviderPrivate RenaProviderPrivate;

struct _RenaProviderPrivate
{
	gchar    *name;
	gchar    *type;
	gchar    *friendly_name;
	gchar    *icon_name;
	gboolean  visible;
	gboolean  ignored;
};

G_DEFINE_TYPE_WITH_PRIVATE (RenaProvider, rena_provider, G_TYPE_OBJECT)

enum
{
	PROP_NAME = 1,
	PROP_TYPE,
	PROP_FRIENDLY_NAME,
	PROP_ICON_NAME,
	PROP_VISIBLE,
	PROP_IGNORED,
	LAST_PROP
};

static GParamSpec *gParamSpecs[LAST_PROP];

/* Provider */

RenaProvider *
rena_provider_new (const gchar *name,
                     const gchar *kind,
                     const gchar *friendly_name,
                     const gchar *icon_name,
                     gboolean     visible,
                     gboolean     ignored)
{
	return g_object_new (RENA_TYPE_PROVIDER,
	                     "name",          name,
	                     "type",          kind,
	                     "friendly-name", friendly_name,
	                     "icon-name",     icon_name,
	                     "visible",       visible,
	                     "ignored",       ignored,
	                     NULL);
}

const gchar *
rena_provider_get_name (RenaProvider *provider)
{
	g_return_val_if_fail(RENA_IS_PROVIDER(provider), NULL);
	RenaProviderPrivate *priv = rena_provider_get_instance_private(provider);
	return priv->name;
}

const gchar *
rena_provider_get_kind (RenaProvider *provider)
{
	g_return_val_if_fail(RENA_IS_PROVIDER(provider), NULL);
	RenaProviderPrivate *priv = rena_provider_get_instance_private(provider);
	return priv->type;
}

const gchar *
rena_provider_get_friendly_name (RenaProvider *provider)
{
	g_return_val_if_fail(RENA_IS_PROVIDER(provider), NULL);
	RenaProviderPrivate *priv = rena_provider_get_instance_private(provider);
	return priv->friendly_name;
}

const gchar *
rena_provider_get_icon_name (RenaProvider *provider)
{
	g_return_val_if_fail(RENA_IS_PROVIDER(provider), NULL);
	RenaProviderPrivate *priv = rena_provider_get_instance_private(provider);
	return priv->icon_name;
}

gboolean
rena_provider_get_visible (RenaProvider *provider)
{
	g_return_val_if_fail(RENA_IS_PROVIDER(provider), FALSE);
	RenaProviderPrivate *priv = rena_provider_get_instance_private(provider);
	return priv->visible;
}

gboolean
rena_provider_get_ignored (RenaProvider *provider)
{
	g_return_val_if_fail(RENA_IS_PROVIDER(provider), FALSE);
	RenaProviderPrivate *priv = rena_provider_get_instance_private(provider);
	return priv->ignored;
}

/*
 * RenaProvider implementation.
 */

static void
rena_provider_finalize (GObject *object)
{
	RenaProvider *provider = RENA_PROVIDER(object);
	RenaProviderPrivate *priv = rena_provider_get_instance_private(provider);

	g_free(priv->name);
	g_free(priv->type);
	g_free(priv->friendly_name);
	g_free(priv->icon_name);

	G_OBJECT_CLASS(rena_provider_parent_class)->finalize(object);
}


static void
rena_provider_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
	RenaProvider *provider = RENA_PROVIDER(object);
	RenaProviderPrivate *priv = rena_provider_get_instance_private(provider);

	switch (prop_id) {
		case PROP_NAME:
			priv->name = g_value_dup_string(value);
			break;
		case PROP_TYPE:
			priv->type = g_value_dup_string(value);
			break;
		case PROP_FRIENDLY_NAME:
			priv->friendly_name = g_value_dup_string(value);
			break;
		case PROP_ICON_NAME:
			priv->icon_name = g_value_dup_string(value);
			break;
		case PROP_VISIBLE:
			priv->visible = g_value_get_boolean (value);
			break;
		case PROP_IGNORED:
			priv->ignored = g_value_get_boolean (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}


static void
rena_provider_class_init (RenaProviderClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = rena_provider_finalize;
	object_class->set_property = rena_provider_set_property;
	//g_type_class_add_private(object_class, sizeof(RenaProviderPrivate));

	/**
	  * RenaPovider:name:
	  *
	  */
	gParamSpecs[PROP_NAME] =
		g_param_spec_string("name",
		                    "Name",
		                    "The name",
		                    "",
		                    G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);
	/**
	  * RenaPovider:type:
	  *
	  */
	gParamSpecs[PROP_TYPE] =
		g_param_spec_string("type",
		                    "Type",
		                    "The type",
		                    "",
		                    G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);
	/**
	  * RenaPovider:friendly_name:
	  *
	  */
	gParamSpecs[PROP_FRIENDLY_NAME] =
		g_param_spec_string("friendly-name",
		                    "FriendyName",
		                    "The fiendly name",
		                    "",
		                    G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);
	/**
	  * RenaPovider:icon_name:
	  *
	  */
	gParamSpecs[PROP_ICON_NAME] =
		g_param_spec_string("icon-name",
		                    "IconName",
		                    "The icon name",
		                    "",
		                    G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);
	/**
	  * RenaPovider:visible:
	  *
	  */
	gParamSpecs[PROP_VISIBLE] =
		g_param_spec_boolean ("visible",
		                      "Visible", "The Visible status",
		                      FALSE,
		                      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);
	/**
	  * RenaPovider:ignore:
	  *
	  */
	gParamSpecs[PROP_IGNORED] =
		g_param_spec_boolean ("ignored",
		                      "Ignored", "The Ignored status",
		                      FALSE,
		                      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);

	g_object_class_install_properties(object_class, LAST_PROP, gParamSpecs);
}

static void
rena_provider_init (RenaProvider *provider)
{
}
