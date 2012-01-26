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

#include "bastile-context.h"
#include "bastile-source.h"
#include "bastile-gtkstock.h"
#include "bastile-util.h"

#include "common/bastile-object-list.h"

#include "pgp/bastile-pgp-key.h"
#include "pgp/bastile-gpgme.h"
#include "pgp/bastile-gpgme-key-op.h"
#include "pgp/bastile-gpgme-photo.h"
#include "pgp/bastile-gpgme-source.h"
#include "pgp/bastile-gpgme-uid.h"

enum {
	PROP_0,
	PROP_PUBKEY,
	PROP_SECKEY,
	PROP_VALIDITY,
	PROP_TRUST
};

G_DEFINE_TYPE (BastileGpgmeKey, bastile_gpgme_key, BASTILE_TYPE_PGP_KEY);

struct _BastileGpgmeKeyPrivate {
	gpgme_key_t pubkey;   		/* The public key */
	gpgme_key_t seckey;      	/* The secret key */
	gboolean has_secret;		/* Whether we have a secret key or not */

	GList *uids;			/* We keep a copy of the uids. */

	int list_mode;                  /* What to load our public key as */
	gboolean photos_loaded;		/* Photos were loaded */
	
	gint block_loading;        	/* Loading is blocked while this flag is set */
};

/* -----------------------------------------------------------------------------
 * INTERNAL HELPERS
 */

static gboolean 
load_gpgme_key (GQuark id, int mode, int secret, gpgme_key_t *key)
{
	GError *err = NULL;
	gpgme_ctx_t ctx;
	gpgme_error_t gerr;
	
	ctx = bastile_gpgme_source_new_context ();
	gpgme_set_keylist_mode (ctx, mode);
	gerr = gpgme_op_keylist_start (ctx, bastile_pgp_key_calc_rawid (id), secret);
	if (GPG_IS_OK (gerr)) {
		gerr = gpgme_op_keylist_next (ctx, key);
		gpgme_op_keylist_end (ctx);
	}

	gpgme_release (ctx);
	
	if (!GPG_IS_OK (gerr)) {
		bastile_gpgme_to_error (gerr, &err);
		g_message ("couldn't load GPGME key: %s", err->message);
		return FALSE;
	}
	
	return TRUE;
}

static void
load_key_public (BastileGpgmeKey *self, int list_mode)
{
	gpgme_key_t key = NULL;
	gboolean ret;
	GQuark id;
	
	if (self->pv->block_loading)
		return;
	
	list_mode |= self->pv->list_mode;
	
	id = bastile_object_get_id (BASTILE_OBJECT (self));
	g_return_if_fail (id);
	
	ret = load_gpgme_key (id, list_mode, FALSE, &key);
	if (ret) {
		self->pv->list_mode = list_mode;
		bastile_gpgme_key_set_public (self, key);
		gpgme_key_unref (key);
	}
}

static gboolean
require_key_public (BastileGpgmeKey *self, int list_mode)
{
	if (!self->pv->pubkey || (self->pv->list_mode & list_mode) != list_mode)
		load_key_public (self, list_mode);
	return self->pv->pubkey && (self->pv->list_mode & list_mode) == list_mode;
}

static void
load_key_private (BastileGpgmeKey *self)
{
	gpgme_key_t key = NULL;
	gboolean ret;
	GQuark id;
	
	if (!self->pv->has_secret || self->pv->block_loading)
		return;
	
	id = bastile_object_get_id (BASTILE_OBJECT (self));
	g_return_if_fail (id);
	
	ret = load_gpgme_key (id, GPGME_KEYLIST_MODE_LOCAL, TRUE, &key);
	if (ret) {
		bastile_gpgme_key_set_private (self, key);
		gpgme_key_unref (key);
	}
}

static gboolean
require_key_private (BastileGpgmeKey *self)
{
	if (!self->pv->seckey)
		load_key_private (self);
	return self->pv->seckey != NULL;
}

