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

#ifndef __BASTILE_PKCS11_OBJECT_H__
#define __BASTILE_PKCS11_OBJECT_H__

#include <gck/gck.h>

#include <glib-object.h>

#include "bastile-object.h"

#define BASTILE_PKCS11_TYPE_OBJECT               (bastile_pkcs11_object_get_type ())
#define BASTILE_PKCS11_OBJECT(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_PKCS11_TYPE_OBJECT, BastilePkcs11Object))
#define BASTILE_PKCS11_OBJECT_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_PKCS11_TYPE_OBJECT, BastilePkcs11ObjectClass))
#define BASTILE_PKCS11_IS_OBJECT(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_PKCS11_TYPE_OBJECT))
#define BASTILE_PKCS11_IS_OBJECT_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_PKCS11_TYPE_OBJECT))
#define BASTILE_PKCS11_OBJECT_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_PKCS11_TYPE_OBJECT, BastilePkcs11ObjectClass))

typedef struct _BastilePkcs11Object BastilePkcs11Object;
typedef struct _BastilePkcs11ObjectClass BastilePkcs11ObjectClass;
typedef struct _BastilePkcs11ObjectPrivate BastilePkcs11ObjectPrivate;
    
struct _BastilePkcs11Object {
	BastileObject parent;
	BastilePkcs11ObjectPrivate *pv;
};

struct _BastilePkcs11ObjectClass {
	BastileObjectClass parent_class;
};

GType                       bastile_pkcs11_object_get_type               (void);

BastilePkcs11Object*       bastile_pkcs11_object_new                    (GckObject* object);

gulong                      bastile_pkcs11_object_get_pkcs11_handle      (BastilePkcs11Object* self);

GckObject*                  bastile_pkcs11_object_get_pkcs11_object      (BastilePkcs11Object* self);

GckAttributes*              bastile_pkcs11_object_get_pkcs11_attributes  (BastilePkcs11Object* self);

void                        bastile_pkcs11_object_set_pkcs11_attributes  (BastilePkcs11Object* self, 
                                                                           GckAttributes* value);

GckAttribute*               bastile_pkcs11_object_require_attribute      (BastilePkcs11Object *self,
                                                                           gulong attr_type);

gboolean                    bastile_pkcs11_object_require_attributes     (BastilePkcs11Object *self,
                                                                           const gulong *attr_types,
                                                                           gsize n_attr_types);

GQuark                      bastile_pkcs11_object_cannonical_id          (GckObject *object);

#endif /* __BASTILE_PKCS11_OBJECT_H__ */
