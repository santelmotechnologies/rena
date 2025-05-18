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

#include <libmtp.h>

#include <gio/gio.h>
#include <glib.h>
#include <glib-object.h>

#include "src/rena-musicobject.h"

#ifndef __RENA_MTP_THREAD_H
#define __RENA_MTP_THREAD_H

#define RENA_TYPE_MTP_THREAD         (rena_mtp_thread_get_type ())
#define RENA_MTP_THREAD(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), RENA_TYPE_MTP_THREAD, RenaMtpThread))
#define RENA_MTP_THREAD_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k),     RENA_TYPE_MTP_THREAD, RenaMtpThreadClass))
#define RENA_IS_MTP_THREAD(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), RENA_TYPE_MTP_THREAD))
#define RENA_IS_MTP_THREAD_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k),    RENA_TYPE_MTP_THREAD))
#define RENA_MTP_THREAD_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  RENA_TYPE_MTP_THREAD, RenaMtpThreadClass))

typedef struct _RenaMtpThread      RenaMtpThread;
typedef struct _RenaMtpThreadClass RenaMtpThreadClass;

/*
 * Public functions.
 */

RenaMtpThread *
rena_mtp_thread_new (void);

void
rena_mtp_thread_open_device    (RenaMtpThread           *thread,
                                  guint                      devnum,
                                  guint                      busnum,
                                  GSourceFunc                finish_func,
                                  gpointer                   user_data);

void
rena_mtp_thread_get_stats      (RenaMtpThread           *thread,
                                  GSourceFunc                finish_func,
                                  gpointer                   user_data);

void
rena_mtp_thread_get_track_list (RenaMtpThread           *thread,
                                  GSourceFunc                finish_func,
                                  GSourceFunc                progress_func,
                                  gpointer                   user_data);

void
rena_mtp_thread_download_track (RenaMtpThread           *thread,
                                  guint                      track_id,
                                  gchar                     *filename,
                                  GSourceFunc                finish_func,
                                  GSourceFunc                progress_func,
                                  gpointer                   data);

void
rena_mtp_thread_upload_track   (RenaMtpThread           *thread,
                                  RenaMusicobject         *mobj,
                                  GSourceFunc                finish_func,
                                  gpointer                   data);

void
rena_mtp_thread_close_device   (RenaMtpThread          *thread,
                                  GSourceFunc               finish_func,
                                  gpointer                  data);

void
rena_mtp_thread_report_errors  (RenaMtpThread           *thread);

#endif // __RENA_MTP_THREAD_H

