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

#include "bastile-gkr-item.h"
#include "bastile-gkr-keyring.h"
#include "bastile-gkr-operation.h"
#include "bastile-gkr-source.h"

#include "common/bastile-registry.h"

#include <mate-keyring.h>

/* Override the DEBUG_REFRESH_ENABLE switch here */
#define DEBUG_REFRESH_ENABLE 0

#ifndef DEBUG_REFRESH_ENABLE
#if _DEBUG
#define DEBUG_REFRESH_ENABLE 1
#else
#define DEBUG_REFRESH_ENABLE 0
#endif
#endif

#if DEBUG_REFRESH_ENABLE
#define DEBUG_REFRESH(x)    g_printerr(x)
#else
#define DEBUG_REFRESH(x)
#endif

enum {
    PROP_0,
    PROP_SOURCE_TAG,
    PROP_SOURCE_LOCATION,
    PROP_FLAGS
};

static void bastile_source_iface (BastileSourceIface *iface);

G_DEFINE_TYPE_EXTENDED (BastileGkrSource, bastile_gkr_source, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (BASTILE_TYPE_SOURCE, bastile_source_iface));

static BastileGkrSource *default_source = NULL;

/* -----------------------------------------------------------------------------
 * LIST OPERATION 
 */
 

#define BASTILE_TYPE_LIST_OPERATION            (bastile_list_operation_get_type ())
#define BASTILE_LIST_OPERATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_LIST_OPERATION, BastileListOperation))
#define BASTILE_LIST_OPERATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_LIST_OPERATION, BastileListOperationClass))
#define BASTILE_IS_LIST_OPERATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_LIST_OPERATION))
#define BASTILE_IS_LIST_OPERATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_LIST_OPERATION))
#define BASTILE_LIST_OPERATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_LIST_OPERATION, BastileListOperationClass))

typedef struct _BastileListOperation BastileListOperation;
typedef struct _BastileListOperationClass BastileListOperationClass;

struct _BastileListOperation {
	BastileOperation parent;
	BastileGkrSource *gsrc;
	gpointer request;
	BastileOperation *loads;
};

struct _BastileListOperationClass {
	BastileOperationClass parent_class;
};

G_DEFINE_TYPE (BastileListOperation, bastile_list_operation, BASTILE_TYPE_OPERATION);

static void
on_loads_complete (BastileOperation *op, gpointer user_data)
{
	BastileListOperation *self = user_data;
	GError *error = NULL;
	
	g_return_if_fail (BASTILE_IS_LIST_OPERATION (self));
	g_return_if_fail (self->loads == op);
	
	bastile_operation_copy_error (op, &error);
	bastile_operation_mark_done (BASTILE_OPERATION (self), 
	                              bastile_operation_is_cancelled (op),
	                              error);
}

static void
on_loads_progress (BastileOperation *op, const gchar *status, gdouble progress, gpointer user_data)
{
	BastileListOperation *self = user_data;
	
	g_return_if_fail (BASTILE_IS_LIST_OPERATION (self));
	g_return_if_fail (self->loads == op);

	bastile_operation_mark_progress (BASTILE_OPERATION (self),
	                                  status, progress);
}

static void
remove_each_keyring_from_context (const gchar *keyring_name, BastileObject *keyring, 
                                  gpointer unused)
{
	bastile_context_remove_object (NULL, keyring);
	bastile_context_remove_source (NULL, BASTILE_SOURCE (keyring));
}

static void
insert_each_keyring_in_hashtable (BastileObject *object, gpointer user_data)
{
	g_hash_table_insert ((GHashTable*)user_data, 
	                     g_strdup (bastile_gkr_keyring_get_name (BASTILE_GKR_KEYRING (object))),
	                     g_object_ref (object));
}

