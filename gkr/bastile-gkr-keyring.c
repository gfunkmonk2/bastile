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

#include "bastile-gkr.h"
#include "bastile-gkr-keyring.h"
#include "bastile-gkr-operation.h"

#include <glib/gi18n.h>

/* -----------------------------------------------------------------------------
 * LIST OPERATION 
 */
 

#define BASTILE_TYPE_GKR_LIST_OPERATION            (bastile_gkr_list_operation_get_type ())
#define BASTILE_GKR_LIST_OPERATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_GKR_LIST_OPERATION, BastileGkrListOperation))
#define BASTILE_GKR_LIST_OPERATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_GKR_LIST_OPERATION, BastileGkrListOperationClass))
#define BASTILE_IS_GKR_LIST_OPERATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_GKR_LIST_OPERATION))
#define BASTILE_IS_GKR_LIST_OPERATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_GKR_LIST_OPERATION))
#define BASTILE_GKR_LIST_OPERATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_GKR_LIST_OPERATION, BastileGkrListOperationClass))

typedef struct _BastileGkrListOperation BastileGkrListOperation;
typedef struct _BastileGkrListOperationClass BastileGkrListOperationClass;

struct _BastileGkrListOperation {
	BastileOperation parent;
	BastileGkrKeyring *keyring;
	gpointer request;
};

struct _BastileGkrListOperationClass {
	BastileOperationClass parent_class;
};

G_DEFINE_TYPE (BastileGkrListOperation, bastile_gkr_list_operation, BASTILE_TYPE_OPERATION);

static void
keyring_operation_failed (BastileGkrListOperation *self, MateKeyringResult result)
{
	BastileOperation *op = BASTILE_OPERATION (self);
	GError *err = NULL;
    
	g_assert (result != MATE_KEYRING_RESULT_OK);
	g_assert (result != MATE_KEYRING_RESULT_CANCELLED);
    
	if (!bastile_operation_is_running (op))
		return;
    
	if (self->request)
	    mate_keyring_cancel_request (self->request);
	self->request = NULL;
    
	bastile_gkr_operation_parse_error (result, &err);
	g_assert (err != NULL);
    
	bastile_operation_mark_done (op, FALSE, err);
}

/* Remove the given key from the context */
static void
remove_key_from_context (gpointer kt, BastileObject *dummy, BastileSource *sksrc)
{
	/* This function gets called as a GHFunc on the self->checks hashtable. */
	GQuark keyid = GPOINTER_TO_UINT (kt);
	BastileObject *sobj;
    
	sobj = bastile_context_get_object (SCTX_APP (), sksrc, keyid);
	if (sobj != NULL)
		bastile_context_remove_object (SCTX_APP (), sobj);
}

static void
insert_id_hashtable (BastileObject *object, gpointer user_data)
{
	g_hash_table_insert ((GHashTable*)user_data, 
	                     GUINT_TO_POINTER (bastile_object_get_id (object)),
	                     GUINT_TO_POINTER (TRUE));
}

static void 
on_list_item_ids (MateKeyringResult result, GList *list, BastileGkrListOperation *self)
{
	BastileObjectPredicate pred;
	BastileGkrItem *git;
	const gchar *keyring_name;
	GHashTable *checks;
	guint32 item_id;
	GQuark id;
	
	if (result == MATE_KEYRING_RESULT_CANCELLED)
		return;
    
	self->request = NULL;

	if (result != MATE_KEYRING_RESULT_OK) {
		keyring_operation_failed (self, result);
		return;
	}
	
	keyring_name = bastile_gkr_keyring_get_name (self->keyring);
	g_return_if_fail (keyring_name);

	/* When loading new keys prepare a list of current */
	checks = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);
	bastile_object_predicate_clear (&pred);
	pred.source = BASTILE_SOURCE (self->keyring);
	pred.type = BASTILE_TYPE_GKR_ITEM;
	bastile_context_for_objects_full (SCTX_APP (), &pred, insert_id_hashtable, checks);
    
	while (list) {
		item_id = GPOINTER_TO_UINT (list->data);
		id = bastile_gkr_item_get_cannonical (keyring_name, item_id);
		
		git = BASTILE_GKR_ITEM (bastile_context_get_object (SCTX_APP (), 
		                                                      BASTILE_SOURCE (self->keyring), id));
		if (!git) {
			git = bastile_gkr_item_new (BASTILE_SOURCE (self->keyring), keyring_name, item_id);

			/* Add to context */ 
			bastile_object_set_parent (BASTILE_OBJECT (git), BASTILE_OBJECT (self->keyring));
			bastile_context_take_object (SCTX_APP (), BASTILE_OBJECT (git));
		}
		
		g_hash_table_remove (checks, GUINT_TO_POINTER (id));
		list = g_list_next (list);
	}
	
	g_hash_table_foreach (checks, (GHFunc)remove_key_from_context, 
	                      BASTILE_SOURCE (self->keyring));
	
	g_hash_table_destroy (checks);
        
        bastile_operation_mark_done (BASTILE_OPERATION (self), FALSE, NULL);
}

