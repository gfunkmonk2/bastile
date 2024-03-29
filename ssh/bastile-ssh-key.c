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
#include <glib/gstdio.h>

#include <errno.h>

#include "bastile-context.h"
#include "bastile-source.h"
#include "bastile-ssh-source.h"
#include "bastile-ssh-key.h"
#include "bastile-ssh-operation.h"
#include "bastile-gtkstock.h"
#include "bastile-validity.h"

enum {
    PROP_0,
    PROP_KEY_DATA,
    PROP_FINGERPRINT,
    PROP_VALIDITY,
    PROP_TRUST,
    PROP_EXPIRES,
    PROP_LENGTH
};

G_DEFINE_TYPE (BastileSSHKey, bastile_ssh_key, BASTILE_TYPE_OBJECT);

/* -----------------------------------------------------------------------------
 * INTERNAL 
 */

static gchar*
parse_first_word (const gchar *line) 
{
    int pos;
    
    #define PARSE_CHARS "\t \n@;,.\\?()[]{}+/"
    line += strspn (line, PARSE_CHARS);
    pos = strcspn (line, PARSE_CHARS);
    return pos == 0 ? NULL : g_strndup (line, pos);
}

static void
changed_key (BastileSSHKey *self)
{
	BastileObject *obj = BASTILE_OBJECT (self);
	BastileUsage usage;
	const gchar *description = NULL;
	const gchar *display = NULL;
	gchar *identifier;
	gchar *simple = NULL;
    
	if (self->keydata) {

  		 /* Try to make display and simple names */
		if (self->keydata->comment) {
			display = self->keydata->comment;
			simple = parse_first_word (self->keydata->comment);
            
		/* No names when not even the fingerpint loaded */
		} else if (!self->keydata->fingerprint) {
			display = _("(Unreadable Secure Shell Key)");

		/* No comment, but loaded */        
		} else {
			display = _("Secure Shell Key");
		}
    
		if (simple == NULL)
			simple = g_strdup (_("Secure Shell Key"));
	}
    
	if (!self->keydata || !self->keydata->fingerprint) {
        
		g_object_set (self,
		              "id", 0,
		              "label", "",
		              "icon", NULL,
		              "usage", BASTILE_USAGE_NONE,
		              "description", _("Invalid"),		              
		              "nickname", "",
		              "location", BASTILE_LOCATION_INVALID,
		              "flags", BASTILE_FLAG_DISABLED,
		              NULL);
		return;
		
	} 

	if (self->keydata->privfile) {
		usage = BASTILE_USAGE_PRIVATE_KEY;
		description = _("Private Secure Shell Key");
	} else {
		usage = BASTILE_USAGE_PUBLIC_KEY;
		description = _("Public Secure Shell Key");
	}
	
	identifier = bastile_ssh_key_calc_identifier (self->keydata->fingerprint);

	g_object_set (obj,
	              "id", bastile_ssh_key_calc_cannonical_id (self->keydata->fingerprint),
	              "label", display,
	              "icon", BASTILE_STOCK_KEY_SSH,
	              "usage", usage,
	              "nickname", simple,
	              "description", description,
	              "location", BASTILE_LOCATION_LOCAL,
	              "identifier", identifier,
	              "flags", (self->keydata->authorized ? BASTILE_FLAG_TRUSTED : 0) | BASTILE_FLAG_EXPORTABLE,
	              NULL);
	
	g_free (identifier);
}

static guint 
calc_validity (BastileSSHKey *skey)
{
	if (skey->keydata->privfile)
		return BASTILE_VALIDITY_ULTIMATE;
	return 0;
}

static guint
calc_trust (BastileSSHKey *skey)
{
	if (skey->keydata->authorized)
		return BASTILE_VALIDITY_FULL;
	return 0;
}

/* -----------------------------------------------------------------------------
 * OBJECT 
 */

static void
bastile_ssh_key_refresh (BastileObject *sobj)
{
	/* TODO: Need to work on key refreshing */
	BASTILE_OBJECT_CLASS (bastile_ssh_key_parent_class)->refresh (sobj);
}

