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

#ifndef __BASTILE_GKR_KEYRING_COMMANDS_H__
#define __BASTILE_GKR_KEYRING_COMMANDS_H__

#include "bastile-commands.h"

#include <glib-object.h>

#define BASTILE_TYPE_GKR_KEYRING_COMMANDS               (bastile_gkr_keyring_commands_get_type ())
#define BASTILE_GKR_KEYRING_COMMANDS(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_GKR_KEYRING_COMMANDS, BastileGkrKeyringCommands))
#define BASTILE_GKR_KEYRING_COMMANDS_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_GKR_KEYRING_COMMANDS, BastileGkrKeyringCommandsClass))
#define BASTILE_IS_GKR_KEYRING_COMMANDS(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_GKR_KEYRING_COMMANDS))
#define BASTILE_IS_GKR_KEYRING_COMMANDS_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_GKR_KEYRING_COMMANDS))
#define BASTILE_GKR_KEYRING_COMMANDS_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_GKR_KEYRING_COMMANDS, BastileGkrKeyringCommandsClass))

typedef struct _BastileGkrKeyringCommands BastileGkrKeyringCommands;
typedef struct _BastileGkrKeyringCommandsClass BastileGkrKeyringCommandsClass;
typedef struct _BastileGkrKeyringCommandsPrivate BastileGkrKeyringCommandsPrivate;
    
struct _BastileGkrKeyringCommands {
	BastileCommands parent;
	BastileGkrKeyringCommandsPrivate *pv;
};

struct _BastileGkrKeyringCommandsClass {
	BastileCommandsClass parent_class;
};

GType                         bastile_gkr_keyring_commands_get_type               (void);

BastileGkrKeyringCommands*   bastile_gkr_keyring_commands_new                    (void);

#endif /* __BASTILE_GKR_KEYRING_COMMANDS_H__ */
