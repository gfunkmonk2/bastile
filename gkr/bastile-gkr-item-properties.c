/*
 * Bastile
 *
 * Copyright (C) 2006 Stefan Walter
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
#include <config.h>

#include <glib/gi18n.h>

#include "bastile-gkr-item.h"
#include "bastile-gkr-source.h"
#include "bastile-gkr-operation.h"

#include "bastile-gtkstock.h"
#include "bastile-object.h"
#include "bastile-object-widget.h"
#include "bastile-secure-memory.h"
#include "bastile-util.h"
#include "bastile-widget.h"

#include "common/bastile-bind.h"
#include "common/bastile-secure-buffer.h"

GType
boxed_access_control_type (void)
{
	static GType type = 0;
	if (!type)
		type = g_boxed_type_register_static ("MateKeyringAccessControl", 
		                                     (GBoxedCopyFunc)mate_keyring_access_control_copy,
		                                     (GBoxedFreeFunc)mate_keyring_access_control_free);
	return type;
}

/* -----------------------------------------------------------------------------
 * MAIN TAB 
 */

static gboolean
transform_item_use (const GValue *from, GValue *to)
{
	gchar *label;
	guint val;
	
	g_return_val_if_fail (G_VALUE_TYPE (from) == G_TYPE_UINT, FALSE);
	g_return_val_if_fail (G_VALUE_TYPE (to) == G_TYPE_STRING, FALSE);
	
	val = g_value_get_uint (from);
	switch (val) {
	case BASTILE_GKR_USE_NETWORK:
		label = _("Access a network share or resource");
		break;
	case BASTILE_GKR_USE_WEB:
		label = _("Access a website");
		break;
	case BASTILE_GKR_USE_PGP:
		label = _("Unlocks a PGP key");
		break;
	case BASTILE_GKR_USE_SSH:
		label = _("Unlocks a Secure Shell key");
		break;
	case BASTILE_GKR_USE_OTHER:
		label = _("Saved password or login");
		break;
	default:
		label = "";
	        g_return_val_if_reached (FALSE);
	};
	
	g_value_set_string (to, label);
	return TRUE;
}

static gboolean
transform_item_type (const GValue *from, GValue *to)
{
	gchar *label;
	guint val;
	
	g_return_val_if_fail (G_VALUE_TYPE (from) == G_TYPE_UINT, FALSE);
	g_return_val_if_fail (G_VALUE_TYPE (to) == G_TYPE_STRING, FALSE);
	
	val = g_value_get_uint (from);
	switch (val) {
	case BASTILE_GKR_USE_NETWORK:
	case BASTILE_GKR_USE_WEB:
		label = _("Network Credentials");
		break;
	case BASTILE_GKR_USE_PGP:
	case BASTILE_GKR_USE_SSH:
	case BASTILE_GKR_USE_OTHER:
		label = _("Password");
		break;
	default:
		label = "";
	        g_return_val_if_reached (FALSE);
	};
	
	g_value_set_string (to, label);
	return TRUE;
}

static gboolean
transform_network_visible (const GValue *from, GValue *to)
{
	gboolean result;
	guint val;
	
	g_return_val_if_fail (G_VALUE_TYPE (from) == G_TYPE_UINT, FALSE);
	g_return_val_if_fail (G_VALUE_TYPE (to) == G_TYPE_BOOLEAN, FALSE);
	
	val = g_value_get_uint (from);
	switch (val) {
	case BASTILE_GKR_USE_NETWORK:
	case BASTILE_GKR_USE_WEB:
		result = TRUE;
		break;
	case BASTILE_GKR_USE_PGP:
	case BASTILE_GKR_USE_SSH:
	case BASTILE_GKR_USE_OTHER:
		result = FALSE;
		break;
	default:
	        g_return_val_if_reached (FALSE);
	};
	
	g_value_set_boolean (to, result);
	return TRUE;
}

static gboolean
transform_attributes_server (const GValue *from, GValue *to)
{
	MateKeyringAttributeList *attrs;
	attrs = g_value_get_boxed (from);
	if (attrs)
		g_value_set_string (to, bastile_gkr_find_string_attribute (attrs, "server"));
	if (!g_value_get_string (to))
		g_value_set_string (to, "");
	return TRUE;
}

