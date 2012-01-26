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

#ifndef __BASTILE_PGP_SUBKEY_H__
#define __BASTILE_PGP_SUBKEY_H__

#include <glib-object.h>

#include "bastile-object.h"

#define BASTILE_TYPE_PGP_SUBKEY            (bastile_pgp_subkey_get_type ())

#define BASTILE_PGP_SUBKEY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_PGP_SUBKEY, BastilePgpSubkey))
#define BASTILE_PGP_SUBKEY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_PGP_SUBKEY, BastilePgpSubkeyClass))
#define BASTILE_IS_PGP_SUBKEY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_PGP_SUBKEY))
#define BASTILE_IS_PGP_SUBKEY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_PGP_SUBKEY))
#define BASTILE_PGP_SUBKEY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_PGP_SUBKEY, BastilePgpSubkeyClass))

typedef struct _BastilePgpSubkey BastilePgpSubkey;
typedef struct _BastilePgpSubkeyClass BastilePgpSubkeyClass;
typedef struct _BastilePgpSubkeyPrivate BastilePgpSubkeyPrivate;

struct _BastilePgpSubkey {
	GObject parent;
	BastilePgpSubkeyPrivate *pv;
};

struct _BastilePgpSubkeyClass {
	GObjectClass parent_class;
};

GType               bastile_pgp_subkey_get_type          (void);

BastilePgpSubkey*  bastile_pgp_subkey_new               (void);

guint               bastile_pgp_subkey_get_index         (BastilePgpSubkey *self);

void                bastile_pgp_subkey_set_index         (BastilePgpSubkey *self,
                                                           guint index);

const gchar*        bastile_pgp_subkey_get_keyid         (BastilePgpSubkey *self);

void                bastile_pgp_subkey_set_keyid         (BastilePgpSubkey *self,
                                                           const gchar *keyid);

guint               bastile_pgp_subkey_get_flags         (BastilePgpSubkey *self);

void                bastile_pgp_subkey_set_flags         (BastilePgpSubkey *self,
                                                           guint flags);

const gchar*        bastile_pgp_subkey_get_algorithm     (BastilePgpSubkey *self);

void                bastile_pgp_subkey_set_algorithm     (BastilePgpSubkey *self,
                                                           const gchar *algorithm);

guint               bastile_pgp_subkey_get_length        (BastilePgpSubkey *self);

void                bastile_pgp_subkey_set_length        (BastilePgpSubkey *self,
                                                           guint index);

gulong              bastile_pgp_subkey_get_created       (BastilePgpSubkey *self);

void                bastile_pgp_subkey_set_created       (BastilePgpSubkey *self,
                                                           gulong created);

gulong              bastile_pgp_subkey_get_expires       (BastilePgpSubkey *self);

void                bastile_pgp_subkey_set_expires       (BastilePgpSubkey *self,
                                                           gulong expires);

const gchar*        bastile_pgp_subkey_get_description   (BastilePgpSubkey *self);

void                bastile_pgp_subkey_set_description   (BastilePgpSubkey *self,
                                                           const gchar *description);

gchar*              bastile_pgp_subkey_calc_description  (const gchar *name, 
                                                           guint index); 

const gchar*        bastile_pgp_subkey_get_fingerprint   (BastilePgpSubkey *self);

void                bastile_pgp_subkey_set_fingerprint   (BastilePgpSubkey *self,
                                                           const gchar *description);

gchar*              bastile_pgp_subkey_calc_fingerprint  (const gchar *raw_fingerprint); 


#endif /* __BASTILE_PGP_SUBKEY_H__ */
