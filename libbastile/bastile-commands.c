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

#include "config.h"

#include "bastile-commands.h"

enum {
	PROP_0,
	PROP_VIEW,
	PROP_KTYPE,
	PROP_COMMAND_ACTIONS,
	PROP_UI_DEFINITION
};

struct _BastileCommandsPrivate {
	BastileView* view;
};

G_DEFINE_TYPE (BastileCommands, bastile_commands, G_TYPE_OBJECT);

#define BASTILE_COMMANDS_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), BASTILE_TYPE_COMMANDS, BastileCommandsPrivate))

/* -----------------------------------------------------------------------------
 * OBJECT 
 */

static void 
bastile_commands_real_show_properties (BastileCommands* self, BastileObject* obj) 
{
	g_critical ("Type `%s' does not implement abstract method `bastile_commands_show_properties'", 
	            g_type_name (G_TYPE_FROM_INSTANCE (self)));
	return;
}

static BastileOperation* 
bastile_commands_real_delete_objects (BastileCommands* self, GList* obj) 
{
	g_critical ("Type `%s' does not implement abstract method `bastile_commands_delete_objects'", 
	            g_type_name (G_TYPE_FROM_INSTANCE (self)));
	return NULL;
}

static GObject* 
bastile_commands_constructor (GType type, guint n_props, GObjectConstructParam *props) 
{
	GObject *obj = G_OBJECT_CLASS (bastile_commands_parent_class)->constructor (type, n_props, props);
	BastileCommands *self = NULL;
	BastileCommandsPrivate *pv;
	
	if (obj) {
		pv = BASTILE_COMMANDS_GET_PRIVATE (obj);
		self = BASTILE_COMMANDS (obj);
		
	}
	
	return obj;
}

static void
bastile_commands_init (BastileCommands *self)
{
	self->pv = BASTILE_COMMANDS_GET_PRIVATE (self);
}

static void
bastile_commands_dispose (GObject *obj)
{
	BastileCommands *self = BASTILE_COMMANDS (obj);
    
	if (self->pv->view)
		g_object_remove_weak_pointer (G_OBJECT (self->pv->view), (gpointer*)&self->pv->view);
	self->pv->view = NULL;
	
	G_OBJECT_CLASS (bastile_commands_parent_class)->dispose (obj);
}

static void
bastile_commands_finalize (GObject *obj)
{
	BastileCommands *self = BASTILE_COMMANDS (obj);

	g_assert (!self->pv->view);
	
	G_OBJECT_CLASS (bastile_commands_parent_class)->finalize (obj);
}

static void
bastile_commands_set_property (GObject *obj, guint prop_id, const GValue *value, 
                                GParamSpec *pspec)
{
	BastileCommands *self = BASTILE_COMMANDS (obj);
	
	switch (prop_id) {
	case PROP_VIEW:
		g_return_if_fail (!self->pv->view);
		self->pv->view = g_value_get_object (value);
		g_return_if_fail (self->pv->view);
		g_object_add_weak_pointer (G_OBJECT (self->pv->view), (gpointer*)&self->pv->view);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
bastile_commands_get_property (GObject *obj, guint prop_id, GValue *value, 
                           GParamSpec *pspec)
{
	BastileCommands *self = BASTILE_COMMANDS (obj);
	
	switch (prop_id) {
	case PROP_VIEW:
		g_value_set_object (value, bastile_commands_get_view (self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
bastile_commands_class_init (BastileCommandsClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    
	bastile_commands_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (BastileCommandsPrivate));

	gobject_class->constructor = bastile_commands_constructor;
	gobject_class->dispose = bastile_commands_dispose;
	gobject_class->finalize = bastile_commands_finalize;
	gobject_class->set_property = bastile_commands_set_property;
	gobject_class->get_property = bastile_commands_get_property;
    
	BASTILE_COMMANDS_CLASS (klass)->show_properties = bastile_commands_real_show_properties;
	BASTILE_COMMANDS_CLASS (klass)->delete_objects = bastile_commands_real_delete_objects;
	
	g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_VIEW, 
	         g_param_spec_object ("view", "view", "view", BASTILE_TYPE_VIEW, 
	                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	
	g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_KTYPE, 
	         g_param_spec_uint ("ktype", "ktype", "ktype", 
	                            0, G_MAXUINT, 0U, G_PARAM_READABLE));
	
	g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_COMMAND_ACTIONS, 
	         g_param_spec_object ("command-actions", "command-actions", "command-actions", 
	                              GTK_TYPE_ACTION_GROUP, G_PARAM_READABLE));
	
	g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_UI_DEFINITION, 
	         g_param_spec_pointer ("ui-definition", "ui-definition", "ui-definition", 
	                               G_PARAM_READABLE));
}

/* -----------------------------------------------------------------------------
 * PUBLIC 
 */

void 
bastile_commands_show_properties (BastileCommands* self, BastileObject* obj) 
{
	g_return_if_fail (BASTILE_IS_COMMANDS (self));
	BASTILE_COMMANDS_GET_CLASS (self)->show_properties (self, obj);
}

BastileOperation* 
bastile_commands_delete_objects (BastileCommands* self, GList* obj) 
{
	g_return_val_if_fail (BASTILE_IS_COMMANDS (self), NULL);
	return BASTILE_COMMANDS_GET_CLASS (self)->delete_objects (self, obj);
}

BastileView*
bastile_commands_get_view (BastileCommands* self) 
{
	g_return_val_if_fail (BASTILE_IS_COMMANDS (self), NULL);
	return self->pv->view;
}

GtkActionGroup*
bastile_commands_get_command_actions (BastileCommands* self) 
{
	GtkActionGroup *actions;
	g_return_val_if_fail (BASTILE_IS_COMMANDS (self), NULL);
	g_object_get (self, "command-actions", &actions, NULL);
	if (actions)
		g_object_unref (actions);
	return actions;
}


const gchar* 
bastile_commands_get_ui_definition (BastileCommands* self) 
{
	const gchar* ui_definition;
	g_return_val_if_fail (BASTILE_IS_COMMANDS (self), NULL);
	g_object_get (self, "ui-definition", &ui_definition, NULL);
	return ui_definition;
}

GtkWindow*
bastile_commands_get_window (BastileCommands* self)
{
	BastileView *view = bastile_commands_get_view (self);
	g_return_val_if_fail (view, NULL);
	return bastile_view_get_window (view);
}
