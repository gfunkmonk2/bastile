/*
 * Bastile
 *
 * Copyright (C) 2003 Jacob Perkins
 * Copyright (C) 2005 Jacob Perkins
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

#include "bastile-prefs.h"
#include "bastile-mateconf.h"
#include "bastile-preferences.h"
#include "bastile-widget.h"
#include "bastile-check-button-control.h"

/**
 * bastile_preferences_show:
 * @tabid: The id of the tab to show
 *
 * Creates a new or shows the current preferences dialog.
 **/
void
bastile_preferences_show (GtkWindow *parent, const gchar *tabid)
{   
    BastileWidget *swidget = bastile_prefs_new (parent);
    GtkWidget *tab;
    
    if (tabid) {
        tab = bastile_widget_get_widget (swidget, tabid);
        g_return_if_fail (tab);
        bastile_prefs_select_tab (swidget, tab);
    }
}
