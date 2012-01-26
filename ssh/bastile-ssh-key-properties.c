/*
 * Bastile
 *
 * Copyright (C) 2005 Stefan Walter
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "config.h"

#include "bastile-gtkstock.h"
#include "bastile-object.h"
#include "bastile-object-widget.h"
#include "bastile-util.h"
#include "bastile-validity.h"

#include "common/bastile-bind.h"

#include "ssh/bastile-ssh-key.h"
#include "ssh/bastile-ssh-operation.h"

#include <glib/gi18n.h>

#define NOTEBOOK "notebook"

G_MODULE_EXPORT void
on_ssh_comment_activate (GtkWidget *entry, BastileWidget *swidget)
{
    BastileObject *object;
    BastileSSHKey *skey;
    BastileSource *sksrc;
    BastileOperation *op;
    const gchar *text;
    gchar *comment;
    GError *err = NULL;
    
    object = BASTILE_OBJECT_WIDGET (swidget)->object;
    skey = BASTILE_SSH_KEY (object);
    sksrc = bastile_object_get_source (object);
    g_return_if_fail (BASTILE_IS_SSH_SOURCE (sksrc));
    
    text = gtk_entry_get_text (GTK_ENTRY (entry));
    
    /* Make sure not the same */
    if (skey->keydata->comment && g_utf8_collate (text, skey->keydata->comment) == 0)
        return;

    gtk_widget_set_sensitive (entry, FALSE);
    
    comment = g_strdup (text);
    op = bastile_ssh_operation_rename (BASTILE_SSH_SOURCE (sksrc), skey, comment);
    g_free (comment);
    
    /* This is usually a quick operation */
    bastile_operation_wait (op);
    
    if (!bastile_operation_is_successful (op)) {
        bastile_operation_copy_error (op, &err);
        bastile_util_handle_error (err, _("Couldn't rename key."));
        g_clear_error (&err);
        gtk_entry_set_text (GTK_ENTRY (entry), skey->keydata->comment ? skey->keydata->comment : "");
    }
    
    gtk_widget_set_sensitive (entry, TRUE);
}

G_MODULE_EXPORT gboolean
on_ssh_comment_focus_out (GtkWidget* widget, GdkEventFocus *event, BastileWidget *swidget)
{
    on_ssh_comment_activate (widget, swidget);
    return FALSE;
}

G_MODULE_EXPORT void
on_ssh_trust_toggled (GtkToggleButton *button, BastileWidget *swidget)
{
    BastileSource *sksrc;
    BastileOperation *op;
    BastileObject *object;
    BastileSSHKey *skey;
    gboolean authorize;
    GError *err = NULL;

    object = BASTILE_OBJECT_WIDGET (swidget)->object;
    skey = BASTILE_SSH_KEY (object);
    sksrc = bastile_object_get_source (object);
    g_return_if_fail (BASTILE_IS_SSH_SOURCE (sksrc));
    
    authorize = gtk_toggle_button_get_active (button);
    gtk_widget_set_sensitive (GTK_WIDGET (button), FALSE);
    
    op = bastile_ssh_operation_authorize (BASTILE_SSH_SOURCE (sksrc), skey, authorize);
    g_return_if_fail (op);
    
    /* A very fast op, so just wait */
    bastile_operation_wait (op);
    
    if (!bastile_operation_is_successful (op)) {
        bastile_operation_copy_error (op, &err);
        bastile_util_handle_error (err, _("Couldn't change authorization for key."));
        g_clear_error (&err);
    }
    
    gtk_widget_set_sensitive (GTK_WIDGET (button), TRUE);
}

static void
passphrase_done (BastileOperation *op, BastileWidget *swidget)
{
    GError *err = NULL;
    GtkWidget *w;

    if (!bastile_operation_is_successful (op)) {
        bastile_operation_copy_error (op, &err);
        bastile_util_handle_error (err, _("Couldn't change passphrase for key."));
        g_clear_error (&err);
    }
    
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "passphrase-button"));
    g_return_if_fail (w != NULL);
    gtk_widget_set_sensitive (w, TRUE);
}