static void 
on_keyring_names (MateKeyringResult result, GList *list, BastileListOperation *self)
{
	BastileGkrKeyring *keyring;
	BastileObjectPredicate pred;
	BastileOperation *oper;
	GError *err = NULL;
	gchar *keyring_name;
	GHashTable *checks;
	GList *l;

	if (result == MATE_KEYRING_RESULT_CANCELLED)
		return;

	self->request = NULL;

	if (!bastile_operation_is_running (BASTILE_OPERATION (self)))
		return;

	if (result != MATE_KEYRING_RESULT_OK) {
	    
		g_assert (result != MATE_KEYRING_RESULT_OK);
		g_assert (result != MATE_KEYRING_RESULT_CANCELLED);
	    
		if (self->request)
			mate_keyring_cancel_request (self->request);
		self->request = NULL;
	    
		bastile_gkr_operation_parse_error (result, &err);
		g_assert (err != NULL);
		
		bastile_operation_mark_done (BASTILE_OPERATION (self), FALSE, err);
		return;
	}
	
	/* Load up a list of all the current names */
	checks = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
	bastile_object_predicate_clear (&pred);
	pred.source = BASTILE_SOURCE (self->gsrc);
	pred.type = BASTILE_TYPE_GKR_KEYRING;
	bastile_context_for_objects_full (NULL, &pred, insert_each_keyring_in_hashtable, checks);

	for (l = list; l; l = g_list_next (l)) {
		keyring_name = l->data;
		
		/* Don't show the 'session' keyring */
		if (g_str_equal (keyring_name, "session"))
			continue;
		
		keyring = g_hash_table_lookup (checks, keyring_name);

		/* Already have a keyring */
		if (keyring != NULL) {
			g_object_ref (keyring);
			g_hash_table_remove (checks, keyring_name);
			
		/* Create a new keyring for this one */
		} else {
			keyring = bastile_gkr_keyring_new (keyring_name);
			g_object_set (keyring, "source", self->gsrc, NULL);
			bastile_context_add_source (NULL, BASTILE_SOURCE (keyring));
			bastile_context_add_object (NULL, BASTILE_OBJECT (keyring));
		}
		
		/* Refresh the keyring as well, and track the load */
		oper = bastile_source_load (BASTILE_SOURCE (keyring));
		bastile_multi_operation_take (BASTILE_MULTI_OPERATION (self->loads), oper);
		g_object_unref (keyring);
	}
	
	g_hash_table_foreach (checks, (GHFunc)remove_each_keyring_from_context, NULL);
	g_hash_table_destroy (checks);
        
	/* Watch the loads until they're done */
	bastile_operation_watch (self->loads, on_loads_complete, self, on_loads_progress, self);
}

static BastileOperation*
start_list_operation (BastileGkrSource *gsrc)
{
	BastileListOperation *self;

	g_assert (BASTILE_IS_GKR_SOURCE (gsrc));

	self = g_object_new (BASTILE_TYPE_LIST_OPERATION, NULL);
	self->gsrc = g_object_ref (gsrc);
    
	bastile_operation_mark_start (BASTILE_OPERATION (self));
	bastile_operation_mark_progress (BASTILE_OPERATION (self), _("Listing password keyrings"), -1);
  
	/* Start listing of ids */
	g_object_ref (self);
	self->request = mate_keyring_list_keyring_names ((MateKeyringOperationGetListCallback)on_keyring_names, 
	                                                  self, g_object_unref);
    
	return BASTILE_OPERATION (self);
}

static void 
bastile_list_operation_cancel (BastileOperation *operation)
{
	BastileListOperation *self = BASTILE_LIST_OPERATION (operation);    

	if (self->request)
		mate_keyring_cancel_request (self->request);
	self->request = NULL;
	
	if (bastile_operation_is_running (self->loads))
		bastile_operation_cancel (self->loads);
    
	if (bastile_operation_is_running (operation))
		bastile_operation_mark_done (operation, TRUE, NULL);
}

static void 
bastile_list_operation_init (BastileListOperation *self)
{
	/* Everything already set to zero */
	self->loads = BASTILE_OPERATION (bastile_multi_operation_new ());
}

static void 
bastile_list_operation_dispose (GObject *gobject)
{
	BastileListOperation *self = BASTILE_LIST_OPERATION (gobject);
    
	/* Cancel it if it's still running */
	if (bastile_operation_is_running (BASTILE_OPERATION (self)))
		bastile_list_operation_cancel (BASTILE_OPERATION (self));
	g_assert (!bastile_operation_is_running (BASTILE_OPERATION (self)));
    
	/* The above cancel should have stopped these */
	g_assert (self->request == NULL);
	g_return_if_fail (!bastile_operation_is_running (self->loads));
	
	if (self->gsrc)
		g_object_unref (self->gsrc);
	self->gsrc = NULL;

	G_OBJECT_CLASS (bastile_list_operation_parent_class)->dispose (gobject);  
}

static void 
bastile_list_operation_finalize (GObject *gobject)
{
	BastileListOperation *self = BASTILE_LIST_OPERATION (gobject);
	g_assert (!bastile_operation_is_running (BASTILE_OPERATION (self)));
    
	/* The above cancel should have stopped this */
	g_assert (self->request == NULL);
	
	g_object_unref (self->loads);
	self->loads = NULL;

	G_OBJECT_CLASS (bastile_list_operation_parent_class)->finalize (gobject);
}

static void
bastile_list_operation_class_init (BastileListOperationClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	BastileOperationClass *operation_class = BASTILE_OPERATION_CLASS (klass);
	
	bastile_list_operation_parent_class = g_type_class_peek_parent (klass);
	gobject_class->dispose = bastile_list_operation_dispose;
	gobject_class->finalize = bastile_list_operation_finalize;
	operation_class->cancel = bastile_list_operation_cancel;   
}

/* -----------------------------------------------------------------------------
 * INTERNAL
 */

