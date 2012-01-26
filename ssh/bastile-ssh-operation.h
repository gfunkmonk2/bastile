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
 * BastilePGPOperation: Operations to be done on SSH keys
 * 
 * - Derived from BastileOperation
 *
 * Properties:
 *  TODO: 
 */
 
#ifndef __BASTILE_SSH_OPERATION_H__
#define __BASTILE_SSH_OPERATION_H__

#include "bastile-operation.h"
#include "bastile-ssh-source.h"
#include "bastile-ssh-key.h"

#define BASTILE_TYPE_SSH_OPERATION            (bastile_ssh_operation_get_type ())
#define BASTILE_SSH_OPERATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_SSH_OPERATION, BastileSSHOperation))
#define BASTILE_SSH_OPERATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_SSH_OPERATION, BastileSSHOperationClass))
#define BASTILE_IS_SSH_OPERATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_SSH_OPERATION))
#define BASTILE_IS_SSH_OPERATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_SSH_OPERATION))
#define BASTILE_SSH_OPERATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_SSH_OPERATION, BastileSSHOperationClass))

DECLARE_OPERATION (SSH, ssh)

    /*< public >*/
    BastileSSHSource *sksrc;
    
END_DECLARE_OPERATION

BastileOperation*   bastile_ssh_operation_new          (BastileSSHSource *ssrc,
                                                          const gchar *command, 
                                                          const gchar *input, 
                                                          gint length,
                                                          BastileSSHKey *key);

gchar*               bastile_ssh_operation_sync         (BastileSSHSource *ssrc,
                                                          const gchar *command, 
                                                          GError **error);

/* result: nothing */
BastileOperation*   bastile_ssh_operation_upload       (BastileSSHSource *ssrc,
                                                          GList *keys,
                                                          const gchar *username,
                                                          const gchar *hostname, 
                                                          const gchar *port);

/* result: the generated BastileKey */
BastileOperation*   bastile_ssh_operation_generate     (BastileSSHSource *src, 
                                                          const gchar *email, 
                                                          guint type, 
                                                          guint bits);

/* result: nothing */
BastileOperation*   bastile_ssh_operation_change_passphrase (BastileSSHKey *skey);

/* result: fingerprint of imported key */
BastileOperation*   bastile_ssh_operation_import_public  (BastileSSHSource *ssrc,
                                                            BastileSSHKeyData *data,
                                                            const gchar* filename);

/* result: fingerprint of imported key */
BastileOperation*   bastile_ssh_operation_import_private (BastileSSHSource *ssrc, 
                                                            BastileSSHSecData *data,
                                                            const gchar* filename);

/* result: nothing */
BastileOperation*  bastile_ssh_operation_authorize       (BastileSSHSource *ssrc,
                                                            BastileSSHKey *skey,
                                                            gboolean authorize);

/* result: nothing */
BastileOperation*  bastile_ssh_operation_rename          (BastileSSHSource *ssrc,
                                                            BastileSSHKey *skey,
                                                            const gchar *newcomment);

#endif /* __BASTILE_SSH_OPERATION_H__ */
