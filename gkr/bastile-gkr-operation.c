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

#include "config.h"

#include <sys/wait.h>
#include <sys/socket.h>
#include <fcntl.h>

#include <glib/gi18n.h>

#include "bastile-gkr-operation.h"
#include "bastile-util.h"
#include "bastile-passphrase.h"

#include <mate-keyring.h>

#ifndef DEBUG_OPERATION_ENABLE
#if _DEBUG
#define DEBUG_OPERATION_ENABLE 1
#else
#define DEBUG_OPERATION_ENABLE 0
#endif
#endif

#if DEBUG_OPERATION_ENABLE
#define DEBUG_OPERATION(x)  g_printerr x
#else
#define DEBUG_OPERATION(x)
#endif

/* -----------------------------------------------------------------------------
 * DEFINITIONS
 */
 
struct _BastileGkrOperationPrivate {
	gpointer request;
	BastileObject *object;   
};

G_DEFINE_TYPE (BastileGkrOperation, bastile_gkr_operation, BASTILE_TYPE_OPERATION);

/* -----------------------------------------------------------------------------
 * HELPERS 
 */

static gboolean
check_operation_result (BastileGkrOperation *self, MateKeyringResult result)
{
    GError *err = NULL;
    gboolean success;
    
    /* This only gets called when we cancel, so ignore */
    if (result == MATE_KEYRING_RESULT_CANCELLED)
        return FALSE;
    
    success = bastile_gkr_operation_parse_error (result, &err);
    g_assert (!success || !err);
    
    bastile_operation_mark_done (BASTILE_OPERATION (self), FALSE, err);
    return success;
}

/* -----------------------------------------------------------------------------
 * OBJECT 
 */

static void 
bastile_gkr_operation_init (BastileGkrOperation *self)
{
	self->pv = G_TYPE_INSTANCE_GET_PRIVATE (self, BASTILE_TYPE_GKR_OPERATION, BastileGkrOperationPrivate);

}

static void 
bastile_gkr_operation_dispose (GObject *gobject)
{
	BastileGkrOperation *self = BASTILE_GKR_OPERATION (gobject);

	if (bastile_operation_is_running (BASTILE_OPERATION (self)))
		bastile_operation_cancel (BASTILE_OPERATION (self));
	g_assert (!bastile_operation_is_running (BASTILE_OPERATION (self)));
    
	if (self->pv->object)
		g_object_unref (self->pv->object);
	self->pv->object = NULL;
    
	/* The above cancel should have stopped this */
	g_assert (self->pv->request == NULL);
    
	G_OBJECT_CLASS (bastile_gkr_operation_parent_class)->dispose (gobject);  
}

static void 
bastile_gkr_operation_finalize (GObject *gobject)
{
	BastileGkrOperation *self = BASTILE_GKR_OPERATION (gobject);
    
	g_assert (!self->pv->request);
    
	G_OBJECT_CLASS (bastile_gkr_operation_parent_class)->finalize (gobject);  
}

static void 
bastile_gkr_operation_cancel (BastileOperation *operation)
{
	BastileGkrOperation *self = BASTILE_GKR_OPERATION (operation);    

	if (self->pv->request)
		mate_keyring_cancel_request (self->pv->request);
	self->pv->request = NULL;
    
	if (bastile_operation_is_running (operation))
		bastile_operation_mark_done (operation, TRUE, NULL);    
}

static void
bastile_gkr_operation_class_init (BastileGkrOperationClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    
	bastile_gkr_operation_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (BastileGkrOperationPrivate));

	gobject_class->dispose = bastile_gkr_operation_dispose;
	gobject_class->finalize = bastile_gkr_operation_finalize;
	
	BASTILE_OPERATION_CLASS (klass)->cancel = bastile_gkr_operation_cancel;
}

/* -----------------------------------------------------------------------------
 * PUBLIC 
 */

gboolean
bastile_gkr_operation_parse_error (MateKeyringResult result, GError **err)
{
    static GQuark errorq = 0;
    const gchar *message = NULL;

    if (result == MATE_KEYRING_RESULT_OK)
        return TRUE;
    
    /* These should be handled in the callbacks */
    g_assert (result != MATE_KEYRING_RESULT_CANCELLED);
    
    /* An error mark it as such */
    switch (result) {
    case MATE_KEYRING_RESULT_DENIED:
        message = _("Access to the key ring was denied");
        break;
    case MATE_KEYRING_RESULT_NO_KEYRING_DAEMON:
        message = _("The mate-keyring daemon is not running");
        break;
    case MATE_KEYRING_RESULT_ALREADY_UNLOCKED:
        message = _("The key ring has already been unlocked");
        break;
    case MATE_KEYRING_RESULT_NO_SUCH_KEYRING:
        message = _("No such key ring exists");
        break;
    case MATE_KEYRING_RESULT_IO_ERROR:
        message = _("Couldn't communicate with key ring daemon");
        break;
    case MATE_KEYRING_RESULT_ALREADY_EXISTS:
        message = _("The item already exists");
        break;
    case MATE_KEYRING_RESULT_BAD_ARGUMENTS:
        g_warning ("bad arguments passed to mate-keyring API");
        /* Fall through */
    default:
        message = _("Internal error accessing mate-keyring");
        break;
    }
    
    if (!errorq) 
        errorq = g_quark_from_static_string ("bastile-mate-keyring");
    
    g_set_error (err, errorq, result, "%s", message);
    return FALSE;
}

