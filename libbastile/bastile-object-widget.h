/*
 * Bastile
 *
 * Copyright (C) 2003 Jacob Perkins
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

#ifndef __BASTILE_OBJECT_WIDGET_H__
#define __BASTILE_OBJECT_WIDGET_H__

#include <glib.h>

#include "bastile-object.h"
#include "bastile-widget.h"

#define BASTILE_TYPE_OBJECT_WIDGET		(bastile_object_widget_get_type ())
#define BASTILE_OBJECT_WIDGET(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_OBJECT_WIDGET, BastileObjectWidget))
#define BASTILE_OBJECT_WIDGET_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_OBJECT_WIDGET, BastileObjectWidgetClass))
#define BASTILE_IS_OBJECT_WIDGET(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_OBJECT_WIDGET))
#define BASTILE_IS_OBJECT_WIDGET_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_OBJECT_WIDGET))
#define BASTILE_OBJECT_WIDGET_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_OBJECT_WIDGET, BastileObjectWidgetClass))

typedef struct _BastileObjectWidget BastileObjectWidget;
typedef struct _BastileObjectWidgetClass BastileObjectWidgetClass;
	
struct _BastileObjectWidget {
	BastileWidget		parent;
	BastileObject		*object;
};

struct _BastileObjectWidgetClass {
	BastileWidgetClass	parent_class;
};

GType           bastile_object_widget_get_type    (void);

BastileWidget*	bastile_object_widget_new         (gchar              *name,
                                                    GtkWindow          *parent,
                                                    BastileObject     *object);

#endif /* __BASTILE_OBJECT_WIDGET_H__ */
