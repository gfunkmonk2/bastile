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

#ifndef __BASTILE_PGP_UID_H__
#define __BASTILE_PGP_UID_H__

#include <glib-object.h>

#include "bastile-object.h"
#include "bastile-validity.h"

#define BASTILE_TYPE_PGP_UID            (bastile_pgp_uid_get_type ())

#define BASTILE_PGP_UID(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_PGP_UID, BastilePgpUid))
#define BASTILE_PGP_UID_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_PGP_UID, BastilePgpUidClass))
#define BASTILE_IS_PGP_UID(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_PGP_UID))
#define BASTILE_IS_PGP_UID_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_PGP_UID))
#define BASTILE_PGP_UID_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_PGP_UID, BastilePgpUidClass))

typedef struct _BastilePgpUid BastilePgpUid;
typedef struct _BastilePgpUidClass BastilePgpUidClass;
typedef struct _BastilePgpUidPrivate BastilePgpUidPrivate;

struct _BastilePgpUid {
	BastileObject parent;
	BastilePgpUidPrivate *pv;
};

struct _BastilePgpUidClass {
	BastileObjectClass parent_class;
};

GType             bastile_pgp_uid_get_type             (void);

BastilePgpUid*   bastile_pgp_uid_new                  (const gchar *uid_string);

GList*            bastile_pgp_uid_get_signatures       (BastilePgpUid *self);

void              bastile_pgp_uid_set_signatures       (BastilePgpUid *self,
                                                         GList *signatures);

BastileValidity  bastile_pgp_uid_get_validity         (BastilePgpUid *self);

void              bastile_pgp_uid_set_validity         (BastilePgpUid *self,
                                                         BastileValidity validity);

const gchar*      bastile_pgp_uid_get_validity_str     (BastilePgpUid *self);


const gchar*      bastile_pgp_uid_get_name             (BastilePgpUid *self);

void              bastile_pgp_uid_set_name             (BastilePgpUid *self,
                                                         const gchar *name);

const gchar*      bastile_pgp_uid_get_email            (BastilePgpUid *self);

void              bastile_pgp_uid_set_email            (BastilePgpUid *self,
                                                         const gchar *comment);

const gchar*      bastile_pgp_uid_get_comment          (BastilePgpUid *self);

void              bastile_pgp_uid_set_comment          (BastilePgpUid *self,
                                                         const gchar *comment);

gchar*            bastile_pgp_uid_calc_label           (const gchar *name,
                                                         const gchar *email,
                                                         const gchar *comment);

gchar*            bastile_pgp_uid_calc_markup          (const gchar *name,
                                                         const gchar *email,
                                                         const gchar *comment,
                                                         guint flags);

GQuark            bastile_pgp_uid_calc_id              (GQuark key_id,
                                                         guint index);

#endif /* __BASTILE_PGP_UID_H__ */
