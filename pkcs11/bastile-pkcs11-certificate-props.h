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

#ifndef __BASTILE_PKCS11_CERTIFICATE_PROPS_H__
#define __BASTILE_PKCS11_CERTIFICATE_PROPS_H__

#include <gcr/gcr.h>

#include <gtk/gtk.h>

#include <glib-object.h>
#include <glib/gi18n.h>

#define BASTILE_TYPE_PKCS11_CERTIFICATE_PROPS               (bastile_pkcs11_certificate_props_get_type ())
#define BASTILE_PKCS11_CERTIFICATE_PROPS(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_PKCS11_CERTIFICATE_PROPS, BastilePkcs11CertificateProps))
#define BASTILE_PKCS11_CERTIFICATE_PROPS_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_PKCS11_CERTIFICATE_PROPS, BastilePkcs11CertificatePropsClass))
#define BASTILE_IS_PKCS11_CERTIFICATE_PROPS(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_PKCS11_CERTIFICATE_PROPS))
#define BASTILE_IS_PKCS11_CERTIFICATE_PROPS_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_PKCS11_CERTIFICATE_PROPS))
#define BASTILE_PKCS11_CERTIFICATE_PROPS_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_PKCS11_CERTIFICATE_PROPS, BastilePkcs11CertificatePropsClass))

typedef struct _BastilePkcs11CertificateProps BastilePkcs11CertificateProps;
typedef struct _BastilePkcs11CertificatePropsClass BastilePkcs11CertificatePropsClass;
typedef struct _BastilePkcs11CertificatePropsPrivate BastilePkcs11CertificatePropsPrivate;
    
struct _BastilePkcs11CertificateProps {
	GtkDialog parent;
};

struct _BastilePkcs11CertificatePropsClass {
	GtkDialogClass parent_class;
};

GType                      bastile_pkcs11_certificate_props_get_type               (void);

GtkDialog*                 bastile_pkcs11_certificate_props_new                    (GcrCertificate *cert);

void                       bastile_pkcs11_certificate_props_set_certificate        (BastilePkcs11CertificateProps *self, 
                                                                                     GcrCertificate *cert);

GcrCertificate*            bastile_pkcs11_certificate_props_get_certificate        (BastilePkcs11CertificateProps *self);

void                       bastile_pkcs11_certificate_props_add_view               (BastilePkcs11CertificateProps *self, 
                                                                                     const gchar *title, 
                                                                                     GtkWidget *view);

void                       bastile_pkcs11_certificate_props_insert_view            (BastilePkcs11CertificateProps *self, 
                                                                                     const gchar *title, 
                                                                                     GtkWidget *view, 
                                                                                     gint position);

void                       bastile_pkcs11_certificate_props_focus_view             (BastilePkcs11CertificateProps *self, 
                                                                                     GtkWidget *view);

void                       bastile_pkcs11_certificate_props_remove_view            (BastilePkcs11CertificateProps *self, 
                                                                                     GtkWidget *view);

#endif /* __BASTILE_PKCS11_CERTIFICATE_PROPS_H__ */
