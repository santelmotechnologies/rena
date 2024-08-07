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

#include "rena-simple-widgets.h"
#include "rena-tags-dialog.h"

#if defined(GETTEXT_PACKAGE)
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#include "rena-hig.h"
#include "rena-utils.h"
#include "rena-musicobject-mgmt.h"

static void     rena_tags_dialog_dispose            (GObject *object);
static void     rena_tags_dialog_finalize           (GObject *object);

static void     rena_title_entry_change             (GtkEntry *entry, RenaTagsDialog *dialog);
static void     rena_title_check_toggled            (GtkToggleButton *toggle, RenaTagsDialog *dialog);

static void     rena_artist_entry_change            (GtkEntry *entry, RenaTagsDialog *dialog);
static void     rena_artist_check_toggled           (GtkToggleButton *toggle, RenaTagsDialog *dialog);

static void     rena_album_entry_change             (GtkEntry *entry, RenaTagsDialog *dialog);
static void     rena_album_check_toggled            (GtkToggleButton *toggle, RenaTagsDialog *dialog);

static void     rena_genre_check_toggled            (GtkToggleButton *toggle, RenaTagsDialog *dialog);
static void     rena_track_no_check_toggled         (GtkToggleButton *toggle, RenaTagsDialog *dialog);
static void     rena_year_check_toggled             (GtkToggleButton *toggle, RenaTagsDialog *dialog);
static void     rena_comment_check_toggled          (GtkToggleButton *toggle, RenaTagsDialog *dialog);

static void     rena_tag_entry_change               (GtkEntry *entry, GtkCheckButton *check);
static void     rena_tag_entry_clear_pressed        (GtkEntry *entry, gint position, GdkEventButton *event);
static void     rena_tag_entry_directory_pressed    (GtkEntry *entry, gint position, GdkEventButton *event, gpointer user_data);
static gboolean rena_tag_entry_select_text_on_click (GtkWidget *widget, GdkEvent  *event, gpointer user_data);
static void     rena_file_entry_populate_menu       (GtkEntry *entry, GtkMenu *menu, gpointer user_data);

GtkEntryCompletion *rena_tags_get_entry_completion_from_table (const gchar *table);

struct _RenaTagsDialogClass {
	GtkDialogClass __parent__;
};

struct _RenaTagsDialog {
	GtkDialog __parent__;

	RenaHeader      *header;

	GtkWidget         *title_entry;
	GtkWidget         *artist_entry;
	GtkWidget         *album_entry;
	GtkWidget         *genre_entry;
	GtkWidget         *track_no_entry;
	GtkWidget         *year_entry;
	GtkWidget         *comment_entry;
	GtkWidget         *file_entry;

	GtkWidget         *title_check_change;
	GtkWidget         *artist_check_change;
	GtkWidget         *album_check_change;
	GtkWidget         *genre_check_change;
	GtkWidget         *track_no_check_change;
	GtkWidget         *year_check_change;
	GtkWidget         *comment_check_change;

	RenaMusicobject *mobj;
};

G_DEFINE_TYPE (RenaTagsDialog, rena_tags_dialog, GTK_TYPE_DIALOG);

/*
 *  Utils.
 */

static gchar *
rena_tags_dialog_get_title (const gchar *title, const gchar *file)
{
	gchar *stitle = NULL;
	if (string_is_not_empty(title))
		stitle = g_strdup (title);
	else if (string_is_not_empty(file))
		stitle = get_display_filename (file, FALSE);
	else
		stitle = g_strdup (_("Unknown"));
	return stitle;
}

static gchar *
rena_tags_dialog_get_subtitle (const gchar *artist, const gchar *album)
{
	gchar *subtitle;
	subtitle = g_strdup_printf ("%s - %s",
	                            string_is_not_empty(artist) ? artist : _("Unknown Artist"),
	                            string_is_not_empty(album) ? album : _("Unknown Album"));
	return subtitle;
}

/*
 * RenaTagDialog
 */

static void
rena_tags_dialog_class_init (RenaTagsDialogClass *klass)
{
  GObjectClass   *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = rena_tags_dialog_dispose;
  gobject_class->finalize = rena_tags_dialog_finalize;
}

