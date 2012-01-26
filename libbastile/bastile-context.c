/*
 * Bastile
 *
 * Copyright (C) 2003 Jacob Perkins
 * Copyright (C) 2005, 2006 Stefan Walter
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

#include "bastile-context.h"
#include "bastile-dns-sd.h"
#include "bastile-mateconf.h"
#include "bastile-libdialogs.h"
#include "bastile-marshal.h"
#include "bastile-servers.h"
#include "bastile-transfer-operation.h"
#include "bastile-unknown.h"
#include "bastile-unknown-source.h"
#include "bastile-util.h"

#include "common/bastile-bind.h"
#include "common/bastile-registry.h"

#ifdef WITH_PGP
#include "pgp/bastile-server-source.h"
#endif

/**
 * SECTION:bastile-context
 * @short_description: This is where all the action in a Bastile process comes together.
 * @include:libbastile/bastile-context.h
 *
 **/

/* The application main context */
BastileContext* app_context = NULL;

enum {
    ADDED,
    REMOVED,
    CHANGED,
    REFRESHING,
    DESTROY,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (BastileContext, bastile_context, G_TYPE_OBJECT);

/* 
 * Two hashtables are used to keep track of the objects:
 *
 *  objects_by_source: This contains a reference to the object and allows us to 
 *    lookup objects by their source. Each object/source combination should be 
 *    unique. Hashkeys are made with hashkey_by_source()
 *  objects_by_type: Each value contains a GList of objects with the same id
 *    (ie: same object from different sources). The objects are 
 *    orderred in by preferred usage. 
 */

struct _BastileContextPrivate {
    GSList *sources;                        /* Sources which add keys to this context */
    GHashTable *auto_sources;               /* Automatically added sources (keyservers) */
    GHashTable *objects_by_source;          /* See explanation above */
    GHashTable *objects_by_type;            /* See explanation above */
    guint notify_id;                        /* Notify for MateConf watch */
    BastileMultiOperation *refresh_ops;    /* Operations for refreshes going on */
    BastileServiceDiscovery *discovery;    /* Adds sources from DNS-SD */
    gboolean in_destruction;                /* In destroy signal */
};

static void bastile_context_dispose    (GObject *gobject);
static void bastile_context_finalize   (GObject *gobject);

/* Forward declarations */
static void refresh_keyservers          (MateConfClient *client, guint id, 
                                         MateConfEntry *entry, BastileContext *sctx);

/**
* klass: The class to initialise
*
* Inits the #BastileContextClass. Adds the signals "added", "removed", "changed"
* and "refreshing"
**/
static void
bastile_context_class_init (BastileContextClass *klass)
{
    GObjectClass *gobject_class;
    
    bastile_context_parent_class = g_type_class_peek_parent (klass);
    gobject_class = G_OBJECT_CLASS (klass);
    
    gobject_class->dispose = bastile_context_dispose;
    gobject_class->finalize = bastile_context_finalize;	
    
    signals[ADDED] = g_signal_new ("added", BASTILE_TYPE_CONTEXT, 
                G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (BastileContextClass, added),
                NULL, NULL, g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, BASTILE_TYPE_OBJECT);
    signals[REMOVED] = g_signal_new ("removed", BASTILE_TYPE_CONTEXT, 
                G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (BastileContextClass, removed),
                NULL, NULL, g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, BASTILE_TYPE_OBJECT);    
    signals[CHANGED] = g_signal_new ("changed", BASTILE_TYPE_CONTEXT, 
                G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (BastileContextClass, changed),
                NULL, NULL, g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, BASTILE_TYPE_OBJECT);    
    signals[REFRESHING] = g_signal_new ("refreshing", BASTILE_TYPE_CONTEXT, 
                G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (BastileContextClass, refreshing),
                NULL, NULL, g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, BASTILE_TYPE_OPERATION);    
    signals[DESTROY] = g_signal_new ("destroy", BASTILE_TYPE_CONTEXT,
                G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (BastileContextClass, destroy),
                NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

/* init context, private vars, set prefs, connect signals */
/**
* sctx: The context to initialise
*
* Initialises the Sahorse Context @sctx
*
**/
static void
bastile_context_init (BastileContext *sctx)
{
    /* init private vars */
    sctx->pv = g_new0 (BastileContextPrivate, 1);

    /* A list of sources */
    sctx->pv->sources = NULL;
    
    /* A table of objects */
    sctx->pv->objects_by_source = g_hash_table_new_full (g_direct_hash, g_direct_equal, 
                                                         NULL, g_object_unref);
    
    sctx->pv->objects_by_type = g_hash_table_new_full (g_direct_hash, g_direct_equal, 
                                                       NULL, NULL);
    
    /* The context is explicitly destroyed */
    g_object_ref (sctx);
}

/**
* object: ignored
* value: the value to prepend
* user_data: GSList ** to prepend to
*
* Prepends #value to #user_data
*
**/
static void
hash_to_ref_slist (gpointer object, gpointer value, gpointer user_data)
{
	*((GSList**)user_data) = g_slist_prepend (*((GSList**)user_data), g_object_ref (value));
}


/**
* gobject: the object to dispose (#BastileContext)
*
* release all references
*
**/
static void
bastile_context_dispose (GObject *gobject)
{
    BastileContext *sctx;
    GSList *objects, *l;
    
    sctx = BASTILE_CONTEXT (gobject);
    
    /* Release all the objects */
    objects = NULL;
    g_hash_table_foreach (sctx->pv->objects_by_source, hash_to_ref_slist, &objects);
    for (l = objects; l; l = g_slist_next (l)) {
            bastile_context_remove_object (sctx, l->data);
            g_object_unref (G_OBJECT (l->data));
    }
    g_slist_free (objects);

    /* Gconf notification */
    if (sctx->pv->notify_id)
        bastile_mateconf_unnotify (sctx->pv->notify_id);
    sctx->pv->notify_id = 0;
    
    /* Auto sources */
    if (sctx->pv->auto_sources) 
        g_hash_table_destroy (sctx->pv->auto_sources);
    sctx->pv->auto_sources = NULL;
        
    /* DNS-SD */    
    if (sctx->pv->discovery) 
        g_object_unref (sctx->pv->discovery);
    sctx->pv->discovery = NULL;
        
    /* Release all the sources */
    for (l = sctx->pv->sources; l; l = g_slist_next (l))
        g_object_unref (BASTILE_SOURCE (l->data));
    g_slist_free (sctx->pv->sources);
    sctx->pv->sources = NULL;
    
    if (sctx->pv->refresh_ops)
	    g_object_unref (sctx->pv->refresh_ops);
    sctx->pv->refresh_ops = NULL;

	if (!sctx->pv->in_destruction) {
		sctx->pv->in_destruction = TRUE;
		g_signal_emit (sctx, signals[DESTROY], 0);
		sctx->pv->in_destruction = FALSE;
	}

	G_OBJECT_CLASS (bastile_context_parent_class)->dispose (gobject);
}


/**
* gobject: The #BastileContext to finalize
*
* destroy all objects, free private vars
*
**/
static void
bastile_context_finalize (GObject *gobject)
{
    BastileContext *sctx = BASTILE_CONTEXT (gobject);
    
    /* Destroy the hash table */        
    if (sctx->pv->objects_by_source)
        g_hash_table_destroy (sctx->pv->objects_by_source);
    if (sctx->pv->objects_by_type)
        g_hash_table_destroy (sctx->pv->objects_by_type);
    
    /* Other stuff already done in dispose */
    g_assert (sctx->pv->sources == NULL);
    g_assert (sctx->pv->auto_sources == NULL);
    g_assert (sctx->pv->discovery == NULL);
    g_assert (sctx->pv->refresh_ops == NULL);
    g_free (sctx->pv);
    
    G_OBJECT_CLASS (bastile_context_parent_class)->finalize (gobject);
}

/**
* bastile_context_for_app:
*
* Returns: the application main context as #BastileContext
*/
BastileContext*
bastile_context_for_app (void)
{
    g_return_val_if_fail (app_context != NULL, NULL);
    return app_context;
}
   
/**
 * bastile_context_new:
 * @flags: Flags define the type of the context to create.
 *
 * Creates a new #BastileContext.
 * Flags:
 * BASTILE_CONTEXT_DAEMON: internal daemon flag will be set
 * BASTILE_CONTEXT_APP: will support DNS-SD discovery and remote key sources
 *
 * Returns: The new context
 */
BastileContext*
bastile_context_new (guint flags)
{
	BastileContext *sctx = g_object_new (BASTILE_TYPE_CONTEXT, NULL);
    
    	if (flags & BASTILE_CONTEXT_DAEMON)
	    sctx->is_daemon = TRUE;
    
	if (flags & BASTILE_CONTEXT_APP) {

		/* DNS-SD discovery */    
		sctx->pv->discovery = bastile_service_discovery_new ();
        
		/* Automatically added remote key sources */
		sctx->pv->auto_sources = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                        g_free, NULL);

		/* Listen for new mateconf remote key sources automatically */
		sctx->pv->notify_id = bastile_mateconf_notify (KEYSERVER_KEY, 
                                    (MateConfClientNotifyFunc)refresh_keyservers, sctx);
        
		if (app_context)
			g_object_unref (app_context);
        
		g_object_ref (sctx);
		g_object_ref_sink (sctx);
		g_object_unref (sctx);
		app_context = sctx;
        
		refresh_keyservers (NULL, 0, NULL, sctx);
	}
    
	return sctx;
}

/**
 * bastile_context_destroy:
 * @sctx: #BastileContext to destroy
 *
 * Emits the destroy signal for @sctx.
 **/
void
bastile_context_destroy (BastileContext *sctx)
{
	g_return_if_fail (BASTILE_IS_CONTEXT (sctx));
	g_object_run_dispose (G_OBJECT (sctx));
	if (sctx == app_context)
		app_context = NULL;
}

/**
* sctx: #BastileContext to add the source to
* sksrc: #BastileSource to add
*
* Adds the source to the context
*
* Returns TRUE if the source was added, FALSE if it was already there
**/
static gboolean                
take_source (BastileContext *sctx, BastileSource *sksrc)
{
	g_return_val_if_fail (BASTILE_IS_SOURCE (sksrc), FALSE);
	if (!g_slist_find (sctx->pv->sources, sksrc)) {
		sctx->pv->sources = g_slist_append (sctx->pv->sources, sksrc);
		return TRUE;
	}
	
	return FALSE;
}

/**
 * bastile_context_take_source:
 * @sctx: A context to add a source to, can be NULL
 * @sksrc: The source to add
 *
 * Adds @sksrc to the @sctx. If @sctx is NULL it will use the application context.
 *
 */
void                
bastile_context_take_source (BastileContext *sctx, BastileSource *sksrc)
{
	g_return_if_fail (BASTILE_IS_SOURCE (sksrc));
    
	if (!sctx)
		sctx = bastile_context_for_app ();
	g_return_if_fail (BASTILE_IS_CONTEXT (sctx));

	take_source (sctx, sksrc);
}

/**
 * bastile_context_add_source:
 * @sctx: A context to add a source to, can be NULL
 * @sksrc: The source to add
 *
 * Adds @sksrc to the @sctx. If @sctx is NULL it will use the application context.
 * It also adds a reference to the new added source.
 */
void
bastile_context_add_source (BastileContext *sctx, BastileSource *sksrc)
{
	g_return_if_fail (BASTILE_IS_SOURCE (sksrc));
    
	if (!sctx)
		sctx = bastile_context_for_app ();
	g_return_if_fail (BASTILE_IS_CONTEXT (sctx));

	if (take_source (sctx, sksrc))
		g_object_ref (sksrc);
}
    
/**
 * bastile_context_remove_source:
 * @sctx: Context to remove objects from
 * @sksrc: The source to remove
 *
 * Remove all objects from source @sksrc from the #BastileContext @sctx
 *
 */
void
bastile_context_remove_source (BastileContext *sctx, BastileSource *sksrc)
{
    GList *l, *objects;
    
    g_return_if_fail (BASTILE_IS_SOURCE (sksrc));

    if (!sctx)
        sctx = bastile_context_for_app ();
    g_return_if_fail (BASTILE_IS_CONTEXT (sctx));

    if (!g_slist_find (sctx->pv->sources, sksrc)) 
        return;

    /* Remove all objects from this source */    
    objects = bastile_context_get_objects (sctx, sksrc);
    for (l = objects; l; l = g_list_next (l)) 
        bastile_context_remove_object (sctx, BASTILE_OBJECT (l->data));
    
    /* Remove the source itself */
    sctx->pv->sources = g_slist_remove (sctx->pv->sources, sksrc);
    g_object_unref (sksrc);
}

/**
 * bastile_context_find_source:
 * @sctx: A #BastileContext
 * @ktype: A bastile tag (BASTILE_TAG_INVALID is wildcard)
 * @location: A location (BASTILE_LOCATION_INVALID is wildcard)
 *
 * Finds a context where @ktype and @location match
 *
 * Returns: The context
 */
BastileSource*  
bastile_context_find_source (BastileContext *sctx, GQuark ktype,
                              BastileLocation location)
{
    BastileSource *ks;
    GSList *l;
    
    if (!sctx)
        sctx = bastile_context_for_app ();
    g_return_val_if_fail (BASTILE_IS_CONTEXT (sctx), NULL);

    for (l = sctx->pv->sources; l; l = g_slist_next (l)) {
        ks = BASTILE_SOURCE (l->data);
        
        if (ktype != BASTILE_TAG_INVALID && 
            bastile_source_get_tag (ks) != ktype)
            continue;
        
        if (location != BASTILE_LOCATION_INVALID && 
            bastile_source_get_location (ks) != location)
            continue;
        
        return ks;
    }
    
    /* If we don't have an unknown source for this type, create it */
    if (location == BASTILE_LOCATION_MISSING && location != BASTILE_TAG_INVALID) {
        ks = BASTILE_SOURCE (bastile_unknown_source_new (ktype));
        bastile_context_add_source (sctx, ks);
        return ks;
    }
    
    return NULL;
}


/**
* bastile_context_find_sources:
* @sctx: the context to work with
* @ktype: the type of the key to match. Or BASTILE_TAG_INVALID
* @location: the location to match. Or BASTILE_LOCATION_INVALID
*
* Returns: A list of bastile sources matching @ktype and @location as #GSList. Must
*  be freed with #g_slist_free
*/
GSList*
bastile_context_find_sources (BastileContext *sctx, GQuark ktype,
                               BastileLocation location)
{
    BastileSource *ks;
    GSList *sources = NULL;
    GSList *l;
    
    if (!sctx)
        sctx = bastile_context_for_app ();
    g_return_val_if_fail (BASTILE_IS_CONTEXT (sctx), NULL);

    for (l = sctx->pv->sources; l; l = g_slist_next (l)) {
        ks = BASTILE_SOURCE (l->data);
        
        if (ktype != BASTILE_TAG_INVALID && 
            bastile_source_get_tag (ks) != ktype)
            continue;
        
        if (location != BASTILE_LOCATION_INVALID && 
            bastile_source_get_location (ks) != location)
            continue;
        
        sources = g_slist_append (sources, ks);
    }
    
    return sources;
}

/**
 * bastile_context_remote_source:
 * @sctx: the context to add the source to (can be NULL)
 * @uri: An URI to add as remote source
 *
 * Add a remote source to the Context @sctx. If it already exists, the source
 * object will be returned.
 *
 * Returns: The #BastileSource with this URI
 */
BastileSource*  
bastile_context_remote_source (BastileContext *sctx, const gchar *uri)
{
    BastileSource *ks = NULL;
    gboolean found = FALSE;
    gchar *ks_uri;
    GSList *l;
    
    g_return_val_if_fail (uri && *uri, NULL);

    if (!sctx)
        sctx = bastile_context_for_app ();
    g_return_val_if_fail (BASTILE_IS_CONTEXT (sctx), NULL);

    for (l = sctx->pv->sources; l; l = g_slist_next (l)) {
        ks = BASTILE_SOURCE (l->data);
        
        if (bastile_source_get_location (ks) != BASTILE_LOCATION_REMOTE)
            continue;
        
        g_object_get (ks, "uri", &ks_uri, NULL);
        if (ks_uri && g_str_equal (ks_uri, uri)) 
            found = TRUE;
        g_free (ks_uri);
        
        if (found)
            return ks;
    }
    
    /* TODO: We need to decouple this properly */
#ifdef WITH_KEYSERVER
#ifdef WITH_PGP
    /* Auto generate one if possible */
    if (sctx->pv->auto_sources) {
        ks = BASTILE_SOURCE (bastile_server_source_new (uri));
        if (ks != NULL) {
            bastile_context_take_source (sctx, ks);
            g_hash_table_replace (sctx->pv->auto_sources, g_strdup (uri), ks);
        }
    }
#endif /* WITH_PGP */
#endif /* WITH_KEYSERVER */	
    
    return ks;
}


/**
* sobj: BastileObject, sending the signal
* spec: ignored
* sctx: The instance to emit the signal on (#BastileContext)
*
* Emits a changed signal.
*
**/
static void
object_notify (BastileObject *sobj, GParamSpec *spec, BastileContext *sctx)
{
	g_signal_emit (sctx, signals[CHANGED], 0, sobj);
}

/**
* sksrc: a #BastileSource, part of the hash
* id: an id, part of the hash
*
* Creates a hash out of @sksrc and @id
*
* Returns an int stored in a pointer, representing the hash
**/
static gpointer                 
hashkey_by_source (BastileSource *sksrc, GQuark id)

{
    return GINT_TO_POINTER (g_direct_hash (sksrc) ^ 
                            g_str_hash (g_quark_to_string (id)));
}

/**
* a: the first #BastileObject
* b: the second #BastileObject
*
* Compares the locations of the two objects
*
* Returns 0 if a==b, -1 or 1 on difference
**/
static gint
sort_by_location (gconstpointer a, gconstpointer b)
{
    guint aloc, bloc;
    
    g_assert (BASTILE_IS_OBJECT (a));
    g_assert (BASTILE_IS_OBJECT (b));
    
    aloc = bastile_object_get_location (BASTILE_OBJECT (a));
    bloc = bastile_object_get_location (BASTILE_OBJECT (b));
    
    if (aloc == bloc)
        return 0;
    return aloc > bloc ? -1 : 1;
}

/**
* sctx: The bastile context to manipulate
* sobj: The object to add/remove
* add: TRUE if the object should be added
*
* Either adds or removes an object from sctx->pv->objects_by_type.
* The new list will be sorted by location.
*
**/
static void
setup_objects_by_type (BastileContext *sctx, BastileObject *sobj, gboolean add)
{
    GList *l, *objects = NULL;
    BastileObject *aobj, *next;
    gpointer kt = GUINT_TO_POINTER (bastile_object_get_id (sobj));
    gboolean first;
    
    /* Get current set of objects in this tag/id */
    if (add)
        objects = g_list_prepend (objects, sobj);
    
    for (aobj = g_hash_table_lookup (sctx->pv->objects_by_type, kt); 
         aobj; aobj = bastile_object_get_preferred (aobj))
    {
        if (aobj != sobj)
            objects = g_list_prepend (objects, aobj);
    }
    
    /* No objects just remove */
    if (!objects) {
        g_hash_table_remove (sctx->pv->objects_by_type, kt);
        return;
    }

    /* Sort and add back */
    objects = g_list_sort (objects, sort_by_location);
    for (l = objects, first = TRUE; l; l = g_list_next (l), first = FALSE) {
        
        aobj = BASTILE_OBJECT (l->data);
        
        /* Set first as start of list */
        if (first)
            g_hash_table_replace (sctx->pv->objects_by_type, kt, aobj);
            
        /* Set next one as preferred */
        else {
            next = g_list_next (l) ? BASTILE_OBJECT (g_list_next (l)->data) : NULL;
            bastile_object_set_preferred (aobj, next);
        }
    }
    
    g_list_free (objects);
}

/**
 * bastile_context_add_object:
 * @sctx: The context to add the object to
 * @sobj: The object to add
 *
 * Adds @sobj to @sctx. References @sobj
 *
 */
void
bastile_context_add_object (BastileContext *sctx, BastileObject *sobj)
{
    if (!sctx)
        sctx = bastile_context_for_app ();
    g_return_if_fail (BASTILE_IS_CONTEXT (sctx));

    g_object_ref (sobj);
    bastile_context_take_object (sctx, sobj);
}

/**
 * bastile_context_take_object:
 * @sctx: The #BastileContext context to add an object to
 * @sobj: The #BastileObject object to add
 *
 * Adds @sobj to @sctx. If a similar object exists, it will be overwritten.
 * Emits the "added" signal.
 */
void
bastile_context_take_object (BastileContext *sctx, BastileObject *sobj)
{
    gpointer ks;
    
    if (!sctx)
        sctx = bastile_context_for_app ();
    g_return_if_fail (BASTILE_IS_CONTEXT (sctx));
    g_return_if_fail (BASTILE_IS_OBJECT (sobj));
    g_return_if_fail (bastile_object_get_id (sobj) != 0);
    
    ks = hashkey_by_source (bastile_object_get_source (sobj), 
                            bastile_object_get_id (sobj));
    
    g_return_if_fail (!g_hash_table_lookup (sctx->pv->objects_by_source, ks));

    g_object_ref (sobj);

    g_object_set (sobj, "context", sctx, NULL);
    g_hash_table_replace (sctx->pv->objects_by_source, ks, sobj);
    setup_objects_by_type (sctx, sobj, TRUE);
    g_signal_emit (sctx, signals[ADDED], 0, sobj);
    g_object_unref (sobj);
    
    g_signal_connect (sobj, "notify", G_CALLBACK (object_notify), sctx);
}

/**
 * bastile_context_get_count:
 * @sctx: The context. If NULL is passed it will take the application context
 *
 *
 *
 * Returns: The number of objects in this context
 */
guint
bastile_context_get_count (BastileContext *sctx)
{
    if (!sctx)
        sctx = bastile_context_for_app ();
    g_return_val_if_fail (BASTILE_IS_CONTEXT (sctx), 0);
    return g_hash_table_size (sctx->pv->objects_by_source);
}

/**
 * bastile_context_get_object:
 * @sctx: The #BastileContext to look in
 * @sksrc: The source to match
 * @id: the id to match
 *
 * Finds the object with the source @sksrc and @id in the context and returns it
 *
 * Returns: The matching object
 */
BastileObject*        
bastile_context_get_object (BastileContext *sctx, BastileSource *sksrc,
                             GQuark id)
{
    gconstpointer k;
    
    if (!sctx)
        sctx = bastile_context_for_app ();
    g_return_val_if_fail (BASTILE_IS_CONTEXT (sctx), NULL);
    g_return_val_if_fail (BASTILE_IS_SOURCE (sksrc), NULL);
    
    k = hashkey_by_source (sksrc, id);
    return BASTILE_OBJECT (g_hash_table_lookup (sctx->pv->objects_by_source, k));
}

/**
 * ObjectMatcher:
 * @kp: A #BastileObjectPredicate to compare an object to
 * @many: TRUE if there can be several matches
 * @func: A function to call for every match
 * @user_data: user data to pass to the function
 *
 *
 **/
typedef struct _ObjectMatcher {
	BastileObjectPredicate *kp;
	gboolean many;
	BastileObjectFunc func;
	gpointer user_data;
} ObjectMatcher;

/**
 * find_matching_objects:
 * @key: ignored
 * @sobj: the object to match
 * @km: an #ObjectMatcher
 *
 * Calls km->func for every matched object
 *
 * Returns: TRUE if the search terminates, FALSE if there could be another
 * match or nothing was found
 */
gboolean
find_matching_objects (gpointer key, BastileObject *sobj, ObjectMatcher *km)
{
	gboolean matched;
	
	if (km->kp && bastile_object_predicate_match (km->kp, BASTILE_OBJECT (sobj))) {
		matched = TRUE;
		(km->func) (sobj, km->user_data);
	}

	/* Terminate search */
	if (!km->many && matched)
		return TRUE;

	/* Keep going */
	return FALSE;
}

/**
* object: A #BastileObject to add to the list
* user_data: a #GList to add the object to
*
* Does not return anything....
*
**/
static void
add_object_to_list (BastileObject *object, gpointer user_data)
{
	GList** list = (GList**)user_data;
	*list = g_list_prepend (*list, object);
}

/**
 * bastile_context_find_objects_full:
 * @self: The #BastileContext to match objects in
 * @pred: #BastileObjectPredicate containing some data to match
 *
 * Finds matching objects and adds them to the list
 *
 * Returns: a #GList list containing the matching objects
 */
GList*
bastile_context_find_objects_full (BastileContext *self, BastileObjectPredicate *pred)
{
	GList *list = NULL;
	ObjectMatcher km;

	if (!self)
		self = bastile_context_for_app ();
	g_return_val_if_fail (BASTILE_IS_CONTEXT (self), NULL);
	g_return_val_if_fail (pred, NULL);

	memset (&km, 0, sizeof (km));
	km.kp = pred;
	km.many = TRUE;
	km.func = add_object_to_list;
	km.user_data = &list;

	g_hash_table_find (self->pv->objects_by_source, (GHRFunc)find_matching_objects, &km);
	return list; 
}

/**
 * bastile_context_for_objects_full:
 * @self: #BastileContext to work with
 * @pred: #BastileObjectPredicate to match
 * @func: Function to call for matching objects
 * @user_data: Data to pass to this function
 *
 * Calls @func for every object in @self matching the criteria in @pred. @user_data
 * is passed to this function
 */
void
bastile_context_for_objects_full (BastileContext *self, BastileObjectPredicate *pred,
                                   BastileObjectFunc func, gpointer user_data)
{
	ObjectMatcher km;

	if (!self)
		self = bastile_context_for_app ();
	g_return_if_fail (BASTILE_IS_CONTEXT (self));
	g_return_if_fail (pred);
	g_return_if_fail (func);

	memset (&km, 0, sizeof (km));
	km.kp = pred;
	km.many = TRUE;
	km.func = func;
	km.user_data = user_data;

	g_hash_table_find (self->pv->objects_by_source, (GHRFunc)find_matching_objects, &km);
}

/**
 * bastile_context_get_objects:
 * @self: A #BastileContext to find objects in
 * @source: A #BastileSource that must match
 *
 *
 *
 * Returns: A #GList of objects from @self that match the source @source
 */
GList*
bastile_context_get_objects (BastileContext *self, BastileSource *source)
{
	BastileObjectPredicate pred;

	if (!self)
		self = bastile_context_for_app ();
	g_return_val_if_fail (BASTILE_IS_CONTEXT (self), NULL);
	g_return_val_if_fail (source == NULL || BASTILE_IS_SOURCE (source), NULL);

	bastile_object_predicate_clear (&pred);
	pred.source = source;
	
	return bastile_context_find_objects_full (self, &pred);
}

/**
 * bastile_context_find_object:
 * @sctx: The #BastileContext to work with (can be NULL)
 * @id: The id to look for
 * @location: The location to look for (at least)
 *
 * Finds the object with the id @id at location @location or better.
 * Local is better than remote...
 *
 * Returns: the matching #BastileObject or NULL if none is found
 */
BastileObject*        
bastile_context_find_object (BastileContext *sctx, GQuark id, BastileLocation location)
{
    BastileObject *sobj; 

    if (!sctx)
        sctx = bastile_context_for_app ();
    g_return_val_if_fail (BASTILE_IS_CONTEXT (sctx), NULL);

    sobj = (BastileObject*)g_hash_table_lookup (sctx->pv->objects_by_type, GUINT_TO_POINTER (id));
    while (sobj) {
        
        /* If at the end and no more objects in list, return */
        if (location == BASTILE_LOCATION_INVALID && !bastile_object_get_preferred (sobj))
            return sobj;
        
        if (location >= bastile_object_get_location (sobj))
            return sobj;
        
        /* Look down the list for this location */
        sobj = bastile_object_get_preferred (sobj);
    }
    
    return NULL;
}


/**
 * bastile_context_find_objects:
 * @sctx: A #BastileContext to look in (can be NULL)
 * @ktype: The tag to look for
 * @usage: the usage (#BastileUsage)
 * @location: the location to look for
 *
 *
 *
 * Returns: A list of matching objects
 */
GList*
bastile_context_find_objects (BastileContext *sctx, GQuark ktype, 
                               BastileUsage usage, BastileLocation location)
{
    BastileObjectPredicate pred;
    memset (&pred, 0, sizeof (pred));

    if (!sctx)
        sctx = bastile_context_for_app ();
    g_return_val_if_fail (BASTILE_IS_CONTEXT (sctx), NULL);
    
    pred.tag = ktype;
    pred.usage = usage;
    pred.location = location;
    
    return bastile_context_find_objects_full (sctx, &pred);
}

/**
* key: a pointer to a key to verify (hashkey)
* value: a bastile object
* user_data: ignored
*
* Asserts that the @key is the same as the one stored in @value
*
**/
static void
verify_each_object (gpointer key, gpointer value, gpointer user_data)
{
	gpointer k;

	g_assert (BASTILE_IS_OBJECT (value));
	k = hashkey_by_source (bastile_object_get_source (value), 
	                       bastile_object_get_id (value));
	g_assert (k == key);
}

/**
 * bastile_context_verify_objects:
 * @self: A #BastileContext to verify
 *
 * Verifies each key in the given context.
 * An assertion handles failure.
 */
void
bastile_context_verify_objects (BastileContext *self)
{
	if (!self)
		self = bastile_context_for_app ();
	g_return_if_fail (BASTILE_IS_CONTEXT (self));
	g_hash_table_foreach (self->pv->objects_by_source, verify_each_object, self);
}

/**
 * bastile_context_remove_object:
 * @sctx: The #BastileContext (can be NULL)
 * @sobj: The #BastileObject to remove
 *
 * Removes the object from the context
 *
 */
void 
bastile_context_remove_object (BastileContext *sctx, BastileObject *sobj)
{
    gconstpointer k;
    
    if (!sctx)
        sctx = bastile_context_for_app ();
    g_return_if_fail (BASTILE_IS_CONTEXT (sctx));
    g_return_if_fail (BASTILE_IS_OBJECT (sobj));
    g_return_if_fail (bastile_object_get_id (sobj) != 0);
    
    k = hashkey_by_source (bastile_object_get_source (sobj), 
                           bastile_object_get_id (sobj));
    
    if (g_hash_table_lookup (sctx->pv->objects_by_source, k)) {
        g_return_if_fail (bastile_object_get_context (sobj) == sctx);

        g_object_ref (sobj);
        g_signal_handlers_disconnect_by_func (sobj, object_notify, sctx);
        g_object_set (sobj, "context", NULL, NULL);
        g_hash_table_remove (sctx->pv->objects_by_source, k);
        setup_objects_by_type (sctx, sobj, FALSE);
        g_signal_emit (sctx, signals[REMOVED], 0, sobj);    
        g_object_unref (sobj);
    }
}


/* -----------------------------------------------------------------------------
 * DEPRECATED 
 */

/**
 * bastile_context_get_default_key:
 * @sctx: Current #BastileContext
 *
 * Returns: the secret key that's the default key
 *
 * Deprecated: No replacement
 */
BastileObject*
bastile_context_get_default_key (BastileContext *sctx)
{
    BastileObject *sobj = NULL;
    gchar *id;
    
    if (!sctx)
        sctx = bastile_context_for_app ();
    g_return_val_if_fail (BASTILE_IS_CONTEXT (sctx), NULL);

    /* TODO: All of this needs to take multiple key types into account */
    
    id = bastile_mateconf_get_string (BASTILE_DEFAULT_KEY);
    if (id != NULL && id[0]) {
        GQuark keyid = g_quark_from_string (id);
        sobj = bastile_context_find_object (sctx, keyid, BASTILE_LOCATION_LOCAL);
    }
    
    g_free (id);
    
    return sobj;
}

/**
 * bastile_context_get_discovery:
 * @sctx: #BastileContext object
 *
 * Gets the Service Discovery object for this context.
 *
 * Returns: The Service Discovery object.
 *
 * Deprecated: No replacement
 */
BastileServiceDiscovery*
bastile_context_get_discovery (BastileContext *sctx)
{
    if (!sctx)
        sctx = bastile_context_for_app ();
    g_return_val_if_fail (BASTILE_IS_CONTEXT (sctx), NULL);
    g_return_val_if_fail (sctx->pv->discovery != NULL, NULL);
    
    return sctx->pv->discovery;
}

/**
 * bastile_context_refresh_auto:
 * @sctx: A #BastileContext (can be NULL)
 *
 * Starts a new refresh operation and emits the "refreshing" signal
 *
 */
void
bastile_context_refresh_auto (BastileContext *sctx)
{
	BastileSource *ks;
	BastileOperation *op = NULL;
	GSList *l;
    
	if (!sctx)
		sctx = bastile_context_for_app ();
	g_return_if_fail (BASTILE_IS_CONTEXT (sctx));
	
	if (!sctx->pv->refresh_ops)
		sctx->pv->refresh_ops = bastile_multi_operation_new ();

	for (l = sctx->pv->sources; l; l = g_slist_next (l)) {
		ks = BASTILE_SOURCE (l->data);
        
		if (bastile_source_get_location (ks) == BASTILE_LOCATION_LOCAL) {

			op = bastile_source_load (ks);
			g_return_if_fail (op);
			bastile_multi_operation_take (sctx->pv->refresh_ops, op);
		}
		
	}
	
	g_signal_emit (sctx, signals[REFRESHING], 0, sctx->pv->refresh_ops);
}

/**
 * bastile_context_search_remote:
 * @sctx: A #BastileContext (can be NULL)
 * @search: a keyword (name, email address...) to search for
 *
 * Searches for the key matching @search o the remote servers
 *
 * Returns: The created search operation
 */
BastileOperation*  
bastile_context_search_remote (BastileContext *sctx, const gchar *search)
{
    BastileSource *ks;
    BastileMultiOperation *mop = NULL;
    BastileOperation *op = NULL;
    GSList *l, *names;
    GHashTable *servers = NULL;
    gchar *uri;
    
    if (!sctx)
        sctx = bastile_context_for_app ();
    g_return_val_if_fail (BASTILE_IS_CONTEXT (sctx), NULL);
    
    /* Get a list of all selected key servers */
    names = bastile_mateconf_get_string_list (LASTSERVERS_KEY);
    if (names) {
        servers = g_hash_table_new (g_str_hash, g_str_equal);
        for (l = names; l; l = g_slist_next (l)) 
            g_hash_table_insert (servers, l->data, GINT_TO_POINTER (TRUE));
    }
        
    for (l = sctx->pv->sources; l; l = g_slist_next (l)) {
        ks = BASTILE_SOURCE (l->data);
        
        if (bastile_source_get_location (ks) != BASTILE_LOCATION_REMOTE)
            continue;

        if (servers) {
            g_object_get (ks, "uri", &uri, NULL);
            if (!g_hash_table_lookup (servers, uri)) {
                g_free (uri);
                continue;
            }
            
            g_free (uri);
        }

        if (mop == NULL && op != NULL) {
            mop = bastile_multi_operation_new ();
            bastile_multi_operation_take (mop, op);
        }
            
        op = bastile_source_search (ks, search);
            
        if (mop != NULL)
            bastile_multi_operation_take (mop, op);
    }   

    bastile_util_string_slist_free (names);
    return mop ? BASTILE_OPERATION (mop) : op;  
}

#ifdef WITH_KEYSERVER
#ifdef WITH_PGP
/* For copying the keys */
/**
* uri: the uri of the source
* sksrc: the source to add or replace
* ht: the hash table to modify
*
* Adds the @sksrc to the hash table @ht
*
**/
static void 
auto_source_to_hash (const gchar *uri, BastileSource *sksrc, GHashTable *ht)

{
    g_hash_table_replace (ht, (gpointer)uri, sksrc);
}

/**
* uri: The uri of this source
* sksrc: The source to remove
* sctx: The Context to remove data from
*
*
*
**/
static void
auto_source_remove (const gchar* uri, BastileSource *sksrc, BastileContext *sctx)
{
    bastile_context_remove_source (sctx, sksrc);
    g_hash_table_remove (sctx->pv->auto_sources, uri);
}
#endif 
#endif

/**
* client: ignored
* id: ignored
* entry: used for validation only
* sctx: The context to work with
*
* Refreshes the sources generated from the keyservers
*
**/
static void
refresh_keyservers (MateConfClient *client, guint id, MateConfEntry *entry, 
                    BastileContext *sctx)
{
#ifdef WITH_KEYSERVER
#ifdef WITH_PGP
    BastileServerSource *ssrc;
    GSList *keyservers, *l;
    GHashTable *check;
    const gchar *uri;
    
    if (!sctx->pv->auto_sources)
        return;
    
    if (entry && !g_str_equal (KEYSERVER_KEY, mateconf_entry_get_key (entry)))
        return;

    /* Make a light copy of the auto_source table */    
    check = g_hash_table_new (g_str_hash, g_str_equal);
    g_hash_table_foreach (sctx->pv->auto_sources, (GHFunc)auto_source_to_hash, check);

    
    /* Load and strip names from keyserver list */
    keyservers = bastile_servers_get_uris ();
    
    for (l = keyservers; l; l = g_slist_next (l)) {
        uri = (const gchar*)(l->data);
        
        /* If we don't have a keysource then add it */
        if (!g_hash_table_lookup (sctx->pv->auto_sources, uri)) {
            ssrc = bastile_server_source_new (uri);
            if (ssrc != NULL) {
                bastile_context_take_source (sctx, BASTILE_SOURCE (ssrc));
                g_hash_table_replace (sctx->pv->auto_sources, g_strdup (uri), ssrc);
            }
        }
        
        /* Mark this one as present */
        g_hash_table_remove (check, uri);
    }
    
    /* Now remove any extras */
    g_hash_table_foreach (check, (GHFunc)auto_source_remove, sctx);
    
    g_hash_table_destroy (check);
    bastile_util_string_slist_free (keyservers);
#endif /* WITH_PGP */
#endif /* WITH_KEYSERVER */
}

/**
 * bastile_context_transfer_objects:
 * @sctx: The #BastileContext (can be NULL)
 * @objects: the objects to import
 * @to: a source to import to (can be NULL)
 *
 *
 *
 * Returns: A transfer operation
 */
BastileOperation*
bastile_context_transfer_objects (BastileContext *sctx, GList *objects, 
                                   BastileSource *to)
{
    BastileSource *from;
    BastileOperation *op = NULL;
    BastileMultiOperation *mop = NULL;
    BastileObject *sobj;
    GSList *ids = NULL;
    GList *next, *l;
    GQuark ktype;

    if (!sctx)
        sctx = bastile_context_for_app ();
    g_return_val_if_fail (BASTILE_IS_CONTEXT (sctx), NULL);

    objects = g_list_copy (objects);
    
    /* Sort by key source */
    objects = bastile_util_objects_sort (objects);
    
    while (objects) {
        
        /* break off one set (same keysource) */
        next = bastile_util_objects_splice (objects);

        g_assert (BASTILE_IS_OBJECT (objects->data));
        sobj = BASTILE_OBJECT (objects->data);

        /* Export from this key source */
        from = bastile_object_get_source (sobj);
        g_return_val_if_fail (from != NULL, FALSE);
        ktype = bastile_source_get_tag (from);
        
        /* Find a local keysource to import to */
        if (!to) {
            to = bastile_context_find_source (sctx, ktype, BASTILE_LOCATION_LOCAL);
            if (!to) {
                /* TODO: How can we warn caller about this. Do we need to? */
                g_warning ("couldn't find a local source for: %s", 
                           g_quark_to_string (ktype));
            }
        }
        
        /* Make sure it's the same type */
        if (ktype != bastile_source_get_tag (to)) {
            /* TODO: How can we warn caller about this. Do we need to? */
            g_warning ("destination is not of type: %s", 
                       g_quark_to_string (ktype));
        }
        
        if (to != NULL && from != to) {
            
            if (op != NULL) {
                if (mop == NULL)
                    mop = bastile_multi_operation_new ();
                bastile_multi_operation_take (mop, op);
            }
            
            /* Build id list */
            for (l = objects; l; l = g_list_next (l)) 
                ids = g_slist_prepend (ids, GUINT_TO_POINTER (bastile_object_get_id (l->data)));
            ids = g_slist_reverse (ids);
        
            /* Start a new transfer operation between the two sources */
            op = bastile_transfer_operation_new (NULL, from, to, ids);
            g_return_val_if_fail (op != NULL, FALSE);
            
            g_slist_free (ids);
            ids = NULL;
        }

        g_list_free (objects);
        objects = next;
    } 
    
    /* No objects done, just return success */
    if (!mop && !op) {
        g_warning ("no valid objects to transfer found");
        return bastile_operation_new_complete (NULL);
    }
    
    return mop ? BASTILE_OPERATION (mop) : op;
}

/**
 * bastile_context_retrieve_objects:
 * @sctx: A #BastilecContext
 * @ktype: The type of the keys to transfer
 * @ids: The key ids to transfer
 * @to: A #BastileSource. If NULL, it will use @ktype to find a source
 *
 * Copies remote objects to a local source
 *
 * Returns: A #BastileOperation
 */
BastileOperation*
bastile_context_retrieve_objects (BastileContext *sctx, GQuark ktype, 
                                   GSList *ids, BastileSource *to)
{
    BastileMultiOperation *mop = NULL;
    BastileOperation *op = NULL;
    BastileSource *sksrc;
    GSList *sources, *l;
    
    if (!sctx)
        sctx = bastile_context_for_app ();
    g_return_val_if_fail (BASTILE_IS_CONTEXT (sctx), NULL);

    if (!to) {
        to = bastile_context_find_source (sctx, ktype, BASTILE_LOCATION_LOCAL);
        if (!to) {
            /* TODO: How can we warn caller about this. Do we need to? */
            g_warning ("couldn't find a local source for: %s", 
                       g_quark_to_string (ktype));
            return bastile_operation_new_complete (NULL);
        }
    }
    
    sources = bastile_context_find_sources (sctx, ktype, BASTILE_LOCATION_REMOTE);
    if (!sources) {
        g_warning ("no sources found for type: %s", g_quark_to_string (ktype));
        return bastile_operation_new_complete (NULL);
    }

    for (l = sources; l; l = g_slist_next (l)) {
        
        sksrc = BASTILE_SOURCE (l->data);
        g_return_val_if_fail (BASTILE_IS_SOURCE (sksrc), NULL);
        
        if (op != NULL) {
            if (mop == NULL)
                mop = bastile_multi_operation_new ();
            bastile_multi_operation_take (mop, op);
        }
        
        /* Start a new transfer operation between the two key sources */
        op = bastile_transfer_operation_new (NULL, sksrc, to, ids);
        g_return_val_if_fail (op != NULL, FALSE);
    }
    
    return mop ? BASTILE_OPERATION (mop) : op;
}


/**
 * bastile_context_discover_objects:
 * @sctx: the context to work with (can be NULL)
 * @ktype: the type of key to discover
 * @rawids: a list of ids to discover
 *
 * Downloads a list of keys from the keyserver
 *
 * Returns: The imported keys
 */
GList*
bastile_context_discover_objects (BastileContext *sctx, GQuark ktype, 
                                   GSList *rawids)
{
    BastileOperation *op = NULL;
    GList *robjects = NULL;
    GQuark id = 0;
    GSList *todiscover = NULL;
    GList *toimport = NULL;
    BastileSource *sksrc;
    BastileObject* sobj;
    BastileLocation loc;
    GSList *l;

    if (!sctx)
        sctx = bastile_context_for_app ();
    g_return_val_if_fail (BASTILE_IS_CONTEXT (sctx), NULL);

    /* Check all the ids */
    for (l = rawids; l; l = g_slist_next (l)) {
        
        id = bastile_context_canonize_id (ktype, (gchar*)l->data);
        if (!id) {
            /* TODO: Try and match this partial id */
            g_warning ("invalid id: %s", (gchar*)l->data);
            continue;
        }
        
        /* Do we know about this object? */
        sobj = bastile_context_find_object (sctx, id, BASTILE_LOCATION_INVALID);

        /* No such object anywhere, discover it */
        if (!sobj) {
            todiscover = g_slist_prepend (todiscover, GUINT_TO_POINTER (id));
            id = 0;
            continue;
        }
        
        /* Our return value */
        robjects = g_list_prepend (robjects, sobj);
        
        /* We know about this object, check where it is */
        loc = bastile_object_get_location (sobj);
        g_assert (loc != BASTILE_LOCATION_INVALID);
        
        /* Do nothing for local objects */
        if (loc >= BASTILE_LOCATION_LOCAL)
            continue;
        
        /* Remote objects get imported */
        else if (loc >= BASTILE_LOCATION_REMOTE)
            toimport = g_list_prepend (toimport, sobj);
        
        /* Searching objects are ignored */
        else if (loc >= BASTILE_LOCATION_SEARCHING)
            continue;
        
        /* TODO: Should we try BASTILE_LOCATION_MISSING objects again? */
    }
    
    /* Start an import process on all toimport */
    if (toimport) {
        op = bastile_context_transfer_objects (sctx, toimport, NULL);
        
        g_list_free (toimport);
        
        /* Running operations ref themselves */
        g_object_unref (op);
    }

    /* Start a discover process on all todiscover */
    if (bastile_mateconf_get_boolean (AUTORETRIEVE_KEY) && todiscover) {
        op = bastile_context_retrieve_objects (sctx, ktype, todiscover, NULL);
        
        /* Running operations ref themselves */
        g_object_unref (op);
    }

    /* Add unknown objects for all these */
    sksrc = bastile_context_find_source (sctx, ktype, BASTILE_LOCATION_MISSING);
    for (l = todiscover; l; l = g_slist_next (l)) {
        if (sksrc) {
            sobj = bastile_unknown_source_add_object (BASTILE_UNKNOWN_SOURCE (sksrc), 
                                                       GPOINTER_TO_UINT (l->data), op);
            robjects = g_list_prepend (robjects, sobj);
        }
    }

    g_slist_free (todiscover);

    return robjects;
}

/**
 * bastile_context_object_from_dbus:
 * @sctx: A #BastileContext
 * @key: the string id of the object to get
 *
 * Finds an object basing on the @key
 *
 * Returns: The #BastileObject found. NULL on not found.
 */
BastileObject*
bastile_context_object_from_dbus (BastileContext *sctx, const gchar *key)
{
    BastileObject *sobj;
    
    /* This will always get the most preferred key */
    sobj = bastile_context_find_object (sctx, g_quark_from_string (key), 
                                         BASTILE_LOCATION_INVALID);
    
    return sobj;
}

/**
 * bastile_context_object_to_dbus:
 * @sctx: A bastile context
 * @sobj: the object
 *
 * Translates an object to a string id
 *
 * Returns: The string id of the object. Free with #g_free
 */
gchar*
bastile_context_object_to_dbus (BastileContext *sctx, BastileObject *sobj)
{
    return bastile_context_id_to_dbus (sctx, bastile_object_get_id (sobj));
}

/**
 * bastile_context_id_to_dbus:
 * @sctx: ignored
 * @id: the id to translate
 *
 * Translates an id to a dbus compatible string
 *
 * Returns: A string, free with #g_free
 */
gchar*
bastile_context_id_to_dbus (BastileContext* sctx, GQuark id)
{
	return g_strdup (g_quark_to_string (id));
}

/**
 * bastile_context_canonize_id:
 * @ktype: a keytype defining the canonization function
 * @id: The id to canonize
 *
 *
 *
 * Returns: A canonized GQuark
 */
GQuark
bastile_context_canonize_id (GQuark ktype, const gchar *id)
{
	BastileCanonizeFunc canonize;

	g_return_val_if_fail (id != NULL, 0);
    
	canonize = bastile_registry_lookup_function (NULL, "canonize", g_quark_to_string (ktype), NULL);
	if (!canonize) 
		return 0;
	
	return (canonize) (id);
}
