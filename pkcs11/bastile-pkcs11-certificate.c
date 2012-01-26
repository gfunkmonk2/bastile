/* 
 * Bastile
 * 
 * Copyright (C) 2008 Stefan Walter
 * 
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *  
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.  
 */

#include "config.h"

#include "bastile-pkcs11-certificate.h"
#include "bastile-pkcs11-helpers.h"

#include "bastile-object.h"
#include "bastile-pkcs11.h"
#include "bastile-util.h"
#include "bastile-validity.h"

#include <gcr/gcr.h>

#include <pkcs11.h>

#include <glib/gi18n-lib.h>

static const gulong REQUIRED_ATTRS[] = {
	CKA_VALUE,
	CKA_ID,
	CKA_TRUSTED,
	CKA_END_DATE
};

enum {
	PROP_0,
	PROP_FINGERPRINT,
	PROP_VALIDITY,
	PROP_VALIDITY_STR,
	PROP_TRUST,
	PROP_TRUST_STR,
	PROP_EXPIRES,
	PROP_EXPIRES_STR
};

struct _BastilePkcs11CertificatePrivate {
	GckAttribute der_value;
};

static void bastile_pkcs11_certificate_iface (GcrCertificateIface *iface);

G_DEFINE_TYPE_WITH_CODE (BastilePkcs11Certificate, bastile_pkcs11_certificate, BASTILE_PKCS11_TYPE_OBJECT,
                         GCR_CERTIFICATE_MIXIN_IMPLEMENT_COMPARABLE ();
                         G_IMPLEMENT_INTERFACE (GCR_TYPE_CERTIFICATE, bastile_pkcs11_certificate_iface);
);

/* -----------------------------------------------------------------------------
 * INTERNAL 
 */

static gchar* 
calc_display_id (BastilePkcs11Certificate* self) 
{
	gsize len;
	gchar *id, *ret;
	
	g_return_val_if_fail (BASTILE_PKCS11_IS_CERTIFICATE (self), NULL);
	
	id = bastile_pkcs11_certificate_get_fingerprint (self);
	g_return_val_if_fail (id, NULL);
	
	len = strlen (id);
	if (len <= 8)
		return id;

	ret = g_strndup (id + (len - 8), 8);
	g_free (id);
	return ret;
}

/* -----------------------------------------------------------------------------
 * OBJECT 
 */

static void
bastile_pkcs11_certificate_realize (BastileObject *obj)
{
	BastilePkcs11Certificate *self = BASTILE_PKCS11_CERTIFICATE (obj);
	gchar *identifier = NULL;
	guint flags;

	BASTILE_OBJECT_CLASS (bastile_pkcs11_certificate_parent_class)->realize (obj);

	bastile_pkcs11_object_require_attributes (BASTILE_PKCS11_OBJECT (self), 
	                                           REQUIRED_ATTRS, G_N_ELEMENTS (REQUIRED_ATTRS));
	
	if (!bastile_object_get_label (obj))
		g_object_set (self, "label", _("Certificate"), NULL);
	
	flags = bastile_object_get_flags (obj);

	/* TODO: Expiry, revoked, disabled etc... */
	if (bastile_pkcs11_certificate_get_trust (self) >= BASTILE_VALIDITY_MARGINAL)
		flags |= BASTILE_FLAG_TRUSTED;

	identifier = calc_display_id (self);

	g_object_set (self,
		      "location", BASTILE_LOCATION_LOCAL,
		      "usage", BASTILE_USAGE_PUBLIC_KEY,
		      "flags", flags,
		      "identifier", identifier,
		      NULL);

	g_free (identifier);
}

static GObject* 
bastile_pkcs11_certificate_constructor (GType type, guint n_props, GObjectConstructParam *props) 
{
	BastilePkcs11Certificate *self = BASTILE_PKCS11_CERTIFICATE (G_OBJECT_CLASS (bastile_pkcs11_certificate_parent_class)->constructor(type, n_props, props));
	
	g_return_val_if_fail (self, NULL);	

	return G_OBJECT (self);
}

static void
bastile_pkcs11_certificate_init (BastilePkcs11Certificate *self)
{
	self->pv = (G_TYPE_INSTANCE_GET_PRIVATE (self, BASTILE_PKCS11_TYPE_CERTIFICATE, BastilePkcs11CertificatePrivate));
	gck_attribute_init_invalid (&self->pv->der_value, CKA_VALUE);
}

