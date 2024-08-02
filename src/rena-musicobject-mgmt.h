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

#ifndef RENA_MUSICOBJECT_MGMT_H
#define RENA_MUSICOBJECT_MGMT_H

#include "rena-musicobject.h"
#include "rena-database.h"

/* Flags to control tags changed. */

#define TAG_TNO_CHANGED     1<<0
#define TAG_TITLE_CHANGED   1<<1
#define TAG_ARTIST_CHANGED  1<<2
#define TAG_ALBUM_CHANGED   1<<3
#define TAG_GENRE_CHANGED   1<<4
#define TAG_YEAR_CHANGED    1<<5
#define TAG_COMMENT_CHANGED 1<<6

RenaMusicobject *
new_musicobject_from_file                 (const gchar *file,
                                           const gchar *provider);

RenaMusicobject *
new_musicobject_from_db                   (RenaDatabase *cdbase,
                                           gint location_id);

RenaMusicobject *
new_musicobject_from_location             (const gchar *uri,
                                           const gchar *name);

RenaMusicobject *
rena_database_get_artist_and_title_song (RenaDatabase *cdbase,
                                           const gchar    *artist,
                                           const gchar    *title);

void
rena_update_musicobject_change_tag      (RenaMusicobject *mobj,
                                           gint               changed,
                                           RenaMusicobject *nmobj);

#endif /* RENA_MUSICOBJECT_MGMT_H */
