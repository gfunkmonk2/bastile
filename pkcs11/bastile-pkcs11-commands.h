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

#ifndef __BASTILE_PKCS11_COMMANDS_H__
#define __BASTILE_PKCS11_COMMANDS_H__

#include "bastile-commands.h"

#include <glib-object.h>

#define BASTILE_PKCS11_TYPE_COMMANDS               (bastile_pkcs11_commands_get_type ())
#define BASTILE_PKCS11_COMMANDS(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_PKCS11_TYPE_COMMANDS, BastilePkcs11Commands))
#define BASTILE_PKCS11_COMMANDS_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_PKCS11_TYPE_COMMANDS, BastilePkcs11CommandsClass))
#define BASTILE_PKCS11_IS_COMMANDS(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_PKCS11_TYPE_COMMANDS))
#define BASTILE_PKCS11_IS_COMMANDS_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_PKCS11_TYPE_COMMANDS))
#define BASTILE_PKCS11_COMMANDS_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_PKCS11_TYPE_COMMANDS, BastilePkcs11CommandsClass))

typedef struct _BastilePkcs11Commands BastilePkcs11Commands;
typedef struct _BastilePkcs11CommandsClass BastilePkcs11CommandsClass;
typedef struct _BastilePkcs11CommandsPrivate BastilePkcs11CommandsPrivate;
    
struct _BastilePkcs11Commands {
	BastileCommands parent;
};

struct _BastilePkcs11CommandsClass {
	BastileCommandsClass parent_class;
};

GType                        bastile_pkcs11_commands_get_type               (void);

BastilePkcs11Commands*      bastile_pkcs11_commands_new                    (void);

#endif /* __BASTILE_PKCS11_COMMANDS_H__ */
