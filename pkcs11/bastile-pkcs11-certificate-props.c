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

#include "config.h"

#include "bastile-pkcs11-certificate.h"
#include "bastile-pkcs11-certificate-props.h"

#include <gcr/gcr.h>

enum {
	PROP_0,
	PROP_CERTIFICATE
};

struct _BastilePkcs11CertificatePropsPrivate {
	GtkNotebook *tabs;
	GcrCertificateWidget *widget;
};

G_DEFINE_TYPE (BastilePkcs11CertificateProps, bastile_pkcs11_certificate_props, GTK_TYPE_DIALOG);

#define BASTILE_PKCS11_CERTIFICATE_PROPS_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), BASTILE_TYPE_PKCS11_CERTIFICATE_PROPS, BastilePkcs11CertificatePropsPrivate))

/* -----------------------------------------------------------------------------
 * INTERNAL 
 */

/* -----------------------------------------------------------------------------
 * OBJECT 
 */


static void
bastile_pkcs11_certificate_props_init (BastilePkcs11CertificateProps *self)
{
	BastilePkcs11CertificatePropsPrivate *pv = BASTILE_PKCS11_CERTIFICATE_PROPS_GET_PRIVATE (self);
	GtkWidget *button;
	
	pv->tabs = GTK_NOTEBOOK (gtk_notebook_new ());
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (self))), GTK_WIDGET (pv->tabs));
	gtk_container_set_border_width (GTK_CONTAINER (self), 5);
	gtk_container_set_border_width (GTK_CONTAINER (pv->tabs), 5);
	gtk_widget_show (GTK_WIDGET (pv->tabs));
	
	pv->widget = gcr_certificate_widget_new (NULL);
	bastile_pkcs11_certificate_props_add_view (self, _("Certificate"), GTK_WIDGET (pv->widget));
	gtk_widget_show (GTK_WIDGET (pv->widget));
	
	button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	gtk_dialog_add_action_widget (GTK_DIALOG (self), button, GTK_RESPONSE_CLOSE);
	gtk_widget_show (button);
}

static void
bastile_pkcs11_certificate_props_dispose (GObject *obj)
{
	BastilePkcs11CertificateProps *self = BASTILE_PKCS11_CERTIFICATE_PROPS (obj);
	BastilePkcs11CertificatePropsPrivate *pv = BASTILE_PKCS11_CERTIFICATE_PROPS_GET_PRIVATE (self);
	
	if (pv->widget) {
		bastile_pkcs11_certificate_props_remove_view (self, GTK_WIDGET (pv->widget));
		pv->widget = NULL;
	}
    
	G_OBJECT_CLASS (bastile_pkcs11_certificate_props_parent_class)->dispose (obj);
}

static void
bastile_pkcs11_certificate_props_finalize (GObject *obj)
{
	BastilePkcs11CertificateProps *self = BASTILE_PKCS11_CERTIFICATE_PROPS (obj);
	BastilePkcs11CertificatePropsPrivate *pv = BASTILE_PKCS11_CERTIFICATE_PROPS_GET_PRIVATE (self);

	g_assert (pv->widget == NULL);
	
	G_OBJECT_CLASS (bastile_pkcs11_certificate_props_parent_class)->finalize (obj);
}

