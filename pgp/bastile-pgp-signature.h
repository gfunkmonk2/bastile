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

#ifndef __BASTILE_PGP_SIGNATURE_H__
#define __BASTILE_PGP_SIGNATURE_H__

#include <glib-object.h>

#include "pgp/bastile-pgp-module.h"

#include "bastile-validity.h"

#define BASTILE_TYPE_PGP_SIGNATURE            (bastile_pgp_signature_get_type ())

#define BASTILE_PGP_SIGNATURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_PGP_SIGNATURE, BastilePgpSignature))
#define BASTILE_PGP_SIGNATURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_PGP_SIGNATURE, BastilePgpSignatureClass))
#define BASTILE_IS_PGP_SIGNATURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_PGP_SIGNATURE))
#define BASTILE_IS_PGP_SIGNATURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_PGP_SIGNATURE))
#define BASTILE_PGP_SIGNATURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_PGP_SIGNATURE, BastilePgpSignatureClass))

typedef struct _BastilePgpSignature BastilePgpSignature;
typedef struct _BastilePgpSignatureClass BastilePgpSignatureClass;
typedef struct _BastilePgpSignaturePrivate BastilePgpSignaturePrivate;

struct _BastilePgpSignature {
	GObject parent;
	BastilePgpSignaturePrivate *pv;
};

struct _BastilePgpSignatureClass {
	GObjectClass parent_class;
};

GType                  bastile_pgp_signature_get_type       (void);

BastilePgpSignature*  bastile_pgp_signature_new            (const gchar *keyid);

const gchar*           bastile_pgp_signature_get_keyid      (BastilePgpSignature *self);

void                   bastile_pgp_signature_set_keyid      (BastilePgpSignature *self,
                                                              const gchar *keyid);

guint                  bastile_pgp_signature_get_flags      (BastilePgpSignature *self);

void                   bastile_pgp_signature_set_flags      (BastilePgpSignature *self,
                                                              guint flags);

guint                  bastile_pgp_signature_get_sigtype    (BastilePgpSignature *self);

#endif /* __BASTILE_PGP_SIGNATURE_H__ */
