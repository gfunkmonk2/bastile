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

#include <glib/gi18n.h>

#include "bastile-source.h"
#include "bastile-gtkstock.h"
#include "bastile-util.h"

#include "common/bastile-object-list.h"

#include "pgp/bastile-pgp.h"
#include "pgp/bastile-pgp-key.h"
#include "pgp/bastile-pgp-uid.h"
#include "pgp/bastile-pgp-subkey.h"

enum {
	PROP_0,
	PROP_PHOTOS,
	PROP_SUBKEYS,
	PROP_UIDS,
	PROP_FINGERPRINT,
	PROP_VALIDITY,
	PROP_VALIDITY_STR,
	PROP_TRUST,
	PROP_TRUST_STR,
	PROP_EXPIRES,
	PROP_EXPIRES_STR,
	PROP_LENGTH,
	PROP_ALGO
};

G_DEFINE_TYPE (BastilePgpKey, bastile_pgp_key, BASTILE_TYPE_OBJECT);

struct _BastilePgpKeyPrivate {
	GList *uids;			/* All the UID objects */
	GList *subkeys;                 /* All the Subkey objects */
	GList *photos;                  /* List of photos */
};

/* -----------------------------------------------------------------------------
 * INTERNAL HELPERS
 */

static const gchar* 
calc_short_name (BastilePgpKey *self)
{
	GList *uids = bastile_pgp_key_get_uids (self);
	return uids ? bastile_pgp_uid_get_name (uids->data) : NULL;
}

static gchar* 
calc_name (BastilePgpKey *self)
{
	GList *uids = bastile_pgp_key_get_uids (self);
	return uids ? bastile_pgp_uid_calc_label (bastile_pgp_uid_get_name (uids->data),
	                                           bastile_pgp_uid_get_email (uids->data),
	                                           bastile_pgp_uid_get_comment (uids->data)) : g_strdup ("");
}

static gchar* 
calc_markup (BastilePgpKey *self, guint flags)
{
	GList *uids = bastile_pgp_key_get_uids (self);
	return uids ? bastile_pgp_uid_calc_markup (bastile_pgp_uid_get_name (uids->data),
	                                            bastile_pgp_uid_get_email (uids->data),
	                                            bastile_pgp_uid_get_comment (uids->data), flags) : g_strdup ("");
}

/* -----------------------------------------------------------------------------
 * OBJECT 
 */

static GList*
_bastile_pgp_key_get_uids (BastilePgpKey *self)
{
	g_return_val_if_fail (BASTILE_IS_PGP_KEY (self), NULL);
	return self->pv->uids;
}

static void
_bastile_pgp_key_set_uids (BastilePgpKey *self, GList *uids)
{
	guint index;
	GQuark id;
	GList *l;
	
	g_return_if_fail (BASTILE_IS_PGP_KEY (self));

	id = bastile_object_get_id (BASTILE_OBJECT (self));
	
	/* Remove the parent on each old one */
	for (l = self->pv->uids; l; l = g_list_next (l))
		bastile_object_set_parent (l->data, NULL);

	bastile_object_list_free (self->pv->uids);
	self->pv->uids = bastile_object_list_copy (uids);
	
	/* Set parent and source on each new one, except the first */
	for (l = self->pv->uids, index = 0; l; l = g_list_next (l), ++index) {
		g_object_set (l->data, "id", bastile_pgp_uid_calc_id (id, index), NULL);
		if (l != self->pv->uids)
			bastile_object_set_parent (l->data, BASTILE_OBJECT (self));
	}
	
	g_object_notify (G_OBJECT (self), "uids");
}

static GList*
_bastile_pgp_key_get_subkeys (BastilePgpKey *self)
{
	g_return_val_if_fail (BASTILE_IS_PGP_KEY (self), NULL);
	return self->pv->subkeys;
}

static void
_bastile_pgp_key_set_subkeys (BastilePgpKey *self, GList *subkeys)
{
	GQuark id;
	
	g_return_if_fail (BASTILE_IS_PGP_KEY (self));
	g_return_if_fail (subkeys);
	
	bastile_object_list_free (self->pv->subkeys);
	self->pv->subkeys = bastile_object_list_copy (subkeys);
	
	id = bastile_pgp_key_canonize_id (bastile_pgp_subkey_get_keyid (subkeys->data));
	g_object_set (self, "id", id, NULL); 
	
	g_object_notify (G_OBJECT (self), "subkeys");
}

