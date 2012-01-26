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

#ifndef BASTILEREGISTRY_H_
#define BASTILEREGISTRY_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define BASTILE_TYPE_REGISTRY             (bastile_registry_get_type())
#define BASTILE_REGISTRY(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), BASTILE_TYPE_REGISTRY, BastileRegistry))
#define BASTILE_REGISTRY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), BASTILE_TYPE_REGISTRY, GObject))
#define BASTILE_IS_REGISTRY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), BASTILE_TYPE_REGISTRY))
#define BASTILE_IS_REGISTRY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), BASTILE_TYPE_REGISTRY))
#define BASTILE_REGISTRY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), BASTILE_TYPE_REGISTRY, BastileRegistryClass))

typedef struct _BastileRegistry      BastileRegistry;
typedef struct _BastileRegistryClass BastileRegistryClass;

struct _BastileRegistry {
	 GObject parent;
};

struct _BastileRegistryClass {
	GObjectClass parent_class;
};

/* member functions */
GType                bastile_registry_get_type          (void) G_GNUC_CONST;

BastileRegistry*    bastile_registry_get               (void);

void                 bastile_registry_register_type     (BastileRegistry *registry, 
                                                          GType type, const gchar *category, 
                                                          ...) G_GNUC_NULL_TERMINATED;

void                 bastile_registry_register_object   (BastileRegistry *registry, 
                                                          GObject *object, const gchar *category, 
                                                          ...) G_GNUC_NULL_TERMINATED;

void                 bastile_registry_register_function (BastileRegistry *registry, 
                                                          gpointer func, const gchar *category, 
                                                          ...) G_GNUC_NULL_TERMINATED;

GType                bastile_registry_object_type       (BastileRegistry *registry, 
                                                          const gchar *category,
                                                          ...) G_GNUC_NULL_TERMINATED;

GList*               bastile_registry_object_types      (BastileRegistry *registry, 
                                                          const gchar *category,
                                                          ...) G_GNUC_NULL_TERMINATED;

GObject*             bastile_registry_object_instance   (BastileRegistry *registry, 
                                                          const gchar *category,
                                                          ...) G_GNUC_NULL_TERMINATED;

GList*               bastile_registry_object_instances  (BastileRegistry *registry, 
                                                          const gchar *category,
                                                          ...) G_GNUC_NULL_TERMINATED;

gpointer             bastile_registry_lookup_function   (BastileRegistry *registry, 
                                                          const gchar *category, 
                                                          ...) G_GNUC_NULL_TERMINATED;

G_END_DECLS

#endif /*BASTILEREGISTRY_H_*/
