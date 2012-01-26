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

#include "bastile-check-button-control.h"
#include "bastile-mateconf.h"

static void
mateconf_notify (MateConfClient *client, guint id, MateConfEntry *entry, GtkCheckButton *check)
{
    const char *mateconf_key = g_object_get_data (G_OBJECT (check), "mateconf-key");
    if (g_str_equal (mateconf_key, mateconf_entry_get_key (entry))) {
        MateConfValue *value = mateconf_entry_get_value (entry);
        if (value)
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check),
                                          mateconf_value_get_bool (value));
    }
}

static void
check_toggled (GtkCheckButton *check, gpointer data)
{
    const char *mateconf_key = g_object_get_data (G_OBJECT (check), "mateconf-key");
    bastile_mateconf_set_boolean (mateconf_key, 
                gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check)));
}

void
bastile_check_button_mateconf_attach  (GtkCheckButton *check, const char *mateconf_key)
{
    MateConfEntry *entry;
    
    g_return_if_fail (GTK_IS_CHECK_BUTTON (check));
    g_return_if_fail (mateconf_key && *mateconf_key);
    
    g_object_set_data_full (G_OBJECT (check), "mateconf-key", g_strdup (mateconf_key), g_free);
    bastile_mateconf_notify_lazy (mateconf_key, (MateConfClientNotifyFunc)mateconf_notify, 
                                check, GTK_WIDGET (check));
    
    /* Load initial value */
    entry = bastile_mateconf_get_entry (mateconf_key);
    g_return_if_fail (entry != NULL);
    mateconf_notify (NULL, 0, entry, check);
    
    g_signal_connect (check, "toggled", G_CALLBACK (check_toggled), NULL); 
}