static void
rena_tags_dialog_init (RenaTagsDialog *dialog)
{
	RenaHeader *header;
	GtkWidget *tag_table;
	GtkWidget *label_title, *label_artist, *label_album, *label_genre, *label_tno, *label_year, *label_comment, *label_file;
	GtkWidget *chk_title, *chk_artist, *chk_album, *chk_genre, *chk_tno, *chk_year, *chk_comment;
	GtkWidget *entry_title, *entry_artist, *entry_album, *entry_genre,  *entry_tno, *entry_year, *entry_comment, *entry_file;
	GtkWidget *hbox_title, *hbox_artist, *hbox_album, *hbox_genre, *hbox_tno, *hbox_year, *hbox_comment;
	GtkWidget *hbox_spins, *comment_view_scroll;
	GtkEntryCompletion *completion;

	/* Set dialog properties */

	gtk_window_set_title (GTK_WINDOW (dialog), _("Edit tags"));
	gtk_window_set_default_size(GTK_WINDOW (dialog), 450, 300);
	gtk_dialog_add_button (GTK_DIALOG (dialog), _("_Cancel"), GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (dialog), _("_Ok"), GTK_RESPONSE_OK);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

	header = rena_header_new ();
	rena_header_set_icon_name (header, "media-optical");

	/* Create table */

	tag_table = gtk_grid_new ();

	gtk_grid_set_row_spacing (GTK_GRID(tag_table), 6);
	gtk_grid_set_column_spacing (GTK_GRID(tag_table), 6);

	gtk_container_set_border_width (GTK_CONTAINER(tag_table), 6);

	/* Create labels */

	label_title = gtk_label_new(_("Title"));
	label_artist = gtk_label_new(_("Artist"));
	label_album = gtk_label_new(_("Album"));
	label_genre = gtk_label_new(_("Genre"));
	label_tno = gtk_label_new(_("Track No"));
	label_year = gtk_label_new(_("Year"));
	label_comment = gtk_label_new(_("Comment"));
	label_file = gtk_label_new(_("File"));

	gtk_widget_set_halign (GTK_WIDGET(label_title), GTK_ALIGN_END);
	gtk_widget_set_valign (GTK_WIDGET(label_title), GTK_ALIGN_CENTER);
	gtk_widget_set_halign (GTK_WIDGET(label_artist), GTK_ALIGN_END);
	gtk_widget_set_valign (GTK_WIDGET(label_artist), GTK_ALIGN_CENTER);
	gtk_widget_set_halign (GTK_WIDGET(label_album), GTK_ALIGN_END);
	gtk_widget_set_valign (GTK_WIDGET(label_album), GTK_ALIGN_CENTER);
	gtk_widget_set_halign (GTK_WIDGET(label_genre), GTK_ALIGN_END);
	gtk_widget_set_valign (GTK_WIDGET(label_genre), GTK_ALIGN_CENTER);
	gtk_widget_set_halign (GTK_WIDGET(label_tno), GTK_ALIGN_END);
	gtk_widget_set_valign (GTK_WIDGET(label_tno), GTK_ALIGN_CENTER);
	gtk_widget_set_halign (GTK_WIDGET(label_year), GTK_ALIGN_END);
	gtk_widget_set_valign (GTK_WIDGET(label_year), GTK_ALIGN_CENTER);
	gtk_widget_set_halign (GTK_WIDGET(label_comment), GTK_ALIGN_END);
	gtk_widget_set_valign (GTK_WIDGET(label_comment), GTK_ALIGN_START);
	gtk_widget_set_halign (GTK_WIDGET(label_file), GTK_ALIGN_END);
	gtk_widget_set_valign (GTK_WIDGET(label_file), GTK_ALIGN_CENTER);

	gtk_label_set_attribute_bold(GTK_LABEL(label_title));
	gtk_label_set_attribute_bold(GTK_LABEL(label_artist));
	gtk_label_set_attribute_bold(GTK_LABEL(label_album));
	gtk_label_set_attribute_bold(GTK_LABEL(label_genre));
	gtk_label_set_attribute_bold(GTK_LABEL(label_tno));
	gtk_label_set_attribute_bold(GTK_LABEL(label_year));
	gtk_label_set_attribute_bold(GTK_LABEL(label_comment));
	gtk_label_set_attribute_bold(GTK_LABEL(label_file));

	gtk_widget_show (GTK_WIDGET(label_title));
	gtk_widget_show (GTK_WIDGET(label_artist));
	gtk_widget_show (GTK_WIDGET(label_album));
	gtk_widget_show (GTK_WIDGET(label_genre));
	gtk_widget_show (GTK_WIDGET(label_tno));
	gtk_widget_show (GTK_WIDGET(label_year));
	gtk_widget_show (GTK_WIDGET(label_comment));
	gtk_widget_show (GTK_WIDGET(label_file));

	/* Create entry fields */

	entry_title = gtk_entry_new();
	entry_artist = gtk_entry_new();
	entry_album = gtk_entry_new();
	entry_genre = gtk_entry_new();

	entry_tno = gtk_spin_button_new_with_range (0, 2030, 1);
	entry_year = gtk_spin_button_new_with_range (0, 2030, 1);

	entry_comment = gtk_text_view_new();
	gtk_text_view_set_accepts_tab (GTK_TEXT_VIEW (entry_comment), FALSE);
	gtk_widget_set_hexpand (entry_comment, TRUE);
	gtk_widget_set_vexpand (entry_comment, TRUE);

	entry_file = gtk_entry_new();

	gtk_entry_set_max_length(GTK_ENTRY(entry_title), TAG_MAX_LEN);
	gtk_entry_set_max_length(GTK_ENTRY(entry_artist), TAG_MAX_LEN);
	gtk_entry_set_max_length(GTK_ENTRY(entry_album), TAG_MAX_LEN);
	gtk_entry_set_max_length(GTK_ENTRY(entry_genre), TAG_MAX_LEN);

	completion = rena_tags_get_entry_completion_from_table ("ARTIST");
	gtk_entry_set_completion(GTK_ENTRY(entry_artist), completion);
	g_object_unref (completion);

	completion = rena_tags_get_entry_completion_from_table ("ALBUM");
	gtk_entry_set_completion(GTK_ENTRY(entry_album), completion);
	g_object_unref (completion);

	completion = rena_tags_get_entry_completion_from_table ("GENRE");
	gtk_entry_set_completion(GTK_ENTRY(entry_genre), completion);
	g_object_unref (completion);

	gtk_entry_set_icon_from_icon_name (GTK_ENTRY(entry_title), GTK_ENTRY_ICON_SECONDARY, "edit-clear");
	gtk_entry_set_icon_from_icon_name (GTK_ENTRY(entry_artist), GTK_ENTRY_ICON_SECONDARY, "edit-clear");
	gtk_entry_set_icon_from_icon_name (GTK_ENTRY(entry_album), GTK_ENTRY_ICON_SECONDARY, "edit-clear");
	gtk_entry_set_icon_from_icon_name (GTK_ENTRY(entry_genre), GTK_ENTRY_ICON_SECONDARY, "edit-clear");

	gtk_entry_set_icon_from_icon_name (GTK_ENTRY(entry_file), GTK_ENTRY_ICON_PRIMARY, "folder");
	gtk_entry_set_icon_from_icon_name (GTK_ENTRY(entry_file), GTK_ENTRY_ICON_SECONDARY, "go-jump");

	gtk_editable_set_editable (GTK_EDITABLE(entry_file), FALSE);

	/* Create checkboxes. */

	chk_title = gtk_check_button_new();
	chk_artist = gtk_check_button_new();
	chk_album = gtk_check_button_new();
	chk_genre = gtk_check_button_new();
	chk_year = gtk_check_button_new();
	chk_tno = gtk_check_button_new();
	chk_comment = gtk_check_button_new();

	/* Connect signals. */

	g_signal_connect(G_OBJECT(entry_title),
	                 "changed",
	                 G_CALLBACK(rena_title_entry_change), dialog);
	g_signal_connect(G_OBJECT(entry_artist),
	                 "changed",
	                 G_CALLBACK(rena_artist_entry_change), dialog);
	g_signal_connect(G_OBJECT(entry_album),
	                 "changed",
	                 G_CALLBACK(rena_album_entry_change), dialog);
	g_signal_connect(G_OBJECT(entry_genre),
	                 "changed",
	                 G_CALLBACK(rena_tag_entry_change), chk_genre);
	g_signal_connect(G_OBJECT(entry_tno),
	                 "changed",
	                 G_CALLBACK(rena_tag_entry_change), chk_tno);
	g_signal_connect(G_OBJECT(entry_year),
	                 "changed",
	                 G_CALLBACK(rena_tag_entry_change), chk_year);
	g_signal_connect(G_OBJECT(gtk_text_view_get_buffer (GTK_TEXT_VIEW (entry_comment))),
	                 "changed",
	                 G_CALLBACK(rena_tag_entry_change), chk_comment);

	g_signal_connect(G_OBJECT(entry_title),
	                 "icon-press",
	                 G_CALLBACK(rena_tag_entry_clear_pressed), NULL);
	g_signal_connect(G_OBJECT(entry_artist),
	                 "icon-press",
	                 G_CALLBACK(rena_tag_entry_clear_pressed), NULL);
	g_signal_connect(G_OBJECT(entry_album),
	                 "icon-press",
	                 G_CALLBACK(rena_tag_entry_clear_pressed), NULL);
	g_signal_connect(G_OBJECT(entry_genre),
	                 "icon-press",
	                 G_CALLBACK(rena_tag_entry_clear_pressed), NULL);
	g_signal_connect(G_OBJECT(entry_file),
	                 "icon-press",
	                 G_CALLBACK(rena_tag_entry_directory_pressed), dialog);

	g_signal_connect(G_OBJECT(entry_tno),
	                 "button-release-event",
	                 G_CALLBACK(rena_tag_entry_select_text_on_click), NULL);
	g_signal_connect(G_OBJECT(entry_year),
	                 "button-release-event",
	                 G_CALLBACK(rena_tag_entry_select_text_on_click), NULL);

	g_signal_connect (G_OBJECT(entry_file),
	                  "populate-popup",
	                  G_CALLBACK (rena_file_entry_populate_menu), dialog);

	g_signal_connect (G_OBJECT(chk_title), "toggled",
	                  G_CALLBACK(rena_title_check_toggled), dialog);
	g_signal_connect (G_OBJECT(chk_artist), "toggled",
	                  G_CALLBACK(rena_artist_check_toggled), dialog);
	g_signal_connect (G_OBJECT(chk_album), "toggled",
	                  G_CALLBACK(rena_album_check_toggled), dialog);
	g_signal_connect (G_OBJECT(chk_genre), "toggled",
	                  G_CALLBACK(rena_genre_check_toggled), dialog);
	g_signal_connect (G_OBJECT(chk_tno), "toggled",
	                  G_CALLBACK(rena_track_no_check_toggled), dialog);
	g_signal_connect (G_OBJECT(chk_year), "toggled",
	                  G_CALLBACK(rena_year_check_toggled), dialog);
	g_signal_connect (G_OBJECT(chk_comment), "toggled",
	                  G_CALLBACK(rena_comment_check_toggled), dialog);

	/* Create boxs and package all. */

	hbox_title = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	hbox_artist = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	hbox_album = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	hbox_genre = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	hbox_year = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	hbox_tno = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	hbox_comment = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

	hbox_spins = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

	/* Create hobxs(ENTRY CHECHK) and attach in table */

	gtk_box_pack_start (GTK_BOX(hbox_title), entry_title, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(hbox_title), chk_title, FALSE, FALSE, 0);
	gtk_widget_set_hexpand (hbox_title, TRUE);
	gtk_widget_show(GTK_WIDGET(entry_title));
	gtk_widget_show(GTK_WIDGET(hbox_title));

	gtk_grid_attach (GTK_GRID(tag_table), label_title, 0, 0, 1, 1);
	gtk_grid_attach (GTK_GRID(tag_table), hbox_title, 1, 0, 1, 1);

	gtk_box_pack_start(GTK_BOX(hbox_artist), entry_artist, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_artist), chk_artist, FALSE, FALSE, 0);
	gtk_widget_set_hexpand (hbox_artist, TRUE);
	gtk_widget_show(GTK_WIDGET(entry_artist));
	gtk_widget_show(GTK_WIDGET(hbox_artist));

	gtk_grid_attach (GTK_GRID(tag_table), label_artist, 0, 1, 1, 1);
	gtk_grid_attach (GTK_GRID(tag_table), hbox_artist, 1, 1, 1, 1);

	gtk_box_pack_start (GTK_BOX(hbox_album), entry_album, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(hbox_album), chk_album, FALSE, FALSE, 0);
	gtk_widget_set_hexpand (hbox_album, TRUE);
	gtk_widget_show(GTK_WIDGET(entry_album));
	gtk_widget_show(GTK_WIDGET(hbox_album));

	gtk_grid_attach (GTK_GRID(tag_table), label_album, 0, 2, 1, 1);
	gtk_grid_attach (GTK_GRID(tag_table), hbox_album, 1, 2, 1, 1);

	gtk_box_pack_start (GTK_BOX(hbox_genre), entry_genre, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(hbox_genre), chk_genre, FALSE, FALSE, 0);
	gtk_widget_set_hexpand (hbox_genre, TRUE);
	gtk_widget_show(GTK_WIDGET(entry_genre));
	gtk_widget_show(GTK_WIDGET(hbox_genre));

	gtk_grid_attach (GTK_GRID(tag_table), label_genre, 0, 3, 1, 1);
	gtk_grid_attach (GTK_GRID(tag_table), hbox_genre, 1, 3, 1, 1);

	gtk_box_pack_start (GTK_BOX(hbox_tno), entry_tno, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_tno), chk_tno, FALSE, FALSE, 0);
	gtk_widget_show(GTK_WIDGET(entry_tno));
	gtk_widget_show(GTK_WIDGET(hbox_tno));

	gtk_box_pack_start (GTK_BOX(hbox_year), label_year, FALSE, FALSE, 5);
	gtk_box_pack_start (GTK_BOX(hbox_year), entry_year, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(hbox_year), chk_year, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX(hbox_spins), hbox_tno, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(hbox_spins), hbox_year, TRUE, TRUE, 0);
	gtk_widget_set_hexpand (hbox_spins, TRUE);
	gtk_widget_show(GTK_WIDGET(entry_year));
	gtk_widget_show(GTK_WIDGET(hbox_year));
	gtk_widget_show(GTK_WIDGET(hbox_spins));

	gtk_grid_attach (GTK_GRID(tag_table), label_tno, 0, 4, 1, 1);
	gtk_grid_attach (GTK_GRID(tag_table), hbox_spins, 1, 4, 1, 1);

	comment_view_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(comment_view_scroll),
	                               GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(comment_view_scroll),
	                                    GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(comment_view_scroll), entry_comment);

	gtk_widget_set_halign (GTK_WIDGET(chk_comment), GTK_ALIGN_CENTER);
	gtk_widget_set_valign (GTK_WIDGET(chk_comment), GTK_ALIGN_START);

	gtk_box_pack_start (GTK_BOX(hbox_comment), comment_view_scroll, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(hbox_comment), chk_comment, FALSE, FALSE, 0);
	gtk_widget_set_hexpand (hbox_comment, TRUE);
	gtk_widget_show_all(GTK_WIDGET(comment_view_scroll));
	gtk_widget_show(GTK_WIDGET(hbox_comment));

	gtk_grid_attach (GTK_GRID(tag_table), label_comment, 0, 5, 1, 1);
	gtk_grid_attach (GTK_GRID(tag_table), hbox_comment, 1, 5, 1, 2);

	gtk_widget_set_hexpand (entry_file, TRUE);
	gtk_widget_show(GTK_WIDGET(entry_file));

	gtk_grid_attach (GTK_GRID(tag_table), label_file, 0, 7, 1, 1);
	gtk_grid_attach (GTK_GRID(tag_table), entry_file, 1, 7, 1, 1);

	/* Save changes when press enter. */

	gtk_entry_set_activates_default (GTK_ENTRY(entry_title), TRUE);
	gtk_entry_set_activates_default (GTK_ENTRY(entry_artist), TRUE);
	gtk_entry_set_activates_default (GTK_ENTRY(entry_album), TRUE);
	gtk_entry_set_activates_default (GTK_ENTRY(entry_genre), TRUE);
	gtk_entry_set_activates_default (GTK_ENTRY(entry_tno), TRUE);
	gtk_entry_set_activates_default (GTK_ENTRY(entry_year), TRUE);

	/* Storage widgets and its to dialog. */

	dialog->header = header;
	dialog->title_entry = entry_title;
	dialog->artist_entry = entry_artist;
	dialog->album_entry = entry_album;
	dialog->genre_entry = entry_genre;
	dialog->track_no_entry = entry_tno;
	dialog->year_entry = entry_year;
	dialog->comment_entry = entry_comment;
	dialog->file_entry = entry_file;

	dialog->title_check_change = chk_title;
	dialog->artist_check_change = chk_artist;
	dialog->album_check_change = chk_album;
	dialog->genre_check_change = chk_genre;
	dialog->track_no_check_change = chk_tno;
	dialog->year_check_change = chk_year;
	dialog->comment_check_change = chk_comment;

	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), GTK_WIDGET(header), FALSE, FALSE, 0);
	gtk_widget_show_all(GTK_WIDGET(header));

	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), tag_table, TRUE, TRUE, 0);
	gtk_widget_show (tag_table);

	/* Init flags */
	dialog->mobj = rena_musicobject_new();
}

