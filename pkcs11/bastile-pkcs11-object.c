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

#include "bastile-pkcs11-object.h"
#include "bastile-pkcs11-operations.h"

#include "bastile-object.h"
#include "bastile-pkcs11.h"
#include "bastile-util.h"

#include <pkcs11.h>

#include <glib/gi18n-lib.h>

static const gulong REQUIRED_ATTRS[] = {
	CKA_LABEL,
	CKA_ID
};

enum {
	PROP_0,
	PROP_PKCS11_HANDLE,
	PROP_PKCS11_OBJECT,
	PROP_PKCS11_ATTRIBUTES
};

struct _BastilePkcs11ObjectPrivate {
	GckObject *pkcs11_object;
	GckAttributes* pkcs11_attributes;
	GCancellable *request_attributes;
};

G_DEFINE_TYPE (BastilePkcs11Object, bastile_pkcs11_object, BASTILE_TYPE_OBJECT);

/* -----------------------------------------------------------------------------
 * INTERNAL 
 */

static void
received_object_attributes (GObject *object, GAsyncResult *result, gpointer user_data)
{
	BastilePkcs11Object *self = BASTILE_PKCS11_OBJECT (user_data);
	GckAttributes *attrs;
	GError *err = NULL;
	
	attrs = gck_object_get_finish (GCK_OBJECT (object), result, &err);
	if (err && err->code == CKR_FUNCTION_CANCELED)
		return;

	g_object_unref (self->pv->request_attributes);
	self->pv->request_attributes = NULL;

	if (attrs == NULL)
		g_message ("couldn't load attributes for pkcs#11 object: %s",
		           err && err->message ? err->message : "");
	else
		bastile_pkcs11_object_set_pkcs11_attributes (self, attrs);

	g_object_unref (self);
}

static void
load_object_attributes (BastilePkcs11Object *self, const gulong *attr_types, 
                        gsize n_attr_types)
{
	gboolean added = FALSE;
	GckAttribute *attr;
	GArray *types;
	gsize i;

	g_assert (BASTILE_PKCS11_IS_OBJECT (self));
	g_assert (self->pv->pkcs11_attributes);
	g_assert (self->pv->pkcs11_object);

	/* Add in any attirbutes not found, as invalid */
	for (i = 0; i < n_attr_types; ++i) {
		attr = gck_attributes_find (self->pv->pkcs11_attributes, attr_types[i]);
		if (attr == NULL) {
			gck_attributes_add_invalid (self->pv->pkcs11_attributes, attr_types[i]);
			added = TRUE;
		}
			
	}

	/* Already a request happening */
	if (self->pv->request_attributes) {
		if (added) {
			/* Need to restart the request, our attributes not covered */
			g_cancellable_cancel (self->pv->request_attributes);
			g_object_unref (self->pv->request_attributes);
			self->pv->request_attributes = NULL;
		} else {
			/* The current request works for us */
			return;
		}
	}

	/* 
	 * Build up an array of attribute types to load, from the 
	 * ones that we alrady have plus the ones that are requested.
	 */

	types = g_array_new (FALSE, TRUE, sizeof (gulong));
	for (i = 0; i < gck_attributes_count (self->pv->pkcs11_attributes); ++i) {
		attr = gck_attributes_at (self->pv->pkcs11_attributes, i);
		g_return_if_fail (attr);
		g_array_append_val (types, attr->type);
	}

	/* Off we go to load them all */
	self->pv->request_attributes = g_cancellable_new ();
	gck_object_get_async (self->pv->pkcs11_object, (gulong*)types->data, types->len,
	                      self->pv->request_attributes, received_object_attributes, g_object_ref (self));

	g_array_free (types, TRUE);
}

/* -----------------------------------------------------------------------------
 * OBJECT 
 */

static void
bastile_pkcs11_object_realize (BastileObject *obj)
{
	BastilePkcs11Object *self = BASTILE_PKCS11_OBJECT (obj);
	gboolean exportable;
	gchar *label = NULL;
	guint flags;

	BASTILE_OBJECT_CLASS (bastile_pkcs11_object_parent_class)->realize (obj);

	bastile_pkcs11_object_require_attributes (self, REQUIRED_ATTRS, G_N_ELEMENTS (REQUIRED_ATTRS));
	
	g_assert (BASTILE_PKCS11_IS_OBJECT (obj));

	flags = 0;
	if (gck_attributes_find_boolean (self->pv->pkcs11_attributes, CKA_EXTRACTABLE, &exportable) && exportable)
		flags |= BASTILE_FLAG_EXPORTABLE;

	gck_attributes_find_string (self->pv->pkcs11_attributes, CKA_LABEL, &label);
	g_object_set (self,
		      "label", label,
		      "location", BASTILE_LOCATION_LOCAL,
		      "flags", flags,
		      NULL);

	g_free (label);
}

