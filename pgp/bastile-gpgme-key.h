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

#ifndef __BASTILE_GPGME_KEY_H__
#define __BASTILE_GPGME_KEY_H__

#include <glib-object.h>

#include <gpgme.h>

#include "bastile-pgp-key.h"

#define BASTILE_TYPE_GPGME_KEY            (bastile_gpgme_key_get_type ())
#define BASTILE_GPGME_KEY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_GPGME_KEY, BastileGpgmeKey))
#define BASTILE_GPGME_KEY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_GPGME_KEY, BastileGpgmeKeyClass))
#define BASTILE_IS_GPGME_KEY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_GPGME_KEY))
#define BASTILE_IS_GPGME_KEY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_GPGME_KEY))
#define BASTILE_GPGME_KEY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_GPGME_KEY, BastileGpgmeKeyClass))


typedef struct _BastileGpgmeKey BastileGpgmeKey;
typedef struct _BastileGpgmeKeyClass BastileGpgmeKeyClass;
typedef struct _BastileGpgmeKeyPrivate BastileGpgmeKeyPrivate;

struct _BastileGpgmeKey {
	BastilePgpKey parent;
	BastileGpgmeKeyPrivate *pv;
};

struct _BastileGpgmeKeyClass {
	BastilePgpKeyClass         parent_class;
};

BastileGpgmeKey* bastile_gpgme_key_new                 (BastileSource *sksrc,
                                                          gpgme_key_t pubkey,
                                                          gpgme_key_t seckey);

GType             bastile_gpgme_key_get_type             (void);

gpgme_key_t       bastile_gpgme_key_get_public           (BastileGpgmeKey *self);

void              bastile_gpgme_key_set_public           (BastileGpgmeKey *self,
                                                           gpgme_key_t key);

gpgme_key_t       bastile_gpgme_key_get_private          (BastileGpgmeKey *self);

void              bastile_gpgme_key_set_private          (BastileGpgmeKey *self,
                                                           gpgme_key_t key);

void              bastile_gpgme_key_refresh_matching     (gpgme_key_t key);

BastileValidity  bastile_gpgme_key_get_validity         (BastileGpgmeKey *self);

BastileValidity  bastile_gpgme_key_get_trust            (BastileGpgmeKey *self);

#endif /* __BASTILE_KEY_H__ */
