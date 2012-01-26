/*
 * Bastile
 *
 * Copyright (C) 2003 Jacob Perkins
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

#ifndef __BASTILE_GPGME_KEY_OP_H__
#define __BASTILE_GPGME_KEY_OP_H__

#include <glib.h>
#include <gpgme.h>
#include <time.h>

#include "bastile-operation.h"

#include "pgp/bastile-gpgme-key.h"
#include "pgp/bastile-gpgme-subkey.h"
#include "pgp/bastile-gpgme-uid.h"
#include "pgp/bastile-gpgme-source.h"
#include "pgp/bastile-gpgme-photo.h"

/* 
 * Key type options. 
 * Sadly these are not consistent between versions of GPG.
 */
#if ( GPG_MAJOR == 2 &&   GPG_MINOR == 0 && GPG_MICRO < 12 ) || \
    ( GPG_MAJOR == 1 && ( GPG_MINOR <  4 || GPG_MICRO < 10 ) )

typedef enum {
	/* DSA key with ElGamal subkey. The ElGamal length is variable
	 * within #ELGAMAL_MIN and #LENGTH_MAX. The DSA key will have a
	 * length equal to the ElGamal key's up to a limit of #DSA_MAX.
	 * Only used in bastile_ops_key_generate().
	 */
	DSA_ELGAMAL = 1,
	/* DSA key, sign only. Can be a subkey or a primary key.
	 * See #DSA_MIN and #DSA_MAX.
	 */
	DSA = 2,
	/* ElGamal subkey, encrypt only. See #ELGAMAL_MIN and #LENGTH_MAX.
	 * Only used in bastile_ops_key_add_subkey().
	 */
	ELGAMAL = 4,
	/* RSA key, sign only. Can be a subkey or a primary key.
	 * See #RSA_MIN and #LENGTH_MAX.
	 */
	RSA_SIGN = 5,
	/* RSA subkey, encrypt only. See #RSA_MIN and #LENGTH_MAX.
	 * Only used in bastile_ops_key_add_subkey().
	 */
	RSA_ENCRYPT = 6,
	/* RSA sign-only key with RSA encrypt-only subkey. See #RSA_MIN and
	 * #LENGTH_MAX. Only used in bastile_ops_key_generate().
	 */
	RSA_RSA = 11
} BastileKeyEncType;

#else /* GPG version >=1.4.10 or >=2.0.12 */

typedef enum {
	RSA_RSA = 1,
	DSA_ELGAMAL = 2,
	DSA = 3,
	RSA_SIGN = 4,
	ELGAMAL = 5,
	RSA_ENCRYPT = 6
} BastileKeyEncType;

#endif /* GPG version >=1.4.10 or >=2.0.12 */

/* Length ranges for key types */
typedef enum {
	/* Minimum length for #DSA. */
	DSA_MIN = 768,
	/* Maximum length for #DSA. */
#if ( GPG_MAJOR == 2 &&   GPG_MINOR == 0 && GPG_MICRO < 12 ) || \
    ( GPG_MAJOR == 1 && ( GPG_MINOR <  4 || GPG_MICRO < 10 ) )
	DSA_MAX = 1024,
#else
	DSA_MAX = 3072,
#endif
	/* Minimum length for #ELGAMAL. Maximum length is #LENGTH_MAX. */
	ELGAMAL_MIN = 768,
	/* Minimum length of #RSA_SIGN and #RSA_ENCRYPT. Maximum length is
	 * #LENGTH_MAX.
	 */
	RSA_MIN = 1024,
	/* Maximum length for #ELGAMAL, #RSA_SIGN, and #RSA_ENCRYPT. */
	LENGTH_MAX = 4096,
	/* Default length for #ELGAMAL, #RSA_SIGN, #RSA_ENCRYPT, and #DSA. */
	LENGTH_DEFAULT = 2048
} BastileKeyLength;

typedef enum {
	/* Unknown key check */
	SIGN_CHECK_NO_ANSWER,
	/* Key not checked */
	SIGN_CHECK_NONE,
	/* Key casually checked */
	SIGN_CHECK_CASUAL,
	/* Key carefully checked */
	SIGN_CHECK_CAREFUL
} BastileSignCheck;