static void
on_keyring_info (MateKeyringResult result, MateKeyringInfo *info, BastileGkrListOperation *self)
{
	if (result == MATE_KEYRING_RESULT_CANCELLED)
		return;
    
	self->request = NULL;

	if (result != MATE_KEYRING_RESULT_OK) {
		keyring_operation_failed (self, result);
		return;
	}

	bastile_gkr_keyring_set_info (self->keyring, info);
	if (mate_keyring_info_get_is_locked (info)) {
		/* Locked, no items... */
		on_list_item_ids (MATE_KEYRING_RESULT_OK, NULL, self);
	} else {
		g_object_ref (self);
		self->request = mate_keyring_list_item_ids (bastile_gkr_keyring_get_name (self->keyring), 
		                                            (MateKeyringOperationGetListCallback)on_list_item_ids, 
		                                            self, g_object_unref);
	}
}

static BastileOperation*
start_gkr_list_operation (BastileGkrKeyring *keyring)
{
	BastileGkrListOperation *self;

	g_assert (BASTILE_IS_GKR_KEYRING (keyring));

	self = g_object_new (BASTILE_TYPE_GKR_LIST_OPERATION, NULL);
	self->keyring = keyring;
    
	/* Start listing of ids */
	bastile_operation_mark_start (BASTILE_OPERATION (self));
	bastile_operation_mark_progress (BASTILE_OPERATION (self), _("Listing passwords"), -1);
	
	g_object_ref (self);
	self->request = mate_keyring_get_info (bastile_gkr_keyring_get_name (keyring), 
	                                        (MateKeyringOperationGetKeyringInfoCallback)on_keyring_info, 
	                                        self, g_object_unref);
    
	return BASTILE_OPERATION (self);
}

static void 
bastile_gkr_list_operation_cancel (BastileOperation *operation)
{
	BastileGkrListOperation *self = BASTILE_GKR_LIST_OPERATION (operation);    

	if (self->request)
		mate_keyring_cancel_request (self->request);
	self->request = NULL;
    
	if (bastile_operation_is_running (operation))
		bastile_operation_mark_done (operation, TRUE, NULL);
}

static void 
bastile_gkr_list_operation_init (BastileGkrListOperation *self)
{	
	/* Everything already set to zero */
}

static void 
bastile_gkr_list_operation_dispose (GObject *gobject)
{
	BastileGkrListOperation *self = BASTILE_GKR_LIST_OPERATION (gobject);
    
	/* Cancel it if it's still running */
	if (bastile_operation_is_running (BASTILE_OPERATION (self)))
		bastile_gkr_list_operation_cancel (BASTILE_OPERATION (self));
	g_assert (!bastile_operation_is_running (BASTILE_OPERATION (self)));
    
	/* The above cancel should have stopped this */
	g_assert (self->request == NULL);

	G_OBJECT_CLASS (bastile_gkr_list_operation_parent_class)->dispose (gobject);
}

