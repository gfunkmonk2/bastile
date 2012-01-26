/*
 * Bastile
 *
 * Copyright (C) 2008 Stefan Walter
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

#ifndef __BASTILE_SERVERS_H__
#define __BASTILE_SERVERS_H__

#include <glib.h>

GSList*         bastile_servers_get_types              (void);

GSList*         bastile_servers_get_names              (void);

char*           bastile_servers_get_description        (const char* type);

GSList*         bastile_servers_get_uris               (void);

gboolean        bastile_servers_is_valid_uri           (const char* uri);

typedef gboolean (*BastileValidUriFunc) (const gchar *uri);

void            bastile_servers_register_type          (const char* type, 
                                                         const char* description, 
                                                         BastileValidUriFunc validate);

#endif
