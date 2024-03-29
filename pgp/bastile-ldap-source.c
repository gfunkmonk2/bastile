/*
 * Bastile
 *
 * Copyright (C) 2004 - 2006 Stefan Walter
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
#include <stdlib.h>
#include <errno.h>

#include <glib/gi18n.h>

#include "bastile-ldap-source.h"

#include "bastile-pgp-key.h"
#include "bastile-pgp-subkey.h"
#include "bastile-pgp-uid.h"

#include "bastile-operation.h"
#include "bastile-servers.h"
#include "bastile-util.h"

#include "common/bastile-registry.h"
#include "common/bastile-object-list.h"

#include <ldap.h>

#ifdef WITH_SOUP
#include <libsoup/soup-address.h>
#endif

#ifdef WITH_LDAP

/* Override the DEBUG_LDAP_ENABLE switch here */
/* #define DEBUG_LDAP_ENABLE 1 */

#ifndef DEBUG_LDAP_ENABLE
#if _DEBUG
#define DEBUG_LDAP_ENABLE 1
#else
#define DEBUG_LDAP_ENABLE 0
#endif
#endif

#if DEBUG_LDAP_ENABLE
#define DEBUG_LDAP(x) g_printerr x
#define DEBUG_LDAP_ENTRY(a,b) dump_ldap_entry(a,b)
#else
#define DEBUG_LDAP(x) 
#define DEBUG_LDAP_ENTRY(a,b) 
#endif

/* Amount of keys to load in a batch */
#define DEFAULT_LOAD_BATCH 30

/* -----------------------------------------------------------------------------
 * SERVER INFO
 */
 
typedef struct _LDAPServerInfo {
    gchar *base_dn;             /* The base dn where PGP keys are found */
    gchar *key_attr;            /* The attribute of PGP key data */
    guint version;              /* The version of the PGP server software */
} LDAPServerInfo;

static void
free_ldap_server_info (LDAPServerInfo *sinfo)
{
    if (sinfo) {
        g_free (sinfo->base_dn);
        g_free (sinfo->key_attr);
        g_free (sinfo);
    }
}

static void
set_ldap_server_info (BastileLDAPSource *lsrc, LDAPServerInfo *sinfo)
{
    g_object_set_data_full (G_OBJECT (lsrc), "server-info", sinfo,
                            (GDestroyNotify)free_ldap_server_info);
}

static LDAPServerInfo*         
get_ldap_server_info (BastileLDAPSource *lsrc, gboolean force)
{
    LDAPServerInfo *sinfo;
    
    sinfo = g_object_get_data (G_OBJECT (lsrc), "server-info");
 
    /* When we're asked to force getting the data, we fill in 
     * some defaults */   
    if (!sinfo && force) {
        sinfo = g_new0 (LDAPServerInfo, 1);
        sinfo->base_dn = g_strdup ("OU=ACTIVE,O=PGP KEYSPACE,C=US");
        sinfo->key_attr = g_strdup ("pgpKey");
        sinfo->version = 0;
        set_ldap_server_info (lsrc, sinfo);
    } 
    
    return sinfo;
}
                                                 

/* -----------------------------------------------------------------------------
 *  LDAP HELPERS
 */
 
#define LDAP_ERROR_DOMAIN (get_ldap_error_domain())

static gchar**
get_ldap_values (LDAP *ld, LDAPMessage *entry, const char *attribute)
{
    GArray *array;
    struct berval **bv;
    gchar *value;
    int num, i;

    bv = ldap_get_values_len (ld, entry, attribute);
    if (!bv)
        return NULL;

    array = g_array_new (TRUE, TRUE, sizeof (gchar*));
    num = ldap_count_values_len (bv);
    for(i = 0; i < num; i++) {
        value = g_strndup (bv[i]->bv_val, bv[i]->bv_len);
        g_array_append_val(array, value);
    }

    return (gchar**)g_array_free (array, FALSE);
}

#if DEBUG_LDAP_ENABLE

static void
dump_ldap_entry (LDAP *ld, LDAPMessage *res)
{
    BerElement *pos;
    gchar **values;
    gchar **v;
    char *t;
    
    t = ldap_get_dn (ld, res);
    g_printerr ("dn: %s\n", t);
    ldap_memfree (t);
    
    for (t = ldap_first_attribute (ld, res, &pos); t; 
         t = ldap_next_attribute (ld, res, pos)) {
             
        values = get_ldap_values (ld, res, t);
        for (v = values; *v; v++) 
            g_printerr ("%s: %s\n", t, *v);
             
        g_strfreev (values);
        ldap_memfree (t);
    }
    
    ber_free (pos, 0);
}

#endif

static GQuark
get_ldap_error_domain ()
{
    static GQuark q = 0;
    if(q == 0)
        q = g_quark_from_static_string ("bastile-ldap-error");
    return q;
}

static gchar*
get_string_attribute (LDAP *ld, LDAPMessage *res, const char *attribute)
{
    gchar **vals;
    gchar *v;
    
    vals = get_ldap_values (ld, res, attribute);
    if (!vals)
        return NULL; 
    v = vals[0] ? g_strdup (vals[0]) : NULL;
    g_strfreev (vals);
    return v;
}

static gboolean
get_boolean_attribute (LDAP* ld, LDAPMessage *res, const char *attribute)
{
    gchar **vals;
    gboolean b;
    
    vals = get_ldap_values (ld, res, attribute);
    if (!vals)
        return FALSE;
    b = vals[0] && atoi (vals[0]) == 1;
    g_strfreev (vals);
    return b;
}

static long int
get_int_attribute (LDAP* ld, LDAPMessage *res, const char *attribute)
{
    gchar **vals;
    long int d;
    
    vals = get_ldap_values (ld, res, attribute);
    if (!vals)
        return 0;
    d = vals[0] ? atoi (vals[0]) : 0;
    g_strfreev (vals);
    return d;         
}

