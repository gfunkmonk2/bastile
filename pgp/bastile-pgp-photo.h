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

#ifndef __BASTILE_PGP_PHOTO_H__
#define __BASTILE_PGP_PHOTO_H__

#include <glib-object.h>
#include <gtk/gtk.h>

#define BASTILE_TYPE_PGP_PHOTO            (bastile_pgp_photo_get_type ())
#define BASTILE_PGP_PHOTO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_PGP_PHOTO, BastilePgpPhoto))
#define BASTILE_PGP_PHOTO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_PGP_PHOTO, BastilePgpPhotoClass))
#define BASTILE_IS_PGP_PHOTO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_PGP_PHOTO))
#define BASTILE_IS_PGP_PHOTO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_PGP_PHOTO))
#define BASTILE_PGP_PHOTO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_PGP_PHOTO, BastilePgpPhotoClass))

typedef struct _BastilePgpPhoto BastilePgpPhoto;
typedef struct _BastilePgpPhotoClass BastilePgpPhotoClass;
typedef struct _BastilePgpPhotoPrivate BastilePgpPhotoPrivate;

struct _BastilePgpPhoto {
	GObject parent;
	BastilePgpPhotoPrivate *pv;
};

struct _BastilePgpPhotoClass {
	GObjectClass parent_class;
};

GType               bastile_pgp_photo_get_type          (void);

BastilePgpPhoto*   bastile_pgp_photo_new               (GdkPixbuf *pixbuf);

GdkPixbuf*          bastile_pgp_photo_get_pixbuf        (BastilePgpPhoto *self);

void                bastile_pgp_photo_set_pixbuf        (BastilePgpPhoto *self,
                                                          GdkPixbuf *pixbuf);

#endif /* __BASTILE_PGP_PHOTO_H__ */
