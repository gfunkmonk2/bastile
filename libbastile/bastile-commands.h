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

#ifndef __BASTILE_COMMANDS_H__
#define __BASTILE_COMMANDS_H__

#include <glib.h>
#include <glib-object.h>

#include "bastile-operation.h"
#include "bastile-object.h"
#include "bastile-view.h"

#define BASTILE_TYPE_COMMANDS                  (bastile_commands_get_type ())
#define BASTILE_COMMANDS(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_COMMANDS, BastileCommands))
#define BASTILE_COMMANDS_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_COMMANDS, BastileCommandsClass))
#define BASTILE_IS_COMMANDS(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_COMMANDS))
#define BASTILE_IS_COMMANDS_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_COMMANDS))
#define BASTILE_COMMANDS_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_COMMANDS, BastileCommandsClass))

typedef struct _BastileCommandsClass BastileCommandsClass;
typedef struct _BastileCommandsPrivate BastileCommandsPrivate;

struct _BastileCommands {
	GObject parent_instance;
	BastileCommandsPrivate *pv;
};

struct _BastileCommandsClass {
	GObjectClass parent_class;
	
	/* Virtual methods */
	
	void                 (*show_properties)      (BastileCommands *self, BastileObject *obj);
	
	BastileOperation *  (*delete_objects)       (BastileCommands *self, GList *obj);
};

GType                 bastile_commands_get_type                 (void);

void                  bastile_commands_show_properties          (BastileCommands *self, BastileObject *obj);

BastileOperation *   bastile_commands_delete_objects           (BastileCommands *self, GList *obj);

BastileView *        bastile_commands_get_view                 (BastileCommands *self);

GtkWindow *           bastile_commands_get_window               (BastileCommands *self);

GtkActionGroup *      bastile_commands_get_command_actions      (BastileCommands *self);

const char *          bastile_commands_get_ui_definition        (BastileCommands *self);

#endif