static void
bastile_pkcs11_certificate_props_set_property (GObject *obj, guint prop_id, const GValue *value, 
                           GParamSpec *pspec)
{
	BastilePkcs11CertificateProps *self = BASTILE_PKCS11_CERTIFICATE_PROPS (obj);
	
	switch (prop_id) {
	case PROP_CERTIFICATE:
		bastile_pkcs11_certificate_props_set_certificate (self, g_value_get_object (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
bastile_pkcs11_certificate_props_get_property (GObject *obj, guint prop_id, GValue *value, 
                           GParamSpec *pspec)
{
	BastilePkcs11CertificateProps *self = BASTILE_PKCS11_CERTIFICATE_PROPS (obj);
	
	switch (prop_id) {
	case PROP_CERTIFICATE:
		g_value_set_object (value, bastile_pkcs11_certificate_props_get_certificate (self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
bastile_pkcs11_certificate_props_class_init (BastilePkcs11CertificatePropsClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	
	bastile_pkcs11_certificate_props_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (BastilePkcs11CertificatePropsPrivate));

	gobject_class->dispose = bastile_pkcs11_certificate_props_dispose;
	gobject_class->finalize = bastile_pkcs11_certificate_props_finalize;
	gobject_class->set_property = bastile_pkcs11_certificate_props_set_property;
	gobject_class->get_property = bastile_pkcs11_certificate_props_get_property;

	g_object_class_install_property (gobject_class, PROP_CERTIFICATE,
	           g_param_spec_object ("certificate", "Certificate", "Certificate to display", 
	                                BASTILE_PKCS11_TYPE_CERTIFICATE, G_PARAM_READWRITE));
}

/* -----------------------------------------------------------------------------
 * PUBLIC 
 */

GtkDialog*
bastile_pkcs11_certificate_props_new (GcrCertificate *cert)
{
	return g_object_new (BASTILE_TYPE_PKCS11_CERTIFICATE_PROPS, "certificate", cert, NULL);
}

void
bastile_pkcs11_certificate_props_set_certificate (BastilePkcs11CertificateProps *self, GcrCertificate *cert)
{
	BastilePkcs11CertificatePropsPrivate *pv = BASTILE_PKCS11_CERTIFICATE_PROPS_GET_PRIVATE (self);
	g_return_if_fail (BASTILE_IS_PKCS11_CERTIFICATE_PROPS (self));
	
	gcr_certificate_widget_set_certificate (pv->widget, cert);
	g_object_notify (G_OBJECT (self), "certificate");
}

GcrCertificate*
bastile_pkcs11_certificate_props_get_certificate (BastilePkcs11CertificateProps *self)
{
	BastilePkcs11CertificatePropsPrivate *pv = BASTILE_PKCS11_CERTIFICATE_PROPS_GET_PRIVATE (self);
	g_return_val_if_fail (BASTILE_IS_PKCS11_CERTIFICATE_PROPS (self), NULL);
	
	return gcr_certificate_widget_get_certificate (pv->widget);
}

void
bastile_pkcs11_certificate_props_add_view (BastilePkcs11CertificateProps *self, const gchar *title, GtkWidget *view)
{
	g_return_if_fail (BASTILE_IS_PKCS11_CERTIFICATE_PROPS (self));
	bastile_pkcs11_certificate_props_insert_view (self, title, view, -1);
}

void
bastile_pkcs11_certificate_props_insert_view (BastilePkcs11CertificateProps *self, const gchar *title, 
                                               GtkWidget *view, gint position)
{
	BastilePkcs11CertificatePropsPrivate *pv = BASTILE_PKCS11_CERTIFICATE_PROPS_GET_PRIVATE (self);
	
	g_return_if_fail (BASTILE_IS_PKCS11_CERTIFICATE_PROPS (self));
	g_return_if_fail (title);
	
	g_return_if_fail (GTK_IS_WIDGET (view));
	g_return_if_fail (gtk_notebook_page_num (pv->tabs, view) == -1);
	
	gtk_notebook_insert_page (pv->tabs, view, gtk_label_new (title), position);
}

void
bastile_pkcs11_certificate_props_focus_view (BastilePkcs11CertificateProps *self, GtkWidget *view)
{
	BastilePkcs11CertificatePropsPrivate *pv = BASTILE_PKCS11_CERTIFICATE_PROPS_GET_PRIVATE (self);
	gint page;
	
	g_return_if_fail (BASTILE_IS_PKCS11_CERTIFICATE_PROPS (self));
	g_return_if_fail (GTK_IS_WIDGET (view));
	
	page = gtk_notebook_page_num (pv->tabs, view);
	g_return_if_fail (page != -1);
	
	gtk_notebook_set_current_page (pv->tabs, page);
}

void
bastile_pkcs11_certificate_props_remove_view (BastilePkcs11CertificateProps *self, GtkWidget *view)
{
	BastilePkcs11CertificatePropsPrivate *pv = BASTILE_PKCS11_CERTIFICATE_PROPS_GET_PRIVATE (self);
	gint page;
	
	g_return_if_fail (BASTILE_IS_PKCS11_CERTIFICATE_PROPS (self));
	g_return_if_fail (GTK_IS_WIDGET (view));
	
	page = gtk_notebook_page_num (pv->tabs, view);
	g_return_if_fail (page != -1);
	
	gtk_notebook_remove_page (pv->tabs, page);
}