static gboolean
require_key_uids (BastileGpgmeKey *self)
{
	return require_key_public (self, GPGME_KEYLIST_MODE_LOCAL | GPGME_KEYLIST_MODE_SIGS);
}

static gboolean
require_key_subkeys (BastileGpgmeKey *self)
{
	return require_key_public (self, GPGME_KEYLIST_MODE_LOCAL);
}

static void
load_key_photos (BastileGpgmeKey *self)
{
	gpgme_error_t gerr;
	
	if (self->pv->block_loading)
		return;

	gerr = bastile_gpgme_key_op_photos_load (self);
	if (!GPG_IS_OK (gerr)) 
		g_message ("couldn't load key photos: %s", gpgme_strerror (gerr));
	else
		self->pv->photos_loaded = TRUE;
}

static gboolean
require_key_photos (BastileGpgmeKey *self)
{
	if (!self->pv->photos_loaded)
		load_key_photos (self);
	return self->pv->photos_loaded;
}

static void
renumber_actual_uids (BastileGpgmeKey *self)
{
	GArray *index_map;
	GList *photos, *uids, *l;
	guint index, i;
	
	g_assert (BASTILE_IS_GPGME_KEY (self));
	
	/* 
	 * This function is necessary because the uid stored in a gpgme_user_id_t
	 * struct is only usable with gpgme functions.  Problems will be caused if 
	 * that uid is used with functions found in bastile-gpgme-key-op.h.  This 
	 * function is only to be called with uids from gpgme_user_id_t structs.
	 */

	++self->pv->block_loading;
	photos = bastile_pgp_key_get_photos (BASTILE_PGP_KEY (self));
	uids = self->pv->uids;
	--self->pv->block_loading;
	
	/* First we build a bitmap of where all the photo uid indexes are */
	index_map = g_array_new (FALSE, TRUE, sizeof (gboolean));
	for (l = photos; l; l = g_list_next (l)) {
		index = bastile_gpgme_photo_get_index (l->data);
		if (index >= index_map->len)
			g_array_set_size (index_map, index + 1);
		g_array_index (index_map, gboolean, index) = TRUE;
	}
	
	/* Now for each UID we add however many photo indexes are below the gpgme index */
	for (l = uids; l; l = g_list_next (l)) {
		index = bastile_gpgme_uid_get_gpgme_index (l->data);
		for (i = 0; i < index_map->len && i < index; ++i) {
			if(g_array_index (index_map, gboolean, index))
				++index;
		}
		bastile_gpgme_uid_set_actual_index (l->data, index + 1);
	}
	
	g_array_free (index_map, TRUE);
}

static void
realize_uids (BastileGpgmeKey *self)
{
	gpgme_user_id_t guid;
	BastileGpgmeUid *uid;
	GList *results = NULL;
	gboolean changed = FALSE;
	GList *uids;

	uids = self->pv->uids;
	guid = self->pv->pubkey ? self->pv->pubkey->uids : NULL;

	/* Look for out of sync or missing UIDs */
	while (uids != NULL) {
		g_return_if_fail (BASTILE_IS_GPGME_UID (uids->data));
		uid = BASTILE_GPGME_UID (uids->data);
		uids = g_list_next (uids);

		/* Bring this UID up to date */
		if (guid && bastile_gpgme_uid_is_same (uid, guid)) {
			if (bastile_gpgme_uid_get_userid (uid) != guid) {
				g_object_set (uid, "pubkey", self->pv->pubkey, "userid", guid, NULL);
				changed = TRUE;
			}
			results = bastile_object_list_append (results, uid);
			guid = guid->next;
		}
	}

	/* Add new UIDs */
	while (guid != NULL) {
		uid = bastile_gpgme_uid_new (self->pv->pubkey, guid);
		changed = TRUE;
		results = bastile_object_list_append (results, uid);
		g_object_unref (uid);
		guid = guid->next;
	}

	if (changed)
		bastile_pgp_key_set_uids (BASTILE_PGP_KEY (self), results);
	bastile_object_list_free (results);
}

