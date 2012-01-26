/*
 * Bastile
 *
 * Copyright (C) 2003 Jacob Perkins
 * Copyright (C) 2004-2005 Stefan Walter
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
 * Various UI elements and dialogs used in libbastile.
 */
 
#ifndef __BASTILE_LIBDIALOGS_H__
#define __BASTILE_LIBDIALOGS_H__

#include <glib.h>

#include "bastile-context.h"
#include "bastile-widget.h"

void            bastile_notify_import              (guint keynum, gchar **keys);
                                                     
void            bastile_notify_import_local        (guint keys,
                                                     GtkWidget *attachto);

void            bastile_notification_display       (const gchar *summary, 
                                                     const gchar* body, 
                                                     gboolean urgent, 
                                                     const gchar *icon,
                                                     GtkWidget *attachto);
  
gboolean        bastile_notification_have          (void);

#endif /* __BASTILE_LIBDIALOGS_H__ */
