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

#ifndef __BASTILE_GKR_ITEM_H__
#define __BASTILE_GKR_ITEM_H__

#include <gtk/gtk.h>
#include <mate-keyring.h>

#include "bastile-gkr-module.h"

#include "bastile-object.h"
#include "bastile-source.h"

typedef enum {
    BASTILE_GKR_USE_OTHER,
    BASTILE_GKR_USE_NETWORK,
    BASTILE_GKR_USE_WEB,
    BASTILE_GKR_USE_PGP,
    BASTILE_GKR_USE_SSH,
} BastileGkrUse;

#define BASTILE_TYPE_GKR_ITEM             (bastile_gkr_item_get_type ())
#define BASTILE_GKR_TYPE_ITEM BASTILE_TYPE_GKR_ITEM
#define BASTILE_GKR_ITEM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_GKR_ITEM, BastileGkrItem))
#define BASTILE_GKR_ITEM_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_GKR_ITEM, BastileGkrItemClass))
#define BASTILE_IS_GKR_ITEM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_GKR_ITEM))
#define BASTILE_IS_GKR_ITEM_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_GKR_ITEM))
#define BASTILE_GKR_ITEM_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_GKR_ITEM_KEY, BastileGkrItemClass))


typedef struct _BastileGkrItem BastileGkrItem;
typedef struct _BastileGkrItemClass BastileGkrItemClass;
typedef struct _BastileGkrItemPrivate BastileGkrItemPrivate;

struct _BastileGkrItem {
	BastileObject parent;
	BastileGkrItemPrivate *pv;
};

struct _BastileGkrItemClass {
    BastileObjectClass         parent_class;
};

GType                        bastile_gkr_item_get_type           (void);

BastileGkrItem*             bastile_gkr_item_new                (BastileSource *source,
                                                                   const gchar *keyring_name,
                                                                   guint32 item_id);

guint32                      bastile_gkr_item_get_item_id        (BastileGkrItem *self);

const gchar*                 bastile_gkr_item_get_keyring_name   (BastileGkrItem *self);

MateKeyringItemInfo*        bastile_gkr_item_get_info           (BastileGkrItem *self);

void                         bastile_gkr_item_set_info           (BastileGkrItem *self,
                                                                   MateKeyringItemInfo* info);

gboolean                     bastile_gkr_item_has_secret         (BastileGkrItem *self);

const gchar*                 bastile_gkr_item_get_secret         (BastileGkrItem *self);

MateKeyringAttributeList*   bastile_gkr_item_get_attributes     (BastileGkrItem *self);

void                         bastile_gkr_item_set_attributes     (BastileGkrItem *self,
                                                                   MateKeyringAttributeList* attrs);

const gchar*                 bastile_gkr_item_get_attribute      (BastileGkrItem *self, 
                                                                   const gchar *name);

const gchar* 		     bastile_gkr_find_string_attribute   (MateKeyringAttributeList *attrs, 
             		                                           const gchar *name);

GList*                       bastile_gkr_item_get_acl            (BastileGkrItem *self);

void                         bastile_gkr_item_set_acl            (BastileGkrItem *self,
                                                                   GList* acl);

GQuark                       bastile_gkr_item_get_cannonical     (const gchar *keyring_name, 
                                                                   guint32 item_id);

BastileGkrUse               bastile_gkr_item_get_use            (BastileGkrItem *self);

#endif /* __BASTILE_GKR_ITEM_H__ */
