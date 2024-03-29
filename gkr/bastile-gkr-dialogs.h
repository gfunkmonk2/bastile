/* 
 * Bastile
 * 
 * Copyright (C) 2008 Stefan Walter
 * 
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *  
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.  
 */

#ifndef __BASTILE_GKR_DIALOGS__
#define __BASTILE_GKR_DIALOGS__

#include <gtk/gtk.h>

#include "bastile-gkr-item.h"
#include "bastile-gkr-keyring.h"
#include "bastile-widget.h"

void            bastile_gkr_add_keyring_register     (void);

void            bastile_gkr_add_item_show            (GtkWindow *parent);

void            bastile_gkr_add_keyring_show         (GtkWindow *parent);

void            bastile_gkr_item_properties_show     (BastileGkrItem *git, GtkWindow *parent);

void            bastile_gkr_keyring_properties_show  (BastileGkrKeyring *gkr, GtkWindow *parent);

void            bastile_gkr_dialog_begin_request     (BastileWidget *swidget, gpointer request);

void            bastile_gkr_dialog_complete_request  (BastileWidget *swidget, gboolean cancel);

#endif /* __BASTILE_GKR_DIALOGS__ */