static gboolean
transform_attributes_user (const GValue *from, GValue *to)
{
	MateKeyringAttributeList *attrs;
	attrs = g_value_get_boxed (from);
	if (attrs)
		g_value_set_string (to, bastile_gkr_find_string_attribute (attrs, "user"));
	if (!g_value_get_string (to))
		g_value_set_string (to, "");
	return TRUE;
}

static void
transfer_password (BastileGkrItem *git, BastileWidget *swidget)
{
	GtkWidget *expander;
	GtkEntry *entry;
	const gchar *secret;

	expander = bastile_widget_get_widget (swidget, "password-expander");
	g_return_if_fail (expander);

	entry = g_object_get_data (G_OBJECT (swidget), "secure-password-entry");
	g_return_if_fail (entry);

	if (gtk_expander_get_expanded (GTK_EXPANDER (expander))) {
		secret = bastile_gkr_item_get_secret (git);
		gtk_entry_set_text (entry, secret ? secret : "");
	} else {
		gtk_entry_set_text (entry, "");
	}
	g_object_set_data (G_OBJECT (entry), "changed", NULL);
}

static void
password_activate (GtkEntry *entry, BastileWidget *swidget)
{
	BastileObject *object;
	BastileGkrItem *git;
	BastileOperation *op;
	MateKeyringItemInfo *info;
	GtkWidget *expander;
    
	object = BASTILE_OBJECT_WIDGET (swidget)->object;
	if (!object)
		return;

	git = BASTILE_GKR_ITEM (object);

	expander = bastile_widget_get_widget (swidget, "password-expander");
	g_return_if_fail (expander);
	if (!gtk_expander_get_expanded (GTK_EXPANDER (expander)))
		return;

	entry = g_object_get_data (G_OBJECT (swidget), "secure-password-entry");
	if (!g_object_get_data (G_OBJECT (entry), "changed"))
		return;

	if (g_object_get_data (G_OBJECT (swidget), "updating-password"))
		return;
	g_object_set_data (G_OBJECT (swidget), "updating-password", "updating");

	g_object_ref (git);
	g_object_ref (entry);
	gtk_widget_set_sensitive (expander, FALSE);
    	
	/* Make sure we've loaded the information */
	bastile_util_wait_until (bastile_gkr_item_get_info (git));
    
	info = mate_keyring_item_info_copy (bastile_gkr_item_get_info (git));
	mate_keyring_item_info_set_secret (info, gtk_entry_get_text (entry));

	op = bastile_gkr_operation_update_info (git, info);
	mate_keyring_item_info_free (info);
    
	/* This is usually a quick operation */
	bastile_operation_wait (op);
	
	/* Set the password back if failed */
	if (!bastile_operation_is_successful (op))
		transfer_password (git, swidget);

	gtk_widget_set_sensitive (expander, TRUE);
	g_object_unref (entry);
	g_object_unref (git);
    
	if (!bastile_operation_is_successful (op))
		bastile_operation_display_error (op, _("Couldn't change password."),
		                                  bastile_widget_get_toplevel (swidget));
	
	g_object_unref (op);
	g_object_set_data (G_OBJECT (swidget), "updating-description", NULL);

}

static void
password_changed (GtkEditable *editable, BastileWidget *swidget)
{
	g_object_set_data (G_OBJECT (editable), "changed", "changed");
}

static gboolean
password_focus_out (GtkEntry* entry, GdkEventFocus *event, BastileWidget *swidget)
{
    password_activate (entry, swidget);
    return FALSE;
}

G_MODULE_EXPORT void 
on_item_show_password_toggled (GtkToggleButton *button, BastileWidget *swidget)
{
	GtkWidget *widget = g_object_get_data (G_OBJECT (swidget), "secure-password-entry");
	gtk_entry_set_visibility (GTK_ENTRY (widget), gtk_toggle_button_get_active (button));
}

G_MODULE_EXPORT void
on_item_password_expander_activate (GtkExpander *expander, BastileWidget *swidget)
{
    BastileObject *object;
    BastileGkrItem *git;
    GtkWidget *widget;

    object = BASTILE_OBJECT_WIDGET (swidget)->object;
    if (!object)
	    return;
    git = BASTILE_GKR_ITEM (object);

    if (!gtk_expander_get_expanded (expander))
        return;

    /* Always have a hidden password when opening box */
    widget = bastile_widget_get_widget (swidget, "show-password-check");
    g_return_if_fail (widget != NULL);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), FALSE);
    
    /* Make sure to trigger retrieving the secret */
    transfer_password (git, swidget);
}

