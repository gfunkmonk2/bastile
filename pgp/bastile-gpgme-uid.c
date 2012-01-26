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

#include "config.h"

#include "bastile-gpgme.h"
#include "bastile-gpgme-uid.h"
#include "bastile-pgp.h"
#include "bastile-pgp-key.h"
#include "bastile-pgp-signature.h"

#include "common/bastile-object-list.h"

#include <string.h>

#include <glib/gi18n.h>

enum {
	PROP_0,
	PROP_PUBKEY,
	PROP_USERID,
	PROP_GPGME_INDEX,
	PROP_ACTUAL_INDEX
};

G_DEFINE_TYPE (BastileGpgmeUid, bastile_gpgme_uid, BASTILE_TYPE_PGP_UID);

struct _BastileGpgmeUidPrivate {
	gpgme_key_t pubkey;         /* The public key that this uid is part of */
	gpgme_user_id_t userid;     /* The userid referred to */
	guint gpgme_index;          /* The GPGME index of the UID */
	gint actual_index;          /* The actual index of this UID */
};

/* -----------------------------------------------------------------------------
 * INTERNAL HELPERS
 */

static gchar*
convert_string (const gchar *str)
{
	if (!str)
  	      return NULL;
    
	/* If not utf8 valid, assume latin 1 */
 	if (!g_utf8_validate (str, -1, NULL))
 		return g_convert (str, -1, "UTF-8", "ISO-8859-1", NULL, NULL, NULL);

	return g_strdup (str);
}

static void
realize_signatures (BastileGpgmeUid *self)
{
	gpgme_key_sig_t gsig;
	BastilePgpSignature *sig;
	GList *sigs = NULL;
	guint flags;
	
	g_return_if_fail (self->pv->pubkey);
	g_return_if_fail (self->pv->userid);
	
	/* If this key was loaded without signatures, then leave them as is */
	if ((self->pv->pubkey->keylist_mode & GPGME_KEYLIST_MODE_SIGS) == 0) 
		return;
	
	for (gsig = self->pv->userid->signatures; gsig; gsig = gsig->next) {
		sig = bastile_pgp_signature_new (gsig->keyid);
		
		/* Order of parsing these flags is important */
		flags = 0;
		if (gsig->revoked)
			flags |= BASTILE_FLAG_REVOKED;
		if (gsig->expired)
			flags |= BASTILE_FLAG_EXPIRED;
		if (flags == 0 && !gsig->invalid)
			flags = BASTILE_FLAG_IS_VALID;
		if (gsig->exportable)
			flags |= BASTILE_FLAG_EXPORTABLE;
	
		bastile_pgp_signature_set_flags (sig, flags);
		sigs = g_list_prepend (sigs, sig);
	}
	
	bastile_pgp_uid_set_signatures (BASTILE_PGP_UID (self), sigs);
	bastile_object_list_free (sigs);
}

static gboolean
compare_strings (const gchar *a, const gchar *b)
{
	if (a == b)
		return TRUE;
	if (!a || !b)
		return FALSE;
	return g_str_equal (a, b);
}

static gboolean
compare_pubkeys (gpgme_key_t a, gpgme_key_t b)
{
	g_assert (a);
	g_assert (b);
	
	g_return_val_if_fail (a->subkeys, FALSE);
	g_return_val_if_fail (b->subkeys, FALSE);
	
	return compare_strings (a->subkeys->keyid, b->subkeys->keyid);
}

/* -----------------------------------------------------------------------------
 * OBJECT 
 */

static void
bastile_gpgme_uid_init (BastileGpgmeUid *self)
{
	self->pv = G_TYPE_INSTANCE_GET_PRIVATE (self, BASTILE_TYPE_GPGME_UID, BastileGpgmeUidPrivate);
	self->pv->gpgme_index = 0;
	self->pv->actual_index = -1;
}

static GObject*
bastile_gpgme_uid_constructor (GType type, guint n_props, GObjectConstructParam *props)
{
	GObject *obj = G_OBJECT_CLASS (bastile_gpgme_uid_parent_class)->constructor (type, n_props, props);
	BastileGpgmeUid *self = NULL;
	
	if (obj) {
		self = BASTILE_GPGME_UID (obj);
		g_object_set (self, "location", BASTILE_LOCATION_LOCAL, NULL);
	}
	
	return obj;
}

