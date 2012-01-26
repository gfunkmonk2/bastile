/*
 * Bastile
 *
 * Copyright (C) 2003 Jacob Perkins
 * Copyright (C) 2004-2005 Stefan Walter
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

/*
 * Various UI elements and dialogs used in pgp component.
 */
 
#ifndef __BASTILE_GPGME_DIALOGS_H__
#define __BASTILE_GPGME_DIALOGS_H__

#include <gtk/gtk.h>

#include "pgp/bastile-gpgme-key.h"
#include "pgp/bastile-gpgme-photo.h"
#include "pgp/bastile-gpgme-subkey.h"
#include "pgp/bastile-gpgme-source.h"
#include "pgp/bastile-gpgme-uid.h"
#include "libbastile/bastile-widget.h"

void            bastile_gpgme_sign_prompt         (BastileGpgmeKey *key,
                                                    GtkWindow *parent);

void            bastile_gpgme_sign_prompt_uid     (BastileGpgmeUid *uid,
                                                    GtkWindow *parent);

void            bastile_gpgme_generate_register    (void);

void            bastile_gpgme_generate_show        (BastileGpgmeSource *sksrc,
                                                     GtkWindow *parent,
                                                     const char * name,
                                                     const char *email,
                                                     const gchar *comment);

void            bastile_gpgme_generate_key         (BastileGpgmeSource *sksrc,
                                                     const gchar *name,
                                                     const gchar *email,
                                                     const gchar *comment,
                                                     guint type,
                                                     guint bits,
                                                     time_t expires);

void            bastile_gpgme_add_revoker_new      (BastileGpgmeKey *pkey,
                                                     GtkWindow *parent);

void            bastile_gpgme_expires_new          (BastileGpgmeSubkey *subkey,
                                                     GtkWindow *parent);

void            bastile_gpgme_add_subkey_new       (BastileGpgmeKey *pkey,
                                                     GtkWindow *parent);

void            bastile_gpgme_add_uid_new          (BastileGpgmeKey *pkey,
                                                     GtkWindow *parent);

void            bastile_gpgme_revoke_new           (BastileGpgmeSubkey *subkey,
                                                     GtkWindow *parent);

gboolean        bastile_gpgme_photo_add            (BastileGpgmeKey *pkey, 
                                                     GtkWindow *parent,
                                                     const gchar *path);
                                         
gboolean        bastile_gpgme_photo_delete         (BastileGpgmePhoto *photo,
                                                     GtkWindow *parent);

#endif /* __BASTILE_GPGME_DIALOGS_H__ */
