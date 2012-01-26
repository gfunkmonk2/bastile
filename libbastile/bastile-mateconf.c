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
 
#include <stdlib.h>
 
#include "bastile-mateconf.h"

static MateConfClient *global_mateconf_client = NULL;

/**
 * global_client_free:
 *
 * Removes the registered dirs and frees the global mateconf client
 *
 */
static void
global_client_free (void)
{
    if (global_mateconf_client == NULL)
        return;
    
    mateconf_client_remove_dir (global_mateconf_client, BASTILE_DESKTOP_KEYS, NULL);
    mateconf_client_remove_dir (global_mateconf_client, BASTILE_SCHEMAS, NULL);
    
    g_object_unref (global_mateconf_client);
    global_mateconf_client = NULL;
}

/**
 * handle_error:
 * @error:the error to report
 *
 * The error is reported as a warning (#g_warning) and freed afterwards.
 *
 * Returns: TRUE if an error has been handled, FALSE else
 */
static gboolean
handle_error (GError **error)
{
    g_return_val_if_fail (error != NULL, FALSE);

    if (*error != NULL) {
        g_warning ("MateConf error:\n  %s", (*error)->message);
        g_error_free (*error);
        *error = NULL;

        return TRUE;
    }

    return FALSE;
}

/**
 * get_global_client:
 *
 * Initializes the global mateconf client if needed. Can be called with a client
 * already initialized.
 *
 * Returns: The global MateConfClient
 */
static MateConfClient*
get_global_client (void)
{
    GError *error = NULL;
    
    /* Initialize mateconf if needed */
    if (!mateconf_is_initialized ()) {
        char *argv[] = { "bastile", NULL };
        
        if (!mateconf_init (1, argv, &error)) {
            if (handle_error (&error))
                return NULL;
        }
    }
    
    if (global_mateconf_client == NULL) {
        global_mateconf_client = mateconf_client_get_default ();

        if (global_mateconf_client) {
            mateconf_client_add_dir (global_mateconf_client, BASTILE_DESKTOP_KEYS, 
                                  MATECONF_CLIENT_PRELOAD_NONE, &error);
            handle_error (&error);
            mateconf_client_add_dir (global_mateconf_client, BASTILE_SCHEMAS, 
                                  MATECONF_CLIENT_PRELOAD_NONE, &error);
            handle_error (&error);
        }

        atexit (global_client_free);
    }
    
    return global_mateconf_client;
}

/**
 * bastile_mateconf_disconnect:
 *
 * Remove registered dirs and free the global mateconf client
 *
 */
void
bastile_mateconf_disconnect ()
{
    global_client_free ();
}

/**
 * bastile_mateconf_set_boolean:
 * @key: The key
 * @boolean_value: The value to set
 *
 * Set a value for a boolean key in the MateConf storage
 *
 * Errors are handled internally and are mapped to a warning
 */
void
bastile_mateconf_set_boolean (const char *key, gboolean boolean_value)
{
	MateConfClient *client;
	GError *error = NULL;
	
	g_return_if_fail (key != NULL);

	client = get_global_client ();
	g_return_if_fail (client != NULL);
	
	mateconf_client_set_bool (client, key, boolean_value, &error);
	handle_error (&error);
}

/**
 * bastile_mateconf_get_boolean:
 * @key: The key to read
 *
 * Read a boolean key from the MateConf storage
 *
 * Returns: It returns the read value. On error it returns FALSE
 */
gboolean
bastile_mateconf_get_boolean (const char *key)
{
	gboolean result;
	MateConfClient *client;
	GError *error = NULL;
	
	g_return_val_if_fail (key != NULL, FALSE);
	
	client = get_global_client ();
	g_return_val_if_fail (client != NULL, FALSE);
	
	result = mateconf_client_get_bool (client, key, &error);
	return handle_error (&error) ? FALSE : result;
}

/**
 * bastile_mateconf_set_integer:
 * @key: The key
 * @int_value: The value to set
 *
 * Set a value for an integer key in the MateConf storage
 *
 * Errors are handled internally and are mapped to a warning
 */
void
bastile_mateconf_set_integer (const char *key, int int_value)
{
	MateConfClient *client;
	GError *error = NULL;

	g_return_if_fail (key != NULL);

	client = get_global_client ();
	g_return_if_fail (client != NULL);

	mateconf_client_set_int (client, key, int_value, &error);
	handle_error (&error);
}

/**
 * bastile_mateconf_get_integer:
 * @key: The key to read
 *
 * Read an integer key from the MateConf storage
 *
 * Returns: It returns the read value. On error it returns 0.
 */
int
bastile_mateconf_get_integer (const char *key)
{
	int result;
	MateConfClient *client;
	GError *error = NULL;
	
	g_return_val_if_fail (key != NULL, 0);
	
	client = get_global_client ();
	g_return_val_if_fail (client != NULL, 0);
	
	result = mateconf_client_get_int (client, key, &error);
    return handle_error (&error) ? 0 : result;
}


/**
 * bastile_mateconf_set_string:
 * @key: The key
 * @string_value: The value to set
 *
 * Set a value for a string key in the MateConf storage
 *
 * Errors are handled internally and are mapped to a warning
 */
void
bastile_mateconf_set_string (const char *key, const char *string_value)
{
	MateConfClient *client;
	GError *error = NULL;

	g_return_if_fail (key != NULL);

	client = get_global_client ();
	g_return_if_fail (client != NULL);
	
	mateconf_client_set_string (client, key, string_value, &error);
	handle_error (&error);
}

