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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(GETTEXT_PACKAGE)
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#include "src/rena-hig.h"
#include "src/rena-preferences.h"
#include "src/rena-preferences-dialog.h"
#include "src/rena-utils.h"

#include "rena-subsonic-preferences.h"


/*
 * RenaSubsonicPreferences *
 */

struct _RenaSubsonicPreferences {
	GObject           _parent;

	RenaPreferencesDialog *dialog;

	GtkWidget         *settings_widget;

	GtkWidget         *server_entry;
	GtkWidget         *username_entry;
	GtkWidget         *password_entry;

	RenaPreferences *preferences;
};

enum {
	SIGNAL_SERVER_CHANGED,
	SIGNAL_CREDENTIALS_CHANGED,
	LAST_SIGNAL
};

static int signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE(RenaSubsonicPreferences, rena_subsonic_preferences, G_TYPE_OBJECT)


/*
 * Settings.
 */

#define GROUP_KEY_SUBSONIC  "subsonic"
#define KEY_SUBSONIC_SERVER "server"
#define KEY_SUBSONIC_USER   "username"
#define KEY_SUBSONIC_PASS   "password"

static gchar *
rena_subsonic_preferences_get_server (RenaSubsonicPreferences *preferences)
{
	gchar *plugin_group = NULL, *string = NULL;

	plugin_group = rena_preferences_get_plugin_group_name (preferences->preferences, GROUP_KEY_SUBSONIC);
	string = rena_preferences_get_string (preferences->preferences,
	                                        plugin_group,
	                                        KEY_SUBSONIC_SERVER);
	g_free (plugin_group);

	return string;
}

static void
rena_subsonic_preferences_set_server (RenaSubsonicPreferences *preferences,
                                        const gchar               *server)
{
	gchar *plugin_group = NULL;

	plugin_group = rena_preferences_get_plugin_group_name (preferences->preferences, GROUP_KEY_SUBSONIC);

	if (string_is_not_empty(server))
		rena_preferences_set_string (preferences->preferences,
		                               plugin_group,
		                               KEY_SUBSONIC_SERVER,
		                               server);
	else
 		rena_preferences_remove_key (preferences->preferences,
		                               plugin_group,
		                               KEY_SUBSONIC_SERVER);

	g_free (plugin_group);
}

static gchar *
rena_subsonic_preferences_get_username (RenaSubsonicPreferences *preferences)
{
	gchar *plugin_group = NULL, *string = NULL;

	plugin_group = rena_preferences_get_plugin_group_name (preferences->preferences, GROUP_KEY_SUBSONIC);
	string = rena_preferences_get_string (preferences->preferences,
	                                        plugin_group,
	                                        KEY_SUBSONIC_USER);
	g_free (plugin_group);

	return string;
}

static void
rena_subsonic_preferences_set_username (RenaSubsonicPreferences *preferences,
                                          const gchar               *username)
{
	gchar *plugin_group = NULL;

	plugin_group = rena_preferences_get_plugin_group_name (preferences->preferences, GROUP_KEY_SUBSONIC);

	if (string_is_not_empty(username))
		rena_preferences_set_string (preferences->preferences,
		                               plugin_group,
		                               KEY_SUBSONIC_USER,
		                               username);
	else
		rena_preferences_remove_key (preferences->preferences,
		                               plugin_group,
		                               KEY_SUBSONIC_USER);

	g_free (plugin_group);
}

static gchar *
rena_subsonic_preferences_get_password (RenaSubsonicPreferences *preferences)
{
	gchar *plugin_group = NULL, *string = NULL;

	plugin_group = rena_preferences_get_plugin_group_name (preferences->preferences, GROUP_KEY_SUBSONIC);
	string = rena_preferences_get_string (preferences->preferences,
	                                        plugin_group,
	                                        KEY_SUBSONIC_PASS);

	g_free (plugin_group);

	return string;
}

static void
rena_subsonic_preferences_set_password (RenaSubsonicPreferences *preferences,
                                          const gchar               *password)
{
	gchar *plugin_group = NULL;
	plugin_group = rena_preferences_get_plugin_group_name (preferences->preferences, GROUP_KEY_SUBSONIC);

	if (string_is_not_empty(password))
		rena_preferences_set_string (preferences->preferences,
		                               plugin_group,
		                               KEY_SUBSONIC_PASS,
		                               password);
	else
 		rena_preferences_remove_key (preferences->preferences,
		                               plugin_group,
		                               KEY_SUBSONIC_PASS);

	g_free (plugin_group);
}


