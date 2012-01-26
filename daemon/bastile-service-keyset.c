/*
 * Bastile
 *
 * Copyright (C) 2005, 2006 Stefan Walter
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

#include "config.h"

#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>

#include "bastile-object.h"
#include "bastile-service.h"
#include "bastile-util.h"

/* Flags for bastile_service_keyset_match_keys */
#define MATCH_KEYS_LOCAL_ONLY       0x00000010

/**
 * SECTION:bastile-service-keyset
 * @short_description: Bastile service Keyset DBus methods. The other DBus
 * methods can be found in other files
 *
 **/

enum {
    KEY_ADDED,
    KEY_REMOVED,
    KEY_CHANGED,
    LAST_SIGNAL
};

G_DEFINE_TYPE (BastileServiceKeyset, bastile_service_keyset, BASTILE_TYPE_SET);
static guint signals[LAST_SIGNAL] = { 0 };

/* -----------------------------------------------------------------------------
 * HELPERS 
 */

/**
* value: the value to free
*
* Clears and frees a pointer containing a GValue
*
**/
static void
value_free (gpointer value)
{
    g_value_unset ((GValue*)value);
    g_free (value);
}

/**
* patterns: an array containing casefold-ed patterns
* value: the haystack. Tries to match patterns in here
*
* Tries to match the patterns. Matched patterns are removed from the array
* Returns: TRUE if one or more pattern matched
**/
static gboolean
match_remove_patterns (GArray *patterns, const gchar *value)
{
    gboolean matched = FALSE;
    gchar *lower;
    int i;
    
    if(value) {
        lower = g_utf8_casefold (value, -1);
        
        /* 
         * Search for each pattern on this key. We try to match
         * as many as possible.
         */
        for (i = 0; i < patterns->len; i++) {
            if (strstr (lower, g_array_index (patterns, gchar*, i)) ? TRUE : FALSE) {
                matched = TRUE;
                g_free (g_array_index (patterns, gchar*, i));
                g_array_remove_index_fast (patterns, i);
                i--;
            }
        }
        
        g_free (lower);
    }
    
    return matched;
}

/* -----------------------------------------------------------------------------
 * DBUS METHODS 
 */

/**
 * bastile_service_keyset_list_keys:
 * @keyset: The BastileServiceKeyset context
 * @keys: a list of keys (out)
 * @error: to return potential errors
 *
 * Returns all stored keys by id
 *
 * Returns: TRUE
 */
gboolean        
bastile_service_keyset_list_keys (BastileServiceKeyset *keyset, gchar ***keys, 
                                   GError **error)
{
	GList *objects, *l;
	GList *children, *k;
	GArray *array;
	gchar *id;

	array = g_array_new (TRUE, TRUE, sizeof (gchar*));

	/* The objects in the set */
	objects = bastile_set_get_objects (BASTILE_SET (keyset));
	for (l = objects; l; l = g_list_next (l)) {
		id = bastile_context_object_to_dbus (SCTX_APP (), BASTILE_OBJECT (l->data));
		if (id == NULL)
			g_warning ("object has no identifier usable over dbus");
		else
			g_array_append_val (array, id);

		/* Children of the object */
		children = bastile_object_get_children (BASTILE_OBJECT (l->data));
		for (k = children; k; k = g_list_next (k)) {
			id = bastile_context_object_to_dbus (SCTX_APP (), BASTILE_OBJECT (k->data));
			if (id == NULL)
				g_warning ("child object has no identifier usable over dbus");
			else
				g_array_append_val (array, id);
		}
	}
    
	*keys = (gchar**)g_array_free (array, FALSE);
	g_list_free (objects);
	return TRUE;
}

/**
 * bastile_service_keyset_get_key_field:
 * @svc: The BastileService context
 * @key: the key to get information for
 * @field: the information field to extract
 * @has: TRUE if value is valid (out)
 * @value: the return value. @field in @key (out)
 * @error: to return potential errors
 *
 * DBus: GetKeyField
 *
 * Extracts the information stored in @field of @key
 *
 * Returns: TRUE on success
 */
