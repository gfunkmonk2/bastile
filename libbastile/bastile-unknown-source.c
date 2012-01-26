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

#include <glib/gi18n.h>

#include "bastile-unknown-source.h"

#include "bastile-context.h"
#include "bastile-unknown.h"

#include "common/bastile-registry.h"

enum {
    PROP_0,
    PROP_SOURCE_TAG,
    PROP_SOURCE_LOCATION,
    PROP_CONSTRUCT_TAG,
};

static void bastile_source_iface (BastileSourceIface *iface);

G_DEFINE_TYPE_EXTENDED (BastileUnknownSource, bastile_unknown_source, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (BASTILE_TYPE_SOURCE, bastile_source_iface));

/* -----------------------------------------------------------------------------
 * INTERNAL
 */

static void
search_done (BastileOperation *op, BastileObject *sobj)
{
	g_object_set (sobj, "location", BASTILE_LOCATION_MISSING, NULL);
}

/* -----------------------------------------------------------------------------
 * OBJECT
 */

static BastileOperation*
bastile_unknown_source_load (BastileSource *src)
{ 
	return bastile_operation_new_complete (NULL);
}

static void 
bastile_unknown_source_set_property (GObject *object, guint prop_id, const GValue *value, 
                                      GParamSpec *pspec)
{
    BastileUnknownSource *usrc = BASTILE_UNKNOWN_SOURCE (object);
    
    switch (prop_id) {
    case PROP_CONSTRUCT_TAG:
        usrc->ktype = g_value_get_uint (value);
        break;
    }
}

static void 
bastile_unknown_source_get_property (GObject *object, guint prop_id, GValue *value, 
                                  GParamSpec *pspec)
{
    BastileUnknownSource *usrc = BASTILE_UNKNOWN_SOURCE (object);
    
    switch (prop_id) {
    case PROP_SOURCE_TAG:
        g_value_set_uint (value, usrc->ktype);
        break;
    case PROP_CONSTRUCT_TAG:
        g_value_set_uint (value, usrc->ktype);
        break;
    case PROP_SOURCE_LOCATION:
        g_value_set_enum (value, BASTILE_LOCATION_MISSING);
        break;
    }
}

static void
bastile_unknown_source_init (BastileUnknownSource *ssrc)
{

}

static void
bastile_unknown_source_class_init (BastileUnknownSourceClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
   
	bastile_unknown_source_parent_class = g_type_class_peek_parent (klass);
    
	gobject_class->set_property = bastile_unknown_source_set_property;
	gobject_class->get_property = bastile_unknown_source_get_property;
    
	g_object_class_override_property (gobject_class, PROP_SOURCE_TAG, "source-tag");
	g_object_class_override_property (gobject_class, PROP_SOURCE_LOCATION, "source-location");
	
	/* 
	 * This is a writable construct only property that lets us construct this  
	 * class with different 'source-tag' property. The 'source-tag' property
	 * is read-only, and so we can't use it directly.
	 */
	g_object_class_install_property (gobject_class, PROP_CONSTRUCT_TAG, 
	         g_param_spec_uint ("construct-tag", "Construct Tag", "Set source-tag during construction of object",
	                            0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    
	bastile_registry_register_type (NULL, BASTILE_TYPE_UNKNOWN_SOURCE, "source", NULL);
}

static void 
bastile_source_iface (BastileSourceIface *iface)
{
	iface->load = bastile_unknown_source_load;
}

/* -----------------------------------------------------------------------------
 * PUBLIC 
 */

BastileUnknownSource*
bastile_unknown_source_new (GQuark ktype)
{
   return g_object_new (BASTILE_TYPE_UNKNOWN_SOURCE, "construct-tag", ktype, NULL);
}

BastileObject*                     
bastile_unknown_source_add_object (BastileUnknownSource *usrc, GQuark id,
                                    BastileOperation *search)
{
    BastileObject *sobj;

    g_return_val_if_fail (id != 0, NULL);

    sobj = bastile_context_get_object (SCTX_APP (), BASTILE_SOURCE (usrc), id);
    if (!sobj) {
        sobj = BASTILE_OBJECT (bastile_unknown_new (usrc, id, NULL));
        bastile_context_add_object (SCTX_APP (), sobj);
    }
    
    if (search) {
        g_object_set (sobj, "location", BASTILE_LOCATION_SEARCHING, NULL);
        bastile_operation_watch (search, (BastileDoneFunc) search_done, sobj, NULL, NULL);
    } else {
        g_object_set (sobj, "location", BASTILE_LOCATION_MISSING, NULL);
    }
    
    return sobj;
}