static void 
realize_subkeys (BastileGpgmeKey *self)
{
	gpgme_subkey_t gsubkey;
	BastileGpgmeSubkey *subkey;
	GList *list = NULL;
	
	if (self->pv->pubkey) {

		for (gsubkey = self->pv->pubkey->subkeys; gsubkey; gsubkey = gsubkey->next) {
			subkey = bastile_gpgme_subkey_new (self->pv->pubkey, gsubkey);
			list = bastile_object_list_prepend (list, subkey);
			g_object_unref (subkey);
		}
		
		list = g_list_reverse (list);
	}

	bastile_pgp_key_set_subkeys (BASTILE_PGP_KEY (self), list);
	bastile_object_list_free (list);
}

static void
refresh_each_object (BastileObject *object, gpointer data)
{
	bastile_object_refresh (object);
}

/* -----------------------------------------------------------------------------
 * OBJECT 
 */

static void
bastile_gpgme_key_realize (BastileObject *obj)
{
	BastileGpgmeKey *self = BASTILE_GPGME_KEY (obj);
	BastileUsage usage;
	guint flags;
	
	if (!self->pv->pubkey)
		return;
	
	if (!require_key_public (self, GPGME_KEYLIST_MODE_LOCAL))
		g_return_if_reached ();
	
	g_return_if_fail (self->pv->pubkey);
	g_return_if_fail (self->pv->pubkey->subkeys);

	/* Update the sub UIDs */
	realize_uids (self);
	realize_subkeys (self);

	/* The flags */
	flags = BASTILE_FLAG_EXPORTABLE;

	if (!self->pv->pubkey->disabled && !self->pv->pubkey->expired && 
	    !self->pv->pubkey->revoked && !self->pv->pubkey->invalid) {
		if (bastile_gpgme_key_get_validity (self) >= BASTILE_VALIDITY_MARGINAL)
			flags |= BASTILE_FLAG_IS_VALID;
		if (self->pv->pubkey->can_encrypt)
			flags |= BASTILE_FLAG_CAN_ENCRYPT;
		if (self->pv->seckey && self->pv->pubkey->can_sign)
			flags |= BASTILE_FLAG_CAN_SIGN;
	}

	if (self->pv->pubkey->expired)
		flags |= BASTILE_FLAG_EXPIRED;

	if (self->pv->pubkey->revoked)
		flags |= BASTILE_FLAG_REVOKED;

	if (self->pv->pubkey->disabled)
		flags |= BASTILE_FLAG_DISABLED;

	if (bastile_gpgme_key_get_trust (self) >= BASTILE_VALIDITY_MARGINAL && 
	    !self->pv->pubkey->revoked && !self->pv->pubkey->disabled && 
	    !self->pv->pubkey->expired)
		flags |= BASTILE_FLAG_TRUSTED;

	g_object_set (obj, "flags", flags, NULL);
	
	/* The type */
	if (self->pv->seckey)
		usage = BASTILE_USAGE_PRIVATE_KEY;
	else
		usage = BASTILE_USAGE_PUBLIC_KEY;

	g_object_set (obj, "usage", usage, NULL);
	
	++self->pv->block_loading;
	BASTILE_OBJECT_CLASS (bastile_gpgme_key_parent_class)->realize (obj);
	--self->pv->block_loading;
}

static void
bastile_gpgme_key_refresh (BastileObject *obj)
{
	BastileGpgmeKey *self = BASTILE_GPGME_KEY (obj);
	
	if (self->pv->pubkey)
		load_key_public (self, self->pv->list_mode);
	if (self->pv->seckey)
		load_key_private (self);
	if (self->pv->photos_loaded)
		load_key_photos (self);

	BASTILE_OBJECT_CLASS (bastile_gpgme_key_parent_class)->refresh (obj);
}

static BastileOperation*
bastile_gpgme_key_delete (BastileObject *obj)
{
	BastileGpgmeKey *self = BASTILE_GPGME_KEY (obj);
	gpgme_error_t gerr;
	GError *err = NULL;
	
	if (self->pv->seckey)
		gerr = bastile_gpgme_key_op_delete_pair (self);
	else
		gerr = bastile_gpgme_key_op_delete (self);
	
	if (!GPG_IS_OK (gerr))
		bastile_gpgme_to_error (gerr, &err);
	
	return bastile_operation_new_complete (err);
}