void
rena_tags_dialog_set_changed(RenaTagsDialog *dialog, gint changed)
{
	if(changed & TAG_TNO_CHANGED)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->track_no_check_change), TRUE);
	if(changed & TAG_TITLE_CHANGED)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->title_check_change), TRUE);
	if(changed & TAG_ARTIST_CHANGED)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->artist_check_change), TRUE);
	if(changed & TAG_ALBUM_CHANGED)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->album_check_change), TRUE);
	if(changed & TAG_GENRE_CHANGED)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->genre_check_change), TRUE);
	if(changed & TAG_YEAR_CHANGED)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->year_check_change), TRUE);
	if(changed & TAG_COMMENT_CHANGED)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->comment_check_change), TRUE);
}

gint
rena_tags_dialog_get_changed(RenaTagsDialog *dialog)
{
	gint changed = 0;

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->track_no_check_change)))
		changed |= TAG_TNO_CHANGED;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->title_check_change)))
		changed |= TAG_TITLE_CHANGED;
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->artist_check_change)))
		changed |= TAG_ARTIST_CHANGED;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->album_check_change)))
		changed |= TAG_ALBUM_CHANGED;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->genre_check_change)))
		changed |= TAG_GENRE_CHANGED;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->year_check_change)))
		changed |= TAG_YEAR_CHANGED;
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->comment_check_change)))
		changed |= TAG_COMMENT_CHANGED;

	return changed;
}

