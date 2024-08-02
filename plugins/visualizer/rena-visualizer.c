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

/*
 * This code is completely based on the Koel audio visualization..
 * https://github.com/koel/core/blob/master/js/utils/visualizer.js
 *
 * Just Thanks!.
 */

#include <gmodule.h>
#include <math.h>

#include "src/rena-backend.h"

#include "rena-visualizer-particle.h"
#include "rena-visualizer.h"


#define RENA_TYPE_VISUALIZER (rena_visualizer_get_type())
#define RENA_VISUALIZER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_VISUALIZER, RenaVisualizer))
#define RENA_VISUALIZER_CONST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), RENA_TYPE_VISUALIZER, RenaVisualizer const))
#define RENA_VISUALIZER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), RENA_TYPE_VISUALIZER, RenaVisualizerClass))
#define RENA_IS_VISUALIZER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RENA_TYPE_VISUALIZER))
#define RENA_IS_VISUALIZER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), RENA_TYPE_VISUALIZER))
#define RENA_VISUALIZER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), RENA_TYPE_VISUALIZER, RenaVisualizerClass))

struct _RenaVisualizerClass {
	 GtkBoxClass     parent_class;
};
struct _RenaVisualizer {
	GtkBox         _parent;

	GtkWidget      *drawing_area;
	GList          *particles;

	guint           width;
	guint           height;

	guint           tick_id;
};

G_DEFINE_TYPE(RenaVisualizer, rena_visualizer, GTK_TYPE_BOX)

void
rena_visualizer_set_magnitudes (RenaVisualizer *visualizer, GValue *magnitudes)
{
	RenaParticle *particle = NULL;
	const GValue *mag = NULL;
	GList *l = NULL;
	guint i = 0;
	gdouble dmag = 0.0;

	if (!gtk_widget_is_visible (GTK_WIDGET(visualizer)))
		return;

	for (l = visualizer->particles, i = 0 ; l != NULL ; l = l->next, i++)
	{
		particle = l->data;
		mag = gst_value_list_get_value (magnitudes, i);
		if (mag != NULL)
			dmag = 80.0 + g_value_get_float (mag);
		else
			dmag = 0.0;

		rena_particle_set_energy (particle, dmag/128);
//		rena_particle_move (particle, visualizer->width, visualizer->height);
	}

	gtk_widget_queue_draw (GTK_WIDGET(visualizer->drawing_area));
}

void
rena_visualizer_stop (RenaVisualizer *visualizer)
{
	RenaParticle *particle = NULL;
	GList *l = NULL;
	for (l = visualizer->particles ; l != NULL ; l = l->next)
	{
		particle = l->data;
		rena_particle_set_energy (particle, 0.0);
	}
}

static gboolean
rena_visualizer_widget_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
	RenaParticle *particle = NULL;
	GList *l = NULL;

	RenaVisualizer *visualizer = RENA_VISUALIZER (user_data);

	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

	cairo_set_tolerance (cr, 1.0);
	cairo_set_antialias (cr, CAIRO_ANTIALIAS_FAST);

	cairo_rectangle (cr, 0, 0, visualizer->width, visualizer->height);
	cairo_fill (cr);

	cairo_set_operator(cr, CAIRO_OPERATOR_ADD);

	for (l = visualizer->particles ; l != NULL ; l = l->next)
	{
		particle = l->data;
		rena_particle_move (particle, visualizer->width, visualizer->height);
		rena_particle_draw (particle, cr);
	}

	return TRUE;
}

static gboolean
rena_visualizer_drawing_tick (gpointer user_data)
{
	GtkWidget *widget = GTK_WIDGET (user_data);

	if (gtk_widget_is_visible (widget))
		gtk_widget_queue_draw(widget);

	return G_SOURCE_CONTINUE;
}

static void
rena_visualizer_size_allocate (GtkWidget *widget, GdkRectangle *allocation, gpointer user_data)
{
	RenaParticle *particle = NULL;
	GList *l = NULL;
	gint x = 0, y = 0;

	RenaVisualizer *visualizer = RENA_VISUALIZER (user_data);

	visualizer->width = allocation->width;
	visualizer->height = allocation->height;

	for (l = visualizer->particles ; l != NULL ; l = l->next)
	{
		particle = l->data;
		x = g_random_int_range (1, visualizer->width);
		y = g_random_int_range (1, visualizer->height);
		rena_particle_reset (particle);
		rena_particle_move_to (particle, x, y);
	}
}

static void
rena_visualizer_dispose (GObject *object)
{
	RenaVisualizer *visualizer = RENA_VISUALIZER (object);

	if (visualizer->tick_id) {
		g_source_remove (visualizer->tick_id);
		visualizer->tick_id = 0;
	}

	if (visualizer->particles) {
		g_list_free_full (visualizer->particles, g_object_unref);
		visualizer->particles = NULL;
	}
	G_OBJECT_CLASS (rena_visualizer_parent_class)->dispose (object);
}

static void
rena_visualizer_class_init (RenaVisualizerClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (class);
	gobject_class->dispose = rena_visualizer_dispose;
}

static void
rena_visualizer_init (RenaVisualizer *visualizer)
{
	GtkWidget *drawing_area;
	RenaParticle *particle = NULL;
	gint i = 0;

	drawing_area = gtk_drawing_area_new ();
	gtk_widget_set_size_request (drawing_area, 640, 480);
	gtk_widget_set_hexpand (drawing_area, TRUE);

	g_signal_connect (drawing_area, "size-allocate",
	                  G_CALLBACK(rena_visualizer_size_allocate), visualizer);
	g_signal_connect (G_OBJECT (drawing_area), "draw",
	                  G_CALLBACK (rena_visualizer_widget_draw), visualizer);

	visualizer->tick_id = g_timeout_add (11, rena_visualizer_drawing_tick, drawing_area);

	for (i = 0 ; i < 128 ; i++) {
		particle = rena_particle_new ();
		rena_particle_set_energy (particle, g_random_double_range (0, i));
		visualizer->particles = g_list_append (visualizer->particles, particle);
	}

	visualizer->drawing_area = drawing_area;
	gtk_widget_set_visible (drawing_area, TRUE);

	gtk_container_add(GTK_CONTAINER(visualizer), drawing_area);
}

RenaVisualizer *
rena_visualizer_new (void)
{
	return g_object_new (RENA_TYPE_VISUALIZER, NULL);
}