static void 
bastile_gkr_list_operation_finalize (GObject *gobject)
{
	BastileGkrListOperation *self = BASTILE_GKR_LIST_OPERATION (gobject);
	g_assert (!bastile_operation_is_running (BASTILE_OPERATION (self)));
    
	/* The above cancel should have stopped this */
	g_assert (self->request == NULL);

	G_OBJECT_CLASS (bastile_gkr_list_operation_parent_class)->finalize (gobject);
}

static void
bastile_gkr_list_operation_class_init (BastileGkrListOperationClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	BastileOperationClass *operation_class = BASTILE_OPERATION_CLASS (klass);
	
	bastile_gkr_list_operation_parent_class = g_type_class_peek_parent (klass);
	gobject_class->dispose = bastile_gkr_list_operation_dispose;
	gobject_class->finalize = bastile_gkr_list_operation_finalize;
	operation_class->cancel = bastile_gkr_list_operation_cancel;
    
}

/* -----------------------------------------------------------------------------------------
 * KEYRING
 */

enum {
	PROP_0,
	PROP_SOURCE_TAG,
	PROP_SOURCE_LOCATION,
	PROP_KEYRING_NAME,
	PROP_KEYRING_INFO,
	PROP_IS_DEFAULT
};

struct _BastileGkrKeyringPrivate {
	gchar *keyring_name;
	gboolean is_default;
	
	gpointer req_info;
	MateKeyringInfo *keyring_info;
};

static void bastile_source_iface (BastileSourceIface *iface);

G_DEFINE_TYPE_EXTENDED (BastileGkrKeyring, bastile_gkr_keyring, BASTILE_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (BASTILE_TYPE_SOURCE, bastile_source_iface));

GType
boxed_type_keyring_info (void)
{
	static GType type = 0;
	if (!type)
		type = g_boxed_type_register_static ("MateKeyringInfo", 
		                                     (GBoxedCopyFunc)mate_keyring_info_copy,
		                                     (GBoxedFreeFunc)mate_keyring_info_free);
	return type;
}

/* -----------------------------------------------------------------------------
 * INTERNAL 
 */

static void
received_keyring_info (MateKeyringResult result, MateKeyringInfo *info, gpointer data)
{
	BastileGkrKeyring *self = BASTILE_GKR_KEYRING (data);
	self->pv->req_info = NULL;
	
	if (result == MATE_KEYRING_RESULT_CANCELLED)
		return;
	
	if (result != MATE_KEYRING_RESULT_OK) {
		/* TODO: Implement so that we can display an error icon, along with some text */
		g_message ("failed to retrieve info for keyring %s: %s",
		           self->pv->keyring_name, 
		           mate_keyring_result_to_message (result));
		return;
	}

	bastile_gkr_keyring_set_info (self, info);
}

static void
load_keyring_info (BastileGkrKeyring *self)
{
	/* Already in progress */
	if (!self->pv->req_info) {
		g_object_ref (self);
		self->pv->req_info = mate_keyring_get_info (self->pv->keyring_name,
		                                             received_keyring_info,
		                                             self, g_object_unref);
	}
}

static gboolean
require_keyring_info (BastileGkrKeyring *self)
{
	if (!self->pv->keyring_info)
		load_keyring_info (self);
	return self->pv->keyring_info != NULL;
}

/* -----------------------------------------------------------------------------
 * OBJECT 
 */

static void
bastile_gkr_keyring_realize (BastileObject *obj)
{
	BastileGkrKeyring *self = BASTILE_GKR_KEYRING (obj);
	gchar *name, *markup;
	
	name = g_strdup_printf (_("Passwords: %s"), self->pv->keyring_name);
	markup = g_markup_printf_escaped (_("<b>Passwords:</b> %s"), self->pv->keyring_name);
	
	g_object_set (self,
	              "label", name,
	              "markup", markup,
	              "nickname", self->pv->keyring_name,
	              "identifier", "",
	              "description", "",
	              "flags", 0,
	              "icon", "folder",
	              "usage", BASTILE_USAGE_OTHER,
	              NULL);
	
	g_free (name);
	g_free (markup);
	
	BASTILE_OBJECT_CLASS (bastile_gkr_keyring_parent_class)->realize (obj);
}

