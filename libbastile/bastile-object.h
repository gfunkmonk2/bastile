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

#ifndef __BASTILE_OBJECT_H__
#define __BASTILE_OBJECT_H__

/* 
 * Derived classes must set the following properties:
 * 
 * id: GQuark: Unique identifier for this object.
 * tag: GQuark: The module (openpgp, pkcs11, etc...) that handles this object. 
 * label: string: The displayable string label of this object.
 * icon: string: The stock ID of an icon to show for this object.
 * usage: BastileUsage: The usage of this object.
 *  
 * The following will be calculated automatically if not set:
 * 
 * markup: string: The markup string label of this object.
 * identifier: string: The displayable key identifier for this object.
 * description: string: The description of the type of object.
 *
 * The following can optionally be set:
 *  
 * location: BastileLocation: The location this object is present at. 
 * flags: guint: Flags from the BASTILE_FLAG_ set. 
 */

#include "bastile-context.h"
#include "bastile-source.h"
#include "bastile-types.h"

#include <glib-object.h>

#define BASTILE_TYPE_OBJECT               (bastile_object_get_type ())
#define BASTILE_OBJECT(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_OBJECT, BastileObject))
#define BASTILE_OBJECT_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_OBJECT, BastileObjectClass))
#define BASTILE_IS_OBJECT(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_OBJECT))
#define BASTILE_IS_OBJECT_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_OBJECT))
#define BASTILE_OBJECT_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_OBJECT, BastileObjectClass))

typedef struct _BastileObject BastileObject;
typedef struct _BastileObjectClass BastileObjectClass;
typedef struct _BastileObjectPrivate BastileObjectPrivate;
    
struct _BastileObject {
	GObject parent;
	BastileObjectPrivate *pv;
};

struct _BastileObjectClass {
	GObjectClass parent_class;
	
	/* virtual methods ------------------------------------------------- */

	void (*realize) (BastileObject *self);
	
	void (*refresh) (BastileObject *self);
	
	BastileOperation* (*delete) (BastileObject *self);
};

GType               bastile_object_get_type               (void);

BastileObject*     bastile_object_new                    (void);

void                bastile_object_realize                (BastileObject *self);

void                bastile_object_refresh                (BastileObject *self);

BastileOperation*  bastile_object_delete                 (BastileObject *self);

GQuark              bastile_object_get_id                 (BastileObject *self);

GQuark              bastile_object_get_tag                (BastileObject *self);

BastileSource*     bastile_object_get_source             (BastileObject *self);

void                bastile_object_set_source             (BastileObject *self, 
                                                            BastileSource *value);

BastileContext*    bastile_object_get_context            (BastileObject *self);

BastileObject*     bastile_object_get_preferred          (BastileObject *self);

void                bastile_object_set_preferred          (BastileObject *self, 
                                                            BastileObject *value);

BastileObject*     bastile_object_get_parent             (BastileObject *self);

void                bastile_object_set_parent             (BastileObject *self, 
                                                            BastileObject *value);
 
GList*              bastile_object_get_children           (BastileObject *self);

BastileObject*     bastile_object_get_nth_child          (BastileObject *self,
                                                            guint index);

const gchar*        bastile_object_get_label              (BastileObject *self);

const gchar*        bastile_object_get_markup             (BastileObject *self);

const gchar*        bastile_object_get_identifier         (BastileObject *self);

const gchar*        bastile_object_get_description        (BastileObject *self);

const gchar*        bastile_object_get_nickname           (BastileObject *self);

const gchar*        bastile_object_get_icon               (BastileObject *self);


BastileLocation    bastile_object_get_location           (BastileObject *self);

BastileUsage       bastile_object_get_usage              (BastileObject *self);

guint               bastile_object_get_flags              (BastileObject *self);

gboolean            bastile_object_lookup_property        (BastileObject *self, 
                                                            const gchar *field, 
                                                            GValue *value);

typedef gboolean (*BastileObjectPredicateFunc) (BastileObject *obj, void *user_data);

typedef struct _BastileObjectPredicate BastileObjectPredicate;

struct _BastileObjectPredicate {
	GQuark tag;
	GQuark id;
	GType type;
	BastileLocation location;
	BastileUsage usage;
	guint flags;
	guint nflags;
	BastileSource *source;
	BastileObjectPredicateFunc custom;
	gpointer custom_target;
};

void               bastile_object_predicate_clear         (BastileObjectPredicate *self);

gboolean           bastile_object_predicate_match         (BastileObjectPredicate *self, 
                                                            BastileObject *obj);

#endif /* __BASTILE_OBJECT_H__ */