G_MODULE_EXPORT void
on_ssh_passphrase_button_clicked (GtkWidget *widget, BastileWidget *swidget)
{
    BastileOperation *op;
    BastileObject *object;
    GtkWidget *w;
    
    object = BASTILE_OBJECT_WIDGET (swidget)->object;
    g_assert (BASTILE_IS_SSH_KEY (object));

    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "passphrase-button"));
    g_return_if_fail (w != NULL);
    gtk_widget_set_sensitive (w, FALSE);
    
    op = bastile_ssh_operation_change_passphrase (BASTILE_SSH_KEY (object));
    bastile_operation_watch (op, (BastileDoneFunc)passphrase_done, swidget, NULL, NULL);

    /* Running operations ref themselves */
    g_object_unref (op);
}

static void
export_complete (GFile *file, GAsyncResult *result, guchar *contents)
{
	GError *err = NULL;
	gchar *uri, *unesc_uri;
	
	g_free (contents);
	
	if (!g_file_replace_contents_finish (file, result, NULL, &err)) {
		uri = g_file_get_uri (file);
		unesc_uri = g_uri_unescape_string (bastile_util_uri_get_last (uri), NULL);
        bastile_util_handle_error (err, _("Couldn't export key to \"%s\""),
                                    unesc_uri);
        g_clear_error (&err);
        g_free (uri);
        g_free (unesc_uri);
	}
}

G_MODULE_EXPORT void
on_ssh_export_button_clicked (GtkWidget *widget, BastileWidget *swidget)
{
	BastileSource *sksrc;
	BastileObject *object;
	GFile *file;
	GtkDialog *dialog;
	guchar *results;
	gsize n_results;
	gchar* uri = NULL;
	GError *err = NULL;

	object = BASTILE_OBJECT_WIDGET (swidget)->object;
	g_return_if_fail (BASTILE_IS_SSH_KEY (object));
	sksrc = bastile_object_get_source (object);
	g_return_if_fail (BASTILE_IS_SSH_SOURCE (sksrc));
	
	dialog = bastile_util_chooser_save_new (_("Export Complete Key"), 
	                                         GTK_WINDOW (bastile_widget_get_toplevel (swidget)));
	bastile_util_chooser_show_key_files (dialog);
	bastile_util_chooser_set_filename (dialog, object);

	uri = bastile_util_chooser_save_prompt (dialog);
	if (!uri) 
		return;
	
	results = bastile_ssh_source_export_private (BASTILE_SSH_SOURCE (sksrc), 
	                                              BASTILE_SSH_KEY (object),
	                                              &n_results, &err);
	
	if (results) {
		g_return_if_fail (err == NULL);
		file = g_file_new_for_uri (uri);
		g_file_replace_contents_async (file, (gchar*)results, n_results, NULL, FALSE, 
		                               G_FILE_CREATE_PRIVATE, NULL, 
		                               (GAsyncReadyCallback)export_complete, results);
	}
	
	if (err) {
	        bastile_util_handle_error (err, _("Couldn't export key."));
	        g_clear_error (&err);
	}
	
	g_free (uri);
}

static void
do_main (BastileWidget *swidget)
{
    BastileObject *object;
    BastileSSHKey *skey;
    GtkWidget *widget;
    gchar *text;
    const gchar *label;
    const gchar *template;

    object = BASTILE_OBJECT_WIDGET (swidget)->object;
    skey = BASTILE_SSH_KEY (object);

    /* Image */
    widget = GTK_WIDGET (gtk_builder_get_object (swidget->gtkbuilder, "key-image"));
    if (widget)
        gtk_image_set_from_stock (GTK_IMAGE (widget), BASTILE_STOCK_KEY_SSH, GTK_ICON_SIZE_DIALOG);

    /* Name and title */
    label = bastile_object_get_label (object);
    widget = GTK_WIDGET (gtk_builder_get_object (swidget->gtkbuilder, "comment-entry"));
    if (widget)
        gtk_entry_set_text (GTK_ENTRY (widget), label);
    widget = bastile_widget_get_toplevel (swidget);
    gtk_window_set_title (GTK_WINDOW (widget), label);

    /* Key id */
    widget = GTK_WIDGET (gtk_builder_get_object (swidget->gtkbuilder, "id-label"));
    if (widget) {
        label = bastile_object_get_identifier (object);
        gtk_label_set_text (GTK_LABEL (widget), label);
    }
    
    /* Put in message */
    widget = bastile_widget_get_widget (swidget, "trust-message");
    g_return_if_fail (widget != NULL);
    template = gtk_label_get_label (GTK_LABEL (widget));
    text = g_strdup_printf (template, g_get_user_name ());
    gtk_label_set_markup (GTK_LABEL (widget), text);
    g_free (text);
    
    /* Setup the check */
    widget = bastile_widget_get_widget (swidget, "trust-check");
    g_return_if_fail (widget != NULL);
    
    g_signal_handlers_block_by_func (widget, on_ssh_trust_toggled, swidget);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), 
                                  bastile_ssh_key_get_trust (skey) >= BASTILE_VALIDITY_FULL);
    g_signal_handlers_unblock_by_func (widget, on_ssh_trust_toggled, swidget);
}

