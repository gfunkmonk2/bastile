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
 * BastileHKPSource: A key source which searches LDAP PGP key servers. 
 * 
 * - Derived from BastileServerSource.
 * - Adds found keys to BastileContext. 
 */
 
#ifndef __BASTILE_LDAP_SOURCE_H__
#define __BASTILE_LDAP_SOURCE_H__

#include "bastile-server-source.h"

#ifdef WITH_LDAP

#define BASTILE_TYPE_LDAP_SOURCE            (bastile_ldap_source_get_type ())
#define BASTILE_LDAP_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_LDAP_SOURCE, BastileLDAPSource))
#define BASTILE_LDAP_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_LDAP_SOURCE, BastileLDAPSourceClass))
#define BASTILE_IS_LDAP_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_LDAP_SOURCE))
#define BASTILE_IS_LDAP_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_LDAP_SOURCE))
#define BASTILE_LDAP_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_LDAP_SOURCE, BastileLDAPSourceClass))

typedef struct _BastileLDAPSource BastileLDAPSource;
typedef struct _BastileLDAPSourceClass BastileLDAPSourceClass;

struct _BastileLDAPSource {
    BastileServerSource parent;
    
    /*< private >*/
};

struct _BastileLDAPSourceClass {
    BastileServerSourceClass parent_class;
};

GType                 bastile_ldap_source_get_type (void);

BastileLDAPSource*   bastile_ldap_source_new     (const gchar *uri, 
                                                    const gchar *host);

gboolean              bastile_ldap_is_valid_uri   (const gchar *uri);

#endif /* WITH_LDAP */

#endif /* __BASTILE_SERVER_SOURCE_H__ */
