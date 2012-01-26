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

/**
 * BastileHKPSource: A key source which searches HTTP PGP key servers. 
 * 
 * - Derived from BastileServerSource.
 * - Adds found keys to BastileContext. 
 * - Used by BastileServiceDiscovery for retrieving shared keys.
 */
 
#ifndef __BASTILE_HKP_SOURCE_H__
#define __BASTILE_HKP_SOURCE_H__

#include "config.h"
#include "bastile-server-source.h"

#ifdef WITH_HKP

#define BASTILE_TYPE_HKP_SOURCE            (bastile_hkp_source_get_type ())
#define BASTILE_HKP_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_HKP_SOURCE, BastileHKPSource))
#define BASTILE_HKP_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_HKP_SOURCE, BastileHKPSourceClass))
#define BASTILE_IS_HKP_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_HKP_SOURCE))
#define BASTILE_IS_HKP_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_HKP_SOURCE))
#define BASTILE_HKP_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_HKP_SOURCE, BastileHKPSourceClass))

typedef struct _BastileHKPSource BastileHKPSource;
typedef struct _BastileHKPSourceClass BastileHKPSourceClass;

struct _BastileHKPSource {
    BastileServerSource parent;
    
    /*< private >*/
};

struct _BastileHKPSourceClass {
    BastileServerSourceClass parent_class;
};

GType                 bastile_hkp_source_get_type (void);

BastileHKPSource*    bastile_hkp_source_new      (const gchar *uri, 
                                                    const gchar *host);

gboolean              bastile_hkp_is_valid_uri    (const gchar *uri);

#endif /* WITH_HKP */

#endif /* __BASTILE_HKP_SOURCE_H__ */
