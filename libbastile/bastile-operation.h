/*
 * Bastile
 *
 * Copyright (C) 2004 Stefan Walter
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

#ifndef __BASTILE_OPERATION_H__
#define __BASTILE_OPERATION_H__

#include <glib-object.h>
#include <gtk/gtk.h>

#define BASTILE_TYPE_OPERATION            (bastile_operation_get_type ())
#define BASTILE_OPERATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_OPERATION, BastileOperation))
#define BASTILE_OPERATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_OPERATION, BastileOperationClass))
#define BASTILE_IS_OPERATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_OPERATION))
#define BASTILE_IS_OPERATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_OPERATION))
#define BASTILE_OPERATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_OPERATION, BastileOperationClass))

struct _BastileOperation;

/**
 * BastileOperation:
 * @parent: The parent #GObject
 * @message: Progress status details ie: "foobar.jpg"
 * @progress: The current progress position, -1 for indeterminate
 * @is_running: If the operation is running or not
 * @is_done: Operation is done or not
 * @is_cancelled: Operation is cancelled or not
 * @error: #GError for the operation
 *
 * An operation taking place over time.
 *
 * - Generally this class is derived and a base class actually hooks in and
 *   performs the operation, keeps the properties updated etc...
 * - Used all over to represent things like key loading operations, search
 * - BastileMultiOperation allows you to combine multiple operations into
 *   a single one. Used when searching multiple key servers for example.
 * - Can be tied to a progress bar (see bastile-progress.h)
 * - Holds a reference to itself while the operation is in progress.
 * - The bastile_operation_mark_* are used by derived classes to update
 *   properties of the operation as things progress.
 *
 * Signals:
 *   done: The operation is complete.
 *   progress: The operation has progressed, or changed state somehow.
 *
 * Properties:
 *   result: The 'result' of the operation (if applicable).
 *           This depends on the derived operation class.
 *   progress: A fraction between 0.0 and 1.0 inclusive representing how far
 *           along this operation is. 0.0 = indeterminate, and 1.0 is done.
 *   message: A progress message to display to the user.
 */

typedef struct _BastileOperation {
    GObject parent;
    
    /*< public >*/
    gchar *message;                /* Progress status details ie: "foobar.jpg" */
    gdouble progress;              /* The current progress position, -1 for indeterminate */
    
    guint is_running : 1;          /* If the operation is running or not */
    guint is_done : 1;             /* Operation is done or not */
    guint is_cancelled : 1;        /* Operation is cancelled or not */;

    GError *error;
    
    /*< private> */
    gpointer result;
    GDestroyNotify result_destroy;

} BastileOperation;

typedef struct _BastileOperationClass {
    GObjectClass parent_class;

    /* signals --------------------------------------------------------- */
    
    /* Signal that occurs when the operation is complete */
    void (*done) (BastileOperation *operation);

    /* Signal that occurs progress or status changes */
    void (*progress) (BastileOperation *operation, const gchar *status, gdouble progress);

    /* virtual methods ------------------------------------------------- */

    /* Cancels a pending operation */
    void (*cancel) (BastileOperation *operation);
    
} BastileOperationClass;

GType       bastile_operation_get_type      (void);

/* Methods ------------------------------------------------------------ */

/* Assumes ownership of |err| */
BastileOperation*  bastile_operation_new_complete (GError *err);

BastileOperation*  bastile_operation_new_cancelled ();

void                bastile_operation_cancel      (BastileOperation *operation);

#define             bastile_operation_is_running(operation) \
                                                   ((operation)->is_running) 
                                                  
#define             bastile_operation_is_cancelled(operation) \
                                                   ((operation)->is_cancelled)
                                                  
#define             bastile_operation_is_successful(operation) \
                                                   ((operation)->error == NULL)                                                                 

void                bastile_operation_copy_error  (BastileOperation *operation,
                                                    GError **err);

const GError*       bastile_operation_get_error   (BastileOperation *operation);

void                bastile_operation_display_error (BastileOperation *operation, 
                                                      const gchar *title,
                                                      GtkWidget *parent);

void                bastile_operation_wait        (BastileOperation *operation);

