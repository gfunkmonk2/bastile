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

#include <string.h>

#include <glib/gi18n.h>
 
#include "bastile-object-widget.h"
#include "bastile-libdialogs.h"
#include "bastile-util.h"

#include "bastile-gpgme-dialogs.h"
#include "bastile-gpgme-key-op.h"
#include "bastile-gpgme-subkey.h"

G_MODULE_EXPORT void
on_gpgme_expire_ok_clicked (GtkButton *button, BastileWidget *swidget)
{
	GtkWidget *widget; 
	BastileGpgmeSubkey *subkey;
	gpgme_error_t err;
	time_t expiry = 0;
	struct tm when;
	
	subkey = BASTILE_GPGME_SUBKEY (g_object_get_data (G_OBJECT (swidget), "subkey"));
	
	widget = GTK_WIDGET (bastile_widget_get_widget (swidget, "expire"));
	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
		
		memset (&when, 0, sizeof (when));            
		widget = GTK_WIDGET (bastile_widget_get_widget (swidget, "calendar"));
		gtk_calendar_get_date (GTK_CALENDAR (widget), (guint*)&(when.tm_year), 
		                       (guint*)&(when.tm_mon), (guint*)&(when.tm_mday));
		when.tm_year -= 1900;
		expiry = mktime (&when);

		if (expiry <= time (NULL)) {
			bastile_util_show_error (widget, _("Invalid expiry date"), 
			                          _("The expiry date must be in the future"));
			return;
		}
	}
	
	widget = bastile_widget_get_widget (swidget, "all-controls");
	gtk_widget_set_sensitive (widget, FALSE);
	g_object_ref (swidget);
	g_object_ref (subkey);
	
	if (expiry != bastile_pgp_subkey_get_expires (BASTILE_PGP_SUBKEY (subkey))) {
		err = bastile_gpgme_key_op_set_expires (subkey, expiry);
		if (!GPG_IS_OK (err))
			bastile_gpgme_handle_error (err, _("Couldn't change expiry date"));
	}
    
	g_object_unref (subkey);
	g_object_unref (swidget);
	bastile_widget_destroy (swidget);
}

G_MODULE_EXPORT void
on_gpgme_expire_toggled (GtkWidget *widget, BastileWidget *swidget)
{
	GtkWidget *expire;
	GtkWidget *cal;
	
	expire = GTK_WIDGET (bastile_widget_get_widget (swidget, "expire"));
	cal = GTK_WIDGET (bastile_widget_get_widget (swidget, "calendar"));

	gtk_widget_set_sensitive (cal, !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (expire)));
}

void
bastile_gpgme_expires_new (BastileGpgmeSubkey *subkey, GtkWindow *parent)
{
	BastileWidget *swidget;
	GtkWidget *date, *expire;
	gulong expires;
	gchar *title;
	const gchar *label;
	
	g_return_if_fail (subkey != NULL && BASTILE_IS_GPGME_SUBKEY (subkey));

	swidget = bastile_widget_new_allow_multiple ("expires", parent);
	g_return_if_fail (swidget != NULL);
	g_object_set_data_full (G_OBJECT (swidget), "subkey", 
	                        g_object_ref (subkey), g_object_unref);
	
	date = GTK_WIDGET (bastile_widget_get_widget (swidget, "calendar"));
	g_return_if_fail (date != NULL);

	expire = GTK_WIDGET (bastile_widget_get_widget (swidget, "expire"));
	expires = bastile_pgp_subkey_get_expires (BASTILE_PGP_SUBKEY (subkey)); 
	if (!expires) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (expire), TRUE);
		gtk_widget_set_sensitive (date, FALSE);
	} else {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (expire), FALSE);
		gtk_widget_set_sensitive (date, TRUE);
	}
    
	if (expires) {
		struct tm t;
		time_t time = (time_t)expires;
		if (gmtime_r (&time, &t)) {
			gtk_calendar_select_month (GTK_CALENDAR (date), t.tm_mon, t.tm_year + 1900);
			gtk_calendar_select_day (GTK_CALENDAR (date), t.tm_mday);
		}
	}
	
	label = bastile_pgp_subkey_get_description (BASTILE_PGP_SUBKEY (subkey));
	title = g_strdup_printf (_("Expiry: %s"), label);
	gtk_window_set_title (GTK_WINDOW (bastile_widget_get_widget (swidget, swidget->name)), title);
	g_free (title);
}
