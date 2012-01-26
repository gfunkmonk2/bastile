/*
 * Bastile
 *
 * Copyright (C) 2004,2005 Stefan Walter
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
 * BastileSource: Base class for other sources. 
 * 
 * - A generic interface for accessing sources.
 * - Eventually more functionality will be merged from bastile-op.* into 
 *   this class and derived classes. 
 * - Each BastileObject has a weak pointer to the BastileSource that 
 *   created it.
 * 
 * Properties base classes must implement:
 *  ktype: (GQuark) The ktype (ie: BASTILE_PGP) of objects originating from this 
 *         object source.
 *  location: (BastileLocation) The location of objects that come from this 
 *         source. (ie: BASTILE_LOCATION_LOCAL, BASTILE_LOCATION_REMOTE)
 *  uri: (gchar*) Only for remote object sources. The full URI of the keyserver 
 *         being used.
 */


#ifndef __BASTILE_SOURCE_H__
#define __BASTILE_SOURCE_H__

#include "bastile-operation.h"
#include "bastile-types.h"

#include <gio/gio.h>
#include <glib-object.h>

#define BASTILE_TYPE_SOURCE                (bastile_source_get_type ())
#define BASTILE_SOURCE(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_SOURCE, BastileSource))
#define BASTILE_IS_SOURCE(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_SOURCE))
#define BASTILE_SOURCE_GET_INTERFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BASTILE_TYPE_SOURCE, BastileSourceIface))

struct _BastileObject;

typedef struct _BastileSource BastileSource;
typedef struct _BastileSourceIface BastileSourceIface;

struct _BastileSourceIface {
	GTypeInterface parent;
    
	/* virtual methods ------------------------------------------------- */

	/**
	 * load
	 * @sksrc: The #BastileSource.
	 * 
	 * Loads the requested objects, and add the objects to BastileContext. 
	 * 
	 * Returns: The load operation.
	 */
	BastileOperation* (*load) (BastileSource *sksrc);

	/**
	 * search
	 * @sksrc: The #BastileSource 
	 * @match: Match text
	 *
	 * Searches for objects in the source.
	 *
	 * Returns: The search operation.
	 */
	BastileOperation* (*search) (BastileSource *sksrc, const gchar *match);

    
	/**
	 * import
	 * @sksrc: The #BastileSource to import into.
	 * @input: The data to import.
	 *
	 * Import objects into the source. When operation is 'done' a GList of 
	 * updated objects may be found as the operation result. 
	 * 
	 * Returns: The import operation
	 */
	BastileOperation* (*import) (BastileSource *sksrc, GInputStream *input);

	/**
	 * export
	 * @sksrc: The #BastileSource to export from.
	 * @objects: A list of objects to export.
	 * @output: Output stream to export to.
	 *
	 * Import objects into the object source. When operation is 'done' the result
	 * of the operation will be a GOutputStream
	 * 
	 * Returns: The export operation
	 */
	BastileOperation* (*export) (BastileSource *sksrc, GList *objects, GOutputStream *output);

	/**
	 * export_raw
	 * @sksrc: The #BastileSource to export from.
	 * @ids: A list of ids to export.
	 * @data: output stream to export to.
	 *
	 * Import objects into the source. When operation is 'done' the result
	 * of the operation will be a GOutputStream
	 * 
	 * Returns: The export operation
	 */
	BastileOperation* (*export_raw) (BastileSource *sksrc, GSList *ids, 
	                                  GOutputStream *output);
    
};

GType               bastile_source_get_type             (void) G_GNUC_CONST;

/* Method helper functions ------------------------------------------- */


BastileOperation*  bastile_source_load                  (BastileSource *sksrc);
                                                          
void                bastile_source_load_sync             (BastileSource *sksrc);

void                bastile_source_load_async            (BastileSource *sksrc);

BastileOperation*  bastile_source_search                (BastileSource *sksrc,
                                                           const gchar *match);

/* Takes ownership of |data| */
BastileOperation*  bastile_source_import                (BastileSource *sksrc,
                                                           GInputStream *input);

/* Takes ownership of |data| */
gboolean            bastile_source_import_sync           (BastileSource *sksrc,
                                                           GInputStream *input,
                                                           GError **err);

BastileOperation*  bastile_source_export_objects        (GList *objects, 
                                                           GOutputStream *output);

BastileOperation*  bastile_source_delete_objects        (GList *objects);

BastileOperation*  bastile_source_export                (BastileSource *sksrc,
                                                           GList *objects,
                                                           GOutputStream *output);

BastileOperation*  bastile_source_export_raw            (BastileSource *sksrc, 
                                                           GSList *ids, 
                                                           GOutputStream *output);

GQuark              bastile_source_get_tag               (BastileSource *sksrc);

BastileLocation    bastile_source_get_location          (BastileSource *sksrc);

#endif /* __BASTILE_SOURCE_H__ */