static void
bastile_gpgme_uid_get_property (GObject *object, guint prop_id,
                               GValue *value, GParamSpec *pspec)
{
	BastileGpgmeUid *self = BASTILE_GPGME_UID (object);
	
	switch (prop_id) {
	case PROP_PUBKEY:
		g_value_set_boxed (value, bastile_gpgme_uid_get_pubkey (self));
		break;
	case PROP_USERID:
		g_value_set_pointer (value, bastile_gpgme_uid_get_userid (self));
		break;
	case PROP_GPGME_INDEX:
		g_value_set_uint (value, bastile_gpgme_uid_get_gpgme_index (self));
		break;
	case PROP_ACTUAL_INDEX:
		g_value_set_uint (value, bastile_gpgme_uid_get_actual_index (self));
		break;
	}
}

static void
bastile_gpgme_uid_set_property (GObject *object, guint prop_id, const GValue *value, 
                               GParamSpec *pspec)
{
	BastileGpgmeUid *self = BASTILE_GPGME_UID (object);
	gpgme_key_t pubkey;
	
	switch (prop_id) {
	case PROP_PUBKEY:
		pubkey = g_value_get_boxed (value);
		g_return_if_fail (pubkey);

		if (pubkey != self->pv->pubkey) {
			
			if (self->pv->pubkey) {
				/* Should always be set to the same actual key */
				g_return_if_fail (compare_pubkeys (pubkey, self->pv->pubkey));
				gpgme_key_unref (self->pv->pubkey);
			}
			
			self->pv->pubkey = g_value_get_boxed (value);
			if (self->pv->pubkey)
				gpgme_key_ref (self->pv->pubkey);
			
			/* This is expected to be set shortly along with pubkey */
			self->pv->userid = NULL;
		}
		break;
	case PROP_ACTUAL_INDEX:
		bastile_gpgme_uid_set_actual_index (self, g_value_get_uint (value));
		break;
	case PROP_USERID:
		bastile_gpgme_uid_set_userid (self, g_value_get_pointer (value));
		break;
	}
}

static void
bastile_gpgme_uid_object_finalize (GObject *gobject)
{
	BastileGpgmeUid *self = BASTILE_GPGME_UID (gobject);

	/* Unref the key */
	if (self->pv->pubkey)
		gpgme_key_unref (self->pv->pubkey);
	self->pv->pubkey = NULL;
	self->pv->userid = NULL;
    
	G_OBJECT_CLASS (bastile_gpgme_uid_parent_class)->finalize (gobject);
}

