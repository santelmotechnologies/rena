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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(GETTEXT_PACKAGE)
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#include <glib.h>
#include <glib-object.h>
#include <gmodule.h>
#include <gtk/gtk.h>

#include <cdio/paranoia/cdda.h>
#include <cdio/cd_types.h>
#include <cddb/cddb.h>

#include <libpeas/peas.h>

#include "src/rena.h"
#include "src/rena-hig.h"
#include "src/rena-utils.h"
#include "src/rena-menubar.h"
#include "src/rena-musicobject.h"
#include "src/rena-musicobject-mgmt.h"
#include "src/rena-plugins-engine.h"
#include "src/rena-statusicon.h"
#include "src/rena-music-enum.h"
#include "src/rena-database-provider.h"
#include "src/rena-window.h"

#if HAVE_GUDEV
#include "src/rena-device-client.h"
#endif

#include "plugins/rena-plugin-macros.h"

#define RENA_TYPE_CDROM_PLUGIN         (rena_cdrom_plugin_get_type ())
#define RENA_CDROM_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), RENA_TYPE_CDROM_PLUGIN, RenaCdromPlugin))
#define RENA_CDROM_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), RENA_TYPE_CDROM_PLUGIN, RenaCdromPlugin))
#define RENA_IS_CDROM_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), RENA_TYPE_CDROM_PLUGIN))
#define RENA_IS_CDROM_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), RENA_TYPE_CDROM_PLUGIN))
#define RENA_CDROM_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), RENA_TYPE_CDROM_PLUGIN, RenaCdromPluginClass))

struct _RenaCdromPluginPrivate {
	RenaApplication *rena;

#if HAVE_GUDEV
	RenaDeviceClient *device_client;
#endif

	guint64             bus_hooked;
	guint64             device_hooked;
	gchar              *disc_id;

	GtkWidget          *device_setting_widget;
	GtkWidget          *audio_cd_device_w;
	GtkWidget          *cddb_setting_widget;
	GtkWidget          *use_cddb_w;

	gchar              *audio_cd_device;
	gboolean            use_cddb;

	GtkActionGroup    *action_group_main_menu;
	guint              merge_id_main_menu;
};
typedef struct _RenaCdromPluginPrivate RenaCdromPluginPrivate;

RENA_PLUGIN_REGISTER (RENA_TYPE_CDROM_PLUGIN,
                        RenaCdromPlugin,
                        rena_cdrom_plugin)

/*
 * CDROM plugin.
 */
#define KEY_USE_CDDB        "use_cddb"
#define KEY_AUDIO_CD_DEVICE "audio_cd_device"

static gboolean
rena_preferences_get_use_cddb (RenaPreferences *preferences)
{
	gchar *plugin_group = NULL;
	gboolean use_cddb = FALSE;

	plugin_group = rena_preferences_get_plugin_group_name (preferences, "cdrom");
	use_cddb = rena_preferences_get_boolean (preferences,
	                                           plugin_group,
	                                           KEY_USE_CDDB);
	g_free (plugin_group);

	return use_cddb;
}

static void
rena_preferences_set_use_cddb (RenaPreferences *preferences,
                                 gboolean           use_cddb)
{
	gchar *plugin_group = NULL;
	plugin_group = rena_preferences_get_plugin_group_name (preferences, "cdrom");
	rena_preferences_set_boolean (preferences,
	                                plugin_group,
	                                KEY_USE_CDDB,
	                                use_cddb);
	g_free (plugin_group);
}

static gchar *
rena_preferences_get_audio_cd_device (RenaPreferences *preferences)
{
	gchar *plugin_group = NULL, *audio_cd_device = NULL;

	plugin_group = rena_preferences_get_plugin_group_name (preferences, "cdrom");
	audio_cd_device = rena_preferences_get_string (preferences,
	                                                 plugin_group,
	                                                 KEY_AUDIO_CD_DEVICE);
	g_free (plugin_group);

	return audio_cd_device;
}

static void
rena_preferences_set_audio_cd_device (RenaPreferences *preferences,
                                        const gchar       *device)
{
	gchar *plugin_group = NULL;
	plugin_group = rena_preferences_get_plugin_group_name (preferences, "cdrom");

	if (string_is_not_empty(device))
		rena_preferences_set_string (preferences,
		                               plugin_group,
		                               KEY_AUDIO_CD_DEVICE,
		                               device);
	else
		rena_preferences_remove_key (preferences,
		                               plugin_group,
		                               KEY_AUDIO_CD_DEVICE);
	g_free (plugin_group);
}

