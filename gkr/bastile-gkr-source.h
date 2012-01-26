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

/** 
 * BastileGkrSource: A key source for mate-keyring credentials
 * 
 * - Derived from BastileSource
 * - Adds the keys it loads to the BastileContext.
 * 
 * Properties:
 *  ktype: (GQuark) The ktype (ie: BASTILE_GKR) of keys originating from this 
           key source.
 *  location: (BastileLocation) The location of keys that come from this 
 *         source. (ie: BASTILE_LOCATION_LOCAL)
 */
 
#ifndef __BASTILE_GKR_SOURCE_H__
#define __BASTILE_GKR_SOURCE_H__

#include "bastile-source.h"

#define BASTILE_TYPE_GKR_SOURCE            (bastile_gkr_source_get_type ())
#define BASTILE_GKR_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_GKR_SOURCE, BastileGkrSource))
#define BASTILE_GKR_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_GKR_SOURCE, BastileGkrSourceClass))
#define BASTILE_IS_GKR_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_GKR_SOURCE))
#define BASTILE_IS_GKR_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_GKR_SOURCE))
#define BASTILE_GKR_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_GKR_SOURCE, BastileGkrSourceClass))

typedef struct _BastileGkrSource BastileGkrSource;
typedef struct _BastileGkrSourceClass BastileGkrSourceClass;
typedef struct _BastileGkrSourcePrivate BastileGkrSourcePrivate;

struct _BastileGkrSource {
	GObject parent;
	BastileGkrSourcePrivate *pv;
};

struct _BastileGkrSourceClass {
	GObjectClass parent_class;
};

GType               bastile_gkr_source_get_type          (void);

BastileGkrSource*  bastile_gkr_source_new               (void);

BastileGkrSource*  bastile_gkr_source_default           (void);

#endif /* __BASTILE_GKR_SOURCE_H__ */
