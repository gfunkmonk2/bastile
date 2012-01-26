/*
 * Bastile
 *
 * Copyright (C) 2005-2006 Stefan Walter
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

#ifndef __BASTILE_SET_H__
#define __BASTILE_SET_H__

#include "bastile-context.h"
#include "bastile-object.h"
#include "bastile-source.h"

#define BASTILE_TYPE_SET               (bastile_set_get_type ())
#define BASTILE_SET(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_SET, BastileSet))
#define BASTILE_SET_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_SET, BastileSetClass))
#define BASTILE_IS_SET(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_SET))
#define BASTILE_IS_SET_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_SET))
#define BASTILE_SET_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_SET, BastileSetClass))

typedef struct _BastileSet BastileSet;
typedef struct _BastileSetClass BastileSetClass;
typedef struct _BastileSetPrivate BastileSetPrivate;

/**
 * BastileSet:
 * @parent: The parent #GtkObject
 *
 * A subset list of the keys in the BastileContext.
 *
 * - Used all over by various views to narrow in on the keys that they're
 *   interested in.
 * - Originally was going to be called BastileView (database parlance) but
 *   that's far too confusing with overloaded terminology.
 * - Uses a BastileObjectPredicate to match keys.
 * - Sends out events when keys get added and removed from it's view. Or a
 *   key in the view changes etc...
 * - Supports per key event 'closures'. When a closure is set for a key, it's
 *   then passed as an argument to the 'changed' and 'removed' events.
 *
 * Signals:
 *   added: A key was added to this keyset.
 *   removed: A key disappeared from this keyset.
 *   changed: A key in the keyset changed.
 *   set-changed: The number of keys in the keyset changed
 *
 * Properties:
 *   count: The number of keys
 *   predicate: (BastileObjectPredicate) The predicate used for matching.
 */

struct _BastileSet {
	GObject parent;

	/*<private>*/
	BastileSetPrivate   *pv;
};

struct _BastileSetClass {
	GObjectClass parent_class;

    /* signals --------------------------------------------------------- */
    
    /* A key was added to this view */
    void (*added)   (BastileSet *skset, BastileObject *sobj);

    /* Removed a key from this view */
    void (*removed) (BastileSet *skset, BastileObject *sobj, gpointer closure);
    
    /* One of the key's attributes has changed */
    void (*changed) (BastileSet *skset, BastileObject *sobj, gpointer closure);
    
    /* The set of keys changed */
    void (*set_changed) (BastileSet *skset);
};

GType               bastile_set_get_type              (void);

BastileSet*        bastile_set_new                   (GQuark             ktype,
                                                        BastileUsage      usage,
                                                        BastileLocation   location,
                                                        guint              flags,
                                                        guint              nflags);

BastileSet*        bastile_set_new_full               (BastileObjectPredicate *pred);

gboolean            bastile_set_has_object             (BastileSet *skset,
                                                         BastileObject *sobj);

GList*              bastile_set_get_objects            (BastileSet *skset);

guint               bastile_set_get_count              (BastileSet *skset);

void                bastile_set_refresh                (BastileSet *skset);

gpointer            bastile_set_get_closure            (BastileSet *skset,
                                                         BastileObject *sobj);

void                bastile_set_set_closure            (BastileSet *skset,
                                                         BastileObject *sobj,
                                                         gpointer closure);

#endif /* __BASTILE_SET_H__ */
