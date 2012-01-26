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

#ifndef __BASTILE_GPGME_UID_H__
#define __BASTILE_GPGME_UID_H__

#include <glib-object.h>

#include <gpgme.h>

#include "bastile-object.h"

#include "pgp/bastile-pgp-uid.h"

#define BASTILE_TYPE_GPGME_UID            (bastile_gpgme_uid_get_type ())
#define BASTILE_GPGME_UID(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_GPGME_UID, BastileGpgmeUid))
#define BASTILE_GPGME_UID_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_GPGME_UID, BastileGpgmeUidClass))
#define BASTILE_IS_GPGME_UID(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_GPGME_UID))
#define BASTILE_IS_GPGME_UID_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_GPGME_UID))
#define BASTILE_GPGME_UID_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_GPGME_UID, BastileGpgmeUidClass))

typedef struct _BastileGpgmeUid BastileGpgmeUid;
typedef struct _BastileGpgmeUidClass BastileGpgmeUidClass;
typedef struct _BastileGpgmeUidPrivate BastileGpgmeUidPrivate;

struct _BastileGpgmeUid {
	BastilePgpUid parent;
	BastileGpgmeUidPrivate *pv;
};

struct _BastileGpgmeUidClass {
	BastilePgpUidClass parent_class;
};

GType               bastile_gpgme_uid_get_type             (void);

BastileGpgmeUid*   bastile_gpgme_uid_new                  (gpgme_key_t pubkey,
                                                             gpgme_user_id_t userid);

gpgme_key_t         bastile_gpgme_uid_get_pubkey           (BastileGpgmeUid *self);

gpgme_user_id_t     bastile_gpgme_uid_get_userid           (BastileGpgmeUid *self);

void                bastile_gpgme_uid_set_userid           (BastileGpgmeUid *self,
                                                             gpgme_user_id_t userid);

guint               bastile_gpgme_uid_get_gpgme_index      (BastileGpgmeUid *self);

guint               bastile_gpgme_uid_get_actual_index     (BastileGpgmeUid *self);

void                bastile_gpgme_uid_set_actual_index     (BastileGpgmeUid *self,
                                                             guint actual_index);

gchar*              bastile_gpgme_uid_calc_name            (gpgme_user_id_t userid);

gchar*              bastile_gpgme_uid_calc_label           (gpgme_user_id_t userid);

gchar*              bastile_gpgme_uid_calc_markup          (gpgme_user_id_t userid,
                                                             guint flags);

gboolean            bastile_gpgme_uid_is_same              (BastileGpgmeUid *self,
                                                             gpgme_user_id_t userid);

#endif /* __BASTILE_GPGME_UID_H__ */
