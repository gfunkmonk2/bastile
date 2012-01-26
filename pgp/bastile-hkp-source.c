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
 
#include <stdlib.h>
#include <string.h>
 
#include <glib/gi18n.h>

#include "bastile-hkp-source.h"

#include "bastile-pgp-key.h"
#include "bastile-pgp-subkey.h"
#include "bastile-pgp-uid.h"

#include "bastile-mateconf.h"
#include "bastile-operation.h"
#include "bastile-servers.h"
#include "bastile-util.h"

#include "common/bastile-registry.h"
#include "common/bastile-object-list.h"

#include <libsoup/soup.h>

/**
 * SECTION: bastile-hkp-source
 * @short_description: Implements the HKP (HTTP Keyserver protocol) Source object
 * @include: bastile-hkp-source.h
 *
 * See: http://tools.ietf.org/html/draft-shaw-openpgp-hkp-00
 **/

#ifdef WITH_HKP

/* Override the DEBUG_HKP_ENABLE switch here */
/* #define DEBUG_HKP_ENABLE 1 */

#ifndef DEBUG_HKP_ENABLE
#if _DEBUG
#define DEBUG_HKP_ENABLE 1
#else
#define DEBUG_HKP_ENABLE 0
#endif
#endif

#define PGP_KEY_BEGIN   "-----BEGIN PGP PUBLIC KEY BLOCK-----"
#define PGP_KEY_END     "-----END PGP PUBLIC KEY BLOCK-----"

#define SOUP_MESSAGE_IS_ERROR(msg) \
    (SOUP_STATUS_IS_TRANSPORT_ERROR((msg)->status_code) || \
     SOUP_STATUS_IS_CLIENT_ERROR((msg)->status_code) || \
     SOUP_STATUS_IS_SERVER_ERROR((msg)->status_code))
    
#define HKP_ERROR_DOMAIN (get_hkp_error_domain())

#define MATECONF_USE_HTTP_PROXY    "/system/http_proxy/use_http_proxy"
#define MATECONF_HTTP_PROXY_HOST   "/system/http_proxy/host"
#define MATECONF_PROXY_PORT        "/system/http_proxy/port"
#define MATECONF_USE_AUTH          "/system/http_proxy/use_authentication"
#define MATECONF_AUTH_USER         "/system/http_proxy/authentication_user"
#define MATECONF_AUTH_PASS         "/system/http_proxy/authentication_password"

/**
*
* Returns The GQuark for the HKP error
*
**/
static GQuark
get_hkp_error_domain ()
{
    static GQuark q = 0;
    if(q == 0)
        q = g_quark_from_static_string ("bastile-hkp-error");
    return q;
}

/**
* src: The BastileSource to use as server for the uri
* path: The path to add to the SOUP uri
*
*
*
* Returns A Soup uri with server, port and paths
**/
static SoupURI*
get_http_server_uri (BastileSource *src, const char *path)
{
    SoupURI *uri;
    gchar *server, *port;

    g_object_get (src, "key-server", &server, NULL);
    g_return_val_if_fail (server != NULL, NULL);

    uri = soup_uri_new (NULL);
    soup_uri_set_scheme (uri, SOUP_URI_SCHEME_HTTP);

    /* If it already has a port then use that */
    port = strchr (server, ':');
    if (port) {
        *port++ = '\0';
        soup_uri_set_port (uri, atoi (port));
    } else {
        /* default HKP port */
        soup_uri_set_port (uri, 11371); 
    }

    soup_uri_set_host (uri, server);
    soup_uri_set_path (uri, path);
    g_free (server);

    return uri;
}

/* -----------------------------------------------------------------------------
 *  HKP OPERATION     
 */
 
#define BASTILE_TYPE_HKP_OPERATION            (bastile_hkp_operation_get_type ())
#define BASTILE_HKP_OPERATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_HKP_OPERATION, BastileHKPOperation))
#define BASTILE_HKP_OPERATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_HKP_OPERATION, BastileHKPOperationClass))
#define BASTILE_IS_HKP_OPERATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_HKP_OPERATION))
#define BASTILE_IS_HKP_OPERATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_HKP_OPERATION))
#define BASTILE_HKP_OPERATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_HKP_OPERATION, BastileHKPOperationClass))

typedef gboolean (*OpHKPCallback)   (BastileOperation *op, SoupMessage *message);
    
DECLARE_OPERATION (HKP, hkp)
    BastileHKPSource *hsrc;        /* The source  */
    SoupSession *session;           /* The HTTP session */
    guint total;                    /* Number of requests queued */
    guint requests;                 /* Number of requests remaining */
    gboolean cancelling;            /* Cancelling the request, don't process */
END_DECLARE_OPERATION

IMPLEMENT_OPERATION (HKP, hkp)