static void
update_each_default_keyring (BastileObject *object, gpointer user_data)
{
	const gchar *default_name = user_data;
	const gchar *keyring_name; 
	gboolean is_default;
	
	keyring_name = bastile_gkr_keyring_get_name (BASTILE_GKR_KEYRING (object));
	g_return_if_fail (keyring_name);
	
	/* Remember default keyring could be null in strange circumstances */
	is_default = default_name && g_str_equal (keyring_name, default_name);
	g_object_set (object, "is-default", is_default, NULL);
}

static void
on_get_default_keyring (MateKeyringResult result, const gchar *default_name, gpointer user_data)
{
	BastileGkrSource *self = user_data;
	BastileObjectPredicate pred;
	
	g_return_if_fail (BASTILE_IS_GKR_SOURCE (self));

	if (result != MATE_KEYRING_RESULT_OK) {
		if (result != MATE_KEYRING_RESULT_CANCELLED)
			g_warning ("couldn't get default keyring name: %s", mate_keyring_result_to_message (result));
		return;
	}
	
	bastile_object_predicate_clear (&pred);
	pred.source = BASTILE_SOURCE (self);
	pred.type = BASTILE_TYPE_GKR_KEYRING;
	bastile_context_for_objects_full (NULL, &pred, update_each_default_keyring, (gpointer)default_name);
}

static void
on_list_operation_done (BastileOperation *op, gpointer userdata)
{
	BastileGkrSource *self = userdata;
	g_return_if_fail (BASTILE_IS_GKR_SOURCE (self));
	
	mate_keyring_get_default_keyring (on_get_default_keyring, g_object_ref (self), g_object_unref);
}

/* -----------------------------------------------------------------------------
 * OBJECT
 */


static GObject* 
bastile_gkr_source_constructor (GType type, guint n_props, GObjectConstructParam *props)
{
	BastileGkrSource *self = BASTILE_GKR_SOURCE (G_OBJECT_CLASS (bastile_gkr_source_parent_class)->constructor (type, n_props, props));
	
	g_return_val_if_fail (BASTILE_IS_GKR_SOURCE (self), NULL);
	
	if (default_source == NULL)
		default_source = self;

	return G_OBJECT (self);

}

static void
bastile_gkr_source_init (BastileGkrSource *self)
{
	self->pv = NULL;
}

static void 
bastile_gkr_source_get_property (GObject *object, guint prop_id, GValue *value, 
                                       GParamSpec *pspec)
{
	switch (prop_id) {
	case PROP_SOURCE_TAG:
		g_value_set_uint (value, BASTILE_GKR);
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
bastile_gkr_source_set_property (GObject *object, guint prop_id, const GValue *value, 
                                  GParamSpec *pspec)
{

}

static void
bastile_gkr_source_finalize (GObject *obj)
{
	BastileGkrSource *self = BASTILE_GKR_SOURCE (obj);

	if (default_source == self)
		default_source = NULL;
	
	G_OBJECT_CLASS (bastile_gkr_source_parent_class)->finalize (obj);
}

static void
bastile_gkr_source_class_init (BastileGkrSourceClass *klass)
{
	GObjectClass *gobject_class;
    
	bastile_gkr_source_parent_class = g_type_class_peek_parent (klass);

	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->constructor = bastile_gkr_source_constructor;
	gobject_class->set_property = bastile_gkr_source_set_property;
	gobject_class->get_property = bastile_gkr_source_get_property;
	gobject_class->finalize = bastile_gkr_source_finalize;
    
	g_object_class_install_property (gobject_class, PROP_FLAGS,
	         g_param_spec_uint ("flags", "Flags", "Object Source flags.", 
	                            0, G_MAXUINT, 0, G_PARAM_READABLE));

	g_object_class_override_property (gobject_class, PROP_SOURCE_TAG, "source-tag");
	g_object_class_override_property (gobject_class, PROP_SOURCE_LOCATION, "source-location");
    
	bastile_registry_register_type (NULL, BASTILE_TYPE_GKR_SOURCE, "source", "local", BASTILE_GKR_STR, NULL);
}

static BastileOperation*
bastile_gkr_source_load (BastileSource *src)
{
	BastileGkrSource *self = BASTILE_GKR_SOURCE (src);
	BastileOperation *op = start_list_operation (self);
	
	g_return_val_if_fail (op, NULL);
	
	/* Hook into the results of the above operation, and look for default */
	bastile_operation_watch (op, on_list_operation_done, src, NULL, NULL);
	
	return op;
}

static void 
bastile_source_iface (BastileSourceIface *iface)
{
	iface->load = bastile_gkr_source_load;
}

/* -------------------------------------------------------------------------- 
 * PUBLIC
 */

BastileGkrSource*
bastile_gkr_source_new (void)
{
   return g_object_new (BASTILE_TYPE_GKR_SOURCE, NULL);
}   

BastileGkrSource*
bastile_gkr_source_default (void)
{
	g_return_val_if_fail (default_source, NULL);
	return default_source;
}
