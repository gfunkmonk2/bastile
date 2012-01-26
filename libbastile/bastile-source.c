/*
 * Bastile
 *
 * Copyright (C) 2004,2005 Stefan Walter
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

#include "bastile-context.h"
#include "bastile-marshal.h"
#include "bastile-object.h"
#include "bastile-source.h"
#include "bastile-util.h"

#include "common/bastile-registry.h"

/**
 * SECTION:bastile-source
 * @short_description: This class stores and handles key sources
 * @include:bastile-source.h
 *
 **/


/* ---------------------------------------------------------------------------------
 * INTERFACE
 */

/**
* gobject_class: The object class to init
*
* Adds the interfaces "source-tag" and "source-location"
*
**/
static void
bastile_source_base_init (gpointer gobject_class)
{
	static gboolean initialized = FALSE;
	if (!initialized) {
		
		/* Add properties and signals to the interface */
		g_object_interface_install_property (gobject_class,
		        g_param_spec_uint ("source-tag", "Source Tag", "Tag of objects that come from this source.", 
		                           0, G_MAXUINT, BASTILE_TAG_INVALID, G_PARAM_READABLE));

		g_object_interface_install_property (gobject_class, 
		        g_param_spec_enum ("source-location", "Source Location", "Objects in this source are at this location. See BastileLocation", 
		                           BASTILE_TYPE_LOCATION, BASTILE_LOCATION_LOCAL, G_PARAM_READABLE));
		
		initialized = TRUE;
	}
}

/**
 * bastile_source_get_type:
 *
 * Registers the type of G_TYPE_INTERFACE
 *
 * Returns: the type id
 */
GType
bastile_source_get_type (void)
{
	static GType type = 0;
	if (!type) {
		static const GTypeInfo info = {
			sizeof (BastileSourceIface),
			bastile_source_base_init,               /* base init */
			NULL,             /* base finalize */
			NULL,             /* class_init */
			NULL,             /* class finalize */
			NULL,             /* class data */
			0,
			0,                /* n_preallocs */
			NULL,             /* instance init */
		};
		type = g_type_register_static (G_TYPE_INTERFACE, "BastileSourceIface", &info, 0);
		g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	}
	
	return type;
}

/* ---------------------------------------------------------------------------------
 * PUBLIC
 */

/**
 * bastile_source_load:
 * @sksrc: A #BastileSource object
 *
 * Refreshes the #BastileSource's internal object listing.
 *
 * Returns: the asynchronous refresh operation.
 **/
BastileOperation*
bastile_source_load (BastileSource *sksrc)
{
    BastileSourceIface *klass;
    
    g_return_val_if_fail (BASTILE_IS_SOURCE (sksrc), NULL);
    klass = BASTILE_SOURCE_GET_INTERFACE (sksrc);
    g_return_val_if_fail (klass->load != NULL, NULL);
    
    return (*klass->load) (sksrc);
}

/**
 * bastile_source_load_sync:
 * @sksrc: A #BastileSource object
 * 
 * Refreshes the #BastileSource's internal object listing, and waits for it to complete.
 **/
void
bastile_source_load_sync (BastileSource *sksrc)
{
    BastileOperation *op = bastile_source_load (sksrc);
    g_return_if_fail (op != NULL);
    bastile_operation_wait (op);
    g_object_unref (op);
}

/**
 * bastile_source_load_sync:
 * @sksrc: A #BastileSource object
 * 
 * Refreshes the #BastileSource's internal object listing. Completes in the background.
 **/
void
bastile_source_load_async (BastileSource *sksrc)
{
    BastileOperation *op = bastile_source_load (sksrc);
    g_return_if_fail (op != NULL);
    g_object_unref (op);
}

/**
 * bastile_source_search:
 * @sksrc: A #BastileSource object
 * @match: Text to search for
 * 
 * Refreshes the #BastileSource's internal listing. 
 * 
 * Returns: the asynchronous refresh operation.
 **/