void
rena_tags_dialog_set_musicobject(RenaTagsDialog *dialog, RenaMusicobject *mobj)
{
	const gchar *title, *artist, *album, *genre, *comment, *file;
	gint track_no, year;
	GtkTextBuffer *buffer;
	gchar *str_title, *str_subtitle;

	g_object_unref(dialog->mobj);
	dialog->mobj = rena_musicobject_dup (mobj);

	title = rena_musicobject_get_title(mobj);
	artist = rena_musicobject_get_artist(mobj);
	album = rena_musicobject_get_album(mobj);
	genre = rena_musicobject_get_genre(mobj);
	track_no = rena_musicobject_get_track_no(mobj);
	year = rena_musicobject_get_year(mobj);
	comment = rena_musicobject_get_comment(mobj);
	file = rena_musicobject_get_file(mobj);

	str_title = rena_tags_dialog_get_title (title, file);
	rena_header_set_title (dialog->header, str_title);
	g_free (str_title);

	str_subtitle = rena_tags_dialog_get_subtitle (artist, album);
	rena_header_set_subtitle (dialog->header, str_subtitle);
	g_free (str_subtitle);

	/* Block changed signal, and force text. */

	g_signal_handlers_block_by_func(G_OBJECT(dialog->title_entry), rena_title_entry_change, dialog);
	gtk_entry_set_text(GTK_ENTRY(dialog->title_entry), title);
	g_signal_handlers_unblock_by_func(G_OBJECT(dialog->title_entry), rena_title_entry_change, dialog);

	g_signal_handlers_block_by_func(G_OBJECT(dialog->artist_entry), rena_artist_entry_change, dialog);
	gtk_entry_set_text(GTK_ENTRY(dialog->artist_entry), artist);
	g_signal_handlers_unblock_by_func(G_OBJECT(dialog->artist_entry), rena_artist_entry_change, dialog);

	g_signal_handlers_block_by_func(G_OBJECT(dialog->album_entry), rena_album_entry_change, dialog);
	gtk_entry_set_text(GTK_ENTRY(dialog->album_entry), album);
	g_signal_handlers_unblock_by_func(G_OBJECT(dialog->album_entry), rena_album_entry_change, dialog);

	g_signal_handlers_block_by_func(G_OBJECT(dialog->genre_entry), rena_tag_entry_change, dialog->genre_check_change);
	gtk_entry_set_text(GTK_ENTRY(dialog->genre_entry), genre);
	g_signal_handlers_unblock_by_func(G_OBJECT(dialog->genre_entry), rena_tag_entry_change, dialog->genre_check_change);

	g_signal_handlers_block_by_func(G_OBJECT(dialog->track_no_entry), rena_tag_entry_change, dialog->track_no_check_change);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->track_no_entry), track_no);
	g_signal_handlers_unblock_by_func(G_OBJECT(dialog->track_no_entry), rena_tag_entry_change, dialog->track_no_check_change);

	g_signal_handlers_block_by_func(G_OBJECT(dialog->year_entry), rena_tag_entry_change, dialog->year_check_change);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->year_entry), year);
	g_signal_handlers_unblock_by_func(G_OBJECT(dialog->year_entry), rena_tag_entry_change, dialog->year_check_change);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (dialog->comment_entry));
	g_signal_handlers_block_by_func(G_OBJECT(buffer), rena_tag_entry_change, dialog->comment_check_change);
	gtk_text_buffer_set_text (buffer, comment, -1);
	g_signal_handlers_unblock_by_func(G_OBJECT(buffer), rena_tag_entry_change, dialog->comment_check_change);

	if (string_is_empty(file))
		gtk_widget_set_sensitive(GTK_WIDGET(dialog->file_entry), FALSE);
	else {
		gtk_entry_set_text(GTK_ENTRY(dialog->file_entry), file);
		gtk_editable_set_position(GTK_EDITABLE(dialog->file_entry), g_utf8_strlen(file, -1));
		gtk_dialog_add_button(GTK_DIALOG(dialog), _("Details"), GTK_RESPONSE_HELP);
	}
}

