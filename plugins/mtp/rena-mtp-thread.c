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
#include <glib/gi18n.h>

#include "src/rena-debug.h"
#include "src/rena-utils.h"

#include "rena-mtp-thread-data.h"
#include "rena-mtp-musicobject.h"

#include "rena-mtp-thread.h"


struct _RenaMtpThread {
	GObject             parent;

	LIBMTP_mtpdevice_t *device;

	gchar              *device_id;

	GThread            *thread;

	GAsyncQueue        *queue;
};

struct _RenaMtpThreadClass {
	GObjectClass parent;
};

G_DEFINE_TYPE(RenaMtpThread, rena_mtp_thread, G_TYPE_OBJECT)


/*
 * Task definitions.
 */

typedef enum {
	OPEN_DEVICE = 1,
	GET_TRACK_LIST,
	GET_STATS,
	DOWNLOAD_TRACK,
	UPLOAD_TRACK,
	CLOSE_DEVICE,
	SHUTDOWN,
} RenaMtpTaskType;

typedef struct {
	RenaMtpTaskType    task;

	guint                devnum;
	guint                busnum;

	RenaMusicobject   *mobj;

	guint                track_id;
	gchar               *filename;

	gpointer             callback;
	gpointer             progress_callback;
	gpointer             user_data;
} RenaMtpThreadTask;

static char *
task_name (RenaMtpThreadTask *task)
{
	switch (task->task) {
		case OPEN_DEVICE:
			return g_strdup ("open device");
		case GET_STATS:
			return g_strdup ("get stats device");
		case GET_TRACK_LIST:
			return g_strdup ("get track list");
		case DOWNLOAD_TRACK:
			return g_strdup_printf ("download track %u to %s",
			                        task->track_id,
			                        task->filename[0] ? task->filename : "<temporary>");
		case UPLOAD_TRACK:
			return g_strdup_printf ("upload track");
		case CLOSE_DEVICE:
			return g_strdup ("close device");
		case SHUTDOWN:
			return g_strdup ("shutdown thread");
		default:
			return g_strdup_printf ("unknown task type %d", task->task);
	}
}


/*
 * Tasks state machine...
 */

static RenaMtpThreadTask *
create_task (guint tasktype)
{
	RenaMtpThreadTask *task = g_slice_new0 (RenaMtpThreadTask);
	task->task = tasktype;
	return task;
}

static void
destroy_task (RenaMtpThreadTask *task)
{
	CDEBUG(DBG_PLUGIN, "Mtp thread %s", G_STRFUNC);

	if (task->filename) {
		g_free (task->filename);
	}

	if (task->mobj) {
		g_object_unref(G_OBJECT(task->mobj));
	}

	g_slice_free (RenaMtpThreadTask, task);
}

static void
queue_task (RenaMtpThread *thread, RenaMtpThreadTask *task)
{
	char *name = task_name (task);
	CDEBUG(DBG_PLUGIN, "Mtp thread queueing task: %s", name);
	g_free (name);

	g_async_queue_push (thread->queue, task);
}

static void
open_device (RenaMtpThread *thread, RenaMtpThreadTask *task)
{
	RenaMtpThreadOpenedData *data;
	LIBMTP_raw_device_t *raw_devices;
	LIBMTP_mtpdevice_t *device = NULL;
	LIBMTP_devicestorage_t *storage;
	gchar *device_id = NULL, *friendly_name = NULL;
	guint64 freeSpace = 0;
	gint num_raw_devices = 0, i = 0;

	CDEBUG(DBG_PLUGIN, "Mtp thread %s", G_STRFUNC);

	LIBMTP_Detect_Raw_Devices (&raw_devices, &num_raw_devices);
	for (i = 0; i < num_raw_devices; i++)
	{
		if (raw_devices[i].bus_location == task->busnum &&
		    raw_devices[i].devnum == task->devnum)
		{
			/*
			 * Open device
			 */
			device = LIBMTP_Open_Raw_Device_Uncached (&raw_devices[i]);
			if (device == NULL)
				continue;

			/*
			 * Check storage and ignore just charging devices.
			 */
			if (!LIBMTP_Get_Storage (device, LIBMTP_STORAGE_SORTBY_FREESPACE)) {
				LIBMTP_Dump_Errorstack (device);
				LIBMTP_Clear_Errorstack (device);
			}
			for (storage = device->storage; storage != 0; storage = storage->next) {
				freeSpace += storage->FreeSpaceInBytes;
			}
			if (!freeSpace) {
				LIBMTP_Release_Device (device);
				device = NULL;
			}

			if (device != NULL) {
				break;
			}
		}
	}

	if (device) {
		device_id = LIBMTP_Get_Serialnumber (device);
		friendly_name = LIBMTP_Get_Friendlyname (device);
		if (!friendly_name)
			friendly_name = LIBMTP_Get_Modelname (device);
		if (!friendly_name)
			friendly_name = g_strdup (_("Unknown MTP device"));

		thread->device = device;
		thread->device_id = g_strdup (device_id);
	}

	data = rena_mtp_thread_opened_data_new (task->user_data, device_id, friendly_name);
	g_idle_add ((GSourceFunc) task->callback, data);

	g_free (device_id);
	g_free (friendly_name);
}

