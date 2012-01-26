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

#include "bastile-gkr-module.h"

#include "bastile-gkr-dialogs.h"
#include "bastile-gkr-item-commands.h"
#include "bastile-gkr-keyring-commands.h"
#include "bastile-gkr-source.h"

#include "bastile-context.h"

#include <mate-keyring.h>

void
bastile_gkr_module_init (void)
{
	BastileSource *source;

	source = BASTILE_SOURCE (bastile_gkr_source_new ());
	bastile_context_take_source (NULL, source);
	
	/* Let these classes register themselves */
	g_type_class_unref (g_type_class_ref (BASTILE_TYPE_GKR_SOURCE));
	g_type_class_unref (g_type_class_ref (BASTILE_TYPE_GKR_ITEM_COMMANDS));
	g_type_class_unref (g_type_class_ref (BASTILE_TYPE_GKR_KEYRING_COMMANDS));
}
