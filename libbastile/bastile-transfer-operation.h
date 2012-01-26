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
 
#ifndef __BASTILE_TRANSFER_OPERATION_H__
#define __BASTILE_TRANSFER_OPERATION_H__

#include "bastile-operation.h"
#include "bastile-source.h"

#define BASTILE_TYPE_TRANSFER_OPERATION            (bastile_transfer_operation_get_type ())
#define BASTILE_TRANSFER_OPERATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_TRANSFER_OPERATION, BastileTransferOperation))
#define BASTILE_TRANSFER_OPERATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_TRANSFER_OPERATION, BastileTransferOperationClass))
#define BASTILE_IS_TRANSFER_OPERATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_TRANSFER_OPERATION))
#define BASTILE_IS_TRANSFER_OPERATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_TRANSFER_OPERATION))
#define BASTILE_TRANSFER_OPERATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_TRANSFER_OPERATION, BastileTransferOperationClass))

/**
 * BastileTransferOperation:
 * @parent: The parent #BastileOperation
 *
 * Transfer a set of keys from one key source to another.
 *
 * - Derived from BastileOperation
 * - Exports all keys from 'from' source key source.
 * - Import them into the 'to' key source.
 *
 * Properties:
 *  from-key-source: (BastileSource) From key source
 *  to-key-source: (BastileSource) To key source
 */

DECLARE_OPERATION (Transfer, transfer)
    /*< public >*/
    BastileSource *from;
    BastileSource *to;
END_DECLARE_OPERATION

BastileOperation*      bastile_transfer_operation_new     (const gchar *message,
                                                             BastileSource *from,
                                                             BastileSource *to,
                                                             GSList *keyids);

#endif /* __BASTILE_TRANSFER_OPERATION_H__ */
