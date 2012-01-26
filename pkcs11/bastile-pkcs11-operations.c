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
#include "bastile-pkcs11-source.h"

#include "common/bastile-object-list.h"

#include <pkcs11.h>
#include <gck/gck.h>

#include <glib/gi18n.h>

static void 
bastile_pkcs11_mark_complete (BastileOperation *self, GError *error) 
{
	BastileOperation *operation = BASTILE_OPERATION (self);
	if (error == NULL) 
		bastile_operation_mark_done (operation, FALSE, NULL);
	else if (error->code == CKR_FUNCTION_CANCELED)
		bastile_operation_mark_done (operation, TRUE, NULL);
	else 	
		bastile_operation_mark_done (operation, FALSE, error);
	g_clear_error (&error);
}

/* -----------------------------------------------------------------------------
 * REFRESHER OPERATION
 */

#define BASTILE_TYPE_PKCS11_REFRESHER               (bastile_pkcs11_refresher_get_type ())
#define BASTILE_PKCS11_REFRESHER(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_PKCS11_REFRESHER, BastilePkcs11Refresher))
#define BASTILE_PKCS11_REFRESHER_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_PKCS11_REFRESHER, BastilePkcs11RefresherClass))
#define BASTILE_IS_PKCS11_REFRESHER(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_PKCS11_REFRESHER))
#define BASTILE_IS_PKCS11_REFRESHER_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_PKCS11_REFRESHER))
#define BASTILE_PKCS11_REFRESHER_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_PKCS11_REFRESHER, BastilePkcs11RefresherClass))

typedef struct _BastilePkcs11Refresher BastilePkcs11Refresher;
typedef struct _BastilePkcs11RefresherClass BastilePkcs11RefresherClass;
    
struct _BastilePkcs11Refresher {
	BastileOperation parent;
	GCancellable *cancellable;
	BastilePkcs11Source *source;
	GckSession *session;
	GHashTable *checks;
};

struct _BastilePkcs11RefresherClass {
	BastileOperationClass parent_class;
};

enum {
	PROP_0,
	PROP_SOURCE
};

G_DEFINE_TYPE (BastilePkcs11Refresher, bastile_pkcs11_refresher, BASTILE_TYPE_OPERATION);

static guint
ulong_hash (gconstpointer k)
{
	return (guint)*((gulong*)k); 
}

static gboolean
ulong_equal (gconstpointer a, gconstpointer b)
{
	return *((gulong*)a) == *((gulong*)b); 
}

static gboolean
remove_each_object (gpointer key, gpointer value, gpointer data)
{
	bastile_context_remove_object (NULL, value);
	return TRUE;
}

static void 
on_find_objects(GckSession *session, GAsyncResult *result, BastilePkcs11Refresher *self)
{
	GList *objects, *l;
	GError *err = NULL;
	gulong handle;
	
	g_assert (BASTILE_IS_PKCS11_REFRESHER (self));
	
	objects = gck_session_find_objects_finish (session, result, &err);
	if (err != NULL) {
		bastile_pkcs11_mark_complete (BASTILE_OPERATION (self), err);
		return;
	}

	/* Remove all objects that were found, from the check table */
	for (l = objects; l; l = g_list_next (l)) {
		bastile_pkcs11_source_receive_object (self->source, l->data);
		handle = gck_object_get_handle (l->data);
		g_hash_table_remove (self->checks, &handle);
	}

	/* Remove everything not found from the context */
	g_hash_table_foreach_remove (self->checks, remove_each_object, NULL);

	bastile_pkcs11_mark_complete (BASTILE_OPERATION (self), NULL);
}

static void 
on_open_session(GckSlot *slot, GAsyncResult *result, BastilePkcs11Refresher *self)
{
	GError *err = NULL;
	GckAttributes *attrs;

	g_return_if_fail (BASTILE_IS_PKCS11_REFRESHER (self));

	self->session = gck_slot_open_session_finish (slot, result, &err);
	if (!self->session) {
		bastile_pkcs11_mark_complete (BASTILE_OPERATION (self), err);
		return;
	}
	
	/* Step 2. Load all the objects that we want */
	attrs = gck_attributes_new ();
	gck_attributes_add_boolean (attrs, CKA_TOKEN, TRUE);
	gck_attributes_add_ulong (attrs, CKA_CLASS, CKO_CERTIFICATE);
	gck_session_find_objects_async (self->session, attrs, self->cancellable,
	                                (GAsyncReadyCallback)on_find_objects, self);
	gck_attributes_unref (attrs);
}

static void
bastile_pkcs11_refresher_cancel (BastileOperation *operation) 
{
	BastilePkcs11Refresher *self = BASTILE_PKCS11_REFRESHER (operation);
	g_return_if_fail (BASTILE_IS_PKCS11_REFRESHER (self));
	g_cancellable_cancel (self->cancellable);
}