static RenaMusicobject *
new_musicobject_from_cdda (RenaCdromPlugin *plugin,
                           cdrom_drive_t     *cdda_drive,
                           cddb_disc_t       *cddb_disc,
                           gint               track_no)
{
	RenaPreferences *preferences;
	RenaMusicEnum *enum_map = NULL;
	RenaMusicobject *mobj = NULL;
	gint channels, start, end;
	gchar *ntitle = NULL, *nfile = NULL;

	RenaCdromPluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "Creating new musicobject from cdda: %d", track_no);

	channels = cdio_get_track_channels(cdda_drive->p_cdio, track_no);
	start = cdio_cddap_track_firstsector(cdda_drive, track_no);
	end = cdio_cddap_track_lastsector(cdda_drive, track_no);

	mobj = g_object_new (RENA_TYPE_MUSICOBJECT,
	                     NULL);

	preferences = rena_application_get_preferences (priv->rena);
	if (rena_preferences_get_use_cddb (preferences) && cddb_disc) {
		cddb_track_t *track;
		const gchar *title, *artist, *album, *genre;
		gint year;

		track = cddb_disc_get_track(cddb_disc, track_no - 1);
		if (track) {
			title = cddb_track_get_title(track);
			if (title)
				ntitle = g_strdup(title);

			artist = cddb_track_get_artist(track);
			if(artist)
				rena_musicobject_set_artist(mobj, artist);

			album = cddb_disc_get_title(cddb_disc);
			if(album)
				rena_musicobject_set_album(mobj, album);

			year = cddb_disc_get_year(cddb_disc);
			if(year)
				rena_musicobject_set_year(mobj, year);

			genre = cddb_disc_get_genre(cddb_disc);
			if(genre)
				rena_musicobject_set_genre(mobj, genre);
		}
	}

	enum_map = rena_music_enum_get ();
	rena_musicobject_set_source (mobj, rena_music_enum_map_get(enum_map, "CDROM"));
	g_object_unref (enum_map);

	if (priv->disc_id)
		rena_musicobject_set_provider (mobj, priv->disc_id);

	nfile = g_strdup_printf("cdda://%d", track_no);
	rena_musicobject_set_file(mobj, nfile);
	rena_musicobject_set_track_no(mobj, track_no);

	if (!ntitle)
		ntitle = g_strdup_printf("Track %d", track_no);
	rena_musicobject_set_title(mobj, ntitle);

	rena_musicobject_set_length(mobj, (end - start) / CDIO_CD_FRAMES_PER_SEC);
	rena_musicobject_set_channels(mobj, (channels > 0) ? channels : 0);

	g_free(nfile);
	g_free(ntitle);

	return mobj;
}

static gint
rena_cdrom_plugin_add_cddb_tracks (cdrom_drive_t *cdda_drive,
                                     cddb_disc_t   *cddb_disc)
{
	cddb_track_t *track;
	lba_t lba;
	gint num_tracks, first_track, i = 0;

	num_tracks = cdio_cddap_tracks(cdda_drive);
	if (!num_tracks)
		return -1;

	first_track = cdio_get_first_track_num(cdda_drive->p_cdio);
	for (i = first_track; i <= num_tracks; i++) {
		track = cddb_track_new();
		if (!track)
			return -1;

		lba = cdio_get_track_lba(cdda_drive->p_cdio, i);
		if (lba == CDIO_INVALID_LBA)
			return -1;

		cddb_disc_add_track(cddb_disc, track);
		cddb_track_set_frame_offset(track, lba);
	}

	return 0;
}

static GList *
rena_cdrom_plugin_get_mobj_list (RenaCdromPlugin *plugin,
                                   cdrom_drive_t     *cdda_drive,
                                   cddb_disc_t       *cddb_disc)
{
	RenaMusicobject *mobj;
	gint num_tracks = 0, i = 0;
	GList *list = NULL;

	num_tracks = cdio_cddap_tracks(cdda_drive);
	if (!num_tracks)
		return NULL;

	for (i = 1; i <= num_tracks; i++) {
		mobj = new_musicobject_from_cdda (plugin, cdda_drive, cddb_disc, i);
		if (G_LIKELY(mobj))
			list = g_list_append(list, mobj);

		rena_process_gtk_events ();
	}
	return list;
}