static long int
get_date_attribute (LDAP* ld, LDAPMessage *res, const char *attribute)
{
    struct tm t;
    gchar **vals;
    long int d;
    
    vals = get_ldap_values (ld, res, attribute);
    if (!vals)
        return 0;
        
    if (vals[0]) {
        memset(&t, 0, sizeof (t));

        /* YYYYMMDDHHmmssZ */
        sscanf(vals[0], "%4d%2d%2d%2d%2d%2d",
            &t.tm_year, &t.tm_mon, &t.tm_mday, 
            &t.tm_hour, &t.tm_min, &t.tm_sec);

        t.tm_year -= 1900;
        t.tm_isdst = -1;
        t.tm_mon--;

        d = mktime (&t);
    }        

    g_strfreev (vals);
    return d;         
}

static const gchar*
get_algo_attribute (LDAP* ld, LDAPMessage *res, const char *attribute)
{
	const gchar *a = NULL;
	gchar **vals;
    
	vals = get_ldap_values (ld, res, attribute);
	if (!vals)
		return 0;
    
	if (vals[0]) {
		if (g_ascii_strcasecmp (vals[0], "DH/DSS") == 0 || 
		    g_ascii_strcasecmp (vals[0], "Elg") == 0 ||
		    g_ascii_strcasecmp (vals[0], "Elgamal") == 0 ||
		    g_ascii_strcasecmp (vals[0], "DSS/DH") == 0)
			a = "Elgamal";
		if (g_ascii_strcasecmp (vals[0], "RSA") == 0)
			a = "RSA";
		if (g_ascii_strcasecmp (vals[0], "DSA") == 0)
			a = "DSA";     
	}
    
	g_strfreev (vals);
	return a;
}

/* 
 * Escapes a value so it's safe to use in an LDAP filter. Also trims
 * any spaces which cause problems with some LDAP servers.
 */
static gchar*
escape_ldap_value (const gchar *v)
{
    GString *value;
    gchar* result;
    
    g_assert (v);
    value = g_string_sized_new (strlen(v));
    
    for ( ; *v; v++) {
        switch(*v) {
        case '#': case ',': case '+': case '\\':
        case '/': case '\"': case '<': case '>': case ';':
            value = g_string_append_c (value, '\\');
            value = g_string_append_c (value, *v);
            continue;
        };  

        if(*v < 32 || *v > 126) {
            g_string_append_printf (value, "\\%02X", *v);
            continue;
        }
        
        value = g_string_append_c (value, *v);
    }
    
    result = g_string_free (value, FALSE);
    g_strstrip (result);
    return result;
}

/* -----------------------------------------------------------------------------
 *  LDAP OPERATION     
 */
 
#define BASTILE_TYPE_LDAP_OPERATION            (bastile_ldap_operation_get_type ())
#define BASTILE_LDAP_OPERATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_LDAP_OPERATION, BastileLDAPOperation))
#define BASTILE_LDAP_OPERATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_LDAP_OPERATION, BastileLDAPOperationClass))
#define BASTILE_IS_LDAP_OPERATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_LDAP_OPERATION))
#define BASTILE_IS_LDAP_OPERATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_LDAP_OPERATION))
#define BASTILE_LDAP_OPERATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_LDAP_OPERATION, BastileLDAPOperationClass))

typedef gboolean (*OpLDAPCallback)   (BastileOperation *op, LDAPMessage *result);
    
DECLARE_OPERATION (LDAP, ldap)
#ifdef WITH_SOUP
    SoupAddress *addr;              /* For async DNS lookup */
#endif
    BastileLDAPSource *lsrc;       /* The source */
    LDAP *ldap;                     /* The LDAP connection */
    int ldap_op;                    /* The current LDAP async msg */
    guint stag;                     /* The tag for the idle event source */
    OpLDAPCallback ldap_cb;         /* Callback for next async result */
    OpLDAPCallback chain_cb;        /* Callback when connection is done */
END_DECLARE_OPERATION

IMPLEMENT_OPERATION (LDAP, ldap)


/* -----------------------------------------------------------------------------
 *  SEARCH OPERATION STUFF 
 */
 
static const char *kServerAttributes[] = {
    "basekeyspacedn",
    "pgpbasekeyspacedn",
    "version",
    NULL
};

static void 
bastile_ldap_operation_init (BastileLDAPOperation *sop)
{
    sop->ldap_op = -1;
}

static void 
bastile_ldap_operation_dispose (GObject *gobject)
{
    BastileLDAPOperation *lop = BASTILE_LDAP_OPERATION (gobject);

#ifdef WITH_SOUP
    if (lop->addr)
        g_object_unref (lop->addr);
    lop->addr = NULL;
#endif 
        
    if (lop->lsrc) {
        g_object_unref (lop->lsrc);
        lop->lsrc = NULL;
    }
    
    if (lop->ldap_op != -1) {
        if (lop->ldap)
            ldap_abandon_ext (lop->ldap, lop->ldap_op, NULL, NULL);
        lop->ldap_op = -1;
    }

    if (lop->ldap) {
        ldap_unbind_ext (lop->ldap, NULL, NULL);
        lop->ldap = NULL;
    }
    
    if (lop->stag) {
        g_source_remove (lop->stag);
        lop->stag = 0;
    }
        
    G_OBJECT_CLASS (ldap_operation_parent_class)->dispose (gobject);  
}

static void 
bastile_ldap_operation_finalize (GObject *gobject)
{
    BastileLDAPOperation *lop = BASTILE_LDAP_OPERATION (gobject);

    g_assert (lop->lsrc == NULL);
    g_assert (lop->ldap_op == -1);
    g_assert (lop->stag == 0);
    g_assert (lop->ldap == NULL);
    
    G_OBJECT_CLASS (ldap_operation_parent_class)->finalize (gobject);  
}

