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

#ifndef __RENA_SUBSONIC_API_H__
#define __RENA_SUBSONIC_API_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef enum {
	S_USER_CANCELLED = -2,
	S_GENERIC_OK = -1,
	S_GENERIC_ERROR = 0,
	S_PARAMETER_MISSING = 10,
	S_CLIENT_MUST_UPGRADE = 20,
	S_SERVER_MUST_UPGRADE = 30,
	S_WROMG_CREDENTIAL = 40,
	S_NOT_SUPPORT_LDAP_USER = 41,
	S_OPERATION_NOT_AUTHORIZED = 50,
	S_TRIAL_OVER = 60,
	S_DATA_NOT_FOUND = 70
} SubsonicStatusCode;

#define RENA_TYPE_SUBSONIC_API            (rena_subsonic_api_get_type())
#define RENA_SUBSONIC_API(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_SUBSONIC_API, RenaSubsonicApi))
#define RENA_SUBSONIC_API_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_SUBSONIC_API, RenaSubsonicApi const))
#define RENA_SUBSONIC_API_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  RENA_TYPE_SUBSONIC_API, RenaSubsonicApiClass))
#define RENA_IS_SUBSONIC_API(obj) (        G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_SUBSONIC_API))
#define RENA_IS_SUBSONIC_API_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  RENA_TYPE_SUBSONIC_API))
#define RENA_SUBSONIC_API_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  RENA_TYPE_SUBSONIC_API, RenaSubsonicApiClass))

typedef struct _RenaSubsonicApi      RenaSubsonicApi;
typedef struct _RenaSubsonicApiClass RenaSubsonicApiClass;

struct _RenaSubsonicApiClass
{
	GObjectClass           parent_class;

	void (*authenticated) (RenaSubsonicApi *subsonic, SubsonicStatusCode code);
	void (*pong)          (RenaSubsonicApi *subsonic, SubsonicStatusCode code);
	void (*scan_progress) (RenaSubsonicApi *subsonic, gint progress);
	void (*scan_total)    (RenaSubsonicApi *subsonic, gint total);
	void (*scan_finished) (RenaSubsonicApi *subsonic, SubsonicStatusCode code);
};


/*
 * Helpers
 */

gchar *
rena_subsonic_api_get_playback_url      (RenaSubsonicApi *subsonic,
                                           const gchar       *friendly_url);


/*
 * Public methods
 */

RenaSubsonicApi *
rena_subsonic_api_new (void);

void
rena_subsonic_api_authentication        (RenaSubsonicApi *subsonic,
                                           const gchar       *server,
                                           const gchar       *username,
                                           const gchar       *password);

void
rena_subsonic_api_deauthentication      (RenaSubsonicApi *subsonic);

void
rena_subsonic_api_ping_server           (RenaSubsonicApi *subsonic);

void
rena_subsonic_api_scan_server           (RenaSubsonicApi *subsonic);

void
rena_subsonic_api_cancel                (RenaSubsonicApi *subsonic);


gboolean
rena_subsonic_api_is_authtenticated     (RenaSubsonicApi *subsonic);

gboolean
rena_subsonic_api_is_connected          (RenaSubsonicApi *subsonic);

gboolean
rena_subsonic_api_is_scanning           (RenaSubsonicApi *subsonic);


GCancellable *
rena_subsonic_get_cancellable           (RenaSubsonicApi *subsonic);

GSList *
rena_subsonic_api_get_songs_list        (RenaSubsonicApi *subsonic);

G_END_DECLS

#endif /* __RENA_SUBSONIC_API_H__ */