/**
* hop: A BastileHKPOperation to init
*
* Reads settings from MateConf and creates a new operation with a running Soup
*
**/
static void 
bastile_hkp_operation_init (BastileHKPOperation *hop)
{
    SoupURI *uri;
    gchar *host;
#if DEBUG_HKP_ENABLE
    SoupLogger *logger;
#endif
    
    if (bastile_mateconf_get_boolean (MATECONF_USE_HTTP_PROXY)) {
        
        host = bastile_mateconf_get_string (MATECONF_HTTP_PROXY_HOST);
        if (host) {
            uri = soup_uri_new (NULL);
            
            if (!uri) {
                g_warning ("creation of SoupURI from '%s' failed", host);
                
            } else {

                soup_uri_set_scheme (uri, SOUP_URI_SCHEME_HTTP);
                soup_uri_set_host (uri, host);
                g_free (host);
                soup_uri_set_port (uri, bastile_mateconf_get_integer (MATECONF_PROXY_PORT));
                
                if (bastile_mateconf_get_boolean (MATECONF_USE_AUTH)) {
                    char *user, *pass;

                    user = bastile_mateconf_get_string (MATECONF_AUTH_USER);
                    soup_uri_set_user (uri, user);
                    g_free (user);
                    pass = bastile_mateconf_get_string (MATECONF_AUTH_PASS);
                    soup_uri_set_password (uri, pass);
                    g_free (pass);
                }
                
                hop->session = soup_session_async_new_with_options (SOUP_SESSION_PROXY_URI, uri, NULL);
                soup_uri_free (uri);
            }
        }
    }
    
    /* Without a proxy */
    if (!hop->session)
        hop->session = soup_session_async_new ();

#if DEBUG_HKP_ENABLE
    logger = soup_logger_new (SOUP_LOGGER_LOG_BODY, -1);
    soup_logger_attach (logger, hop->session);
    g_object_unref (logger);
#endif
}

/**
* gobject: A BastileHKPOperation to dispose
*
*
*
**/
static void 
bastile_hkp_operation_dispose (GObject *gobject)
{
    BastileHKPOperation *hop = BASTILE_HKP_OPERATION (gobject);
    
    if (hop->hsrc) {
        g_object_unref (hop->hsrc);
        hop->hsrc = NULL;
    }

    if (hop->session) {
        g_object_unref (hop->session);
        hop->session = NULL;
    }
    
    G_OBJECT_CLASS (hkp_operation_parent_class)->dispose (gobject);  
}

/**
* gobject: A BastileHKPOperation to finalize
*
*
*
**/
static void 
bastile_hkp_operation_finalize (GObject *gobject)
{
    BastileHKPOperation *hop = BASTILE_HKP_OPERATION (gobject);

    g_assert (hop->hsrc == NULL);
    g_assert (hop->session == NULL);
    
    G_OBJECT_CLASS (hkp_operation_parent_class)->finalize (gobject);  
}

/**
* operation: A BastileHKPOperation
*
* Cancels the operation, aborts the soup session
*
**/
static void 
bastile_hkp_operation_cancel (BastileOperation *operation)
{
    BastileHKPOperation *hop;
    
    g_assert (BASTILE_IS_HKP_OPERATION (operation));
    hop = BASTILE_HKP_OPERATION (operation);
    
    g_return_if_fail (bastile_operation_is_running (operation));
    hop->cancelling = TRUE;
    
    if (hop->session != NULL) 
        soup_session_abort (hop->session);
    
    bastile_operation_mark_done (operation, TRUE, NULL);
}

/* Thanks to GnuPG */
/**
* line: The line to modify
*
*
* Remove anything <between brackets> and de-urlencode in place.  Note
* that this requires all brackets to be closed on the same line.  It
* also means that the result is never larger than the input.
*
**/
static void
dehtmlize(gchar *line)
{
    int parsedindex = 0;
    gchar *parsed = line;
    
    g_return_if_fail (line);

    while (*line != '\0') {
        switch(*line) {
	    case '<':
	        while (*line != '>' && *line != '\0')
	            line++;
	        if (*line!='\0')
	            line++;
	        break;
	    case '&':
	        if ((*(line + 1) != '\0' && g_ascii_tolower (*(line + 1)) == 'l') &&
	            (*(line + 2) != '\0' && g_ascii_tolower (*(line + 2)) == 't') &&
	            (*(line + 3) != '\0' && *(line + 3) == ';')) {
	            parsed[parsedindex++] = '<';
	            line += 4;
	            break;
	        } else if ((*(line + 1) != '\0' && g_ascii_tolower (*(line + 1)) == 'g') &&
		               (*(line + 2) != '\0' && g_ascii_tolower (*(line + 2)) == 't') &&
		               (*(line + 3) != '\0' && *(line + 3) == ';')) {
	            parsed[parsedindex++] = '>';
	            line += 4;
	            break;
	        } else if((*(line + 1) != '\0' && g_ascii_tolower (*(line + 1)) == 'a') &&
		              (*(line + 2) != '\0' && g_ascii_tolower (*(line + 2))=='m') &&
		              (*(line + 3) != '\0' && g_ascii_tolower (*(line + 3))=='p') &&
		              (*(line + 4) != '\0' && *(line + 4) == ';')) {
	            parsed[parsedindex++] = '&';
	            line += 5;
	            break;
	        }
            /* Fall through */
        default:
        	parsed[parsedindex++] = *line;
	        line++;
	        break;
	    };
    }

    parsed[parsedindex] = '\0';

    /* 
     * Chop off any trailing whitespace.  Note that the HKP servers have
     * \r\n as line endings, and the NAI HKP servers have just \n. 
     */

    if(parsedindex > 0) {
        parsedindex--;
        while (g_ascii_isspace(((unsigned char*)parsed)[parsedindex])) {
	        parsed[parsedindex] = '\0';
	        if(parsedindex == 0)
	            break;
	        parsedindex--;
	    }
    }
}