static void 
bastile_ldap_operation_cancel (BastileOperation *operation)
{
    BastileLDAPOperation *lop;
    
    g_assert (BASTILE_IS_LDAP_OPERATION (operation));
    lop = BASTILE_LDAP_OPERATION (operation);
    
#ifdef WITH_SOUP
    /* This cancels lookup */
    if (lop->addr)
        g_object_unref (lop->addr);
    lop->addr = NULL;
#endif
    
    if (lop->ldap_op != -1) {
        if (lop->ldap)
            ldap_abandon_ext (lop->ldap, lop->ldap_op, NULL, NULL);
        lop->ldap_op = -1;
    }

    if (lop->ldap) {
        ldap_unbind_ext (lop->ldap, NULL, NULL);
        lop->ldap = NULL;
    }

    if (lop->stag) {
        g_source_remove (lop->stag);
        lop->stag = 0;
    }
        
    bastile_operation_mark_done (operation, TRUE, NULL);
}

/* Cancels operation and marks the LDAP operation as failed */
static void
fail_ldap_operation (BastileLDAPOperation *lop, int code)
{
    gchar *t;
    
    if (code == 0)
        ldap_get_option (lop->ldap, LDAP_OPT_ERROR_NUMBER, &code);
    
    g_object_get (lop->lsrc, "key-server", &t, NULL);
    bastile_operation_mark_done (BASTILE_OPERATION (lop), FALSE, 
            g_error_new (LDAP_ERROR_DOMAIN, code, _("Couldn't communicate with '%s': %s"), 
                         t, ldap_err2string(code)));
    g_free (t);
}

/* Gets called regularly to check for results of LDAP async work */
static gboolean
result_callback (BastileLDAPOperation *lop)
{
    struct timeval timeout;
    LDAPMessage *result;
    gboolean ret;
    int r, i;
    
    for (i = 0; i < DEFAULT_LOAD_BATCH; i++) {

        g_assert (BASTILE_IS_LDAP_OPERATION (lop));
        g_assert (lop->ldap != NULL);
        g_assert (lop->ldap_op != -1);
        
        /* This effects a poll */
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        r = ldap_result (lop->ldap, lop->ldap_op, 0, &timeout, &result);
        switch (r) {   
        case -1: /* Strange system error */
            bastile_operation_mark_done (BASTILE_OPERATION (lop), FALSE, 
                g_error_new (LDAP_ERROR_DOMAIN, errno, "%s", g_strerror (errno)));
            return FALSE;
        
        case 0: /* Timeout exceeded */
            return TRUE;
        };
        
        ret = (lop->ldap_cb) (BASTILE_OPERATION (lop), result);
        ldap_msgfree (result);
        
        if(!ret)
            break;
    }

    /* We can't access lop at this point if not continuing. 
     * It could have been freed */
    if (ret) {
        /* We always need a callback if we're continuing */
        g_assert (lop->ldap_cb);

        /* Should not be marked as done. */
        g_assert (bastile_operation_is_running (BASTILE_OPERATION (lop)));
    }    
        
    return ret;
}

/* Called when retrieving server info is done, and we need to start work */
static gboolean
done_info_start_op (BastileOperation *op, LDAPMessage *result)
{
    BastileLDAPOperation *lop = BASTILE_LDAP_OPERATION (op);
    LDAPServerInfo *sinfo;
    char *message;
    int code;
    int r;
    
    g_assert (BASTILE_IS_LDAP_OPERATION (lop));

    /* This can be null when we short-circuit the server info */
    if (result) {
        r = ldap_msgtype (result);
        g_return_val_if_fail (r == LDAP_RES_SEARCH_ENTRY || r == LDAP_RES_SEARCH_RESULT, FALSE);
        
        /* If we have results then fill in the server info */
        if (r == LDAP_RES_SEARCH_ENTRY) {

            DEBUG_LDAP (("[ldap] Server Info Result:\n"));
            DEBUG_LDAP_ENTRY (lop->ldap, result);

            /* NOTE: When adding attributes here make sure to add them to kServerAttributes */
            sinfo = g_new0 (LDAPServerInfo, 1);
            sinfo->version = get_int_attribute (lop->ldap, result, "version");
            sinfo->base_dn = get_string_attribute (lop->ldap, result, "basekeyspacedn");
            if (!sinfo->base_dn)
                sinfo->base_dn = get_string_attribute (lop->ldap, result, "pgpbasekeyspacedn");
            sinfo->key_attr = g_strdup (sinfo->version > 1 ? "pgpkeyv2" : "pgpkey");
            set_ldap_server_info (lop->lsrc, sinfo);
            
            ldap_abandon_ext (lop->ldap, lop->ldap_op, NULL, NULL);
            lop->ldap_op = -1;
            
        } else {
            lop->ldap_op = -1;
            r = ldap_parse_result (lop->ldap, result, &code, NULL, &message, NULL, NULL, 0);
            g_return_val_if_fail (r == LDAP_SUCCESS, FALSE);

            if (code != LDAP_SUCCESS) 
                g_warning ("operation to get LDAP server info failed: %s", message);
            
            ldap_memfree (message);
        }
    }
    
    /* Call the main operation callback */
    return (lop->chain_cb) (op, NULL);
}

/* Called when LDAP bind is done, and we need to retrieve server info */
static gboolean
done_bind_start_info (BastileOperation *op, LDAPMessage *result)
{
    BastileLDAPOperation *lop = BASTILE_LDAP_OPERATION (op);
    LDAPServerInfo *sinfo;
    char *message;
    int code;
    int r;
    
    /* Always do this first, because we're done with the 
     * operation regardless of the result */
    lop->ldap_op = -1;

    g_return_val_if_fail (BASTILE_IS_LDAP_OPERATION (lop), FALSE);
    g_return_val_if_fail (result != NULL, FALSE);
    g_return_val_if_fail (ldap_msgtype (result) == LDAP_RES_BIND, FALSE);

    /* The result of the bind operation */
    r = ldap_parse_result (lop->ldap, result, &code, NULL, &message, NULL, NULL, 0);
    g_return_val_if_fail (r == LDAP_SUCCESS, FALSE);

    if (code != LDAP_SUCCESS) {
        bastile_operation_mark_done (op, FALSE, 
                g_error_new_literal (LDAP_ERROR_DOMAIN, code, message));
        return FALSE;
    }

    ldap_memfree (message);
     
    /* Check if we need server info */
    sinfo = get_ldap_server_info (lop->lsrc, FALSE);
    if (sinfo != NULL)
        return done_info_start_op (op, NULL);
        
    /* Retrieve the server info */
    r = ldap_search_ext (lop->ldap, "cn=PGPServerInfo", LDAP_SCOPE_BASE,
                         "(objectclass=*)", (char**)kServerAttributes, 0,
                         NULL, NULL, NULL, 0, &(lop->ldap_op));    
    if (r != LDAP_SUCCESS) {
        fail_ldap_operation (lop, r);
        return FALSE;
    }

    lop->ldap_cb = done_info_start_op;
    return TRUE;
}