static cdrom_drive_t *
rena_cdrom_plugin_get_drive (RenaPreferences *preferences)
{
	cdrom_drive_t *drive = NULL;
	gchar **cdda_devices = NULL;
	const gchar *audio_cd_device = NULL;

	audio_cd_device = rena_preferences_get_audio_cd_device(preferences);

	if (!audio_cd_device) {
		cdda_devices = cdio_get_devices_with_cap(NULL, CDIO_FS_AUDIO, FALSE);
		if (!cdda_devices || (cdda_devices && !*cdda_devices)) {
			g_warning("No Audio CD found");
			return NULL;
		}

		CDEBUG(DBG_PLUGIN, "Trying Audio CD Device: %s", *cdda_devices);

		drive = cdio_cddap_identify(*cdda_devices, 0, NULL);
		if (!drive) {
			g_warning("Unable to identify Audio CD");
			goto exit;
		}
	}
	else {
		CDEBUG(DBG_PLUGIN, "Trying Audio CD Device: %s", audio_cd_device);

		drive = cdio_cddap_identify(audio_cd_device, 0, NULL);
		if (!drive) {
			g_warning("Unable to identify Audio CD");
			return NULL;
		}
	}
exit:
	if (cdda_devices)
		cdio_free_device_list(cdda_devices);

	return drive;
}

static void
rena_application_append_audio_cd (RenaCdromPlugin *plugin)
{
	RenaDatabaseProvider *provider;
	RenaDatabase *database;
	RenaPlaylist *playlist;
	RenaPreferences *preferences;
	RenaMusicobject *mobj;
	lba_t lba;
	gint matches;
	cdrom_drive_t *cdda_drive = NULL;
	cddb_disc_t *cddb_disc = NULL;
	cddb_conn_t *cddb_conn = NULL;
	const gchar *title_disc = NULL;
	guint discid = 0;
	GList *list = NULL, *l;

	RenaCdromPluginPrivate *priv = plugin->priv;

	preferences = rena_application_get_preferences (priv->rena);

	cdda_drive = rena_cdrom_plugin_get_drive (preferences);
	if (!cdda_drive)
		return;

	if (cdio_cddap_open(cdda_drive)) {
		g_warning("Unable to open Audio CD");
		return;
	}

	if (rena_preferences_get_use_cddb (preferences)) {
		cddb_conn = cddb_new ();
		if (!cddb_conn)
			goto add;

		cddb_disc = cddb_disc_new();
		if (!cddb_disc)
			goto add;

		lba = cdio_get_track_lba(cdda_drive->p_cdio, CDIO_CDROM_LEADOUT_TRACK);
		if (lba == CDIO_INVALID_LBA)
			goto add;

		cddb_disc_set_length(cddb_disc, FRAMES_TO_SECONDS(lba));
		if (rena_cdrom_plugin_add_cddb_tracks(cdda_drive, cddb_disc) < 0)
			goto add;

		if (!cddb_disc_calc_discid(cddb_disc))
			goto add;

		discid = cddb_disc_get_discid (cddb_disc);
		if (discid)
			priv->disc_id = g_strdup_printf ("Discid://%x", discid);

		cddb_disc_set_category(cddb_disc, CDDB_CAT_MISC);

		matches = cddb_query(cddb_conn, cddb_disc);
		if (matches == -1)
			goto add;

		if (!cddb_read(cddb_conn, cddb_disc)) {
			cddb_error_print(cddb_errno(cddb_conn));
			goto add;
		}

		CDEBUG(DBG_PLUGIN, "Successfully initialized CDDB");

		goto add;
	}

add:
	list = rena_cdrom_plugin_get_mobj_list (plugin, cdda_drive, cddb_disc);
	if (list) {
		playlist = rena_application_get_playlist (priv->rena);
		rena_playlist_append_mobj_list (playlist, list);

		if (priv->disc_id) {
			title_disc = cddb_disc_get_title (cddb_disc);

			provider = rena_database_provider_get ();
			rena_provider_add_new (provider,
			                         priv->disc_id,
			                         "CDROM",
			                         title_disc ? title_disc : _("Audio CD"),
			                         "media-optical");
			rena_provider_set_visible (provider, priv->disc_id, TRUE);

			database = rena_application_get_database (priv->rena);
			for (l = list; l != NULL; l = l->next) {
				mobj = l->data;
				rena_database_add_new_musicobject (database, mobj);
			}
			rena_provider_update_done (provider);
			g_object_unref (provider);
		}
		g_list_free (list);
	}

	CDEBUG(DBG_PLUGIN, "Successfully opened Audio CD device");

	if (cdda_drive)
		cdio_cddap_close(cdda_drive);
	if (cddb_disc)
		cddb_disc_destroy(cddb_disc);
	if (cddb_conn)
		cddb_destroy(cddb_conn);
}