BastileOperation*
bastile_source_search (BastileSource *sksrc, const gchar *match)
{
    BastileSourceIface *klass;
    
    g_return_val_if_fail (BASTILE_IS_SOURCE (sksrc), NULL);
    klass = BASTILE_SOURCE_GET_INTERFACE (sksrc);
    g_return_val_if_fail (klass->search != NULL, NULL);
    
    return (*klass->search) (sksrc, match);
}

/**
 * bastile_source_import:
 * @sksrc: A #BastileSource object
 * @input: A stream of data to import
 *
 * Imports data from the stream
 *
 * Returns: the asynchronous import operation
 */
BastileOperation* 
bastile_source_import (BastileSource *sksrc, GInputStream *input)
{
	BastileSourceIface *klass;
    
	g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
    
	g_return_val_if_fail (BASTILE_IS_SOURCE (sksrc), NULL);
	klass = BASTILE_SOURCE_GET_INTERFACE (sksrc);   
	g_return_val_if_fail (klass->import != NULL, NULL);
    
	return (*klass->import) (sksrc, input);  
}

/**
 * bastile_source_import_sync:
 * @sksrc: The #BastileSource
 * @input: the input data
 * @err: error
 *
 * Imports data from the stream
 *
 * Returns: Imports the stream, synchronous
 */
gboolean            
bastile_source_import_sync (BastileSource *sksrc, GInputStream *input,
                             GError **err)
{
	BastileOperation *op;
    	gboolean ret;

	g_return_val_if_fail (G_IS_INPUT_STREAM (input), FALSE);

	op = bastile_source_import (sksrc, input);
	g_return_val_if_fail (op != NULL, FALSE);
    
	bastile_operation_wait (op);
	ret = bastile_operation_is_successful (op);
	if (!ret)
		bastile_operation_copy_error (op, err);
    
	g_object_unref (op);
	return ret;    
}

/**
 * bastile_source_export_objects:
 * @objects: The objects to export
 * @output: The output stream to export the objects to
 *
 * Exports objects. The objects are sorted by source.
 *
 * Returns: The #BastileOperation created to export the data
 */
BastileOperation*
bastile_source_export_objects (GList *objects, GOutputStream *output)
{
    BastileOperation *op = NULL;
    BastileMultiOperation *mop = NULL;
    BastileSource *sksrc;
    BastileObject *sobj;
    GList *next;
    
	g_return_val_if_fail (G_IS_OUTPUT_STREAM (output), NULL);
	g_object_ref (output);

    /* Sort by object source */
    objects = g_list_copy (objects);
    objects = bastile_util_objects_sort (objects);
    
    while (objects) {
     
        /* Break off one set (same source) */
        next = bastile_util_objects_splice (objects);

        g_assert (BASTILE_IS_OBJECT (objects->data));
        sobj = BASTILE_OBJECT (objects->data);

        /* Export from this object source */        
        sksrc = bastile_object_get_source (sobj);
        g_return_val_if_fail (sksrc != NULL, FALSE);
        
        if (op != NULL) {
            if (mop == NULL)
                mop = bastile_multi_operation_new ();
            bastile_multi_operation_take (mop, op);
        }
        
        /* We pass our own data object, to which data is appended */
        op = bastile_source_export (sksrc, objects, output);
        g_return_val_if_fail (op != NULL, FALSE);

        g_list_free (objects);
        objects = next;
    }
    
    if (mop) {
        op = BASTILE_OPERATION (mop);
        
        /* 
         * Setup the result data properly, as we would if it was a 
         * single export operation.
         */
        bastile_operation_mark_result (op, output, g_object_unref);
    }
    
    if (!op) 
        op = bastile_operation_new_complete (NULL);
    
    return op;
}

/**
 * bastile_source_delete_objects:
 * @objects: A list of objects to delete
 *
 * Deletes a list of objects
 *
 * Returns: The #BastileOperation to delete the objects
 */
