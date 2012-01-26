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

#include "bastile-pgp.h"
#include "bastile-pgp-photo.h"

#include <string.h>

#include <glib/gi18n.h>

enum {
	PROP_0,
	PROP_PIXBUF
};

G_DEFINE_TYPE (BastilePgpPhoto, bastile_pgp_photo, G_TYPE_OBJECT);

struct _BastilePgpPhotoPrivate {
	GdkPixbuf *pixbuf;         
};

/* -----------------------------------------------------------------------------
 * OBJECT 
 */

static void
bastile_pgp_photo_init (BastilePgpPhoto *self)
{
	self->pv = G_TYPE_INSTANCE_GET_PRIVATE (self, BASTILE_TYPE_PGP_PHOTO, BastilePgpPhotoPrivate);
}

static void
bastile_pgp_photo_get_property (GObject *object, guint prop_id,
                                  GValue *value, GParamSpec *pspec)
{
	BastilePgpPhoto *self = BASTILE_PGP_PHOTO (object);
	
	switch (prop_id) {
	case PROP_PIXBUF:
		g_value_set_object (value, bastile_pgp_photo_get_pixbuf (self));
		break;
	}
}

static void
bastile_pgp_photo_set_property (GObject *object, guint prop_id, const GValue *value, 
                                  GParamSpec *pspec)
{
	BastilePgpPhoto *self = BASTILE_PGP_PHOTO (object);

	switch (prop_id) {
	case PROP_PIXBUF:
		bastile_pgp_photo_set_pixbuf (self, g_value_get_object (value));
		break;
	}
}

static void
bastile_pgp_photo_finalize (GObject *gobject)
{
	BastilePgpPhoto *self = BASTILE_PGP_PHOTO (gobject);

	if (self->pv->pixbuf)
		g_object_unref (self->pv->pixbuf);
	self->pv->pixbuf = NULL;
    
	G_OBJECT_CLASS (bastile_pgp_photo_parent_class)->finalize (gobject);
}

static void
bastile_pgp_photo_class_init (BastilePgpPhotoClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);    

	bastile_pgp_photo_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (BastilePgpPhotoPrivate));
    
	gobject_class->finalize = bastile_pgp_photo_finalize;
	gobject_class->set_property = bastile_pgp_photo_set_property;
	gobject_class->get_property = bastile_pgp_photo_get_property;
    
	g_object_class_install_property (gobject_class, PROP_PIXBUF,
	        g_param_spec_object ("pixbuf", "Pixbuf", "Photo Pixbuf",
	                             GDK_TYPE_PIXBUF, G_PARAM_READWRITE));
}

/* -----------------------------------------------------------------------------
 * PUBLIC 
 */

BastilePgpPhoto* 
bastile_pgp_photo_new (GdkPixbuf *pixbuf) 
{
	return g_object_new (BASTILE_TYPE_PGP_PHOTO, "pixbuf", pixbuf, NULL);
}

GdkPixbuf*
bastile_pgp_photo_get_pixbuf (BastilePgpPhoto *self)
{
	g_return_val_if_fail (BASTILE_IS_PGP_PHOTO (self), NULL);
	return self->pv->pixbuf;
}

void
bastile_pgp_photo_set_pixbuf (BastilePgpPhoto *self, GdkPixbuf* pixbuf)
{
	g_return_if_fail (BASTILE_IS_PGP_PHOTO (self));

	if (self->pv->pixbuf)
		g_object_unref (self->pv->pixbuf);
	self->pv->pixbuf = pixbuf;
	if (self->pv->pixbuf)
		g_object_ref (self->pv->pixbuf);
	
	g_object_notify (G_OBJECT (self), "pixbuf");
}
