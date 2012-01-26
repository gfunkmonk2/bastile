/*
 * Bastile
 *
 * Copyright (C) 2005 Stefan Walter
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

#include <dbus/dbus-glib-bindings.h>

#include "bastile-context.h"
#include "bastile-daemon.h"
#include "bastile-libdialogs.h"
#include "bastile-object.h"
#include "bastile-service.h"
#include "bastile-source.h"
#include "bastile-util.h"

#if WITH_PGP
#include "pgp/bastile-pgp.h"
#include "pgp/bastile-gpgme-source.h"
#include "pgp/bastile-gpgme-dialogs.h"
#include "pgp/bastile-gpgme-key-op.h"
#endif

#include <gio/gio.h>

#define KEYSET_PATH "/org/mate/bastile/keys/%s"
#define KEYSET_PATH_LOCAL "/org/mate/bastile/keys/%s/local"

G_DEFINE_TYPE (BastileService, bastile_service, G_TYPE_OBJECT);

/**
 * SECTION:bastile-service
 * @short_description: Bastile service DBus methods. The other DBus methods can
 * be found in other files
 * @include:bastile-service.h
 *
 **/

/**
* type: a string (gchar)
* dummy: not used
* a: an array (GArray)
*
* Adds a copy of "type" to "a"
*
*/
static void
copy_to_array (const gchar *type, gpointer dummy, GArray *a)
{
    gchar *v = g_strdup (type);
    g_array_append_val (a, v);
}

/**
* svc: the bastile context
* ktype: the key source to add
*
* Adds DBus ids for the keysets. The keysets "ktype" of the service will be set to the
* local keyset.
*
*/
static void 
add_key_source (BastileService *svc, GQuark ktype)
{
    const gchar *keytype = g_quark_to_string (ktype);
    BastileSet *keyset;
    gchar *dbus_id;
    
    /* Check if we have a keyset for this key type, and add if not */
    if (svc->keysets && !g_hash_table_lookup (svc->keysets, keytype)) {

        /* Keyset for all keys */
        keyset = bastile_service_keyset_new (ktype, BASTILE_LOCATION_INVALID);
        dbus_id = g_strdup_printf (KEYSET_PATH, keytype);
        dbus_g_connection_register_g_object (bastile_dbus_server_get_connection (),
                                             dbus_id, G_OBJECT (keyset));    
        g_free (dbus_id);
        
        /* Keyset for local keys */
        keyset = bastile_service_keyset_new (ktype, BASTILE_LOCATION_LOCAL);
        dbus_id = g_strdup_printf (KEYSET_PATH_LOCAL, keytype);
        dbus_g_connection_register_g_object (bastile_dbus_server_get_connection (),
                                             dbus_id, G_OBJECT (keyset));    
        g_free (dbus_id);
        
        g_hash_table_replace (svc->keysets, g_strdup (keytype), keyset);
    }
}

/* -----------------------------------------------------------------------------
 * DBUS METHODS 
 */

/**
* bastile_service_get_key_types:
* @svc: the bastile context
* @ret: the supported keytypes of bastile
* @error: an error var, not used
*
* DBus: GetKeyTypes
*
* Returns all available keytypes
*
* Returns: True
*/
gboolean 
bastile_service_get_key_types (BastileService *svc, gchar ***ret, 
                                GError **error)
{
    GArray *a;
    
    if (svc->keysets) {
        a = g_array_new (TRUE, TRUE, sizeof (gchar*));
        g_hash_table_foreach (svc->keysets, (GHFunc)copy_to_array, a);
        *ret = (gchar**)g_array_free (a, FALSE);
        
    /* No keysets */
    } else {
        *ret = (gchar**)g_new0 (gchar*, 1);
    }
    
    return TRUE;
}