static void
bastile_pkcs11_object_refresh (BastileObject *obj)
{
	/* Reload the same attributes */
	load_object_attributes (BASTILE_PKCS11_OBJECT (obj), NULL, 0);
	BASTILE_OBJECT_CLASS (bastile_pkcs11_object_parent_class)->refresh (obj);
}

static BastileOperation*
bastile_pkcs11_object_delete (BastileObject *obj)
{
	return bastile_pkcs11_deleter_new (BASTILE_PKCS11_OBJECT (obj));
}

static GObject* 
bastile_pkcs11_object_constructor (GType type, guint n_props, GObjectConstructParam *props) 
{
	BastilePkcs11Object *self = BASTILE_PKCS11_OBJECT (G_OBJECT_CLASS (bastile_pkcs11_object_parent_class)->constructor(type, n_props, props));
	
	g_return_val_if_fail (self, NULL);	

	g_return_val_if_fail (self->pv->pkcs11_object, NULL);
	g_object_set (self, "id", bastile_pkcs11_object_cannonical_id (self->pv->pkcs11_object), NULL);
	
	return G_OBJECT (self);
}

static void
bastile_pkcs11_object_init (BastilePkcs11Object *self)
{
	self->pv = (G_TYPE_INSTANCE_GET_PRIVATE (self, BASTILE_PKCS11_TYPE_OBJECT, BastilePkcs11ObjectPrivate));
	self->pv->pkcs11_attributes = gck_attributes_new ();
}

static void
bastile_pkcs11_object_dispose (GObject *obj)
{
	BastilePkcs11Object *self = BASTILE_PKCS11_OBJECT (obj);

	if (self->pv->pkcs11_object)
		g_object_unref (self->pv->pkcs11_object);
	self->pv->pkcs11_object = NULL;
	
	G_OBJECT_CLASS (bastile_pkcs11_object_parent_class)->dispose (obj);
}

static void
bastile_pkcs11_object_finalize (GObject *obj)
{
	BastilePkcs11Object *self = BASTILE_PKCS11_OBJECT (obj);

	g_assert (self->pv->pkcs11_object == NULL);
	
	if (self->pv->pkcs11_attributes)
		gck_attributes_unref (self->pv->pkcs11_attributes);
	self->pv->pkcs11_attributes = NULL;
	
	G_OBJECT_CLASS (bastile_pkcs11_object_parent_class)->finalize (obj);
}

