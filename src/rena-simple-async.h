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

#ifndef RENA_SIMPLE_ASYNC_H
#define RENA_SIMPLE_ASYNC_H

#include <glib.h>

typedef struct _AsyncSimple AsyncSimple;

typedef struct _IdleMessage IdleMessage;

IdleMessage *
rena_idle_message_new (gchar    *title,
                         gchar    *message,
                         gboolean  transient);

void
rena_idle_message_free (IdleMessage *im);

gboolean rena_async_set_idle_message (gpointer user_data);

void     rena_async_launch           (GThreadFunc worker_func, GSourceFunc finish_func, gpointer userdata);

GThread *rena_async_launch_full      (GThreadFunc worker_func, GSourceFunc finish_func, gpointer userdata);

#endif /* RENA_SIMPLE_ASYNC_H */
