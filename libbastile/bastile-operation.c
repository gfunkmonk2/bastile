/*
 * Bastile
 *
 * Copyright (C) 2004-2006 Stefan Walter
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

#include "bastile-util.h"
#include "bastile-marshal.h"
#include "bastile-operation.h"

/**
 * SECTION:bastile-operation
 * @short_description: Contains code for operations and multi operations (container for several operations)
 * @include:bastile-operation.h
 *
 **/

/* Override the DEBUG_OPERATION_ENABLE switch here */
#define DEBUG_OPERATION_ENABLE 0

#ifndef DEBUG_OPERATION_ENABLE
#if _DEBUG
#define DEBUG_OPERATION_ENABLE 1
#else
#define DEBUG_OPERATION_ENABLE 0
#endif
#endif

#if DEBUG_OPERATION_ENABLE
#define DEBUG_OPERATION(x) g_printerr x
#else
#define DEBUG_OPERATION(x) 
#endif

/* -----------------------------------------------------------------------------
 * BASTILE OPERATION
 */

enum {
    PROP_0,
    PROP_PROGRESS,
    PROP_MESSAGE,
    PROP_RESULT
};

enum {
    DONE,
    PROGRESS,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (BastileOperation, bastile_operation, G_TYPE_OBJECT);

/* OBJECT ------------------------------------------------------------------- */

/**
* operation:the #BastileOperation to initialise
*
* Initialises an operation
*
**/
static void
bastile_operation_init (BastileOperation *operation)
{

    /* This is the state of non-started operation. all other flags are zero */
    operation->progress = -1;
}

/**
* object: an object of the type BASTILE_OPERATION
* prop_id: the id of the property to set
* value: The value to set
* pspec: ignored
*
* Sets a property of the BASTILE_OPERATION passed
* At the moment only PROP_MESSAGE is supported
*
**/
static void
bastile_operation_set_property (GObject *object, guint prop_id, 
                                 const GValue *value, GParamSpec *pspec)
{
    BastileOperation *op = BASTILE_OPERATION (object);
    
    switch (prop_id) {
    case PROP_MESSAGE:
        g_free (op->message);
        op->message = g_value_dup_string (value);
        break;
    }
}

/**
* object: an object of the type BASTILE_OPERATION
* prop_id: the id of the property to get
* value: The returned value
* pspec: ignored
*
* Gets a property of the BASTILE_OPERATION passed
* Supported properties are: PROP_PROGRESS, PROP_MESSAGE, PROP_RESULT
*
**/
static void
bastile_operation_get_property (GObject *object, guint prop_id, 
                                 GValue *value, GParamSpec *pspec)
{
    BastileOperation *op = BASTILE_OPERATION (object);
    
    switch (prop_id) {
    case PROP_PROGRESS:
        g_value_set_double (value, op->progress);
        break;
    case PROP_MESSAGE:
        g_value_set_string (value, op->message);
        break;
    case PROP_RESULT:
        g_value_set_pointer (value, op->result);
        break;
    }
}

/**
* gobject: The operation to dispose
*
* dispose of all our internal references
*
**/
static void
bastile_operation_dispose (GObject *gobject)
{
    BastileOperation *op = BASTILE_OPERATION (gobject);
    
    if (op->is_running)
        bastile_operation_cancel (op);
    g_assert (!op->is_running);

    /* We do this in dispose in case it's a circular reference */
    if (op->result && op->result_destroy)
        (op->result_destroy) (op->result);
    op->result = NULL;
    op->result_destroy = NULL;
    
    G_OBJECT_CLASS (bastile_operation_parent_class)->dispose (gobject);
}


/**
* gobject: The #BASTILE_OPERATION to finalize
*
* free private vars
*
**/
static void
bastile_operation_finalize (GObject *gobject)
{
    BastileOperation *op = BASTILE_OPERATION (gobject);
    g_assert (!op->is_running);
    
    if (op->error) {
        g_error_free (op->error);
        op->error = NULL;
    }
        
    g_free (op->message);
    op->message = NULL;
        
    G_OBJECT_CLASS (bastile_operation_parent_class)->finalize (gobject);
}

/**
* klass: The #BastileOperationClass to initialise
*
* Initialises the class, adds properties and signals
*
**/
static void
bastile_operation_class_init (BastileOperationClass *klass)
{
    GObjectClass *gobject_class;
   
    bastile_operation_parent_class = g_type_class_peek_parent (klass);
    gobject_class = G_OBJECT_CLASS (klass);
    
    gobject_class->dispose = bastile_operation_dispose;
    gobject_class->finalize = bastile_operation_finalize;
    gobject_class->set_property = bastile_operation_set_property;
    gobject_class->get_property = bastile_operation_get_property;

    g_object_class_install_property (gobject_class, PROP_PROGRESS,
        g_param_spec_double ("progress", "Progress position", "Current progress position (fraction between 0 and 1)", 
                             0.0, 1.0, 0.0, G_PARAM_READABLE));

    g_object_class_install_property (gobject_class, PROP_MESSAGE,
        g_param_spec_string ("message", "Status message", "Current progress message", 
                             NULL, G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_RESULT,
        g_param_spec_pointer ("result", "Operation Result", "Exact value depends on derived class.", 
                              G_PARAM_READABLE));

    
    signals[DONE] = g_signal_new ("done", BASTILE_TYPE_OPERATION, 
                G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BastileOperationClass, done),
                NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

    signals[PROGRESS] = g_signal_new ("progress", BASTILE_TYPE_OPERATION, 
                G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (BastileOperationClass, progress),
                NULL, NULL, bastile_marshal_VOID__STRING_DOUBLE, G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_DOUBLE);
}

/* PUBLIC ------------------------------------------------------------------- */

/**
 * bastile_operation_new_complete:
 * @err: an optional error to set
 *
 * Creates a new operation and sets it's state to done
 *
 * Returns: the operation
 */
BastileOperation*  
bastile_operation_new_complete (GError *err)
{
    BastileOperation *operation;
    
    operation = g_object_new (BASTILE_TYPE_OPERATION, NULL);
    bastile_operation_mark_start (operation);
    bastile_operation_mark_done (operation, FALSE, err);
    return operation;
}

/**
 * bastile_operation_new_cancelled:
 *
 * Creates a new operation and sets in to cancelled  state
 *
 * Returns: The new operation
 */
BastileOperation*
bastile_operation_new_cancelled ()
{
    BastileOperation *operation;
    
    operation = g_object_new (BASTILE_TYPE_OPERATION, NULL);
    bastile_operation_mark_start (operation);
    bastile_operation_mark_done (operation, TRUE, NULL);
    return operation;
}

/**
 * bastile_operation_cancel:
 * @op: the #BastileOperation to cancel
 *
 * Cancels the operation
 *
 */
void
bastile_operation_cancel (BastileOperation *op)
{
    BastileOperationClass *klass;

    g_return_if_fail (BASTILE_IS_OPERATION (op));
    g_return_if_fail (op->is_running);

    g_object_ref (op);
 
    klass = BASTILE_OPERATION_GET_CLASS (op);
    
    /* A derived operation exists */
    if (klass->cancel)
        (*klass->cancel) (op); 
    
    /* No derived operation exists */
    else
        bastile_operation_mark_done (op, TRUE, NULL);
    
    g_object_unref (op);
}

/**
 * bastile_operation_copy_error:
 * @op: the #BastileOperation
 * @err: The resulting error
 *
 * Copies an internal error from the #BastileOperation to @err
 *
 */
void
bastile_operation_copy_error  (BastileOperation *op, GError **err)
{
	g_return_if_fail (BASTILE_IS_OPERATION (op));
	g_return_if_fail (err == NULL || *err == NULL);
    
	if (err) 
		*err = op->error ? g_error_copy (op->error) : NULL;
}

/**
 * bastile_operation_get_error:
 * @op: A #BastileOperation
 *
 * Directly return the error from operation
 *
 * Returns: the #GError error from the operation
 */
const GError*       
bastile_operation_get_error (BastileOperation *op)
{
	g_return_val_if_fail (BASTILE_IS_OPERATION (op), NULL);
	return op->error;
}

/**
 * bastile_operation_display_error:
 * @operation: a #BastileOperation
 * @title: The title of the error box
 * @parent: ignored
 *
 * Displays an error box if there is an error in the operation
 *
 */
void
bastile_operation_display_error (BastileOperation *operation, 
                                  const gchar *title, GtkWidget *parent)
{
	g_return_if_fail (BASTILE_IS_OPERATION (operation));
	g_return_if_fail (operation->error);
	bastile_util_handle_error (operation->error, "%s", title);
}

/**
 * bastile_operation_get_result:
 * @op: a #BastileOperation
 *
 *
 *
 * Returns: the results of this operation
 */
gpointer
bastile_operation_get_result (BastileOperation *op)
{
	g_return_val_if_fail (BASTILE_IS_OPERATION (op), NULL);
	return op->result;
}

/**
 * bastile_operation_wait:
 * @op: The operation to wait for
 *
 * Waits till the #BastileOperation finishes.
 *
 */
void                
bastile_operation_wait (BastileOperation *op)
{
	g_return_if_fail (BASTILE_IS_OPERATION (op));

	g_object_ref (op);
	bastile_util_wait_until (!bastile_operation_is_running (op));
	g_object_unref (op);
}

/**
 * bastile_operation_watch:
 * @operation: The operation to watch
 * @done_callback: callback when done
 * @donedata: data for this callback
 * @progress_callback: Callback when in progress mode
 * @progdata: data for this callback
 *
 * If the operation already finished, calls the done callback. Does progress
 * handling else.
 */
void
bastile_operation_watch (BastileOperation *operation, BastileDoneFunc done_callback,
                          gpointer donedata, BastileProgressFunc progress_callback, gpointer progdata)
{
    g_return_if_fail (BASTILE_IS_OPERATION (operation));

    if (!bastile_operation_is_running (operation)) {
        if (done_callback)
            (done_callback) (operation, donedata);
        return;
    }
    
    if (done_callback)
        g_signal_connect (operation, "done", G_CALLBACK (done_callback), donedata);
    
    if (progress_callback) {
        (progress_callback) (operation, 
                bastile_operation_get_message (operation),
                bastile_operation_get_progress (operation), progdata);
        g_signal_connect (operation, "progress", G_CALLBACK (progress_callback), progdata);
    }
}

/* METHODS FOR DERIVED CLASSES ---------------------------------------------- */

/**
 * bastile_operation_mark_start:
 * @op: a #BastileOperation
 *
 * Sets everything in the bastile operation to the start state
 *
 */
void
bastile_operation_mark_start (BastileOperation *op)
{
    g_return_if_fail (BASTILE_IS_OPERATION (op));
        
    /* A running operation always refs itself */
    g_object_ref (op);
    op->is_running = TRUE;
    op->is_done = FALSE;
    op->is_cancelled = FALSE;
    op->progress = -1;
    
    g_free (op->message);
    op->message = NULL;
}

/**
 * bastile_operation_mark_progress:
 * @op: A #BastileOperation to set
 * @message: The new message of the operation, Can be NULL
 * @progress: the new progress of the operation
 *
 * Sets the new message and the new progress. After a change
 * it emits the progress signal
 */
void 
bastile_operation_mark_progress (BastileOperation *op, const gchar *message, 
                                  gdouble progress)
{
    gboolean emit = FALSE;
    
    g_return_if_fail (BASTILE_IS_OPERATION (op));
    g_return_if_fail (op->is_running);

    if (progress != op->progress) {
        op->progress = progress;
        emit = TRUE;
    }

    if (!bastile_util_string_equals (op->message, message)) {
        g_free (op->message);
        op->message = message ? g_strdup (message) : NULL;
        emit = TRUE;
    }

    if (emit)    
        g_signal_emit (G_OBJECT (op), signals[PROGRESS], 0, op->message, progress);
}

/**
 * bastile_operation_mark_progress_full:
 * @op: The operation to set the progress for
 * @message: an optional message (can be NULL)
 * @current: the current value of the progress
 * @total: the max. value of the progress
 *
 *
 *
 */
void
bastile_operation_mark_progress_full (BastileOperation *op, const gchar *message,
                                       gdouble current, gdouble total)
{
    if (current > total)
        current = total;
    bastile_operation_mark_progress (op, message, total <= 0 ? -1 : current / total);
}


/**
 * bastile_operation_mark_done:
 * @op: a #BastileOperation
 * @cancelled: TRUE if this operation was cancelled
 * @error: An error to set
 *
 * Sets everything in the bastile operation to the end state
 *
 */
void                
bastile_operation_mark_done (BastileOperation *op, gboolean cancelled,
                              GError *error)
{
    g_return_if_fail (BASTILE_IS_OPERATION (op));
    g_return_if_fail (op->is_running);

    /* No message on completed operations */
    g_free (op->message);
    op->message = NULL;
    
    op->progress = cancelled ? -1 : 1.0;
    op->is_running = FALSE;
    op->is_done = TRUE;
    op->is_cancelled = cancelled;
    op->error = error;

    g_signal_emit (op, signals[PROGRESS], 0, NULL, op->progress);
    g_signal_emit (op, signals[DONE], 0);

    /* A running operation always refs itself */
    g_object_unref (op);
}

/**
 * bastile_operation_mark_result:
 * @op: A #BastileOperation
 * @result: a result
 * @destroy_func: a destroy function
 *
 * If @op already has a result and a destroy function, the function is called.
 * If there is none, it will be set to @result and @destroy_func
 */
void
bastile_operation_mark_result (BastileOperation *op, gpointer result,
                                GDestroyNotify destroy_func)
{
    if (op->result && op->result_destroy)
        (op->result_destroy) (op->result);
    op->result = result;
    op->result_destroy = destroy_func;
}

/* -----------------------------------------------------------------------------
 * MULTI OPERATION
 */

/* GObject handlers */
static void bastile_multi_operation_class_init (BastileMultiOperationClass *klass);
static void bastile_multi_operation_init       (BastileMultiOperation *mop);
static void bastile_multi_operation_dispose    (GObject *gobject);
static void bastile_multi_operation_finalize   (GObject *gobject);
static void bastile_multi_operation_cancel     (BastileOperation *operation);

G_DEFINE_TYPE (BastileMultiOperation, bastile_multi_operation, BASTILE_TYPE_OPERATION);

/* HELPERS ------------------------------------------------------------------ */

/**
* operation: the operation that contains progress and message
* message: the message, will be set by this function
* fract: ignored
* mop: a #BastileMultiOperation container
*
* Calculates the combined progress of all the contained operations, sets this
* as progress for @mop
*
**/
static void
multi_operation_progress (BastileOperation *operation, const gchar *message, 
                          gdouble fract, BastileMultiOperation *mop)
{
    GSList *list;
    
    g_assert (BASTILE_IS_MULTI_OPERATION (mop));
    g_assert (BASTILE_IS_OPERATION (operation));
    
    if (bastile_operation_is_cancelled (BASTILE_OPERATION (mop)))
        return;
    
    list = mop->operations;
    
    /* One or two operations, simple */
    if (g_slist_length (list) <= 1) {

        DEBUG_OPERATION (("[multi-operation 0x%08X] single progress: %s %lf\n", (guint)mop, 
            bastile_operation_get_message (operation), operation->progress));
        
        bastile_operation_mark_progress (BASTILE_OPERATION (mop), 
                            bastile_operation_get_message (operation), 
                            operation->progress);


    /* When more than one operation calculate the fraction */
    } else {
    
        BastileOperation *op;
        gdouble total = 0;
        gdouble current = 0;
        gdouble progress;
        
        message = bastile_operation_get_message (operation);

        /* Get the total progress */
        while (list) {
            op = BASTILE_OPERATION (list->data);
            list = g_slist_next (list);

            if (message && !message[0])
                message = NULL;
            
            if (!message && bastile_operation_is_running (op))
                message = bastile_operation_get_message (op);
            
            if (!bastile_operation_is_cancelled (op)) {
                total += 1;
                if (op->progress < 0)
                    current += (bastile_operation_is_running (op) ? 0 : 1);
                else
                    current += op->progress;
                DEBUG_OPERATION (("    progres is: %lf\n", op->progress));
            }
        } 
        
        progress = total ? current / total : -1;

        DEBUG_OPERATION (("[multi-operation 0x%08X] multi progress: %s %lf\n", (guint)mop, 
                          message, progress));
        
        bastile_operation_mark_progress (BASTILE_OPERATION (mop), message, progress);
    }
}

/**
* op: an operation that is done and will be cleaned
* mop: multi operation to process
*
* Sets a multi-operation to done. If one of the sub operations is running it
* just handles the progress.
*
**/
static void
multi_operation_done (BastileOperation *op, BastileMultiOperation *mop)
{
    GSList *l;
    gboolean done = TRUE;
    
    g_assert (BASTILE_IS_MULTI_OPERATION (mop));
    g_assert (BASTILE_IS_OPERATION (op));  
  
    g_signal_handlers_disconnect_by_func (op, multi_operation_done, mop);
    g_signal_handlers_disconnect_by_func (op, multi_operation_progress, mop);
    
    if (!bastile_operation_is_successful (op) && !BASTILE_OPERATION (mop)->error)
        bastile_operation_copy_error (op, &(BASTILE_OPERATION (mop)->error));

    DEBUG_OPERATION (("[multi-operation 0x%08X] part complete (%d): 0x%08X/%s\n", (guint)mop, 
                g_slist_length (mop->operations), (guint)op, bastile_operation_get_message (op)));
        
    /* Are we done with all of them? */
    for (l = mop->operations; l; l = g_slist_next (l)) {
        if (bastile_operation_is_running (BASTILE_OPERATION (l->data))) 
            done = FALSE;
    }
    
    /* Not done, just update progress. */
    if (!done) {
        multi_operation_progress (BASTILE_OPERATION (mop), NULL, -1, mop);
        return;
    }

    DEBUG_OPERATION (("[multi-operation 0x%08X] complete\n", (guint)mop));

    /* Remove all the listeners */
    for (l = mop->operations; l; l = g_slist_next (l)) {
        g_signal_handlers_disconnect_by_func (l->data, multi_operation_done, mop);
        g_signal_handlers_disconnect_by_func (l->data, multi_operation_progress, mop);
    }

    mop->operations = bastile_operation_list_purge (mop->operations);
    
    if (!bastile_operation_is_cancelled (BASTILE_OPERATION (mop))) 
        bastile_operation_mark_done (BASTILE_OPERATION (mop), FALSE, 
                                      BASTILE_OPERATION (mop)->error);
}

/* OBJECT ------------------------------------------------------------------- */

/**
* mop: The #BastileMultiOperation object to initialise
*
* Initializes @mop
*
**/
static void
bastile_multi_operation_init (BastileMultiOperation *mop)
{
    mop->operations = NULL;
}

/**
* gobject: A #BASTILE_MULTI_OPERATION
*
* dispose of all our internal references, frees the stored operations
*
**/
static void
bastile_multi_operation_dispose (GObject *gobject)
{
    BastileMultiOperation *mop;
    GSList *l;
    
    mop = BASTILE_MULTI_OPERATION (gobject);
    
    /* Remove all the listeners */
    for (l = mop->operations; l; l = g_slist_next (l)) {
        g_signal_handlers_disconnect_by_func (l->data, multi_operation_done, mop);
        g_signal_handlers_disconnect_by_func (l->data, multi_operation_progress, mop);
    }
    
    /* Anything remaining, gets released  */
    mop->operations = bastile_operation_list_free (mop->operations);
        
    G_OBJECT_CLASS (bastile_multi_operation_parent_class)->dispose (gobject);
}


/**
* gobject: a #BASTILE_MULTI_OPERATION
*
*
* finalizes the multi operation
*
**/
static void
bastile_multi_operation_finalize (GObject *gobject)

{
    BastileMultiOperation *mop;
    mop = BASTILE_MULTI_OPERATION (gobject);
    
    g_assert (mop->operations == NULL);
    
    G_OBJECT_CLASS (bastile_multi_operation_parent_class)->finalize (gobject);
}

/**
* operation: A #BastileMultiOperation
*
* Cancels a multi operation and all the operations within
*
**/
static void 
bastile_multi_operation_cancel (BastileOperation *operation)
{
    BastileMultiOperation *mop;
    
    g_assert (BASTILE_IS_MULTI_OPERATION (operation));
    mop = BASTILE_MULTI_OPERATION (operation);

    bastile_operation_mark_done (operation, TRUE, NULL);

    bastile_operation_list_cancel (mop->operations);
    mop->operations = bastile_operation_list_purge (mop->operations);
}

/**
* klass: The Class to init
*
* Initialises the multi-operation class. This is a container for several
* operations
*
**/
static void
bastile_multi_operation_class_init (BastileMultiOperationClass *klass)
{
    BastileOperationClass *op_class = BASTILE_OPERATION_CLASS (klass);
    GObjectClass *gobject_class;

    bastile_multi_operation_parent_class = g_type_class_peek_parent (klass);
    gobject_class = G_OBJECT_CLASS (klass);
    
    op_class->cancel = bastile_multi_operation_cancel;        
    gobject_class->dispose = bastile_multi_operation_dispose;
    gobject_class->finalize = bastile_multi_operation_finalize;
}

/* PUBLIC ------------------------------------------------------------------- */

/**
 * bastile_multi_operation_new:
 *
 * Creates a new multi operation
 *
 * Returns: the new multi operation
 */
BastileMultiOperation*  
bastile_multi_operation_new ()
{
    BastileMultiOperation *mop = g_object_new (BASTILE_TYPE_MULTI_OPERATION, NULL); 
    return mop;
}

/**
 * bastile_multi_operation_take:
 * @mop: the multi operation container
 * @op: an operation to add
 *
 * Adds @op to @mop
 *
 */
void
bastile_multi_operation_take (BastileMultiOperation* mop, BastileOperation *op)
{
    g_return_if_fail (BASTILE_IS_MULTI_OPERATION (mop));
    g_return_if_fail (BASTILE_IS_OPERATION (op));
    
    /* We should never add an operation to itself */
    g_return_if_fail (BASTILE_OPERATION (op) != BASTILE_OPERATION (mop));
    
    if(mop->operations == NULL) {
        DEBUG_OPERATION (("[multi-operation 0x%08X] start\n", (guint)mop));
        bastile_operation_mark_start (BASTILE_OPERATION (mop));
    }
    
    DEBUG_OPERATION (("[multi-operation 0x%08X] adding part: 0x%08X\n", (guint)mop, (guint)op));
    
    mop->operations = bastile_operation_list_add (mop->operations, op);
    bastile_operation_watch (op, (BastileDoneFunc)multi_operation_done, mop,
                              (BastileProgressFunc)multi_operation_progress, mop);
}

/* -----------------------------------------------------------------------------
 * OPERATION LIST FUNCTIONS
 */

/**
 * bastile_operation_list_add:
 * @list: a #GSList
 * @operation: a #BastileOperation to add to the lit
 *
 * Prepends the bastile operation to the list
 *
 * Returns: the list
 */
GSList*
bastile_operation_list_add (GSList *list, BastileOperation *operation)
{
    /* This assumes the main reference */
    return g_slist_prepend (list, operation);
}

/**
 * bastile_operation_list_remove:
 * @list: A list to remove an operation from
 * @operation: the operation to remove
 *
 * Removes an operation from the list
 *
 * Returns: The new list
 */
GSList*             
bastile_operation_list_remove (GSList *list, BastileOperation *operation)
{
   GSList *element;
   
   element = g_slist_find (list, operation);
   if (element) {
        g_object_unref (operation);
        list = g_slist_remove_link (list, element);
        g_slist_free (element);
   } 
   
   return list;
}

/**
 * bastile_operation_list_cancel:
 * @list: a list of Bastile operations
 *
 * Cancels every operation in the list
 *
 */
void             
bastile_operation_list_cancel (GSList *list)
{
    BastileOperation *operation;
    
    while (list) {
        operation = BASTILE_OPERATION (list->data);
        list = g_slist_next (list);
        
        if (bastile_operation_is_running (operation))
            bastile_operation_cancel (operation);
    }
}

/**
 * bastile_operation_list_purge:
 * @list: A list of operations
 *
 * Purges a list of Bastile operations
 *
 * Returns: the purged list
 */
GSList*             
bastile_operation_list_purge (GSList *list)
{
    GSList *l, *p;
    
    p = list;
    
    while (p != NULL) {

        l = p;
        p = g_slist_next (p);
        
        if (!bastile_operation_is_running (BASTILE_OPERATION (l->data))) {
            g_object_unref (G_OBJECT (l->data));

            list = g_slist_remove_link (list, l);
            g_slist_free (l);
        }
    }
    
    return list;
}

/**
 * bastile_operation_list_free:
 * @list: A #GSList of #BASTILE_OPERATION s
 *
 * Frees the list of bastile operations
 *
 * Returns: NULL
 */
GSList*
bastile_operation_list_free (GSList *list)
{
    GSList *l;
    
    for (l = list; l; l = g_slist_next (l)) {
        g_assert (BASTILE_IS_OPERATION (l->data));
        g_object_unref (G_OBJECT (l->data));
    }
    
    g_slist_free (list);
    return NULL;
}