static GList*
bastile_gpgme_key_get_uids (BastilePgpKey *base)
{
	BastileGpgmeKey *self = BASTILE_GPGME_KEY (base);
	require_key_uids (self);
	return BASTILE_PGP_KEY_CLASS (bastile_gpgme_key_parent_class)->get_uids (base);
}

static void
bastile_gpgme_key_set_uids (BastilePgpKey *base, GList *uids)
{
	BastileGpgmeKey *self = BASTILE_GPGME_KEY (base);
	GList *l;
	
	BASTILE_PGP_KEY_CLASS (bastile_gpgme_key_parent_class)->set_uids (base, uids);
	
	/* Remove the parent on each old one */
	for (l = self->pv->uids; l; l = g_list_next (l))
		bastile_context_remove_object (bastile_context_for_app (), l->data);

	/* Keep our own copy of the UID list */
	bastile_object_list_free (self->pv->uids);
	self->pv->uids = bastile_object_list_copy (uids);
	
	/* Add UIDS to context so that they show up in libmatecryptui */
	for (l = self->pv->uids; l; l = g_list_next (l))
		bastile_context_add_object (bastile_context_for_app (), l->data);

	renumber_actual_uids (self);
}

static GList*
bastile_gpgme_key_get_subkeys (BastilePgpKey *base)
{
	BastileGpgmeKey *self = BASTILE_GPGME_KEY (base);
	require_key_subkeys (self);
	return BASTILE_PGP_KEY_CLASS (bastile_gpgme_key_parent_class)->get_subkeys (base);
}

static GList*
bastile_gpgme_key_get_photos (BastilePgpKey *base)
{
	BastileGpgmeKey *self = BASTILE_GPGME_KEY (base);
	require_key_photos (self);
	return BASTILE_PGP_KEY_CLASS (bastile_gpgme_key_parent_class)->get_photos (base);
}

static void
bastile_gpgme_key_set_photos (BastilePgpKey *base, GList *photos)
{
	BastileGpgmeKey *self = BASTILE_GPGME_KEY (base);
	self->pv->photos_loaded = TRUE;
	BASTILE_PGP_KEY_CLASS (bastile_gpgme_key_parent_class)->set_photos (base, photos);
	renumber_actual_uids (self);
}

static void
bastile_gpgme_key_init (BastileGpgmeKey *self)
{
	self->pv = G_TYPE_INSTANCE_GET_PRIVATE (self, BASTILE_TYPE_GPGME_KEY, BastileGpgmeKeyPrivate);
	g_object_set (self, "location", BASTILE_LOCATION_LOCAL, NULL);
}

static void
bastile_gpgme_key_get_property (GObject *object, guint prop_id,
                               GValue *value, GParamSpec *pspec)
{
	BastileGpgmeKey *self = BASTILE_GPGME_KEY (object);
    
	switch (prop_id) {
	case PROP_PUBKEY:
		g_value_set_boxed (value, bastile_gpgme_key_get_public (self));
		break;
	case PROP_SECKEY:
		g_value_set_boxed (value, bastile_gpgme_key_get_private (self));
		break;
	case PROP_VALIDITY:
		g_value_set_uint (value, bastile_gpgme_key_get_validity (self));
		break;
	case PROP_TRUST:
		g_value_set_uint (value, bastile_gpgme_key_get_trust (self));
		break;
	}
}

static void
bastile_gpgme_key_set_property (GObject *object, guint prop_id, const GValue *value, 
                               GParamSpec *pspec)
{
	BastileGpgmeKey *self = BASTILE_GPGME_KEY (object);

	switch (prop_id) {
	case PROP_PUBKEY:
		bastile_gpgme_key_set_public (self, g_value_get_boxed (value));
		break;
	case PROP_SECKEY:
		bastile_gpgme_key_set_private (self, g_value_get_boxed (value));
		break;
	}
}