gboolean
bastile_service_keyset_get_key_field (BastileService *svc, gchar *key, gchar *field,
                                       gboolean *has, GValue *value, GError **error)
{
    BastileObject *sobj;

    sobj = bastile_context_object_from_dbus (SCTX_APP(), key);
    if (!sobj) {
        g_set_error (error, BASTILE_DBUS_ERROR, BASTILE_DBUS_ERROR_INVALID, 
                     _("Invalid or unrecognized key: %s"), key);
        return FALSE;
    }
 
    if (BASTILE_IS_OBJECT (sobj) && 
        bastile_object_lookup_property (sobj, field, value)) {
        *has = TRUE;
        
    } else {
        *has = FALSE;
        
        /* As close as we can get to 'null' */
        g_value_init (value, G_TYPE_INT);
        g_value_set_int (value, 0);
    }

    return TRUE;
}

/**
 * bastile_service_keyset_get_key_fields:
 * @svc: The BastileService context
 * @key: the key to get information for
 * @fields: the information field to extract
 * @values: a hash table field->value
 * @error: to return potential errors
 *
 * DBus: GetKeyFields
 *
 * Extracts the information stored in @fields of @key and returns a hash table
 *
 * Returns: TRUE on success
 */
gboolean
bastile_service_keyset_get_key_fields (BastileService *svc, gchar *key, gchar **fields,
                                        GHashTable **values, GError **error)
{
	BastileObject *sobj;
	GValue *value;
    
	sobj = bastile_context_object_from_dbus (SCTX_APP(), key);
	if (!sobj) {
		g_set_error (error, BASTILE_DBUS_ERROR, BASTILE_DBUS_ERROR_INVALID, 
		             _("Invalid or unrecognized key: %s"), key);
		return FALSE;
	}

	*values = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, value_free);
    
	if (BASTILE_IS_OBJECT (sobj)) {
		while (*fields) {
			value = g_new0 (GValue, 1);
			if (bastile_object_lookup_property (sobj, *fields, value))
				g_hash_table_insert (*values, g_strdup (*fields), value);
			else
				g_free (value);
			fields++;
		}
	}
	
	return TRUE; 
}

/**
 * bastile_service_keyset_discover_keys:
 * @keyset: The BastileServiceKeyset context
 * @keyids: a list of keyids to import
 * @flags: ignored
 * @keys: the found keys
 * @error: to return potential errors
 *
 * DBus: DiscoverKeys
 *
 * finds and imports the @keyids keys and returns the found @keys
 *
 * Returns: TRUE on success
 */
gboolean        
bastile_service_keyset_discover_keys (BastileServiceKeyset *keyset, const gchar **keyids, 
                                       gint flags, gchar ***keys, GError **error)
{
    GArray *akeys = NULL;
    GList *lkeys, *l;
    GSList *rawids = NULL;
    const gchar **k;
    GQuark keyid;
    gchar *id;
    
    /* Check to make sure the key ids are valid */
    for (k = keyids; *k; k++) {
        keyid = bastile_context_canonize_id (keyset->ktype, *k);
        if (!keyid) {
            g_set_error (error, BASTILE_DBUS_ERROR, BASTILE_DBUS_ERROR_INVALID, 
                         _("Invalid key id: %s"), *k);
            return FALSE;
        }
        rawids = g_slist_prepend (rawids, (char*)(*k));
    }
    
    /* Pass it on to do the work */
    lkeys = bastile_context_discover_objects (SCTX_APP (), keyset->ktype, rawids);
    g_slist_free (rawids);
    
    /* Prepare return value */
    akeys = g_array_new (TRUE, TRUE, sizeof (gchar*));
    for (l = lkeys; l; l = g_list_next (l)) {
        id = bastile_context_object_to_dbus (SCTX_APP(), BASTILE_OBJECT (l->data));
        akeys = g_array_append_val (akeys, id);
    }
    *keys = (gchar**)g_array_free (akeys, FALSE);
    g_list_free (lkeys);
    
    return TRUE;
}

