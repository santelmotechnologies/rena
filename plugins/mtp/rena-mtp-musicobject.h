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

#ifndef __RENA_MTP_MUSICOBJECT_H__
#define __RENA_MTP_MUSICOBJECT_H__

#include <libmtp.h>

#include "src/rena-musicobject.h"

G_BEGIN_DECLS

LIBMTP_track_t    *mtp_track_new_from_rena_musicobject (LIBMTP_mtpdevice_t *mtp_device, RenaMusicobject *mobj);
RenaMusicobject *rena_musicobject_new_from_mtp_track (LIBMTP_track_t *track);

gint               rena_mtp_plugin_get_track_id        (RenaMusicobject *mobj);
gchar             *rena_mtp_plugin_get_temp_filename   (RenaMusicobject *mobj);
gboolean           rena_musicobject_is_mtp_file        (RenaMusicobject *mobj);

G_END_DECLS

#endif /* __RENA_MTP_MUSICOBJECT_H__ */