/**
* bastile_service_get_keyset:
* @svc: the bastile context
* @ktype: the type of the key
* @path: the path to the keyset
* @error: set if the key type was not found
*
* DBus: GetKeyset
*
* Returns: False if there is no matching keyset, True else
*/
gboolean
bastile_service_get_keyset (BastileService *svc, gchar *ktype, 
                             gchar **path, GError **error)
{
    if (!g_hash_table_lookup (svc->keysets, ktype)) {
        g_set_error (error, BASTILE_DBUS_ERROR, BASTILE_DBUS_ERROR_INVALID, 
                     _("Invalid or unrecognized key type: %s"), ktype);
        return FALSE;
    }
    
    *path = g_strdup_printf (KEYSET_PATH, ktype);
    return TRUE;
}


/**
* bastile_service_generate_credentials:
* @svc: the bastile context
* @ktype: the keytype (example: "openpgp")
* @values: key-value pairs
* @error: the error
*
* DBus: GenerateCredentials
*
* Generates credentials. Will pop up the data input window (name, email, comment)
* pre-filled with the supplied data. A password dialog will be next. After that
* the key is created.
*
* Returns: True on success
*/
gboolean
bastile_service_generate_credentials (BastileService *svc, gchar *ktype,
                                       GHashTable *values, GError **error)
{
    BastileSource *sksrc;
    GValue  val={0};
    GValue  *pval=NULL;
    gchar   *name=NULL;
    gchar   *email=NULL;
    gchar   *comment=NULL;

    #if WITH_PGP
        sksrc = bastile_context_find_source (bastile_context_for_app (),
                                              BASTILE_PGP_TYPE,
                                              BASTILE_LOCATION_LOCAL);
        g_return_val_if_fail (sksrc != NULL, FALSE);

        pval = &val;

        if (g_strcmp0 (ktype,"openpgp")==0) {
            
                pval = (GValue *)g_hash_table_lookup (values,"name");
                if ((pval) && (G_VALUE_TYPE (pval) == G_TYPE_STRING))
                    name=g_value_dup_string (pval);

                pval = g_hash_table_lookup (values,"email");
                if ((pval) && (G_VALUE_TYPE (pval) == G_TYPE_STRING))
                    email=g_value_dup_string (pval);

                pval = g_hash_table_lookup (values,"comment");
                if ((pval) && (G_VALUE_TYPE (pval) == G_TYPE_STRING))
                    comment=g_value_dup_string (pval);

                bastile_gpgme_generate_key(BASTILE_GPGME_SOURCE (sksrc),
                                            name, email, comment, DSA_ELGAMAL, 2048,0);

                g_free (name);
                name = NULL;
                g_free (email);
                email = NULL;
                g_free (comment);
                comment = NULL;
            
        }
        else {
            g_set_error (error, BASTILE_DBUS_ERROR, BASTILE_DBUS_ERROR_INVALID,
                         _("This keytype is not supported: %s"), ktype);
            return FALSE;
        }

        return TRUE;
    #else
        g_set_error (error, BASTILE_DBUS_ERROR, BASTILE_DBUS_ERROR_INVALID,
                 _("Support for this feature was not enabled at build time"), NULL);
        return FALSE;
    #endif
}