RenaMusicobject *
rena_tags_dialog_get_musicobject(RenaTagsDialog *dialog)
{
	GtkTextBuffer *buffer;
	GtkTextIter start, end;

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->track_no_check_change)))
		rena_musicobject_set_track_no(dialog->mobj,
		                                gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(dialog->track_no_entry)));
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->title_check_change)))
		rena_musicobject_set_title(dialog->mobj,
		                             gtk_entry_get_text (GTK_ENTRY(dialog->title_entry)));
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->artist_check_change)))
		rena_musicobject_set_artist(dialog->mobj,
		                              gtk_entry_get_text (GTK_ENTRY(dialog->artist_entry)));
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->album_check_change)))
		rena_musicobject_set_album(dialog->mobj,
		                             gtk_entry_get_text (GTK_ENTRY(dialog->album_entry)));
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->genre_check_change)))
		rena_musicobject_set_genre(dialog->mobj,
		                             gtk_entry_get_text (GTK_ENTRY(dialog->genre_entry)));
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->year_check_change)))
		rena_musicobject_set_year(dialog->mobj,
		                            gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(dialog->year_entry)));
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dialog->comment_check_change))) {
		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (dialog->comment_entry));
		gtk_text_buffer_get_start_iter (buffer, &start);
		gtk_text_buffer_get_end_iter (buffer, &end);
		rena_musicobject_set_comment(dialog->mobj,
		                               gtk_text_buffer_get_text (buffer, &start, &end, FALSE));
	}

	return dialog->mobj;
}

static void
rena_tags_dialog_dispose (GObject *object)
{
	RenaTagsDialog *dialog = RENA_TAGS_DIALOG (object);

	if(dialog->mobj) {
		g_object_unref(dialog->mobj);
		dialog->mobj = NULL;
	}

	(*G_OBJECT_CLASS (rena_tags_dialog_parent_class)->dispose) (object);
}

static void
rena_tags_dialog_finalize (GObject *object)
{
	//RenaTagsDialog *dialog = RENA_TAGS_DIALOG (object);

	/*
	 * Need free dialog->loc_arr or dialog->rlist?
	 */

	(*G_OBJECT_CLASS (rena_tags_dialog_parent_class)->finalize) (object);
}

GtkWidget *
rena_tags_dialog_new (void)
{
  return g_object_new (RENA_TYPE_TAGS_DIALOG, NULL);
}


/*
 * Track properties dialog
 */