G_MODULE_EXPORT void
on_item_description_activate (GtkWidget *entry, BastileWidget *swidget)
{
	BastileObject *object;
	BastileGkrItem *git;
	BastileOperation *op = NULL;
	MateKeyringItemInfo *info;
	const gchar *text;
	const gchar *original;
    
	object = BASTILE_OBJECT_WIDGET (swidget)->object;
	if (!object)
		return;

	if (g_object_get_data (G_OBJECT (swidget), "updating-description"))
		return;
	g_object_set_data (G_OBJECT (swidget), "updating-description", "updating");

	git = BASTILE_GKR_ITEM (object);
	
	g_object_ref (git);
	g_object_ref (entry);
	gtk_widget_set_sensitive (entry, FALSE);

	/* Make sure we've loaded the information */
	bastile_util_wait_until (bastile_gkr_item_get_info (git));
	info = bastile_gkr_item_get_info (git);
    
	/* Make sure not the same */
	text = gtk_entry_get_text (GTK_ENTRY (entry));
	original = bastile_object_get_label (object);
	if (text != original && g_utf8_collate (text, original ? original : "") != 0) {
		
		info = mate_keyring_item_info_copy (info);
		mate_keyring_item_info_set_display_name (info, text);
		
		op = bastile_gkr_operation_update_info (git, info);
		mate_keyring_item_info_free (info);

		/* This is usually a quick operation */
		bastile_operation_wait (op);

		if (!bastile_operation_is_successful (op)) 
			gtk_entry_set_text (GTK_ENTRY (entry), original);

	}

	gtk_widget_set_sensitive (entry, TRUE);
	g_object_unref (entry);
	g_object_unref (git);
    
	if (op) {
		if (!bastile_operation_is_successful (op)) 
			bastile_operation_display_error (op, _("Couldn't set description."),
			                                  bastile_widget_get_toplevel (swidget));
		g_object_unref (op);
	}
	
	g_object_set_data (G_OBJECT (swidget), "updating-description", NULL);
}

G_MODULE_EXPORT gboolean
on_item_description_focus_out (GtkWidget* widget, GdkEventFocus *event, BastileWidget *swidget)
{
	on_item_description_activate (widget, swidget);
	return FALSE;
}


static void
setup_main (BastileWidget *swidget)
{
	BastileObject *object;
	GtkEntryBuffer *buffer;
	GtkWidget *widget;
	GtkWidget *box;

	object = BASTILE_OBJECT_WIDGET (swidget)->object;

	/* Setup the image properly */
	bastile_bind_property ("icon", object, "stock", 
	                        bastile_widget_get_widget (swidget, "key-image"));
	
	/* Setup the label properly */
	bastile_bind_property ("label", object, "text", 
	                        bastile_widget_get_widget (swidget, "description-field") );
	
	/* Window title */
	bastile_bind_property ("label", object, "title", 
	                        bastile_widget_get_toplevel (swidget));
	
	/* Usage */
	bastile_bind_property_full ("use", object, transform_item_use, "label", 
	                             bastile_widget_get_widget (swidget, "use-field"), NULL);
	
	/* Item Type */
	bastile_bind_property_full ("use", object, transform_item_type, "label", 
	                             bastile_widget_get_widget (swidget, "type-field"), NULL);
	
	/* Network field visibility */
	bastile_bind_property_full ("use", object, transform_network_visible, "visible",
	                             bastile_widget_get_widget (swidget, "server-label"),
	                             bastile_widget_get_widget (swidget, "server-field"),
	                             bastile_widget_get_widget (swidget, "login-label"),
	                             bastile_widget_get_widget (swidget, "login-field"), NULL);

	/* Server name */
	bastile_bind_property_full ("item-attributes", object, transform_attributes_server, "label", 
	                             bastile_widget_get_widget (swidget, "server-field"), NULL);
	
	/* User name */
	bastile_bind_property_full ("item-attributes", object, transform_attributes_user, "label", 
	                             bastile_widget_get_widget (swidget, "login-field"), NULL);

	/* Create the password entry */
	buffer = bastile_secure_buffer_new ();
	widget = gtk_entry_new_with_buffer (buffer);
	g_object_unref (buffer);

	box = bastile_widget_get_widget (swidget, "password-box-area");
	g_return_if_fail (box != NULL);
	gtk_container_add (GTK_CONTAINER (box), widget);
	g_object_set_data (G_OBJECT (swidget), "secure-password-entry", widget);
	gtk_widget_show (widget);
	        
	/* Now watch for changes in the password */
	g_signal_connect (widget, "activate", G_CALLBACK (password_activate), swidget);
	g_signal_connect (widget, "changed", G_CALLBACK (password_changed), swidget);
	g_signal_connect_after (widget, "focus-out-event", G_CALLBACK (password_focus_out), swidget);

	/* Sensitivity of the password entry */
	bastile_bind_property ("has-secret", object, "sensitive", widget);

	/* Updating of the password entry */
	bastile_bind_objects ("has-secret", object, (BastileTransfer)transfer_password, swidget);

	widget = bastile_widget_get_widget (swidget, "show-password-check");
	on_item_show_password_toggled (GTK_TOGGLE_BUTTON (widget), swidget);
}

