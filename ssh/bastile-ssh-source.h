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
 * BastileSSHSource: A key source for SSH keys. 
 * 
 * - Derived from BastileSource
 * - Lists all the keys in ~/.ssh/ by searching through every file.
 * - Loads public keys from ~/.ssh/authorized_keys and ~/.ssh/other_keys.bastile
 * - Adds the keys it loads to the BastileContext.
 * - Monitors ~/.ssh for changes and reloads the key ring as necessary.
 * 
 * Properties:
 *  ktype: (GQuark) The ktype (ie: BASTILE_SSH) of keys originating from this 
           key source.
 *  location: (BastileLocation) The location of keys that come from this 
 *         source. (ie: BASTILE_LOCATION_LOCAL)
 */
 
#ifndef __BASTILE_SSH_SOURCE_H__
#define __BASTILE_SSH_SOURCE_H__

#include "bastile-ssh-key.h"
#include "bastile-source.h"

#define BASTILE_TYPE_SSH_SOURCE            (bastile_ssh_source_get_type ())
#define BASTILE_SSH_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_SSH_SOURCE, BastileSSHSource))
#define BASTILE_SSH_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_SSH_SOURCE, BastileSSHSourceClass))
#define BASTILE_IS_SSH_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_SSH_SOURCE))
#define BASTILE_IS_SSH_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_SSH_SOURCE))
#define BASTILE_SSH_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_SSH_SOURCE, BastileSSHSourceClass))

struct _BastileSSHKey;
typedef struct _BastileSSHSource BastileSSHSource;
typedef struct _BastileSSHSourceClass BastileSSHSourceClass;
typedef struct _BastileSSHSourcePrivate BastileSSHSourcePrivate;

struct _BastileSSHSource {
	GObject parent;
	BastileSSHSourcePrivate *priv;
};

struct _BastileSSHSourceClass {
	GObjectClass parent_class;
};

GType                bastile_ssh_source_get_type           (void);

BastileSSHSource*   bastile_ssh_source_new                (void);

struct _BastileSSHKey*      
                     bastile_ssh_source_key_for_filename   (BastileSSHSource *ssrc, 
                                                             const gchar *privfile);

gchar*               bastile_ssh_source_file_for_public    (BastileSSHSource *ssrc,
                                                             gboolean authorized);

gchar*               bastile_ssh_source_file_for_algorithm (BastileSSHSource *ssrc,
                                                             guint algo);

guchar*              bastile_ssh_source_export_private     (BastileSSHSource *ssrc,
                                                             BastileSSHKey *skey,
                                                             gsize *n_results,
                                                             GError **err);


#endif /* __BASTILE_SSH_SOURCE_H__ */