static void
get_stats (RenaMtpThread *thread, RenaMtpThreadTask *task)
{
	RenaMtpThreadStatsData *data;
	LIBMTP_devicestorage_t *storage;
	guint storagei = 0;
	gchar *first_storage_description = NULL;
	guint64 first_storage_capacity, first_storage_free_space;
	gchar *second_storage_description = NULL;
	guint64 second_storage_capacity = 0, second_storage_free_space = 0;
	guint8 maximum_battery_level = 0, current_battery_level = 0;

	CDEBUG(DBG_PLUGIN, "Mtp thread %s", G_STRFUNC);

	LIBMTP_Get_Storage (thread->device, LIBMTP_STORAGE_SORTBY_FREESPACE);
	for (storage = thread->device->storage; storage != 0; storage = storage->next, storagei++) {
		if (storagei == 0) {
			first_storage_description = g_strdup(storage->StorageDescription);
			first_storage_capacity = storage->MaxCapacity;
			first_storage_free_space = storage->FreeSpaceInBytes;
		}
		else if (storagei == 1) {
			second_storage_description = g_strdup(storage->StorageDescription);
			second_storage_capacity = storage->MaxCapacity;
			second_storage_free_space = storage->FreeSpaceInBytes;
		}
		else {
			CDEBUG(DBG_PLUGIN, "Mtp plugin we ignore the storage %s", storage->StorageDescription);
		}
	}

	if (!LIBMTP_Get_Batterylevel(thread->device, &maximum_battery_level, &current_battery_level)) {
		// Silently ignore. Some devices does not support getting the battery level.
		LIBMTP_Clear_Errorstack(thread->device);
	}

	data = rena_mtp_thread_stats_data_new (task->user_data,
	                                         first_storage_description,
	                                         first_storage_capacity,
	                                         first_storage_free_space,
	                                         second_storage_description,
	                                         second_storage_capacity,
	                                         second_storage_free_space,
	                                         maximum_battery_level,
	                                         current_battery_level,
	                                         NULL);
	g_idle_add ((GSourceFunc) task->callback, data);

	g_free (first_storage_description);
	g_free (second_storage_description);
}

static GList *
get_track_list_recursive (RenaMtpThread     *thread,
                          RenaMtpThreadTask *task,
                          guint                storageid,
                          gint                 leaf,
                          GList               *list)
{
	RenaMtpThreadProgressData *data;
	RenaMusicobject *mobj = NULL;
	LIBMTP_file_t *folders = NULL, *lfolder = NULL, *audios = NULL, *laudio = NULL;
	LIBMTP_file_t *files, *file, *tmp;
	gboolean nomedia = FALSE;

	files = LIBMTP_Get_Files_And_Folders (thread->device,
	                                      storageid,
	                                      leaf);

	if (!files) {
		rena_mtp_thread_report_errors (thread);
		return list;
	}

	file = files;
	while (file != NULL)
	{
		if (file->filetype == LIBMTP_FILETYPE_FOLDER)
		{
			if (folders == NULL)
				folders = lfolder = file;
			else {
				lfolder->next = file;
				lfolder = lfolder->next;
			}
		}
		else if (LIBMTP_FILETYPE_IS_AUDIO(file->filetype))
		{
			if (audios == NULL)
				audios = laudio = file;
			else {
				laudio->next = file;
				laudio = laudio->next;
			}
		}
		else {
			if (g_ascii_strcasecmp(file->filename, ".nomedia") == 0) {
				nomedia = TRUE;
				break;
			}
		}
		file = file->next;
	}

	if (nomedia == FALSE)
	{
		/* Add folders recursively */
		file = folders;
		while (file != NULL) {
			list = get_track_list_recursive (thread, task, storageid, file->item_id, list);
			file = file->next;
		}

		/* Add music files */
		file = audios;
		while (file != NULL)
		{
			LIBMTP_track_t *track;
			track = LIBMTP_Get_Trackmetadata(thread->device, file->item_id);
			if (G_LIKELY(track)) {
				mobj = rena_musicobject_new_from_mtp_track (track);
				if (G_LIKELY(mobj)) {
					rena_musicobject_set_provider (mobj, thread->device_id);
					list = g_list_prepend(list, mobj);
				}
				LIBMTP_destroy_track_t(track);
			}
			file = file->next;
		}
	}

	/* Clean memory. */
	file = files;
	while (file != NULL) {
		tmp = file;
		file = file->next;
		LIBMTP_destroy_file_t(tmp);
	}

	data = rena_mtp_thread_progress_data_new (task->user_data,
	                                            g_list_length (list),
	                                            0);
	g_idle_add ((GSourceFunc) task->progress_callback, data);

	return list;
}