/* Once the DNS name is resolved, we end up here */
static void
resolved_callback (gpointer unused, guint status, BastileLDAPOperation *lop)
{
    guint port = LDAP_PORT;
    gchar *server = NULL;
    struct berval cred;
    gchar *t;
    int rc;
    
    g_object_get (lop->lsrc, "key-server", &server, NULL);
    g_return_if_fail (server && server[0]);
    
    if ((t = strchr (server, ':')) != NULL) {
        *t = 0;
        t++;
        port = atoi (t);
        if (port <= 0 || port >= G_MAXUINT16) {
            g_warning ("invalid port number: %s (using default)", t);
            port = LDAP_PORT;
        }
    }

#ifdef WITH_SOUP
    
    /* DNS failed */
    if (!SOUP_STATUS_IS_SUCCESSFUL (status)) {
        bastile_operation_mark_done (BASTILE_OPERATION (lop), FALSE, 
            g_error_new (BASTILE_ERROR, -1, _("Couldn't resolve address: %s"), server));
        g_free (server);
        return;
    } 
    
    g_assert (lop->addr);
    
    {
        /* Now that we've resolved our address, connect via IP */
        gchar *url;

        url = g_strdup_printf ("ldap://%s:%u", soup_address_get_physical (lop->addr), port);
        ldap_initialize (&(lop->ldap), url);
        g_free (url);
        g_return_if_fail (lop->ldap != NULL);
    }
    
#else /* WITH_SOUP */
    
    /* No async DNS resolve, let libldap handle resolving synchronously */
    ldap_initialize (&(lop->ldap), server);
    g_return_if_fail (lop->ldap != NULL);    
    
#endif /* WITH_SOUP */
    
    /* The ldap_cb and chain_cb were set in bastile_ldap_operation_start */
    
    t = g_strdup_printf (_("Connecting to: %s"), server);
    bastile_operation_mark_progress (BASTILE_OPERATION (lop), t, -1);
    g_free (t);

    g_free (server);
    
    /* Start the bind operation */
    cred.bv_val = "";
    cred.bv_len = 0;
    rc = ldap_sasl_bind (lop->ldap, NULL, LDAP_SASL_SIMPLE, &cred, NULL, NULL, &(lop->ldap_op));
    if (rc != LDAP_SUCCESS) 
        fail_ldap_operation (lop, rc);
        
    else   /* This starts looking for results */
        lop->stag = g_idle_add_full (G_PRIORITY_DEFAULT_IDLE, 
                        (GSourceFunc)result_callback, lop, NULL);
}

/* Start an LDAP (bind, server info) request */
static BastileLDAPOperation*
bastile_ldap_operation_start (BastileLDAPSource *lsrc, OpLDAPCallback cb,
                               guint total)
{
    BastileLDAPOperation *lop;
#ifdef WITH_SOUP
    gchar *server = NULL;
    gchar *t;
#endif

    g_assert (BASTILE_IS_LDAP_SOURCE (lsrc));
    
    lop = g_object_new (BASTILE_TYPE_LDAP_OPERATION, NULL);
    lop->lsrc = lsrc;
    g_object_ref (lsrc);

    /* Not used until step after resolve */
    lop->ldap_cb = done_bind_start_info;
    lop->chain_cb = cb;
    
    bastile_operation_mark_start (BASTILE_OPERATION (lop));
    
#ifdef WITH_SOUP
    
    /* Try and resolve asynchronously */
    g_object_get (lsrc, "key-server", &server, NULL);
    g_return_val_if_fail (server && server[0], NULL);

    if ((t = strchr (server, ':')) != NULL)
        *t = 0;
    lop->addr = soup_address_new (server, LDAP_PORT);
    
    t = g_strdup_printf (_("Resolving server address: %s"), server);
    bastile_operation_mark_progress (BASTILE_OPERATION (lop), t, -1);
    g_free (t);

    g_free (server);
    
    soup_address_resolve_async (lop->addr, NULL, NULL,
                                (SoupAddressCallback)resolved_callback, lop);
    
#else /* no WITH_SOUP */
    
    resolved_callback (NULL, 0, lop);
    
#endif
    
    return lop;
}

/* -----------------------------------------------------------------------------
 *  SEARCH OPERATION 
 */

static const char *kPGPAttributes[] = {
    "pgpcertid",
    "pgpuserid",
    "pgprevoked",
    "pgpdisabled",
    "pgpkeycreatetime",
    "pgpkeyexpiretime"
    "pgpkeysize",
    "pgpkeytype",
    NULL
};

static void
add_key (BastileLDAPSource *ssrc, BastilePgpKey *key)
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