/**
* hop: The running BastileHKPOperation
* msg: The optional Soup error message
* text: The optional error text
*
* Either text or msg is needed
*
* Cancels operation and marks the HKP operation as failed
**/
static void
fail_hkp_operation (BastileHKPOperation *hop, SoupMessage *msg, const gchar *text)
{
    gchar *t, *server;
    GError *error = NULL;
    
    if (!bastile_operation_is_running (BASTILE_OPERATION (hop)))
        return;

    g_object_get (hop->hsrc, "key-server", &server, NULL);

    if (text) {
        error = g_error_new (HKP_ERROR_DOMAIN, msg ? msg->status_code : 0, "%s", text);

    } else if (msg) {
        /* Make the body lower case, and no tags */
        t = g_strndup (msg->response_body->data, msg->response_body->length);
        if (t != NULL) {
            dehtmlize (t);        
            bastile_util_string_lower (t);
        }

        if (t && strstr (t, "no keys"))
            error = NULL; /* not found is not an error */
        else if (t && strstr (t, "too many"))
            error = g_error_new (HKP_ERROR_DOMAIN, 0, _("Search was not specific enough. Server '%s' found too many keys."), server);
        else 
            error = g_error_new (HKP_ERROR_DOMAIN, msg->status_code, _("Couldn't communicate with server '%s': %s"),
                                 server, msg->reason_phrase);
        g_free (t);
    } else {

        /* We should always have msg or text */
        g_assert (FALSE);
    }        

    bastile_operation_mark_done (BASTILE_OPERATION (hop), FALSE, error);
    g_free (server);
}

/**
* hsrc: The BastileHKPSource to set in the new object
*
*
*
* Returns A new BastileHKPOperation
**/
static BastileHKPOperation*
setup_hkp_operation (BastileHKPSource *hsrc)
{
    BastileHKPOperation *hop;

    g_return_val_if_fail (BASTILE_IS_HKP_SOURCE (hsrc), NULL);
    
    hop = g_object_new (BASTILE_TYPE_HKP_OPERATION, NULL);
    hop->hsrc = hsrc;
    g_object_ref (hsrc);

    bastile_operation_mark_start (BASTILE_OPERATION (hop));
    return hop;    
}

/**
 * parse_hkp_date:
 * @text: The date string to parse, YYYY-MM-DD
 *
 *
 *
 * Returns: 0 on error or the timestamp
 */
unsigned int
parse_hkp_date (const gchar *text)
{
    int year, month, day;
    struct tm tmbuf;
    time_t stamp;

    if (strlen (text) != 10 || text[4] != '-' || text[7] != '-')
	    return 0;
    
    /* YYYY-MM-DD */
    sscanf (text, "%4d-%2d-%2d", &year, &month, &day);

    /* some basic checks */
    if (year < 1970 || month < 1 || month > 12 || day < 1 || day > 31)
	    return 0;
    
    memset (&tmbuf, 0, sizeof tmbuf);
    tmbuf.tm_mday = day;
    tmbuf.tm_mon = month - 1;
    tmbuf.tm_year = year - 1900;
    tmbuf.tm_isdst = -1;
    
    stamp = mktime (&tmbuf);
    return stamp == (time_t)-1 ? 0 : stamp;
}

