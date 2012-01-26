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
 
#ifndef __BASTILE_SET_MODEL_H__
#define __BASTILE_SET_MODEL_H__

#include <gtk/gtk.h>
#include "bastile-object.h"
#include "bastile-set.h"

typedef struct _BastileSetModelColumn {
	const gchar *property;
	GType type;
	gpointer data;
} BastileSetModelColumn;

#define BASTILE_TYPE_SET_MODEL               (bastile_set_model_get_type ())
#define BASTILE_SET_MODEL(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_SET_MODEL, BastileSetModel))
#define BASTILE_SET_MODEL_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_SET_MODEL, BastileSetModelClass))
#define BASTILE_IS_SET_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_SET_MODEL))
#define BASTILE_IS_SET_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_SET_MODEL))
#define BASTILE_SET_MODEL_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_SET_MODEL, BastileSetModelClass))

typedef struct _BastileSetModel BastileSetModel;
typedef struct _BastileSetModelClass BastileSetModelClass;

/**
 * BastileSetModel:
 * @parent: #GObject #BastileSetModel inherits from
 * @set: #BastileSet that belongs to the model
 *
 * A GtkTreeModel which represents all objects in a BastileSet
 */

struct _BastileSetModel {
	GObject parent;
	BastileSet *set;
};

struct _BastileSetModelClass {
	GObjectClass parent_class;
};

GType               bastile_set_model_get_type                (void);

BastileSetModel*   bastile_set_model_new                     (BastileSet *set,
                                                                ...) G_GNUC_NULL_TERMINATED;

BastileSetModel*   bastile_set_model_new_full                (BastileSet *set,
                                                                const BastileSetModelColumn *columns,
                                                                guint n_columns);

gint                bastile_set_model_set_columns             (BastileSetModel *smodel, 
                                                                const BastileSetModelColumn *columns,
                                                                guint n_columns);

BastileObject*     bastile_set_model_object_for_iter         (BastileSetModel *smodel,
                                                                const GtkTreeIter *iter);

gboolean            bastile_set_model_iter_for_object         (BastileSetModel *smodel,
                                                                BastileObject *object,
                                                                GtkTreeIter *iter);

#endif /* __BASTILE_SET_H__ */