static GObject* 
bastile_pkcs11_refresher_constructor (GType type, guint n_props, GObjectConstructParam *props) 
{
	BastilePkcs11Refresher *self = BASTILE_PKCS11_REFRESHER (G_OBJECT_CLASS (bastile_pkcs11_refresher_parent_class)->constructor(type, n_props, props));
	GckSlot *slot;
	GList *objects, *l;
	gulong handle;

	g_return_val_if_fail (self, NULL);	
	g_return_val_if_fail (self->source, NULL);	

	objects = bastile_context_get_objects (NULL, BASTILE_SOURCE (self->source));
	for (l = objects; l; l = g_list_next (l)) {
		if (g_object_class_find_property (G_OBJECT_GET_CLASS (l->data), "pkcs11-handle")) {
			g_object_get (l->data, "pkcs11-handle", &handle, NULL);
			g_hash_table_insert (self->checks, g_memdup (&handle, sizeof (handle)), g_object_ref (l->data));
		}
		
	}

	g_list_free (objects);

	/* Step 1. Load the session */
	slot = bastile_pkcs11_source_get_slot (self->source);
	gck_slot_open_session_async (slot, GCK_SESSION_READ_WRITE, self->cancellable,
	                              (GAsyncReadyCallback)on_open_session, self);
	bastile_operation_mark_start (BASTILE_OPERATION (self));
	
	return G_OBJECT (self);
}

static void
bastile_pkcs11_refresher_init (BastilePkcs11Refresher *self)
{
	self->cancellable = g_cancellable_new ();
	self->checks = g_hash_table_new_full (ulong_hash, ulong_equal, g_free, g_object_unref);
}

static void
bastile_pkcs11_refresher_finalize (GObject *obj)
{
	BastilePkcs11Refresher *self = BASTILE_PKCS11_REFRESHER (obj);
	
	if (self->cancellable)
		g_object_unref (self->cancellable);
	self->cancellable = NULL;
	
	if (self->source)
		g_object_unref (self->source);
	self->source = NULL;

	if (self->session)
		g_object_unref (self->session);
	self->session = NULL;

	g_hash_table_destroy (self->checks);

	G_OBJECT_CLASS (bastile_pkcs11_refresher_parent_class)->finalize (obj);
}

