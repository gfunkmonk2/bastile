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
#include "bastile-service.h"

#include "bastile-service-bindings.h"
#include "bastile-service-keyset-bindings.h"
#ifdef WITH_PGP
#include "bastile-service-crypto-bindings.h"
#endif

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>

#define APPLICATION_SERVICE_NAME "org.mate.bastile"

/* The main service objects */
BastileService *the_service = NULL;
#ifdef WITH_PGP
BastileServiceCrypto *the_crypto = NULL;
#endif
DBusGConnection *connection = NULL;

/**
* bastile_dbus_server_init:
*
* Initialisation for the dbus service.
*
*/
void 
bastile_dbus_server_init ()
{
    DBusGProxy *driver_proxy;
    GError *err = NULL;
    guint request_name_result;
    
    g_return_if_fail (connection == NULL);
    g_return_if_fail (the_service == NULL);

    connection = dbus_g_bus_get (DBUS_BUS_STARTER, &err);
    if (connection == NULL) {
        g_warning ("DBUS Service registration failed: %s", err ? err->message : "");
        g_error_free (err);
        return;
    }
    
    dbus_connection_set_exit_on_disconnect (dbus_g_connection_get_connection (connection), FALSE);

    driver_proxy = dbus_g_proxy_new_for_name (connection, DBUS_SERVICE_DBUS,
                                              DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);

    if (!org_freedesktop_DBus_request_name (driver_proxy, APPLICATION_SERVICE_NAME,
                                            0, &request_name_result, &err)) {
        g_warning ("DBUS Service name request failed.");
        g_clear_error (&err);
    }

    if (request_name_result == DBUS_REQUEST_NAME_REPLY_EXISTS) {
        g_warning ("DBUS Service already started elsewhere");
        return;
    }

       
    dbus_g_object_type_install_info (BASTILE_TYPE_SERVICE, 
                                     &dbus_glib_bastile_service_object_info);
    
    dbus_g_object_type_install_info (BASTILE_TYPE_SERVICE_KEYSET, 
                                     &dbus_glib_bastile_service_keyset_object_info);

    the_service = g_object_new (BASTILE_TYPE_SERVICE, NULL);
    dbus_g_connection_register_g_object (connection, "/org/mate/bastile/keys",
                                         G_OBJECT (the_service));

#ifdef WITH_PGP
    dbus_g_object_type_install_info (BASTILE_TYPE_SERVICE_CRYPTO,
                                     &dbus_glib_bastile_service_crypto_object_info);
    
    the_crypto = g_object_new (BASTILE_TYPE_SERVICE_CRYPTO, NULL);
    dbus_g_connection_register_g_object (connection, "/org/mate/bastile/crypto",
                                         G_OBJECT (the_crypto));
#endif 
}

/**
* bastile_dbus_server_get_connection:
*
*
*
* Returns: The DBus connection #DBusGConnection
*/
DBusGConnection*    
bastile_dbus_server_get_connection ()
{
    return connection;
}

/**
* bastile_dbus_server_cleanup:
*
* Cleans up the dbus service
*
*/
void 
bastile_dbus_server_cleanup ()
{
    /* The DBUS Glib bindings (apparently) note when this goes 
       away and unregister it for incoming calls */
    if (the_service)
        g_object_unref (the_service);
    the_service = NULL;
    
#ifdef WITH_PGP
    if (the_crypto)
        g_object_unref (the_crypto);
    the_crypto = NULL;
#endif
    
    if (connection)
        dbus_g_connection_unref (connection);
    connection = NULL;
}