/* -----------------------------------------------------------------------------
 * DETAILS TAB
 */

static gboolean
transform_item_details (const GValue *from, GValue *to)
{
	MateKeyringAttributeList *attrs;
	MateKeyringAttribute *attr;
	GString *details;
	gchar *text;
	guint i;

	g_return_val_if_fail (G_VALUE_TYPE (to) == G_TYPE_STRING, FALSE);
	attrs = g_value_get_boxed (from);
	
	details = g_string_new (NULL);
	if (attrs) {
		/* Build up the display string */
		for(i = 0; i < attrs->len; ++i) {
			attr = &(mate_keyring_attribute_list_index (attrs, i));
			text = g_markup_escape_text (attr->name, -1);
			g_string_append_printf (details, "<b>%s</b>: ", text);
			g_free (text);
			switch (attr->type) {
			case MATE_KEYRING_ATTRIBUTE_TYPE_STRING:
				text = g_markup_escape_text (attr->value.string, -1);
				g_string_append_printf (details, "%s\n", text);
				g_free (text);
				break;
			case MATE_KEYRING_ATTRIBUTE_TYPE_UINT32:
				g_string_append_printf (details, "%u\n", attr->value.integer);
				break;
			default:
				g_string_append (details, "<i>[invalid]</i>\n");
				break;
			}
		}
	}

	g_value_take_string (to, g_string_free (details, FALSE));
	return TRUE;
}

static void 
setup_details (BastileWidget *swidget)
{
	BastileObject *object;
	GtkWidget *widget;

	object = BASTILE_OBJECT_WIDGET (swidget)->object;
    
	widget = bastile_widget_get_widget (swidget, "details-box");
	g_return_if_fail (widget != NULL);
	gtk_label_set_use_markup (GTK_LABEL (widget), TRUE);
    
	bastile_bind_property_full ("item-attributes", object, transform_item_details, 
	                             "label", widget, NULL);
}

/* -----------------------------------------------------------------------------
 * APPLICATIONS TAB 
 */

enum {
	APPS_ACCESS,
	APPS_NAME,
	APPS_N_COLUMNS
};

