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

#include "bastile-ssh-module.h"

#include "bastile-ssh-commands.h"
#include "bastile-ssh-dialogs.h"
#include "bastile-ssh-source.h"

#include "bastile-context.h"

void
bastile_ssh_module_init (void)
{
	BastileSource *source;

	/* Always have a default pgp source added */
	source = g_object_new (BASTILE_TYPE_SSH_SOURCE, NULL);
	bastile_context_take_source (NULL, source);

	g_type_class_unref (g_type_class_ref (BASTILE_TYPE_SSH_SOURCE));
	g_type_class_unref (g_type_class_ref (BASTILE_TYPE_SSH_COMMANDS));

	bastile_ssh_generate_register ();
}