static void
get_track_list (RenaMtpThread *thread, RenaMtpThreadTask *task)
{
	RenaMtpThreadTracklistData *data;
	GList *list = NULL;
	LIBMTP_devicestorage_t *storage;

	CDEBUG(DBG_PLUGIN, "Mtp thread %s", G_STRFUNC);

	LIBMTP_Get_Storage (thread->device, LIBMTP_STORAGE_SORTBY_FREESPACE);

	for (storage = thread->device->storage; storage != NULL; storage = storage->next) {
		list = get_track_list_recursive (thread,
		                                 task,
		                                 storage->id,
		                                 LIBMTP_FILES_AND_FOLDERS_ROOT,
		                                 list);
	}

	if (!list) {
		CDEBUG(DBG_PLUGIN, "Mtp plugin no tracks on the device");
		rena_mtp_thread_report_errors (thread);
	}

	data = rena_mtp_thread_tracklist_data_new (task->user_data, list);
	g_idle_add ((GSourceFunc) task->callback, data);
}

static void
download_track (RenaMtpThread     *thread,
                RenaMtpThreadTask *task)
{
	RenaMtpThreadDownloadData *data;
	LIBMTP_file_t *fileinfo;
	LIBMTP_error_t *stack;

	CDEBUG(DBG_PLUGIN, "Mtp thread %s", G_STRFUNC);

	fileinfo = LIBMTP_Get_Filemetadata (thread->device, task->track_id);
	if (fileinfo == NULL) {
		stack = LIBMTP_Get_Errorstack (thread->device);
		CDEBUG(DBG_PLUGIN, "Mtp thread unable to get track metadata for %u: %s", task->track_id, stack->error_text);

		data = rena_mtp_thread_download_data_new (task->user_data, NULL, stack->error_text);
		g_idle_add ((GSourceFunc) task->callback, data);

		LIBMTP_Clear_Errorstack (thread->device);
		return;
	}

	if (LIBMTP_Get_Track_To_File (thread->device, task->track_id, task->filename, NULL, NULL)) {
		stack = LIBMTP_Get_Errorstack (thread->device);
		CDEBUG(DBG_PLUGIN, "Mtp thread unable to copy file from MTP device: %s", stack->error_text);

		data = rena_mtp_thread_download_data_new (task->user_data, NULL, stack->error_text);
		g_idle_add ((GSourceFunc) task->callback, data);

		LIBMTP_Clear_Errorstack (thread->device);
		return;
	}

	data = rena_mtp_thread_download_data_new (task->user_data, task->filename, NULL);
	g_idle_add ((GSourceFunc) task->callback, data);
}

