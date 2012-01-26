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

#include "bastile-pgp.h"
#include "bastile-pgp-key.h"
#include "bastile-pgp-signature.h"

#include "bastile-context.h"
#include "bastile-object.h"

#include <string.h>

#include <glib/gi18n.h>

enum {
	PROP_0,
	PROP_KEYID,
	PROP_FLAGS,
	PROP_SIGTYPE
};

G_DEFINE_TYPE (BastilePgpSignature, bastile_pgp_signature, G_TYPE_OBJECT);

struct _BastilePgpSignaturePrivate {
	guint flags;
	gchar *keyid;
};

/* -----------------------------------------------------------------------------
 * OBJECT 
 */

static void
bastile_pgp_signature_init (BastilePgpSignature *self)
{
	self->pv = G_TYPE_INSTANCE_GET_PRIVATE (self, BASTILE_TYPE_PGP_SIGNATURE, BastilePgpSignaturePrivate);
}

static void
bastile_pgp_signature_get_property (GObject *object, guint prop_id,
                                  GValue *value, GParamSpec *pspec)
{
	BastilePgpSignature *self = BASTILE_PGP_SIGNATURE (object);
	
	switch (prop_id) {
	case PROP_KEYID:
		g_value_set_string (value, bastile_pgp_signature_get_keyid (self));
		break;
	case PROP_FLAGS:
		g_value_set_uint (value, bastile_pgp_signature_get_flags (self));
		break;
	case PROP_SIGTYPE:
		g_value_set_uint (value, bastile_pgp_signature_get_sigtype (self));
		break;
	}
}

static void
bastile_pgp_signature_set_property (GObject *object, guint prop_id, const GValue *value, 
                                  GParamSpec *pspec)
{
	BastilePgpSignature *self = BASTILE_PGP_SIGNATURE (object);
	
	switch (prop_id) {
	case PROP_KEYID:
		bastile_pgp_signature_set_keyid (self, g_value_get_string (value));
		break;
	case PROP_FLAGS:
		bastile_pgp_signature_set_flags (self, g_value_get_uint (value));
		break;
	}
}

static void
bastile_pgp_signature_finalize (GObject *gobject)
{
	BastilePgpSignature *self = BASTILE_PGP_SIGNATURE (gobject);

	g_free (self->pv->keyid);
	self->pv->keyid = NULL;
    
	G_OBJECT_CLASS (bastile_pgp_signature_parent_class)->finalize (gobject);
}

static void
bastile_pgp_signature_class_init (BastilePgpSignatureClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);    

	bastile_pgp_signature_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (BastilePgpSignaturePrivate));
	
	gobject_class->finalize = bastile_pgp_signature_finalize;
	gobject_class->set_property = bastile_pgp_signature_set_property;
	gobject_class->get_property = bastile_pgp_signature_get_property;

        g_object_class_install_property (gobject_class, PROP_KEYID,
                g_param_spec_string ("keyid", "Key ID", "GPG Key ID",
                                     "", G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_FLAGS,
	        g_param_spec_uint ("flags", "Flags", "PGP signature flags",
	                           0, G_MAXUINT, 0, G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_SIGTYPE,
	        g_param_spec_uint ("sigtype", "Sig Type", "PGP signature type",
	                           0, G_MAXUINT, 0, G_PARAM_READABLE));
}

/* -----------------------------------------------------------------------------
 * PUBLIC 
 */

BastilePgpSignature*
bastile_pgp_signature_new (const gchar *keyid)
{
	return g_object_new (BASTILE_TYPE_PGP_SIGNATURE, "keyid", keyid, NULL);
}
 
guint
bastile_pgp_signature_get_flags (BastilePgpSignature *self)
{
	g_return_val_if_fail (BASTILE_IS_PGP_SIGNATURE (self), 0);
	return self->pv->flags;
}

void
bastile_pgp_signature_set_flags (BastilePgpSignature *self, guint flags)
{
	GObject *obj;
	
	g_return_if_fail (BASTILE_IS_PGP_SIGNATURE (self));
	self->pv->flags = flags;

	obj = G_OBJECT (self);
	g_object_freeze_notify (obj);
	g_object_notify (obj, "flags");
	g_object_notify (obj, "sigtype");
	g_object_thaw_notify (obj);
}

const gchar*
bastile_pgp_signature_get_keyid (BastilePgpSignature *self)
{
	g_return_val_if_fail (BASTILE_IS_PGP_SIGNATURE (self), NULL);
	return self->pv->keyid;
}

void
bastile_pgp_signature_set_keyid (BastilePgpSignature *self, const gchar *keyid)
{
	GObject *obj;
	
	g_return_if_fail (BASTILE_IS_PGP_SIGNATURE (self));
	g_free (self->pv->keyid);
	self->pv->keyid = g_strdup (keyid);
	
	obj = G_OBJECT (self);
	g_object_freeze_notify (obj);
	g_object_notify (obj, "keyid");
	g_object_notify (obj, "sigtype");
	g_object_thaw_notify (obj);
}

guint
bastile_pgp_signature_get_sigtype (BastilePgpSignature *self)
{
	BastileObject *sobj;
	GQuark id;

	g_return_val_if_fail (BASTILE_IS_PGP_SIGNATURE (self), 0);
    
	id = bastile_pgp_key_canonize_id (self->pv->keyid);
	sobj = bastile_context_find_object (SCTX_APP (), id, BASTILE_LOCATION_LOCAL);
    
	if (sobj) {
		if (bastile_object_get_usage (sobj) == BASTILE_USAGE_PRIVATE_KEY) 
			return SKEY_PGPSIG_TRUSTED | SKEY_PGPSIG_PERSONAL;
		if (bastile_object_get_flags (sobj) & BASTILE_FLAG_TRUSTED)
			return SKEY_PGPSIG_TRUSTED;
	}

	return 0;
}