/**
 * bastile_service_keyset_match_keys:
 * @keyset: The BastileServiceKeyset context
 * @patterns: the patterns to match to name and keyid of the keys
 * @flags: MATCH_KEYS_LOCAL_ONLY to do local only lookups
 * @keys: the matched keys
 * @unmatched: the unmatched patterns
 * @error: to return potential errors
 *
 * DBus: MatchKeys
 *
 * Matches name and main id of the key with the list of patterns
 *
 * Returns: TRUE
 */
gboolean
bastile_service_keyset_match_keys (BastileServiceKeyset *keyset, gchar **patterns, 
                                    gint flags, gchar ***keys, gchar***unmatched, 
                                    GError **error)
{
    GArray *results = NULL;
    GArray *remaining = NULL;
    GList *lkeys, *l;
    BastileObject *object;
    const gchar *value;
    gchar *id;
    gchar **k;
    int i;

    /* Copy the keys so we can see what's left. */
    remaining = g_array_new (TRUE, TRUE, sizeof (gchar*));
    for(k = patterns; *k; k++) {
        value = g_utf8_casefold (*k, -1);
        g_array_append_val (remaining, value);
    }
    
    results = g_array_new (TRUE, TRUE, sizeof (gchar*));
    

    lkeys = bastile_context_find_objects (SCTX_APP (), keyset->ktype, BASTILE_USAGE_NONE, 
                                           (flags & MATCH_KEYS_LOCAL_ONLY) ? BASTILE_LOCATION_LOCAL : BASTILE_LOCATION_INVALID);
    for (l = lkeys; l; l = g_list_next(l)) {
        
        if (remaining->len <= 0)
            break;

        if (!BASTILE_IS_OBJECT (l->data))
            continue;
        object = BASTILE_OBJECT (l->data);
        
        /* Note that match_remove_patterns frees value */
        
        /* Main name */        
        value = bastile_object_get_label (object);
        if (match_remove_patterns (remaining, value)) {
            id = bastile_context_object_to_dbus (SCTX_APP (), object);
            g_array_append_val (results, id);
        }
        
        /* Key ID */
        value = bastile_object_get_identifier (object);
        if (match_remove_patterns (remaining, value)) {
            id = bastile_context_object_to_dbus (SCTX_APP (), object);
            g_array_append_val (results, id);
        }
    }
    
    /* Sort results and remove duplicates */
    g_array_sort (results, (GCompareFunc)g_utf8_collate);
    for (i = 0; i < results->len; i++) {
        if (i > 0 && i < results->len) {
            if (strcmp (g_array_index (results, gchar*, i), 
                        g_array_index (results, gchar*, i - 1)) == 0) {
                g_free (g_array_index (results, gchar*, i));
                g_array_remove_index (results, i);
                i--;
            }
        }
    }
    
    *keys = (gchar**)g_array_free (results, FALSE);
    *unmatched = (gchar**)g_array_free (remaining, FALSE);

    return TRUE;
}

/* -----------------------------------------------------------------------------
 * DBUS SIGNALS 
 */

/**
*
* Signal handler for keyset added
*
**/
static void
bastile_service_keyset_added (BastileSet *skset, BastileObject *sobj, 
                               gpointer userdata)
{
	GList *children, *k;
	gchar *id;

	id = bastile_context_object_to_dbus (SCTX_APP (), sobj);
	g_signal_emit (BASTILE_SERVICE_KEYSET (skset), signals[KEY_ADDED], 0, id);
	g_free (id);        

	children = bastile_object_get_children (sobj);
	for (k = children; k; k = g_list_next (k)) {
		id = bastile_context_object_to_dbus (SCTX_APP (), sobj);
		g_signal_emit (BASTILE_SERVICE_KEYSET (skset), signals[KEY_ADDED], 0, id);
		g_free (id);
	}
	
	g_list_free (children);
}