static void
bastile_gpgme_uid_class_init (BastileGpgmeUidClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);    

	bastile_gpgme_uid_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (BastileGpgmeUidPrivate));

	gobject_class->constructor = bastile_gpgme_uid_constructor;
	gobject_class->finalize = bastile_gpgme_uid_object_finalize;
	gobject_class->set_property = bastile_gpgme_uid_set_property;
	gobject_class->get_property = bastile_gpgme_uid_get_property;
    
	g_object_class_install_property (gobject_class, PROP_PUBKEY,
	        g_param_spec_boxed ("pubkey", "Public Key", "GPGME Public Key that this uid is on",
	                            BASTILE_GPGME_BOXED_KEY, G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_USERID,
	        g_param_spec_pointer ("userid", "User ID", "GPGME User ID",
	                              G_PARAM_READWRITE));
                      
	g_object_class_install_property (gobject_class, PROP_GPGME_INDEX,
	        g_param_spec_uint ("gpgme-index", "GPGME Index", "GPGME User ID Index",
	                           0, G_MAXUINT, 0, G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class, PROP_ACTUAL_INDEX,
	        g_param_spec_uint ("actual-index", "Actual Index", "Actual GPG Index",
	                           0, G_MAXUINT, 0, G_PARAM_READWRITE));
}

/* -----------------------------------------------------------------------------
 * PUBLIC 
 */

BastileGpgmeUid* 
bastile_gpgme_uid_new (gpgme_key_t pubkey, gpgme_user_id_t userid) 
{
	return g_object_new (BASTILE_TYPE_GPGME_UID, 
	                     "pubkey", pubkey, 
	                     "userid", userid, NULL);
}

gpgme_key_t
bastile_gpgme_uid_get_pubkey (BastileGpgmeUid *self)
{
	g_return_val_if_fail (BASTILE_IS_GPGME_UID (self), NULL);
	g_return_val_if_fail (self->pv->pubkey, NULL);
	return self->pv->pubkey;
}

gpgme_user_id_t
bastile_gpgme_uid_get_userid (BastileGpgmeUid *self)
{
	g_return_val_if_fail (BASTILE_IS_GPGME_UID (self), NULL);
	g_return_val_if_fail (self->pv->userid, NULL);
	return self->pv->userid;
}

void
bastile_gpgme_uid_set_userid (BastileGpgmeUid *self, gpgme_user_id_t userid)
{
	BastilePgpUid *base;
	GObject *obj;
	gpgme_user_id_t uid;
	gchar *string;
	gint index, i;
	
	g_return_if_fail (BASTILE_IS_GPGME_UID (self));
	g_return_if_fail (userid);
	
	if (self->pv->userid)
		g_return_if_fail (bastile_gpgme_uid_is_same (self, userid));
	
	/* Make sure that this userid is in the pubkey */
	index = -1;
	for (i = 0, uid = self->pv->pubkey->uids; uid; ++i, uid = uid->next) {
		if(userid == uid) {
			index = i;
			break;
		}
	}
	
	g_return_if_fail (index >= 0);
	
	self->pv->userid = userid;
	self->pv->gpgme_index = index;
	
	obj = G_OBJECT (self);
	g_object_freeze_notify (obj);
	g_object_notify (obj, "userid");
	g_object_notify (obj, "gpgme_index");
	
	base = BASTILE_PGP_UID (self);
	
	string = convert_string (userid->name);
	bastile_pgp_uid_set_name (base, string);
	g_free (string);
	
	string = convert_string (userid->email);
	bastile_pgp_uid_set_email (base, string);
	g_free (string);
	
	string = convert_string (userid->comment);
	bastile_pgp_uid_set_comment (base, string);
	g_free (string);
	
	realize_signatures (self);

	bastile_pgp_uid_set_validity (base, bastile_gpgme_convert_validity (userid->validity));

	g_object_thaw_notify (obj);
}

guint
bastile_gpgme_uid_get_gpgme_index (BastileGpgmeUid *self)
{
	g_return_val_if_fail (BASTILE_IS_GPGME_UID (self), 0);
	return self->pv->gpgme_index;
}

guint
bastile_gpgme_uid_get_actual_index (BastileGpgmeUid *self)
{
	g_return_val_if_fail (BASTILE_IS_GPGME_UID (self), 0);
	if(self->pv->actual_index < 0)
		return self->pv->gpgme_index + 1;
	return self->pv->actual_index;
}

void
bastile_gpgme_uid_set_actual_index (BastileGpgmeUid *self, guint actual_index)
{
	g_return_if_fail (BASTILE_IS_GPGME_UID (self));
	self->pv->actual_index = actual_index;
	g_object_notify (G_OBJECT (self), "actual-index");
}

gchar*
bastile_gpgme_uid_calc_label (gpgme_user_id_t userid)
{
	g_return_val_if_fail (userid, NULL);
	return convert_string (userid->uid);
}

gchar*
bastile_gpgme_uid_calc_name (gpgme_user_id_t userid)
{
	g_return_val_if_fail (userid, NULL);
	return convert_string (userid->name);
}

gchar*
bastile_gpgme_uid_calc_markup (gpgme_user_id_t userid, guint flags)
{
	gchar *email, *name, *comment, *ret;

	g_return_val_if_fail (userid, NULL);
	
	name = convert_string (userid->name);
	email = convert_string (userid->email);
	comment = convert_string (userid->comment);
	
	ret = bastile_pgp_uid_calc_markup (name, email, comment, flags);
	
	g_free (name);
	g_free (email);
	g_free (comment);
	
	return ret;
}

gboolean
bastile_gpgme_uid_is_same (BastileGpgmeUid *self, gpgme_user_id_t userid)
{
	g_return_val_if_fail (BASTILE_IS_GPGME_UID (self), FALSE);
	g_return_val_if_fail (userid, FALSE);
	
	return compare_strings (self->pv->userid->uid, userid->uid);	
}
