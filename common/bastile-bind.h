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
   
#ifndef BASTILEBIND_H_
#define BASTILEBIND_H_

#include <glib-object.h>

typedef gboolean (*BastileTransform) (const GValue *src, GValue *dest);

gpointer bastile_bind_property      (const gchar *prop_src, gpointer obj_src, 
                                      const gchar *prop_dest, gpointer obj_dest);

gpointer bastile_bind_property_full (const gchar *prop_src, gpointer obj_src,
                                      BastileTransform transform, 
                                      const gchar *prop_dest, ...) G_GNUC_NULL_TERMINATED;

typedef gboolean (*BastileTransfer) (GObject *src, GObject *dest);

gpointer bastile_bind_objects       (const gchar *property, gpointer obj_src,
                                      BastileTransfer transfer, gpointer obj_dest);

void     bastile_bind_disconnect    (gpointer binding);

#endif /* BASTILEBIND_H_ */