/**
* bastile_service_import_keys:
* @svc: the bastile context
* @ktype: the keytype (example: "openpgp")
* @data: ASCII armored key data (one or more keys)
* @keys: the keys that have been imorted (out)
* @error: the error
*
* DBus: ImportKeys
*
* Imports a buffer containing one or more keys. Returns the keyids
*
* Returns: True on success
*/
gboolean
bastile_service_import_keys (BastileService *svc, gchar *ktype, 
                              gchar *data, gchar ***keys, GError **error)
{
    BastileSource *sksrc;
    BastileOperation *op;
    GInputStream *input;
    GArray *a;
    GList *l;
    gchar *t;
    guint keynum = 0;
    
    sksrc = bastile_context_find_source (SCTX_APP (), g_quark_from_string (ktype), 
                                          BASTILE_LOCATION_LOCAL);
    if (!sksrc) {
        g_set_error (error, BASTILE_DBUS_ERROR, BASTILE_DBUS_ERROR_INVALID, 
                     _("Invalid or unrecognized key type: %s"), ktype);
        return FALSE;
    }
    
	/* TODO: We should be doing base64 on these */
	input = g_memory_input_stream_new_from_data (data, strlen (data), NULL);
	g_return_val_if_fail (input, FALSE);
    
    op = bastile_source_import (sksrc, G_INPUT_STREAM (input));
    bastile_operation_wait (op);
    
    a = g_array_new (TRUE, TRUE, sizeof (gchar*));
    for (l = (GList*)bastile_operation_get_result (op); l; l = g_list_next (l)) {
        t = bastile_context_id_to_dbus (SCTX_APP (), 
                                bastile_object_get_id (BASTILE_OBJECT (l->data)));
        g_array_append_val (a, t);
        keynum = keynum + 1;
    }
        
    *keys = (gchar**)g_array_free (a, FALSE);
    
	if (keynum > 0)
		bastile_notify_import (keynum, *keys);
    
	g_object_unref (op);
	g_object_unref (input);

	return TRUE;
}

/**
* bastile_service_export_keys:
* @svc: the bastile context
* @ktype: the keytype (example: "openpgp")
* @keys: the keys to export (keyids)
* @data: ASCII armored key data (one or more keys) (out)
* @error: the error
*
* DBus: ExportKeys
*
* Exports keys. Keys to export are defined by keyid. The result is a buffer
* containing ASCII armored keys
*
* Returns: True on success
*/
gboolean
bastile_service_export_keys (BastileService *svc, gchar *ktype,
                              gchar **keys, gchar **data, GError **error)
{
    BastileSource *sksrc;
    BastileOperation *op;
    BastileObject *sobj;
    GMemoryOutputStream *output;
    GList *next;
    GList *l = NULL;
    GQuark type;
    
    type = g_quark_from_string (ktype);
    
    while (*keys) {
        sobj = bastile_context_object_from_dbus (SCTX_APP (), *keys);
        
        if (!sobj || bastile_object_get_tag (sobj) != type) {
            g_set_error (error, BASTILE_DBUS_ERROR, BASTILE_DBUS_ERROR_INVALID, 
                         _("Invalid or unrecognized key: %s"), *keys);
            return FALSE;
        }
        
        l = g_list_prepend (l, sobj);
        keys++;
    }    

    output = G_MEMORY_OUTPUT_STREAM (g_memory_output_stream_new (NULL, 0, g_realloc, NULL));
    g_return_val_if_fail (output, FALSE);
    
    /* Sort by key source */
    l = bastile_util_objects_sort (l);
    
    while (l) {
     
        /* Break off one set (same keysource) */
        next = bastile_util_objects_splice (l);
        
        sobj = BASTILE_OBJECT (l->data);

        /* Export from this key source */        
        sksrc = bastile_object_get_source (sobj);
        g_return_val_if_fail (sksrc != NULL, FALSE);
        
        /* We pass our own data object, to which data is appended */
        op = bastile_source_export (sksrc, l, G_OUTPUT_STREAM (output));
        g_return_val_if_fail (op != NULL, FALSE);

        g_list_free (l);
        l = next;
        
        bastile_operation_wait (op);
    
        if (!bastile_operation_is_successful (op)) {

            /* Ignore the rest, break loop */
            g_list_free (l);
            
            bastile_operation_copy_error (op, error);
            g_object_unref (op);
            
            g_object_unref (output);
            return FALSE;
        }        
        
        g_object_unref (op);
    } 
    
    /* TODO: We should be base64 encoding this */
    *data = g_memory_output_stream_get_data (output);
    
    /* The above pointer is not freed, because we passed null to g_memory_output_stream_new() */
    g_object_unref (output);
    return TRUE;
}