/* Add a key to the key source from an LDAP entry */
static void
parse_key_from_ldap_entry (BastileLDAPOperation *lop, LDAPMessage *res)
{
	const gchar *algo;
	long int timestamp;
	long int expires;
        gchar *fpr, *fingerprint;
        gchar *uidstr;
        gboolean revoked;
        gboolean disabled;
        int length;
        
        g_assert (BASTILE_IS_LDAP_OPERATION (lop));
        g_return_if_fail (res && ldap_msgtype (res) == LDAP_RES_SEARCH_ENTRY);  
    
        fpr = get_string_attribute (lop->ldap, res, "pgpcertid");
        uidstr = get_string_attribute (lop->ldap, res, "pgpuserid");
        revoked = get_boolean_attribute (lop->ldap, res, "pgprevoked");
        disabled = get_boolean_attribute (lop->ldap, res, "pgpdisabled");
        timestamp = get_date_attribute (lop->ldap, res, "pgpkeycreatetime");
        expires = get_date_attribute (lop->ldap, res, "pgpkeyexpiretime");
        algo = get_algo_attribute (lop->ldap, res, "pgpkeytype");
        length = get_int_attribute (lop->ldap, res, "pgpkeysize");
    
        if (fpr && uidstr) {

        	BastilePgpSubkey *subkey;
        	BastilePgpKey *key;
        	BastilePgpUid *uid;
                GList *list;
                guint flags;

        	/* Build up a subkey */
        	subkey = bastile_pgp_subkey_new ();
        	bastile_pgp_subkey_set_keyid (subkey, fpr);
        	fingerprint = bastile_pgp_subkey_calc_fingerprint (fpr);
        	bastile_pgp_subkey_set_fingerprint (subkey, fingerprint);
        	g_free (fingerprint);
        	bastile_pgp_subkey_set_created (subkey, timestamp);
        	bastile_pgp_subkey_set_expires (subkey, expires);
        	bastile_pgp_subkey_set_algorithm (subkey, algo);
        	bastile_pgp_subkey_set_length (subkey, length);

        	flags = BASTILE_FLAG_EXPORTABLE;
        	if (revoked)
        		flags |= BASTILE_FLAG_REVOKED;
        	if (disabled)
        		flags |= BASTILE_FLAG_DISABLED;
       		bastile_pgp_subkey_set_flags (subkey, flags);

        	/* Build up a uid */
        	uid = bastile_pgp_uid_new (uidstr);
        	if (revoked)
        		bastile_pgp_uid_set_validity (uid, BASTILE_VALIDITY_REVOKED);
        	
        	/* Now build them into a key */
        	key = bastile_pgp_key_new ();
        	list = g_list_prepend (NULL, uid);
        	bastile_pgp_key_set_uids (key, list);
        	bastile_object_list_free (list);
        	list = g_list_prepend (NULL, subkey);
        	bastile_pgp_key_set_subkeys (key, list);
        	bastile_object_list_free (list);
        	g_object_set (key, "location", BASTILE_LOCATION_REMOTE, 
        	              "flags", flags, NULL);
        
        	add_key (lop->lsrc, key);
        	g_object_unref (key);
        }
    
        g_free (fpr);
        g_free (uidstr);
}

/* Got a search result */
static gboolean 
search_entry (BastileOperation *op, LDAPMessage *result)
{
    BastileLDAPOperation *lop = BASTILE_LDAP_OPERATION (op);
    char *message;
    int code;
    int r;
  
    r = ldap_msgtype (result);
    g_return_val_if_fail (r == LDAP_RES_SEARCH_ENTRY || r == LDAP_RES_SEARCH_RESULT, FALSE);
     
    /* An LDAP entry */
    if (r == LDAP_RES_SEARCH_ENTRY) {
        
        DEBUG_LDAP (("[ldap] Retrieved Key Entry:\n"));
        DEBUG_LDAP_ENTRY (lop->ldap, result);
        
        parse_key_from_ldap_entry (lop, result);
        return TRUE;
        
    /* All entries done */
    } else {
        lop->ldap_op = -1;
        r = ldap_parse_result (lop->ldap, result, &code, NULL, &message, NULL, NULL, 0);
        g_return_val_if_fail (r == LDAP_SUCCESS, FALSE);
        
        /* Error codes that we ignore */
        switch (code) {
        case LDAP_SIZELIMIT_EXCEEDED:
            code = LDAP_SUCCESS;
            break;
        };
        
        /* Failure */
        if (code != LDAP_SUCCESS) {
            if (!message || !message[0]) 
                fail_ldap_operation (lop, code);
            else
                bastile_operation_mark_done (BASTILE_OPERATION (lop), FALSE, 
                            g_error_new_literal (LDAP_ERROR_DOMAIN, code, message));
            
        /* Success */
        } else {
            bastile_operation_mark_done (BASTILE_OPERATION (lop), FALSE, NULL);
        }
        ldap_memfree (message);

        return FALSE;
    }       
}

/* Performs a search on an open LDAP connection */
static gboolean
start_search (BastileOperation *op, LDAPMessage *result)
{
    BastileLDAPOperation *lop = BASTILE_LDAP_OPERATION (op);  
    LDAPServerInfo *sinfo;
    gchar *filter, *t;
    int r;
    
    g_return_val_if_fail (lop->ldap != NULL, FALSE);
    g_assert (lop->ldap_op == -1);
    
    filter = (gchar*)g_object_get_data (G_OBJECT (lop), "filter");
    g_return_val_if_fail (filter != NULL, FALSE);

    t = (gchar*)g_object_get_data (G_OBJECT (lop), "details");
    bastile_operation_mark_progress (BASTILE_OPERATION (lop), t, -1);
    
    sinfo = get_ldap_server_info (lop->lsrc, TRUE);

    DEBUG_LDAP (("[ldap] Searching Server ... base: %s, filter: %s\n", 
                 sinfo->base_dn, filter));
    
    r = ldap_search_ext (lop->ldap, sinfo->base_dn, LDAP_SCOPE_SUBTREE,
                         filter, (char**)kPGPAttributes, 0,
                         NULL, NULL, NULL, 0, &(lop->ldap_op));    
    if (r != LDAP_SUCCESS) {
        fail_ldap_operation (lop, r);
        return FALSE;
    }                                    
    
    lop->ldap_cb = search_entry;
    return TRUE;                                
}