static BastileOperation*
bastile_ssh_key_delete (BastileObject *sobj)
{
	BastileSSHKeyData *keydata = NULL;
	gboolean ret = TRUE;
	GError *err = NULL;
    
	g_return_val_if_fail (BASTILE_IS_SSH_KEY (sobj), NULL);

	g_object_get (sobj, "key-data", &keydata, NULL);
	g_return_val_if_fail (keydata, FALSE);
    
	/* Just part of a file for this key */
	if (keydata->partial) {
	
		/* Take just that line out of the file */
		if (keydata->pubfile)
			bastile_ssh_key_data_filter_file (keydata->pubfile, NULL, keydata, &err);
	
	/* A full file for this key */
	} else {
	
		if (keydata->pubfile) {
			if (g_unlink (keydata->pubfile) == -1) {
				g_set_error (&err, G_FILE_ERROR, g_file_error_from_errno (errno), 
				             "%s", g_strerror (errno));
			}
		}

		if (ret && keydata->privfile) {
			if (g_unlink (keydata->privfile) == -1) {
				g_set_error (&err, G_FILE_ERROR, g_file_error_from_errno (errno), 
				             "%s", g_strerror (errno));
			}
		}
	}

	if (err == NULL)
		bastile_context_remove_object (SCTX_APP (), sobj);

	return bastile_operation_new_complete (err);
}

static void
bastile_ssh_key_get_property (GObject *object, guint prop_id,
                               GValue *value, GParamSpec *pspec)
{
    BastileSSHKey *skey = BASTILE_SSH_KEY (object);
    
    switch (prop_id) {
    case PROP_KEY_DATA:
        g_value_set_pointer (value, skey->keydata);
        break;
    case PROP_FINGERPRINT:
        g_value_set_string (value, skey->keydata ? skey->keydata->fingerprint : NULL);
        break;
    case PROP_VALIDITY:
        g_value_set_uint (value, calc_validity (skey));
        break;
    case PROP_TRUST:
        g_value_set_uint (value, calc_trust (skey));
        break;
    case PROP_EXPIRES:
        g_value_set_ulong (value, 0);
        break;
    case PROP_LENGTH:
        g_value_set_uint (value, skey->keydata ? skey->keydata->length : 0);
        break;
    }
}

static void
bastile_ssh_key_set_property (GObject *object, guint prop_id, 
                               const GValue *value, GParamSpec *pspec)
{
    BastileSSHKey *skey = BASTILE_SSH_KEY (object);
    BastileSSHKeyData *keydata;
    
    switch (prop_id) {
    case PROP_KEY_DATA:
        keydata = (BastileSSHKeyData*)g_value_get_pointer (value);
        if (skey->keydata != keydata) {
            bastile_ssh_key_data_free (skey->keydata);
            skey->keydata = keydata;
        }
        changed_key (skey);
        break;
    }
}

/* Unrefs gpgme key and frees data */
static void
bastile_ssh_key_finalize (GObject *gobject)
{
    BastileSSHKey *skey = BASTILE_SSH_KEY (gobject);
    
    bastile_ssh_key_data_free (skey->keydata);
    
    G_OBJECT_CLASS (bastile_ssh_key_parent_class)->finalize (gobject);
}

static void
bastile_ssh_key_init (BastileSSHKey *self)
{
	
}

