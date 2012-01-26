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

#include <string.h>

#include <glib/gi18n.h>
 
#include "bastile-object-widget.h"
#include "bastile-util.h"

#include "bastile-gpgme-dialogs.h"
#include "bastile-gpgme-key-op.h"

#define NAME "name"
#define EMAIL "email"

static void
check_ok (BastileWidget *swidget)
{
	const gchar *name, *email;
	
	/* must be at least 5 characters */
	name = gtk_entry_get_text (GTK_ENTRY (
		bastile_widget_get_widget (swidget, NAME)));
	/* must be empty or be *@* */
	email = gtk_entry_get_text (GTK_ENTRY (
		bastile_widget_get_widget (swidget, EMAIL)));
	
	gtk_widget_set_sensitive (GTK_WIDGET (bastile_widget_get_widget (swidget, "ok")),
		strlen (name) >= 5 && (strlen (email) == 0  ||
		(g_pattern_match_simple ("?*@?*", email))));
}

G_MODULE_EXPORT void
on_gpgme_add_uid_name_changed (GtkEditable *editable, BastileWidget *swidget)
{
	check_ok (swidget);
}

G_MODULE_EXPORT void
on_gpgme_add_uid_email_changed (GtkEditable *editable, BastileWidget *swidget)
{
	check_ok (swidget);
}

G_MODULE_EXPORT void
on_gpgme_add_uid_ok_clicked (GtkButton *button, BastileWidget *swidget)
{
	BastileObject *object;
	const gchar *name, *email, *comment;
	gpgme_error_t err;
	
	object = BASTILE_OBJECT_WIDGET (swidget)->object;
	
	name = gtk_entry_get_text (GTK_ENTRY (
		bastile_widget_get_widget (swidget, NAME)));
	email = gtk_entry_get_text (GTK_ENTRY (
		bastile_widget_get_widget (swidget, EMAIL)));
	comment = gtk_entry_get_text (GTK_ENTRY (
		bastile_widget_get_widget (swidget, "comment")));
	
	err = bastile_gpgme_key_op_add_uid (BASTILE_GPGME_KEY (object),
	                                     name, email, comment);
	if (!GPG_IS_OK (err))
		bastile_gpgme_handle_error (err, _("Couldn't add user id"));
	else
		bastile_widget_destroy (swidget);
}

/**
 * bastile_add_uid_new:
 * @skey: #BastileKey
 *
 * Creates a new #BastileKeyWidget dialog for adding a user ID to @skey.
 **/
void
bastile_gpgme_add_uid_new (BastileGpgmeKey *pkey, GtkWindow *parent)
{
	BastileWidget *swidget;
	const gchar *userid;
	
	swidget = bastile_object_widget_new ("add-uid", parent, BASTILE_OBJECT (pkey));
	g_return_if_fail (swidget != NULL);
	
	userid = bastile_object_get_label (BASTILE_OBJECT (pkey));
	gtk_window_set_title (GTK_WINDOW (bastile_widget_get_widget (swidget, swidget->name)),
		g_strdup_printf (_("Add user ID to %s"), userid));
}
