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

#ifndef RENA_PLAYBACK_H
#define RENA_PLAYBACK_H

#include "rena-backend.h"
#include "rena-playlist.h"

#include "rena.h"

void rena_playback_set_playlist_track   (RenaPlaylist *playlist, RenaMusicobject *mobj, RenaApplication *rena);

void rena_playback_prev_track           (RenaApplication *rena);
void rena_playback_play_pause_resume    (RenaApplication *rena);
void rena_playback_stop                 (RenaApplication *rena);
void rena_playback_next_track           (RenaApplication *rena);
void rena_advance_playback              (RenaApplication *rena);

gboolean rena_playback_can_go_prev      (RenaApplication *rena);
gboolean rena_playback_can_go_next      (RenaApplication *rena);
gint     rena_playback_get_no_tracks    (RenaApplication *rena);

void rena_backend_finished_song         (RenaBackend *backend, RenaApplication *rena);
void rena_backend_finished_error        (RenaBackend *backend, const GError *error, RenaApplication *rena);

void rena_backend_tags_changed          (RenaBackend *backend, gint changed, RenaApplication *rena);

void rena_playback_show_current_album_art (GObject *object, RenaApplication *rena);
void rena_playback_edit_current_track   (RenaApplication *rena);

void rena_playback_seek_fraction        (GObject *object, gdouble fraction, RenaApplication *rena);
void rena_playback_toogle_favorite      (GObject *object, RenaApplication *rena);

#endif /* RENA_PLAYBACK_H */