static void
bastile_ssh_key_class_init (BastileSSHKeyClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    BastileObjectClass *bastile_class = BASTILE_OBJECT_CLASS (klass);
    
    bastile_ssh_key_parent_class = g_type_class_peek_parent (klass);
    
    gobject_class->finalize = bastile_ssh_key_finalize;
    gobject_class->set_property = bastile_ssh_key_set_property;
    gobject_class->get_property = bastile_ssh_key_get_property;
    
    bastile_class->delete = bastile_ssh_key_delete;
    bastile_class->refresh = bastile_ssh_key_refresh;
    
    g_object_class_install_property (gobject_class, PROP_KEY_DATA,
        g_param_spec_pointer ("key-data", "SSH Key Data", "SSH key data pointer",
                              G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_FINGERPRINT,
        g_param_spec_string ("fingerprint", "Fingerprint", "Unique fingerprint for this key",
                             "", G_PARAM_READABLE));

    g_object_class_install_property (gobject_class, PROP_VALIDITY,
        g_param_spec_uint ("validity", "Validity", "Validity of this key",
                           0, G_MAXUINT, 0, G_PARAM_READABLE));

    g_object_class_install_property (gobject_class, PROP_TRUST,
        g_param_spec_uint ("trust", "Trust", "Trust in this key",
                           0, G_MAXUINT, 0, G_PARAM_READABLE));

    g_object_class_install_property (gobject_class, PROP_EXPIRES,
        g_param_spec_ulong ("expires", "Expires On", "Date this key expires on",
                           0, G_MAXULONG, 0, G_PARAM_READABLE));

    g_object_class_install_property (gobject_class, PROP_LENGTH,
        g_param_spec_uint ("length", "Length of", "The length of this key",
                           0, G_MAXUINT, 0, G_PARAM_READABLE));
}

/* -----------------------------------------------------------------------------
 * PUBLIC METHODS
 */

BastileSSHKey* 
bastile_ssh_key_new (BastileSource *sksrc, BastileSSHKeyData *data)
{
    BastileSSHKey *skey;
    skey = g_object_new (BASTILE_TYPE_SSH_KEY, "source", sksrc, 
                         "key-data", data, NULL);
    return skey;
}

guint 
bastile_ssh_key_get_algo (BastileSSHKey *skey)
{
    g_return_val_if_fail (BASTILE_IS_SSH_KEY (skey), SSH_ALGO_UNK);
    return skey->keydata->algo;
}

const gchar*
bastile_ssh_key_get_algo_str (BastileSSHKey *skey)
{
    g_return_val_if_fail (BASTILE_IS_SSH_KEY (skey), "");
    
    switch(skey->keydata->algo) {
    case SSH_ALGO_UNK:
        return "";
    case SSH_ALGO_RSA:
        return "RSA";
    case SSH_ALGO_DSA:
        return "DSA";
    default:
        g_assert_not_reached ();
        return NULL;
    };
}

guint           
bastile_ssh_key_get_strength (BastileSSHKey *skey)
{
    g_return_val_if_fail (BASTILE_IS_SSH_KEY (skey), 0);
    return skey->keydata ? skey->keydata->length : 0;
}

const gchar*    
bastile_ssh_key_get_location (BastileSSHKey *skey)
{
    g_return_val_if_fail (BASTILE_IS_SSH_KEY (skey), NULL);
    if (!skey->keydata)
        return NULL;
    return skey->keydata->privfile ? 
                    skey->keydata->privfile : skey->keydata->pubfile;
}

GQuark
bastile_ssh_key_calc_cannonical_id (const gchar *id)
{
    #define SSH_ID_SIZE 16
    gchar *hex, *canonical_id = g_malloc0 (SSH_ID_SIZE + 1);
    gint i, off, len = strlen (id);
    GQuark ret;

    /* Strip out all non alpha numeric chars and limit length to SSH_ID_SIZE */
    for (i = len, off = SSH_ID_SIZE; i >= 0 && off > 0; --i) {
         if (g_ascii_isalnum (id[i]))
             canonical_id[--off] = g_ascii_toupper (id[i]);
    }
    
    /* Not enough characters */
    g_return_val_if_fail (off == 0, 0);

    hex = g_strdup_printf ("%s:%s", BASTILE_SSH_STR, canonical_id);
    ret = g_quark_from_string (hex);
    
    g_free (canonical_id);
    g_free (hex);
    
    return ret;
}


gchar*
bastile_ssh_key_calc_identifier (const gchar *id)
{
	#define SSH_IDENTIFIER_SIZE 8
	gchar *canonical_id = g_malloc0 (SSH_IDENTIFIER_SIZE + 1);
	gint i, off, len = strlen (id);

	/* Strip out all non alpha numeric chars and limit length to SSH_ID_SIZE */
	for (i = len, off = SSH_IDENTIFIER_SIZE; i >= 0 && off > 0; --i) {
		if (g_ascii_isalnum (id[i]))
			canonical_id[--off] = g_ascii_toupper (id[i]);
	}
    
	/* Not enough characters */
	g_return_val_if_fail (off == 0, NULL);

	return canonical_id;
}

gchar*
bastile_ssh_key_get_fingerprint (BastileSSHKey *self)
{
	gchar *fpr;
	g_object_get (self, "fingerprint", &fpr, NULL);
	return fpr;	
}

BastileValidity
bastile_ssh_key_get_trust (BastileSSHKey *self)
{
    guint validity;
    g_object_get (self, "trust", &validity, NULL);
    return validity;
}