static void
update_application_details (BastileWidget *swidget)
{
	MateKeyringAccessControl *ac;
	GtkLabel *label;
	GtkToggleButton *toggle;
	MateKeyringAccessType access;
	GtkTreeView *tree;
	GtkTreePath *path;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	gchar *filename;
    
	tree = GTK_TREE_VIEW (bastile_widget_get_widget (swidget, "application-list"));
	g_return_if_fail (GTK_IS_TREE_VIEW (tree));
	    
	selection = gtk_tree_view_get_selection (tree);
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;
	        
	path = gtk_tree_model_get_path (model, &iter);
	g_return_if_fail (path);
	
	/* Dig out the current value */
	gtk_tree_model_get (model, &iter, APPS_ACCESS, &ac, -1);
	
	bastile_widget_set_sensitive (swidget, "application-details", ac != NULL);
    
	label = GTK_LABEL (bastile_widget_get_widget (swidget, "application-path"));
	g_return_if_fail (GTK_IS_LABEL (label));
	filename = ac ? mate_keyring_item_ac_get_path_name (ac) : NULL;
	gtk_label_set_text (label, filename ? filename : "");
	g_free (filename);

	g_object_set_data (G_OBJECT (swidget), "updating", "updating");
	access = ac ? mate_keyring_item_ac_get_access_type (ac) : 0;
    
	toggle = GTK_TOGGLE_BUTTON (bastile_widget_get_widget (swidget, "application-read"));
	g_return_if_fail (GTK_IS_TOGGLE_BUTTON (toggle));
	gtk_toggle_button_set_active (toggle, access & MATE_KEYRING_ACCESS_READ);    

	toggle = GTK_TOGGLE_BUTTON (bastile_widget_get_widget (swidget, "application-write"));
	g_return_if_fail (GTK_IS_TOGGLE_BUTTON (toggle));    
	gtk_toggle_button_set_active (toggle, access & MATE_KEYRING_ACCESS_WRITE);    

	toggle = GTK_TOGGLE_BUTTON (bastile_widget_get_widget (swidget, "application-delete"));
	g_return_if_fail (GTK_IS_TOGGLE_BUTTON (toggle));
	gtk_toggle_button_set_active (toggle, access & MATE_KEYRING_ACCESS_REMOVE);
    
	g_object_set_data (G_OBJECT (swidget), "updating", NULL);
	mate_keyring_access_control_free (ac);
}

static void
application_selection_changed (GtkTreeSelection *selection, BastileWidget *swidget)
{
	update_application_details (swidget);
}

static void 
merge_toggle_button_access (BastileWidget *swidget, const gchar *identifier, 
                            MateKeyringAccessType *access, MateKeyringAccessType type)
{
	GtkToggleButton *toggle;
    
	toggle = GTK_TOGGLE_BUTTON (bastile_widget_get_widget (swidget, identifier));
	g_return_if_fail (GTK_IS_TOGGLE_BUTTON (toggle));
	if (gtk_toggle_button_get_active (toggle))
		*access |= type;
	else
		*access &= ~type;
}

G_MODULE_EXPORT void
on_item_application_access_toggled (GtkCheckButton *check, BastileWidget *swidget)
{
	BastileObject *object;
	BastileGkrItem *git;
	BastileOperation *op;
	MateKeyringAccessType access;
	MateKeyringAccessControl *ac;
	GtkTreeView *tree;
	GtkTreePath *path;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	GError *err = NULL;
	GList *acl;
    
	/* Just us setting up the controls, not the user */
	if (g_object_get_data (G_OBJECT (swidget), "updating"))
		return;
    
	object = BASTILE_OBJECT_WIDGET (swidget)->object;
	git = BASTILE_GKR_ITEM (object);

	tree = GTK_TREE_VIEW (bastile_widget_get_widget (swidget, "application-list"));
	g_return_if_fail (GTK_IS_TREE_VIEW (tree));
	    
	selection = gtk_tree_view_get_selection (tree);
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		g_return_if_reached ();
	        
	path = gtk_tree_model_get_path (model, &iter);
	g_return_if_fail (path);
	
	/* Update the access control on that one */
	gtk_tree_model_get (model, &iter, APPS_ACCESS, &ac, -1);
	access = mate_keyring_item_ac_get_access_type (ac);
	merge_toggle_button_access (swidget, "application-read", &access, MATE_KEYRING_ACCESS_READ);    
	merge_toggle_button_access (swidget, "application-write", &access, MATE_KEYRING_ACCESS_WRITE);    
	merge_toggle_button_access (swidget, "application-delete", &access, MATE_KEYRING_ACCESS_REMOVE);
	
	/* If it's changed */
	if (access != mate_keyring_item_ac_get_access_type (ac)) {
		
		/* Update the store with this new stuff */
	        mate_keyring_item_ac_set_access_type (ac, access);
	        gtk_list_store_set (GTK_LIST_STORE (model), &iter, APPS_ACCESS, ac, -1);
	        mate_keyring_access_control_free (ac);
		
	        /* Build up a full ACL from what we have */
	        acl = NULL;
	        if (!gtk_tree_model_get_iter_first (model, &iter))
	        	g_return_if_reached ();
        	do {
        		gtk_tree_model_get (model, &iter, APPS_ACCESS, &ac, -1);
        		acl = g_list_append (acl, ac);
        	} while (gtk_tree_model_iter_next (model, &iter));

        	bastile_widget_set_sensitive (swidget, "application-details", FALSE);
        	g_object_ref (git);
        
        	op = bastile_gkr_operation_update_acl (git, acl);
        	g_return_if_fail (op);
        
        	bastile_operation_wait (op);
        	
        	mate_keyring_acl_free (acl);
        	
        	if (!bastile_operation_is_successful (op))
        		update_application_details (swidget);

                bastile_widget_set_sensitive (swidget, "application-details", TRUE);
                g_object_unref (git);

        	if (!bastile_operation_is_successful (op)) {
        		bastile_operation_copy_error (op, &err);
        		bastile_util_handle_error (err, _("Couldn't set application access."));
        		g_clear_error (&err);
        	}
        	
        	g_object_unref (op);
	}
}