/**
 * bastile_mateconf_get_string:
 * @key: The key to read
 *
 * Read a string key from the MateConf storage
 *
 * Returns: It returns the read value. On error it returns "". The returned string should be 
 * freed with #g_free when no longer needed.
 */
char *
bastile_mateconf_get_string (const char *key)
{
	char *result;
	MateConfClient *client;
	GError *error = NULL;
	
	g_return_val_if_fail (key != NULL, NULL);
	
	client = get_global_client ();
	g_return_val_if_fail (client != NULL, NULL);
	
	result = mateconf_client_get_string (client, key, &error);
	return handle_error (&error) ? g_strdup ("") : result;
}

/**
 * bastile_mateconf_set_string_list:
 * @key: The key
 * @slist: The list to set
 *
 * Set a value for a string list (linked list of strings) key in the MateConf storage
 *
 * Errors are handled internally and are mapped to a warning
 */
void
bastile_mateconf_set_string_list (const char *key, const GSList *slist)
{
	MateConfClient *client;
	GError *error = NULL;

	g_return_if_fail (key != NULL);

	client = get_global_client ();
	g_return_if_fail (client != NULL);

	mateconf_client_set_list (client, key, MATECONF_VALUE_STRING,
			               (GSList *) slist, &error);
	handle_error (&error);
}

/**
 * bastile_mateconf_get_string_list:
 * @key: The key to read
 *
 * Read a string list (linked list of strings) key from the MateConf storage
 *
 * Returns: It returns the read value. On error it returns NULL. Each returned
 * string should be freed with #g_free when no longer needed. The list must be
 * freed with #g_slist_free
 */
GSList*
bastile_mateconf_get_string_list (const char *key)
{
	GSList *slist;
	MateConfClient *client;
	GError *error = NULL;
	
	g_return_val_if_fail (key != NULL, NULL);
	
	client = get_global_client ();
	g_return_val_if_fail (client != NULL, NULL);
	
	slist = mateconf_client_get_list (client, key, MATECONF_VALUE_STRING, &error);
    return handle_error (&error) ? NULL : slist;
}

/**
 * bastile_mateconf_get_entry:
 * @key: The key to read
 *
 * Get an entry key from the MateConf storage
 *
 * Returns: It returns the read value. On error it returns NULL. Must be freed
 * with #mateconf_entry_free
 */
MateConfEntry*
bastile_mateconf_get_entry (const char *key)
{
    MateConfEntry *entry;
    MateConfClient *client;
    GError *error = NULL;
    
    g_return_val_if_fail (key != NULL, NULL);
    
    client = get_global_client ();
    g_return_val_if_fail (client != NULL, NULL);

    entry = mateconf_client_get_entry (client, key, NULL, TRUE, &error);
    return handle_error (&error) ? NULL : entry;
}

/**
 * bastile_mateconf_notify:
 * @key: The key to add the notification for
 * @notification_callback: The callback function to add
 * @callback_data: user data to pass to the callback function
 *
 * Returns: The connection ID. On error it returns 0
 */
guint
bastile_mateconf_notify (const char *key, MateConfClientNotifyFunc notification_callback,
				       gpointer callback_data)
{
	guint notification_id;
	MateConfClient *client;
	GError *error = NULL;
	
	g_return_val_if_fail (key != NULL, 0);
	g_return_val_if_fail (notification_callback != NULL, 0);

	client = get_global_client ();
	g_return_val_if_fail (client != NULL, 0);
	
	notification_id = mateconf_client_notify_add (client, key, notification_callback,
						                       callback_data, NULL, &error);
	
	if (handle_error (&error)) {
		if (notification_id != 0) {
			mateconf_client_notify_remove (client, notification_id);
			notification_id = 0;
		}
	}
	
	return notification_id;
}

/**
 * internal_mateconf_unnotify:
 * @data: Casts the pointer to an integer ID and removes the notification with
 * this ID
 *
 */
static void
internal_mateconf_unnotify (gpointer data)
{
    guint notify_id = GPOINTER_TO_INT (data);
    bastile_mateconf_unnotify (notify_id);
}

/**
 * bastile_mateconf_notify_lazy:
 * @key: The key to add the notification for
 * @notification_callback: The callback function to add
 * @callback_data: user data to pass to the callback function
 * @lifetime: a GObject to bind the callback to
 *
 * The callback is bound to the object (#g_object_set_data_full) and
 * the notification is automatically unnotified as soon as the object is
 * destroyed
 *
 */
void
bastile_mateconf_notify_lazy (const char *key, MateConfClientNotifyFunc notification_callback,
                            gpointer callback_data, gpointer lifetime)
{
    guint notification_id;
    gchar *t;
    
    notification_id = bastile_mateconf_notify (key, notification_callback, callback_data);
    if (notification_id != 0) {
        t = g_strdup_printf ("_mateconf-notify-lazy-%d", notification_id);
        g_object_set_data_full (G_OBJECT (lifetime), t, 
                GINT_TO_POINTER (notification_id), internal_mateconf_unnotify);
    }
}

/**
 * bastile_mateconf_unnotify:
 * @notification_id: ID to remove
 *
 * Removes a notification identified by the ID
 */
void
bastile_mateconf_unnotify (guint notification_id)
{
	MateConfClient *client;

	if (notification_id == 0)
		return;
	
	client = get_global_client ();
	g_return_if_fail (client != NULL);

	mateconf_client_notify_remove (client, notification_id);
}
