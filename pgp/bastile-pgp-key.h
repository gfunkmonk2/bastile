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

#ifndef __BASTILE_PGP_KEY_H__
#define __BASTILE_PGP_KEY_H__

#include <glib-object.h>

#include "pgp/bastile-pgp-module.h"

#include "bastile-object.h"
#include "bastile-validity.h"

enum {
    SKEY_PGPSIG_TRUSTED = 0x0001,
    SKEY_PGPSIG_PERSONAL = 0x0002
};

#define BASTILE_TYPE_PGP_KEY            (bastile_pgp_key_get_type ())
#define BASTILE_PGP_KEY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_PGP_KEY, BastilePgpKey))
#define BASTILE_PGP_KEY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_PGP_KEY, BastilePgpKeyClass))
#define BASTILE_IS_PGP_KEY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_PGP_KEY))
#define BASTILE_IS_PGP_KEY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_PGP_KEY))
#define BASTILE_PGP_KEY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_PGP_KEY, BastilePgpKeyClass))


typedef struct _BastilePgpKey BastilePgpKey;
typedef struct _BastilePgpKeyClass BastilePgpKeyClass;
typedef struct _BastilePgpKeyPrivate BastilePgpKeyPrivate;

struct _BastilePgpKey {
	BastileObject parent;
	BastilePgpKeyPrivate *pv;
};

struct _BastilePgpKeyClass {
	BastileObjectClass parent_class;
	
	/* virtual methods */
	GList* (*get_uids) (BastilePgpKey *self);
	void   (*set_uids) (BastilePgpKey *self, GList *uids);
	
	GList* (*get_subkeys) (BastilePgpKey *self);
	void   (*set_subkeys) (BastilePgpKey *self, GList *uids);
	
	GList* (*get_photos) (BastilePgpKey *self);
	void   (*set_photos) (BastilePgpKey *self, GList *uids);
};

GType             bastile_pgp_key_get_type             (void);

BastilePgpKey*   bastile_pgp_key_new                  (void);

GList*            bastile_pgp_key_get_subkeys          (BastilePgpKey *self);

void              bastile_pgp_key_set_subkeys          (BastilePgpKey *self,
                                                         GList *subkeys);

GList*            bastile_pgp_key_get_uids             (BastilePgpKey *self);

void              bastile_pgp_key_set_uids             (BastilePgpKey *self,
                                                         GList *subkeys);

GList*            bastile_pgp_key_get_photos           (BastilePgpKey *self);

void              bastile_pgp_key_set_photos           (BastilePgpKey *self,
                                                         GList *subkeys);

const gchar*      bastile_pgp_key_get_fingerprint      (BastilePgpKey *self);

BastileValidity  bastile_pgp_key_get_validity         (BastilePgpKey *self);

const gchar*      bastile_pgp_key_get_validity_str     (BastilePgpKey *self);

gulong            bastile_pgp_key_get_expires          (BastilePgpKey *self);

gchar*            bastile_pgp_key_get_expires_str      (BastilePgpKey *self);

BastileValidity  bastile_pgp_key_get_trust            (BastilePgpKey *self);

const gchar*      bastile_pgp_key_get_trust_str        (BastilePgpKey *self);

guint             bastile_pgp_key_get_length           (BastilePgpKey *self);

const gchar*      bastile_pgp_key_get_algo             (BastilePgpKey *self);

const gchar*      bastile_pgp_key_get_keyid            (BastilePgpKey *self);

gboolean          bastile_pgp_key_has_keyid            (BastilePgpKey *self, 
                                                         const gchar *keyid);

gchar*            bastile_pgp_key_calc_identifier      (const gchar *keyid);

gchar*            bastile_pgp_key_calc_id              (const gchar *keyid,
                                                         guint index);

const gchar*      bastile_pgp_key_calc_rawid           (GQuark id);

GQuark            bastile_pgp_key_canonize_id          (const gchar *keyid);

#endif /* __BASTILE_KEY_H__ */
