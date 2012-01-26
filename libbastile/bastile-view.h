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

#ifndef __BASTILE_VIEW_H__
#define __BASTILE_VIEW_H__

#include <glib.h>
#include <glib-object.h>

#include "bastile-set.h"
#include "bastile-object.h"

typedef struct _BastileCommands BastileCommands;

#define BASTILE_TYPE_VIEW                 (bastile_view_get_type ())
#define BASTILE_VIEW(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_VIEW, BastileView))
#define BASTILE_IS_VIEW(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_VIEW))
#define BASTILE_VIEW_GET_INTERFACE(obj)   (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BASTILE_TYPE_VIEW, BastileViewIface))

typedef struct _BastileView BastileView;
typedef struct _BastileViewIface BastileViewIface;

struct _BastileViewIface {
	GTypeInterface parent_iface;

	/* virtual metdods */
	GList*          (*get_selected_objects)   (BastileView *self);
	
	void            (*set_selected_objects)   (BastileView *self, 
	                                           GList *objects);
	
	GList*          (*get_selected_matching)  (BastileView *self, 
	                                           BastileObjectPredicate *pred);
	
	BastileObject* (*get_selected)           (BastileView *self);
	
	void            (*set_selected)           (BastileView *self, 
	                                           BastileObject *value);
	
	BastileSet*    (*get_current_set)        (BastileView *self);
	
	GtkWindow*      (*get_window)             (BastileView *self);
	
	void            (*register_commands)      (BastileView *self, 
	                                           BastileObjectPredicate *pred,
	                                           BastileCommands *commands);
	
	void            (*register_ui)            (BastileView *self, 
	                                           BastileObjectPredicate *pred, 
	                                           const gchar *ui_definition, 
	                                           GtkActionGroup *actions);
};


GType             bastile_view_get_type                    (void);

GList*            bastile_view_get_selected_objects        (BastileView *self);

void              bastile_view_set_selected_objects        (BastileView *self, 
                                                             GList *objects);

GList*            bastile_view_get_selected_matching       (BastileView *self, 
                                                             BastileObjectPredicate *pred);

BastileObject*   bastile_view_get_selected                (BastileView *self);

void              bastile_view_set_selected                (BastileView *self, 
                                                             BastileObject *value);

BastileSet*      bastile_view_get_current_set             (BastileView *self);

GtkWindow*        bastile_view_get_window                  (BastileView *self);

void              bastile_view_register_ui                 (BastileView *self, 
                                                             BastileObjectPredicate *pred,
                                                             const gchar *ui_definition,
                                                             GtkActionGroup *actions);

void              bastile_view_register_commands           (BastileView *self, 
                                                             BastileObjectPredicate *pred,
                                                             BastileCommands *commands);

#endif
