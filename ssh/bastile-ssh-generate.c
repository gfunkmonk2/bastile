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

#include "config.h"

#include <string.h>
 
#include <glib/gi18n.h>

#include "bastile-ssh-dialogs.h"
#include "bastile-ssh-source.h"
#include "bastile-ssh-key.h"
#include "bastile-ssh.h"
#include "bastile-ssh-operation.h"

#include "bastile-widget.h"
#include "bastile-util.h"
#include "bastile-progress.h"
#include "bastile-gtkstock.h"

#include "common/bastile-registry.h"

/* --------------------------------------------------------------------------
 * ACTIONS
 */

static void
on_ssh_generate_key (GtkAction *action, gpointer unused)
{
	BastileSource* sksrc;
	
	g_return_if_fail (GTK_IS_ACTION (action));
	
	sksrc = bastile_context_find_source (bastile_context_for_app (), BASTILE_SSH_TYPE, BASTILE_LOCATION_LOCAL);
	g_return_if_fail (sksrc != NULL);
	
	bastile_ssh_generate_show (BASTILE_SSH_SOURCE (sksrc), NULL);
}

static const GtkActionEntry ACTION_ENTRIES[] = {
	{ "ssh-generate-key", BASTILE_SSH_STOCK_ICON, N_ ("Secure Shell Key"), "", 
	  N_("Used to access other computers (eg: via a terminal)"), G_CALLBACK (on_ssh_generate_key) }
};

void
bastile_ssh_generate_register (void)
{
	GtkActionGroup *actions;
	
	actions = gtk_action_group_new ("ssh-generate");

	gtk_action_group_set_translation_domain (actions, GETTEXT_PACKAGE);
	gtk_action_group_add_actions (actions, ACTION_ENTRIES, G_N_ELEMENTS (ACTION_ENTRIES), NULL);
	
	/* Register this as a generator */
	bastile_registry_register_object (NULL, G_OBJECT (actions), BASTILE_SSH_TYPE_STR, "generator", NULL);
}

/* --------------------------------------------------------------------
 * DIALOGS
 */

#define DSA_SIZE 1024
#define DEFAULT_RSA_SIZE 2048

static void
completion_handler (BastileOperation *op, gpointer data)
{
    GError *error = NULL;
    if (!bastile_operation_is_successful (op)) {
        bastile_operation_copy_error (op, &error);
        bastile_util_handle_error (error, _("Couldn't generate Secure Shell key"));
        g_clear_error (&error);
    }
}

static void
upload_handler (BastileOperation *op, BastileWidget *swidget)
{
    BastileSSHKey *skey;
    GList *keys;
    
    if (!bastile_operation_is_successful (op) ||
        bastile_operation_is_cancelled (op)) {
		bastile_widget_destroy (swidget);
        return;
	}
    
    skey = BASTILE_SSH_KEY (bastile_operation_get_result (op));
	if (!BASTILE_IS_SSH_KEY (skey)) {
		bastile_widget_destroy (swidget);
		return;
	}
	
    keys = g_list_append (NULL, skey);
    bastile_ssh_upload_prompt (keys, GTK_WINDOW (bastile_widget_get_widget (swidget, swidget->name)));
    g_list_free (keys);
	bastile_widget_destroy (swidget);
}

static void
on_change (GtkComboBox *combo, BastileWidget *swidget)
{
    const gchar *t;    
    GtkWidget *widget;
    
    widget = bastile_widget_get_widget (swidget, "bits-entry");
    g_return_if_fail (widget != NULL);
    
#if GTK_CHECK_VERSION (2,91,2)
    t = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (combo));
#else
    t = gtk_combo_box_get_active_text (combo);
#endif
    if (t && strstr (t, "DSA")) {
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), DSA_SIZE);
        gtk_widget_set_sensitive (widget, FALSE);
    } else {
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), DEFAULT_RSA_SIZE);
        gtk_widget_set_sensitive (widget, TRUE);
    }
}