/* -----------------------------------------------------------------------------
 * UPDATE INFO OPERATION
 */

static void 
basic_operation_done (MateKeyringResult result, BastileGkrOperation *self)
{
	g_assert (BASTILE_IS_GKR_OPERATION (self));
	self->pv->request = NULL;

	if (!check_operation_result (self, result))
		return;
    
	/* When operation is successful reload the key */
	bastile_object_refresh (BASTILE_OBJECT (self->pv->object));
}

BastileOperation*
bastile_gkr_operation_update_info (BastileGkrItem *item, MateKeyringItemInfo *info)
{
	BastileGkrOperation *self;
    
	g_return_val_if_fail (BASTILE_IS_GKR_ITEM (item), NULL);
    
	self = g_object_new (BASTILE_TYPE_GKR_OPERATION, NULL);
    
	g_object_ref (item);
	self->pv->object = BASTILE_OBJECT (item);
    
	/* Start actual save request */
	g_object_ref (self);
	self->pv->request = mate_keyring_item_set_info (bastile_gkr_item_get_keyring_name (item),
	                                                 bastile_gkr_item_get_item_id (item), info, 
	                                                 (MateKeyringOperationDoneCallback)basic_operation_done,
	                                                 self, g_object_unref);
	g_return_val_if_fail (self->pv->request, NULL);
    
	bastile_operation_mark_start (BASTILE_OPERATION (self));
	bastile_operation_mark_progress (BASTILE_OPERATION (self), _("Saving item..."), -1);

	return BASTILE_OPERATION (self);
}

BastileOperation*
bastile_gkr_operation_update_acl (BastileGkrItem *item, GList *acl)
{
	BastileGkrOperation *self;
    
	g_return_val_if_fail (BASTILE_IS_GKR_ITEM (item), NULL);
    
	self = g_object_new (BASTILE_TYPE_GKR_OPERATION, NULL);

	g_object_ref (item);
	self->pv->object = BASTILE_OBJECT (item);
    
	/* Start actual save request */
	g_object_ref (self);
	self->pv->request = mate_keyring_item_set_acl (bastile_gkr_item_get_keyring_name (item), 
	                                                bastile_gkr_item_get_item_id (item), acl, 
	                                                (MateKeyringOperationDoneCallback)basic_operation_done,
	                                                self, g_object_unref);
	g_return_val_if_fail (self->pv->request, NULL);
    
	bastile_operation_mark_start (BASTILE_OPERATION (self));
	bastile_operation_mark_progress (BASTILE_OPERATION (self), _("Saving item..."), -1);

	return BASTILE_OPERATION (self);
}

static void 
delete_operation_done (MateKeyringResult result, BastileGkrOperation *self)
{
	g_assert (BASTILE_IS_GKR_OPERATION (self));

	self->pv->request = NULL;
	if (check_operation_result (self, result))
		bastile_context_remove_object (NULL, self->pv->object);
}

BastileOperation*
bastile_gkr_operation_delete_item (BastileGkrItem *item)
{
	BastileGkrOperation *self;
	    
	g_return_val_if_fail (BASTILE_IS_GKR_ITEM (item), NULL);
	    
	self = g_object_new (BASTILE_TYPE_GKR_OPERATION, NULL);
	    
	g_object_ref (item);
	self->pv->object = BASTILE_OBJECT (item);
	    
	/* Start actual save request */
	g_object_ref (self);
	self->pv->request = mate_keyring_item_delete (bastile_gkr_item_get_keyring_name (item), 
	                                               bastile_gkr_item_get_item_id (item), 
	                                               (MateKeyringOperationDoneCallback)delete_operation_done,
	                                               self, g_object_unref);
	g_return_val_if_fail (self->pv->request, NULL);
	
	bastile_operation_mark_start (BASTILE_OPERATION (self));
	bastile_operation_mark_progress (BASTILE_OPERATION (self), _("Deleting item..."), -1);
	
	return BASTILE_OPERATION (self);
}

BastileOperation*
bastile_gkr_operation_delete_keyring (BastileGkrKeyring *keyring)
{
	BastileGkrOperation *self;
	    
	g_return_val_if_fail (BASTILE_IS_GKR_KEYRING (keyring), NULL);
	    
	self = g_object_new (BASTILE_TYPE_GKR_OPERATION, NULL);
	
	g_object_ref (keyring);
	self->pv->object = BASTILE_OBJECT (keyring);
	    
	/* Start actual save request */
	g_object_ref (self);
	self->pv->request = mate_keyring_delete (bastile_gkr_keyring_get_name (keyring), 
	                                          (MateKeyringOperationDoneCallback)delete_operation_done,
	                                          self, g_object_unref);
	g_return_val_if_fail (self->pv->request, NULL);
	
	bastile_operation_mark_start (BASTILE_OPERATION (self));
	bastile_operation_mark_progress (BASTILE_OPERATION (self), _("Deleting keyring..."), -1);
	
	return BASTILE_OPERATION (self);
}
