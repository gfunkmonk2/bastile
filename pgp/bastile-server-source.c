/*
 * Bastile
 *
 * Copyright (C) 2004 Stefan Walter
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

#include <config.h>

#include <stdlib.h>
#include <string.h>

#include <glib/gi18n.h>

#include "bastile-operation.h"
#include "bastile-ldap-source.h"
#include "bastile-hkp-source.h"
#include "bastile-server-source.h"
#include "bastile-util.h"
#include "bastile-pgp-key.h"

#include "common/bastile-registry.h"

/**
 * SECTION:bastile-server-source
 * @short_description: Objects for handling of internet sources of keys (hkp/ldap)
 * @include:bastile-server-source.h
 *
 **/

enum {
    PROP_0,
    PROP_SOURCE_TAG,
    PROP_SOURCE_LOCATION,
    PROP_KEY_SERVER,
    PROP_URI
};

/* -----------------------------------------------------------------------------
 *  SERVER SOURCE
 */
 
struct _BastileServerSourcePrivate {
    BastileMultiOperation *mop;
    gchar *server;
    gchar *uri;
};

static void bastile_source_iface (BastileSourceIface *iface);

G_DEFINE_TYPE_EXTENDED (BastileServerSource, bastile_server_source, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (BASTILE_TYPE_SOURCE, bastile_source_iface));

/* GObject handlers */
static void bastile_server_source_dispose    (GObject *gobject);
static void bastile_server_source_finalize   (GObject *gobject);
static void bastile_server_get_property      (GObject *object, guint prop_id,
                                               GValue *value, GParamSpec *pspec);
static void bastile_server_set_property      (GObject *object, guint prop_id,
                                               const GValue *value, GParamSpec *pspec);
                                               
/* BastileSource methods */
static BastileOperation*  bastile_server_source_load (BastileSource *src);

static GObjectClass *parent_class = NULL;


