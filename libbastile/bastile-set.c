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

#include "bastile-set.h"
#include "bastile-marshal.h"
#include "bastile-mateconf.h"

enum {
    PROP_0,
    PROP_PREDICATE
};

enum {
    ADDED,
    REMOVED,
    CHANGED,
    SET_CHANGED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

struct _BastileSetPrivate {
    GHashTable *objects;
    BastileObjectPredicate *pred;
};

G_DEFINE_TYPE (BastileSet, bastile_set, G_TYPE_OBJECT);

/* -----------------------------------------------------------------------------
 * INTERNAL 
 */

static gboolean
remove_update (BastileObject *sobj, gpointer closure, BastileSet *skset)
{
    if (closure == GINT_TO_POINTER (TRUE))
        closure = NULL;
    
    g_signal_emit (skset, signals[REMOVED], 0, sobj, closure);
    g_signal_emit (skset, signals[SET_CHANGED], 0);
    return TRUE;
}

static void
remove_object  (BastileObject *sobj, gpointer closure, BastileSet *skset)
{
    if (!closure)
        closure = g_hash_table_lookup (skset->pv->objects, sobj);
    
    g_hash_table_remove (skset->pv->objects, sobj);
    remove_update (sobj, closure, skset);
}

static gboolean
maybe_add_object (BastileSet *skset, BastileObject *sobj)
{
    if (bastile_object_get_context (sobj) != SCTX_APP ())
        return FALSE;

    if (g_hash_table_lookup (skset->pv->objects, sobj))
        return FALSE;
    
    if (!skset->pv->pred || !bastile_object_predicate_match (skset->pv->pred, sobj))
        return FALSE;
    
    g_hash_table_replace (skset->pv->objects, sobj, GINT_TO_POINTER (TRUE));
    g_signal_emit (skset, signals[ADDED], 0, sobj);
    g_signal_emit (skset, signals[SET_CHANGED], 0);
    return TRUE;
}

static gboolean
maybe_remove_object (BastileSet *skset, BastileObject *sobj)
{
    if (!g_hash_table_lookup (skset->pv->objects, sobj))
        return FALSE;
    
    if (skset->pv->pred && bastile_object_predicate_match (skset->pv->pred, sobj))
        return FALSE;
    
    remove_object (sobj, NULL, skset);
    return TRUE;
}

static void
object_added (BastileContext *sctx, BastileObject *sobj, BastileSet *skset)
{
    g_assert (BASTILE_IS_OBJECT (sobj));
    g_assert (BASTILE_IS_SET (skset));

    maybe_add_object (skset, sobj);
}

static void 
object_removed (BastileContext *sctx, BastileObject *sobj, BastileSet *skset)
{
    g_assert (BASTILE_IS_OBJECT (sobj));
    g_assert (BASTILE_IS_SET (skset));

    if (g_hash_table_lookup (skset->pv->objects, sobj))
        remove_object (sobj, NULL, skset);
}

static void
object_changed (BastileContext *sctx, BastileObject *sobj, BastileSet *skset)
{
    gpointer closure = g_hash_table_lookup (skset->pv->objects, sobj);

    g_assert (BASTILE_IS_OBJECT (sobj));
    g_assert (BASTILE_IS_SET (skset));

    if (closure) {
        
        /* See if needs to be removed, otherwise emit signal */
        if (!maybe_remove_object (skset, sobj)) {
            
            if (closure == GINT_TO_POINTER (TRUE))
                closure = NULL;
            
            g_signal_emit (skset, signals[CHANGED], 0, sobj, closure);
        }
        
    /* Not in our set yet */
    } else 
        maybe_add_object (skset, sobj);
}

static void
objects_to_list (BastileObject *sobj, gpointer *c, GList **l)
{
    *l = g_list_append (*l, sobj);
}

static void
objects_to_hash (BastileObject *sobj, gpointer *c, GHashTable *ht)
{
    g_hash_table_replace (ht, sobj, NULL);
}

/* -----------------------------------------------------------------------------
 * OBJECT 
 */

static void
bastile_set_dispose (GObject *gobject)
{
    BastileSet *skset = BASTILE_SET (gobject);
    
    /* Release all our pointers and stuff */
    g_hash_table_foreach_remove (skset->pv->objects, (GHRFunc)remove_update, skset);
    skset->pv->pred = NULL;
    
    g_signal_handlers_disconnect_by_func (SCTX_APP (), object_added, skset);    
    g_signal_handlers_disconnect_by_func (SCTX_APP (), object_removed, skset);    
    g_signal_handlers_disconnect_by_func (SCTX_APP (), object_changed, skset);    
    
	G_OBJECT_CLASS (bastile_set_parent_class)->dispose (gobject);
}

static void
bastile_set_finalize (GObject *gobject)
{
    BastileSet *skset = BASTILE_SET (gobject);

    g_hash_table_destroy (skset->pv->objects);
    g_assert (skset->pv->pred == NULL);
    g_free (skset->pv);
    
	G_OBJECT_CLASS (bastile_set_parent_class)->finalize (gobject);
}

static void
bastile_set_set_property (GObject *object, guint prop_id, const GValue *value, 
                              GParamSpec *pspec)
{
	BastileSet *skset = BASTILE_SET (object);
	
    switch (prop_id) {
    case PROP_PREDICATE:
        skset->pv->pred = (BastileObjectPredicate*)g_value_get_pointer (value);        
        bastile_set_refresh (skset);
        break;
    }
}

static void
bastile_set_get_property (GObject *object, guint prop_id, GValue *value, 
                              GParamSpec *pspec)
{
    BastileSet *skset = BASTILE_SET (object);
	
    switch (prop_id) {
    case PROP_PREDICATE:
        g_value_set_pointer (value, skset->pv->pred);
        break;
    }
}

static void
bastile_set_init (BastileSet *skset)
{
	/* init private vars */
	skset->pv = g_new0 (BastileSetPrivate, 1);

    skset->pv->objects = g_hash_table_new (g_direct_hash, g_direct_equal);
    
    g_signal_connect (SCTX_APP (), "added", G_CALLBACK (object_added), skset);
    g_signal_connect (SCTX_APP (), "removed", G_CALLBACK (object_removed), skset);
    g_signal_connect (SCTX_APP (), "changed", G_CALLBACK (object_changed), skset);
}

static void
bastile_set_class_init (BastileSetClass *klass)
{
    GObjectClass *gobject_class;
    
    bastile_set_parent_class = g_type_class_peek_parent (klass);
    gobject_class = G_OBJECT_CLASS (klass);
    
    gobject_class->dispose = bastile_set_dispose;
    gobject_class->finalize = bastile_set_finalize;
    gobject_class->set_property = bastile_set_set_property;
    gobject_class->get_property = bastile_set_get_property;
    
    g_object_class_install_property (gobject_class, PROP_PREDICATE,
        g_param_spec_pointer ("predicate", "Predicate", "Predicate for matching objects into this set.", 
                              G_PARAM_READWRITE));
    
    signals[ADDED] = g_signal_new ("added", BASTILE_TYPE_SET, 
                G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (BastileSetClass, added),
                NULL, NULL, g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, BASTILE_TYPE_OBJECT);
    
    signals[REMOVED] = g_signal_new ("removed", BASTILE_TYPE_SET, 
                G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (BastileSetClass, removed),
                NULL, NULL, bastile_marshal_VOID__OBJECT_POINTER, G_TYPE_NONE, 2, BASTILE_TYPE_OBJECT, G_TYPE_POINTER);    
    
    signals[CHANGED] = g_signal_new ("changed", BASTILE_TYPE_SET, 
                G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (BastileSetClass, changed),
                NULL, NULL, bastile_marshal_VOID__OBJECT_POINTER, G_TYPE_NONE, 2, BASTILE_TYPE_OBJECT, G_TYPE_POINTER);
                
    signals[SET_CHANGED] = g_signal_new ("set-changed", BASTILE_TYPE_SET,
                G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (BastileSetClass, set_changed),
                NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

/* -----------------------------------------------------------------------------
 * PUBLIC 
 */

BastileSet*
bastile_set_new (GQuark ktype, BastileUsage usage, BastileLocation location,
                     guint flags, guint nflags)
{
    BastileSet *skset;
    BastileObjectPredicate *pred = g_new0(BastileObjectPredicate, 1);
    
    pred->location = location;
    pred->id = ktype;
    pred->usage = usage;
    pred->flags = flags;
    pred->nflags = nflags;
    
    skset = bastile_set_new_full (pred);
    g_object_set_data_full (G_OBJECT (skset), "quick-predicate", pred, g_free);
    
    return skset;
}

BastileSet*     
bastile_set_new_full (BastileObjectPredicate *pred)
{
    return g_object_new (BASTILE_TYPE_SET, "predicate", pred, NULL);
}

gboolean
bastile_set_has_object (BastileSet *skset, BastileObject *sobj)
{
    if (g_hash_table_lookup (skset->pv->objects, sobj))
        return TRUE;
    
    /* 
     * This happens when the object has changed state, but we have 
     * not yet received the signal. 
     */
    if (maybe_add_object (skset, sobj))
        return TRUE;
    
    return FALSE;
}

GList*             
bastile_set_get_objects (BastileSet *skset)
{
    GList *objs = NULL;
    g_hash_table_foreach (skset->pv->objects, (GHFunc)objects_to_list, &objs);
    return objs;
}

guint
bastile_set_get_count (BastileSet *skset)
{
    return g_hash_table_size (skset->pv->objects);
}

void
bastile_set_refresh (BastileSet *skset)
{
    GHashTable *check = g_hash_table_new (g_direct_hash, g_direct_equal);
    GList *l, *objects = NULL;
    BastileObject *sobj;
    
    /* Make note of all the objects we had prior to refresh */
    g_hash_table_foreach (skset->pv->objects, (GHFunc)objects_to_hash, check);
    
    if (skset->pv->pred)
        objects = bastile_context_find_objects_full (SCTX_APP (), skset->pv->pred);
    
    for (l = objects; l; l = g_list_next (l)) {
        sobj = BASTILE_OBJECT (l->data);
        
        /* Make note that we've seen this object */
        g_hash_table_remove (check, sobj);

        /* This will add to set */
        maybe_add_object (skset, sobj);
    }
    
    g_list_free (objects);
    
    g_hash_table_foreach (check, (GHFunc)remove_object, skset);
    g_hash_table_destroy (check);
}

gpointer
bastile_set_get_closure (BastileSet *skset, BastileObject *sobj)
{
    gpointer closure = g_hash_table_lookup (skset->pv->objects, sobj);
    g_return_val_if_fail (closure != NULL, NULL);

    /* |TRUE| means no closure has been set */
    if (closure == GINT_TO_POINTER (TRUE))
        return NULL;
    
    return closure;
}

void
bastile_set_set_closure (BastileSet *skset, BastileObject *sobj, 
                             gpointer closure)
{
    /* Make sure we have the object */
    g_return_if_fail (g_hash_table_lookup (skset->pv->objects, sobj) != NULL);
    
    /* |TRUE| means no closure has been set */
    if (closure == NULL)
        closure = GINT_TO_POINTER (TRUE);

    g_hash_table_insert (skset->pv->objects, sobj, closure);    
}