typedef void (*BastileDoneFunc) (BastileOperation *op, gpointer userdata);
typedef void (*BastileProgressFunc) (BastileOperation *op, const gchar *status, 
                                      gdouble progress, gpointer userdata);

/* When called on an already complete operation, the callbacks are called immediately */
void                bastile_operation_watch       (BastileOperation *operation,
                                                    BastileDoneFunc done_callback,
                                                    gpointer donedata,
                                                    BastileProgressFunc progress_callback,
                                                    gpointer progdata);

#define             bastile_operation_get_progress(op) \
                                                   ((op)->progress)

#define             bastile_operation_get_message(operation) \
                                                   ((const gchar*)((operation)->message))

gpointer            bastile_operation_get_result  (BastileOperation *operation);
                                                      
/* Helpers for Tracking Operation Lists ------------------------------ */

GSList*             bastile_operation_list_add    (GSList *list,
                                                    BastileOperation *operation);

GSList*             bastile_operation_list_remove (GSList *list,
                                                    BastileOperation *operation);

void                bastile_operation_list_cancel (GSList *list);

GSList*             bastile_operation_list_purge  (GSList *list);
                                                    
GSList*             bastile_operation_list_free   (GSList *list);
                           
/* Combining parallel operations ------------------------------------- */

#define BASTILE_TYPE_MULTI_OPERATION            (bastile_multi_operation_get_type ())
#define BASTILE_MULTI_OPERATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_MULTI_OPERATION, BastileMultiOperation))
#define BASTILE_MULTI_OPERATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_MULTI_OPERATION, BastileMultiOperationClass))
#define BASTILE_IS_MULTI_OPERATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_MULTI_OPERATION))
#define BASTILE_IS_MULTI_OPERATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_MULTI_OPERATION))
#define BASTILE_MULTI_OPERATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_MULTI_OPERATION, BastileMultiOperationClass))

typedef struct _BastileMultiOperation {
    BastileOperation parent;
    
    /*< public >*/
    GSList *operations;
    
} BastileMultiOperation;

typedef struct _BastileMultiOperationClass {
    BastileOperationClass parent_class;
} BastileMultiOperationClass;

GType                    bastile_multi_operation_get_type  ();

BastileMultiOperation*  bastile_multi_operation_new       ();

void                     bastile_multi_operation_take      (BastileMultiOperation* mop,
                                                             BastileOperation *op);

/* ----------------------------------------------------------------------------
 *  SUBCLASS DECLARATION MACROS 
 */
 
/* 
 * To declare a subclass you need to put in code like this:
 *
 
#define BASTILE_TYPE_XX_OPERATION            (bastile_xx_operation_get_type ())
#define BASTILE_XX_OPERATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_XX_OPERATION, BastileXxOperation))
#define BASTILE_XX_OPERATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_XX_OPERATION, BastileXxOperationClass))
#define BASTILE_IS_XX_OPERATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_XX_OPERATION))
#define BASTILE_IS_XX_OPERATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_XX_OPERATION))
#define BASTILE_XX_OPERATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_XX_OPERATION, BastileXxOperationClass))

DECLARE_OPERATION (Xx, xx)
   ... member vars here ...
END_DECLARE_OPERATION 

 *
 * And then in your implementation something like this 
 *
 
IMPLEMENT_OPERATION (Xx, xx)
 
static void 
bastile_xx_operation_init (BastileXxOperation *lop)
{

}

static void 
bastile_xx_operation_dispose (GObject *gobject)
{
    G_OBJECT_CLASS (operation_parent_class)->dispose (gobject);  
}

static void 
bastile_xx_operation_finalize (GObject *gobject)
{
    G_OBJECT_CLASS (operation_parent_class)->finalize (gobject);  
}

static void 
bastile_xx_operation_cancel (BastileOperation *operation)
{
    bastile_operation_mark_done (operation, TRUE, NULL);
}
 
 *
 * 
 */
 
#define DECLARE_OPERATION(Opx, opx)                                            \
    typedef struct _Bastile##Opx##Operation Bastile##Opx##Operation;              \
    typedef struct _Bastile##Opx##OperationClass Bastile##Opx##OperationClass;    \
    GType          bastile_##opx##_operation_get_type      (void);                 \
    struct _Bastile##Opx##OperationClass {                                         \
        BastileOperationClass parent_class;                                        \
    };                                                                              \
    struct _Bastile##Opx##Operation {                                              \
        BastileOperation parent;                                                   \

