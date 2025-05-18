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

#ifndef __RENA_MTP_THREAD_DATA_H
#define __RENA_MTP_THREAD_DATA_H

/*
 * RenaMtpThreadOpenedData *
 */
typedef struct _RenaMtpThreadOpenedData RenaMtpThreadOpenedData;

RenaMtpThreadOpenedData *
rena_mtp_thread_opened_data_new (gpointer     user_data,
                                   const gchar *device_id,
                                   const gchar *friendly_name);

void
rena_mtp_thread_opened_data_free (RenaMtpThreadOpenedData *data);

gpointer
rena_mtp_thread_opened_data_get_user_data (RenaMtpThreadOpenedData *data);

const gchar *
rena_mtp_thread_opened_data_get_device_id (RenaMtpThreadOpenedData *data);

const gchar *
rena_mtp_thread_opened_data_get_friendly_name (RenaMtpThreadOpenedData *data);


/*
 * RenaMtpThreadTracklistData *
 */
typedef struct _RenaMtpThreadTracklistData RenaMtpThreadTracklistData;

RenaMtpThreadTracklistData *
rena_mtp_thread_tracklist_data_new (gpointer  user_data,
                                      GList    *list);

void
rena_mtp_thread_tracklist_data_free (RenaMtpThreadTracklistData *data);

gpointer
rena_mtp_thread_tracklist_data_get_user_data (RenaMtpThreadTracklistData *data);

GList *
rena_mtp_thread_tracklist_data_get_list (RenaMtpThreadTracklistData *data);


/*
 * RenaMtpThreadProgressData *
 */
typedef struct _RenaMtpThreadProgressData RenaMtpThreadProgressData;


RenaMtpThreadProgressData *
rena_mtp_thread_progress_data_new (gpointer user_data,
                                     guint    progress,
                                     guint    total);

void
rena_mtp_thread_progress_data_free (RenaMtpThreadProgressData *data);

gpointer
rena_mtp_thread_progress_data_get_user_data (RenaMtpThreadProgressData *data);

guint
rena_mtp_thread_progress_data_get_progress (RenaMtpThreadProgressData *data);

guint
rena_mtp_thread_progress_data_get_total (RenaMtpThreadProgressData *data);


/*
 * RenaMtpThreadDownloadData *
 */
typedef struct _RenaMtpThreadDownloadData  RenaMtpThreadDownloadData;

RenaMtpThreadDownloadData *
rena_mtp_thread_download_data_new (gpointer     user_data,
                                     const gchar *filename,
                                     const gchar *error);

void
rena_mtp_thread_download_data_free (RenaMtpThreadDownloadData *data);

gpointer
rena_mtp_thread_download_data_get_user_data (RenaMtpThreadDownloadData *data);

const gchar *
rena_mtp_thread_download_data_get_filename (RenaMtpThreadDownloadData *data);

const gchar *
rena_mtp_thread_download_data_get_error (RenaMtpThreadDownloadData *data);


/*
 * RenaMtpThreadUploadData *
 */
typedef struct _RenaMtpThreadUploadData  RenaMtpThreadUploadData;

RenaMtpThreadUploadData *
rena_mtp_thread_upload_data_new (gpointer           user_data,
                                   RenaMusicobject *mobj,
                                   const gchar       *error);

void
rena_mtp_thread_upload_data_free (RenaMtpThreadUploadData *data);

gpointer
rena_mtp_thread_upload_data_get_user_data (RenaMtpThreadUploadData *data);

RenaMusicobject *
rena_mtp_thread_upload_data_get_musicobject (RenaMtpThreadUploadData *data);

const gchar *
rena_mtp_thread_upload_data_get_error (RenaMtpThreadUploadData *data);


/*
 * RenaMtpThreadStatsData *
 */
typedef struct _RenaMtpThreadStatsData  RenaMtpThreadStatsData;

RenaMtpThreadStatsData *
rena_mtp_thread_stats_data_new (gpointer     user_data,
                                  const gchar *first_storage_description,
                                  guint64      first_storage_capacity,
                                  guint64      first_storage_free_space,
                                  const gchar *second_storage_description,
                                  guint64      second_storage_capacity,
                                  guint64      second_storage_free_space,
                                  guint8       maximum_battery_level,
                                  guint8       current_battery_level,
                                  const gchar *error);

void
rena_mtp_thread_stats_data_free (RenaMtpThreadStatsData *data);

gpointer
rena_mtp_thread_stats_data_get_user_data (RenaMtpThreadStatsData *data);

const gchar *
rena_mtp_thread_stats_data_get_first_storage_description (RenaMtpThreadStatsData *data);

guint64
rena_mtp_thread_stats_data_get_first_storage_capacity (RenaMtpThreadStatsData *data);

guint64
rena_mtp_thread_stats_data_get_first_storage_free_space (RenaMtpThreadStatsData *data);

const gchar *
rena_mtp_thread_stats_data_get_second_storage_description (RenaMtpThreadStatsData *data);

guint64
rena_mtp_thread_stats_data_get_second_storage_capacity (RenaMtpThreadStatsData *data);

guint64
rena_mtp_thread_stats_data_get_second_storage_free_space (RenaMtpThreadStatsData *data);

guint8
rena_mtp_thread_stats_data_get_maximun_battery_level (RenaMtpThreadStatsData *data);

guint8
rena_mtp_thread_stats_data_get_current_battery_level (RenaMtpThreadStatsData *data);

const gchar *
rena_mtp_thread_stats_data_get_error (RenaMtpThreadStatsData *data);

#endif // __RENA_MTP_THREAD_DATA_H