static gboolean
rena_musicobject_is_cdda_type (RenaMusicobject *mobj)
{
	RenaMusicEnum *enum_map = NULL;
	RenaMusicSource file_source = FILE_NONE;

	enum_map = rena_music_enum_get ();
	file_source = rena_music_enum_map_get(enum_map, "CDROM");
	g_object_unref (enum_map);

	return (file_source == rena_musicobject_get_source (mobj));
}

static void
rena_cdrom_plugin_set_device (RenaBackend *backend, GObject *obj, gpointer user_data)
{
	RenaPreferences *preferences;
	RenaMusicobject *mobj = NULL;
	const gchar *audio_cd_device;
	GObject *source;

	RenaCdromPlugin *plugin = user_data;
	RenaCdromPluginPrivate *priv = plugin->priv;

	mobj = rena_backend_get_musicobject (backend);
	if (!rena_musicobject_is_cdda_type (mobj))
		return;

	g_object_get (obj, "source", &source, NULL);
	if (source) {
		preferences = rena_application_get_preferences (priv->rena);
		audio_cd_device = rena_preferences_get_audio_cd_device (preferences);
		if (audio_cd_device) {
			g_object_set (source, "device", audio_cd_device, NULL);
		}
		g_object_unref (source);
	}
}

static void
rena_cdrom_plugin_prepare_source (RenaBackend *backend, gpointer user_data)
{
	RenaMusicobject *mobj;
	const gchar *uri = NULL;

	mobj = rena_backend_get_musicobject (backend);
	if (!rena_musicobject_is_cdda_type (mobj))
		return;

	uri = rena_musicobject_get_file (mobj);
	rena_backend_set_playback_uri (backend, uri);
}

/*
 * GUDEV signals.
 */

#ifdef HAVE_GUDEV
static void
rena_cdrom_plugin_device_added_response (GtkWidget *dialog,
                                           gint       response,
                                           gpointer   user_data)
{
	RenaCdromPlugin *plugin = user_data;

	switch (response) {
		case RENA_DEVICE_RESPONSE_PLAY:
			rena_application_append_audio_cd (plugin);
			break;
		case RENA_DEVICE_RESPONSE_NONE:
		default:
			break;
	}

	gtk_widget_destroy (dialog);
}

static void
rena_cdrom_plugin_device_added (RenaDeviceClient *device_client,
                                  RenaDeviceType    device_type,
                                  GUdevDevice        *u_device,
                                  gpointer            user_data)
{
	GtkWidget *dialog;

	RenaCdromPlugin *plugin = user_data;
	RenaCdromPluginPrivate *priv = plugin->priv;

	if (device_type != RENA_DEVICE_AUDIO_CD)
		return;

	if (priv->bus_hooked || priv->device_hooked)
		return;

	priv->bus_hooked = g_udev_device_get_property_as_uint64 (u_device, "BUSNUM");
	priv->device_hooked = g_udev_device_get_property_as_uint64 (u_device, "DEVNUM");

	dialog = rena_gudev_dialog_new (rena_application_get_window (priv->rena),
	                                 _("Audio/Data CD"), "media-optical",
	                                 _("An audio CD was inserted"), NULL,
	                                 _("Add Audio _CD"), RENA_DEVICE_RESPONSE_PLAY);

	g_signal_connect (G_OBJECT (dialog), "response",
	                  G_CALLBACK (rena_cdrom_plugin_device_added_response), plugin);

	gtk_widget_show_all (dialog);
}