static void
bastile_gkr_keyring_refresh (BastileObject *obj)
{
	BastileGkrKeyring *self = BASTILE_GKR_KEYRING (obj);

	if (self->pv->keyring_info)
		load_keyring_info (self);
	
	BASTILE_OBJECT_CLASS (bastile_gkr_keyring_parent_class)->refresh (obj);
}

static BastileOperation*
bastile_gkr_keyring_delete (BastileObject *obj)
{
	BastileGkrKeyring *self = BASTILE_GKR_KEYRING (obj);
	return bastile_gkr_operation_delete_keyring (self);
}

static BastileOperation*
bastile_gkr_keyring_load (BastileSource *src)
{
	BastileGkrKeyring *self = BASTILE_GKR_KEYRING (src);
	return start_gkr_list_operation (self);
}

static GObject* 
bastile_gkr_keyring_constructor (GType type, guint n_props, GObjectConstructParam *props) 
{
	GObject *obj = G_OBJECT_CLASS (bastile_gkr_keyring_parent_class)->constructor (type, n_props, props);
	BastileGkrKeyring *self = NULL;
	gchar *id;
	
	if (obj) {
		self = BASTILE_GKR_KEYRING (obj);
		
		g_return_val_if_fail (self->pv->keyring_name, obj);
		id = g_strdup_printf ("%s:%s", BASTILE_GKR_TYPE_STR, self->pv->keyring_name);
		g_object_set (self, 
		              "id", g_quark_from_string (id), 
		              "usage", BASTILE_USAGE_NONE, 
		              NULL);
		g_free (id);
	}
	
	return obj;
}

static void
bastile_gkr_keyring_init (BastileGkrKeyring *self)
{
	self->pv = G_TYPE_INSTANCE_GET_PRIVATE (self, BASTILE_TYPE_GKR_KEYRING, BastileGkrKeyringPrivate);
	g_object_set (self, "tag", BASTILE_GKR_TYPE, "location", BASTILE_LOCATION_LOCAL, NULL);
}

static void
bastile_gkr_keyring_finalize (GObject *obj)
{
	BastileGkrKeyring *self = BASTILE_GKR_KEYRING (obj);
	
	if (self->pv->keyring_info)
		mate_keyring_info_free (self->pv->keyring_info);
	self->pv->keyring_info = NULL;
	g_assert (self->pv->req_info == NULL);
    
	g_free (self->pv->keyring_name);
	self->pv->keyring_name = NULL;
	
	if (self->pv->keyring_info) 
		mate_keyring_info_free (self->pv->keyring_info);
	self->pv->keyring_info = NULL;
	
	G_OBJECT_CLASS (bastile_gkr_keyring_parent_class)->finalize (obj);
}