/**
* response: The HKP server response to parse
*
* Extracts the key data from the HKP server response
*
* Returns A GList of keys
**/
static GList*
parse_hkp_index (const gchar *response)
{
	/* 
	 * Luckily enough, both the HKP server and NAI HKP interface to their
	 * LDAP server are close enough in output so the same function can
	 * parse them both. 
	 */

	/* pub  2048/<a href="/pks/lookup?op=get&search=0x3CB3B415">3CB3B415</a> 1998/04/03 David M. Shaw &lt;<a href="/pks/lookup?op=get&search=0x3CB3B415">dshaw@jabberwocky.com</a>&gt; */

	gchar **lines, **l;
	gchar **v;
	gchar *line, *t;
    
	BastilePgpKey *key = NULL;
	GList *keys = NULL;
	GList *subkeys = NULL;
	GList *uids = NULL;
	guint flags;
    
	lines = g_strsplit (response, "\n", 0);
    
	for (l = lines; *l; l++) {

		line = *l;	
		dehtmlize (line);
#if DEBUG_HKP_ENABLE
        fprintf(stderr,"%s\n", line);
#endif

		/* Start a new key */
		if (g_ascii_strncasecmp (line, "pub ", 4) == 0) {
            
			t = line + 4;
			while (*t && g_ascii_isspace (*t))
				t++;
            
			v = g_strsplit_set (t, " ", 3);
			if (!v[0] || !v[1] || !v[2]) {
				g_warning ("Invalid key line from server: %s", line);
                
			} else {
				gchar *fingerprint, *fpr = NULL;
				const gchar *algo;
				gboolean has_uid = TRUE;
				BastilePgpSubkey *subkey;
				
				flags = BASTILE_FLAG_EXPORTABLE;
       	
				/* Cut the length and fingerprint */
				fpr = strchr (v[0], '/');
				if (fpr == NULL) {
					g_warning ("couldn't find key fingerprint in line from server: %s", line);
					fpr = "";
				} else {
					*(fpr++) = 0;
				}
                
				/* Check out the key type */
				switch (g_ascii_toupper (v[0][strlen(v[0]) - 1])) {
				case 'D':
					algo = "DSA";
					break;
				case 'R':
					algo = "RSA";
					break;
				default:
					algo = "";
					break;
				};

				/* Format the date for our parse function */
				g_strdelimit (v[1], "/", '-');
                
				/* Cleanup the UID */
				g_strstrip (v[2]);
            
				if (g_ascii_strcasecmp (v[2], "*** KEY REVOKED ***") == 0) {
					flags |= BASTILE_FLAG_REVOKED;
					has_uid = FALSE;
				} 
				
				if (key) {
					bastile_pgp_key_set_uids (BASTILE_PGP_KEY (key), uids);
					bastile_object_list_free (uids);
					bastile_pgp_key_set_subkeys (BASTILE_PGP_KEY (key), subkeys);
					bastile_object_list_free (subkeys);
					uids = subkeys = NULL;
					key = NULL;
				}

				key = bastile_pgp_key_new ();
				keys = g_list_prepend (keys, key);
		        	g_object_set (key, "location", BASTILE_LOCATION_REMOTE, "flags", 
		        	              flags, NULL);
		        	
				/* Add all the info to the key */
				subkey = bastile_pgp_subkey_new ();
				bastile_pgp_subkey_set_keyid (subkey, fpr);
				fingerprint = bastile_pgp_subkey_calc_fingerprint (fpr);
				bastile_pgp_subkey_set_fingerprint (subkey, fingerprint);
				g_free (fingerprint);
				bastile_pgp_subkey_set_flags (subkey, flags);
				bastile_pgp_subkey_set_created (subkey, parse_hkp_date (v[1]));
				bastile_pgp_subkey_set_length (subkey, strtol (v[0], NULL, 10));
				bastile_pgp_subkey_set_algorithm (subkey, algo);
				subkeys = g_list_prepend (subkeys, subkey);

				/* And the UID if one was found */                
				if (has_uid) {
					BastilePgpUid *uid = bastile_pgp_uid_new (v[2]);
					uids = g_list_prepend (uids, uid);
				}
			}
            
			g_strfreev (v);
            
		/* A UID for the key */
		} else if (key && g_ascii_strncasecmp (line, "    ", 4) == 0) {
            
			BastilePgpUid *uid;
			
			g_strstrip (line);
			uid = bastile_pgp_uid_new (line);
			uids = g_list_prepend (uids, uid);
            
		/* Signatures */
		} else if (key && g_ascii_strncasecmp (line, "sig ", 4) == 0) {
            
			/* TODO: Implement signatures */
            
		} 
	}
    
	g_strfreev (lines);

	if (key) {
		bastile_pgp_key_set_uids (BASTILE_PGP_KEY (key), g_list_reverse (uids));
		bastile_object_list_free (uids);
		bastile_pgp_key_set_subkeys (BASTILE_PGP_KEY (key), g_list_reverse (subkeys));
		bastile_object_list_free (subkeys);
	}
	
	return keys; 
}