void
rena_cdrom_plugin_device_removed (RenaDeviceClient *device_client,
                                    RenaDeviceType    device_type,
                                    GUdevDevice        *u_device,
                                    gpointer            user_data)
{
	RenaDatabaseProvider *provider;
	RenaMusicEnum *enum_map = NULL;
	guint64 busnum = 0;
	guint64 devnum = 0;

	RenaCdromPlugin *plugin = user_data;
	RenaCdromPluginPrivate *priv = plugin->priv;

	if (device_type != RENA_DEVICE_AUDIO_CD)
		return;

	busnum = g_udev_device_get_property_as_uint64(u_device, "BUSNUM");
	devnum = g_udev_device_get_property_as_uint64(u_device, "DEVNUM");

	if (busnum == priv->bus_hooked && devnum == priv->device_hooked) {
		if (priv->disc_id) {
			provider = rena_database_provider_get ();
			rena_provider_remove (provider, priv->disc_id);
			rena_provider_update_done (provider);
			g_object_unref (provider);
		}

		priv->bus_hooked = 0;
		priv->device_hooked = 0;

		if (priv->disc_id) {
			g_free (priv->disc_id);
			priv->disc_id = NULL;
		}

		enum_map = rena_music_enum_get ();
		rena_music_enum_map_remove (enum_map, "CDROM");
		g_object_unref (enum_map);
	}
}
#endif

/*
 * Menubar
 */
static void
rena_cdrom_plugin_append_action (GtkAction *action, RenaCdromPlugin *plugin)
{
	rena_application_append_audio_cd (plugin);
}

static void
rena_gmenu_add_cdrom_action (GSimpleAction *action,
                               GVariant      *parameter,
                               gpointer       user_data)
{
	rena_cdrom_plugin_append_action (NULL, RENA_CDROM_PLUGIN(user_data));
}

static const GtkActionEntry main_menu_actions [] = {
	{"Add Audio CD", NULL, N_("Add Audio _CD"),
	 "", "Append a Audio CD", G_CALLBACK(rena_cdrom_plugin_append_action)}
};

static const gchar *main_menu_xml = "<ui>							\
	<menubar name=\"Menubar\">										\
		<menu action=\"PlaylistMenu\">								\
			<placeholder name=\"rena-append-music-placeholder\">	\
				<menuitem action=\"Add Audio CD\"/>					\
			</placeholder>											\
		</menu>														\
	</menubar>														\
</ui>";

/*
 * Cdrom Settings
 */
static void
rena_cdrom_preferences_dialog_response (GtkDialog         *dialog_w,
                                          gint               response_id,
                                          RenaCdromPlugin *plugin)
{
	RenaPreferences *preferences;
	const gchar *audio_cd_device;

	RenaCdromPluginPrivate *priv = plugin->priv;

	preferences = rena_preferences_get();
	switch(response_id) {
	case GTK_RESPONSE_CANCEL:
		rena_gtk_entry_set_text(GTK_ENTRY(priv->audio_cd_device_w),
			priv->audio_cd_device);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->use_cddb_w),
			priv->use_cddb);
		break;
	case GTK_RESPONSE_OK:
		audio_cd_device = gtk_entry_get_text (GTK_ENTRY(priv->audio_cd_device_w));
		if (audio_cd_device) {
			rena_preferences_set_audio_cd_device (preferences, audio_cd_device);

			g_free (priv->audio_cd_device);
			priv->audio_cd_device = g_strdup(audio_cd_device);
		}
		priv->use_cddb =
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->use_cddb_w));
		rena_preferences_set_use_cddb (preferences, priv->use_cddb);
		break;
	default:
		break;
	}
	g_object_unref (preferences);
}

static void
rena_cdrom_init_settings (RenaCdromPlugin *plugin)
{
	RenaPreferences *preferences;
	gchar *plugin_group = NULL;

	RenaCdromPluginPrivate *priv = plugin->priv;

	preferences = rena_preferences_get();
	plugin_group = rena_preferences_get_plugin_group_name (preferences, "cdrom");
	if (rena_preferences_has_group (preferences, plugin_group)) {
		priv->audio_cd_device =
			rena_preferences_get_audio_cd_device (preferences);
		priv->use_cddb =
			rena_preferences_get_use_cddb(preferences);
	}
	else {
		priv->audio_cd_device = NULL;
		priv->use_cddb = TRUE;
	}

	rena_gtk_entry_set_text(GTK_ENTRY(priv->audio_cd_device_w), priv->audio_cd_device);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->use_cddb_w), priv->use_cddb);

	g_object_unref (preferences);
	g_free (plugin_group);
}

