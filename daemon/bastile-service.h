/*
 * Bastile
 *
 * Copyright (C) 2005 Stefan Walter
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

#ifndef __BASTILE_SERVICE_H__
#define __BASTILE_SERVICE_H__

#include <glib.h>

#include "bastile-set.h"

G_BEGIN_DECLS

/* TODO: This needs to be refined */
enum {
    BASTILE_DBUS_ERROR_CANCELLED = -1,
    BASTILE_DBUS_ERROR_INVALID = 1,
    BASTILE_DBUS_ERROR_CRYPTO = 2,
    BASTILE_DBUS_ERROR_NOTIMPLEMENTED = 100
};

#define BASTILE_DBUS_ERROR  	  g_quark_from_static_string ("org.mate.bastile.Error.Failed")
#define BASTILE_DBUS_CANCELLED   g_quark_from_static_string ("org.mate.bastile.Error.Cancelled")

typedef struct _BastileService BastileService;
typedef struct _BastileServiceClass BastileServiceClass;
    
#define BASTILE_TYPE_SERVICE               (bastile_service_get_type ())
#define BASTILE_SERVICE(object)            (G_TYPE_CHECK_INSTANCE_CAST((object), BASTILE_TYPE_SERVICE, BastileService))
#define BASTILE_SERVICE_CLASS(klass)       (G_TYPE_CHACK_CLASS_CAST((klass), BASTILE_TYPE_SERVICE, BastileServiceClass))
#define BASTILE_IS_SERVICE(object)         (G_TYPE_CHECK_INSTANCE_TYPE((object), BASTILE_TYPE_SERVICE))
#define BASTILE_IS_SERVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE((klass), BASTILE_TYPE_SERVICE))
#define BASTILE_SERVICE_GET_CLASS(object)  (G_TYPE_INSTANCE_GET_CLASS((object), BASTILE_TYPE_SERVICE, BastileServiceClass))

struct _BastileService {
    GObject base;
    
    /* <public> */
    GHashTable *keysets;
};

struct _BastileServiceClass {
    GObjectClass base;
};

GType          bastile_service_get_type              (void);

gboolean       bastile_service_get_key_types         (BastileService *svc, gchar*** ret, 
                                                       GError **error);

gboolean        bastile_service_get_keyset           (BastileService *svc, gchar *ktype, 
                                                       gchar **path, GError **error);

gboolean       bastile_service_generate_credentials  (BastileService *svc, gchar *ktype,
                              GHashTable *values, GError **error);

gboolean        bastile_service_import_keys           (BastileService *svc, gchar *ktype, 
                                                        gchar *data, gchar ***keys, GError **error);
                              
gboolean        bastile_service_export_keys           (BastileService *svc, gchar *ktype,
                                                        gchar **keys, gchar **data, GError **error);

gboolean        bastile_service_display_notification  (BastileService *svc, gchar *heading,
                                                        gchar *text, gchar *icon, 
                                                        gboolean urgent, GError **error);

#if 0
gboolean        bastile_service_match_save            (BastileService *svc, gchar *ktype, 
                                                        gint flags, gchar **patterns, 
                                                        gchar **keys, GError **error);
#endif

/* -----------------------------------------------------------------------------
 * KEYSET SERVICE
 */
 
typedef struct _BastileServiceKeyset BastileServiceKeyset;
typedef struct _BastileServiceKeysetClass BastileServiceKeysetClass;
    
#define BASTILE_TYPE_SERVICE_KEYSET               (bastile_service_keyset_get_type ())
#define BASTILE_SERVICE_KEYSET(object)            (G_TYPE_CHECK_INSTANCE_CAST((object), BASTILE_TYPE_SERVICE_KEYSET, BastileServiceKeyset))
#define BASTILE_SERVICE_KEYSET_CLASS(klass)       (G_TYPE_CHACK_CLASS_CAST((klass), BASTILE_TYPE_SERVICE_KEYSET, BastileServiceKeysetClass))
#define BASTILE_IS_SERVICE_KEYSET(object)         (G_TYPE_CHECK_INSTANCE_TYPE((object), BASTILE_TYPE_SERVICE_KEYSET))
#define BASTILE_IS_SERVICE_KEYSET_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE((klass), BASTILE_TYPE_SERVICE_KEYSET))
#define BASTILE_SERVICE_KEYSET_GET_CLASS(object)  (G_TYPE_INSTANCE_GET_CLASS((object), BASTILE_TYPE_SERVICE_KEYSET, BastileServiceKeysetClass))

struct _BastileServiceKeyset {
    BastileSet base;
    
    /* <public> */
    GQuark ktype;
};

struct _BastileServiceKeysetClass {
    BastileSetClass base;
    
    /* signals --------------------------------------------------------- */
    
