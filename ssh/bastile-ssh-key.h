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

#ifndef __BASTILE_SSH_KEY_H__
#define __BASTILE_SSH_KEY_H__

#include <gtk/gtk.h>

#include "bastile-object.h"
#include "bastile-operation.h"
#include "bastile-source.h"
#include "bastile-validity.h"

#include "bastile-ssh-module.h"
#include "bastile-ssh-key-data.h"

/* For vala's sake */
#define BASTILE_SSH_TYPE_KEY            BASTILE_TYPE_SSH_KEY

#define BASTILE_TYPE_SSH_KEY            (bastile_ssh_key_get_type ())
#define BASTILE_SSH_KEY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_SSH_KEY, BastileSSHKey))
#define BASTILE_SSH_KEY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_SSH_KEY, BastileSSHKeyClass))
#define BASTILE_IS_SSH_KEY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_SSH_KEY))
#define BASTILE_IS_SSH_KEY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_SSH_KEY))
#define BASTILE_SSH_KEY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_SSH_KEY, BastileSSHKeyClass))

typedef struct _BastileSSHKey BastileSSHKey;
typedef struct _BastileSSHKeyClass BastileSSHKeyClass;
typedef struct _BastileSSHKeyPrivate BastileSSHKeyPrivate;

struct _BastileSSHKey {
    BastileObject	parent;

    /*< public >*/
    struct _BastileSSHKeyData *keydata;
        
    /*< private >*/
    struct _BastileSSHKeyPrivate *priv;
};


struct _BastileSSHKeyClass {
    BastileObjectClass            parent_class;
};

BastileSSHKey*         bastile_ssh_key_new                  (BastileSource *sksrc, 
                                                               BastileSSHKeyData *data);

GType                   bastile_ssh_key_get_type             (void);

guint                   bastile_ssh_key_get_algo             (BastileSSHKey *skey);

const gchar*            bastile_ssh_key_get_algo_str         (BastileSSHKey *skey);

guint                   bastile_ssh_key_get_strength         (BastileSSHKey *skey);

const gchar*            bastile_ssh_key_get_location         (BastileSSHKey *skey);
                                                                   
BastileOperation*      bastile_ssh_key_op_change_passphrase (BastileSSHKey *skey);

BastileValidity        bastile_ssh_key_get_trust            (BastileSSHKey *self);

gchar*                  bastile_ssh_key_get_fingerprint      (BastileSSHKey *self);

GQuark                  bastile_ssh_key_calc_cannonical_id    (const gchar *id);

gchar*                  bastile_ssh_key_calc_identifier      (const gchar *id);

#endif /* __BASTILE_KEY_H__ */