/**
* ssrc: The BastileHKPSource to add
* key: The BastilePgpKey to add
*
* Adds the key with the source ssrc to the application context
*
**/
static void
add_key (BastileHKPSource *ssrc, BastilePgpKey *key)
{
	BastileObject *prev;
	GQuark keyid;
       
	keyid = bastile_pgp_key_canonize_id (bastile_pgp_key_get_keyid (key));
	prev = bastile_context_get_object (SCTX_APP (), BASTILE_SOURCE (ssrc), keyid);
	if (prev != NULL) {
		g_return_if_fail (BASTILE_IS_PGP_KEY (prev));
		bastile_pgp_key_set_uids (BASTILE_PGP_KEY (prev), bastile_pgp_key_get_uids (key));
		bastile_pgp_key_set_subkeys (BASTILE_PGP_KEY (prev), bastile_pgp_key_get_subkeys (key));
		return;
	}

	/* Add to context */ 
	bastile_object_set_source (BASTILE_OBJECT (key), BASTILE_SOURCE (ssrc));
	bastile_context_add_object (SCTX_APP (), BASTILE_OBJECT (key));
}

/**
* session: The soup session
* msg: The soup message
* hop: The BastileHKPOperation involved
*
* Called from soup_session_queue_message-when a soup message got completed
* Adds the new keys to the application context.
*
**/
static void 
refresh_callback (SoupSession *session, SoupMessage *msg, BastileHKPOperation *hop) 
{
    GList *keys, *k;
    
    if (hop->cancelling)
        return;
    
    if (SOUP_MESSAGE_IS_ERROR (msg)) {
        fail_hkp_operation (hop, msg, NULL);
        return;
    }
    
    keys = parse_hkp_index (msg->response_body->data);
    
    for (k = keys; k; k = g_list_next (k))
        add_key (hop->hsrc, BASTILE_PGP_KEY (k->data));
    bastile_object_list_free (keys);

    if (--hop->requests <= 0)
        bastile_operation_mark_done (BASTILE_OPERATION (hop), FALSE, NULL);
    else
        bastile_operation_mark_progress_full (BASTILE_OPERATION (hop), _("Searching for keys..."), 
                                               hop->requests, hop->total);
}

/**
* response: The server response
*
* Parses the response and extracts an error message
*
* Free the error message with g_free after usage
*
* Returns NULL if there was no error. The error message else
**/
static gchar*
get_send_result (const gchar *response)
{
    gchar **lines, **l;
    gchar *t;
    gchar *last = NULL;
    gboolean is_error = FALSE;
    
    if (!*response)
        return g_strdup ("");
    
    lines = g_strsplit (response, "\n", 0); 

    for (l = lines; *l; l++) {

        dehtmlize (*l);
        g_strstrip (*l);
        
        if (!(*l)[0])
            continue;

        t = g_ascii_strdown (*l, -1);
        
        /* Look for the word 'error' */
        if (strstr (t, "error"))
            is_error = TRUE;
        
        g_free (t);

        if ((*l)[0])
            last = *l;
    }
    
    /* Use last line as the message */
    last = is_error ? g_strdup (last) : NULL;
    
    g_strfreev (lines);
    
    return last;
}

/**
* session: The soup session the message belongs to
* msg: The message that has finished
* hop: The BastileHKPOperation associated
*
* Callback for soup_session_queue_message from bastile_hkp_source_import
*
**/
static void 
send_callback (SoupSession *session, SoupMessage *msg, BastileHKPOperation *hop) 
{
    gchar *errmsg;

    if (hop->cancelling)
        return;

    if (SOUP_MESSAGE_IS_ERROR (msg)) {
        fail_hkp_operation (hop, msg, NULL);
        return;
    }
    
    errmsg = get_send_result (msg->response_body->data);
    if (errmsg) {
        fail_hkp_operation (hop, NULL, errmsg);
        g_free (errmsg);
        return;
    }
    
    /* A successful status from the server is all we want
       in this case */
    
    if (--hop->requests <= 0)
        bastile_operation_mark_done (BASTILE_OPERATION (hop), FALSE, NULL);
    else
        bastile_operation_mark_progress_full (BASTILE_OPERATION (hop), _("Uploading keys..."), 
                                               hop->requests, hop->total);
}

/**
 * detect_key:
 * @text: The ASCII armoured key
 * @len: Length of the ASCII block or -1
 * @start: Returned, start of the key
 * @end: Returned, end of the key
 *
 * Finds a key in a char* block
 *
 * Returns: FALSE if no key is contained, TRUE else
 */
gboolean
detect_key (const gchar *text, gint len, const gchar **start, const gchar **end)
{
    const gchar *t;

    if (len == -1)
        len = strlen (text);
    
    /* Find the first of the headers */
    if((t = g_strstr_len (text, len, PGP_KEY_BEGIN)) == NULL)
        return FALSE;
    if (start)
        *start = t;
        
    /* Find the end of that block */
    if((t = g_strstr_len (t, len - (t - text), PGP_KEY_END)) == NULL)
        return FALSE;
    if (end)
        *end = t;
    
    return TRUE; 
}

