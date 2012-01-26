/*
 * Bastile
 *
 * Copyright (C) 2003 Jacob Perkins
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

#include <glib/gi18n.h>

#include "egg-datetime.h"
 
#include "bastile-object-widget.h"
#include "bastile-util.h"

#include "bastile-gpgme-dialogs.h"
#include "bastile-gpgme-key-op.h"

#define LENGTH "length"

enum {
  COMBO_STRING,
  COMBO_INT,
  N_COLUMNS
};

G_MODULE_EXPORT void
hanlder_gpgme_add_subkey_type_changed (GtkComboBox *combo, BastileWidget *swidget)
{
	gint type;
	GtkSpinButton *length;
    GtkTreeModel *model;
    GtkTreeIter iter;
    	
	length = GTK_SPIN_BUTTON (bastile_widget_get_widget (swidget, LENGTH));
	
	model = gtk_combo_box_get_model (combo);
	gtk_combo_box_get_active_iter (combo, &iter);
	gtk_tree_model_get (model, &iter,
                        COMBO_INT, &type,
                        -1);
	
	switch (type) {
		/* DSA */
		case 0:
			gtk_spin_button_set_range (length, DSA_MIN, DSA_MAX);
			gtk_spin_button_set_value (length, LENGTH_DEFAULT < DSA_MAX ? LENGTH_DEFAULT : DSA_MAX);
			break;
		/* ElGamal */
		case 1:
			gtk_spin_button_set_range (length, ELGAMAL_MIN, LENGTH_MAX);
			gtk_spin_button_set_value (length, LENGTH_DEFAULT);
			break;
		/* RSA */
		default:
			gtk_spin_button_set_range (length, RSA_MIN, LENGTH_MAX);
			gtk_spin_button_set_value (length, LENGTH_DEFAULT);
			break;
	}
}

G_MODULE_EXPORT void
on_gpgme_add_subkey_never_expires_toggled (GtkToggleButton *togglebutton, BastileWidget *swidget)
{
    GtkWidget *widget;

    widget = GTK_WIDGET (g_object_get_data (G_OBJECT (swidget), "expires-datetime"));
    g_return_if_fail (widget);

    gtk_widget_set_sensitive (GTK_WIDGET (widget),
                              !gtk_toggle_button_get_active (togglebutton));
}

G_MODULE_EXPORT void
on_gpgme_add_subkey_ok_clicked (GtkButton *button, BastileWidget *swidget)
{
	BastileObjectWidget *skwidget;
	BastileKeyEncType real_type;
	gint type;
	guint length;
	time_t expires;
	gpgme_error_t err;
	GtkWidget *widget;
	GtkComboBox *combo;
	GtkTreeModel *model;
    GtkTreeIter iter;
	
	skwidget = BASTILE_OBJECT_WIDGET (swidget);
	
	combo = GTK_COMBO_BOX (bastile_widget_get_widget (swidget, "type"));
	gtk_combo_box_get_active_iter (combo, &iter);
	model = gtk_combo_box_get_model (combo);
	gtk_tree_model_get (model, &iter,
                        COMBO_INT, &type,
                        -1);	
		
	length = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (
		bastile_widget_get_widget (swidget, LENGTH)));
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (
	bastile_widget_get_widget (swidget, "never_expires"))))
		expires = 0;
	else {
        widget = GTK_WIDGET (g_object_get_data (G_OBJECT (swidget), "expires-datetime"));
        g_return_if_fail (widget);

        egg_datetime_get_as_time_t (EGG_DATETIME (widget), &expires);
   }
	
	switch (type) {
		case 0:
			real_type = DSA;
			break;
		case 1:
			real_type = ELGAMAL;
			break;
		case 2:
			real_type = RSA_SIGN;
			break;
		default:
			real_type = RSA_ENCRYPT;
			break;
	}
	
	widget = GTK_WIDGET (bastile_widget_get_widget (swidget, swidget->name));
	gtk_widget_set_sensitive (widget, FALSE);
	err = bastile_gpgme_key_op_add_subkey (BASTILE_GPGME_KEY (skwidget->object), 
	                                        real_type, length, expires);
	gtk_widget_set_sensitive (widget, TRUE);
	
	if (!GPG_IS_OK (err))
		bastile_gpgme_handle_error (err, _("Couldn't add subkey"));

	bastile_widget_destroy (swidget);
}

void
bastile_gpgme_add_subkey_new (BastileGpgmeKey *pkey, GtkWindow *parent)
{
	BastileWidget *swidget;
	GtkComboBox* combo;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkCellRenderer *renderer;
	GtkWidget *widget, *datetime;
	
	swidget = bastile_object_widget_new ("add-subkey", parent, BASTILE_OBJECT (pkey));
	g_return_if_fail (swidget != NULL);
	
	gtk_window_set_title (GTK_WINDOW (bastile_widget_get_widget (swidget, swidget->name)),
		g_strdup_printf (_("Add subkey to %s"), bastile_object_get_label (BASTILE_OBJECT (pkey))));
    
    combo = GTK_COMBO_BOX (bastile_widget_get_widget (swidget, "type"));
    model = GTK_TREE_MODEL (gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_INT));
    
    gtk_combo_box_set_model (combo, model);
        
    gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo));
    renderer = gtk_cell_renderer_text_new ();
    
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
    gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), renderer,
                                    "text", COMBO_STRING);
                                    
    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                        COMBO_STRING, _("DSA (sign only)"),
                        COMBO_INT, 0,
                        -1);
                        
    gtk_combo_box_set_active_iter (combo, &iter);
    
    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                        COMBO_STRING, _("ElGamal (encrypt only)"),
                        COMBO_INT, 1,
                        -1);
                        
	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                        COMBO_STRING, _("RSA (sign only)"),
                        COMBO_INT, 2,
                        -1);
    
	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                        COMBO_STRING, _("RSA (encrypt only)"),
                        COMBO_INT, 3,
                        -1);
    
	widget = bastile_widget_get_widget (swidget, "datetime-placeholder");
	g_return_if_fail (widget != NULL);

	datetime = egg_datetime_new ();
	gtk_container_add (GTK_CONTAINER (widget), datetime);
	gtk_widget_show (datetime);
	gtk_widget_set_sensitive (datetime, FALSE);
	g_object_set_data (G_OBJECT (swidget), "expires-datetime", datetime);
}