static void
bastile_gkr_keyring_set_property (GObject *obj, guint prop_id, const GValue *value, 
                                   GParamSpec *pspec)
{
	BastileGkrKeyring *self = BASTILE_GKR_KEYRING (obj);
	
	switch (prop_id) {
	case PROP_KEYRING_NAME:
		g_return_if_fail (self->pv->keyring_name == NULL);
		self->pv->keyring_name = g_value_dup_string (value);
		g_object_notify (obj, "keyring-name");
		break;
	case PROP_KEYRING_INFO:
		bastile_gkr_keyring_set_info (self, g_value_get_boxed (value));
		break;
	case PROP_IS_DEFAULT:
		bastile_gkr_keyring_set_is_default (self, g_value_get_boolean (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
bastile_gkr_keyring_get_property (GObject *obj, guint prop_id, GValue *value, 
                           GParamSpec *pspec)
{
	BastileGkrKeyring *self = BASTILE_GKR_KEYRING (obj);
	
	switch (prop_id) {
	case PROP_SOURCE_TAG:
		g_value_set_uint (value, BASTILE_GKR_TYPE);
		break;
	case PROP_SOURCE_LOCATION:
		g_value_set_enum (value, BASTILE_LOCATION_LOCAL);
		break;
	case PROP_KEYRING_NAME:
		g_value_set_string (value, bastile_gkr_keyring_get_name (self));
		break;
	case PROP_KEYRING_INFO:
		g_value_set_boxed (value, bastile_gkr_keyring_get_info (self));
		break;
	case PROP_IS_DEFAULT:
		g_value_set_boolean (value, bastile_gkr_keyring_get_is_default (self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
bastile_gkr_keyring_class_init (BastileGkrKeyringClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	BastileObjectClass *bastile_class;
	
	bastile_gkr_keyring_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (BastileGkrKeyringPrivate));

	gobject_class->constructor = bastile_gkr_keyring_constructor;
	gobject_class->finalize = bastile_gkr_keyring_finalize;
	gobject_class->set_property = bastile_gkr_keyring_set_property;
	gobject_class->get_property = bastile_gkr_keyring_get_property;
    
	bastile_class = BASTILE_OBJECT_CLASS (klass);
	bastile_class->realize = bastile_gkr_keyring_realize;
	bastile_class->refresh = bastile_gkr_keyring_refresh;
	bastile_class->delete = bastile_gkr_keyring_delete;
	
	g_object_class_override_property (gobject_class, PROP_SOURCE_TAG, "source-tag");
	g_object_class_override_property (gobject_class, PROP_SOURCE_LOCATION, "source-location");

	g_object_class_install_property (gobject_class, PROP_KEYRING_NAME,
	           g_param_spec_string ("keyring-name", "Mate Keyring Name", "Name of keyring.", 
	                                "", G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (gobject_class, PROP_KEYRING_INFO,
	           g_param_spec_boxed ("keyring-info", "Mate Keyring Info", "Info about keyring.", 
	                               boxed_type_keyring_info (), G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class, PROP_IS_DEFAULT,
	           g_param_spec_boolean ("is-default", "Is default", "Is the default keyring.",
	                                 FALSE, G_PARAM_READWRITE));
}

static void 
bastile_source_iface (BastileSourceIface *iface)
{
	iface->load = bastile_gkr_keyring_load;
}

/* -----------------------------------------------------------------------------
 * PUBLIC 
 */

BastileGkrKeyring*
bastile_gkr_keyring_new (const gchar *keyring_name)
{
	return g_object_new (BASTILE_TYPE_GKR_KEYRING, "keyring-name", keyring_name, NULL);
}

const gchar*
bastile_gkr_keyring_get_name (BastileGkrKeyring *self)
{
	g_return_val_if_fail (BASTILE_IS_GKR_KEYRING (self), NULL);
	return self->pv->keyring_name;
}

MateKeyringInfo*
bastile_gkr_keyring_get_info (BastileGkrKeyring *self)
{
	g_return_val_if_fail (BASTILE_IS_GKR_KEYRING (self), NULL);
	require_keyring_info (self);
	return self->pv->keyring_info;
}

void
bastile_gkr_keyring_set_info (BastileGkrKeyring *self, MateKeyringInfo *info)
{
	GObject *obj;
	
	g_return_if_fail (BASTILE_IS_GKR_KEYRING (self));
	
	if (self->pv->keyring_info)
		mate_keyring_info_free (self->pv->keyring_info);
	if (info)
		self->pv->keyring_info = mate_keyring_info_copy (info);
	else
		self->pv->keyring_info = NULL;
	
	obj = G_OBJECT (self);
	g_object_freeze_notify (obj);
	bastile_gkr_keyring_realize (BASTILE_OBJECT (self));
	g_object_notify (obj, "keyring-info");
	g_object_thaw_notify (obj);
}

gboolean
bastile_gkr_keyring_get_is_default (BastileGkrKeyring *self)
{
	g_return_val_if_fail (BASTILE_IS_GKR_KEYRING (self), FALSE);
	return self->pv->is_default;
}

void
bastile_gkr_keyring_set_is_default (BastileGkrKeyring *self, gboolean is_default)
{
	g_return_if_fail (BASTILE_IS_GKR_KEYRING (self));
	self->pv->is_default = is_default;
	g_object_notify (G_OBJECT (self), "is-default");
}