/**
* session: The associated soup session
* msg: The message received
* hop: BastileHKPOperation associated
*
* Callback for soup_session_queue_message from bastile_hkp_source_export_raw
* Extracts keys from the message and writes them to the results of the BastileOperation
*
**/
static void 
get_callback (SoupSession *session, SoupMessage *msg, BastileHKPOperation *hop) 
{
    GError *err = NULL;
    const gchar *start;
    const gchar *end;
    GOutputStream *output;
    const gchar *text;
    gboolean ret;
    guint len;
    gsize written;
    
    if (hop->cancelling)
        return;
    
    if (SOUP_MESSAGE_IS_ERROR (msg)) {
        fail_hkp_operation (hop, msg, NULL);
        return;
    }
    
    end = text = msg->response_body->data;
    len = msg->response_body->length;
    
    for (;;) {

        len -= end - text;
        text = end;
        
        if(!detect_key (text, len, &start, &end))
            break;
        
		/* Any key blocks get written to our result data */
		output = bastile_operation_get_result (BASTILE_OPERATION (hop));
		g_return_if_fail (G_IS_OUTPUT_STREAM (output));

		ret = g_output_stream_write_all (output, start, end - start, &written, NULL, &err) &&
		      g_output_stream_write_all (output, "\n", 1, &written, NULL, &err) &&
		      g_output_stream_flush (output, NULL, &err);

		if (!ret) {
			bastile_operation_mark_done (BASTILE_OPERATION (hop), FALSE, err);
			return;
		}
    	}
        
    	if (--hop->requests <= 0) {
    		output = bastile_operation_get_result (BASTILE_OPERATION (hop));
    		g_return_if_fail (G_IS_OUTPUT_STREAM (output));
    		g_output_stream_close (output, NULL, &err);
    		bastile_operation_mark_done (BASTILE_OPERATION (hop), FALSE, err);
    	} else {
    		bastile_operation_mark_progress_full (BASTILE_OPERATION (hop), _("Retrieving keys..."), 
    		                                       hop->requests, hop->total);
    	}
}

 
/* -----------------------------------------------------------------------------
 *  BASTILE HKP SOURCE
 */

enum {
    PROP_0,
    PROP_SOURCE_TAG,
    PROP_SOURCE_LOCATION
};

static void bastile_source_iface (BastileSourceIface *iface);

G_DEFINE_TYPE_EXTENDED (BastileHKPSource, bastile_hkp_source, BASTILE_TYPE_SERVER_SOURCE, 0,
                        G_IMPLEMENT_INTERFACE (BASTILE_TYPE_SOURCE, bastile_source_iface));

static void 
bastile_hkp_source_init (BastileHKPSource *hsrc)
{

}

/**
* object: ignored
* prop_id: id of the property
* value: the resulting value
* pspec: ignored
*
* Returns data for PROP_SOURCE_TAG and PROP_SOURCE_LOCATION
*
**/
static void 
bastile_hkp_source_get_property (GObject *object, guint prop_id, GValue *value,
                                  GParamSpec *pspec)
{
    switch (prop_id) {
    case PROP_SOURCE_TAG:
        g_value_set_uint (value, BASTILE_PGP);
        break;
    case PROP_SOURCE_LOCATION:
        g_value_set_enum (value, BASTILE_LOCATION_REMOTE);
        break;
    };
}

static void 
bastile_hkp_source_set_property (GObject *object, guint prop_id, const GValue *value,
                                   GParamSpec *pspec)
{

}

/**
* match: the \0 terminated string to test
*
*
*
* Returns TRUE if it is a hex keyid
**/
static gboolean
is_hex_keyid (const gchar *match)
{
    const gchar *t;
    
    t = match;
    
    if (strlen (match) != 8)
        return FALSE;
    
    while (*t != '\0') {
        if (!((*t >= 0x30 && *t <= 0x39) || (*t >= 0x41 && *t <= 0x46) || (*t >= 0x61 && *t <= 0x66)))
            return FALSE;
            
        t++;
    }
    
    return TRUE;
}