static GList*
_bastile_pgp_key_get_photos (BastilePgpKey *self)
{
	g_return_val_if_fail (BASTILE_IS_PGP_KEY (self), NULL);
	return self->pv->photos;
}

static void
_bastile_pgp_key_set_photos (BastilePgpKey *self, GList *photos)
{
	g_return_if_fail (BASTILE_IS_PGP_KEY (self));
	
	bastile_object_list_free (self->pv->photos);
	self->pv->photos = bastile_object_list_copy (photos);
	
	g_object_notify (G_OBJECT (self), "photos");
}

static void
bastile_pgp_key_realize (BastileObject *obj)
{
	BastilePgpKey *self = BASTILE_PGP_KEY (obj);
	const gchar *nickname, *keyid;
	const gchar *description, *icon;
	gchar *markup, *name, *identifier;
	BastileUsage usage;
	GList *subkeys;
	
	
	BASTILE_OBJECT_CLASS (bastile_pgp_key_parent_class)->realize (obj);
	
	subkeys = bastile_pgp_key_get_subkeys (self);
	if (subkeys) {
		keyid = bastile_pgp_subkey_get_keyid (subkeys->data);
		identifier = bastile_pgp_key_calc_identifier (keyid);
	} else {
		identifier = g_strdup ("");
	}

	name = calc_name (self);
	markup = calc_markup (self, bastile_object_get_flags (obj));
	nickname = calc_short_name (self);
	
	g_object_get (obj, "usage", &usage, NULL);
		
	/* The type */
	if (usage == BASTILE_USAGE_PRIVATE_KEY) {
		description = _("Private PGP Key");
		icon = BASTILE_STOCK_SECRET;
	} else {
		description = _("Public PGP Key");
		icon = BASTILE_STOCK_KEY;
		if (usage == BASTILE_USAGE_NONE)
			g_object_set (obj, "usage", BASTILE_USAGE_PUBLIC_KEY, NULL);
	}
	
	g_object_set (obj,
		      "label", name,
		      "markup", markup,
		      "nickname", nickname,
		      "identifier", identifier,
		      "description", description,
		      "icon", icon,
		      NULL);
		
	g_free (identifier);
	g_free (markup);
	g_free (name);
}

static void
bastile_pgp_key_init (BastilePgpKey *self)
{
	self->pv = G_TYPE_INSTANCE_GET_PRIVATE (self, BASTILE_TYPE_PGP_KEY, BastilePgpKeyPrivate);
}

static void
bastile_pgp_key_get_property (GObject *object, guint prop_id,
                               GValue *value, GParamSpec *pspec)
{
	BastilePgpKey *self = BASTILE_PGP_KEY (object);
    
	switch (prop_id) {
	case PROP_PHOTOS:
		g_value_set_boxed (value, bastile_pgp_key_get_photos (self));
		break;
	case PROP_SUBKEYS:
		g_value_set_boxed (value, bastile_pgp_key_get_subkeys (self));
		break;
	case PROP_UIDS:
		g_value_set_boxed (value, bastile_pgp_key_get_uids (self));
		break;
	case PROP_FINGERPRINT:
		g_value_set_string (value, bastile_pgp_key_get_fingerprint (self));
		break;
	case PROP_VALIDITY_STR:
		g_value_set_string (value, bastile_pgp_key_get_validity_str (self));
		break;
	case PROP_TRUST_STR:
		g_value_set_string (value, bastile_pgp_key_get_trust_str (self));
		break;
	case PROP_EXPIRES:
		g_value_set_ulong (value, bastile_pgp_key_get_expires (self));
		break;
	case PROP_EXPIRES_STR:
		g_value_take_string (value, bastile_pgp_key_get_expires_str (self));
		break;
	case PROP_LENGTH:
		g_value_set_uint (value, bastile_pgp_key_get_length (self));
		break;
	case PROP_ALGO:
		g_value_set_string (value, bastile_pgp_key_get_algo (self));
		break;
	case PROP_VALIDITY:
		g_value_set_uint (value, BASTILE_VALIDITY_UNKNOWN);
		break;
	case PROP_TRUST:
		g_value_set_uint (value, BASTILE_VALIDITY_UNKNOWN);
		break;
	}
}

