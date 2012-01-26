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
 
#ifndef __BASTILE_UNKNOWN_SOURCE_H__
#define __BASTILE_UNKNOWN_SOURCE_H__

#include "bastile-object.h"
#include "bastile-source.h"

#define BASTILE_TYPE_UNKNOWN_SOURCE            (bastile_unknown_source_get_type ())
#define BASTILE_UNKNOWN_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_UNKNOWN_SOURCE, BastileUnknownSource))
#define BASTILE_UNKNOWN_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_UNKNOWN_SOURCE, BastileUnknownSourceClass))
#define BASTILE_IS_UNKNOWN_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_UNKNOWN_SOURCE))
#define BASTILE_IS_UNKNOWN_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_UNKNOWN_SOURCE))
#define BASTILE_UNKNOWN_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_UNKNOWN_SOURCE, BastileUnknownSourceClass))

typedef struct _BastileUnknownSource BastileUnknownSource;
typedef struct _BastileUnknownSourceClass BastileUnknownSourceClass;
typedef struct _BastileUnknownSourcePrivate BastileUnknownSourcePrivate;

/**
 * BastileUnknownSource:
 * @parent: The parent #GObject
 * @ktype: The #GQuark key type for the source
 *
 * A  source for unknown objects
 *
 * - Derived from BastileSource
 * - Is used for objects that haven't been found on a key server.
 *
 * Properties:
 *  ktype: (GQuark) The ktype (ie: SKEY_UNKNOWN) of keys originating from this
           key source.
 *  location: (BastileLocation) The location of objects that come from this
 *         source. (ie: BASTILE_LOCATION_MISSING)
 */

struct _BastileUnknownSource {
	GObject parent;
	GQuark ktype;
};

struct _BastileUnknownSourceClass {
	GObjectClass parent_class;
};

GType                    bastile_unknown_source_get_type      (void);

BastileUnknownSource*   bastile_unknown_source_new           (GQuark ktype);

BastileObject*          bastile_unknown_source_add_object    (BastileUnknownSource *usrc,
                                                                GQuark id,
                                                                BastileOperation *search);

#endif /* __BASTILE_UNKNOWN_SOURCE_H__ */