/* Initiate a serch operation by uid  */
static BastileLDAPOperation*
start_search_operation (BastileLDAPSource *lsrc, const gchar *pattern)
{
    BastileLDAPOperation *lop;
    gchar *filter;
    gchar *t;
    
    g_assert (pattern && pattern[0]);
    
    t = escape_ldap_value (pattern);
    filter = g_strdup_printf ("(pgpuserid=*%s*)", t);
    g_free (t);
    
    lop = bastile_ldap_operation_start (lsrc, start_search, 0);
    g_return_val_if_fail (lop != NULL, NULL);
    
    g_object_set_data_full (G_OBJECT (lop), "filter", filter, g_free);
    
    t = g_strdup_printf (_("Searching for keys containing '%s'..."), pattern);
    g_object_set_data_full (G_OBJECT (lop), "details", t, g_free);
    
    return lop;
}

#if 0 /* UNUSED */
/* Initiate a search operation by fingerprint */
static BastileLDAPOperation *
start_search_operation_fpr (BastileLDAPSource *lsrc, const gchar *fpr)
{
    BastileLDAPOperation *lop;
    gchar *filter, *t;
    guint l;
    
    g_assert (fpr && fpr[0]);
    
    l = strlen (fpr);
    if (l > 16)
        fpr += (l - 16);
    
    filter = g_strdup_printf ("(pgpcertid=%.16s)", fpr);
    
    lop = bastile_ldap_operation_start (lsrc, start_search, 1);
    g_return_val_if_fail (lop != NULL, NULL);
    
    g_object_set_data_full (G_OBJECT (lop), "filter", filter, g_free);
    
    t = g_strdup_printf (_("Searching for key id '%s'..."), fpr);
    g_object_set_data_full (G_OBJECT (lop), "details", t, g_free);
    
    return lop;
}
#endif /* UNUSED */

/* -----------------------------------------------------------------------------
 *  GET OPERATION 
 */
 
static gboolean get_key_from_ldap (BastileOperation *op, LDAPMessage *result);

/* Called when results come in from a key get */
static gboolean 
get_callback (BastileOperation *op, LDAPMessage *result)
{
    BastileLDAPOperation *lop = BASTILE_LDAP_OPERATION (op);  
    LDAPServerInfo *sinfo;
    GOutputStream *output;
    char *message;
    GError *err = NULL;
    gboolean ret;
    gchar *key;
    gsize written;
    int code;
    int r;
  
    r = ldap_msgtype (result);
    g_return_val_if_fail (r == LDAP_RES_SEARCH_ENTRY || r == LDAP_RES_SEARCH_RESULT, FALSE);

    sinfo = get_ldap_server_info (lop->lsrc, TRUE);
     
    /* An LDAP Entry */
    if (r == LDAP_RES_SEARCH_ENTRY) {
        
        DEBUG_LDAP (("[ldap] Retrieved Key Data:\n"));
        DEBUG_LDAP_ENTRY (lop->ldap, result);

        key = get_string_attribute (lop->ldap, result, sinfo->key_attr);
        
        if (key == NULL) {
            g_warning ("key server missing pgp key data");
            fail_ldap_operation (lop, LDAP_NO_SUCH_OBJECT); 
        }
        
        	output = bastile_operation_get_result (BASTILE_OPERATION (lop));
        	g_return_val_if_fail (G_IS_OUTPUT_STREAM (output), FALSE);

        	ret = g_output_stream_write_all (output, key, strlen (key), &written, NULL, &err) &&
        	      g_output_stream_write_all (output, "\n", 1, &written, NULL, &err) && 
        	      g_output_stream_flush (output, NULL, &err);
        
        	g_free (key);
        
        if (!ret) {
            bastile_operation_mark_done (BASTILE_OPERATION (lop), FALSE, err);
            return FALSE;
        }
        
        return TRUE;

    /* No more entries, result */
    } else {
       
        lop->ldap_op = -1;
        r = ldap_parse_result (lop->ldap, result, &code, NULL, &message, NULL, NULL, 0);
        g_return_val_if_fail (r == LDAP_SUCCESS, FALSE);
        
        if (code != LDAP_SUCCESS) {
            bastile_operation_mark_done (BASTILE_OPERATION (lop), FALSE, 
                            g_error_new_literal (LDAP_ERROR_DOMAIN, code, message));
        }

        ldap_memfree (message);

        /* Process more keys if possible */
        if (code == LDAP_SUCCESS) 
            return get_key_from_ldap (op, NULL);
    }       
    
    return FALSE;
}

/* Gets a key over an open LDAP connection */
static gboolean
get_key_from_ldap (BastileOperation *op, LDAPMessage *result)
{
    BastileLDAPOperation *lop = BASTILE_LDAP_OPERATION (op);  
    LDAPServerInfo *sinfo;
    GOutputStream *output;
    GSList *fingerprints, *fprfull;
    gchar *filter;
    char *attrs[2];
    const gchar *fpr;
    GError *err = NULL;
    int l, r;
    
    g_assert (lop->ldap != NULL);
    g_assert (lop->ldap_op == -1);
    
    fingerprints = (GSList*)g_object_get_data (G_OBJECT (lop), "fingerprints");
    fprfull = (GSList*)g_object_get_data (G_OBJECT (lop), "fingerprints-full");

    l = g_slist_length (fprfull);
    bastile_operation_mark_progress_full (BASTILE_OPERATION (lop), 
                                           _("Retrieving remote keys..."), 
                                           l - g_slist_length (fingerprints), l);
    
    if (fingerprints) {
     
        fpr = (const gchar*)(fingerprints->data);
        g_assert (fpr != NULL);

        /* Keep track of the ones that have already been done */
        fingerprints = g_slist_next (fingerprints);
        g_object_set_data (G_OBJECT (lop), "fingerprints", fingerprints);
                
        l = strlen (fpr);
        if (l > 16)
            fpr += (l - 16);
    
        filter = g_strdup_printf ("(pgpcertid=%.16s)", fpr);
        sinfo = get_ldap_server_info (lop->lsrc, TRUE);
        
        attrs[0] = sinfo->key_attr;
        attrs[1] = NULL;
        
        r = ldap_search_ext (lop->ldap, sinfo->base_dn, LDAP_SCOPE_SUBTREE,
                             filter, attrs, 0,
                             NULL, NULL, NULL, 0, &(lop->ldap_op));
        g_free (filter);

        if (r != LDAP_SUCCESS) {
            fail_ldap_operation (lop, r);
            return FALSE;
        }                                    
                
        lop->ldap_cb = get_callback;
        return TRUE;   
    }

	/* At this point we're done */
	output = bastile_operation_get_result (op);
	g_return_val_if_fail (G_IS_OUTPUT_STREAM (output), FALSE);
	g_output_stream_close (output, NULL, &err);
	bastile_operation_mark_done (op, FALSE, err); 
    
	return FALSE;
}

