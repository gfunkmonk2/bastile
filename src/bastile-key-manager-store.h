/*
 * Bastile
 *
 * Copyright (C) 2003 Jacob Perkins
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

#ifndef __BASTILE_KEY_MANAGER_STORE_H__
#define __BASTILE_KEY_MANAGER_STORE_H__

#include <gtk/gtk.h>

#include "bastile-context.h"
#include "bastile-set.h"
#include "bastile-set-model.h"

#define BASTILE_TYPE_KEY_MANAGER_STORE             (bastile_key_manager_store_get_type ())
#define BASTILE_KEY_MANAGER_STORE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_KEY_MANAGER_STORE, BastileKeyManagerStore))
#define BASTILE_KEY_MANAGER_STORE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_KEY_MANAGER_STORE, BastileKeyManagerStoreClass))
#define BASTILE_IS_KEY_MANAGER_STORE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_KEY_MANAGER_STORE))
#define BASTILE_IS_KEY_MANAGER_STORE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_KEY_MANAGER_STORE))
#define BASTILE_KEY_MANAGER_STORE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_KEY_MANAGER_STORE, BastileKeyManagerStoreClass))

typedef struct _BastileKeyManagerStore BastileKeyManagerStore;
typedef struct _BastileKeyManagerStorePriv BastileKeyManagerStorePriv;
typedef struct _BastileKeyManagerStoreClass BastileKeyManagerStoreClass;

typedef enum _BastileKeyManagerStoreMode {
	KEY_STORE_MODE_ALL,
	KEY_STORE_MODE_FILTERED
} BastileKeyManagerStoreMode;

struct _BastileKeyManagerStore {
	BastileSetModel               parent;
	
	/*< private >*/
	BastileKeyManagerStorePriv    *priv;
};

struct _BastileKeyManagerStoreClass {
	BastileSetModelClass           parent_class;
};

BastileKeyManagerStore*   bastile_key_manager_store_new                    (BastileSet *skset,
                                                                              GtkTreeView *view);

BastileObject*            bastile_key_manager_store_get_object_from_path   (GtkTreeView *view,
                                                                              GtkTreePath *path);

GList*                     bastile_key_manager_store_get_all_objects        (GtkTreeView *view);

void                       bastile_key_manager_store_set_selected_objects   (GtkTreeView *view,
                                                                              GList* objects);
                                                             
GList*                     bastile_key_manager_store_get_selected_objects   (GtkTreeView *view);

BastileObject*            bastile_key_manager_store_get_selected_object    (GtkTreeView *view);

#endif /* __BASTILE_KEY_MANAGER_STORE_H__ */
