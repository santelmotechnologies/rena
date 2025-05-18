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

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdlib.h>

#include <glib.h>

#include "src/rena-musicobject.h"

#include "rena-mtp-thread-data.h"

/*
 * RenaMtpThreadOpenedData *
 */
struct _RenaMtpThreadOpenedData {
	gpointer  user_data;
	gchar    *device_id;
	gchar    *friendly_name;
};

RenaMtpThreadOpenedData *
rena_mtp_thread_opened_data_new (gpointer     user_data,
                                   const gchar *device_id,
                                   const gchar *friendly_name)
{
	RenaMtpThreadOpenedData *data;

	data = g_slice_new (RenaMtpThreadOpenedData);

	data->user_data = user_data;
	data->device_id = g_strdup(device_id);
	data->friendly_name = g_strdup(friendly_name);

	return data;
}

void
rena_mtp_thread_opened_data_free (RenaMtpThreadOpenedData *data)
{
	g_free (data->device_id);
	g_free (data->friendly_name);

	g_slice_free (RenaMtpThreadOpenedData, data);
}

gpointer
rena_mtp_thread_opened_data_get_user_data (RenaMtpThreadOpenedData *data)
{
	return data->user_data;
}

const gchar *
rena_mtp_thread_opened_data_get_device_id (RenaMtpThreadOpenedData *data)
{
	return data->device_id;
}

const gchar *
rena_mtp_thread_opened_data_get_friendly_name (RenaMtpThreadOpenedData *data)
{
	return data->friendly_name;
}


/*
 * RenaMtpThreadTracklistData *
 */
struct _RenaMtpThreadTracklistData {
	gpointer user_data;
	GList   *list;
};

RenaMtpThreadTracklistData *
rena_mtp_thread_tracklist_data_new (gpointer  user_data,
                                      GList    *list)
{
	RenaMtpThreadTracklistData *data;

	data = g_slice_new (RenaMtpThreadTracklistData);

	data->user_data = user_data;
	data->list = list;

	return data;
}

void
rena_mtp_thread_tracklist_data_free (RenaMtpThreadTracklistData *data)
{
	g_slice_free (RenaMtpThreadTracklistData, data);
}

gpointer
rena_mtp_thread_tracklist_data_get_user_data (RenaMtpThreadTracklistData *data)
{
	return data->user_data;
}

GList *
rena_mtp_thread_tracklist_data_get_list (RenaMtpThreadTracklistData *data)
{
	return data->list;
}


/*
 * RenaMtpThreadProgressData *
 */
struct _RenaMtpThreadProgressData {
	gpointer user_data;
	guint    progress;
	guint    total;
};

RenaMtpThreadProgressData *
rena_mtp_thread_progress_data_new (gpointer user_data,
                                     guint    progress,
                                     guint    total)
{
	RenaMtpThreadProgressData *data;

	data = g_slice_new (RenaMtpThreadProgressData);

	data->user_data = user_data;
	data->progress = progress;
	data->total = total;

	return data;
}

void
rena_mtp_thread_progress_data_free (RenaMtpThreadProgressData *data)
{
	g_slice_free (RenaMtpThreadProgressData, data);
}

gpointer
rena_mtp_thread_progress_data_get_user_data (RenaMtpThreadProgressData *data)
{
	return data->user_data;
}

guint
rena_mtp_thread_progress_data_get_progress (RenaMtpThreadProgressData *data)
{
	return data->progress;
}

guint
rena_mtp_thread_progress_data_get_total (RenaMtpThreadProgressData *data)
{
	return data->total;
}


/*
 * RenaMtpThreadDownloadData *
 */
struct _RenaMtpThreadDownloadData {
	gpointer  user_data;
	gchar    *filename;
	gchar    *error;
};

RenaMtpThreadDownloadData *
rena_mtp_thread_download_data_new (gpointer     user_data,
                                     const gchar *filename,
                                     const gchar *error)
{
	RenaMtpThreadDownloadData *data;

	data = g_slice_new (RenaMtpThreadDownloadData);

	data->user_data = user_data;
	data->filename = g_strdup(filename);
	data->error = g_strdup (error);

	return data;
}

void
rena_mtp_thread_download_data_free (RenaMtpThreadDownloadData *data)
{
	g_free (data->filename);
	g_free (data->error);

	g_slice_free (RenaMtpThreadDownloadData, data);
}

gpointer
rena_mtp_thread_download_data_get_user_data (RenaMtpThreadDownloadData *data)
{
	return data->user_data;
}