/**
* klass: Class to initialize
*
* Initialize the basic class stuff
*
**/
static void
bastile_server_source_class_init (BastileServerSourceClass *klass)
{
    GObjectClass *gobject_class;
   
    parent_class = g_type_class_peek_parent (klass);
    gobject_class = G_OBJECT_CLASS (klass);
    
    gobject_class->dispose = bastile_server_source_dispose;
    gobject_class->finalize = bastile_server_source_finalize;
    gobject_class->set_property = bastile_server_set_property;
    gobject_class->get_property = bastile_server_get_property;

	g_object_class_override_property (gobject_class, PROP_SOURCE_TAG, "source-tag");
	g_object_class_override_property (gobject_class, PROP_SOURCE_LOCATION, "source-location");
	
    g_object_class_install_property (gobject_class, PROP_KEY_SERVER,
            g_param_spec_string ("key-server", "Key Server",
                                 "Key Server to search on", "",
                                 G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_URI,
            g_param_spec_string ("uri", "Key Server URI",
                                 "Key Server full URI", "",
                                 G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
    
	bastile_registry_register_function (NULL, bastile_pgp_key_canonize_id, "canonize", BASTILE_PGP_STR, NULL);
}

/**
* iface: The #BastileSourceIface to init
*
* Sets the load function in @iface
*
* This is the init function of the interface BASTILE_TYPE_SOURCE
*
**/
static void 
bastile_source_iface (BastileSourceIface *iface)
{
	iface->load = bastile_server_source_load;
}

/**
* ssrc: A #BastileServerSource object
*
* init context, private vars, set prefs, connect signals
*
**/
static void
bastile_server_source_init (BastileServerSource *ssrc)

{
    /* init private vars */
    ssrc->priv = g_new0 (BastileServerSourcePrivate, 1);
    ssrc->priv->mop = bastile_multi_operation_new ();
}


/**
* gobject: A #BastileServerSource object
*
* dispose of all our internal references
*
**/
static void
bastile_server_source_dispose (GObject *gobject)
{
    BastileServerSource *ssrc;
    BastileSource *sksrc;
    
    /*
     * Note that after this executes the rest of the object should
     * still work without a segfault. This basically nullifies the 
     * object, but doesn't free it.
     * 
     * This function should also be able to run multiple times.
     */
  
    ssrc = BASTILE_SERVER_SOURCE (gobject);
    sksrc = BASTILE_SOURCE (gobject);
    g_assert (ssrc->priv);
    
    /* Clear out all the operations */
    if (ssrc->priv->mop) {
        if(bastile_operation_is_running (BASTILE_OPERATION (ssrc->priv->mop)))
            bastile_operation_cancel (BASTILE_OPERATION (ssrc->priv->mop));
        g_object_unref (ssrc->priv->mop);
        ssrc->priv->mop = NULL;
    }
 
    G_OBJECT_CLASS (parent_class)->dispose (gobject);
}


/**
* gobject: A #BastileServerSource object
*
* free private vars
*
**/
static void
bastile_server_source_finalize (GObject *gobject)
{
    BastileServerSource *ssrc;
  
    ssrc = BASTILE_SERVER_SOURCE (gobject);
    g_assert (ssrc->priv);
    
    g_free (ssrc->priv->server);
    g_free (ssrc->priv->uri);
    g_assert (ssrc->priv->mop == NULL);    
    g_free (ssrc->priv);
 
    G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

/**
* object: A BastileServerSource object
* prop_id: The ID of the property to set
* value: The value to set
* pspec: ignored
*
* Properties that can be set:
* PROP_KEY_SERVER, PROP_URI
*
**/
static void 
bastile_server_set_property (GObject *object, guint prop_id, 
                              const GValue *value, GParamSpec *pspec)
{
    BastileServerSource *ssrc = BASTILE_SERVER_SOURCE (object);
 
    switch (prop_id) {
    case PROP_KEY_SERVER:
        g_assert (ssrc->priv->server == NULL);
        ssrc->priv->server = g_strdup (g_value_get_string (value));
        g_return_if_fail (ssrc->priv->server && ssrc->priv->server[0]);
        break;
    case PROP_URI:
        g_assert (ssrc->priv->uri == NULL);
        ssrc->priv->uri = g_strdup (g_value_get_string (value));
        g_return_if_fail (ssrc->priv->uri && ssrc->priv->uri[0]);
        break;
    default:
        break;
    }  
}

/**
* object: A #BastileServerSource object
* prop_id: The id of the property
* value: The value to get
* pspec: ignored
*
* The properties that can be read are:
* PROP_KEY_SERVER, PROP_URI, PROP_SOURCE_TAG, PROP_SOURCE_LOCATION
*
**/
static void 
bastile_server_get_property (GObject *object, guint prop_id, GValue *value,
                              GParamSpec *pspec)
{
    BastileServerSource *ssrc = BASTILE_SERVER_SOURCE (object);
  
    switch (prop_id) {
    case PROP_KEY_SERVER:
        g_value_set_string (value, ssrc->priv->server);
        break;
    case PROP_URI:
        g_value_set_string (value, ssrc->priv->uri);
        break;
    case PROP_SOURCE_TAG:
        g_value_set_uint (value, BASTILE_PGP);
        break;
    case PROP_SOURCE_LOCATION:
        g_value_set_enum (value, BASTILE_LOCATION_REMOTE);
        break;
    }        
}


/* --------------------------------------------------------------------------
 * HELPERS 
 */

/**
 * bastile_server_source_take_operation:
 * @ssrc: the #BastileServerSource to add the @op to
 * @op: add this to the multioperations stored in @ssrc
 *
 *
 *
 */
void
bastile_server_source_take_operation (BastileServerSource *ssrc, BastileOperation *op)
{
    g_return_if_fail (BASTILE_IS_SERVER_SOURCE (ssrc));
    g_return_if_fail (BASTILE_IS_OPERATION (op));
    
    bastile_multi_operation_take (ssrc->priv->mop, op);
}

/* --------------------------------------------------------------------------
 * METHODS
 */

/**
* src: #BastileSource
*
* This function is a stub
*
* Returns NULL
**/
static BastileOperation*
bastile_server_source_load (BastileSource *src)
{
    BastileServerSource *ssrc;
    
    g_assert (BASTILE_IS_SOURCE (src));
    ssrc = BASTILE_SERVER_SOURCE (src);

    /* We should never be called directly */
    return NULL;
}

/**
* uri: the uri to parse
* scheme: the scheme ("http") of this uri
* host: the host part of the uri
*
*
* Code adapted from GnuPG (file g10/keyserver.c)
*
* Returns FALSE if the separation failed
**/
static gboolean 
parse_keyserver_uri (char *uri, const char **scheme, const char **host)
{
    int assume_ldap = 0;

    g_assert (uri != NULL);
    g_assert (scheme != NULL && host != NULL);

    *scheme = NULL;
    *host = NULL;

    /* Get the scheme */

    *scheme = strsep(&uri, ":");
    if (uri == NULL) {
        /* Assume LDAP if there is no scheme */
        assume_ldap = 1;
        uri = (char*)*scheme;
        *scheme = "ldap";
    }

    if (assume_ldap || (uri[0] == '/' && uri[1] == '/')) {
        /* Two slashes means network path. */

        /* Skip over the "//", if any */
        if (!assume_ldap)
            uri += 2;

        /* Get the host */
        *host = strsep (&uri, "/");
        if (*host[0] == '\0')
            return FALSE;
    }

    if (*scheme[0] == '\0')
        return FALSE;

    return TRUE;
}

/**
 * bastile_server_source_new:
 * @server: The server uri to create an object for
 *
 * Creates a #BastileServerSource object out of @server. Depending
 * on the defines at compilation time other sources are supported
 * (ldap, hkp)
 *
 * Returns: A new BastileServerSource or NULL
 */
BastileServerSource* 
bastile_server_source_new (const gchar *server)
{
    BastileServerSource *ssrc = NULL;
    const gchar *scheme;
    const gchar *host;
    gchar *uri, *t;
    
    g_return_val_if_fail (server && server[0], NULL);
    
    uri = g_strdup (server);
        
    if (!parse_keyserver_uri (uri, &scheme, &host)) {
        g_warning ("invalid uri passed: %s", server);

        
    } else {
        
#ifdef WITH_LDAP       
        /* LDAP Uris */ 
        if (g_ascii_strcasecmp (scheme, "ldap") == 0) 
            ssrc = BASTILE_SERVER_SOURCE (bastile_ldap_source_new (server, host));
        else
#endif /* WITH_LDAP */
        
#ifdef WITH_HKP
        /* HKP Uris */
        if (g_ascii_strcasecmp (scheme, "hkp") == 0) {
            
            ssrc = BASTILE_SERVER_SOURCE (bastile_hkp_source_new (server, host));

        /* HTTP Uris */
        } else if (g_ascii_strcasecmp (scheme, "http") == 0 ||
                   g_ascii_strcasecmp (scheme, "https") == 0) {

            /* If already have a port */
            if (strchr (host, ':')) 
	            ssrc = BASTILE_SERVER_SOURCE (bastile_hkp_source_new (server, host));

            /* No port make sure to use defaults */
            else {
                t = g_strdup_printf ("%s:%d", host, 
                                     (g_ascii_strcasecmp (scheme, "http") == 0) ? 80 : 443);
                ssrc = BASTILE_SERVER_SOURCE (bastile_hkp_source_new (server, t));
                g_free (t);
            }

        } else
#endif /* WITH_HKP */
        
            g_warning ("unsupported key server uri scheme: %s", scheme);
    }
    
    g_free (uri);
    return ssrc;
}