static void 
update_application (BastileGkrItem *git, BastileWidget *swidget)
{
	GtkTreeView *tree;
	GtkListStore *store;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkCellRenderer *renderer;
	MateKeyringAccessControl *ac;
	GtkTreeViewColumn *column;
	GList *acl;
	gboolean valid;
	gchar *display;

	tree = GTK_TREE_VIEW (bastile_widget_get_widget (swidget, "application-list"));
	g_return_if_fail (tree);
    
	model = gtk_tree_view_get_model (tree);
	if (!model) {
		g_assert (2 == APPS_N_COLUMNS);
		store = gtk_list_store_new (2, boxed_access_control_type (), G_TYPE_STRING);
		model = GTK_TREE_MODEL (store);
		gtk_tree_view_set_model (tree, model);

		renderer = gtk_cell_renderer_text_new ();
		column = gtk_tree_view_column_new_with_attributes ("name", renderer, "text", APPS_NAME, NULL);
		gtk_tree_view_append_column (tree, column);
	} else {
		store = GTK_LIST_STORE (model);
	}
    
	acl = bastile_gkr_item_get_acl (git);
    
	/* Fill in the tree store, replacing any rows already present */
	valid = gtk_tree_model_get_iter_first (model, &iter);
	for ( ; acl; acl = g_list_next (acl)) {
    
		ac = (MateKeyringAccessControl*)acl->data;
		g_return_if_fail (ac);
        
		if (!valid)
			gtk_list_store_append (store, &iter);

		display = mate_keyring_item_ac_get_display_name (ac);
		gtk_list_store_set (store, &iter, 
		                    APPS_NAME, display ? display : "", 
		                    APPS_ACCESS, ac,
		                    -1);
		g_free (display);
        
		if (valid)
			valid = gtk_tree_model_iter_next (model, &iter);
	}
    
	/* Remove all the remaining rows */
	while (valid)
		valid = gtk_list_store_remove (store, &iter);
        
	update_application_details (swidget);
}

static void 
setup_application (BastileWidget *swidget)
{
	bastile_bind_objects ("item-acl", BASTILE_OBJECT_WIDGET (swidget)->object,
	                       (BastileTransfer)update_application, swidget);
}

/* -----------------------------------------------------------------------------
 * GENERAL
 */

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
bastile_gkr_item_properties_show (BastileGkrItem *git, GtkWindow *parent)
{
    BastileObject *object = BASTILE_OBJECT (git);
    BastileWidget *swidget = NULL;
    GtkWidget *widget;

    swidget = bastile_object_widget_new ("gkr-item-properties", parent, object);
    
    /* This happens if the window is already open */
    if (swidget == NULL)
        return;

    bastile_object_refresh (object);

    widget = GTK_WIDGET (bastile_widget_get_widget (swidget, swidget->name));
    g_signal_connect (widget, "response", G_CALLBACK (properties_response), swidget);

    /* 
     * The signals don't need to keep getting connected. Everytime a key changes the
     * do_* functions get called. Therefore, seperate functions connect the signals
     * have been created
     */

    setup_main (swidget);
    setup_details (swidget);
    setup_application (swidget);
    
    widget = bastile_widget_get_widget (swidget, "application-list");
    g_return_if_fail (GTK_IS_TREE_VIEW (widget));
    
    g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (widget)), "changed", 
                      G_CALLBACK (application_selection_changed), swidget);
}