typedef enum {
	/* If signature is local */
	SIGN_LOCAL = 1 << 0,
	/* If signature is non-revocable */
	SIGN_NO_REVOKE = 1 << 1,
	/* If signature expires with key */
	SIGN_EXPIRES = 1 << 2
} BastileSignOptions;

typedef enum {
	/* No revocation reason */
	REVOKE_NO_REASON = 0,
	/* Key compromised */
	REVOKE_COMPROMISED = 1,
	/* Key replaced */
	REVOKE_SUPERSEDED = 2,
	/* Key no longer used */
	REVOKE_NOT_USED = 3
} BastileRevokeReason;

BastileOperation*    bastile_gpgme_key_op_generate         (BastileGpgmeSource  *sksrc,
                                                              const gchar *name,
                                                              const gchar *email,
                                                              const gchar *comment,
                                                              const gchar *passphrase,
                                                              BastileKeyEncType type,
                                                              guint length,
                                                              time_t expires,
                                                              gpgme_error_t *err);

gpgme_error_t         bastile_gpgme_key_op_delete           (BastileGpgmeKey *pkey);

gpgme_error_t         bastile_gpgme_key_op_delete_pair      (BastileGpgmeKey *pkey);

gpgme_error_t         bastile_gpgme_key_op_sign             (BastileGpgmeKey *key,
                                                              BastileGpgmeKey *signer,
                                                              BastileSignCheck check,
                                                              BastileSignOptions options);

gpgme_error_t         bastile_gpgme_key_op_sign_uid         (BastileGpgmeUid *uid, 
                                                              BastileGpgmeKey *signer, 
                                                              BastileSignCheck check, 
                                                              BastileSignOptions options);

gpgme_error_t         bastile_gpgme_key_op_change_pass      (BastileGpgmeKey *pkey);

gpgme_error_t         bastile_gpgme_key_op_set_trust        (BastileGpgmeKey *pkey,
                                                              BastileValidity validity);

gpgme_error_t         bastile_gpgme_key_op_set_disabled     (BastileGpgmeKey *pkey,
                                                              gboolean disabled);

gpgme_error_t         bastile_gpgme_key_op_set_expires      (BastileGpgmeSubkey *subkey,
                                                              time_t expires);

gpgme_error_t         bastile_gpgme_key_op_add_revoker      (BastileGpgmeKey *pkey, 
                                                              BastileGpgmeKey *revoker);

gpgme_error_t         bastile_gpgme_key_op_add_uid          (BastileGpgmeKey *pkey,
                                                              const gchar *name,
                                                              const gchar *email,
                                                              const gchar *comment);

gpgme_error_t         bastile_gpgme_key_op_primary_uid      (BastileGpgmeUid *uid);

gpgme_error_t         bastile_gpgme_key_op_del_uid          (BastileGpgmeUid *uid);
                             
gpgme_error_t         bastile_gpgme_key_op_add_subkey       (BastileGpgmeKey *pkey,
                                                              BastileKeyEncType type,
                                                              guint length,
                                                              time_t expires);

gpgme_error_t         bastile_gpgme_key_op_del_subkey       (BastileGpgmeSubkey *subkey);

gpgme_error_t         bastile_gpgme_key_op_revoke_subkey    (BastileGpgmeSubkey *subkey,
                                                              BastileRevokeReason reason,
                                                              const gchar *description);

gpgme_error_t         bastile_gpgme_key_op_photo_add        (BastileGpgmeKey *pkey,
                                                              const gchar *filename);
 
gpgme_error_t         bastile_gpgme_key_op_photo_delete     (BastileGpgmePhoto *photo);
                                                     
gpgme_error_t         bastile_gpgme_key_op_photos_load      (BastileGpgmeKey *key);

gpgme_error_t         bastile_gpgme_key_op_photo_primary    (BastileGpgmePhoto *photo);

#endif /* __BASTILE_GPGME_KEY_OP_H__ */
