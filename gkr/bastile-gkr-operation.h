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
 * BastileGkrOperation: Operations to be done on mate-keyring items
 * 
 * - Derived from BastileOperation
 */
 
#ifndef __BASTILE_GKR_OPERATION_H__
#define __BASTILE_GKR_OPERATION_H__

#include "bastile-operation.h"
#include "bastile-gkr-source.h"
#include "bastile-gkr-item.h"
#include "bastile-gkr-keyring.h"

#define BASTILE_TYPE_GKR_OPERATION            (bastile_gkr_operation_get_type ())
#define BASTILE_GKR_OPERATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_GKR_OPERATION, BastileGkrOperation))
#define BASTILE_GKR_OPERATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_GKR_OPERATION, BastileGkrOperationClass))
#define BASTILE_IS_GKR_OPERATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_GKR_OPERATION))
#define BASTILE_IS_GKR_OPERATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_GKR_OPERATION))
#define BASTILE_GKR_OPERATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_GKR_OPERATION, BastileGkrOperationClass))

typedef struct _BastileGkrOperation BastileGkrOperation;
typedef struct _BastileGkrOperationClass BastileGkrOperationClass;
typedef struct _BastileGkrOperationPrivate BastileGkrOperationPrivate;
    
struct _BastileGkrOperation {
	BastileOperation parent;
	BastileGkrOperationPrivate *pv;
};

struct _BastileGkrOperationClass {
	BastileOperationClass parent_class;
};

GType                bastile_gkr_operation_get_type               (void);


gboolean             bastile_gkr_operation_parse_error       (MateKeyringResult result, 
                                                               GError **err);

/* result: nothing */
BastileOperation*   bastile_gkr_operation_update_info       (BastileGkrItem *git,
                                                               MateKeyringItemInfo *info);

/* result: nothing */
BastileOperation*   bastile_gkr_operation_update_acl        (BastileGkrItem *git,
                                                               GList *acl);

/* result: nothing */
BastileOperation*   bastile_gkr_operation_delete_item       (BastileGkrItem *git);

/* result: nothing */
BastileOperation*   bastile_gkr_operation_delete_keyring    (BastileGkrKeyring *git);

#endif /* __BASTILE_GKR_OPERATION_H__ */
