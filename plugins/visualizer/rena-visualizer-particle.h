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

#ifndef RENA_VISUALIZER_PARTICLE_H
#define RENA_VISUALIZER_PARTICLE_H

#include <gtk/gtk.h>

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define RENA_TYPE_PARTICLE (rena_particle_get_type())
#define RENA_PARTICLE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_PARTICLE, RenaParticle))
#define RENA_PARTICLE_CONST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_PARTICLE, RenaParticle const))
#define RENA_PARTICLE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), RENA_TYPE_PARTICLE, RenaParticleClass))
#define RENA_IS_PARTICLE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_PARTICLE))
#define RENA_IS_PARTICLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), RENA_TYPE_PARTICLE))
#define RENA_PARTICLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), RENA_TYPE_PARTICLE, RenaParticleClass))

typedef struct _RenaParticle RenaParticle;
typedef struct _RenaParticleClass RenaParticleClass;

struct _RenaParticleClass
{
	GObjectClass parent_class;
};

void
rena_particle_reset (RenaParticle *particle);

void
rena_particle_move_to (RenaParticle *particle, gint x, gint y);

void
rena_particle_move (RenaParticle *particle, guint width, guint height);

void
rena_particle_set_energy (RenaParticle *particle, gdouble energy);

void
rena_particle_draw (RenaParticle *particle, cairo_t *cr);

RenaParticle *
rena_particle_new (void);

G_END_DECLS

#endif /* RENA_VISUALIZER_PARTICLE_H */
