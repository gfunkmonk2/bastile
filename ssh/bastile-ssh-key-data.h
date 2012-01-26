/*
 * Bastile
 *
 * Copyright (C) 2006 Stefan Walter
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

/**
 * BastileSSHKeyData: The data in an SSH key
 */
 
#ifndef __BASTILE_SSH_KEY_DATA_H__
#define __BASTILE_SSH_KEY_DATA_H__

#include <gtk/gtk.h>

/* The various algorithm types */
enum {
    SSH_ALGO_UNK,
    SSH_ALGO_RSA,
    SSH_ALGO_DSA
};

/* 
 * We prefix SSH keys that come from bastile with a unique 
 * prefix. Otherwise SSH private keys look like any other 
 * PEM encoded 'x509' keys. 
 */
#define SSH_KEY_SECRET_SIG "# SSH PRIVATE KEY: "


struct _BastileSSHKeyData {
    /* Used by callers */
    gchar *pubfile;         /* The public key file */
    gboolean partial;       /* Only part of the public key file */
    gchar *privfile;        /* The secret key file */
    
    /* Filled in by parser */
    gchar *rawdata;         /* The raw data of the public key */
    gchar *comment;         /* The comment for the public key */
    gchar *fingerprint;     /* The full fingerprint hash */
    guint length;           /* Number of bits */
    guint algo;             /* Key algorithm */
    gboolean authorized;    /* Is in authorized_keys */
};

struct _BastileSSHSecData {
    gchar *rawdata;
    gchar *comment;
    guint algo;
};

typedef struct _BastileSSHKeyData BastileSSHKeyData;
typedef struct _BastileSSHSecData BastileSSHSecData;
    
typedef gboolean (*BastileSSHPublicKeyParsed)(BastileSSHKeyData *data, gpointer arg);
typedef gboolean (*BastileSSHSecretKeyParsed)(BastileSSHSecData *data, gpointer arg);

guint                   bastile_ssh_key_data_parse           (const gchar *data, 
                                                               BastileSSHPublicKeyParsed public_cb,
                                                               BastileSSHSecretKeyParsed secret_cb,
                                                               gpointer arg);

guint                   bastile_ssh_key_data_parse_file      (const gchar *filename, 
                                                               BastileSSHPublicKeyParsed public_cb,
                                                               BastileSSHSecretKeyParsed secret_cb,
                                                               gpointer arg,
                                                               GError **error);

BastileSSHKeyData*     bastile_ssh_key_data_parse_line      (const gchar *line,
                                                               guint length);

gboolean                bastile_ssh_key_data_match           (const gchar *line,
                                                               gint length,
                                                               BastileSSHKeyData *match);

gboolean                bastile_ssh_key_data_filter_file     (const gchar *filename,
                                                               BastileSSHKeyData *add,
                                                               BastileSSHKeyData *remove,
                                                               GError **error);

gboolean                bastile_ssh_key_data_is_valid        (BastileSSHKeyData *data);

BastileSSHKeyData*     bastile_ssh_key_data_dup             (BastileSSHKeyData *data);

void                    bastile_ssh_key_data_free            (BastileSSHKeyData *data);

void                    bastile_ssh_sec_data_free            (BastileSSHSecData *data);


#endif /* __BASTILE_KEY_DATA_H__ */