/**
* src: The HKP source to search in
* match: The value to match (keyid or anything else)
*
* Creates a search operation, finds data on the keyserver
*
* Returns A running Bastile search operation
**/
static BastileOperation*
bastile_hkp_source_search (BastileSource *src, const gchar *match)
{
    BastileHKPOperation *hop;
    SoupMessage *message;
    GHashTable *form;
    gchar *t;
    SoupURI *uri;
    gchar hexfpr[11];
    
    g_assert (BASTILE_IS_SOURCE (src));
    g_assert (BASTILE_IS_HKP_SOURCE (src));

    hop = setup_hkp_operation (BASTILE_HKP_SOURCE (src));
    
    uri = get_http_server_uri (src, "/pks/lookup");
    g_return_val_if_fail (uri, FALSE);
    
    form = g_hash_table_new (g_str_hash, g_str_equal);
    g_hash_table_insert (form, "op", "index");
    
    if (is_hex_keyid (match)) {
        strncpy (hexfpr, "0x", 3);
        strncpy (hexfpr + 2, match, 9);
        
        g_hash_table_insert (form, "search", hexfpr);
    } else 
        g_hash_table_insert (form, "search", (char *)match);
        
    soup_uri_set_query_from_form (uri, form);
    g_hash_table_destroy (form);
    
    message = soup_message_new_from_uri ("GET", uri);
    
    soup_session_queue_message (hop->session, message, 
                                (SoupSessionCallback)refresh_callback, hop);

    hop->total = hop->requests = 1;
    t = g_strdup_printf (_("Searching for keys on: %s"), uri->host);
    bastile_operation_mark_progress (BASTILE_OPERATION (hop), t, -1);
    g_free (t);

    soup_uri_free (uri);

    bastile_server_source_take_operation (BASTILE_SERVER_SOURCE (src), 
                                           BASTILE_OPERATION (hop));
    g_object_ref (hop);
    return BASTILE_OPERATION (hop);
}

/**
* sksrc: The HKP source to use
* input: The input stream to add
*
* Imports a list of keys from the input stream to the keyserver
*
* Returns A running input operation (or NULL on error)
**/
static BastileOperation* 
bastile_hkp_source_import (BastileSource *sksrc, GInputStream *input)
{
    BastileHKPOperation *hop;
    BastileHKPSource *hsrc;
    SoupMessage *message;
    GSList *keydata = NULL;
    GString *buf = NULL;
    GHashTable *form;
    gchar *key, *t;
    SoupURI *uri;
    GSList *l;
    guint len;
    
    g_return_val_if_fail (BASTILE_IS_HKP_SOURCE (sksrc), NULL);
    hsrc = BASTILE_HKP_SOURCE (sksrc);
    
    g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
    g_object_ref (input);
    
    for (;;) {
     
        buf = g_string_sized_new (2048);
        len = bastile_util_read_data_block (buf, input, "-----BEGIN PGP PUBLIC KEY BLOCK-----",
                                             "-----END PGP PUBLIC KEY BLOCK-----");
    
        if (len > 0) {
            keydata = g_slist_prepend (keydata, g_string_free (buf, FALSE));
        } else {
            g_string_free (buf, TRUE);
            break;
        }
    }
    
    	if (g_slist_length (keydata) == 0) {
    		g_object_unref (input);
    		return bastile_operation_new_complete (NULL);
    	}
    
    /* Figure out the URI we're sending to */    
    uri = get_http_server_uri (sksrc, "/pks/add");
    g_return_val_if_fail (uri, FALSE);

    /* New operation and away we go */
    keydata = g_slist_reverse (keydata);   
    hop = setup_hkp_operation (hsrc);
    
    form = g_hash_table_new (g_str_hash, g_str_equal);
    for (l = keydata; l; l = g_slist_next (l)) {
        g_assert (l->data != NULL);

        g_hash_table_insert (form, "keytext", l->data);
        key = soup_form_encode_urlencoded (form);

        message = soup_message_new_from_uri ("POST", uri);
        soup_message_set_request (message, "application/x-www-form-urlencoded",
                                  SOUP_MEMORY_TAKE, key, strlen (key));
        
        soup_session_queue_message (hop->session, message, 
                                    (SoupSessionCallback)send_callback, hop);
        hop->requests++;
    }
    g_hash_table_destroy (form);

    hop->total = hop->requests;
    t = g_strdup_printf (_("Connecting to: %s"), uri->host);
    bastile_operation_mark_progress (BASTILE_OPERATION (hop), t, -1);
    g_free (t);

    soup_uri_free (uri);
    
    	bastile_util_string_slist_free (keydata);
    	g_object_unref (input);
    
    return BASTILE_OPERATION (hop);
}