static void
rena_cdrom_plugin_append_setting (RenaCdromPlugin *plugin)
{
	RenaPreferencesDialog *dialog;
	GtkWidget *table;
	GtkWidget *audio_cd_device_label,*audio_cd_device_entry, *use_cddb;
	guint row = 0;

	RenaCdromPluginPrivate *priv = plugin->priv;

	/* Cd Device */

	table = rena_hig_workarea_table_new();

	rena_hig_workarea_table_add_section_title(table, &row, _("Audio CD"));

	audio_cd_device_label = gtk_label_new(_("Audio CD Device"));
	gtk_widget_set_halign (GTK_WIDGET(audio_cd_device_label), GTK_ALIGN_START);
	gtk_widget_set_valign (GTK_WIDGET(audio_cd_device_label), GTK_ALIGN_START);

	audio_cd_device_entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY(audio_cd_device_entry), AUDIO_CD_DEVICE_ENTRY_LEN);
	gtk_entry_set_activates_default (GTK_ENTRY(audio_cd_device_entry), TRUE);

	rena_hig_workarea_table_add_row (table, &row, audio_cd_device_label, audio_cd_device_entry);

	/* Store references */

	priv->device_setting_widget = table;
	priv->audio_cd_device_w = audio_cd_device_entry;

	/* CDDB Option */
	row = 0;
	table = rena_hig_workarea_table_new();

	rena_hig_workarea_table_add_section_title (table, &row, "CDDB");

	use_cddb = gtk_check_button_new_with_label (_("Connect to CDDB server"));
	rena_hig_workarea_table_add_wide_control (table, &row, use_cddb);

	priv->cddb_setting_widget = table;
	priv->use_cddb_w = use_cddb;

	/* Append panes */

	dialog = rena_application_get_preferences_dialog (priv->rena);
	rena_preferences_append_audio_setting (dialog,
	                                         priv->device_setting_widget, FALSE);
	rena_preferences_append_services_setting (dialog,
	                                            priv->cddb_setting_widget, FALSE);

	/* Configure handler and settings */
	rena_preferences_dialog_connect_handler (dialog,
	                                           G_CALLBACK(rena_cdrom_preferences_dialog_response),
	                                           plugin);

	rena_cdrom_init_settings (plugin);
}

static void
rena_cdrom_plugin_remove_setting (RenaCdromPlugin *plugin)
{
	RenaPreferencesDialog *dialog;
	RenaCdromPluginPrivate *priv = plugin->priv;

	dialog = rena_application_get_preferences_dialog (priv->rena);

	rena_preferences_dialog_disconnect_handler (dialog,
	                                              G_CALLBACK(rena_cdrom_preferences_dialog_response),
	                                              plugin);

	rena_preferences_remove_audio_setting (dialog,
	                                         priv->device_setting_widget);
	rena_preferences_remove_services_setting (dialog,
	                                            priv->cddb_setting_widget);
}

/*
 * Cdrom plugin
 */
static void
rena_plugin_activate (PeasActivatable *activatable)
{
	GMenuItem *item;
	GSimpleAction *action;
	RenaBackend *backend;
	RenaStatusIcon *status_icon = NULL;
	RenaMusicEnum *enum_map = NULL;

	RenaCdromPlugin *plugin = RENA_CDROM_PLUGIN (activatable);
	RenaCdromPluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN,"CDROM plugin %s", G_STRFUNC);

	priv->rena = g_object_get_data (G_OBJECT (plugin), "object");

	/* Attach main menu */

	priv->action_group_main_menu = rena_menubar_plugin_action_new ("RenaCdromPlugin",
	                                                                 main_menu_actions,
	                                                                 G_N_ELEMENTS (main_menu_actions),
	                                                                 NULL,
	                                                                 0,
	                                                                 plugin);

	priv->merge_id_main_menu = rena_menubar_append_plugin_action (priv->rena,
	                                                                priv->action_group_main_menu,
	                                                                main_menu_xml);

	/* Systray and gear Menu*/

	action = g_simple_action_new ("add-cdrom", NULL);
	g_signal_connect (G_OBJECT (action), "activate",
	                  G_CALLBACK (rena_gmenu_add_cdrom_action), plugin);

	item = g_menu_item_new (_("Add Audio _CD"), "syst.add-cdrom");
	status_icon = rena_application_get_status_icon(priv->rena);
	rena_systray_append_action (status_icon, "rena-systray-append-music", action, item);
	g_object_unref (item);

	item = g_menu_item_new (_("Add Audio _CD"), "win.add-cdrom");
	rena_menubar_append_action (priv->rena, "rena-plugins-append-music", action, item);
	g_object_unref (item);

	/* Connect signals */

	backend = rena_application_get_backend (priv->rena);
	g_signal_connect (backend, "set-device",
	                  G_CALLBACK(rena_cdrom_plugin_set_device), plugin);
	g_signal_connect (backend, "prepare-source",
	                  G_CALLBACK(rena_cdrom_plugin_prepare_source), plugin);