static void
bastile_pkcs11_refresher_set_property (GObject *obj, guint prop_id, const GValue *value, 
                                      GParamSpec *pspec)
{
	BastilePkcs11Refresher *self = BASTILE_PKCS11_REFRESHER (obj);
	
	switch (prop_id) {
	case PROP_SOURCE:
		g_return_if_fail (!self->source);
		self->source = g_value_get_object (value);
		g_return_if_fail (self->source);
		g_object_ref (self->source);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
bastile_pkcs11_refresher_get_property (GObject *obj, guint prop_id, GValue *value, 
                                      GParamSpec *pspec)
{
	BastilePkcs11Refresher *self = BASTILE_PKCS11_REFRESHER (obj);
	
	switch (prop_id) {
	case PROP_SOURCE:
		g_value_set_object (value, self->source);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
bastile_pkcs11_refresher_class_init (BastilePkcs11RefresherClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	BastileOperationClass *operation_class = BASTILE_OPERATION_CLASS (klass);
	
	bastile_pkcs11_refresher_parent_class = g_type_class_peek_parent (klass);

	gobject_class->constructor = bastile_pkcs11_refresher_constructor;
	gobject_class->finalize = bastile_pkcs11_refresher_finalize;
	gobject_class->set_property = bastile_pkcs11_refresher_set_property;
	gobject_class->get_property = bastile_pkcs11_refresher_get_property;
	
	operation_class->cancel = bastile_pkcs11_refresher_cancel;
	
	g_object_class_install_property (gobject_class, PROP_SOURCE,
	           g_param_spec_object ("source", "Source", "Source", 
	                                BASTILE_TYPE_SOURCE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    
}

BastileOperation*
bastile_pkcs11_refresher_new (BastilePkcs11Source *source)
{
	return g_object_new (BASTILE_TYPE_PKCS11_REFRESHER, "source", source, NULL);
}


/* -----------------------------------------------------------------------------
 * DELETER OPERATION
 */

#define BASTILE_TYPE_PKCS11_DELETER               (bastile_pkcs11_deleter_get_type ())
#define BASTILE_PKCS11_DELETER(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_PKCS11_DELETER, BastilePkcs11Deleter))
#define BASTILE_PKCS11_DELETER_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_PKCS11_DELETER, BastilePkcs11DeleterClass))
#define BASTILE_IS_PKCS11_DELETER(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_PKCS11_DELETER))
#define BASTILE_IS_PKCS11_DELETER_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_PKCS11_DELETER))
#define BASTILE_PKCS11_DELETER_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_PKCS11_DELETER, BastilePkcs11DeleterClass))

typedef struct _BastilePkcs11Deleter BastilePkcs11Deleter;
typedef struct _BastilePkcs11DeleterClass BastilePkcs11DeleterClass;
    
struct _BastilePkcs11Deleter {
	BastileOperation parent;
	BastilePkcs11Object *object;
	GCancellable *cancellable;
};

struct _BastilePkcs11DeleterClass {
	BastileOperationClass parent_class;
};

enum {
	PROP_D0,
	PROP_OBJECT
};

G_DEFINE_TYPE (BastilePkcs11Deleter, bastile_pkcs11_deleter, BASTILE_TYPE_OPERATION);

static void 
on_deleted (GckObject *object, GAsyncResult *result, BastilePkcs11Deleter *self)
{
	GError *err = NULL;
	
	g_return_if_fail (BASTILE_IS_PKCS11_DELETER (self));
	
	if (!gck_object_destroy_finish (object, result, &err)) {

		/* Ignore objects that have gone away */
		if (err->code != CKR_OBJECT_HANDLE_INVALID) { 
			bastile_pkcs11_mark_complete (BASTILE_OPERATION (self), err);
			return;
		}
		
		g_error_free (err);
	}
	
	bastile_context_remove_object (NULL, BASTILE_OBJECT (self->object));
	bastile_pkcs11_mark_complete (BASTILE_OPERATION (self), NULL);
}
			
static void
bastile_pkcs11_deleter_cancel (BastileOperation *operation) 
{
	BastilePkcs11Deleter *self = BASTILE_PKCS11_DELETER (operation);
	g_return_if_fail (BASTILE_IS_PKCS11_DELETER (self));
	g_cancellable_cancel (self->cancellable);
}

static GObject* 
bastile_pkcs11_deleter_constructor (GType type, guint n_props, GObjectConstructParam *props) 
{
	BastilePkcs11Deleter *self = BASTILE_PKCS11_DELETER (G_OBJECT_CLASS (bastile_pkcs11_deleter_parent_class)->constructor(type, n_props, props));
	
	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (self->object, NULL);

	/* Start the delete */
	gck_object_destroy_async (bastile_pkcs11_object_get_pkcs11_object (self->object),
	                          self->cancellable, (GAsyncReadyCallback)on_deleted, self);
	bastile_operation_mark_start (BASTILE_OPERATION (self));
	
	return G_OBJECT (self);
}

static void
bastile_pkcs11_deleter_init (BastilePkcs11Deleter *self)
{
	self->cancellable = g_cancellable_new ();
}

static void
bastile_pkcs11_deleter_finalize (GObject *obj)
{
	BastilePkcs11Deleter *self = BASTILE_PKCS11_DELETER (obj);
	
	if (self->cancellable)
		g_object_unref (self->cancellable);
	self->cancellable = NULL;
	
	if (self->object)
		g_object_unref (self->object);
	self->object = NULL;

	G_OBJECT_CLASS (bastile_pkcs11_deleter_parent_class)->finalize (obj);
}

static void
bastile_pkcs11_deleter_set_property (GObject *obj, guint prop_id, const GValue *value, 
                                      GParamSpec *pspec)
{
	BastilePkcs11Deleter *self = BASTILE_PKCS11_DELETER (obj);
	
	switch (prop_id) {
	case PROP_OBJECT:
		g_return_if_fail (!self->object);
		self->object = g_value_get_object (value);
		g_return_if_fail (self->object);
		g_object_ref (self->object);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
bastile_pkcs11_deleter_get_property (GObject *obj, guint prop_id, GValue *value, 
                                      GParamSpec *pspec)
{
	BastilePkcs11Deleter *self = BASTILE_PKCS11_DELETER (obj);
	
	switch (prop_id) {
	case PROP_OBJECT:
		g_value_set_object (value, self->object);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
bastile_pkcs11_deleter_class_init (BastilePkcs11DeleterClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	BastileOperationClass *operation_class = BASTILE_OPERATION_CLASS (klass);
	
	bastile_pkcs11_deleter_parent_class = g_type_class_peek_parent (klass);

	gobject_class->constructor = bastile_pkcs11_deleter_constructor;
	gobject_class->finalize = bastile_pkcs11_deleter_finalize;
	gobject_class->set_property = bastile_pkcs11_deleter_set_property;
	gobject_class->get_property = bastile_pkcs11_deleter_get_property;
	
	operation_class->cancel = bastile_pkcs11_deleter_cancel;
	
	g_object_class_install_property (gobject_class, PROP_OBJECT,
	           g_param_spec_object ("object", "Object", "Deleting Object", 
	                                BASTILE_PKCS11_TYPE_OBJECT, 
	                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    
}

BastileOperation*
bastile_pkcs11_deleter_new (BastilePkcs11Object *object)
{
	return g_object_new (BASTILE_TYPE_PKCS11_DELETER, "object", object, NULL);
}