/* Starts a get operation for multiple keys */
static BastileLDAPOperation *
start_get_operation_multiple (BastileLDAPSource *lsrc, GSList *fingerprints, 
                              GOutputStream *output)
{
	BastileLDAPOperation *lop;
    
    	g_assert (g_slist_length (fingerprints) > 0);
    	g_return_val_if_fail (G_IS_OUTPUT_STREAM (output), NULL);
    
    	lop = bastile_ldap_operation_start (lsrc, get_key_from_ldap, 
    	                                     g_slist_length (fingerprints));
    	g_return_val_if_fail (lop != NULL, NULL);
        
	g_object_ref (output);
	bastile_operation_mark_result (BASTILE_OPERATION (lop), output, 
	                                g_object_unref);

	g_object_set_data (G_OBJECT (lop), "fingerprints", fingerprints);
	g_object_set_data_full (G_OBJECT (lop), "fingerprints-full", fingerprints, 
	                        (GDestroyNotify)bastile_util_string_slist_free);

    	return lop;
}

/* -----------------------------------------------------------------------------
 *  SEND OPERATION 
 */

static gboolean send_key_to_ldap (BastileOperation *op, LDAPMessage *result);

/* Called when results come in for a key send */
static gboolean 
send_callback (BastileOperation *op, LDAPMessage *result)
{
    BastileLDAPOperation *lop = BASTILE_LDAP_OPERATION (op);  
    char *message;
    int code;
    int r;

    lop->ldap_op = -1;

    g_return_val_if_fail (ldap_msgtype (result) == LDAP_RES_ADD, FALSE);

    r = ldap_parse_result (lop->ldap, result, &code, NULL, &message, NULL, NULL, 0);
    g_return_val_if_fail (r == LDAP_SUCCESS, FALSE);
    
    /* TODO: Somehow communicate this to the user */
    if (code == LDAP_ALREADY_EXISTS)
        code = LDAP_SUCCESS;
        
    if (code != LDAP_SUCCESS) 
        bastile_operation_mark_done (BASTILE_OPERATION (lop), FALSE, 
                        g_error_new_literal (LDAP_ERROR_DOMAIN, code, message));

    ldap_memfree (message);

    /* Process more keys */
    if (code == LDAP_SUCCESS)
        return send_key_to_ldap (op, NULL);

    return FALSE;
}

/* Initiate a key send over an open LDAP connection */
static gboolean
send_key_to_ldap (BastileOperation *op, LDAPMessage *result)
{
    BastileLDAPOperation *lop = BASTILE_LDAP_OPERATION (op);  
    LDAPServerInfo *sinfo;
    GSList *keys, *keysfull;
    gchar *key;
    gchar *base;
    LDAPMod mod;
    LDAPMod *attrs[2];
    char *values[2];
    guint l;
    int r;

    g_assert (lop->ldap != NULL);
    g_assert (lop->ldap_op == -1);
    
    keys = (GSList*)g_object_get_data (G_OBJECT (lop), "key-data");
    keysfull = (GSList*)g_object_get_data (G_OBJECT (lop), "key-data-full");
    
    l = g_slist_length (keysfull);
    bastile_operation_mark_progress_full (BASTILE_OPERATION (lop), 
                                           _("Sending keys to key server..."), 
                                           l - g_slist_length (keys), l);
    
    if (keys) {
     
        key = (gchar*)(keys->data);
        g_assert (key != NULL);

        /* Keep track of the ones that have already been done */
        keys = g_slist_next (keys);
        g_object_set_data (G_OBJECT (lop), "key-data", keys);

        sinfo = get_ldap_server_info (lop->lsrc, TRUE);
        
        values[0] = key;
        values[1] = NULL;
                
        memset (&mod, 0, sizeof (mod));
        mod.mod_op = LDAP_MOD_ADD;
        mod.mod_type = sinfo->key_attr;
        mod.mod_values = values;
        
        attrs[0] = &mod;
        attrs[1] = NULL;
        
        base = g_strdup_printf ("pgpCertid=virtual,%s", sinfo->base_dn);
        
        r = ldap_add_ext (lop->ldap, base, attrs, NULL, NULL, &(lop->ldap_op));

        g_free (base);
        
        if (r != LDAP_SUCCESS) {
            fail_ldap_operation (lop, r);
            return FALSE;
        }                                    
                
        lop->ldap_cb = send_callback;
        return TRUE;   
    }
    
    /* At this point we're done */
    bastile_operation_mark_done (op, FALSE, NULL);
    return FALSE;
}

/* Start a key send operation for multiple keys */
static BastileLDAPOperation *
start_send_operation_multiple (BastileLDAPSource *lsrc, GSList *keys)
{
    BastileLDAPOperation *lop;

    g_assert (g_slist_length (keys) > 0);
    
    lop = bastile_ldap_operation_start (lsrc, send_key_to_ldap, 
                                         g_slist_length (keys));
    g_return_val_if_fail (lop != NULL, NULL);

    g_object_set_data (G_OBJECT (lop), "key-data", keys);
    g_object_set_data_full (G_OBJECT (lop), "key-data-full", keys, 
                            (GDestroyNotify)bastile_util_string_slist_free);

    return lop;
}

/* -----------------------------------------------------------------------------
 *  BASTILE LDAP SOURCE
 */
 