#ifdef HAVE_GUDEV
	priv->device_client = rena_device_client_get();

	g_signal_connect (G_OBJECT(priv->device_client), "device-added",
	                  G_CALLBACK(rena_cdrom_plugin_device_added), plugin);
	g_signal_connect (G_OBJECT(priv->device_client), "device-removed",
	                  G_CALLBACK(rena_cdrom_plugin_device_removed), plugin);
#endif

	enum_map = rena_music_enum_get ();
	rena_music_enum_map_get (enum_map, "CDROM");
	g_object_unref (enum_map);

	/* Settings */
	rena_cdrom_plugin_append_setting (plugin);
}

static void
rena_plugin_deactivate (PeasActivatable *activatable)
{
	RenaDatabaseProvider *provider;
	RenaBackend *backend;
	RenaPreferences *preferences;
	RenaStatusIcon *status_icon = NULL;
	RenaMusicEnum *enum_map = NULL;
	gchar *plugin_group = NULL;

	RenaCdromPlugin *plugin = RENA_CDROM_PLUGIN (activatable);
	RenaCdromPluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN,"CDROM plugin %s", G_STRFUNC);

	rena_menubar_remove_plugin_action (priv->rena,
	                                     priv->action_group_main_menu,
	                                     priv->merge_id_main_menu);
	priv->merge_id_main_menu = 0;

	status_icon = rena_application_get_status_icon(priv->rena);
	rena_systray_remove_action (status_icon, "rena-systray-append-music", "add-cdrom");

	rena_menubar_remove_action (priv->rena, "rena-plugins-append-music", "add-cdrom");

	backend = rena_application_get_backend (priv->rena);
	g_signal_handlers_disconnect_by_func (backend, rena_cdrom_plugin_set_device, plugin);
	g_signal_handlers_disconnect_by_func (backend, rena_cdrom_plugin_prepare_source, plugin);

#ifdef HAVE_GUDEV
	g_signal_handlers_disconnect_by_func (priv->device_client,
	                                      rena_cdrom_plugin_device_added,
	                                      plugin);
	g_signal_handlers_disconnect_by_func (priv->device_client,
	                                      rena_cdrom_plugin_device_removed,
	                                      plugin);
	g_object_unref (priv->device_client);
#endif

	/* Remove from database */

	if (priv->disc_id) {
		provider = rena_database_provider_get ();
		rena_provider_remove (provider, priv->disc_id);
		g_object_unref (provider);
	}

	/* Crop library to not save from playlist */

	enum_map = rena_music_enum_get ();
	rena_music_enum_map_remove (enum_map, "CDROM");
	g_object_unref (enum_map);

	/* If plugin is disables by user remove the rest of preferences */

	if (!rena_plugins_engine_is_shutdown(rena_application_get_plugins_engine(priv->rena)))
	{
		/* Remove setting widgets */

		rena_cdrom_plugin_remove_setting (plugin);

		/* Remove settings */

		preferences = rena_application_get_preferences (priv->rena);
		plugin_group = rena_preferences_get_plugin_group_name (preferences, "cdrom");
		rena_preferences_remove_group (preferences, plugin_group);
		g_free (plugin_group);

		/* Force update library view */

		if (priv->disc_id) {
			provider = rena_database_provider_get ();
			rena_provider_update_done (provider);
			g_object_unref (provider);
		}
	}

	/* Free and shutdown */

	if (priv->disc_id)
		g_free (priv->disc_id);

	libcddb_shutdown ();
}