static void
bastile_pkcs11_object_get_property (GObject *obj, guint prop_id, GValue *value, 
                                     GParamSpec *pspec)
{
	BastilePkcs11Object *self = BASTILE_PKCS11_OBJECT (obj);
	
	switch (prop_id) {
	case PROP_PKCS11_OBJECT:
		g_value_set_object (value, bastile_pkcs11_object_get_pkcs11_object (self));
		break;
	case PROP_PKCS11_ATTRIBUTES:
		g_value_set_boxed (value, bastile_pkcs11_object_get_pkcs11_attributes (self));
		break;
	case PROP_PKCS11_HANDLE:
		g_value_set_ulong (value, bastile_pkcs11_object_get_pkcs11_handle (self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
bastile_pkcs11_object_set_property (GObject *obj, guint prop_id, const GValue *value, 
                                     GParamSpec *pspec)
{
	BastilePkcs11Object *self = BASTILE_PKCS11_OBJECT (obj);
	
	switch (prop_id) {
	case PROP_PKCS11_OBJECT:
		g_return_if_fail (!self->pv->pkcs11_object);
		self->pv->pkcs11_object = g_value_get_object (value);
		g_return_if_fail (self->pv->pkcs11_object);
		g_object_ref (self->pv->pkcs11_object);
		break;
	case PROP_PKCS11_ATTRIBUTES:
		bastile_pkcs11_object_set_pkcs11_attributes (self, g_value_get_boxed (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
bastile_pkcs11_object_class_init (BastilePkcs11ObjectClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	BastileObjectClass *bastile_class = BASTILE_OBJECT_CLASS (klass);
	
	bastile_pkcs11_object_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (BastilePkcs11ObjectPrivate));

	gobject_class->constructor = bastile_pkcs11_object_constructor;
	gobject_class->dispose = bastile_pkcs11_object_dispose;
	gobject_class->finalize = bastile_pkcs11_object_finalize;
	gobject_class->set_property = bastile_pkcs11_object_set_property;
	gobject_class->get_property = bastile_pkcs11_object_get_property;

	bastile_class->realize = bastile_pkcs11_object_realize;
	bastile_class->refresh = bastile_pkcs11_object_refresh;
	bastile_class->delete = bastile_pkcs11_object_delete;
	
	g_object_class_install_property (gobject_class, PROP_PKCS11_OBJECT, 
	         g_param_spec_object ("pkcs11-object", "pkcs11-object", "pkcs11-object", GCK_TYPE_OBJECT,
	                              G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
	
	g_object_class_install_property (gobject_class, PROP_PKCS11_ATTRIBUTES, 
	         g_param_spec_boxed ("pkcs11-attributes", "pkcs11-attributes", "pkcs11-attributes", GCK_TYPE_ATTRIBUTES,
	                             G_PARAM_READABLE | G_PARAM_WRITABLE));
	
	g_object_class_install_property (gobject_class, PROP_PKCS11_HANDLE, 
	         g_param_spec_ulong ("pkcs11-handle", "pkcs11-handle", "pkcs11-handle", 
	                             0, G_MAXULONG, 0, G_PARAM_READABLE));
}

/* -----------------------------------------------------------------------------
 * PUBLIC 
 */

BastilePkcs11Object*
bastile_pkcs11_object_new (GckObject* object)
{
	return g_object_new (BASTILE_PKCS11_TYPE_OBJECT, 
	                     "pkcs11-object", object, NULL);
}

GckObject*
bastile_pkcs11_object_get_pkcs11_object (BastilePkcs11Object* self) 
{
	g_return_val_if_fail (BASTILE_PKCS11_IS_OBJECT (self), NULL);
	return self->pv->pkcs11_object;
}

GckAttributes*
bastile_pkcs11_object_get_pkcs11_attributes (BastilePkcs11Object* self) 
{
	g_return_val_if_fail (BASTILE_PKCS11_IS_OBJECT (self), NULL);
	return self->pv->pkcs11_attributes;
}


void 
bastile_pkcs11_object_set_pkcs11_attributes (BastilePkcs11Object* self, GckAttributes* value)
{
	GObject *obj;
	
	g_return_if_fail (BASTILE_PKCS11_IS_OBJECT (self));
	g_return_if_fail (value);
	
	if (value)
		gck_attributes_ref (value);
	if (self->pv->pkcs11_attributes)
		gck_attributes_unref (self->pv->pkcs11_attributes);
	self->pv->pkcs11_attributes = value;
	
	obj = G_OBJECT (self);
	g_object_freeze_notify (obj);
	bastile_pkcs11_object_realize (BASTILE_OBJECT (obj));
	g_object_notify (obj, "pkcs11-attributes");
	g_object_thaw_notify (obj);

	gck_attributes_find (self->pv->pkcs11_attributes, CKA_LABEL);
}

gulong
bastile_pkcs11_object_get_pkcs11_handle (BastilePkcs11Object* self)
{
	g_return_val_if_fail (BASTILE_PKCS11_IS_OBJECT (self), 0);
	if (self->pv->pkcs11_object)
		return gck_object_get_handle (self->pv->pkcs11_object);
	return GCK_INVALID;
}

GckAttribute*
bastile_pkcs11_object_require_attribute (BastilePkcs11Object *self, gulong attr_type)
{
	GckAttribute* attr;

	g_return_val_if_fail (BASTILE_PKCS11_IS_OBJECT (self), NULL);

	attr = gck_attributes_find (self->pv->pkcs11_attributes, attr_type);
	if (attr == NULL)
		load_object_attributes (self, &attr_type, 1);
	if (gck_attribute_is_invalid (attr))
		attr = NULL;
	
	return attr;
}

gboolean
bastile_pkcs11_object_require_attributes (BastilePkcs11Object *self, const gulong *attr_types,
                                           gsize n_attr_types)
{
	gsize i;
	
	g_return_val_if_fail (BASTILE_PKCS11_IS_OBJECT (self), FALSE);
	
	/* See if we have these attributes loaded */
	for (i = 0; i < n_attr_types; ++i) {
		if (!gck_attributes_find (self->pv->pkcs11_attributes, attr_types[i]))
			break;
	}
		
	if (i == n_attr_types)
		return TRUE;
	
	/* Go ahead and load these attributes */
	load_object_attributes (self, attr_types, n_attr_types);
	return FALSE;
}

GQuark
bastile_pkcs11_object_cannonical_id (GckObject *object)
{
	GckSession *session;
	GckSlot *slot;
	GQuark quark;
	gchar *text;

	/* TODO: This whole ID thing needs rethinking */

	session = gck_object_get_session (object);
	slot = gck_session_get_slot (session);

	text = g_strdup_printf("%s:%lu/%lu", BASTILE_PKCS11_TYPE_STR, 
	                       gck_slot_get_handle (slot),
	                       gck_object_get_handle (object));

	g_object_unref (session);
	g_object_unref (slot);

	quark = g_quark_from_string (text);
	g_free (text);
	return quark;	
}
