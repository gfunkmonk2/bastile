/*
 * Bastile
 *
 * Copyright (C) 2004 Stefan Walter
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

/**
 * Function for displaying the preferences dialog.
 */
 
#ifndef __BASTILE_PREFS_H__
#define __BASTILE_PREFS_H__

#include "bastile-context.h"
#include "bastile-widget.h"

BastileWidget *    bastile_prefs_new          (GtkWindow *parent);

void                bastile_prefs_add_tab      (BastileWidget *swidget,
                                                 GtkWidget *label,
                                                 GtkWidget *tab);
                                                 
void                bastile_prefs_select_tab   (BastileWidget *swidget,
                                                 GtkWidget *tab);

void                bastile_prefs_select_tabid (BastileWidget *swidget,
                                                 const gchar *tab);

void                bastile_prefs_remove_tab   (BastileWidget *swidget,
                                                 GtkWidget *tab);

#endif /* __BASTILE_PREFERENCES_H__ */       
