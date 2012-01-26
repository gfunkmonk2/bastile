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
 * BastileServerSoruce: A base class for key sources that retrieve keys
 * from remote key servers.
 * 
 * - Derived from BastileSource.
 * - Also includes functions for parsing keyserver URIs and mapping them
 *   to the appropriate key sources (such as BastileHKPSource)
 * - There's some GPGME specific stuff in here that may eventually need to be 
 *   moved elsewhere.
 * 
 * Properties:
 *   key-type: (GQuark) The type of keys generated (ie: SKEY_PGP)
 *   location: (gchar*) The location of keys from this key source (ie: BASTILE_LOCATION_REMOTE)
 *   key-server: (gchar*) The host:port of the keyserver to search.
 *   uri: (gchar*) Only for remote key sources. The full URI of the keyserver 
 *        being used. 
 */
 
#ifndef __BASTILE_SERVER_SOURCE_H__
#define __BASTILE_SERVER_SOURCE_H__

#include "bastile-source.h"
#include "bastile-operation.h"

#define BASTILE_TYPE_SERVER_SOURCE            (bastile_server_source_get_type ())
#define BASTILE_SERVER_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_SERVER_SOURCE, BastileServerSource))
#define BASTILE_SERVER_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_SERVER_SOURCE, BastileServerSourceClass))
#define BASTILE_IS_SERVER_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_SERVER_SOURCE))
#define BASTILE_IS_SERVER_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_SERVER_SOURCE))
#define BASTILE_SERVER_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_SERVER_SOURCE, BastileServerSourceClass))

typedef struct _BastileServerSource BastileServerSource;
typedef struct _BastileServerSourceClass BastileServerSourceClass;
typedef struct _BastileServerSourcePrivate BastileServerSourcePrivate;

struct _BastileServerSource {
	GObject parent;
	BastileServerSourcePrivate *priv;
};

struct _BastileServerSourceClass {
	GObjectClass parent_class;
};

GType        bastile_server_source_get_type         (void);

void         bastile_server_source_take_operation   (BastileServerSource *ssrc,
                                                      BastileOperation *operation);

BastileServerSource* bastile_server_source_new     (const gchar *uri);

#endif /* __BASTILE_SERVER_SOURCE_H__ */
