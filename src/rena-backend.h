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

#ifndef RENA_BACKEND_H
#define RENA_BACKEND_H

#include <gst/gst.h>
#include <glib-object.h>
#include "rena-musicobject.h"

G_BEGIN_DECLS

typedef enum {
	ST_PLAYING = 1,
	ST_STOPPED,
	ST_PAUSED,
	ST_BUFFERING
} RenaBackendState;

#define RENA_TYPE_BACKEND                  (rena_backend_get_type ())
#define RENA_BACKEND(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_BACKEND, RenaBackend))
#define RENA_IS_BACKEND(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_BACKEND))
#define RENA_BACKEND_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), RENA_TYPE_BACKEND, RenaBackendClass))
#define RENA_IS_BACKEND_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), RENA_TYPE_BACKEND))
#define RENA_BACKEND_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), RENA_TYPE_BACKEND, RenaBackendClass))

struct RenaBackendPrivate;
typedef struct RenaBackendPrivate RenaBackendPrivate;

typedef struct {
	GObject parent;
	RenaBackendPrivate *priv;
} RenaBackend;

typedef struct {
	GObjectClass parent_class;
	void (*set_device) (RenaBackend *backend, GObject *obj);
	void (*prepare_source) (RenaBackend *backend);
	void (*clean_source) (RenaBackend *backend);
	void (*tick) (RenaBackend *backend);
	void (*half_played) (RenaBackend *backend);
	void (*seeked) (RenaBackend *backend);
	void (*buffering) (RenaBackend *backend, gint percent);
	void (*download_done) (RenaBackend *backend, gchar *filename);
	void (*finished) (RenaBackend *backend);
	void (*error) (RenaBackend *backend, const GError *error);
	void (*tags_changed) (RenaBackend *backend, gint changed);
	void (*spectrum) (RenaBackend *backend, gpointer value);
} RenaBackendClass;

gboolean           rena_backend_can_seek             (RenaBackend *backend);
void               rena_backend_seek                 (RenaBackend *backend, gint64 seek);

gint64             rena_backend_get_current_length   (RenaBackend *backend);
gint64             rena_backend_get_current_position (RenaBackend *backend);

void               rena_backend_set_local_storage    (RenaBackend *backend, gboolean local_storage);

void               rena_backend_set_soft_volume      (RenaBackend *backend, gboolean value);
gdouble            rena_backend_get_volume           (RenaBackend *backend);
void               rena_backend_set_volume           (RenaBackend *backend, gdouble volume);
void               rena_backend_set_delta_volume     (RenaBackend *backend, gdouble delta);

gboolean           rena_backend_emitted_error        (RenaBackend *backend);
GError            *rena_backend_get_error            (RenaBackend *backend);
RenaBackendState rena_backend_get_state            (RenaBackend *backend);

void               rena_backend_pause                (RenaBackend *backend);
void               rena_backend_resume               (RenaBackend *backend);
void               rena_backend_play                 (RenaBackend *backend);
void               rena_backend_stop                 (RenaBackend *backend);

void               rena_backend_set_playback_uri     (RenaBackend *backend, const gchar *uri);
void               rena_backend_set_musicobject      (RenaBackend *backend, RenaMusicobject *mobj);
RenaMusicobject *rena_backend_get_musicobject      (RenaBackend *backend);

GstElement        *rena_backend_get_equalizer        (RenaBackend *backend);
void               rena_backend_update_equalizer     (RenaBackend *backend, const gdouble *bands);
GstElement        *rena_backend_get_preamp           (RenaBackend *backend);

void               rena_backend_enable_spectrum      (RenaBackend *backend);
void               rena_backend_disable_spectrum     (RenaBackend *backend);

RenaBackend     *rena_backend_new                  (void);

G_END_DECLS

#endif /* RENA_BACKEND_H */