#define END_DECLARE_OPERATION                                                       \
    };                                                                              \

#define IMPLEMENT_OPERATION_EX(Opx, opx)                                                        \
    static void bastile_##opx##_operation_class_init (Bastile##Opx##OperationClass *klass);   \
    static void bastile_##opx##_operation_init       (Bastile##Opx##Operation *lop);          \
    static void bastile_##opx##_operation_dispose    (GObject *gobject);                       \
    static void bastile_##opx##_operation_finalize   (GObject *gobject);                       \
    static void bastile_##opx##_operation_cancel     (BastileOperation *operation);           \
    GType bastile_##opx##_operation_get_type (void) {                                          \
        static GType operation_type = 0;                                                        \
        if (!operation_type) {                                                                  \
            static const GTypeInfo operation_info = {                                           \
                sizeof (Bastile##Opx##OperationClass), NULL, NULL,                             \
                (GClassInitFunc) bastile_##opx##_operation_class_init, NULL, NULL,             \
                sizeof (Bastile##Opx##Operation), 0,                                           \
                (GInstanceInitFunc)bastile_##opx##_operation_init                              \
            };                                                                                  \
            operation_type = g_type_register_static (BASTILE_TYPE_OPERATION,                   \
                                "Bastile" #Opx "Operation", &operation_info, 0);               \
        }                                                                                       \
        return operation_type;                                                                  \
    }                                                                                           \
    static GObjectClass *opx##_operation_parent_class = NULL;                                         \
    static void bastile_##opx##_operation_class_init (Bastile##Opx##OperationClass *klass) {  \
        GObjectClass *gobject_class  = G_OBJECT_CLASS (klass);                                  \
        BastileOperationClass *op_class = BASTILE_OPERATION_CLASS (klass);                    \
        opx##_operation_parent_class = g_type_class_peek_parent (klass);                              \
        op_class->cancel = bastile_##opx##_operation_cancel;                                   \
        gobject_class->dispose = bastile_##opx##_operation_dispose;                            \
        gobject_class->finalize = bastile_##opx##_operation_finalize;                          \
        
#define END_IMPLEMENT_OPERATION_EX                                                              \
    }                                                                                           \

#define IMPLEMENT_OPERATION_PROPS(Opx, opx)                                                     \
    static void bastile_##opx##_operation_set_property (GObject *gobject, guint prop_id,       \
                        const GValue *value, GParamSpec *pspec);                                \
    static void bastile_##opx##_operation_get_property (GObject *gobject, guint prop_id,       \
                        GValue *value, GParamSpec *pspec);                                      \
    IMPLEMENT_OPERATION_EX(Opx, opx)                                                            \
        gobject_class->set_property = bastile_##opx##_operation_set_property;                  \
        gobject_class->get_property = bastile_##opx##_operation_get_property;                  \
    
#define END_IMPLEMENT_OPERATION_PROPS                                                           \
    END_IMPLEMENT_OPERATION_EX                                                                  \
    
#define IMPLEMENT_OPERATION(Opx, opx)                                                           \
    IMPLEMENT_OPERATION_EX(Opx, opx)                                                            \
    END_IMPLEMENT_OPERATION_EX                                                                  \


/* Helpers for Derived ----------------------------------------------- */

#define BASTILE_CALC_PROGRESS(cur, tot)
    
void              bastile_operation_mark_start         (BastileOperation *operation);

/* Takes ownership of |error| */
void              bastile_operation_mark_done          (BastileOperation *operation,
                                                         gboolean cancelled, GError *error);

void              bastile_operation_mark_progress      (BastileOperation *operation,
                                                         const gchar *message,
                                                         gdouble progress);

void              bastile_operation_mark_progress_full (BastileOperation *operation,
                                                         const gchar *message,
                                                         gdouble current, gdouble total);

void              bastile_operation_mark_result        (BastileOperation *operation,
                                                         gpointer result, GDestroyNotify notify_func);

#endif /* __BASTILE_OPERATION_H__ */
