/*
 * Bastile
 *
 * Copyright (C) 2010 Stefan Walter
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

#ifndef __BASTILE_SECURE_BUFFER_H__
#define __BASTILE_SECURE_BUFFER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BASTILE_TYPE_SECURE_BUFFER            (bastile_secure_buffer_get_type ())
#define BASTILE_SECURE_BUFFER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_SECURE_BUFFER, BastileSecureBuffer))
#define BASTILE_SECURE_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_SECURE_BUFFER, BastileSecureBufferClass))
#define BASTILE_IS_SECURE_BUFFER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_SECURE_BUFFER))
#define BASTILE_IS_SECURE_BUFFER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_SECURE_BUFFER))
#define BASTILE_SECURE_BUFFER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_SECURE_BUFFER, BastileSecureBufferClass))

typedef struct _BastileSecureBuffer            BastileSecureBuffer;
typedef struct _BastileSecureBufferClass       BastileSecureBufferClass;
typedef struct _BastileSecureBufferPrivate     BastileSecureBufferPrivate;

struct _BastileSecureBuffer
{
	GtkEntryBuffer parent;
	BastileSecureBufferPrivate *priv;
};

struct _BastileSecureBufferClass
{
	GtkEntryBufferClass parent_class;
};

GType                     bastile_secure_buffer_get_type               (void) G_GNUC_CONST;

GtkEntryBuffer*           bastile_secure_buffer_new                    (void);

G_END_DECLS

#endif /* __BASTILE_SECURE_BUFFER_H__ */
