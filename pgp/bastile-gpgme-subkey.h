/*
 * Bastile
 *
 * Copyright (C) 2008 Stefan Walter
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

#ifndef __BASTILE_GPGME_SUBKEY_H__
#define __BASTILE_GPGME_SUBKEY_H__

#include <glib-object.h>

#include <gpgme.h>

#include "pgp/bastile-pgp-subkey.h"

#define BASTILE_TYPE_GPGME_SUBKEY            (bastile_gpgme_subkey_get_type ())
#define BASTILE_GPGME_SUBKEY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_GPGME_SUBKEY, BastileGpgmeSubkey))
#define BASTILE_GPGME_SUBKEY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_GPGME_SUBKEY, BastileGpgmeSubkeyClass))
#define BASTILE_IS_GPGME_SUBKEY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_GPGME_SUBKEY))
#define BASTILE_IS_GPGME_SUBKEY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_GPGME_SUBKEY))
#define BASTILE_GPGME_SUBKEY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_GPGME_SUBKEY, BastileGpgmeSubkeyClass))

typedef struct _BastileGpgmeSubkey BastileGpgmeSubkey;
typedef struct _BastileGpgmeSubkeyClass BastileGpgmeSubkeyClass;
typedef struct _BastileGpgmeSubkeyPrivate BastileGpgmeSubkeyPrivate;

struct _BastileGpgmeSubkey {
	BastilePgpSubkey parent;
	BastileGpgmeSubkeyPrivate *pv;
};

struct _BastileGpgmeSubkeyClass {
	BastilePgpSubkeyClass parent_class;
};

GType                 bastile_gpgme_subkey_get_type          (void);

BastileGpgmeSubkey*  bastile_gpgme_subkey_new               (gpgme_key_t pubkey,
                                                               gpgme_subkey_t subkey);

gpgme_key_t           bastile_gpgme_subkey_get_pubkey        (BastileGpgmeSubkey *self);

gpgme_subkey_t        bastile_gpgme_subkey_get_subkey        (BastileGpgmeSubkey *self);

void                  bastile_gpgme_subkey_set_subkey        (BastileGpgmeSubkey *self,
                                                               gpgme_subkey_t subkey);

#endif /* __BASTILE_GPGME_SUBKEY_H__ */
