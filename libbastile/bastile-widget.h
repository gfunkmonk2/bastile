/*
 * Bastile
 *
 * Copyright (C) 2003 Jacob Perkins
 * Copyright (C) 2005 Stefan Walter
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
 
#ifndef __BASTILE_WIDGET_H__
#define __BASTILE_WIDGET_H__

#include <glib.h>
#include <gtk/gtk.h>

#include "bastile-context.h"

#define BASTILE_TYPE_WIDGET            (bastile_widget_get_type ())
#define BASTILE_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_WIDGET, BastileWidget))
#define BASTILE_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_WIDGET, BastileWidgetClass))
#define BASTILE_IS_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_WIDGET))
#define BASTILE_IS_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_WIDGET))
#define BASTILE_WIDGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_WIDGET, BastileWidgetClass))

typedef struct _BastileWidget BastileWidget;
typedef struct _BastileWidgetClass BastileWidgetClass;

/**
 * BastileWidget:
 * @parent: The parent #GtkObject
 * @gtkbuilder: The #GtkBuilder object for the #BastileWidget
 * @name: The name of the gtkbuilder file
 *
 * A window created from a gtkbuilder file.
 *
 * - All BastileWidget objects are destroyed when the BastileContext
 *   goes bye-bye.
 * - Implements fun GtkUIManager stuff.
 *
 * Signals:
 *   destroy: The window was destroyed.
 *
 * Properties:
 *   name: (gchar*) The name of the gtkbuilder file to load.
 */

struct _BastileWidget {
	GObject parent;

	/*< public >*/
	GtkBuilder *gtkbuilder;
	gchar *name;

	/*< private >*/
	gboolean destroying;
	gboolean in_destruction;
};

struct _BastileWidgetClass {
	GObjectClass parent_class;

	/*< signals >*/
	void (*destroy) (BastileWidget *swidget);
};

GType            bastile_widget_get_type ();

BastileWidget*  bastile_widget_new                (const gchar      *name,
                                                     GtkWindow        *parent);

BastileWidget*  bastile_widget_new_allow_multiple (const gchar      *name,
                                                     GtkWindow        *parent);

BastileWidget*  bastile_widget_find               (const gchar      *name);

const gchar*     bastile_widget_get_name           (BastileWidget   *swidget);

GtkWidget*       bastile_widget_get_toplevel       (BastileWidget   *swidget);

GtkWidget*       bastile_widget_get_widget         (BastileWidget   *swidget,
                                                     const char       *identifier);

void             bastile_widget_show               (BastileWidget   *swidget);

void             bastile_widget_show_help          (BastileWidget   *swidget);

void             bastile_widget_set_visible        (BastileWidget   *swidget,
                                                     const char       *identifier,
                                                     gboolean         visible);

void             bastile_widget_set_sensitive      (BastileWidget   *swidget,
                                                     const char       *identifier,
                                                     gboolean         sensitive);

void             bastile_widget_destroy            (BastileWidget   *swidget);

#endif /* __BASTILE_WIDGET_H__ */