static void
rena_track_properties_response(GtkDialog *dialog,
                                 gint response,
                                 gpointer data)
{
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

void
rena_track_properties_dialog(RenaMusicobject *mobj,
                               GtkWidget *parent)
{
	GtkWidget *dialog;
	GtkWidget *properties_table;
	GtkWidget *label_length, *label_bitrate, *label_channels, *label_samplerate, *label_folder, *label_filename, *label_mimetype;
	GtkWidget *info_length, *info_bitrate, *info_channels, *info_samplerate, *info_folder, *info_filename, *info_mimetype;

	gchar *length = NULL, *bitrate = NULL, *channels = NULL, *samplerate = NULL, *folder = NULL, *filename = NULL;
	const gchar *mimetype = NULL;

	if(!mobj)
		return;

	gint channels_n = rena_musicobject_get_channels(mobj);

	length = convert_length_str(rena_musicobject_get_length(mobj));
	bitrate = g_strdup_printf("%d kbps", rena_musicobject_get_bitrate(mobj));
	channels = g_strdup_printf("%d %s", channels_n, ngettext("channel", "channels", channels_n));
	samplerate = g_strdup_printf("%d Hz", rena_musicobject_get_samplerate(mobj));
	folder = get_display_filename(rena_musicobject_get_file(mobj), TRUE);
	filename = get_display_name(mobj);
	mimetype = rena_musicobject_get_mime_type(mobj);

	/* Create table */

	properties_table = gtk_grid_new ();

	gtk_grid_set_row_spacing (GTK_GRID(properties_table), 5);
	gtk_grid_set_column_spacing (GTK_GRID(properties_table), 5);

	gtk_container_set_border_width (GTK_CONTAINER(properties_table), 5);

	/* Create labels */

	label_length = gtk_label_new(_("Length"));
	label_bitrate = gtk_label_new(_("Bitrate"));
	label_channels = gtk_label_new(_("Channels"));
	label_samplerate = gtk_label_new(_("Samplerate"));
	label_folder = gtk_label_new(_("Folder"));
	label_filename = gtk_label_new(_("Filename"));
	label_mimetype = gtk_label_new(_("Mimetype"));

	gtk_widget_set_halign (GTK_WIDGET(label_length), GTK_ALIGN_END);
	gtk_widget_set_valign (GTK_WIDGET(label_length), GTK_ALIGN_START);
	gtk_widget_set_halign (GTK_WIDGET(label_bitrate), GTK_ALIGN_END);
	gtk_widget_set_valign (GTK_WIDGET(label_bitrate), GTK_ALIGN_START);
	gtk_widget_set_halign (GTK_WIDGET(label_channels), GTK_ALIGN_END);
	gtk_widget_set_valign (GTK_WIDGET(label_channels), GTK_ALIGN_START);
	gtk_widget_set_halign (GTK_WIDGET(label_samplerate), GTK_ALIGN_END);
	gtk_widget_set_valign (GTK_WIDGET(label_samplerate), GTK_ALIGN_START);
	gtk_widget_set_halign (GTK_WIDGET(label_folder), GTK_ALIGN_END);
	gtk_widget_set_valign (GTK_WIDGET(label_folder), GTK_ALIGN_START);
	gtk_widget_set_halign (GTK_WIDGET(label_filename), GTK_ALIGN_END);
	gtk_widget_set_valign (GTK_WIDGET(label_filename), GTK_ALIGN_START);
	gtk_widget_set_halign (GTK_WIDGET(label_mimetype), GTK_ALIGN_END);
	gtk_widget_set_valign (GTK_WIDGET(label_mimetype), GTK_ALIGN_START);

	gtk_label_set_attribute_bold(GTK_LABEL(label_length));
	gtk_label_set_attribute_bold(GTK_LABEL(label_bitrate));
	gtk_label_set_attribute_bold(GTK_LABEL(label_channels));
	gtk_label_set_attribute_bold(GTK_LABEL(label_samplerate));
	gtk_label_set_attribute_bold(GTK_LABEL(label_folder));
	gtk_label_set_attribute_bold(GTK_LABEL(label_filename));
	gtk_label_set_attribute_bold(GTK_LABEL(label_mimetype));

	/* Create info labels */

	info_length = gtk_label_new(length);
	info_bitrate = gtk_label_new(bitrate);
	info_channels = gtk_label_new(channels);
	info_samplerate = gtk_label_new(samplerate);
	info_folder = gtk_label_new(folder);
	info_filename = gtk_label_new(filename);
	info_mimetype = gtk_label_new(mimetype);

	gtk_widget_set_halign (GTK_WIDGET(info_length), GTK_ALIGN_START);
	gtk_widget_set_valign (GTK_WIDGET(info_length), GTK_ALIGN_START);
	gtk_widget_set_halign (GTK_WIDGET(info_bitrate), GTK_ALIGN_START);
	gtk_widget_set_valign (GTK_WIDGET(info_bitrate), GTK_ALIGN_START);
	gtk_widget_set_halign (GTK_WIDGET(info_channels), GTK_ALIGN_START);
	gtk_widget_set_valign (GTK_WIDGET(info_channels), GTK_ALIGN_START);
	gtk_widget_set_halign (GTK_WIDGET(info_samplerate), GTK_ALIGN_START);
	gtk_widget_set_valign (GTK_WIDGET(info_samplerate), GTK_ALIGN_START);
	gtk_widget_set_halign (GTK_WIDGET(info_folder), GTK_ALIGN_START);
	gtk_widget_set_valign (GTK_WIDGET(info_folder), GTK_ALIGN_START);
	gtk_widget_set_halign (GTK_WIDGET(info_filename), GTK_ALIGN_START);
	gtk_widget_set_valign (GTK_WIDGET(info_filename), GTK_ALIGN_START);
	gtk_widget_set_halign (GTK_WIDGET(info_mimetype), GTK_ALIGN_START);
	gtk_widget_set_valign (GTK_WIDGET(info_mimetype), GTK_ALIGN_START);

	gtk_label_set_selectable(GTK_LABEL(info_length), TRUE);
	gtk_label_set_selectable(GTK_LABEL(info_bitrate), TRUE);
	gtk_label_set_selectable(GTK_LABEL(info_channels), TRUE);
	gtk_label_set_selectable(GTK_LABEL(info_samplerate), TRUE);
	gtk_label_set_selectable(GTK_LABEL(info_folder), TRUE);
	gtk_label_set_selectable(GTK_LABEL(info_filename), TRUE);
	gtk_label_set_selectable(GTK_LABEL(info_mimetype), TRUE);

	gtk_widget_set_hexpand (GTK_WIDGET(info_length), TRUE);
	gtk_widget_set_hexpand (GTK_WIDGET(info_bitrate), TRUE);
	gtk_widget_set_hexpand (GTK_WIDGET(info_channels), TRUE);
	gtk_widget_set_hexpand (GTK_WIDGET(info_samplerate), TRUE);
	gtk_widget_set_hexpand (GTK_WIDGET(info_folder), TRUE);
	gtk_widget_set_hexpand (GTK_WIDGET(info_filename), TRUE);
	gtk_widget_set_hexpand (GTK_WIDGET(info_mimetype), TRUE);

	/* Attach labels */

	gtk_grid_attach (GTK_GRID(properties_table), label_length, 0, 0, 1, 1);
	gtk_grid_attach (GTK_GRID(properties_table), info_length, 1, 0, 1, 1);

	gtk_grid_attach (GTK_GRID(properties_table), label_bitrate, 0, 1, 1, 1);
	gtk_grid_attach (GTK_GRID(properties_table), info_bitrate, 1, 1, 1, 1);

	gtk_grid_attach (GTK_GRID(properties_table), label_channels, 0, 2, 1, 1);
	gtk_grid_attach (GTK_GRID(properties_table), info_channels, 1, 2, 1, 1);

	gtk_grid_attach (GTK_GRID(properties_table), label_samplerate, 0, 3, 1, 1);
	gtk_grid_attach (GTK_GRID(properties_table), info_samplerate, 1, 3, 1, 1);

	gtk_grid_attach (GTK_GRID(properties_table), label_folder, 0, 4, 1, 1);
	gtk_grid_attach (GTK_GRID(properties_table), info_folder, 1, 4, 1, 1);

	gtk_grid_attach (GTK_GRID(properties_table), label_filename, 0, 5, 1, 1);
	gtk_grid_attach (GTK_GRID(properties_table), info_filename, 1, 5, 1, 1);

	gtk_grid_attach (GTK_GRID(properties_table), label_mimetype, 0, 6, 1, 1);
	gtk_grid_attach (GTK_GRID(properties_table), info_mimetype, 1, 6, 1, 1);

	/* The main edit dialog */

	dialog = gtk_dialog_new_with_buttons (_("Details"),
	                                      GTK_WINDOW(parent),
	                                      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	                                      _("_Ok"), GTK_RESPONSE_OK,
	                                      NULL);

	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), properties_table);

	g_signal_connect(G_OBJECT(dialog), "response",
			G_CALLBACK(rena_track_properties_response), NULL);

	gtk_widget_show_all(dialog);

	g_free(length);
	g_free(bitrate);
	g_free(channels);
	g_free(samplerate);
	g_free(folder);
	g_free(filename);
}

