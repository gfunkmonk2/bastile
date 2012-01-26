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
 * BastileGpgmeOperation: A way to track operations done via GPGME. 
 * 
 * - Derived from BastileOperation
 * - Uses the arcane gpgme_io_cbs API.
 * - Creates a new context so this doesn't block other operations.
 *
 * Signals:
 *   results: The GPGME results are ready. This is called before 
 *            the 'done' signal of BastileOperation.
 *
 * Properties:
 *  gctx:  (pointer) GPGME context.
 *  message: (string) message to display instead of from GPGME.
 *  default-total: (guint) When GPGME reports 0 as progress total, use this instead.
 * 
 * HOW TO USE: 
 * 1. Create a BastileGpgmeOperation with bastile_gpgme_operation_new. 
 * 2. You'll be using the gpgme_ctx_t out of the gctx data member.
 * 3. Use one of the gpgme_op_*_start functions to start an operation with gctx.
 * 4. Hand off the operation to bastile_progress_* functions (which claim 
 *    ownership of the operation) to see a progress dialog or status bar.
 * 
 * NOTES:
 * - Never use with gpgme_op_keylist_start and gpgme_op_trustlist_start. 
 */
 
#ifndef __BASTILE_GPGME_OPERATION_H__
#define __BASTILE_GPGME_OPERATION_H__

/*
 * TODO: Eventually most of the stuff from bastile-pgp-key-op and 
 * bastile-op should get merged into here.
 */

#include "bastile-operation.h"

#include "pgp/bastile-gpgme.h"

#define BASTILE_TYPE_GPGME_OPERATION            (bastile_gpgme_operation_get_type ())
#define BASTILE_GPGME_OPERATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_GPGME_OPERATION, BastileGpgmeOperation))
#define BASTILE_GPGME_OPERATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_GPGME_OPERATION, BastileGpgmeOperationClass))
#define BASTILE_IS_GPGME_OPERATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_GPGME_OPERATION))
#define BASTILE_IS_GPGME_OPERATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_GPGME_OPERATION))
#define BASTILE_GPGME_OPERATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_GPGME_OPERATION, BastileGpgmeOperationClass))

typedef struct _BastileGpgmeOperation BastileGpgmeOperation;
typedef struct _BastileGpgmeOperationClass BastileGpgmeOperationClass;

struct _BastileGpgmeOperation {
    BastileOperation parent;
    
    /*< public >*/
    gpgme_ctx_t gctx;
};

struct _BastileGpgmeOperationClass {
    BastileOperationClass parent_class;

    /* Signal that occurs when the results of the GPGME operation are ready */
    void (*results) (BastileGpgmeOperation *pop);
};


GType                   bastile_gpgme_operation_get_type     (void);

BastileGpgmeOperation*   bastile_gpgme_operation_new          (const gchar *message);

/* Call when calling of gpgme_op_*_start failed */
void                    bastile_gpgme_operation_mark_failed  (BastileGpgmeOperation *pop, 
                                                             gpgme_error_t gerr);

#endif /* __BASTILE_GPGME_OPERATION_H__ */
