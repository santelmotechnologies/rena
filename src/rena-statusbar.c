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

#include "rena-statusbar.h"
#include "rena-background-task-bar.h"

#include "rena-preferences.h"

struct _RenaStatusbarClass
{
	GtkStatusbarClass __parent__;
};

struct _RenaStatusbar
{
	GtkBox    __parent__;

	GtkWidget *label;
};

G_DEFINE_TYPE (RenaStatusbar, rena_statusbar, GTK_TYPE_BOX)

static void
rena_statusbar_class_init (RenaStatusbarClass *klass)
{
}

static void
rena_statusbar_init (RenaStatusbar *statusbar)
{
	GtkStyleContext *context;

	statusbar->label = gtk_label_new (NULL);
	g_object_set (statusbar->label,
	              "margin-top", 2,
	              "margin-bottom", 2,
	              "margin-start", 12,
	              "margin-end", 12,
	              NULL);

	gtk_container_add(GTK_CONTAINER(statusbar), statusbar->label);

	context = gtk_widget_get_style_context (GTK_WIDGET (statusbar));
	gtk_style_context_add_class (context, "floating-bar");

	gtk_widget_show_all (GTK_WIDGET(statusbar));
}

/**
 * rena_statusbar_set_main_text:
 * @statusbar : a #RenaStatusbar instance.
 * @text      : the main text to be displayed in @statusbar.
 *
 * Sets up a new main text for @statusbar.
 **/
void
rena_statusbar_set_main_text (RenaStatusbar *statusbar,
                                const gchar     *text)
{
	g_return_if_fail (RENA_IS_STATUSBAR (statusbar));
	g_return_if_fail (text != NULL);

	gtk_label_set_text (GTK_LABEL (statusbar->label), text);
}

/**
 * rena_statusbar_get:
 *
 * Queries the global #GtkStatusbar instance, which is shared
 * by all modules. The function automatically takes a reference
 * for the caller, so you'll need to call g_object_unref() when
 * you're done with it.
 *
 * Return value: the global #GtkStatusbar instance.
 **/

RenaStatusbar *
rena_statusbar_get (void)
{
	static RenaStatusbar *statusbar = NULL;

	if (G_UNLIKELY (statusbar == NULL)) {
		statusbar = g_object_new(RENA_TYPE_STATUSBAR, NULL);
		g_object_add_weak_pointer(G_OBJECT (statusbar),
		                          (gpointer) &statusbar);
	}
	else {
		g_object_ref (G_OBJECT(statusbar));
	}
	return statusbar;
}
