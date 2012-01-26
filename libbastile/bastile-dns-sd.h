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
 
#ifndef __BASTILE_DNS_SD_H__
#define __BASTILE_DNS_SD_H__

#include <glib.h>

#define BASTILE_TYPE_SERVICE_DISCOVERY             (bastile_service_discovery_get_type ())
#define BASTILE_SERVICE_DISCOVERY(obj)	            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_SERVICE_DISCOVERY, BastileServiceDiscovery))
#define BASTILE_SERVICE_DISCOVERY_CLASS(klass)	    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_SERVICE_DISCOVERY, BastileServiceDiscoveryClass))
#define BASTILE_IS_SERVICE_DISCOVERY(obj)		    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_SERVICE_DISCOVERY))
#define BASTILE_IS_SERVICE_DISCOVERY_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_SERVICE_DISCOVERY))
#define BASTILE_SERVICE_DISCOVERY_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_SERVICE_DISCOVERY, BastileServiceDiscoveryClass))

typedef struct _BastileServiceDiscovery BastileServiceDiscovery;
typedef struct _BastileServiceDiscoveryClass BastileServiceDiscoveryClass;
typedef struct _BastileServiceDiscoveryPriv BastileServiceDiscoveryPriv;

/**
 * BastileServiceDiscovery:
 * @parent: The parent #GObject
 * @services: A #GHashTable of known services
 *
 * Listens for DNS-SD shared keys on the network and
 * adds BastileKeySoruce objects to the BastileContext as necessary.
 *
 * Signals:
 *   added: A new shared key source was found.
 *   removed: A shared key source went away.
 */

struct _BastileServiceDiscovery {
    GObject parent;

    /*< public >*/
    GHashTable  *services;
    
    /*< private >*/
    BastileServiceDiscoveryPriv *priv;
};

struct _BastileServiceDiscoveryClass {
    GObjectClass parent_class;

    /* A relevant service appeared on the network */
    void (*added) (BastileServiceDiscovery *ssd, const gchar* service);

    /* A service disappeared */
    void (*removed) (BastileServiceDiscovery *ssd, const gchar* service);
};

GType                       bastile_service_discovery_get_type  (void);

BastileServiceDiscovery*   bastile_service_discovery_new       ();

GSList*                     bastile_service_discovery_list      (BastileServiceDiscovery *ssd);

const gchar*                bastile_service_discovery_get_uri   (BastileServiceDiscovery *ssd,
                                                                  const gchar *service);

GSList*                     bastile_service_discovery_get_uris  (BastileServiceDiscovery *ssd,
                                                                  GSList *services);

#endif /* __BASTILE_KEY_H__ */