static void
bastile_pkcs11_certificate_notify (GObject *obj, GParamSpec *pspec)
{
	/* When the base class pkcs11-attributes changes, it can affects lots */
	if (g_str_equal (pspec->name, "pkcs11-attributes")) {
		g_object_notify (obj, "fingerprint");
		g_object_notify (obj, "validity");
		g_object_notify (obj, "validity-str");
		g_object_notify (obj, "trust");
		g_object_notify (obj, "trust-str");
		g_object_notify (obj, "expires");
		g_object_notify (obj, "expires-str");
	}
	
	if (G_OBJECT_CLASS (bastile_pkcs11_certificate_parent_class)->notify)
		G_OBJECT_CLASS (bastile_pkcs11_certificate_parent_class)->notify (obj, pspec);
}


static void
bastile_pkcs11_certificate_finalize (GObject *obj)
{
	BastilePkcs11Certificate *self = BASTILE_PKCS11_CERTIFICATE (obj);

	gck_attribute_clear (&self->pv->der_value);

	G_OBJECT_CLASS (bastile_pkcs11_certificate_parent_class)->finalize (obj);
}

static void
bastile_pkcs11_certificate_get_property (GObject *obj, guint prop_id, GValue *value, 
                                          GParamSpec *pspec)
{
	BastilePkcs11Certificate *self = BASTILE_PKCS11_CERTIFICATE (obj);
	
	switch (prop_id) {
	case PROP_FINGERPRINT:
		g_value_take_string (value, bastile_pkcs11_certificate_get_fingerprint (self));
		break;
	case PROP_VALIDITY:
		g_value_set_uint (value, bastile_pkcs11_certificate_get_validity (self));
		break;
	case PROP_VALIDITY_STR:
		g_value_set_string (value, bastile_pkcs11_certificate_get_validity_str (self));
		break;
	case PROP_TRUST:
		g_value_set_uint (value, bastile_pkcs11_certificate_get_trust (self));
		break;
	case PROP_TRUST_STR:
		g_value_set_string (value, bastile_pkcs11_certificate_get_trust_str (self));
		break;
	case PROP_EXPIRES:
		g_value_set_ulong (value, bastile_pkcs11_certificate_get_expires (self));
		break;
	case PROP_EXPIRES_STR:
		g_value_take_string (value, bastile_pkcs11_certificate_get_expires_str (self));
		break;
	default:
		gcr_certificate_mixin_get_property (obj, prop_id, value, pspec);
		break;
	}
}

static void
bastile_pkcs11_certificate_set_property (GObject *obj, guint prop_id, const GValue *value, 
                                          GParamSpec *pspec)
{
	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
bastile_pkcs11_certificate_class_init (BastilePkcs11CertificateClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	BastileObjectClass *bastile_class = BASTILE_OBJECT_CLASS (klass);
	
	bastile_pkcs11_certificate_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (BastilePkcs11CertificatePrivate));

	gobject_class->constructor = bastile_pkcs11_certificate_constructor;
	gobject_class->finalize = bastile_pkcs11_certificate_finalize;
	gobject_class->set_property = bastile_pkcs11_certificate_set_property;
	gobject_class->get_property = bastile_pkcs11_certificate_get_property;
	gobject_class->notify = bastile_pkcs11_certificate_notify;

	bastile_class->realize = bastile_pkcs11_certificate_realize;
	
	g_object_class_install_property (gobject_class, PROP_FINGERPRINT, 
	         g_param_spec_string ("fingerprint", "fingerprint", "fingerprint", NULL, 
	                              G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE));
	
	g_object_class_install_property (gobject_class, PROP_VALIDITY, 
	         g_param_spec_uint ("validity", "validity", "validity", 0, G_MAXUINT, 0U, 
	                            G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE));
	
	g_object_class_install_property (gobject_class, PROP_VALIDITY_STR, 
	         g_param_spec_string ("validity-str", "validity-str", "validity-str", NULL, 
	                              G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE));
	
	g_object_class_install_property (gobject_class, PROP_TRUST, 
	         g_param_spec_uint ("trust", "trust", "trust", 0, G_MAXUINT, 0U, 
	                            G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE));
	
	g_object_class_install_property (gobject_class, PROP_TRUST_STR, 
	         g_param_spec_string ("trust-str", "trust-str", "trust-str", NULL, 
	                              G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE));
	
	g_object_class_install_property (gobject_class, PROP_EXPIRES, 
	         g_param_spec_ulong ("expires", "expires", "expires", 0, G_MAXULONG, 0UL, 
	                             G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE));
	
	g_object_class_install_property (gobject_class, PROP_EXPIRES_STR, 
	         g_param_spec_string ("expires-str", "expires-str", "expires-str", NULL, 
	                              G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE));

	gcr_certificate_mixin_class_init (gobject_class);
}