static void
bastile_gpgme_key_object_dispose (GObject *obj)
{
	BastileGpgmeKey *self = BASTILE_GPGME_KEY (obj);
	GList *l;
	
	/* Remove the attached UIDs */
	for (l = self->pv->uids; l; l = g_list_next (l))
		bastile_context_remove_object (bastile_context_for_app (), l->data);

	if (self->pv->pubkey)
		gpgme_key_unref (self->pv->pubkey);
	if (self->pv->seckey)
		gpgme_key_unref (self->pv->seckey);
	self->pv->pubkey = self->pv->seckey = NULL;
	
	bastile_object_list_free (self->pv->uids);
	self->pv->uids = NULL;

	G_OBJECT_CLASS (bastile_gpgme_key_parent_class)->dispose (obj);
}

static void
bastile_gpgme_key_object_finalize (GObject *obj)
{
	BastileGpgmeKey *self = BASTILE_GPGME_KEY (obj);

	g_assert (self->pv->pubkey == NULL);
	g_assert (self->pv->seckey == NULL);
	g_assert (self->pv->uids == NULL);
    
	G_OBJECT_CLASS (bastile_gpgme_key_parent_class)->finalize (obj);
}

static void
bastile_gpgme_key_class_init (BastileGpgmeKeyClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	BastileObjectClass *bastile_class = BASTILE_OBJECT_CLASS (klass);
	BastilePgpKeyClass *pgp_class = BASTILE_PGP_KEY_CLASS (klass);
	
	bastile_gpgme_key_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (BastileGpgmeKeyPrivate));

	gobject_class->dispose = bastile_gpgme_key_object_dispose;
	gobject_class->finalize = bastile_gpgme_key_object_finalize;
	gobject_class->set_property = bastile_gpgme_key_set_property;
	gobject_class->get_property = bastile_gpgme_key_get_property;
	
	bastile_class->refresh = bastile_gpgme_key_refresh;
	bastile_class->realize = bastile_gpgme_key_realize;
	bastile_class->delete = bastile_gpgme_key_delete;
	
	pgp_class->get_uids = bastile_gpgme_key_get_uids;
	pgp_class->set_uids = bastile_gpgme_key_set_uids;
	pgp_class->get_subkeys = bastile_gpgme_key_get_subkeys;
	pgp_class->get_photos = bastile_gpgme_key_get_photos;
	pgp_class->set_photos = bastile_gpgme_key_set_photos;
	
	g_object_class_install_property (gobject_class, PROP_PUBKEY,
	        g_param_spec_boxed ("pubkey", "GPGME Public Key", "GPGME Public Key that this object represents",
	                            BASTILE_GPGME_BOXED_KEY, G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_SECKEY,
                g_param_spec_boxed ("seckey", "GPGME Secret Key", "GPGME Secret Key that this object represents",
                                    BASTILE_GPGME_BOXED_KEY, G_PARAM_READWRITE));

	g_object_class_override_property (gobject_class, PROP_VALIDITY, "validity");

        g_object_class_override_property (gobject_class, PROP_TRUST, "trust");
}


/* -----------------------------------------------------------------------------
 * PUBLIC 
 */

BastileGpgmeKey* 
bastile_gpgme_key_new (BastileSource *sksrc, gpgme_key_t pubkey, 
                        gpgme_key_t seckey)
{
	const gchar *keyid;

	g_return_val_if_fail (pubkey || seckey, NULL);
	
	if (pubkey != NULL)
		keyid = pubkey->subkeys->keyid;
	else
		keyid = seckey->subkeys->keyid;
	
	return g_object_new (BASTILE_TYPE_GPGME_KEY, "source", sksrc,
	                     "id", bastile_pgp_key_canonize_id (keyid),
	                     "pubkey", pubkey, "seckey", seckey, 
	                     NULL);
}

gpgme_key_t
bastile_gpgme_key_get_public (BastileGpgmeKey *self)
{
	g_return_val_if_fail (BASTILE_IS_GPGME_KEY (self), NULL);
	if (require_key_public (self, GPGME_KEYLIST_MODE_LOCAL))
		return self->pv->pubkey;
	return NULL;
}