static void
on_response (GtkDialog *dialog, guint response, BastileWidget *swidget)
{
    BastileSSHSource *src;
    BastileOperation *op;
    GtkWidget *widget;
    const gchar *email;
    const gchar *t;
    gboolean upload;
    guint type;
    guint bits;
    
    if (response == GTK_RESPONSE_HELP) {
        bastile_widget_show_help (swidget);
        return;
    }
    
    if (response == GTK_RESPONSE_OK) 
        upload = TRUE;
    else if (response == GTK_RESPONSE_CLOSE)
        upload = FALSE;
    else {
        bastile_widget_destroy (swidget);
        return;
    }
    
    /* The email address */
    widget = bastile_widget_get_widget (swidget, "email-entry");
    g_return_if_fail (widget != NULL);
    email = gtk_entry_get_text (GTK_ENTRY (widget));
    
    /* The algorithm */
    widget = bastile_widget_get_widget (swidget, "algorithm-choice");
    g_return_if_fail (widget != NULL);
#if GTK_CHECK_VERSION (2,91,2)
    t = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (widget));
#else
    t = gtk_combo_box_get_active_text (GTK_COMBO_BOX (widget));
#endif
    if (t && strstr (t, "DSA"))
        type = SSH_ALGO_DSA;
    else
        type = SSH_ALGO_RSA;
    
    /* The number of bits */
    widget = bastile_widget_get_widget (swidget, "bits-entry");
    g_return_if_fail (widget != NULL);
    bits = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (widget));
    if (bits < 512 || bits > 8192) {
        g_message ("invalid key size: %s defaulting to 2048", t);
        bits = 2048;
    }
    
    src = BASTILE_SSH_SOURCE (g_object_get_data (G_OBJECT (swidget), "source"));
    g_return_if_fail (BASTILE_IS_SSH_SOURCE (src));
    
    /* We start creation */
    op = bastile_ssh_operation_generate (src, email, type, bits);
    g_return_if_fail (op != NULL);
    
    /* Watch for errors so we can display */
    bastile_operation_watch (op, (BastileDoneFunc)completion_handler, NULL, NULL, NULL);
    
    /* When completed upload */
    if (upload) {
        bastile_operation_watch (op, (BastileDoneFunc)upload_handler, swidget, NULL, NULL);
		bastile_widget_set_visible (swidget, swidget->name, FALSE);
	}
	else
		bastile_widget_destroy (swidget);
    
    bastile_progress_show (op, _("Creating Secure Shell Key"), TRUE);
    g_object_unref (op);
}

void
bastile_ssh_generate_show (BastileSSHSource *src, GtkWindow *parent)
{
    BastileWidget *swidget;
    GtkWidget *widget;
    
    swidget = bastile_widget_new ("ssh-generate", parent);
    
    /* Widget already present */
    if (swidget == NULL)
        return;

    g_object_ref (src);
    g_object_set_data_full (G_OBJECT (swidget), "source", src, g_object_unref);

    g_signal_connect (G_OBJECT (bastile_widget_get_widget (swidget, "algorithm-choice")), "changed", 
                    G_CALLBACK (on_change), swidget);

    g_signal_connect (bastile_widget_get_toplevel (swidget), "response", 
                    G_CALLBACK (on_response), swidget);

    widget = bastile_widget_get_widget (swidget, "ssh-image");
    g_return_if_fail (widget != NULL);
    gtk_image_set_from_stock (GTK_IMAGE (widget), BASTILE_STOCK_KEY_SSH, GTK_ICON_SIZE_DIALOG);

    /* on_change() gets called, bits entry is setup */
    widget = bastile_widget_get_widget (swidget, "algorithm-choice");
    g_return_if_fail (widget != NULL);
    gtk_combo_box_set_active (GTK_COMBO_BOX (widget), 0);
}
