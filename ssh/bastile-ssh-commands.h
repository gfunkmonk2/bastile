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

#ifndef __BASTILE_SSH_COMMANDS_H__
#define __BASTILE_SSH_COMMANDS_H__

#include <glib.h>
#include <glib-object.h>

#include "bastile-commands.h"
#include "bastile-object.h"
#include <bastile-operation.h>


#define BASTILE_TYPE_SSH_COMMANDS                 (bastile_ssh_commands_get_type ())
#define BASTILE_SSH_COMMANDS(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_SSH_COMMANDS, BastileSshCommands))
#define BASTILE_SSH_COMMANDS_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_SSH_COMMANDS, BastileSshCommandsClass))
#define BASTILE_IS_SSH_COMMANDS(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_SSH_COMMANDS))
#define BASTILE_IS_SSH_COMMANDS_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_SSH_COMMANDS))
#define BASTILE_SSH_COMMANDS_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_SSH_COMMANDS, BastileSshCommandsClass))

typedef struct _BastileSshCommands BastileSshCommands;
typedef struct _BastileSshCommandsClass BastileSshCommandsClass;
typedef struct _BastileSshCommandsPrivate BastileSshCommandsPrivate;

struct _BastileSshCommands {
	BastileCommands parent_instance;
	BastileSshCommandsPrivate *pv;
};

struct _BastileSshCommandsClass {
	BastileCommandsClass parent_class;
};

GType                         bastile_ssh_commands_get_type   (void);

BastileSshCommands*          bastile_ssh_commands_new        (void);


#endif