const guchar*
bastile_pkcs11_certificate_get_der_data (GcrCertificate *cert, gsize *n_length)
{
	BastilePkcs11Object *obj = BASTILE_PKCS11_OBJECT (cert);
	BastilePkcs11Certificate *self = BASTILE_PKCS11_CERTIFICATE (cert);
	GckAttributes *attrs;
	GckAttribute *attr;

	g_return_val_if_fail (BASTILE_PKCS11_IS_CERTIFICATE (cert), NULL);
	
	if (!bastile_pkcs11_object_require_attributes (obj, REQUIRED_ATTRS, 
	                                                G_N_ELEMENTS (REQUIRED_ATTRS)))
		return NULL;
	
	/* TODO: This whole copying and possibly freeing memory in use is risky */

	if (gck_attribute_is_invalid (&self->pv->der_value)) {
		attrs = bastile_pkcs11_object_get_pkcs11_attributes (obj);
		g_return_val_if_fail (attrs, NULL);
		attr = gck_attributes_find (attrs, CKA_VALUE);
		g_return_val_if_fail (attr, NULL);
		gck_attribute_clear (&self->pv->der_value);
		gck_attribute_init_copy (&self->pv->der_value, attr);
	}
	
	g_return_val_if_fail (!gck_attribute_is_invalid (&self->pv->der_value), NULL);
	*n_length = self->pv->der_value.length;
	return self->pv->der_value.value;
}

static void 
bastile_pkcs11_certificate_iface (GcrCertificateIface *iface)
{
	iface->get_der_data = (gpointer)bastile_pkcs11_certificate_get_der_data;
}

/* -----------------------------------------------------------------------------
 * PUBLIC 
 */

BastilePkcs11Certificate*
bastile_pkcs11_certificate_new (GckObject* object)
{
	return g_object_new (BASTILE_PKCS11_TYPE_CERTIFICATE, 
	                     "pkcs11-object", object, NULL);
}

gchar* 
bastile_pkcs11_certificate_get_fingerprint (BastilePkcs11Certificate* self) 
{
	GckAttribute* attr;

	/* TODO: We should be using the fingerprint off the DER */
	attr = bastile_pkcs11_object_require_attribute (BASTILE_PKCS11_OBJECT (self), CKA_ID);
	if (attr == NULL)
		return g_strdup ("");

	return bastile_util_hex_encode (attr->value, attr->length);
}

guint 
bastile_pkcs11_certificate_get_validity (BastilePkcs11Certificate* self) 
{
	g_return_val_if_fail (BASTILE_PKCS11_IS_CERTIFICATE (self), 0U);
	
	/* TODO: We need to implement proper validity checking */
	return BASTILE_VALIDITY_UNKNOWN;
}

const char* 
bastile_pkcs11_certificate_get_validity_str (BastilePkcs11Certificate* self) 
{
	g_return_val_if_fail (BASTILE_PKCS11_IS_CERTIFICATE (self), NULL);
	return bastile_validity_get_string (bastile_pkcs11_certificate_get_validity (self));
}

guint 
bastile_pkcs11_certificate_get_trust (BastilePkcs11Certificate* self) 
{
	GckAttribute *attr;

	g_return_val_if_fail (BASTILE_PKCS11_IS_CERTIFICATE (self), 0);

	attr = bastile_pkcs11_object_require_attribute (BASTILE_PKCS11_OBJECT (self), CKA_TRUSTED);
	if (attr && gck_attribute_get_boolean (attr))
		return BASTILE_VALIDITY_FULL;

	return BASTILE_VALIDITY_UNKNOWN;
}

const char* 
bastile_pkcs11_certificate_get_trust_str (BastilePkcs11Certificate* self) 
{
	g_return_val_if_fail (BASTILE_PKCS11_IS_CERTIFICATE (self), NULL);
	return bastile_validity_get_string (bastile_pkcs11_certificate_get_trust (self));
}


gulong
bastile_pkcs11_certificate_get_expires (BastilePkcs11Certificate* self) 
{
	GckAttribute *attr;
	GDate date = { 0 };
	struct tm time = { 0 };

	g_return_val_if_fail (BASTILE_PKCS11_IS_CERTIFICATE (self), 0);
	
	attr = bastile_pkcs11_object_require_attribute (BASTILE_PKCS11_OBJECT (self), CKA_END_DATE);
	if (attr == NULL)
		return 0;

	gck_attribute_get_date (attr, &date);
	g_date_to_struct_tm (&date, &time);
	return (gulong)(mktime (&time));
}

char* 
bastile_pkcs11_certificate_get_expires_str (BastilePkcs11Certificate* self) 
{
	gulong expiry;
	
	g_return_val_if_fail (BASTILE_PKCS11_IS_CERTIFICATE (self), NULL);
	
	/* TODO: When expired return Expired */
	expiry = bastile_pkcs11_certificate_get_expires (self);
	if (expiry == 0)
		return g_strdup ("");
	return bastile_util_get_date_string (expiry);
}