static void
upload_track (RenaMtpThread     *thread,
              RenaMtpThreadTask *task)
{
	RenaMtpThreadUploadData *data;
	RenaMusicobject *mobj;
	LIBMTP_folder_t *folders, *music;
	LIBMTP_track_t *track;
	LIBMTP_error_t *stack;
	const gchar *filename = NULL, *artist = NULL, *album = NULL;
	guint32 storage_id, music_id, artist_id, album_id;

	CDEBUG(DBG_PLUGIN, "Mtp thread %s", G_STRFUNC);

	/*
	 * Create an Music/Artist/Album tree to put the file, with fallback to root
	 */

	folders = LIBMTP_Get_Folder_List (thread->device);
	music = LIBMTP_Find_Folder (folders, thread->device->default_music_folder);
	if (music) {
		storage_id = music->storage_id;
		music_id = music->folder_id;
	}
	else {
		CDEBUG(DBG_PLUGIN, "unable to find default music folder");
		storage_id = thread->device->storage->id;
		music_id = LIBMTP_Create_Folder (thread->device,
		                                 g_strdup(_("Music")),
		                                 LIBMTP_FILES_AND_FOLDERS_ROOT, storage_id);
		if (music_id == 0) {
			CDEBUG(DBG_PLUGIN, "couldn't create the Music folder");
			rena_mtp_thread_report_errors (thread);
		}
	}

	artist = rena_musicobject_get_artist (task->mobj);
	artist_id = LIBMTP_Create_Folder (thread->device,
	                                  g_strdup(string_is_not_empty(artist) ? artist : _("Unknown Artist")),
	                                  music_id, storage_id);
	if (artist_id == 0) {
		CDEBUG(DBG_PLUGIN, "couldn't create the artist folder");
		rena_mtp_thread_report_errors (thread);
	}

	album = rena_musicobject_get_album (task->mobj);
	album_id = LIBMTP_Create_Folder (thread->device,
	                                 g_strdup(string_is_not_empty(album) ? album : _("Unknown Album")),
	                                 artist_id, storage_id);
	if (album_id == 0) {
		CDEBUG(DBG_PLUGIN, "couldn't create the album folder");
		rena_mtp_thread_report_errors (thread);
	}

	/*
	 *  Create Track and upload it.
	 */
	filename = rena_musicobject_get_file (task->mobj);
	track = mtp_track_new_from_rena_musicobject (thread->device, task->mobj);

	track->parent_id = album_id;
	track->storage_id = storage_id;

	if (LIBMTP_Send_Track_From_File (thread->device, filename, track, NULL, NULL)) {
		stack = LIBMTP_Get_Errorstack (thread->device);

		CDEBUG(DBG_PLUGIN, "Mtp thread unable to upload file to MTP device: %s", stack->error_text);

		data = rena_mtp_thread_upload_data_new (task->user_data, NULL, stack->error_text);
		g_idle_add ((GSourceFunc) task->callback, data);

		LIBMTP_Clear_Errorstack (thread->device);

		return;
	}

	mobj = rena_musicobject_new_from_mtp_track (track);
	rena_musicobject_set_provider (mobj, thread->device_id);
	data = rena_mtp_thread_upload_data_new (task->user_data, mobj, NULL);
	g_idle_add ((GSourceFunc) task->callback, data);

	LIBMTP_destroy_track_t (track);
	LIBMTP_destroy_folder_t (folders);
}

static void
close_device (RenaMtpThread *thread, RenaMtpThreadTask *task)
{
	CDEBUG(DBG_PLUGIN, "Mtp thread %s", G_STRFUNC);

	if (thread->device != NULL) {
		LIBMTP_Release_Device (thread->device);
		thread->device = NULL;
	}

	if (thread->device_id != NULL) {
		g_free (thread->device_id);
		thread->device_id = NULL;
	}

	g_idle_add ((GSourceFunc) task->callback, task->user_data);
}

static gboolean
run_task (RenaMtpThread *thread, RenaMtpThreadTask *task)
{
	char *name = task_name (task);
	CDEBUG(DBG_PLUGIN, "Mtp thread running task: %s", name);
	g_free (name);

	switch (task->task) {
	case OPEN_DEVICE:
		open_device (thread, task);
		break;
	case GET_STATS:
		get_stats (thread, task);
		break;
	case GET_TRACK_LIST:
		get_track_list (thread, task);
		break;
	case DOWNLOAD_TRACK:
		download_track (thread, task);
		break;
	case UPLOAD_TRACK:
		upload_track (thread, task);
		break;
	case CLOSE_DEVICE:
		close_device (thread, task);
		break;
	case SHUTDOWN:
		return TRUE;
	default:
		g_assert_not_reached ();
	}

	return FALSE;
}

static gpointer
task_thread (RenaMtpThread *thread)
{
	RenaMtpThreadTask *task;
	gboolean quit = FALSE;

	GAsyncQueue *queue = g_async_queue_ref (thread->queue);

	CDEBUG(DBG_PLUGIN, "Mtp thread worker starting");

	while (quit == FALSE) {
		task = g_async_queue_pop (queue);
		quit = run_task (thread, task);
		destroy_task (task);
	}

	CDEBUG(DBG_PLUGIN, "Mtp thread worker exiting");

	/* clean up any queued tasks */
	while ((task = g_async_queue_try_pop (queue)) != NULL)
		destroy_task (task);

	g_async_queue_unref (queue);

	return NULL;
}


/*
 * Public functions
 */

void
rena_mtp_thread_open_device (RenaMtpThread     *thread,
                               guint                devnum,
                               guint                busnum,
                               GSourceFunc          finish_func,
                               gpointer             data)
{
	RenaMtpThreadTask *task = create_task (OPEN_DEVICE);

	CDEBUG(DBG_PLUGIN, "Mtp thread %s", G_STRFUNC);

	task->devnum = devnum;
	task->busnum = busnum;
	task->callback = finish_func;
	task->user_data = data;

	queue_task (thread, task);
}