/*
 * Track tags editing dialog.
 */

static void
rena_title_entry_change (GtkEntry *entry, RenaTagsDialog *dialog)
{
	gchar *str_title = NULL;
	const gchar *title = NULL, *file = NULL;

	title = gtk_entry_get_text (GTK_ENTRY(dialog->title_entry));
	file = gtk_entry_get_text (GTK_ENTRY(dialog->file_entry));

	str_title = rena_tags_dialog_get_title (title, file);
	rena_header_set_title (dialog->header, str_title);
	g_free (str_title);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(dialog->title_check_change), TRUE);
	gtk_widget_show (GTK_WIDGET(dialog->title_check_change));
}

static void
rena_title_check_toggled (GtkToggleButton *toggle, RenaTagsDialog *dialog)
{
	const gchar *title = NULL;
	if (!gtk_toggle_button_get_active(toggle))
	{
		title = rena_musicobject_get_title (dialog->mobj);
		gtk_entry_set_text (GTK_ENTRY(dialog->title_entry), title);
		gtk_widget_hide (GTK_WIDGET(dialog->title_check_change));
	}
}

static void
rena_artist_entry_change (GtkEntry *entry, RenaTagsDialog *dialog)
{
	gchar *str_subtitle = NULL;
	const gchar *artist = NULL, *album = NULL;

	artist = gtk_entry_get_text (GTK_ENTRY(dialog->artist_entry));
	album = gtk_entry_get_text (GTK_ENTRY(dialog->album_entry));

	str_subtitle = rena_tags_dialog_get_subtitle (artist, album);
	rena_header_set_subtitle (dialog->header, str_subtitle);
	g_free (str_subtitle);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(dialog->artist_check_change), TRUE);
	gtk_widget_show (GTK_WIDGET(dialog->artist_check_change));
}

static void
rena_artist_check_toggled (GtkToggleButton *toggle, RenaTagsDialog *dialog)
{
	const gchar *artist = NULL;
	if (!gtk_toggle_button_get_active(toggle))
	{
		artist = rena_musicobject_get_artist (dialog->mobj);
		gtk_entry_set_text (GTK_ENTRY(dialog->artist_entry), artist);
		gtk_widget_hide (GTK_WIDGET(dialog->artist_check_change));
	}
}

static void
rena_album_entry_change (GtkEntry *entry, RenaTagsDialog *dialog)
{
	gchar *str_subtitle = NULL;
	const gchar *artist = NULL, *album = NULL;

	artist = gtk_entry_get_text (GTK_ENTRY(dialog->artist_entry));
	album = gtk_entry_get_text (GTK_ENTRY(dialog->album_entry));

	str_subtitle = rena_tags_dialog_get_subtitle (artist, album);
	rena_header_set_subtitle (dialog->header, str_subtitle);
	g_free (str_subtitle);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(dialog->album_check_change), TRUE);
	gtk_widget_show (GTK_WIDGET(dialog->album_check_change));
}

static void
rena_album_check_toggled (GtkToggleButton *toggle, RenaTagsDialog *dialog)
{
	const gchar *album = NULL;
	if (!gtk_toggle_button_get_active(toggle))
	{
		album = rena_musicobject_get_album (dialog->mobj);
		gtk_entry_set_text (GTK_ENTRY(dialog->album_entry), album);
		gtk_widget_hide (GTK_WIDGET(dialog->album_check_change));
	}
}

static void
rena_genre_check_toggled (GtkToggleButton *toggle, RenaTagsDialog *dialog)
{
	const gchar *genre = NULL;
	if (!gtk_toggle_button_get_active(toggle))
	{
		genre = rena_musicobject_get_genre (dialog->mobj);
		gtk_entry_set_text (GTK_ENTRY(dialog->genre_entry), genre);
		gtk_widget_hide (GTK_WIDGET(dialog->genre_check_change));
	}
}

static void
rena_track_no_check_toggled (GtkToggleButton *toggle, RenaTagsDialog *dialog)
{
	if (!gtk_toggle_button_get_active(toggle))
	{
		gtk_spin_button_set_value (GTK_SPIN_BUTTON(dialog->track_no_entry),
			rena_musicobject_get_track_no (dialog->mobj));
		gtk_widget_hide (GTK_WIDGET(dialog->track_no_check_change));
	}
}

static void
rena_year_check_toggled (GtkToggleButton *toggle, RenaTagsDialog *dialog)
{
	if (!gtk_toggle_button_get_active(toggle))
	{
		gtk_spin_button_set_value (GTK_SPIN_BUTTON(dialog->year_entry),
			rena_musicobject_get_year (dialog->mobj));
		gtk_widget_hide (GTK_WIDGET(dialog->year_check_change));
	}
}

static void
rena_comment_check_toggled (GtkToggleButton *toggle, RenaTagsDialog *dialog)
{
	GtkTextBuffer *buffer;
	const gchar *comment = NULL;
	if (!gtk_toggle_button_get_active(toggle))
	{
		comment = rena_musicobject_get_comment (dialog->mobj);
		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (dialog->comment_entry));
		gtk_text_buffer_set_text (buffer, comment, -1);
		gtk_widget_hide (GTK_WIDGET(dialog->comment_check_change));
	}
}

static void
rena_tag_entry_change (GtkEntry *entry, GtkCheckButton *check)
{
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), TRUE);
	gtk_widget_show (GTK_WIDGET(check));
}

static void
rena_tag_entry_clear_pressed (GtkEntry       *entry,
                                gint            position,
                                GdkEventButton *event)
{
	if (position == GTK_ENTRY_ICON_SECONDARY) {
		gtk_entry_set_text (entry, "");
		gtk_widget_grab_focus(GTK_WIDGET(entry));
	}
}

static void
rena_tag_entry_directory_pressed (GtkEntry       *entry,
                                    gint            position,
                                    GdkEventButton *event,
                                    gpointer        user_data)
{
	RenaMusicobject *mobj;
	GtkWidget *toplevel;

	RenaTagsDialog *dialog = user_data;

	if (position == GTK_ENTRY_ICON_SECONDARY) {
		mobj = rena_tags_dialog_get_musicobject(dialog);
		toplevel = gtk_widget_get_toplevel(GTK_WIDGET(entry));

		gchar *uri = path_get_dir_as_uri (rena_musicobject_get_file(mobj));
		open_url(uri, toplevel);
		g_free (uri);
	}
}