const gchar *
rena_mtp_thread_download_data_get_filename (RenaMtpThreadDownloadData *data)
{
	return data->filename;
}

const gchar *
rena_mtp_thread_download_data_get_error (RenaMtpThreadDownloadData *data)
{
	return data->error;
}


/*
 * RenaMtpThreadUploadData *
 */
struct _RenaMtpThreadUploadData {
	gpointer           user_data;
	RenaMusicobject *mobj;
	gchar             *error;
};

RenaMtpThreadUploadData *
rena_mtp_thread_upload_data_new (gpointer           user_data,
                                   RenaMusicobject *mobj,
                                   const gchar       *error)
{
	RenaMtpThreadUploadData *data;

	data = g_slice_new (RenaMtpThreadUploadData);

	data->user_data = user_data;
	if (mobj)
		data->mobj = g_object_ref(mobj);
	data->error = g_strdup (error);

	return data;
}

void
rena_mtp_thread_upload_data_free (RenaMtpThreadUploadData *data)
{
	if (data->mobj)
		g_object_unref (data->mobj);
	g_free (data->error);

	g_slice_free (RenaMtpThreadUploadData, data);
}

gpointer
rena_mtp_thread_upload_data_get_user_data (RenaMtpThreadUploadData *data)
{
	return data->user_data;
}

RenaMusicobject *
rena_mtp_thread_upload_data_get_musicobject (RenaMtpThreadUploadData *data)
{
	return data->mobj;
}

const gchar *
rena_mtp_thread_upload_data_get_error (RenaMtpThreadUploadData *data)
{
	return data->error;
}

/*
 * RenaMtpThreadStatsData *
 */
struct _RenaMtpThreadStatsData {
	gpointer  user_data;
	gchar    *first_storage_description;
	guint64   first_storage_capacity;
	guint64   first_storage_free_space;
	gchar    *second_storage_description;
	guint64   second_storage_capacity;
	guint64   second_storage_free_space;
	guint8    maximum_battery_level;
	guint8    current_battery_level;
	gchar    *error;
};

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
                                  const gchar *error)
{
	RenaMtpThreadStatsData *data;

	data = g_slice_new (RenaMtpThreadStatsData);

	data->user_data = user_data;
	data->first_storage_description = g_strdup (first_storage_description);
	data->first_storage_capacity = first_storage_capacity;
	data->first_storage_free_space = first_storage_free_space;
	data->second_storage_description = g_strdup (second_storage_description);
	data->second_storage_capacity = second_storage_capacity;
	data->second_storage_free_space = second_storage_free_space;
	data->maximum_battery_level = maximum_battery_level;
	data->current_battery_level = current_battery_level;
	data->error = g_strdup (error);

	return data;
}

void
rena_mtp_thread_stats_data_free (RenaMtpThreadStatsData *data)
{
	g_free (data->first_storage_description);
	g_free (data->second_storage_description);
	g_free (data->error);

	g_slice_free (RenaMtpThreadStatsData, data);
}

gpointer
rena_mtp_thread_stats_data_get_user_data (RenaMtpThreadStatsData *data)
{
	return data->user_data;
}

const gchar *
rena_mtp_thread_stats_data_get_first_storage_description (RenaMtpThreadStatsData *data)
{
	return data->first_storage_description;
}

guint64
rena_mtp_thread_stats_data_get_first_storage_capacity (RenaMtpThreadStatsData *data)
{
	return data->first_storage_capacity;
}

guint64
rena_mtp_thread_stats_data_get_first_storage_free_space (RenaMtpThreadStatsData *data)
{
	return data->first_storage_free_space;
}

const gchar *
rena_mtp_thread_stats_data_get_second_storage_description (RenaMtpThreadStatsData *data)
{
	return data->second_storage_description;
}

guint64
rena_mtp_thread_stats_data_get_second_storage_capacity (RenaMtpThreadStatsData *data)
{
	return data->second_storage_capacity;
}

guint64
rena_mtp_thread_stats_data_get_second_storage_free_space (RenaMtpThreadStatsData *data)
{
	return data->second_storage_free_space;
}

guint8
rena_mtp_thread_stats_data_get_maximun_battery_level (RenaMtpThreadStatsData *data)
{
	return data->maximum_battery_level;
}

guint8
rena_mtp_thread_stats_data_get_current_battery_level (RenaMtpThreadStatsData *data)
{
	return data->current_battery_level;
}

const gchar *
rena_mtp_thread_stats_data_get_error (RenaMtpThreadStatsData *data)
{
	return data->error;
}

