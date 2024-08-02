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

#ifndef __RENA_SUBSONIC_PREFERENCES_H__
#define __RENA_SUBSONIC_PREFERENCES_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define RENA_TYPE_SUBSONIC_PREFERENCES            (rena_subsonic_preferences_get_type())
#define RENA_SUBSONIC_PREFERENCES(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_SUBSONIC_PREFERENCES, RenaSubsonicPreferences))
#define RENA_SUBSONIC_PREFERENCES_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_SUBSONIC_PREFERENCES, RenaSubsonicPreferences const))
#define RENA_SUBSONIC_PREFERENCES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  RENA_TYPE_SUBSONIC_PREFERENCES, RenaSubsonicPreferencesClass))
#define RENA_IS_SUBSONIC_PREFERENCES(obj) (        G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_SUBSONIC_PREFERENCES))
#define RENA_IS_SUBSONIC_PREFERENCES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  RENA_TYPE_SUBSONIC_PREFERENCES))
#define RENA_SUBSONIC_PREFERENCES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  RENA_TYPE_SUBSONIC_PREFERENCES, RenaSubsonicPreferencesClass))

typedef struct _RenaSubsonicPreferences      RenaSubsonicPreferences;
typedef struct _RenaSubsonicPreferencesClass RenaSubsonicPreferencesClass;

struct _RenaSubsonicPreferencesClass
{
	GObjectClass                 parent_class;

	void (*server_changed)      (RenaSubsonicPreferences *subsonic);
	void (*credentials_changed) (RenaSubsonicPreferences *subsonic);
};


/*
 * Public methods
 */

const gchar *
rena_subsonic_preferences_get_server_text   (RenaSubsonicPreferences *preferences);

const gchar *
rena_subsonic_preferences_get_username_text (RenaSubsonicPreferences *preferences);

const gchar *
rena_subsonic_preferences_get_password_text (RenaSubsonicPreferences *preferences);

void
rena_subsonic_preferences_forget_settings   (RenaSubsonicPreferences *preferences);

RenaSubsonicPreferences *
rena_subsonic_preferences_new               (void);

G_END_DECLS

#endif /* __RENA_SUBSONIC_PREFERENCES_H__ */