/**
* sksrc: A HKP source
* keyids: the keyids to look up
* output: The output stream the data will end in
*
* Gets data from the keyserver, writes it to the output stream
*
* Returns A new Bastile operation
**/
static BastileOperation*  
bastile_hkp_source_export_raw (BastileSource *sksrc, GSList *keyids,
                                GOutputStream *output)
{
    BastileHKPOperation *hop;
    BastileHKPSource *hsrc;
    SoupMessage *message;
    gchar *t;
    SoupURI *uri;
    const gchar *fpr;
    gchar hexfpr[11];
    GHashTable *form;
    guint len;
    GSList *l;
    
    g_return_val_if_fail (BASTILE_IS_HKP_SOURCE (sksrc), NULL);
    g_return_val_if_fail (output == NULL || G_IS_OUTPUT_STREAM (output), NULL);

    hsrc = BASTILE_HKP_SOURCE (sksrc);
    
    if (g_slist_length (keyids) == 0)
        return bastile_operation_new_complete (NULL);

    uri = get_http_server_uri (sksrc, "/pks/lookup");
    g_return_val_if_fail (uri, FALSE);

    /* New operation started */    
    hop = setup_hkp_operation (hsrc);

	g_object_ref (output);
	bastile_operation_mark_result (BASTILE_OPERATION (hop), output, g_object_unref);

    /* prepend the hex prefix (0x) to make keyservers happy */
    strncpy(hexfpr, "0x", 3);

    form = g_hash_table_new (g_str_hash, g_str_equal);
    for (l = keyids; l; l = g_slist_next (l)) {

        /* Get the key id and limit it to 8 characters */
        fpr = bastile_pgp_key_calc_rawid (GPOINTER_TO_UINT (l->data));
        len = strlen (fpr);
        if (len > 8)
            fpr += (len - 8);
	
        strncpy(hexfpr + 2, fpr, 9);

        /* The get key URI */
        g_hash_table_insert (form, "op", "get");
        g_hash_table_insert (form, "search", (char *)hexfpr);
        soup_uri_set_query_from_form (uri, form);

        message = soup_message_new_from_uri ("GET", uri);
        
        soup_session_queue_message (hop->session, message, 
                                    (SoupSessionCallback)get_callback, hop);

        hop->requests++;
    }
    g_hash_table_destroy (form);
    
    hop->total = hop->requests;
    t = g_strdup_printf (_("Connecting to: %s"), uri->host);
    bastile_operation_mark_progress (BASTILE_OPERATION (hop), t, -1);
    g_free (t);

    soup_uri_free (uri);
    return BASTILE_OPERATION (hop);    
}


/**
* klass:
*
* Initialize the basic class stuff
*
**/
static void
bastile_hkp_source_class_init (BastileHKPSourceClass *klass)
{
	GObjectClass *gobject_class;
   
	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->get_property = bastile_hkp_source_get_property;
	gobject_class->set_property = bastile_hkp_source_set_property;
	
	bastile_hkp_source_parent_class = g_type_class_peek_parent (klass);

	g_object_class_override_property (gobject_class, PROP_SOURCE_TAG, "source-tag");
	g_object_class_override_property (gobject_class, PROP_SOURCE_LOCATION, "source-location");

	bastile_registry_register_type (NULL, BASTILE_TYPE_HKP_SOURCE, "source", "remote", BASTILE_PGP_STR, NULL);
	bastile_servers_register_type ("hkp", _("HTTP Key Server"), bastile_hkp_is_valid_uri);
	
	bastile_registry_register_function (NULL, bastile_pgp_key_canonize_id, "canonize", BASTILE_PGP_STR, NULL);
}

/**
* iface: The interface to set
*
* Set up the default BastileSourceIface
*
**/
static void 
bastile_source_iface (BastileSourceIface *iface)
{
	iface->search = bastile_hkp_source_search;
	iface->import = bastile_hkp_source_import;
	iface->export_raw = bastile_hkp_source_export_raw;
}

/**
 * bastile_hkp_source_new
 * @uri: The server to connect to 
 * 
 * Creates a new key source for an HKP PGP server.
 * 
 * Returns: A new HKP Key Source
 */
BastileHKPSource*   
bastile_hkp_source_new (const gchar *uri, const gchar *host)
{
    g_return_val_if_fail (bastile_hkp_is_valid_uri (uri), NULL);
    g_return_val_if_fail (host && *host, NULL);
    return g_object_new (BASTILE_TYPE_HKP_SOURCE, "uri", uri,
                         "key-server", host, NULL);
}

/**
 * bastile_hkp_is_valid_uri
 * @uri: The uri to check
 * 
 * Returns: Whether the passed uri is valid for an HKP key source
 */
gboolean              
bastile_hkp_is_valid_uri (const gchar *uri)
{
    SoupURI *soup;
    gboolean ret = FALSE;
    gchar *t;
    
    g_return_val_if_fail (uri && uri[0], FALSE);
    
    /* Replace 'hkp' with 'http' at the beginning of the URI */
    if (strncasecmp (uri, "hkp:", 4) == 0) {
        t = g_strdup_printf("http:%s", uri + 4);
        soup = soup_uri_new (t);
        g_free (t);
        
    /* Not 'hkp', but maybe 'http' */
    } else {
        soup = soup_uri_new (uri);
    }
    
    if (soup) {
        /* Must be http or https, have a host. No querystring, user, path, passwd etc... */
        if ((soup->scheme == SOUP_URI_SCHEME_HTTP || soup->scheme == SOUP_URI_SCHEME_HTTPS) && 
            !soup->user && !soup->password && !soup->query && !soup->fragment &&
            soup->host && g_str_equal (soup->path ? soup->path : "/", "/"))
            ret = TRUE;
        soup_uri_free (soup);
    }

    return ret;
}

#endif /* WITH_HKP */
