/* 
 * Bastile
 * 
 * Copyright (C) 2008 Stefan Walter
 * 
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *  
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.  
 */

#ifndef __BASTILE_GKR_KEYRING_H__
#define __BASTILE_GKR_KEYRING_H__

#include <glib-object.h>

#include "bastile-object.h"

#include <mate-keyring.h>

#define BASTILE_TYPE_GKR_KEYRING               (bastile_gkr_keyring_get_type ())
#define BASTILE_GKR_KEYRING(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_GKR_KEYRING, BastileGkrKeyring))
#define BASTILE_GKR_KEYRING_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_GKR_KEYRING, BastileGkrKeyringClass))
#define BASTILE_IS_GKR_KEYRING(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_GKR_KEYRING))
#define BASTILE_IS_GKR_KEYRING_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_GKR_KEYRING))
#define BASTILE_GKR_KEYRING_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_GKR_KEYRING, BastileGkrKeyringClass))

typedef struct _BastileGkrKeyring BastileGkrKeyring;
typedef struct _BastileGkrKeyringClass BastileGkrKeyringClass;
typedef struct _BastileGkrKeyringPrivate BastileGkrKeyringPrivate;
    
struct _BastileGkrKeyring {
	BastileObject parent;
	BastileGkrKeyringPrivate *pv;
};

struct _BastileGkrKeyringClass {
	BastileObjectClass parent_class;
};

GType                bastile_gkr_keyring_get_type         (void);

BastileGkrKeyring*  bastile_gkr_keyring_new              (const gchar *keyring_name);

const gchar*         bastile_gkr_keyring_get_name         (BastileGkrKeyring *self);

MateKeyringInfo*    bastile_gkr_keyring_get_info         (BastileGkrKeyring *self);

void                 bastile_gkr_keyring_set_info         (BastileGkrKeyring *self,
                                                            MateKeyringInfo *info);

gboolean             bastile_gkr_keyring_get_is_default   (BastileGkrKeyring *self);

void                 bastile_gkr_keyring_set_is_default   (BastileGkrKeyring *self,
                                                            gboolean is_default);

#endif /* __BASTILE_GKR_KEYRING_H__ */
