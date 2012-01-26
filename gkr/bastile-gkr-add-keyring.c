/*
 * Bastile
 *
 * Copyright (C) 2008 Stefan Walter
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

#include "bastile-gkr-dialogs.h"
#include "bastile-gkr-keyring.h"
#include "bastile-gkr-source.h"

#include "bastile-widget.h"
#include "bastile-util.h"

#include <glib/gi18n.h>

/**
 * SECTION:bastile-gkr-add-keyring
 * @short_description: Contains the functions to add a new mate-keyring keyring
 *
 **/

/**
 * keyring_add_done:
 * @result:MATE_KEYRING_RESULT_CANCELLED or MATE_KEYRING_RESULT_OK
 * @data: the swidget
 *
 * This callback is called when the new keyring was created. It updates the
 * internal bastile list of keyrings
 */
static void
keyring_add_done (MateKeyringResult result, gpointer data)
{
	BastileWidget *swidget = BASTILE_WIDGET (data);
	BastileOperation *op;

	g_return_if_fail (swidget);

	/* Clear the operation without cancelling it since it is complete */
	bastile_gkr_dialog_complete_request (swidget, FALSE);
    
	/* Successful. Update the listings and stuff. */
	if (result == MATE_KEYRING_RESULT_OK) {
		
		op = bastile_source_load (BASTILE_SOURCE (bastile_gkr_source_default ()));
		
		/* 
		 * HACK: Major hack alert. This whole area needs some serious refactoring,
		 * so for now we're just going to let any viewers listen in on this
		 * operation like so:
		 */
		g_signal_emit_by_name (bastile_context_for_app (), "refreshing", op);
		g_object_unref (op);

	/* Setting the default keyring failed */
	} else if (result != MATE_KEYRING_RESULT_CANCELLED) {     
		bastile_util_show_error (bastile_widget_get_toplevel (swidget),
		                          _("Couldn't add keyring"),
		                          mate_keyring_result_to_message (result));
	}
	
	bastile_widget_destroy (swidget);
}

/**
 * on_add_keyring_name_changed:
 * @entry: the entry widget
 * @swidget: the bastile widget
 *
 * Sets the widget named "ok" in the @swidget to sensitive as soon as the
 * entry @entry contains text
 */
G_MODULE_EXPORT void
on_add_keyring_name_changed (GtkEntry *entry, BastileWidget *swidget)
{
	const gchar *keyring = gtk_entry_get_text (entry);
	bastile_widget_set_sensitive (swidget, "ok", keyring && keyring[0]);
}

/**
 * on_add_keyring_properties_response:
 * @dialog: ignored
 * @response: the response of the choose-keyring-name dialog
 * @swidget: the bastile widget
 *
 * Requests a password using #mate_keyring_create - the name of the keyring
 * was already entered by the user. The password will be requested in a window
 * provided by mate-keyring
 */
G_MODULE_EXPORT void
on_add_keyring_properties_response (GtkDialog *dialog, int response, BastileWidget *swidget)
{
	GtkEntry *entry;
	const gchar *keyring;
	gpointer request;
	
	if (response == GTK_RESPONSE_HELP) {
		bastile_widget_show_help (swidget);
		
	} else if (response == GTK_RESPONSE_ACCEPT) {
	    
		entry = GTK_ENTRY (bastile_widget_get_widget (swidget, "keyring-name"));
		g_return_if_fail (entry); 

		keyring = gtk_entry_get_text (entry);
		g_return_if_fail (keyring && keyring[0]);
	    
		request = mate_keyring_create (keyring, NULL, keyring_add_done, g_object_ref (swidget), g_object_unref);
		g_return_if_fail (request);
		bastile_gkr_dialog_begin_request (swidget, request);
		
	} else {
		bastile_widget_destroy (swidget);
	}
}

/**
 * bastile_gkr_add_keyring_show:
 * @parent: the parent widget of the new window
 *
 * Displays the window requesting the keyring name. Starts the whole
 * "create a new keyring" process.
 */
void
bastile_gkr_add_keyring_show (GtkWindow *parent)
{
	BastileWidget *swidget = NULL;
	GtkWidget *widget;
	GtkEntry *entry;

	swidget = bastile_widget_new_allow_multiple ("add-keyring", parent);
	g_return_if_fail (swidget);

	entry = GTK_ENTRY (bastile_widget_get_widget (swidget, "keyring-name"));
	g_return_if_fail (entry); 
	on_add_keyring_name_changed (entry, swidget);

	widget = bastile_widget_get_toplevel (swidget);
	
	gtk_widget_show (widget);
	gtk_window_present (GTK_WINDOW (widget));
}
