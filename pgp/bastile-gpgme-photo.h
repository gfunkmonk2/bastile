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

#ifndef __BASTILE_GPGME_PHOTO_H__
#define __BASTILE_GPGME_PHOTO_H__

#include <glib-object.h>

#include <gpgme.h>

#include "pgp/bastile-gpgme.h"
#include "pgp/bastile-pgp-photo.h"

#define BASTILE_TYPE_GPGME_PHOTO            (bastile_gpgme_photo_get_type ())
#define BASTILE_GPGME_PHOTO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_GPGME_PHOTO, BastileGpgmePhoto))
#define BASTILE_GPGME_PHOTO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_GPGME_PHOTO, BastileGpgmePhotoClass))
#define BASTILE_IS_GPGME_PHOTO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_GPGME_PHOTO))
#define BASTILE_IS_GPGME_PHOTO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_GPGME_PHOTO))
#define BASTILE_GPGME_PHOTO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_GPGME_PHOTO, BastileGpgmePhotoClass))

typedef struct _BastileGpgmePhoto BastileGpgmePhoto;
typedef struct _BastileGpgmePhotoClass BastileGpgmePhotoClass;
typedef struct _BastileGpgmePhotoPrivate BastileGpgmePhotoPrivate;

struct _BastileGpgmePhoto {
	BastilePgpPhoto parent;
	BastileGpgmePhotoPrivate *pv;
};

struct _BastileGpgmePhotoClass {
	BastilePgpPhotoClass parent_class;
};

GType               bastile_gpgme_photo_get_type        (void);

BastileGpgmePhoto* bastile_gpgme_photo_new             (gpgme_key_t key,
                                                          GdkPixbuf *pixbuf,
                                                          guint index);

gpgme_key_t         bastile_gpgme_photo_get_pubkey      (BastileGpgmePhoto *self);

guint               bastile_gpgme_photo_get_index       (BastileGpgmePhoto *self);

void                bastile_gpgme_photo_set_index       (BastileGpgmePhoto *self,
                                                          guint index);

#endif /* __BASTILE_GPGME_PHOTO_H__ */