static gboolean
rena_tag_entry_select_text_on_click (GtkWidget *widget,
                                       GdkEvent  *event,
                                       gpointer   user_data)
{
	gtk_editable_select_region(GTK_EDITABLE(widget), 0, -1);

	return FALSE;
}

gchar *
rena_tag_entry_get_selected_text(GtkWidget *entry)
{
	gint start_sel, end_sel;

	if (!gtk_editable_get_selection_bounds (GTK_EDITABLE(entry), &start_sel, &end_sel))
		return NULL;

	return gtk_editable_get_chars (GTK_EDITABLE(entry), start_sel, end_sel);
}

static void
rena_tag_entry_set_text(GtkWidget *entry, const gchar *text)
{
	gtk_entry_set_text(GTK_ENTRY(entry), text);
	gtk_widget_grab_focus(GTK_WIDGET(entry));
}

static void
rena_file_entry_open_folder (GtkMenuItem *menuitem, RenaTagsDialog *dialog)
{
	GtkWidget *toplevel;
	const gchar *file;
	gchar *uri;

	file = gtk_entry_get_text (GTK_ENTRY(dialog->file_entry));
	toplevel = gtk_widget_get_toplevel(GTK_WIDGET(dialog->file_entry));

	uri = path_get_dir_as_uri (file);
	open_url(uri, toplevel);
	g_free (uri);
}

static void
rena_file_entry_selection_to_title (GtkMenuItem *menuitem,RenaTagsDialog *dialog)
{
	gchar *text = rena_tag_entry_get_selected_text(dialog->file_entry);
	if(text) {
		rena_tag_entry_set_text(dialog->title_entry, text);
		g_free(text);
	}
}

static void
rena_file_entry_selection_to_artist (GtkMenuItem *menuitem, RenaTagsDialog *dialog)
{
	gchar *text = rena_tag_entry_get_selected_text(dialog->file_entry);
	if(text) {
		rena_tag_entry_set_text(dialog->artist_entry, text);
		g_free(text);
	}
}

static void
rena_file_entry_selection_to_album (GtkMenuItem *menuitem, RenaTagsDialog *dialog)
{
	gchar *text = rena_tag_entry_get_selected_text(dialog->file_entry);
	if(text) {
		rena_tag_entry_set_text(dialog->album_entry, text);
		g_free(text);
	}
}

static void
rena_file_entry_selection_to_genre (GtkMenuItem *menuitem, RenaTagsDialog *dialog)
{
	gchar *text = rena_tag_entry_get_selected_text(dialog->file_entry);
	if(text) {
		rena_tag_entry_set_text(dialog->genre_entry, text);
		g_free(text);
	}
}

static void
rena_file_entry_selection_to_comment (GtkMenuItem *menuitem, RenaTagsDialog *dialog)
{
	GtkTextBuffer *buffer;

	gchar *text = rena_tag_entry_get_selected_text(dialog->file_entry);
	if(text) {
		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (dialog->comment_entry));
		gtk_text_buffer_set_text (buffer, text, -1);
		g_free(text);
	}
}

static void
rena_file_entry_populate_menu (GtkEntry *entry, GtkMenu *menu, gpointer user_data)
{
	GtkWidget *submenu, *item;

	RenaTagsDialog *dialog = user_data;

	item = gtk_separator_menu_item_new ();
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
	gtk_widget_show (item);

	item = gtk_menu_item_new_with_mnemonic (_("Selection to"));
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
	gtk_widget_show (item);

	submenu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);

	item = gtk_menu_item_new_with_label (_("Title"));
	g_signal_connect (G_OBJECT (item),
	                  "activate",
	                  G_CALLBACK (rena_file_entry_selection_to_title), dialog);
	gtk_menu_shell_append (GTK_MENU_SHELL (submenu), item);
	item = gtk_menu_item_new_with_label (_("Artist"));
	g_signal_connect (G_OBJECT (item),
	                  "activate",
	                  G_CALLBACK (rena_file_entry_selection_to_artist), dialog);
	gtk_menu_shell_append (GTK_MENU_SHELL (submenu), item);
	item = gtk_menu_item_new_with_label (_("Album"));
	g_signal_connect (G_OBJECT (item),
	                  "activate",
	                  G_CALLBACK (rena_file_entry_selection_to_album), dialog);
	gtk_menu_shell_append (GTK_MENU_SHELL (submenu), item);
	item = gtk_menu_item_new_with_label (_("Genre"));
	g_signal_connect (G_OBJECT (item),
	                  "activate",
	                  G_CALLBACK (rena_file_entry_selection_to_genre), dialog);
	gtk_menu_shell_append (GTK_MENU_SHELL (submenu), item);
	item = gtk_menu_item_new_with_label (_("Comment"));
	g_signal_connect (G_OBJECT (item),
	                  "activate",
	                  G_CALLBACK (rena_file_entry_selection_to_comment), dialog);
	gtk_menu_shell_append (GTK_MENU_SHELL (submenu), item);

	gtk_widget_show_all (submenu);

	if (!gtk_editable_get_selection_bounds (GTK_EDITABLE(dialog->file_entry), NULL, NULL))
		gtk_widget_set_sensitive (submenu, FALSE);

	item = gtk_menu_item_new_with_mnemonic (_("Open folder"));
	g_signal_connect (G_OBJECT (item),
	                  "activate",
	                  G_CALLBACK (rena_file_entry_open_folder), dialog);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
	gtk_widget_show (item);
}

GtkEntryCompletion *
rena_tags_get_entry_completion_from_table(const gchar *table)
{
	RenaDatabase *cdbase;
	RenaPreparedStatement *statement;
	gchar *sql;
	GtkEntryCompletion *completion;
	GtkListStore *model;
	GtkTreeIter iter;

	cdbase = rena_database_get ();

	model = gtk_list_store_new(1, G_TYPE_STRING);

	sql = g_strdup_printf("SELECT name FROM %s ORDER BY name DESC", table);
	statement = rena_database_create_statement (cdbase, sql);
	while (rena_prepared_statement_step (statement)) {
		const gchar *name = rena_prepared_statement_get_string (statement, 0);
		gtk_list_store_insert_with_values (GTK_LIST_STORE(model), &iter, 0, 0, name, -1);
	}
	rena_prepared_statement_free (statement);
	g_object_unref(G_OBJECT(cdbase));
	g_free(sql);

	completion = gtk_entry_completion_new();
	gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(model));
	gtk_entry_completion_set_text_column(completion, 0);
	g_object_unref(model);

	return completion;
}
