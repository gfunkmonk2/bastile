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

#ifndef __BASTILE_PKCS11_CERTIFICATE_H__
#define __BASTILE_PKCS11_CERTIFICATE_H__

#include <gck/gck.h>

#include <glib-object.h>

#include "bastile-pkcs11-object.h"

#define BASTILE_PKCS11_TYPE_CERTIFICATE               (bastile_pkcs11_certificate_get_type ())
#define BASTILE_PKCS11_CERTIFICATE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_PKCS11_TYPE_CERTIFICATE, BastilePkcs11Certificate))
#define BASTILE_PKCS11_CERTIFICATE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_PKCS11_TYPE_CERTIFICATE, BastilePkcs11CertificateClass))
#define BASTILE_PKCS11_IS_CERTIFICATE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_PKCS11_TYPE_CERTIFICATE))
#define BASTILE_PKCS11_IS_CERTIFICATE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_PKCS11_TYPE_CERTIFICATE))
#define BASTILE_PKCS11_CERTIFICATE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_PKCS11_TYPE_CERTIFICATE, BastilePkcs11CertificateClass))

typedef struct _BastilePkcs11Certificate BastilePkcs11Certificate;
typedef struct _BastilePkcs11CertificateClass BastilePkcs11CertificateClass;
typedef struct _BastilePkcs11CertificatePrivate BastilePkcs11CertificatePrivate;
    
struct _BastilePkcs11Certificate {
	BastilePkcs11Object parent;
	BastilePkcs11CertificatePrivate *pv;
};

struct _BastilePkcs11CertificateClass {
	BastilePkcs11ObjectClass parent_class;
};

GType                       bastile_pkcs11_certificate_get_type               (void);

BastilePkcs11Certificate*  bastile_pkcs11_certificate_new                    (GckObject* object);

gchar*                      bastile_pkcs11_certificate_get_fingerprint        (BastilePkcs11Certificate* self);

guint                       bastile_pkcs11_certificate_get_validity           (BastilePkcs11Certificate* self);

const gchar*                bastile_pkcs11_certificate_get_validity_str       (BastilePkcs11Certificate* self);

guint                       bastile_pkcs11_certificate_get_trust              (BastilePkcs11Certificate* self);

const gchar*                bastile_pkcs11_certificate_get_trust_str          (BastilePkcs11Certificate* self);

gulong                      bastile_pkcs11_certificate_get_expires            (BastilePkcs11Certificate* self);

gchar*                      bastile_pkcs11_certificate_get_expires_str        (BastilePkcs11Certificate* self);

#endif /* __BASTILE_PKCS11_CERTIFICATE_H__ */