BastileOperation*
bastile_source_delete_objects (GList *objects)
{
	BastileOperation *op = NULL;
	BastileMultiOperation *mop = NULL;
	BastileObject *sobj;
	GList *l;

	for (l = objects; l; l = g_list_next (l)) {
		
		sobj = BASTILE_OBJECT (l->data);
		g_return_val_if_fail (BASTILE_IS_OBJECT (sobj), NULL);;
		
		if (op != NULL) {
			if (mop == NULL)
				mop = bastile_multi_operation_new ();
			bastile_multi_operation_take (mop, op);
		}

		op = bastile_object_delete (sobj);
		g_return_val_if_fail (op != NULL, NULL);
	}
    
	if (mop) 
		op = BASTILE_OPERATION (mop);
    
	if (!op) 
		op = bastile_operation_new_complete (NULL);
    
	return op;
}

/**
 * bastile_source_export:
 * @sksrc: The BastileSource
 * @objects: The objects to export
 * @output: The resulting output stream
 *
 *
 *
 * Returns: An export Operation (#BastileOperation)
 */
BastileOperation* 
bastile_source_export (BastileSource *sksrc, GList *objects, GOutputStream *output)
{
	BastileSourceIface *klass;
	BastileOperation *op;
	GSList *ids = NULL;
	GList *l;
    
	g_return_val_if_fail (BASTILE_IS_SOURCE (sksrc), NULL);
	g_return_val_if_fail (G_IS_OUTPUT_STREAM (output), NULL);
	
	klass = BASTILE_SOURCE_GET_INTERFACE (sksrc);   
	if (klass->export)
		return (*klass->export) (sksrc, objects, output);

	/* Either export or export_raw must be implemented */
	g_return_val_if_fail (klass->export_raw != NULL, NULL);
    
	for (l = objects; l; l = g_list_next (l)) 
		ids = g_slist_prepend (ids, GUINT_TO_POINTER (bastile_object_get_id (l->data)));
    
	ids = g_slist_reverse (ids);
	op = (*klass->export_raw) (sksrc, ids, output);	
	g_slist_free (ids);
	return op;
}

/**
 * bastile_source_export_raw:
 * @sksrc: The BastileSource
 * @ids: A list of IDs to export
 * @output: The resulting output stream
 *
 *
 *
 * Returns: An export Operation (#BastileOperation)
 */
BastileOperation* 
bastile_source_export_raw (BastileSource *sksrc, GSList *ids, GOutputStream *output)
{
	BastileSourceIface *klass;
	BastileOperation *op;
	BastileObject *sobj;
	GList *objects = NULL;
	GSList *l;
    
	g_return_val_if_fail (BASTILE_IS_SOURCE (sksrc), NULL);
	g_return_val_if_fail (output == NULL || G_IS_OUTPUT_STREAM (output), NULL);

	klass = BASTILE_SOURCE_GET_INTERFACE (sksrc);   
    
	/* Either export or export_raw must be implemented */
	if (klass->export_raw)
		return (*klass->export_raw)(sksrc, ids, output);
    
	g_return_val_if_fail (klass->export != NULL, NULL);
        
	for (l = ids; l; l = g_slist_next (l)) {
		sobj = bastile_context_get_object (SCTX_APP (), sksrc, GPOINTER_TO_UINT (l->data));
        
		/* TODO: A proper error message here 'not found' */
		if (sobj)
			objects = g_list_prepend (objects, sobj);
	}
    
	objects = g_list_reverse (objects);
	op = (*klass->export) (sksrc, objects, output);
	g_list_free (objects);
	return op;
}

/**
* bastile_source_get_tag:
* @sksrc: The bastile source object
*
*
* Returns: The source-tag property of the object. As #GQuark
*/
GQuark              
bastile_source_get_tag (BastileSource *sksrc)
{
    GQuark ktype;
    g_object_get (sksrc, "source-tag", &ktype, NULL);
    return ktype;
}

/**
 * bastile_source_get_location:
 * @sksrc: The bastile source object
 *
 *
 *
 * Returns: The location (#BastileLocation) of this object
 */
BastileLocation   
bastile_source_get_location (BastileSource *sksrc)
{
    BastileLocation loc;
    g_object_get (sksrc, "source-location", &loc, NULL);
    return loc;
}