static void 
do_details (BastileWidget *swidget)
{
    BastileObject *object;
    BastileSSHKey *skey;
    GtkWidget *widget;
    const gchar *label;
    gchar *text;

    object = BASTILE_OBJECT_WIDGET (swidget)->object;
    skey = BASTILE_SSH_KEY (object);

    widget = GTK_WIDGET (gtk_builder_get_object (swidget->gtkbuilder, "fingerprint-label"));
    if (widget) {
        text = bastile_ssh_key_get_fingerprint (skey);
        gtk_label_set_text (GTK_LABEL (widget), text);
        g_free (text);
    }

    widget = GTK_WIDGET (gtk_builder_get_object (swidget->gtkbuilder, "algo-label"));
    if (widget) {
        label = bastile_ssh_key_get_algo_str (skey);
        gtk_label_set_text (GTK_LABEL (widget), label);
    }

    widget = GTK_WIDGET (gtk_builder_get_object (swidget->gtkbuilder, "location-label"));
    if (widget) {
        label = bastile_ssh_key_get_location (skey);
        gtk_label_set_text (GTK_LABEL (widget), label);  
    }

    widget = GTK_WIDGET (gtk_builder_get_object (swidget->gtkbuilder, "strength-label"));
    if (widget) {
        text = g_strdup_printf ("%d", bastile_ssh_key_get_strength (skey));
        gtk_label_set_text (GTK_LABEL (widget), text);
        g_free (text);
    }
}

static void
key_notify (BastileObject *object, BastileWidget *swidget)
{
    do_main (swidget);
    do_details (swidget);
}

static void
properties_response (GtkDialog *dialog, int response, BastileWidget *swidget)
{
    if (response == GTK_RESPONSE_HELP) {
        bastile_widget_show_help(swidget);
        return;
    }
   
    bastile_widget_destroy (swidget);
}

void
bastile_ssh_key_properties_show (BastileSSHKey *skey, GtkWindow *parent)
{
    BastileObject *object = BASTILE_OBJECT (skey);
    BastileWidget *swidget = NULL;
    GtkWidget *widget;

    swidget = bastile_object_widget_new ("ssh-key-properties", parent, object);
    
    /* This happens if the window is already open */
    if (swidget == NULL)
        return;

    /* 
     * The signals don't need to keep getting connected. Everytime a key changes the
     * do_* functions get called. Therefore, seperate functions connect the signals
     * have been created
     */

    do_main (swidget);
    do_details (swidget);
    
    widget = bastile_widget_get_widget (swidget, "comment-entry");
    g_return_if_fail (widget != NULL);

    /* A public key only */
    if (bastile_object_get_usage (object) != BASTILE_USAGE_PRIVATE_KEY) {
        bastile_widget_set_visible (swidget, "passphrase-button", FALSE);
        bastile_widget_set_visible (swidget, "export-button", FALSE);
    }

    widget = GTK_WIDGET (bastile_widget_get_widget (swidget, swidget->name));
    g_signal_connect (widget, "response", G_CALLBACK (properties_response), swidget);
    bastile_bind_objects (NULL, skey, (BastileTransfer)key_notify, swidget);

    bastile_widget_show (swidget);
}
