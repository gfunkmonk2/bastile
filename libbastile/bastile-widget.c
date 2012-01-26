/*
 * Bastile
 *
 * Copyright (C) 2002 Jacob Perkins
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

#include <config.h>

#include <string.h>

#include <glib/gi18n.h>

#include "bastile-widget.h"
#include "bastile-mateconf.h"
#include "bastile-gtkstock.h"

/**
 * SECTION:bastile-widget
 * @short_description: wrapping gtk-builder widgets to simplify usage.
 * @include:bastile-widget.h
 *
 **/

#define STATUS "status"

enum {
	PROP_0,
	PROP_NAME
};

enum {
	DESTROY,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void     class_init          (BastileWidgetClass    *klass);
static void     object_init         (BastileWidget         *swidget);

static void     object_dispose     (GObject                *gobject);
static void     object_finalize     (GObject                *gobject);

static void     object_set_property (GObject                *object,
                                     guint                  prop_id,
                                     const GValue           *value,
                                     GParamSpec             *pspec);

static void     object_get_property (GObject                *object,
                                     guint                  prop_id,
                                     GValue                 *value,
                                     GParamSpec             *pspec);
                                     
static GObject* bastile_widget_constructor (GType                  type, 
                                             guint                  n_props, 
                                             GObjectConstructParam* props);                    

/* signal functions */
G_MODULE_EXPORT void on_widget_closed   (GtkWidget             *widget,
                                              BastileWidget        *swidget);

G_MODULE_EXPORT void on_widget_help     (GtkWidget             *widget,
                                              BastileWidget        *swidget);

G_MODULE_EXPORT gboolean on_widget_delete_event  (GtkWidget             *widget,
                                                       GdkEvent              *event,
                                                       BastileWidget        *swidget);

static void     context_destroyed    (BastileContext       *sctx,
                                      BastileWidget        *swidget);

static GObjectClass *parent_class = NULL;

/* Hash of widgets with name as key */
static GHashTable *widgets = NULL;

/**
 * bastile_widget_get_type:
 *
 * Registers the widget type "BastileWidget"
 *
 * Returns: The widget identifier
 */
GType
bastile_widget_get_type (void)
{
	static GType widget_type = 0;
	
	if (!widget_type) {
		static const GTypeInfo widget_info = {
			sizeof (BastileWidgetClass), NULL, NULL,
			(GClassInitFunc) class_init,
			NULL, NULL, sizeof (BastileWidget), 0, (GInstanceInitFunc) object_init
		};

		widget_type = g_type_register_static (G_TYPE_OBJECT, "BastileWidget",
		                                      &widget_info, 0);
	}
	
	return widget_type;
}

/**
* klass: the #BastileWidgetClass
*
* Initialises the BastileWidgetClass
*
**/
static void
class_init (BastileWidgetClass *klass)
{
	GObjectClass *gobject_class;
	
	parent_class = g_type_class_peek_parent (klass);
	gobject_class = G_OBJECT_CLASS (klass);
	
	gobject_class->constructor = bastile_widget_constructor;
	gobject_class->dispose = object_dispose;
	gobject_class->finalize = object_finalize;
	gobject_class->set_property = object_set_property;
	gobject_class->get_property = object_get_property;

	g_object_class_install_property (gobject_class, PROP_NAME,
	           g_param_spec_string ("name", "Widget name", "Name of gtkbuilder file and main widget",
	                                NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	signals[DESTROY] = g_signal_new ("destroy", BASTILE_TYPE_WIDGET,
	                                 G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (BastileWidgetClass, destroy),
	                                 NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

/**
* sctx: ignored
* swidget: The swidget being destroyed
*
* Destroy widget when context is destroyed
*
**/
static void
context_destroyed (BastileContext *sctx, BastileWidget *swidget)
{
	bastile_widget_destroy (swidget);
}

/**
* swidget: The #BastileWidget being initialised
*
* Connects the destroy-signal
*
**/
static void
object_init (BastileWidget *swidget)
{
	g_signal_connect_after (SCTX_APP(), "destroy",
	                        G_CALLBACK (context_destroyed), swidget);
}

/**
* type: the #GType to construct
* n_props: number of properties
* props: properties
*
*
*
* Returns the initialised object
**/
static GObject*  
bastile_widget_constructor (GType type, guint n_props, GObjectConstructParam* props)
{
    BastileWidget *swidget;
    GObject *obj;
    
    GtkWindow *window;
    gint width, height;
    gchar *widthkey, *heightkey;
    
    obj = G_OBJECT_CLASS (parent_class)->constructor (type, n_props, props);
    swidget = BASTILE_WIDGET (obj);

    /* Load window size for windows that aren't dialogs */
    window = GTK_WINDOW (bastile_widget_get_toplevel (swidget));
    if (!GTK_IS_DIALOG (window)) {
	    widthkey = g_strdup_printf ("%s%s%s", WINDOW_SIZE, swidget->name, "_width");
	    width = bastile_mateconf_get_integer (widthkey);
    
	    heightkey = g_strdup_printf ("%s%s%s", WINDOW_SIZE, swidget->name, "_height");
	    height = bastile_mateconf_get_integer (heightkey);

	    if (width > 0 && height > 0)
		    gtk_window_resize (window, width, height);

	    g_free (widthkey);
	    g_free (heightkey);
    }
    
    return obj;
}

static void
object_dispose (GObject *object)
{
	BastileWidget *swidget = BASTILE_WIDGET (object);

	if (!swidget->in_destruction) {
		swidget->in_destruction = TRUE;
		g_signal_emit (swidget, signals[DESTROY], 0);
		swidget->in_destruction = FALSE;
	}

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

/**
* gobject: The #BastileWidget to finalize
*
* Disconnects callbacks, destroys main window widget,
* and frees the xml definition and any other data
*
**/
static void
object_finalize (GObject *gobject)
{
	BastileWidget *swidget;
	
	swidget = BASTILE_WIDGET (gobject);
	
	/* Remove widget from hash and destroy hash if empty */
    if (widgets) {
    	g_hash_table_remove (widgets, swidget->name);
    	if (g_hash_table_size (widgets) == 0) {
    		g_hash_table_destroy (widgets);
    		widgets = NULL;
    	}
    }

	g_signal_handlers_disconnect_by_func (SCTX_APP (), context_destroyed, swidget);
    if (bastile_widget_get_widget (swidget, swidget->name))
        gtk_widget_destroy (GTK_WIDGET (bastile_widget_get_widget (swidget, swidget->name)));
	
	g_object_unref (swidget->gtkbuilder);
	swidget->gtkbuilder = NULL;
	
	g_free (swidget->name);
	
	G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

/**
* object: the object to set the value for
* prop_id: the id of the property to set
* value: the value to set
* pspec: ignored
*
* Only property PROP_NAME is available so far
*
**/
static void
object_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    BastileWidget *swidget;
    GtkWidget *w;
    char *path;
    
    swidget = BASTILE_WIDGET (object);
    
    switch (prop_id) {
    /* Loads gtkbuilder xml definition from name, connects common callbacks */
    case PROP_NAME:
        g_return_if_fail (swidget->name == NULL);
        swidget->name = g_value_dup_string (value);
        path = g_strdup_printf ("%sbastile-%s.xml",
                                BASTILE_UIDIR, swidget->name);
        swidget->gtkbuilder = gtk_builder_new ();
        gtk_builder_add_from_file (swidget->gtkbuilder, path, NULL);
        g_free (path);
        g_return_if_fail (swidget->gtkbuilder != NULL);
        
        gtk_builder_connect_signals (swidget->gtkbuilder, swidget);
        
        w = GTK_WIDGET (bastile_widget_get_widget (swidget, swidget->name));
        /*TODO: glade_xml_ensure_accel (swidget->gtkbuilder);*/
        
        gtk_window_set_icon_name (GTK_WINDOW (w), "bastile");
        break;
    }
}

/**
* object: the object to get the property for
* prop_id: ID of the property
* value: Value to return
* pspec: ignored
*
* Only the property PROP_NAME is available so far
*
**/
static void
object_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)

{
	BastileWidget *swidget;
	swidget = BASTILE_WIDGET (object);
	
	switch (prop_id) {
		case PROP_NAME:
			g_value_set_string (value, swidget->name);
			break;
		default:
			break;
	}
}

/**
 * on_widget_help:
 * @widget: The widget
 * @swidget: The #BastileWidget
 *
 * Shows help to the widget
 *
 */
G_MODULE_EXPORT void
on_widget_help (GtkWidget *widget, BastileWidget *swidget)
{
    bastile_widget_show_help (swidget);
}


/**
 * on_widget_closed:
 * @widget: ignored
 * @swidget: The #BastileWidget
 *
 * Closes the widget, calls destroy-function
 *
 */
/* Destroys widget */
G_MODULE_EXPORT void
on_widget_closed (GtkWidget *widget, BastileWidget *swidget)
{
	bastile_widget_destroy (swidget);
}


/**
 * on_widget_delete_event:
 * @widget: the widget being deleted
 * @event: ignored
 * @swidget: the #BastileWidget
 *
 *
 *
 * Returns: FALSE
 */
/* Closed widget */
G_MODULE_EXPORT gboolean
on_widget_delete_event (GtkWidget *widget, GdkEvent *event, BastileWidget *swidget)
{
	on_widget_closed (widget, swidget);
    return FALSE; /* propogate event */
}

/**
 * bastile_widget_new:
 * @name: Name of widget, filename part of gtkbuilder file, and name of main window
 * @parent: GtkWindow to make the parent of the new swidget
 *
 * Creates a new #BastileWidget. Date is read from the gtk-builder file
 * bastile-%name%.xml
 *
 * Returns: The new #BastileWidget, or NULL if the widget already exists
 **/
BastileWidget*
bastile_widget_new (const gchar *name, GtkWindow *parent)
{
        /* Check if have widget hash */
    BastileWidget *swidget = bastile_widget_find (name);
    GtkWindow *window;
    
    /* If widget already exists, present */
    if (swidget != NULL) {
        gtk_window_present (GTK_WINDOW (bastile_widget_get_widget (swidget, swidget->name)));
        return NULL;
    }

    /* If widget doesn't already exist, create & insert into hash */
    swidget = g_object_new (BASTILE_TYPE_WIDGET, "name", name, NULL);
    if(!widgets)
        widgets = g_hash_table_new ((GHashFunc)g_str_hash, (GCompareFunc)g_str_equal);
    g_hash_table_insert (widgets, g_strdup (name), swidget);
    
    if (parent != NULL) {
        window = GTK_WINDOW (bastile_widget_get_widget (swidget, swidget->name));
        gtk_window_set_transient_for (window, parent);
    }

    return swidget;
}

/**
 * bastile_widget_new_allow_multiple:
 * @name: Name of widget, filename part of gtkbuilder file, and name of main window
 * @parent: GtkWindow to make the parent of the new swidget
 *
 * Creates a new #BastileWidget without checking if it already exists.
 *
 * Returns: The new #BastileWidget
 **/
BastileWidget*
bastile_widget_new_allow_multiple (const gchar *name, GtkWindow *parent)
{
	GtkWindow *window;
	BastileWidget *swidget;

	swidget = g_object_new (BASTILE_TYPE_WIDGET, "name", name,  NULL);

	if (parent != NULL) {
		window = GTK_WINDOW (bastile_widget_get_widget (swidget, swidget->name));
		gtk_window_set_transient_for (window, parent);
	}

	gtk_builder_connect_signals (swidget->gtkbuilder, NULL);
	return swidget;
}

/**
 * bastile_widget_find:
 * @name: The name to look for
 *
 * Returns: The widget with the @name or NULL if not found
 */
BastileWidget*
bastile_widget_find (const gchar *name)

{
    /* Check if have widget hash */
    if (widgets != NULL)
        return BASTILE_WIDGET (g_hash_table_lookup (widgets, name));
    return NULL;
}

/**
 * bastile_widget_show_help:
 * @swidget: The #BastileWidget.
 * 
 * Show help appropriate for the top level widget.
 */
void
bastile_widget_show_help (BastileWidget *swidget)
{
    GError *error = NULL;
    gchar *document = NULL;
    GtkWidget *dialog = NULL;

    if (g_str_equal (swidget->name, "key-manager") || 
        g_str_equal (swidget->name, "keyserver-results")) {
        document = g_strdup ("ghelp:" PACKAGE "?introduction");
    } else {
        document = g_strdup_printf ("ghelp:" PACKAGE "?%s", swidget->name);
    }

    if (!g_app_info_launch_default_for_uri (document, NULL, &error)) {
        dialog = gtk_message_dialog_new (GTK_WINDOW (bastile_widget_get_toplevel (swidget)),
                                         GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                         _("Could not display help: %s"), error->message);
        g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (gtk_widget_destroy), NULL);
        gtk_widget_show (dialog);
    }

    g_free (document);

    if (error)
        g_error_free (error);
}

/**
 * bastile_widget_get_name:
 * @swidget: The widget to get the name for
 *
 * Returns: The name of the widget
 */
const gchar*
bastile_widget_get_name (BastileWidget   *swidget)
{
	g_return_val_if_fail (BASTILE_IS_WIDGET (swidget), NULL);
	return swidget->name;
}

/**
 * bastile_widget_get_toplevel:
 * @swidget: The bastile widget
 * 
 * Return the top level widget in this bastile widget
 *
 * Returns: The top level widget
 **/
GtkWidget*      
bastile_widget_get_toplevel (BastileWidget     *swidget)
{
    GtkWidget *widget = GTK_WIDGET (bastile_widget_get_widget (swidget, swidget->name));
    g_return_val_if_fail (widget != NULL, NULL);
    return widget;
}

/**
 * bastile_widget_get_widget:
 * @swidget: a #BastileWidget
 * @identifier: the name of the widget to get
 *
 * Returns: the widget named @identifier in @swidget
 */
GtkWidget*
bastile_widget_get_widget (BastileWidget *swidget, const char *identifier)
{
    GtkWidget *widget = GTK_WIDGET (gtk_builder_get_object (swidget->gtkbuilder, identifier));
    if (widget == NULL)
	    g_warning ("could not find widget %s for bastile-%s.xml", identifier, swidget->name);
    return widget;
}

/**
 * bastile_widget_show:
 * @swidget: #BastileWidget to show
 * 
 * Show the toplevel widget in the gtkbuilder file.
 **/
void
bastile_widget_show (BastileWidget *swidget)
{
    GtkWidget *widget;

    widget = GTK_WIDGET (bastile_widget_get_widget (swidget, swidget->name));
    g_return_if_fail (widget != NULL);
    gtk_widget_show (widget);
}
 
/**
 * bastile_widget_set_visible:
 * @swidget: the #BastileWidget
 * @identifier: The identifier of the widget to set visibility for
 * @visible: TRUE to show the widget, FALSE to hide it
 *
 */
void             
bastile_widget_set_visible (BastileWidget *swidget, const char *identifier,
                             gboolean visible)

{
    GtkWidget *widget = GTK_WIDGET (bastile_widget_get_widget (swidget, identifier));
    g_return_if_fail (widget != NULL);
    
    if (visible)
        gtk_widget_show (widget);
    else
        gtk_widget_hide (widget);
}

/**
 * bastile_widget_set_sensitive:
 * @swidget: the #BastileWidget to find the widget @identifier in
 * @identifier: the name of the widget to set sensitive
 * @sensitive: TRUE if the widget should be switched to sensitive
 *
 * Sets the widget @identifier 's sensitivity to @sensitive
 */
void             
bastile_widget_set_sensitive (BastileWidget *swidget, const char *identifier,
                               gboolean sensitive)
{
    GtkWidget *widget = GTK_WIDGET (bastile_widget_get_widget (swidget, identifier));
    g_return_if_fail (widget != NULL);
    gtk_widget_set_sensitive (widget, sensitive);
}

/**
 * bastile_widget_destroy:
 * @swidget: #BastileWidget to destroy
 *
 * Unrefs @swidget.
 **/
void
bastile_widget_destroy (BastileWidget *swidget)
{
    GtkWidget *widget;
    gchar *widthkey, *heightkey;
    gint width, height;

    g_return_if_fail (swidget != NULL && BASTILE_IS_WIDGET (swidget));
    widget = bastile_widget_get_toplevel (swidget);
    
    /* Don't save window size for dialogs */
    if (!GTK_IS_DIALOG (widget)) {

	    /* Save window size */
	    gtk_window_get_size (GTK_WINDOW (widget), &width, &height);
    
	    widthkey = g_strdup_printf ("%s%s%s", WINDOW_SIZE, swidget->name, "_width");
	    bastile_mateconf_set_integer (widthkey, width);
    
	    heightkey = g_strdup_printf ("%s%s%s", WINDOW_SIZE, swidget->name, "_height");
	    bastile_mateconf_set_integer (heightkey, height);
    
	    g_free (widthkey);
	    g_free (heightkey);
    }
    
    /* Destroy Widget */
    if (!swidget->destroying) {
        swidget->destroying = TRUE;
        gtk_widget_destroy (bastile_widget_get_toplevel (swidget));
        g_object_unref (swidget);
    }
}
