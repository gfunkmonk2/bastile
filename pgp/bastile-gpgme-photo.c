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

#include "config.h"

#include "bastile-gpgme.h"
#include "bastile-gpgme-photo.h"

#include <string.h>

#include <glib/gi18n.h>

enum {
	PROP_0,
	PROP_PUBKEY,
	PROP_INDEX
};

G_DEFINE_TYPE (BastileGpgmePhoto, bastile_gpgme_photo, BASTILE_TYPE_PGP_PHOTO);

struct _BastileGpgmePhotoPrivate {
	gpgme_key_t pubkey;        /* Key that this photo is on */
	guint index;               /* The GPGME index of the photo */
};

/* -----------------------------------------------------------------------------
 * OBJECT 
 */

static void
bastile_gpgme_photo_init (BastileGpgmePhoto *self)
{
	self->pv = G_TYPE_INSTANCE_GET_PRIVATE (self, BASTILE_TYPE_GPGME_PHOTO, BastileGpgmePhotoPrivate);
}

static GObject*
bastile_gpgme_photo_constructor (GType type, guint n_props, GObjectConstructParam *props)
{
	GObject *obj = G_OBJECT_CLASS (bastile_gpgme_photo_parent_class)->constructor (type, n_props, props);
	BastileGpgmePhoto *self = NULL;
	
	if (obj) {
		self = BASTILE_GPGME_PHOTO (obj);
		g_return_val_if_fail (self->pv->pubkey, NULL);
	}
	
	return obj;
}

static void
bastile_gpgme_photo_get_property (GObject *object, guint prop_id,
                                  GValue *value, GParamSpec *pspec)
{
	BastileGpgmePhoto *self = BASTILE_GPGME_PHOTO (object);
	
	switch (prop_id) {
	case PROP_PUBKEY:
		g_value_set_boxed (value, bastile_gpgme_photo_get_pubkey (self));
		break;
	case PROP_INDEX:
		g_value_set_uint (value, bastile_gpgme_photo_get_index (self));
		break;
	}
}

static void
bastile_gpgme_photo_set_property (GObject *object, guint prop_id, const GValue *value, 
                                  GParamSpec *pspec)
{
	BastileGpgmePhoto *self = BASTILE_GPGME_PHOTO (object);

	switch (prop_id) {
	case PROP_PUBKEY:
		g_return_if_fail (!self->pv->pubkey);
		self->pv->pubkey = g_value_get_boxed (value);
		if (self->pv->pubkey)
			gpgme_key_ref (self->pv->pubkey);
		break;
	case PROP_INDEX:
		bastile_gpgme_photo_set_index (self, g_value_get_uint (value));
		break;
	}
}

static void
bastile_gpgme_photo_finalize (GObject *gobject)
{
	BastileGpgmePhoto *self = BASTILE_GPGME_PHOTO (gobject);

	if (self->pv->pubkey)
		gpgme_key_unref (self->pv->pubkey);
	self->pv->pubkey = NULL;
    
	G_OBJECT_CLASS (bastile_gpgme_photo_parent_class)->finalize (gobject);
}

static void
bastile_gpgme_photo_class_init (BastileGpgmePhotoClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);    

	bastile_gpgme_photo_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (BastileGpgmePhotoPrivate));
    
	gobject_class->constructor = bastile_gpgme_photo_constructor;
	gobject_class->finalize = bastile_gpgme_photo_finalize;
	gobject_class->set_property = bastile_gpgme_photo_set_property;
	gobject_class->get_property = bastile_gpgme_photo_get_property;
    
	g_object_class_install_property (gobject_class, PROP_PUBKEY,
	        g_param_spec_boxed ("pubkey", "Public Key", "GPGME Public Key that this subkey is on",
	                            BASTILE_GPGME_BOXED_KEY, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (gobject_class, PROP_INDEX,
	        g_param_spec_uint ("index", "Index", "Index of photo UID",
	                           0, G_MAXUINT, 0, G_PARAM_READWRITE));
}

/* -----------------------------------------------------------------------------
 * PUBLIC 
 */

BastileGpgmePhoto* 
bastile_gpgme_photo_new (gpgme_key_t pubkey, GdkPixbuf *pixbuf, guint index) 
{
	return g_object_new (BASTILE_TYPE_GPGME_PHOTO,
	                     "pubkey", pubkey,
	                     "pixbuf", pixbuf, 
	                     "index", index, NULL);
}

gpgme_key_t
bastile_gpgme_photo_get_pubkey (BastileGpgmePhoto *self)
{
	g_return_val_if_fail (BASTILE_IS_GPGME_PHOTO (self), NULL);
	g_return_val_if_fail (self->pv->pubkey, NULL);
	return self->pv->pubkey;
}

guint
bastile_gpgme_photo_get_index (BastileGpgmePhoto *self)
{
	g_return_val_if_fail (BASTILE_IS_GPGME_PHOTO (self), 0);
	return self->pv->index;
}

void
bastile_gpgme_photo_set_index (BastileGpgmePhoto *self, guint index)
{
	g_return_if_fail (BASTILE_IS_GPGME_PHOTO (self));

	self->pv->index = index;
	g_object_notify (G_OBJECT (self), "index");
}
