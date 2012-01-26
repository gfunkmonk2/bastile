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

#ifndef __BASTILE_PGP_COMMANDS_H__
#define __BASTILE_PGP_COMMANDS_H__

#include <glib.h>
#include <glib-object.h>

#include "bastile-commands.h"
#include "bastile-object.h"
#include <bastile-operation.h>


#define BASTILE_TYPE_PGP_COMMANDS                 (bastile_pgp_commands_get_type ())
#define BASTILE_PGP_COMMANDS(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_PGP_COMMANDS, BastilePgpCommands))
#define BASTILE_PGP_COMMANDS_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_PGP_COMMANDS, BastilePgpCommandsClass))
#define BASTILE_IS_PGP_COMMANDS(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_PGP_COMMANDS))
#define BASTILE_IS_PGP_COMMANDS_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_PGP_COMMANDS))
#define BASTILE_PGP_COMMANDS_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_PGP_COMMANDS, BastilePgpCommandsClass))

typedef struct _BastilePgpCommands BastilePgpCommands;
typedef struct _BastilePgpCommandsClass BastilePgpCommandsClass;
typedef struct _BastilePgpCommandsPrivate BastilePgpCommandsPrivate;

struct _BastilePgpCommands {
	BastileCommands parent_instance;
	BastilePgpCommandsPrivate *pv;
};

struct _BastilePgpCommandsClass {
	BastileCommandsClass parent_class;
};

GType                         bastile_pgp_commands_get_type   (void);

BastilePgpCommands*          bastile_pgp_commands_new        (void);


#endif