/**
* bastile_service_display_notification:
* @svc: the bastile context
* @heading: the heading of the notification
* @text: the text of the notification
* @icon: the icon of the notification
* @urgent: set to TRUE if the message is urgent
* @error: the error
*
* DBus: DisplayNotification
*
* Displays a notification
*
* Returns: True
*/
gboolean
bastile_service_display_notification (BastileService *svc, gchar *heading,
                                       gchar *text, gchar *icon, gboolean urgent, 
                                       GError **error)
{
    if (!icon || !icon[0])
        icon = NULL;
    
    bastile_notification_display (heading, text, urgent, icon, NULL);
    return TRUE;
}

#if 0

gboolean
bastile_service_match_save (BastileService *svc, gchar *ktype, gint flags, 
                             gchar **patterns, gchar **keys, GError **error)
{
    /* TODO: Implement match keys */
    g_set_error (error, BASTILE_DBUS_ERROR, BASTILE_DBUS_ERROR_NOTIMPLEMENTED, "TODO");
    return FALSE;    
}

#endif /* 0 */

/* -----------------------------------------------------------------------------
 * SIGNAL HANDLERS 
 */

/**
* sctx: ignored
* sobj: bastile object
* svc: bastile service
*
* Handler to update added key sources
*
*/
static void
bastile_service_added (BastileContext *sctx, BastileObject *sobj, BastileService *svc)
{
    GQuark ktype = bastile_object_get_tag (sobj);
    add_key_source (svc, ktype);
}

/**
* sctx: ignored
* sobj: bastile object
* svc: bastile service
*
* Handler to update changing key sources
*
*/
static void
bastile_service_changed (BastileContext *sctx, BastileObject *sobj, BastileService *svc)
{
    /* Do the same thing as when a key is added */
    GQuark ktype = bastile_object_get_tag (sobj);
    add_key_source (svc, ktype);
}

/* -----------------------------------------------------------------------------
 * OBJECT 
 */

/**
* gobject:
*
* Disposes the bastile dbus service
*
*/
static void
bastile_service_dispose (GObject *gobject)
{
    BastileService *svc = BASTILE_SERVICE (gobject);
    
    if (svc->keysets)
        g_hash_table_destroy (svc->keysets);
    svc->keysets = NULL;
    
    g_signal_handlers_disconnect_by_func (SCTX_APP (), bastile_service_added, svc);
    g_signal_handlers_disconnect_by_func (SCTX_APP (), bastile_service_changed, svc);
    
    G_OBJECT_CLASS (bastile_service_parent_class)->dispose (gobject);
}

/**
* klass: The class to init
*
*
*/
static void
bastile_service_class_init (BastileServiceClass *klass)
{
    GObjectClass *gobject_class;
   
    bastile_service_parent_class = g_type_class_peek_parent (klass);
    gobject_class = G_OBJECT_CLASS (klass);
    
    gobject_class->dispose = bastile_service_dispose;
}


/**
* svc:
*
*
* Initialises the service, adds local key sources and connects the signals.
*
*/
static void
bastile_service_init (BastileService *svc)
{
    GSList *srcs, *l;
    
    /* We keep around a keyset for each keytype */
    svc->keysets = g_hash_table_new_full (g_str_hash, g_str_equal, 
                                          g_free, g_object_unref);
    
    /* Fill in keysets for any keys already in the context */
    srcs = bastile_context_find_sources (SCTX_APP (), BASTILE_TAG_INVALID, BASTILE_LOCATION_LOCAL);
    for (l = srcs; l; l = g_slist_next (l)) 
        add_key_source (svc, bastile_source_get_tag (BASTILE_SOURCE (l->data)));
    g_slist_free (srcs);
    
    /* And now listen for new key types */
    g_signal_connect (SCTX_APP (), "added", G_CALLBACK (bastile_service_added), svc);
    g_signal_connect (SCTX_APP (), "changed", G_CALLBACK (bastile_service_changed), svc);
}