/**
*
* Signal handler for keyset removed
*
**/
static void
bastile_service_keyset_removed (BastileSet *skset, BastileObject *sobj, 
                                 gpointer closure, gpointer userdata)
{
	GList *children, *k;
	gchar *id;

	children = bastile_object_get_children (sobj);
	for (k = children; k; k = g_list_next (k)) {
		id = bastile_context_object_to_dbus (SCTX_APP (), sobj);
		g_signal_emit (BASTILE_SERVICE_KEYSET (skset), signals[KEY_REMOVED], 0, id);
		g_free (id);
	}

	id = bastile_context_object_to_dbus (SCTX_APP (), sobj);
	g_signal_emit (BASTILE_SERVICE_KEYSET (skset), signals[KEY_REMOVED], 0, id);
	g_free (id);        

	g_list_free (children);
}

/**
*
* Signal handler for the changed event
*
**/
static void
bastile_service_keyset_changed (BastileSet *skset, BastileObject *sobj, 
                                 gpointer closure, gpointer userdata)
{
	gchar *id;
    
	id = bastile_context_object_to_dbus (SCTX_APP (), sobj);
	g_signal_emit (BASTILE_SERVICE_KEYSET (skset), signals[KEY_CHANGED], 0, id);
	g_free (id);
}

/* -----------------------------------------------------------------------------
 * OBJECT 
 */

/**
*
* Connects the signals
*
**/
static void
bastile_service_keyset_init (BastileServiceKeyset *keyset)
{
    g_signal_connect_after (keyset, "added", G_CALLBACK (bastile_service_keyset_added), NULL);
    g_signal_connect_after (keyset, "removed", G_CALLBACK (bastile_service_keyset_removed), NULL);
    g_signal_connect_after (keyset, "changed", G_CALLBACK (bastile_service_keyset_changed), NULL);
}

static void
bastile_service_keyset_class_init (BastileServiceKeysetClass *klass)
{
    GObjectClass *gclass;

    gclass = G_OBJECT_CLASS (klass);
    
    signals[KEY_ADDED] = g_signal_new ("key_added", BASTILE_TYPE_SERVICE_KEYSET, 
                G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED, G_STRUCT_OFFSET (BastileServiceKeysetClass, key_added),
                NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
    
    signals[KEY_REMOVED] = g_signal_new ("key_removed", BASTILE_TYPE_SERVICE_KEYSET, 
                G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED, G_STRUCT_OFFSET (BastileServiceKeysetClass, key_removed),
                NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
    
    signals[KEY_CHANGED] = g_signal_new ("key_changed", BASTILE_TYPE_SERVICE_KEYSET, 
                G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED, G_STRUCT_OFFSET (BastileServiceKeysetClass, key_changed),
                NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
}

/* -----------------------------------------------------------------------------
 * PUBLIC METHODS
 */

/**
* bastile_service_keyset_new:
* @ktype: The  key type
* @location: the location of the key
*
*
* Returns: a new keyset of type #BastileSet
*/
BastileSet* 
bastile_service_keyset_new (GQuark ktype, BastileLocation location)
{
    BastileServiceKeyset *skset;
    BastileObjectPredicate *pred = g_new0(BastileObjectPredicate, 1);
    
    pred->tag = ktype;
    pred->location = location;
    
    skset = g_object_new (BASTILE_TYPE_SERVICE_KEYSET, "predicate", pred, NULL);
    g_object_set_data_full (G_OBJECT (skset), "quick-predicate", pred, g_free);
    
    skset->ktype = ktype;
    return BASTILE_SET (skset);
}