enum {
    PROP_0,
    PROP_SOURCE_TAG,
    PROP_SOURCE_LOCATION
};

static void bastile_source_iface (BastileSourceIface *iface);

G_DEFINE_TYPE_EXTENDED (BastileLDAPSource, bastile_ldap_source, BASTILE_TYPE_SERVER_SOURCE, 0,
                        G_IMPLEMENT_INTERFACE (BASTILE_TYPE_SOURCE, bastile_source_iface));

static void 
bastile_ldap_source_init (BastileLDAPSource *lsrc)
{

}

static void 
bastile_ldap_source_get_property (GObject *object, guint prop_id, GValue *value,
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
bastile_ldap_source_set_property (GObject *object, guint prop_id, const GValue *value,
                                   GParamSpec *pspec)
{

}

static BastileOperation*
bastile_ldap_source_search (BastileSource *src, const gchar *match)
{
    BastileLDAPOperation *lop = NULL;

    g_assert (BASTILE_IS_SOURCE (src));
    g_assert (BASTILE_IS_LDAP_SOURCE (src));

    /* Search for keys */
    lop = start_search_operation (BASTILE_LDAP_SOURCE (src), match);
     
    g_return_val_if_fail (lop != NULL, NULL);
    bastile_server_source_take_operation (BASTILE_SERVER_SOURCE (src),
                                           BASTILE_OPERATION (lop));
    g_object_ref (lop);
    return BASTILE_OPERATION (lop);
}

static BastileOperation* 
bastile_ldap_source_import (BastileSource *sksrc, GInputStream *input)
{
    BastileLDAPOperation *lop;
    BastileLDAPSource *lsrc;
    GSList *keydata = NULL;
    GString *buf = NULL;
    guint len;
    
    	g_return_val_if_fail (BASTILE_IS_LDAP_SOURCE (sksrc), NULL);
    	lsrc = BASTILE_LDAP_SOURCE (sksrc);
    
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
    
    keydata = g_slist_reverse (keydata);
    
    lop = start_send_operation_multiple (lsrc, keydata);
    g_return_val_if_fail (lop != NULL, NULL);
    
    	g_object_unref (input);
    	return BASTILE_OPERATION (lop);
}

static BastileOperation* 
bastile_ldap_source_export_raw (BastileSource *sksrc, GSList *keyids, 
                                 GOutputStream *output)
{
    BastileLDAPOperation *lop;
    BastileLDAPSource *lsrc;
    GSList *l, *fingerprints = NULL;
    
    g_return_val_if_fail (BASTILE_IS_LDAP_SOURCE (sksrc), NULL);
    g_return_val_if_fail (G_IS_OUTPUT_STREAM (output), NULL);

    lsrc = BASTILE_LDAP_SOURCE (sksrc);
    
    for (l = keyids; l; l = g_slist_next (l)) 
        fingerprints = g_slist_prepend (fingerprints, 
            g_strdup (bastile_pgp_key_calc_rawid (GPOINTER_TO_UINT (l->data))));
    fingerprints = g_slist_reverse (fingerprints);

    lop = start_get_operation_multiple (lsrc, fingerprints, output);
    g_return_val_if_fail (lop != NULL, NULL);
    
    return BASTILE_OPERATION (lop);    
}

/* Initialize the basic class stuff */
static void
bastile_ldap_source_class_init (BastileLDAPSourceClass *klass)
{
	GObjectClass *gobject_class;
   
	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->get_property = bastile_ldap_source_get_property;
	gobject_class->set_property = bastile_ldap_source_set_property;

	bastile_ldap_source_parent_class = g_type_class_peek_parent (klass);
    
	g_object_class_override_property (gobject_class, PROP_SOURCE_TAG, "source-tag");
	g_object_class_override_property (gobject_class, PROP_SOURCE_LOCATION, "source-location");
	    
	bastile_registry_register_type (NULL, BASTILE_TYPE_LDAP_SOURCE, "source", "remote", BASTILE_PGP_STR, NULL);
	bastile_servers_register_type ("ldap", _("LDAP Key Server"), bastile_ldap_is_valid_uri);

	bastile_registry_register_function (NULL, bastile_pgp_key_canonize_id, "canonize", BASTILE_PGP_STR, NULL);
}

static void 
bastile_source_iface (BastileSourceIface *iface)
{
	iface->search = bastile_ldap_source_search;
	iface->import = bastile_ldap_source_import;
	iface->export_raw = bastile_ldap_source_export_raw;
}

/**
 * bastile_ldap_source_new
 * @uri: The server to connect to 
 * 
 * Creates a new key source for an LDAP PGP server.
 * 
 * Returns: A new LDAP Key Source
 */
BastileLDAPSource*   
bastile_ldap_source_new (const gchar* uri, const gchar *host)
{
    g_return_val_if_fail (bastile_ldap_is_valid_uri (uri), NULL);
    g_return_val_if_fail (host && *host, NULL);
    return g_object_new (BASTILE_TYPE_LDAP_SOURCE, "key-server", host, 
                         "uri", uri, NULL);
}

/**
 * bastile_ldap_is_valid_uri
 * @uri: The uri to check
 * 
 * Returns: Whether the passed uri is valid for an ldap key source
 */
gboolean              
bastile_ldap_is_valid_uri (const gchar *uri)
{
    LDAPURLDesc *url;
    int r;
    
    g_return_val_if_fail (uri && *uri, FALSE);
    
    r = ldap_url_parse (uri, &url);
    if (r == LDAP_URL_SUCCESS) {
        
        /* Some checks to make sure it's a simple URI */
        if (!(url->lud_host && url->lud_host[0]) || 
            (url->lud_dn && url->lud_dn[0]) || 
            (url->lud_attrs || url->lud_attrs))
            r = LDAP_URL_ERR_PARAM;
        
        ldap_free_urldesc (url);
    }
        
    return r == LDAP_URL_SUCCESS;
}

#endif /* WITH_LDAP */