/*
 *  Public api.
 */

const gchar *
rena_subsonic_preferences_get_server_text (RenaSubsonicPreferences *preferences)
{
	return gtk_entry_get_text (GTK_ENTRY(preferences->server_entry));
}

const gchar *
rena_subsonic_preferences_get_username_text (RenaSubsonicPreferences *preferences)
{
	return gtk_entry_get_text (GTK_ENTRY(preferences->username_entry));
}

const gchar *
rena_subsonic_preferences_get_password_text (RenaSubsonicPreferences *preferences)
{
	return gtk_entry_get_text (GTK_ENTRY(preferences->password_entry));
}

static void
rena_subsonic_preferences_dialog_response (GtkDialog                 *dialog,
                                             gint                       response_id,
                                             RenaSubsonicPreferences *preferences)
{
	const gchar *entry_server = NULL, *entry_user = NULL, *entry_pass = NULL;
	gchar *test_server = NULL, *test_user = NULL, *test_pass = NULL;
	gboolean changed = FALSE, changed_server = FALSE;

	test_server = rena_subsonic_preferences_get_server (preferences);
	test_user = rena_subsonic_preferences_get_username (preferences);
	test_pass = rena_subsonic_preferences_get_password (preferences);

	switch(response_id)
	{
		case GTK_RESPONSE_CANCEL:
			rena_gtk_entry_set_text (GTK_ENTRY(preferences->server_entry), test_server);
			rena_gtk_entry_set_text (GTK_ENTRY(preferences->username_entry), test_user);
			rena_gtk_entry_set_text (GTK_ENTRY(preferences->password_entry), test_pass);
			break;
		case GTK_RESPONSE_OK:
			entry_server = gtk_entry_get_text (GTK_ENTRY(preferences->server_entry));
			entry_user = gtk_entry_get_text (GTK_ENTRY(preferences->username_entry));
			entry_pass = gtk_entry_get_text (GTK_ENTRY(preferences->password_entry));

			if (g_strcmp0 (test_server, entry_server)) {
				rena_subsonic_preferences_set_server (preferences, entry_server);
				changed = TRUE;
				changed_server = TRUE;
			}
			if (g_strcmp0 (test_user, entry_user)) {
				rena_subsonic_preferences_set_username (preferences, entry_user);
				changed = TRUE;
			}
			if (g_strcmp0 (test_pass, entry_pass)) {
				rena_subsonic_preferences_set_password (preferences, entry_pass);
				changed = TRUE;
			}

			if (changed_server) {
				g_signal_emit (preferences, signals[SIGNAL_SERVER_CHANGED], 0, NULL);
			}
			if (changed) {
				g_signal_emit (preferences, signals[SIGNAL_CREDENTIALS_CHANGED], 0, NULL);
			}
			break;
		default:
			break;
	}

	g_free (test_server);
	g_free (test_user);
	g_free (test_pass);
}

GtkWidget *
rena_subsonic_preferences_get_widget (RenaSubsonicPreferences *preferences)
{
	return preferences->settings_widget;
}

void
rena_subsonic_preferences_forget_settings (RenaSubsonicPreferences *preferences)
{
	gchar *plugin_group = NULL;
	plugin_group = rena_preferences_get_plugin_group_name (preferences->preferences, GROUP_KEY_SUBSONIC);
	rena_preferences_remove_group (preferences->preferences, plugin_group);
	g_free (plugin_group);
}


/*
 * RenaSubsonicPreferences *
 */

