/*
 * Bastile
 *
 * Copyright (C) 2008 Stefan Walter
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


#ifndef __BASTILE_PKCS11_SOURCE_H__
#define __BASTILE_PKCS11_SOURCE_H__

#include "bastile-source.h"

#include <gck/gck.h>

#define BASTILE_TYPE_PKCS11_SOURCE            (bastile_pkcs11_source_get_type ())
#define BASTILE_PKCS11_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_PKCS11_SOURCE, BastilePkcs11Source))
#define BASTILE_PKCS11_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_PKCS11_SOURCE, BastilePkcs11SourceClass))
#define BASTILE_IS_PKCS11_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_PKCS11_SOURCE))
#define BASTILE_IS_PKCS11_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_PKCS11_SOURCE))
#define BASTILE_PKCS11_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_PKCS11_SOURCE, BastilePkcs11SourceClass))

typedef struct _BastilePkcs11Source BastilePkcs11Source;
typedef struct _BastilePkcs11SourceClass BastilePkcs11SourceClass;
typedef struct _BastilePkcs11SourcePrivate BastilePkcs11SourcePrivate;

struct _BastilePkcs11Source {
	GObject parent;
	BastilePkcs11SourcePrivate *pv;
};

struct _BastilePkcs11SourceClass {
	GObjectClass parent_class;
};

GType                  bastile_pkcs11_source_get_type          (void);

BastilePkcs11Source*  bastile_pkcs11_source_new               (GckSlot *slot);

GckSlot*               bastile_pkcs11_source_get_slot          (BastilePkcs11Source *self);

void                   bastile_pkcs11_source_receive_object    (BastilePkcs11Source *self, GckObject *obj);

#endif /* __BASTILE_PKCS11_SOURCE_H__ */