static void
bastile_pgp_key_set_property (GObject *object, guint prop_id, const GValue *value, 
                               GParamSpec *pspec)
{
	BastilePgpKey *self = BASTILE_PGP_KEY (object);
    
	switch (prop_id) {
	case PROP_UIDS:
		bastile_pgp_key_set_uids (self, g_value_get_boxed (value));
		break;
	case PROP_SUBKEYS:
		bastile_pgp_key_set_subkeys (self, g_value_get_boxed (value));
		break;
	case PROP_PHOTOS:
		bastile_pgp_key_set_photos (self, g_value_get_boxed (value));
		break;
	}
}

static void
bastile_pgp_key_object_dispose (GObject *obj)
{
	BastilePgpKey *self = BASTILE_PGP_KEY (obj);

	GList *l;
	
	/* Free all the attached UIDs */
	for (l = self->pv->uids; l; l = g_list_next (l))
		bastile_object_set_parent (l->data, NULL);

	bastile_object_list_free (self->pv->uids);
	self->pv->uids = NULL;

	/* Free all the attached Photos */
	bastile_object_list_free (self->pv->photos);
	self->pv->photos = NULL;
	
	/* Free all the attached Subkeys */
	bastile_object_list_free (self->pv->subkeys);
	self->pv->subkeys = NULL;

	G_OBJECT_CLASS (bastile_pgp_key_parent_class)->dispose (obj);
}

static void
bastile_pgp_key_object_finalize (GObject *obj)
{
	BastilePgpKey *self = BASTILE_PGP_KEY (obj);

	g_assert (self->pv->uids == NULL);
	g_assert (self->pv->photos == NULL);
	g_assert (self->pv->subkeys == NULL);

	G_OBJECT_CLASS (bastile_pgp_key_parent_class)->finalize (obj);
}