void
rena_mtp_thread_get_stats (RenaMtpThread *thread,
                             GSourceFunc      finish_func,
                             gpointer         data)
{
	RenaMtpThreadTask *task = create_task (GET_STATS);

	CDEBUG(DBG_PLUGIN, "Mtp thread %s", G_STRFUNC);

	task->callback = finish_func;
	task->user_data = data;

	queue_task (thread, task);
}

void
rena_mtp_thread_get_track_list (RenaMtpThread *thread,
                                  GSourceFunc      finish_func,
                                  GSourceFunc      progress_func,
                                  gpointer         data)
{
	RenaMtpThreadTask *task = create_task (GET_TRACK_LIST);

	CDEBUG(DBG_PLUGIN, "Mtp thread %s", G_STRFUNC);

	task->callback = finish_func;
	task->progress_callback = progress_func;
	task->user_data = data;

	queue_task (thread, task);
}

void
rena_mtp_thread_download_track (RenaMtpThread *thread,
                                  guint            track_id,
                                  gchar           *filename,
                                  GSourceFunc      finish_func,
                                  GSourceFunc      progress_func,
                                  gpointer         data)
{
	RenaMtpThreadTask *task = create_task (DOWNLOAD_TRACK);

	CDEBUG(DBG_PLUGIN, "Mtp thread %s", G_STRFUNC);

	task->track_id = track_id;
	task->filename = g_strdup(filename);
	task->callback = finish_func;
	task->progress_callback = progress_func;
	task->user_data = data;

	queue_task (thread, task);
}

void
rena_mtp_thread_upload_track (RenaMtpThread   *thread,
                                RenaMusicobject *mobj,
                                GSourceFunc        finish_func,
                                gpointer           data)
{
	RenaMtpThreadTask  *task = create_task (UPLOAD_TRACK);

	CDEBUG(DBG_PLUGIN, "Mtp thread %s", G_STRFUNC);

	task->mobj = g_object_ref(mobj);
	task->callback = finish_func;
	task->user_data = data;

	queue_task (thread, task);
}

void
rena_mtp_thread_close_device (RenaMtpThread     *thread,
                                GSourceFunc          finish_func,
                                gpointer             data)
{
	RenaMtpThreadTask *task = create_task (CLOSE_DEVICE);

	CDEBUG(DBG_PLUGIN, "Mtp thread %s", G_STRFUNC);

	task->callback = finish_func;
	task->user_data = data;

	queue_task (thread, task);
}

void
rena_mtp_thread_report_errors (RenaMtpThread *thread)
{
	LIBMTP_error_t *stack;

	CDEBUG(DBG_PLUGIN, "Mtp thread %s", G_STRFUNC);

	for (stack = LIBMTP_Get_Errorstack (thread->device); stack != NULL; stack = stack->next) {
		g_warning ("libmtp error: %s", stack->error_text);
	}

	LIBMTP_Clear_Errorstack (thread->device);
}


/*
 * GObject things
 */

static void
impl_finalize (GObject *object)
{
	RenaMtpThread *thread = RENA_MTP_THREAD (object);
	RenaMtpThreadTask *task;

	CDEBUG(DBG_PLUGIN, "Mtp thread killing MTP worker thread");

	task = create_task (SHUTDOWN);
	queue_task (thread, task);

	if (thread->thread != g_thread_self ()) {
		g_thread_join (thread->thread);
		CDEBUG(DBG_PLUGIN, "Mtp thread MTP worker thread exited");
	} else {
		CDEBUG(DBG_PLUGIN, "Mtp thread we're on the MTP worker thread..");
	}

	g_async_queue_unref (thread->queue);

	if (thread->device != NULL) {
		LIBMTP_Release_Device (thread->device);
	}

	if (thread->device_id != NULL) {
		g_free(thread->device_id);
	}

	G_OBJECT_CLASS (rena_mtp_thread_parent_class)->finalize (object);
}

static void
rena_mtp_thread_init (RenaMtpThread *thread)
{
	CDEBUG(DBG_PLUGIN, "Mtp thread %s", G_STRFUNC);

	LIBMTP_Init ();

	thread->queue = g_async_queue_new ();

	thread->thread = g_thread_new ("rena-mtp", (GThreadFunc) task_thread, thread);
}

static void
rena_mtp_thread_class_init (RenaMtpThreadClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = impl_finalize;
}

RenaMtpThread *
rena_mtp_thread_new (void)
{
	CDEBUG(DBG_PLUGIN, "Mtp thread %s", G_STRFUNC);

	return g_object_new (RENA_TYPE_MTP_THREAD, NULL);
}

