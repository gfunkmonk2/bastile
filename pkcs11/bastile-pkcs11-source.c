/*
 * Bastile
 *
 * Copyright (C) 2006 Stefan Walter
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
#include <libintl.h>

#include <glib/gi18n.h>

#include "bastile-operation.h"
#include "bastile-util.h"
#include "bastile-secure-memory.h"
#include "bastile-passphrase.h"

#include "bastile-pkcs11.h"
#include "bastile-pkcs11-certificate.h"
#include "bastile-pkcs11-object.h"
#include "bastile-pkcs11-operations.h"
#include "bastile-pkcs11-source.h"

#include "common/bastile-registry.h"

enum {
    PROP_0,
    PROP_SLOT,
    PROP_SOURCE_TAG,
    PROP_SOURCE_LOCATION,
    PROP_FLAGS
};

struct _BastilePkcs11SourcePrivate {
	GckSlot *slot;
};

static void bastile_source_iface (BastileSourceIface *iface);

G_DEFINE_TYPE_EXTENDED (BastilePkcs11Source, bastile_pkcs11_source, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (BASTILE_TYPE_SOURCE, bastile_source_iface));

/* -----------------------------------------------------------------------------
 * OBJECT
 */

static void
bastile_pkcs11_source_init (BastilePkcs11Source *self)
{
	self->pv = (G_TYPE_INSTANCE_GET_PRIVATE (self, BASTILE_TYPE_PKCS11_SOURCE, BastilePkcs11SourcePrivate));
}

static GObject*  
bastile_pkcs11_source_constructor (GType type, guint n_props, GObjectConstructParam* props)
{
	GObject* obj = G_OBJECT_CLASS (bastile_pkcs11_source_parent_class)->constructor (type, n_props, props);
	BastilePkcs11Source *self = NULL;
	
	if (obj) {
		self = BASTILE_PKCS11_SOURCE (obj);
		g_return_val_if_fail (self->pv->slot, NULL);
	}
	
	return obj;
}

static void 
bastile_pkcs11_source_get_property (GObject *object, guint prop_id, GValue *value, 
                                       GParamSpec *pspec)
{
	BastilePkcs11Source *self = BASTILE_PKCS11_SOURCE (object);

	switch (prop_id) {
	case PROP_SLOT:
		g_value_set_object (value, self->pv->slot);
		break;
	case PROP_SOURCE_TAG:
		g_value_set_uint (value, BASTILE_PKCS11_TYPE);
		break;
	case PROP_SOURCE_LOCATION:
		g_value_set_enum (value, BASTILE_LOCATION_LOCAL);
		break;
	case PROP_FLAGS:
		g_value_set_uint (value, 0);
		break;
	}
}

static void 
bastile_pkcs11_source_set_property (GObject *object, guint prop_id, const GValue *value, 
                                     GParamSpec *pspec)
{
	BastilePkcs11Source *self = BASTILE_PKCS11_SOURCE (object);

	switch (prop_id) {
	case PROP_SLOT:
		g_return_if_fail (!self->pv->slot);
		self->pv->slot = g_value_get_object (value);
		g_return_if_fail (self->pv->slot);
		g_object_ref (self->pv->slot);
		break;
	};
}

static BastileOperation*
bastile_pkcs11_source_load (BastileSource *src)
{
	return bastile_pkcs11_refresher_new (BASTILE_PKCS11_SOURCE (src));
}

static void
bastile_pkcs11_source_dispose (GObject *obj)
{
	BastilePkcs11Source *self = BASTILE_PKCS11_SOURCE (obj);
    
	/* The keyring object */
	if (self->pv->slot)
		g_object_unref (self->pv->slot);
	self->pv->slot = NULL;

	G_OBJECT_CLASS (bastile_pkcs11_source_parent_class)->dispose (obj);
}

static void
bastile_pkcs11_source_finalize (GObject *obj)
{
	BastilePkcs11Source *self = BASTILE_PKCS11_SOURCE (obj);
  
	g_assert (self->pv->slot == NULL);
    
	G_OBJECT_CLASS (bastile_pkcs11_source_parent_class)->finalize (obj);
}

static void
bastile_pkcs11_source_class_init (BastilePkcs11SourceClass *klass)
{
	GObjectClass *gobject_class;
    
	bastile_pkcs11_source_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (BastilePkcs11SourcePrivate));
	
	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->constructor = bastile_pkcs11_source_constructor;
	gobject_class->dispose = bastile_pkcs11_source_dispose;
	gobject_class->finalize = bastile_pkcs11_source_finalize;
	gobject_class->set_property = bastile_pkcs11_source_set_property;
	gobject_class->get_property = bastile_pkcs11_source_get_property;
    
	g_object_class_install_property (gobject_class, PROP_SLOT,
	         g_param_spec_object ("slot", "Slot", "Pkcs#11 SLOT",
	                              GCK_TYPE_SLOT, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (gobject_class, PROP_FLAGS,
	         g_param_spec_uint ("flags", "Flags", "Object Source flags.", 
	                            0, G_MAXUINT, 0, G_PARAM_READABLE));

	g_object_class_override_property (gobject_class, PROP_SOURCE_TAG, "source-tag");
	g_object_class_override_property (gobject_class, PROP_SOURCE_LOCATION, "source-location");
    
	bastile_registry_register_type (NULL, BASTILE_TYPE_PKCS11_SOURCE, "source", "local", BASTILE_PKCS11_TYPE_STR, NULL);
}

static void 
bastile_source_iface (BastileSourceIface *iface)
{
	iface->load = bastile_pkcs11_source_load;
}

/* -------------------------------------------------------------------------- 
 * PUBLIC
 */

BastilePkcs11Source*
bastile_pkcs11_source_new (GckSlot *slot)
{
	return g_object_new (BASTILE_TYPE_PKCS11_SOURCE, "slot", slot, NULL);
}

GckSlot*
bastile_pkcs11_source_get_slot (BastilePkcs11Source *self)
{
	g_return_val_if_fail (BASTILE_IS_PKCS11_SOURCE (self), NULL);
	return self->pv->slot;
}

void
bastile_pkcs11_source_receive_object (BastilePkcs11Source *self, GckObject *obj)
{
	GQuark id = bastile_pkcs11_object_cannonical_id (obj);
	BastilePkcs11Certificate *cert;
	BastileObject *prev;
	
	g_return_if_fail (BASTILE_IS_PKCS11_SOURCE (self));
	
	/* TODO: This will need to change once we get other kinds of objects */
	
	prev = bastile_context_get_object (NULL, BASTILE_SOURCE (self), id);
	if (prev) {
		bastile_object_refresh (prev);
		g_object_unref (obj);
		return;
	}

	cert = bastile_pkcs11_certificate_new (obj);
	g_object_set (cert, "source", self, NULL);
	bastile_context_add_object (NULL, BASTILE_OBJECT (cert));
}