    /* A key was added to this view */
    void (*key_added)   (BastileServiceKeyset *skset, const gchar *key);

    /* Removed a key from this view */
    void (*key_removed) (BastileServiceKeyset *skset, const gchar *key);
    
    /* One of the key's attributes has changed */
    void (*key_changed) (BastileServiceKeyset *skset, const gchar *key);   
};

GType           bastile_service_keyset_get_type       (void);

BastileSet* bastile_service_keyset_new            (GQuark keytype, 
                                                        BastileLocation location);

gboolean        bastile_service_keyset_list_keys      (BastileServiceKeyset *keyset,
                                                        gchar ***keys, GError **error);

gboolean        bastile_service_keyset_get_key_field  (BastileService *svc, gchar *key, 
                                                        gchar *field, gboolean *has, 
                                                        GValue *value, GError **error);

gboolean        bastile_service_keyset_get_key_fields (BastileService *svc, gchar *key, 
                                                        gchar **fields, GHashTable **values, 
                                                        GError **error);

gboolean        bastile_service_keyset_discover_keys  (BastileServiceKeyset *keyset, 
                                                        const gchar **keyids, gint flags,
                                                        gchar ***keys, GError **error);


gboolean        bastile_service_keyset_match_keys     (BastileServiceKeyset *keyset, 
                                                        gchar **patterns, gint flags, 
                                                        gchar ***keys, gchar***unmatched, 
                                                        GError **error);

/* -----------------------------------------------------------------------------
 * CRYPTO SERVICE
 */

typedef struct _BastileServiceCrypto BastileServiceCrypto;
typedef struct _BastileServiceCryptoClass BastileServiceCryptoClass;
    
#define BASTILE_TYPE_SERVICE_CRYPTO               (bastile_service_crypto_get_type ())
#define BASTILE_SERVICE_CRYPTO(object)            (G_TYPE_CHECK_INSTANCE_CAST((object), BASTILE_TYPE_SERVICE_CRYPTO, BastileServiceCrypto))
#define BASTILE_SERVICE_CRYPTO_CLASS(klass)       (G_TYPE_CHACK_CLASS_CAST((klass), BASTILE_TYPE_SERVICE_CRYPTO, BastileServiceCryptoClass))
#define BASTILE_IS_SERVICE_CRYPTO(object)         (G_TYPE_CHECK_INSTANCE_TYPE((object), BASTILE_TYPE_SERVICE_CRYPTO))
#define BASTILE_IS_SERVICE_CRYPTO_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE((klass), BASTILE_TYPE_SERVICE_CRYPTO))
#define BASTILE_SERVICE_CRYPTO_GET_CLASS(object)  (G_TYPE_INSTANCE_GET_CLASS((object), BASTILE_TYPE_SERVICE_CRYPTO, BastileServiceCryptoClass))

struct _BastileServiceCrypto {
    GObject base;
};

struct _BastileServiceCryptoClass {
    GObjectClass base;
};

GType                   bastile_service_crypto_get_type       (void);

BastileServiceCrypto*  bastile_service_crypto_new            ();

gboolean                bastile_service_crypto_encrypt_text    (BastileServiceCrypto *crypto, 
                                                                 const gchar **recipients, 
                                                                 const gchar *signer, int flags, 
                                                                 const gchar *cleartext, 
                                                                 gchar **crypttext, GError **error);

gboolean                bastile_service_crypto_encrypt_file    (BastileServiceCrypto *crypto,
                                                                 const char **recipients,
                                                                 const char *signer,
                                                                 int flags,
                                                                 const char  *clearuri,
                                                                 const char  *crypturi,
                                                                 GError **error);

gboolean                bastile_service_crypto_decrypt_file    (BastileServiceCrypto *crypto,
                                                                 const char *ktype,
                                                                 int flags,
                                                                 const char * crypturi,
                                                                 const char *clearuri,
                                                                 char **signer,
                                                                 GError **error);

gboolean                bastile_service_crypto_sign_text       (BastileServiceCrypto *crypto, 
                                                                 const gchar *signer, int flags, 
                                                                 const gchar *cleartext, 
                                                                 gchar **crypttext, GError **error);

gboolean                bastile_service_crypto_decrypt_text    (BastileServiceCrypto *crypto, 
                                                                 const gchar *ktype, int flags, 
                                                                 const gchar *crypttext, 
                                                                 gchar **cleartext, gchar **signer,
                                                                 GError **error);

gboolean                bastile_service_crypto_verify_text     (BastileServiceCrypto *crypto, 
                                                                 const gchar *ktype, int flags, 
                                                                 const gchar *crypttext, 
                                                                 gchar **cleartext, gchar **signer,
                                                                 GError **error);

G_END_DECLS

#endif /* __BASTILE_SERVICE_H__ */
