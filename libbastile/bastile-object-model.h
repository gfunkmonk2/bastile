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
 
#ifndef __BASTILE_OBJECT_MODEL_H__
#define __BASTILE_OBJECT_MODEL_H__

#include <gtk/gtk.h>

#include "bastile-object.h"

#define BASTILE_TYPE_OBJECT_MODEL               (bastile_object_model_get_type ())
#define BASTILE_OBJECT_MODEL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_OBJECT_MODEL, BastileObjectModel))
#define BASTILE_OBJECT_MODEL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_OBJECT_MODEL, BastileObjectModelClass))
#define BASTILE_IS_OBJECT_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_OBJECT_MODEL))
#define BASTILE_IS_OBJECT_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_OBJECT_MODEL))
#define BASTILE_OBJECT_MODEL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_OBJECT_MODEL, BastileObjectModelClass))

typedef struct _BastileObjectModel BastileObjectModel;
typedef struct _BastileObjectModelClass BastileObjectModelClass;

/**
 * BastileObjectModel:
 * @parent: The parent #GtkTreeStore
 *
 * A GtkTreeModel that can assign certain rows as
 * 'key rows' which are updated when a key is updated.
 *
 * Signals:
 *   update-row: A request to update a row
 */

struct _BastileObjectModel {
    GtkTreeStore parent;
};

struct _BastileObjectModelClass {
    GtkTreeStoreClass parent_class;
    
    /* signals --------------------------------------------------------- */
    
    /* A key was added to this view */
    void (*update_row)   (BastileObjectModel *self, BastileObject *object, GtkTreeIter *iter);
};

GType               bastile_object_model_get_type                (void);

BastileObjectModel*   bastile_object_model_new                  (gint n_columns,
                                                                   GType *types);

void                bastile_object_model_set_column_types        (BastileObjectModel *self, 
                                                                   gint n_columns,
                                                                   GType *types);

void                bastile_object_model_set_row_object          (BastileObjectModel *self,
                                                                   GtkTreeIter *iter,
                                                                   BastileObject *object);

BastileObject*     bastile_object_model_get_row_key             (BastileObjectModel *self,
                                                                   GtkTreeIter *iter);

GSList*             bastile_object_model_get_rows_for_object     (BastileObjectModel *self,
                                                                   BastileObject *object);

void                bastile_object_model_remove_rows_for_object  (BastileObjectModel *self,
                                                                   BastileObject *object);

void                bastile_object_model_free_rows               (GSList *rows);

#endif /* __BASTILE_OBJECT_MODEL_H__ */
