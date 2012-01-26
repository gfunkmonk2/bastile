/*
 * Bastile
 *
 * Copyright (C) 2003 Jacob Perkins
 * Copyright (C) 2004-2005 Stefan Walter
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

#ifndef __BASTILE_CONTEXT_H__
#define __BASTILE_CONTEXT_H__

#include <gtk/gtk.h>

#include "bastile-source.h"
#include "bastile-dns-sd.h"

#define BASTILE_TYPE_CONTEXT			(bastile_context_get_type ())
#define BASTILE_CONTEXT(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_CONTEXT, BastileContext))
#define BASTILE_CONTEXT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_CONTEXT, BastileContextClass))
#define BASTILE_IS_CONTEXT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_CONTEXT))
#define BASTILE_IS_CONTEXT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_CONTEXT))
#define BASTILE_CONTEXT_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_CONTEXT, BastileContextClass))

struct _BastileKey;
struct _BastileObject;
struct _BastileObjectPredicate;

typedef struct _BastileContext BastileContext;
typedef struct _BastileContextClass BastileContextClass;
typedef struct _BastileContextPrivate BastileContextPrivate;

/**
 * BastileContext:
 * @parent: The parent #GtkObject
 * @is_daemon: a #gboolean indicating whether the context is being used in a
 *             program that is daemonized
 *
 * This is where all the action in a Bastile process comes together.
 *
 * - Usually there's only one #BastileContext per process created by passing
 *   %BASTILE_CONTEXT_APP to bastile_context_new(), and accessed via
 *   the %SCTX_APP macro.
 * - Retains the list of all valid struct _BastileObject objects.
 * - Has a collection of #BastileSource objects which add objects to the
 *   #BastileContext.
 *
 * Signals:
 *   added: A object was added to the context.
 *   removed: A object was removed from the context.
 *   changed: A object changed.
 *   destroy: The context was destroyed.
 */

struct _BastileContext {
	GObject parent;

	/*< public >*/
	gboolean is_daemon;

	/*< private >*/
	BastileContextPrivate  *pv;
};

struct _BastileContextClass {
	GObjectClass parent_class;

    /* signals --------------------------------------------------------- */
    
    /* A object was added to this source */
    void (*added) (BastileContext *sctx, struct _BastileObject *sobj);

    /* Removed a object from this source */
    void (*removed) (BastileContext *sctx, struct _BastileObject *sobj);
    
    /* This object has changed */
    void (*changed) (BastileContext *sctx, struct _BastileObject *sobj);
    
    /* The source is being refreshed */
    void (*refreshing) (BastileContext *sctx, BastileOperation *op);

	void (*destroy) (BastileContext *sctx);
};

enum BastileContextType {
    BASTILE_CONTEXT_APP = 1,
    BASTILE_CONTEXT_DAEMON = 2,
};

typedef void (*BastileObjectFunc) (struct _BastileObject *obj, gpointer user_data);

#define             SCTX_APP()                          (bastile_context_for_app ())

GType               bastile_context_get_type           (void);

BastileContext*    bastile_context_for_app            (void);

BastileContext*    bastile_context_new                (guint              flags);

void                bastile_context_destroy            (BastileContext    *sctx);

#define             bastile_context_is_daemon(ctx)     ((ctx)->is_daemon)

void                bastile_context_add_source         (BastileContext    *sctx,
                                                         BastileSource  *sksrc);

void                bastile_context_take_source        (BastileContext    *sctx,
                                                         BastileSource  *sksrc);

void                bastile_context_remove_source      (BastileContext    *sctx,
                                                         BastileSource  *sksrc);

BastileSource*     bastile_context_find_source        (BastileContext    *sctx,
                                                         GQuark             ktype,
                                                         BastileLocation   location);

GSList*             bastile_context_find_sources       (BastileContext    *sctx,
                                                         GQuark             ktype,
                                                         BastileLocation   location);
                                                         

BastileSource*     bastile_context_remote_source      (BastileContext    *sctx,
                                                         const gchar        *uri);

void                bastile_context_add_object         (BastileContext    *sctx,
                                                         struct _BastileObject     *sobj);

void                bastile_context_take_object        (BastileContext    *sctx, 
                                                         struct _BastileObject     *sobj);

guint               bastile_context_get_count          (BastileContext    *sctx);

struct _BastileObject*     bastile_context_get_object  (BastileContext    *sctx,
                                                         BastileSource  *sksrc,
                                                         GQuark             id);

GList*              bastile_context_get_objects        (BastileContext    *sctx, 
                                                         BastileSource  *sksrc);

struct _BastileObject*     bastile_context_find_object (BastileContext    *sctx,
                                                         GQuark             id,
                                                         BastileLocation   location);

GList*              bastile_context_find_objects       (BastileContext    *sctx,
                                                         GQuark             ktype,
                                                         BastileUsage      usage,
                                                         BastileLocation   location);

GList*              bastile_context_find_objects_full  (BastileContext *self,
                                                         struct _BastileObjectPredicate *skpred);

void                bastile_context_for_objects_full   (BastileContext *self,
                                                         struct _BastileObjectPredicate *skpred,
                                                         BastileObjectFunc func,
                                                         gpointer user_data);

void                bastile_context_verify_objects     (BastileContext *self);

void                bastile_context_remove_object      (BastileContext *sctx,
                                                         struct _BastileObject *sobj);

BastileServiceDiscovery*
                    bastile_context_get_discovery      (BastileContext    *sctx);

struct _BastileObject*   
                    bastile_context_get_default_key    (BastileContext    *sctx);

void                bastile_context_refresh_auto       (BastileContext *sctx);

BastileOperation*  bastile_context_search_remote      (BastileContext    *sctx,
                                                         const gchar        *search);

BastileOperation*  bastile_context_transfer_objects   (BastileContext    *sctx, 
                                                         GList              *objs, 
                                                         BastileSource  *to);

BastileOperation*  bastile_context_retrieve_objects   (BastileContext    *sctx, 
                                                         GQuark             ktype, 
                                                         GSList             *ids, 
                                                         BastileSource  *to);

GList*              bastile_context_discover_objects   (BastileContext    *sctx, 
                                                         GQuark             ktype, 
                                                         GSList             *ids);

struct _BastileObject*     bastile_context_object_from_dbus   (BastileContext    *sctx,
                                                         const gchar        *dbusid);

gchar*              bastile_context_object_to_dbus     (BastileContext    *sctx,
                                                         struct _BastileObject *sobj);

gchar*              bastile_context_id_to_dbus         (BastileContext    *sctx,
                                                         GQuark             id);


typedef GQuark (*BastileCanonizeFunc) (const gchar *id);

GQuark              bastile_context_canonize_id        (GQuark ktype, const gchar *id);

#endif /* __BASTILE_CONTEXT_H__ */