static void
rena_subsonic_plugin_build_widget (RenaSubsonicPreferences *preferences)
{
	GtkWidget *table, *label, *server_entry, *username_entry, *password_entry;
	gchar *server = NULL, *username = NULL, *password = NULL;
	guint row = 0;

	table = rena_hig_workarea_table_new ();

	rena_hig_workarea_table_add_section_title (table, &row, _("Subsonic"));

	label = gtk_label_new (_("Server"));

	server_entry = gtk_entry_new ();
	gtk_entry_set_icon_from_icon_name (GTK_ENTRY(server_entry), GTK_ENTRY_ICON_PRIMARY, "network-server");
	gtk_entry_set_activates_default (GTK_ENTRY(server_entry), TRUE);

	rena_hig_workarea_table_add_row (table, &row, label, server_entry);

	label = gtk_label_new (_("Username"));

	username_entry = gtk_entry_new ();
	gtk_entry_set_icon_from_icon_name (GTK_ENTRY(username_entry), GTK_ENTRY_ICON_PRIMARY, "system-users");
	gtk_entry_set_activates_default (GTK_ENTRY(username_entry), TRUE);

	rena_hig_workarea_table_add_row (table, &row, label, username_entry);

	label = gtk_label_new (_("Password"));

	password_entry = gtk_entry_new ();
	gtk_entry_set_icon_from_icon_name (GTK_ENTRY(password_entry), GTK_ENTRY_ICON_PRIMARY, "changes-prevent");
	gtk_entry_set_visibility (GTK_ENTRY(password_entry), FALSE);
	gtk_entry_set_activates_default (GTK_ENTRY(password_entry), TRUE);

	rena_hig_workarea_table_add_row (table, &row, label, password_entry);

	/* Init values */

	server = rena_subsonic_preferences_get_server (preferences);
	rena_gtk_entry_set_text (GTK_ENTRY(server_entry), server);
	g_free (server);

	username = rena_subsonic_preferences_get_username (preferences);
	rena_gtk_entry_set_text (GTK_ENTRY(username_entry), username);
	g_free (username);

	password = rena_subsonic_preferences_get_password (preferences);
	rena_gtk_entry_set_text (GTK_ENTRY(password_entry), password);
	g_free (password);

	/* Append pane */

	preferences->settings_widget = table;
	preferences->server_entry = server_entry;
	preferences->username_entry = username_entry;
	preferences->password_entry = password_entry;
}

static void
rena_subsonic_preferences_finalize (GObject *object)
{
	RenaSubsonicPreferences *preferences = RENA_SUBSONIC_PREFERENCES(object);

	rena_preferences_dialog_disconnect_handler (preferences->dialog,
	                                              G_CALLBACK(rena_subsonic_preferences_dialog_response),
	                                              preferences);

	rena_preferences_remove_services_setting (preferences->dialog,
	                                            preferences->settings_widget);

	G_OBJECT_CLASS(rena_subsonic_preferences_parent_class)->finalize(object);
}

static void
rena_subsonic_preferences_dispose (GObject *object)
{
	RenaSubsonicPreferences *preferences = RENA_SUBSONIC_PREFERENCES(object);

	if (preferences->preferences) {
		g_object_unref(preferences->preferences);
		preferences->preferences = NULL;
	}

	G_OBJECT_CLASS(rena_subsonic_preferences_parent_class)->dispose(object);
}

static void
rena_subsonic_preferences_class_init (RenaSubsonicPreferencesClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = rena_subsonic_preferences_finalize;
	object_class->dispose = rena_subsonic_preferences_dispose;

	signals[SIGNAL_SERVER_CHANGED] =
		g_signal_new ("server-changed",
		              G_TYPE_FROM_CLASS (object_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (RenaSubsonicPreferencesClass, server_changed),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);
	signals[SIGNAL_CREDENTIALS_CHANGED] =
		g_signal_new ("credentials-changed",
		              G_TYPE_FROM_CLASS (object_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (RenaSubsonicPreferencesClass, credentials_changed),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);
}

static void
rena_subsonic_preferences_init (RenaSubsonicPreferences *preferences)
{
	preferences->preferences = rena_preferences_get ();

	rena_subsonic_plugin_build_widget (preferences);
}

RenaSubsonicPreferences *
rena_subsonic_preferences_new (void)
{
	RenaPreferencesDialog *dialog;
	RenaSubsonicPreferences *preferences;

	preferences = RENA_SUBSONIC_PREFERENCES(g_object_new (RENA_TYPE_SUBSONIC_PREFERENCES, NULL));

	dialog = rena_preferences_dialog_get ();

	rena_preferences_append_services_setting (dialog,
	                                            preferences->settings_widget,
	                                            FALSE);

	rena_preferences_dialog_connect_handler (dialog,
	                                           G_CALLBACK(rena_subsonic_preferences_dialog_response),
	                                           preferences);

	preferences->dialog = dialog;

	return preferences;
}

