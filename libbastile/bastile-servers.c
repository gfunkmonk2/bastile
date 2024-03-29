/*
 * Bastile
 *
 * Copyright (C) 2008 Stefan Walter
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "bastile-servers.h"
#include "bastile-mateconf.h"

#include "common/bastile-cleanup.h"

#include <string.h>

typedef struct _ServerInfo {
	gchar* type;
	gchar* description;
	BastileValidUriFunc validator;
} ServerInfo;

static GHashTable* server_types = NULL;

static void
free_server_info (gpointer data)
{
	ServerInfo *info = data;
	if (info) {
		g_free (info->type);
		g_free (info->description);
		g_free (info);
	}
}

static void
types_to_slist (gpointer key, gpointer value, gpointer data)
{
	GSList **list = (GSList**)data;
	*list = g_slist_prepend (*list, g_strdup (((ServerInfo*)value)->type));
}

GSList* 
bastile_servers_get_types (void) 
{
	GSList* results = NULL;
	if (server_types)
		g_hash_table_foreach (server_types, types_to_slist, &results);
	return results;
}

gchar* 
bastile_servers_get_description (const char* type) 
{
	ServerInfo* info;
	
	if (!server_types)
		return NULL;
	
	info = g_hash_table_lookup (server_types, type);
	if (info != NULL)
		return g_strdup (info->description);
	
	return NULL;
}


void
bastile_servers_register_type (const char* type, const char* description, 
                                BastileValidUriFunc validate) 
{
	ServerInfo *info;
	
	info = g_new0 (ServerInfo, 1);
	info->description = g_strdup (description);
	info->type = g_strdup (type);
	info->validator = validate;
	
	if (!server_types) {
		server_types = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, free_server_info);
		bastile_cleanup_register ((GDestroyNotify)g_hash_table_destroy, server_types);
	}
	
	g_hash_table_replace (server_types, info->type, info);
}


GSList* 
bastile_servers_get_uris (void) 
{
	GSList* servers, *l;
	gchar *name, *value;
	
	servers = bastile_mateconf_get_string_list (KEYSERVER_KEY);
	
	/* The values are 'uri name', remove the name part */
	for (l = servers; l; l = g_slist_next (l)) {
		value = l->data;
		g_strstrip (value);
		name = strchr (value, ' ');
		if (name)
			*name = 0;
	}
	
	return servers;
}

GSList* 
bastile_servers_get_names (void) 
{
	GSList* servers, *l;
	gchar *name, *value;

	servers = bastile_mateconf_get_string_list (KEYSERVER_KEY);
	
	/* The values are 'uri name', remove the name part */
	for (l = servers; l; l = g_slist_next (l)) {
		value = l->data;
		g_strstrip (value);
		name = strchr (value, ' ');
		if (name) {
			memset (value, ' ', name - value);
			g_strstrip (value);
		}
	}
	
	return servers;
}

/* Check to see if the passed uri is valid against registered validators */
gboolean 
bastile_servers_is_valid_uri (const char* uri) 
{	
	char** parts;
	ServerInfo* info;
	gboolean ret = FALSE;
	
	g_return_val_if_fail (uri != NULL, FALSE);
	
	if (!server_types)
		return FALSE;
	
	parts = g_strsplit (uri, ":", 2);
	if (parts && parts[0]) {
		info = g_hash_table_lookup (server_types, parts[0]);
		if (info && info->validator && (info->validator) (uri))
			ret = TRUE;
	}
	
	g_strfreev (parts);
	return ret;
}
