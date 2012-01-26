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

#include "bastile-pkcs11-module.h"

#include "bastile-pkcs11-commands.h"
#include "bastile-pkcs11-source.h"

#include "bastile-mateconf.h"
#include "bastile-util.h"

#include <gck/gck.h>

void
bastile_pkcs11_module_init (void)
{
	BastileSource *source;
	GckModule *module;
	GSList *l, *module_paths;
	GList *slots, *s;
	GError *err = NULL;
	
	/* Load each module in turn, and each slot for each module */
	module_paths = bastile_mateconf_get_string_list ("/system/pkcs11/modules");
	for (l = module_paths; l; l = g_slist_next (l)) {

		module = gck_module_initialize (l->data, NULL, 0, &err);
		if (!module) {
			g_warning ("couldn't initialize %s pkcs11 module: %s", 
			           (gchar*)l->data, err ? err->message : NULL);
			g_clear_error (&err);
			continue;
		}

		slots = gck_module_get_slots (module, FALSE);
		for (s = slots; s; s = g_list_next (s)) {
			source = BASTILE_SOURCE (bastile_pkcs11_source_new (s->data));
			bastile_context_take_source (NULL, source);
		}

		/* These will have been refed by the source above */
		gck_list_unref_free (slots);
		g_object_unref (module);
	}

	bastile_util_string_slist_free (module_paths);

	/* Let these register themselves */
	g_type_class_unref (g_type_class_ref (BASTILE_TYPE_PKCS11_SOURCE));
	g_type_class_unref (g_type_class_ref (BASTILE_PKCS11_TYPE_COMMANDS));
}