void
bastile_gpgme_key_set_public (BastileGpgmeKey *self, gpgme_key_t key)
{
	GObject *obj;
	
	g_return_if_fail (BASTILE_IS_GPGME_KEY (self));
	
	if (self->pv->pubkey)
		gpgme_key_unref (self->pv->pubkey);
	self->pv->pubkey = key;
	if (self->pv->pubkey) {
		gpgme_key_ref (self->pv->pubkey);
		self->pv->list_mode |= self->pv->pubkey->keylist_mode;
	}
	
	obj = G_OBJECT (self);
	g_object_freeze_notify (obj);
	bastile_gpgme_key_realize (BASTILE_OBJECT (self));
	g_object_notify (obj, "fingerprint");
	g_object_notify (obj, "validity");
	g_object_notify (obj, "validity-str");
	g_object_notify (obj, "trust");
	g_object_notify (obj, "trust-str");
	g_object_notify (obj, "expires");
	g_object_notify (obj, "expires-str");
	g_object_notify (obj, "length");
	g_object_notify (obj, "algo");
	g_object_thaw_notify (obj);
}

gpgme_key_t
bastile_gpgme_key_get_private (BastileGpgmeKey *self)
{
	g_return_val_if_fail (BASTILE_IS_GPGME_KEY (self), NULL);
	if (require_key_private (self))
		return self->pv->seckey;
	return NULL;	
}

void
bastile_gpgme_key_set_private (BastileGpgmeKey *self, gpgme_key_t key)
{
	GObject *obj;
	
	g_return_if_fail (BASTILE_IS_GPGME_KEY (self));
	
	if (self->pv->seckey)
		gpgme_key_unref (self->pv->seckey);
	self->pv->seckey = key;
	if (self->pv->seckey)
		gpgme_key_ref (self->pv->seckey);
	
	obj = G_OBJECT (self);
	g_object_freeze_notify (obj);
	bastile_gpgme_key_realize (BASTILE_OBJECT (self));
	g_object_thaw_notify (obj);
}

BastileValidity
bastile_gpgme_key_get_validity (BastileGpgmeKey *self)
{
	g_return_val_if_fail (BASTILE_IS_GPGME_KEY (self), BASTILE_VALIDITY_UNKNOWN);
	
	if (!require_key_public (self, GPGME_KEYLIST_MODE_LOCAL))
		return BASTILE_VALIDITY_UNKNOWN;
	
	g_return_val_if_fail (self->pv->pubkey, BASTILE_VALIDITY_UNKNOWN);
	g_return_val_if_fail (self->pv->pubkey->uids, BASTILE_VALIDITY_UNKNOWN);
	
	if (self->pv->pubkey->revoked)
		return BASTILE_VALIDITY_REVOKED;
	if (self->pv->pubkey->disabled)
		return BASTILE_VALIDITY_DISABLED;
	return bastile_gpgme_convert_validity (self->pv->pubkey->uids->validity);
}

BastileValidity
bastile_gpgme_key_get_trust (BastileGpgmeKey *self)
{
	g_return_val_if_fail (BASTILE_IS_GPGME_KEY (self), BASTILE_VALIDITY_UNKNOWN);
	if (!require_key_public (self, GPGME_KEYLIST_MODE_LOCAL))
		return BASTILE_VALIDITY_UNKNOWN;
	
	return bastile_gpgme_convert_validity (self->pv->pubkey->owner_trust);
}

void
bastile_gpgme_key_refresh_matching (gpgme_key_t key)
{
	BastileObjectPredicate pred;
	
	g_return_if_fail (key->subkeys->keyid);
	
	memset (&pred, 0, sizeof (pred));
	pred.type = BASTILE_TYPE_GPGME_KEY;
	pred.id = bastile_pgp_key_canonize_id (key->subkeys->keyid);
	
	bastile_context_for_objects_full (NULL, &pred, refresh_each_object, NULL);
}
