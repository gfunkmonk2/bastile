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

#include "bastile-view.h"

GList* 
bastile_view_get_selected_objects (BastileView* self) 
{
	g_return_val_if_fail (BASTILE_VIEW_GET_INTERFACE (self)->get_selected_objects, NULL);
	return BASTILE_VIEW_GET_INTERFACE (self)->get_selected_objects (self);
}


void 
bastile_view_set_selected_objects (BastileView* self, GList* objects) 
{
	g_return_if_fail (BASTILE_VIEW_GET_INTERFACE (self)->set_selected_objects);
	BASTILE_VIEW_GET_INTERFACE (self)->set_selected_objects (self, objects);
}


GList*
bastile_view_get_selected_matching (BastileView *self, BastileObjectPredicate *pred)
{
	g_return_val_if_fail (BASTILE_VIEW_GET_INTERFACE (self)->get_selected_matching, NULL);
	return BASTILE_VIEW_GET_INTERFACE (self)->get_selected_matching (self, pred);
}

BastileObject* 
bastile_view_get_selected (BastileView* self) 
{
	g_return_val_if_fail (BASTILE_VIEW_GET_INTERFACE (self)->get_selected, NULL);
	return BASTILE_VIEW_GET_INTERFACE (self)->get_selected (self);
}


void 
bastile_view_set_selected (BastileView* self, BastileObject* value) 
{
	g_return_if_fail (BASTILE_VIEW_GET_INTERFACE (self)->set_selected);
	BASTILE_VIEW_GET_INTERFACE (self)->set_selected (self, value);
}


BastileSet* 
bastile_view_get_current_set (BastileView* self) 
{
	g_return_val_if_fail (BASTILE_VIEW_GET_INTERFACE (self)->get_current_set, NULL);
	return BASTILE_VIEW_GET_INTERFACE (self)->get_current_set (self);
}


GtkWindow* 
bastile_view_get_window (BastileView* self) 
{
	g_return_val_if_fail (BASTILE_VIEW_GET_INTERFACE (self)->get_window, NULL);
	return BASTILE_VIEW_GET_INTERFACE (self)->get_window (self);
}

void
bastile_view_register_commands (BastileView *self, BastileObjectPredicate *pred,
                                 BastileCommands *commands)
{
	g_return_if_fail (BASTILE_VIEW_GET_INTERFACE (self)->register_commands);
	BASTILE_VIEW_GET_INTERFACE (self)->register_commands (self, pred, commands);
}

void
bastile_view_register_ui (BastileView *self, BastileObjectPredicate *pred, 
                           const gchar *ui_definition, GtkActionGroup *actions)
{
	g_return_if_fail (BASTILE_VIEW_GET_INTERFACE (self)->register_ui);
	BASTILE_VIEW_GET_INTERFACE (self)->register_ui (self, pred, ui_definition, actions);
}

static void 
bastile_view_base_init (BastileViewIface * iface) 
{
	static gboolean initialized = FALSE;
	if (!initialized) {
		initialized = TRUE;
		g_object_interface_install_property (iface, 
		         g_param_spec_object ("selected", "selected", "selected", 
		                              BASTILE_TYPE_OBJECT, G_PARAM_READWRITE));
		
		g_object_interface_install_property (iface, 
		         g_param_spec_object ("current-set", "current-set", "current-set", 
		                              BASTILE_TYPE_SET, G_PARAM_READABLE));
		
		g_object_interface_install_property (iface, 
		         g_param_spec_object ("window", "window", "window", 
		                              GTK_TYPE_WINDOW, G_PARAM_READABLE));
		
		g_signal_new ("selection_changed", BASTILE_TYPE_VIEW, G_SIGNAL_RUN_LAST, 0, NULL, NULL, 
		              g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	}
}

GType 
bastile_view_get_type (void) 
{
	static GType bastile_view_type_id = 0;
	
	if (bastile_view_type_id == 0) {
		static const GTypeInfo type_info = { 
			sizeof (BastileViewIface), (GBaseInitFunc) bastile_view_base_init, 
			(GBaseFinalizeFunc) NULL, (GClassInitFunc) NULL, 
			(GClassFinalizeFunc) NULL, NULL, 0, 0, (GInstanceInitFunc) NULL 
		};
		
		bastile_view_type_id = g_type_register_static (G_TYPE_INTERFACE, "BastileView", &type_info, 0);
		g_type_interface_add_prerequisite (bastile_view_type_id, G_TYPE_OBJECT);
	}
	
	return bastile_view_type_id;
}
