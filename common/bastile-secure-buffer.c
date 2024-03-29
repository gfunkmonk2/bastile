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

#include "config.h"

#include "bastile-secure-buffer.h"
#include <mate-keyring-memory.h>

#include <string.h>

/* Initial size of buffer, in bytes */
#define MIN_SIZE 16

struct _BastileSecureBufferPrivate
{
	gchar *text;
	gsize text_size;
	gsize text_bytes;
	guint text_chars;
};

G_DEFINE_TYPE (BastileSecureBuffer, bastile_secure_buffer, GTK_TYPE_ENTRY_BUFFER);

/* --------------------------------------------------------------------------------
 * SECURE IMPLEMENTATIONS OF TEXT BUFFER
 */

static const gchar*
bastile_secure_buffer_real_get_text (GtkEntryBuffer *buffer, gsize *n_bytes)
{
	BastileSecureBuffer *self = BASTILE_SECURE_BUFFER (buffer);
	if (n_bytes)
		*n_bytes = self->priv->text_bytes;
	if (!self->priv->text)
		return "";
	return self->priv->text;
}

static guint
bastile_secure_buffer_real_get_length (GtkEntryBuffer *buffer)
{
	BastileSecureBuffer *self = BASTILE_SECURE_BUFFER (buffer);
	return self->priv->text_chars;
}

static guint
bastile_secure_buffer_real_insert_text (GtkEntryBuffer *buffer, guint position,
                                         const gchar *chars, guint n_chars)
{
	BastileSecureBuffer *self = BASTILE_SECURE_BUFFER (buffer);
	BastileSecureBufferPrivate *pv = self->priv;
	gsize n_bytes;
	gsize at;

	n_bytes = g_utf8_offset_to_pointer (chars, n_chars) - chars;

	/* Need more memory */
	if (n_bytes + pv->text_bytes + 1 > pv->text_size) {

		/* Calculate our new buffer size */
		while (n_bytes + pv->text_bytes + 1 > pv->text_size) {
			if (pv->text_size == 0) {
				pv->text_size = MIN_SIZE;
			} else {
				if (2 * pv->text_size < GTK_ENTRY_BUFFER_MAX_SIZE) {
					pv->text_size *= 2;
				} else {
					pv->text_size = GTK_ENTRY_BUFFER_MAX_SIZE;
					if (n_bytes > pv->text_size - pv->text_bytes - 1) {
						n_bytes = pv->text_size - pv->text_bytes - 1;
						n_bytes = g_utf8_find_prev_char (chars, chars + n_bytes + 1) - chars;
						n_chars = g_utf8_strlen (chars, n_bytes);
					}
					break;
				}
			}
		}

		pv->text = mate_keyring_memory_realloc (pv->text, pv->text_size);
	}

	/* Actual text insertion */
	at = g_utf8_offset_to_pointer (pv->text, position) - pv->text;
	g_memmove (pv->text + at + n_bytes, pv->text + at, pv->text_bytes - at);
	memcpy (pv->text + at, chars, n_bytes);

	/* Book keeping */
	pv->text_bytes += n_bytes;
	pv->text_chars += n_chars;
	pv->text[pv->text_bytes] = '\0';

	gtk_entry_buffer_emit_inserted_text (buffer, position, chars, n_chars);
	return n_chars;
}

static guint
bastile_secure_buffer_real_delete_text (GtkEntryBuffer *buffer, guint position, guint n_chars)
{
	BastileSecureBuffer *self = BASTILE_SECURE_BUFFER (buffer);
	BastileSecureBufferPrivate *pv = self->priv;
	gsize start, end;

	if (position > pv->text_chars)
		position = pv->text_chars;
	if (position + n_chars > pv->text_chars)
		n_chars = pv->text_chars - position;

	if (n_chars > 0) {
		start = g_utf8_offset_to_pointer (pv->text, position) - pv->text;
		end = g_utf8_offset_to_pointer (pv->text, position + n_chars) - pv->text;

		g_memmove (pv->text + start, pv->text + end, pv->text_bytes + 1 - end);
		pv->text_chars -= n_chars;
		pv->text_bytes -= (end - start);

		gtk_entry_buffer_emit_deleted_text (buffer, position, n_chars);
	}

	return n_chars;
}

/* --------------------------------------------------------------------------------
 *
 */

static void
bastile_secure_buffer_init (BastileSecureBuffer *self)
{
	BastileSecureBufferPrivate *pv;
	pv = self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, BASTILE_TYPE_SECURE_BUFFER, BastileSecureBufferPrivate);

	pv->text = NULL;
	pv->text_chars = 0;
	pv->text_bytes = 0;
	pv->text_size = 0;
}

static void
bastile_secure_buffer_finalize (GObject *obj)
{
	BastileSecureBuffer *self = BASTILE_SECURE_BUFFER (obj);
	BastileSecureBufferPrivate *pv = self->priv;

	if (pv->text) {
		mate_keyring_memory_free (pv->text);
		pv->text = NULL;
		pv->text_bytes = pv->text_size = 0;
		pv->text_chars = 0;
	}

	G_OBJECT_CLASS (bastile_secure_buffer_parent_class)->finalize (obj);
}

static void
bastile_secure_buffer_class_init (BastileSecureBufferClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GtkEntryBufferClass *buffer_class = GTK_ENTRY_BUFFER_CLASS (klass);

	gobject_class->finalize = bastile_secure_buffer_finalize;

	buffer_class->get_text = bastile_secure_buffer_real_get_text;
	buffer_class->get_length = bastile_secure_buffer_real_get_length;
	buffer_class->insert_text = bastile_secure_buffer_real_insert_text;
	buffer_class->delete_text = bastile_secure_buffer_real_delete_text;

	g_type_class_add_private (gobject_class, sizeof (BastileSecureBufferPrivate));
}

/* --------------------------------------------------------------------------------
 *
 */

GtkEntryBuffer*
bastile_secure_buffer_new (void)
{
	return g_object_new (BASTILE_TYPE_SECURE_BUFFER, NULL);
}
