/*
 * Bastile
 *
 * Copyright (C) 2004 Stefan Walter
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
 * BastileGpgmeSource: A key source for PGP keys retrieved from GPGME. 
 * 
 * - Derived from BastileSource
 * - Since GPGME represents secret keys as seperate from public keys, this
 *   class takes care to combine them into one logical BastilePGPKey object. 
 * - Adds the keys it loads to the BastileContext.
 * - Eventually a lot of stuff from bastile-op.* should probably be merged
 *   into this class.
 * - Monitors ~/.gnupg for changes and reloads the key ring as necessary.
 * 
 * Properties:
 *  ktype: (GQuark) The ktype (ie: BASTILE_PGP) of keys originating from this 
           key source.
 *  location: (BastileLocation) The location of keys that come from this 
 *         source. (ie: BASTILE_LOCATION_LOCAL, BASTILE_LOCATION_REMOTE)
 */
 
#ifndef __BASTILE_GPGME_SOURCE_H__
#define __BASTILE_GPGME_SOURCE_H__

#include "bastile-source.h"

#include <gpgme.h>

#define BASTILE_TYPE_GPGME_SOURCE            (bastile_gpgme_source_get_type ())
#define BASTILE_GPGME_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_GPGME_SOURCE, BastileGpgmeSource))
#define BASTILE_GPGME_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_GPGME_SOURCE, BastileGpgmeSourceClass))
#define BASTILE_IS_GPGME_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_GPGME_SOURCE))
#define BASTILE_IS_GPGME_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_GPGME_SOURCE))
#define BASTILE_GPGME_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_GPGME_SOURCE, BastileGpgmeSourceClass))

typedef struct _BastileGpgmeSource BastileGpgmeSource;
typedef struct _BastileGpgmeSourceClass BastileGpgmeSourceClass;
typedef struct _BastileGpgmeSourcePrivate BastileGpgmeSourcePrivate;

struct _BastileGpgmeSource {
	GObject parent;
	gpgme_ctx_t gctx;
	BastileGpgmeSourcePrivate *pv;
};

struct _BastileGpgmeSourceClass {
	GObjectClass parent_class;
};

GType                bastile_gpgme_source_get_type       (void);

BastileGpgmeSource*   bastile_gpgme_source_new            (void);

gpgme_ctx_t          bastile_gpgme_source_new_context    ();

#endif /* __BASTILE_GPGME_SOURCE_H__ */