static void
bastile_pgp_key_class_init (BastilePgpKeyClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	BastileObjectClass *bastile_class = BASTILE_OBJECT_CLASS (klass);
	
	bastile_pgp_key_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (BastilePgpKeyPrivate));

	gobject_class->dispose = bastile_pgp_key_object_dispose;
	gobject_class->finalize = bastile_pgp_key_object_finalize;
	gobject_class->set_property = bastile_pgp_key_set_property;
	gobject_class->get_property = bastile_pgp_key_get_property;
	
	bastile_class->realize = bastile_pgp_key_realize;
	
	klass->get_uids = _bastile_pgp_key_get_uids;
	klass->set_uids = _bastile_pgp_key_set_uids;
	klass->get_subkeys = _bastile_pgp_key_get_subkeys;
	klass->set_subkeys = _bastile_pgp_key_set_subkeys;
	klass->get_photos = _bastile_pgp_key_get_photos;
	klass->set_photos = _bastile_pgp_key_set_photos;
	
	g_object_class_install_property (gobject_class, PROP_PHOTOS,
                g_param_spec_boxed ("photos", "Key Photos", "Photos for the key",
                                    BASTILE_BOXED_OBJECT_LIST, G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class, PROP_SUBKEYS,
                g_param_spec_boxed ("subkeys", "PGP subkeys", "PGP subkeys",
                                    BASTILE_BOXED_OBJECT_LIST, G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class, PROP_UIDS,
                g_param_spec_boxed ("uids", "PGP User Ids", "PGP User Ids",
                                    BASTILE_BOXED_OBJECT_LIST, G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_FINGERPRINT,
                g_param_spec_string ("fingerprint", "Fingerprint", "Unique fingerprint for this key",
                                     "", G_PARAM_READABLE));

	g_object_class_install_property (gobject_class, PROP_VALIDITY,
	        g_param_spec_uint ("validity", "Validity", "Validity of this key",
                                   0, G_MAXUINT, 0, G_PARAM_READABLE));

        g_object_class_install_property (gobject_class, PROP_VALIDITY_STR,
                g_param_spec_string ("validity-str", "Validity String", "Validity of this key as a string",
                                     "", G_PARAM_READABLE));

        g_object_class_install_property (gobject_class, PROP_TRUST,
                g_param_spec_uint ("trust", "Trust", "Trust in this key",
                                   0, G_MAXUINT, 0, G_PARAM_READABLE));

        g_object_class_install_property (gobject_class, PROP_TRUST_STR,
                g_param_spec_string ("trust-str", "Trust String", "Trust in this key as a string",
                                     "", G_PARAM_READABLE));

        g_object_class_install_property (gobject_class, PROP_EXPIRES,
                g_param_spec_ulong ("expires", "Expires On", "Date this key expires on",
                                    0, G_MAXULONG, 0, G_PARAM_READABLE));

 	g_object_class_install_property (gobject_class, PROP_EXPIRES_STR,
 	        g_param_spec_string ("expires-str", "Expires String", "Readable expiry date",
 	                             "", G_PARAM_READABLE));

 	g_object_class_install_property (gobject_class, PROP_LENGTH,
 	        g_param_spec_uint ("length", "Length", "The length of this key.",
 	                           0, G_MAXUINT, 0, G_PARAM_READABLE));
 	
 	g_object_class_install_property (gobject_class, PROP_ALGO,
 	        g_param_spec_string ("algo", "Algorithm", "The algorithm of this key.",
 	                             "", G_PARAM_READABLE));
}


/* -----------------------------------------------------------------------------
 * PUBLIC 
 */

GQuark
bastile_pgp_key_canonize_id (const gchar *keyid)
{
	gchar *str;
	GQuark id;
	
	str = bastile_pgp_key_calc_id (keyid, 0);
	g_return_val_if_fail (str, 0);
	
	id = g_quark_from_string (str);
	g_free (str);
	
	return id;
}

gchar*
bastile_pgp_key_calc_id (const gchar *keyid, guint index)
{
	guint len;
	
	g_return_val_if_fail (keyid, 0);
	len = strlen (keyid);
    
	if (len < 16)
		g_message ("invalid keyid (less than 16 chars): %s", keyid);
    
	if (len > 16)
		keyid += len - 16;
    
	if (index == 0)
		return g_strdup_printf ("%s:%s", BASTILE_PGP_STR, keyid);
	else
		return g_strdup_printf ("%s:%s:%u", BASTILE_PGP_STR, keyid, index);
}

const gchar* 
bastile_pgp_key_calc_rawid (GQuark id)
{
	const gchar* keyid, *strid;
	
	strid = g_quark_to_string (id);
	g_return_val_if_fail (strid != NULL, NULL);
	
	keyid = strchr (strid, ':');
	return keyid ? keyid + 1 : strid;
}

gchar*
bastile_pgp_key_calc_identifier (const gchar *keyid)
{
	guint len;
	
	g_return_val_if_fail (keyid, NULL);
	
	len = strlen (keyid);
	if (len > 8)
		keyid += len - 8;
	
	return g_strdup (keyid);
}

BastilePgpKey*
bastile_pgp_key_new (void)
{
	return g_object_new (BASTILE_TYPE_PGP_KEY, NULL);
}

GList*
bastile_pgp_key_get_uids (BastilePgpKey *self)
{
	g_return_val_if_fail (BASTILE_IS_PGP_KEY (self), NULL);
	if (!BASTILE_PGP_KEY_GET_CLASS (self)->get_uids)
		return NULL;
	return BASTILE_PGP_KEY_GET_CLASS (self)->get_uids (self);
}

void
bastile_pgp_key_set_uids (BastilePgpKey *self, GList *uids)
{
	g_return_if_fail (BASTILE_IS_PGP_KEY (self));
	g_return_if_fail (BASTILE_PGP_KEY_GET_CLASS (self)->set_uids);
	BASTILE_PGP_KEY_GET_CLASS (self)->set_uids (self, uids);
}

GList*
bastile_pgp_key_get_subkeys (BastilePgpKey *self)
{
	g_return_val_if_fail (BASTILE_IS_PGP_KEY (self), NULL);
	if (!BASTILE_PGP_KEY_GET_CLASS (self)->get_subkeys)
		return NULL;
	return BASTILE_PGP_KEY_GET_CLASS (self)->get_subkeys (self);
}

void
bastile_pgp_key_set_subkeys (BastilePgpKey *self, GList *subkeys)
{
	g_return_if_fail (BASTILE_IS_PGP_KEY (self));
	g_return_if_fail (BASTILE_PGP_KEY_GET_CLASS (self)->set_subkeys);
	BASTILE_PGP_KEY_GET_CLASS (self)->set_subkeys (self, subkeys);
}

GList*
bastile_pgp_key_get_photos (BastilePgpKey *self)
{
	g_return_val_if_fail (BASTILE_IS_PGP_KEY (self), NULL);
	if (!BASTILE_PGP_KEY_GET_CLASS (self)->get_photos)
		return NULL;
	return BASTILE_PGP_KEY_GET_CLASS (self)->get_photos (self);
}

void
bastile_pgp_key_set_photos (BastilePgpKey *self, GList *photos)
{
	g_return_if_fail (BASTILE_IS_PGP_KEY (self));
	g_return_if_fail (BASTILE_PGP_KEY_GET_CLASS (self)->set_photos);
	BASTILE_PGP_KEY_GET_CLASS (self)->set_photos (self, photos);
}

const gchar*
bastile_pgp_key_get_fingerprint (BastilePgpKey *self)
{
	GList *subkeys;
	    
	g_return_val_if_fail (BASTILE_IS_PGP_KEY (self), NULL);

	subkeys = bastile_pgp_key_get_subkeys (self);
	if (!subkeys)
		return "";
	
	return bastile_pgp_subkey_get_fingerprint (subkeys->data);
}

BastileValidity
bastile_pgp_key_get_validity (BastilePgpKey *self)
{
	guint validity = BASTILE_VALIDITY_UNKNOWN;
	g_object_get (self, "validity", &validity, NULL);
	return validity;
}

const gchar*
bastile_pgp_key_get_validity_str (BastilePgpKey *self)
{
	return bastile_validity_get_string (bastile_pgp_key_get_validity (self));
}

gulong
bastile_pgp_key_get_expires (BastilePgpKey *self)
{
	GList *subkeys;
	    
	g_return_val_if_fail (BASTILE_IS_PGP_KEY (self), 0);

	subkeys = bastile_pgp_key_get_subkeys (self);
	if (!subkeys)
		return 0;
	
	return bastile_pgp_subkey_get_expires (subkeys->data);
}

gchar*
bastile_pgp_key_get_expires_str (BastilePgpKey *self)
{
	GTimeVal timeval;
	gulong expires;
	
	g_return_val_if_fail (BASTILE_IS_PGP_KEY (self), NULL);
	
	expires = bastile_pgp_key_get_expires (self);
	if (expires == 0)
		return g_strdup ("");
	
	g_get_current_time (&timeval);
	if (timeval.tv_sec > expires)
		return g_strdup (_("Expired"));
	
	return bastile_util_get_date_string (expires);
}

BastileValidity
bastile_pgp_key_get_trust (BastilePgpKey *self)
{
	guint trust = BASTILE_VALIDITY_UNKNOWN;
	g_object_get (self, "trust", &trust, NULL);
	return trust;
}

const gchar*
bastile_pgp_key_get_trust_str (BastilePgpKey *self)
{
	return bastile_validity_get_string (bastile_pgp_key_get_trust (self));
}

guint
bastile_pgp_key_get_length (BastilePgpKey *self)
{
	GList *subkeys;
	    
	g_return_val_if_fail (BASTILE_IS_PGP_KEY (self), 0);

	subkeys = bastile_pgp_key_get_subkeys (self);
	if (!subkeys)
		return 0;
	
	return bastile_pgp_subkey_get_length (subkeys->data);
}

const gchar*
bastile_pgp_key_get_algo (BastilePgpKey *self)
{
	GList *subkeys;
	    
	g_return_val_if_fail (BASTILE_IS_PGP_KEY (self), 0);

	subkeys = bastile_pgp_key_get_subkeys (self);
	if (!subkeys)
		return 0;
	
	return bastile_pgp_subkey_get_algorithm (subkeys->data);
}

const gchar*
bastile_pgp_key_get_keyid (BastilePgpKey *self)
{
	GList *subkeys;
	    
	g_return_val_if_fail (BASTILE_IS_PGP_KEY (self), 0);

	subkeys = bastile_pgp_key_get_subkeys (self);
	if (!subkeys)
		return 0;
	
	return bastile_pgp_subkey_get_keyid (subkeys->data);
}

gboolean
bastile_pgp_key_has_keyid (BastilePgpKey *self, const gchar *match)
{
	GList *subkeys, *l;
	BastilePgpSubkey *subkey;
	const gchar *keyid;
	guint n_match, n_keyid;
	
	g_return_val_if_fail (BASTILE_IS_PGP_KEY (self), FALSE);
	g_return_val_if_fail (match, FALSE);

	subkeys = bastile_pgp_key_get_subkeys (self);
	if (!subkeys)
		return FALSE;
	
	n_match = strlen (match);
	
	for (l = subkeys; l && (subkey = BASTILE_PGP_SUBKEY (l->data)); l = g_list_next (l)) {
		keyid = bastile_pgp_subkey_get_keyid (subkey);
		g_return_val_if_fail (keyid, FALSE);
		n_keyid = strlen (keyid);
		if (n_match <= n_keyid) {
			keyid += (n_keyid - n_match);
			if (strncmp (keyid, match, n_match) == 0)
				return TRUE;
		}
	}
	
	return FALSE;
}
