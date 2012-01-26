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

#include "common/bastile-secure-buffer.h"

#include <glib/gi18n.h>

static void
item_add_done (MateKeyringResult result, guint32 item, gpointer data)
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

G_MODULE_EXPORT void
on_add_item_label_changed (GtkEntry *entry, BastileWidget *swidget)
{
	const gchar *keyring = gtk_entry_get_text (entry);
	bastile_widget_set_sensitive (swidget, "ok", keyring && keyring[0]);
}

G_MODULE_EXPORT void 
on_add_item_password_toggled (GtkToggleButton *button, BastileWidget *swidget)
{
	GtkWidget *widget= g_object_get_data (G_OBJECT (swidget), "gkr-secure-entry");
	gtk_entry_set_visibility (GTK_ENTRY (widget), gtk_toggle_button_get_active (button));
}

G_MODULE_EXPORT void
on_add_item_response (GtkDialog *dialog, int response, BastileWidget *swidget)
{
	GtkWidget *widget;
	gchar *keyring;
	const gchar *secret;
	const gchar *label;
	gpointer request;
	GArray *attributes;
	
	if (response == GTK_RESPONSE_HELP) {
		bastile_widget_show_help (swidget);
		
	} else if (response == GTK_RESPONSE_ACCEPT) {
	    
		widget = bastile_widget_get_widget (swidget, "item-label");
		label = gtk_entry_get_text (GTK_ENTRY (widget));
		g_return_if_fail (label && label[0]);
		
		widget = g_object_get_data (G_OBJECT (swidget), "gkr-secure-entry");
		secret = gtk_entry_get_text (GTK_ENTRY (widget));
		
		widget = bastile_widget_get_widget (swidget, "item-keyring");
#if GTK_CHECK_VERSION (2,91,2)
		keyring = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (widget));
#else
		keyring = gtk_combo_box_get_active_text (GTK_COMBO_BOX (widget));
#endif

		attributes = mate_keyring_attribute_list_new ();
	    
		request = mate_keyring_item_create (keyring, MATE_KEYRING_ITEM_NOTE, label, 
		                                     attributes, secret, FALSE, item_add_done, 
		                                     g_object_ref (swidget), g_object_unref);
		g_return_if_fail (request);
		bastile_gkr_dialog_begin_request (swidget, request);
		
		g_free (keyring);
		mate_keyring_attribute_list_free (attributes);
		
	} else {
		bastile_widget_destroy (swidget);
	}
}

void
bastile_gkr_add_item_show (GtkWindow *parent)
{
	BastileWidget *swidget = NULL;
	GtkEntryBuffer *buffer;
	GtkWidget *entry, *widget;
	GList *keyrings, *l;
	GtkCellRenderer *cell;
	GtkListStore *store;
	GtkTreeIter iter;
	gint i;
	
	swidget = bastile_widget_new_allow_multiple ("gkr-add-item", parent);
	g_return_if_fail (swidget);
	
	/* Load up a list of all the keyrings, and select the default */
	widget = bastile_widget_get_widget (swidget, "item-keyring");
	store = gtk_list_store_new (1, G_TYPE_STRING);
	gtk_combo_box_set_model (GTK_COMBO_BOX (widget), GTK_TREE_MODEL (store));
	cell = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget), cell, TRUE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (widget), cell, "text", 0);
	                                    
	keyrings = bastile_context_find_objects (NULL, BASTILE_GKR, BASTILE_USAGE_OTHER, 
	                                          BASTILE_LOCATION_LOCAL);
	for (i = 0, l = keyrings; l; l = g_list_next (l)) {
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, bastile_gkr_keyring_get_name (l->data), -1);
		if (bastile_gkr_keyring_get_is_default (l->data))
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX (widget), &iter);
	}
	
	g_object_unref (store);

	widget = bastile_widget_get_widget (swidget, "item-label");
	g_return_if_fail (widget); 
	on_add_item_label_changed (GTK_ENTRY (widget), swidget);
	
	widget = bastile_widget_get_widget (swidget, "password-area");
	g_return_if_fail (widget);
	buffer = bastile_secure_buffer_new ();
	entry = gtk_entry_new_with_buffer (buffer);
	g_object_unref (buffer);
	gtk_container_add (GTK_CONTAINER (widget), GTK_WIDGET (entry));
	gtk_widget_show (GTK_WIDGET (entry));
	g_object_set_data (G_OBJECT (swidget), "gkr-secure-entry", entry);

	widget = bastile_widget_get_widget (swidget, "show-password");
	on_add_item_password_toggled (GTK_TOGGLE_BUTTON (widget), swidget);

	widget = bastile_widget_get_toplevel (swidget);
	gtk_widget_show (widget);
	gtk_window_present (GTK_WINDOW (widget));
}
