/* 
 * Bastile
 * 
 * Copyright (C) 2008 Stefan Walter
 * 
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *  
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.  
 */

#include "config.h"

#include "bastile-keyserver-results.h"
#include "bastile-key-manager-store.h"
#include "bastile-windows.h"

#include "bastile-operation.h"
#include "bastile-progress.h"

#include <glib/gi18n.h>

#include <string.h>

enum {
	PROP_0,
	PROP_SEARCH,
	PROP_SELECTED
};

struct _BastileKeyserverResultsPrivate {
	char *search_string;
	GtkTreeView *view;
	GtkActionGroup *object_actions;
	BastileKeyManagerStore *store;
	BastileSet *objects;
	BastileObjectPredicate pred;
};

G_DEFINE_TYPE (BastileKeyserverResults, bastile_keyserver_results, BASTILE_TYPE_VIEWER);

/* -----------------------------------------------------------------------------
 * INTERNAL 
 */


static gboolean 
on_filter_objects (BastileObject *obj, BastileKeyserverResults *self) 
{
	const gchar *match;
	gboolean ret = FALSE;
	gchar* value;
	gsize n_value, n_match;

	g_return_val_if_fail (BASTILE_IS_KEYSERVER_RESULTS (self), FALSE);
	g_return_val_if_fail (BASTILE_IS_OBJECT (obj), FALSE);

	match = self->pv->search_string;
	if (g_utf8_strlen (match, -1) == 0)
		ret = TRUE;

	/* Match against the label */
	if (ret != TRUE) {
		value = g_utf8_casefold (bastile_object_get_label (obj), -1);
		ret = strstr (value, match) != NULL;
		g_free (value);
	}
	
	/* Match against the key identifier */
	if (ret != TRUE) {
		if (strncmp (match, "0x", 2) == 0)
			match += 2;
		value = g_utf8_casefold (bastile_object_get_identifier (obj), -1);
		
		/* Only compare as many bytes as exist in the key id */
		n_value = strlen (value);
		n_match = strlen (match);
		if (n_value > n_match)
			match += (n_value - n_match);
		ret = strstr (value, match) != NULL;
		
		g_free (value);
	}
	
	return ret;
}


static gboolean 
fire_selection_changed (BastileKeyserverResults* self) 
{
	gint rows;
	GtkTreeSelection* selection;
	g_return_val_if_fail (BASTILE_IS_KEYSERVER_RESULTS (self), FALSE);

	selection = gtk_tree_view_get_selection (self->pv->view);
	rows = gtk_tree_selection_count_selected_rows (selection);
	bastile_viewer_set_numbered_status (BASTILE_VIEWER (self), ngettext ("Selected %d key", "Selected %d keys", rows), rows);
	gtk_action_group_set_sensitive (self->pv->object_actions, rows > 0);
	g_signal_emit_by_name (G_OBJECT (BASTILE_VIEW (self)), "selection-changed");
	return FALSE;
}

/**
* selection:
* self: the results object
*
* Adds fire_selection_changed as idle function
*
**/
static void 
on_view_selection_changed (GtkTreeSelection *selection, BastileKeyserverResults *self) 
{
	g_return_if_fail (BASTILE_IS_KEYSERVER_RESULTS (self));
	g_return_if_fail (GTK_IS_TREE_SELECTION (selection));
	g_idle_add ((GSourceFunc)fire_selection_changed, self);
}

static void 
on_row_activated (GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *column, BastileKeyserverResults *self) 
{
	BastileObject *obj;

	g_return_if_fail (BASTILE_IS_KEYSERVER_RESULTS (self));
	g_return_if_fail (GTK_IS_TREE_VIEW (view));
	g_return_if_fail (path != NULL);
	g_return_if_fail (GTK_IS_TREE_VIEW_COLUMN (column));

	obj = bastile_key_manager_store_get_object_from_path (view, path);
	if (obj != NULL) 
		bastile_viewer_show_properties (BASTILE_VIEWER (self), obj);
}

G_MODULE_EXPORT gboolean 
on_key_list_button_pressed (GtkTreeView* view, GdkEventButton* event, BastileKeyserverResults* self) 
{
	g_return_val_if_fail (BASTILE_IS_KEYSERVER_RESULTS (self), FALSE);
	g_return_val_if_fail (GTK_IS_TREE_VIEW (view), FALSE);
	if (event->button == 3)
		bastile_viewer_show_context_menu (BASTILE_VIEWER (self), event->button, event->time);
	return FALSE;
}

G_MODULE_EXPORT gboolean 
on_key_list_popup_menu (GtkTreeView* view, BastileKeyserverResults* self) 
{
	BastileObject* key;

	g_return_val_if_fail (BASTILE_IS_KEYSERVER_RESULTS (self), FALSE);
	g_return_val_if_fail (GTK_IS_TREE_VIEW (view), FALSE);

	key = bastile_viewer_get_selected (BASTILE_VIEWER (self));
	if (key == NULL)
		return FALSE;
	bastile_viewer_show_context_menu (BASTILE_VIEWER (self), 0, gtk_get_current_event_time ());
	return TRUE;
}

/**
* action:
* self: The result object to expand the nodes in
*
* Expands all the nodes in the view of the object
*
**/
static void 
on_view_expand_all (GtkAction* action, BastileKeyserverResults* self) 
{
	g_return_if_fail (BASTILE_IS_KEYSERVER_RESULTS (self));
	g_return_if_fail (GTK_IS_ACTION (action));
	gtk_tree_view_expand_all (self->pv->view);
}


/**
* action:
* self: The result object to collapse the nodes in
*
* Collapses all the nodes in the view of the object
*
**/
static void 
on_view_collapse_all (GtkAction* action, BastileKeyserverResults* self) 

{
	g_return_if_fail (BASTILE_IS_KEYSERVER_RESULTS (self));
	g_return_if_fail (GTK_IS_ACTION (action));
	gtk_tree_view_collapse_all (self->pv->view);
}

/**
* action: the closing action or NULL
* self: The BastileKeyServerResults widget to destroy
*
* destroys the widget
*
**/
static void 
on_app_close (GtkAction* action, BastileKeyserverResults* self) 
{
	g_return_if_fail (BASTILE_IS_KEYSERVER_RESULTS (self));
	g_return_if_fail (action == NULL || GTK_IS_ACTION (action));
	bastile_widget_destroy (BASTILE_WIDGET (self));
}

static void 
on_remote_find (GtkAction* action, BastileKeyserverResults* self) 
{
	g_return_if_fail (BASTILE_IS_KEYSERVER_RESULTS (self));
	g_return_if_fail (GTK_IS_ACTION (action));
	bastile_keyserver_search_show (bastile_viewer_get_window (BASTILE_VIEWER (self)));
}



/**
* widget: sending widget
* event: ignored
* self: The BastileKeyserverResults widget to destroy
*
* When this window closes we quit bastile
*
* Returns TRUE on success
**/
static gboolean 
on_delete_event (GtkWidget* widget, GdkEvent* event, BastileKeyserverResults* self) 
{
	g_return_val_if_fail (BASTILE_IS_KEYSERVER_RESULTS (self), FALSE);
	g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
	on_app_close (NULL, self);
	return TRUE;
}

static const GtkActionEntry GENERAL_ENTRIES[] = {
	{ "remote-menu", NULL, N_("_Remote") }, 
	{ "app-close", GTK_STOCK_CLOSE, NULL, "<control>W", 
	  N_("Close this window"), G_CALLBACK (on_app_close) }, 
	{ "view-expand-all", GTK_STOCK_ADD, N_("_Expand All"), NULL, 
	  N_("Expand all listings"), G_CALLBACK (on_view_expand_all) }, 
	{ "view-collapse-all", GTK_STOCK_REMOVE, N_("_Collapse All"), NULL, 
	  N_("Collapse all listings"), G_CALLBACK (on_view_collapse_all) } 
};

static const GtkActionEntry SERVER_ENTRIES[] = {
	{ "remote-find", GTK_STOCK_FIND, N_("_Find Remote Keys..."), "", 
	  N_("Search for keys on a key server"), G_CALLBACK (on_remote_find) }
};

/* -----------------------------------------------------------------------------
 * OBJECT 
 */

static GList* 
bastile_keyserver_results_get_selected_objects (BastileViewer* base) 
{
	BastileKeyserverResults * self = BASTILE_KEYSERVER_RESULTS (base);
	return bastile_key_manager_store_get_selected_objects (self->pv->view);
}

static void 
bastile_keyserver_results_set_selected_objects (BastileViewer* base, GList* keys) 
{
	BastileKeyserverResults * self = BASTILE_KEYSERVER_RESULTS (base);
	bastile_key_manager_store_set_selected_objects (self->pv->view, keys);
}

static BastileObject* 
bastile_keyserver_results_get_selected (BastileViewer* base) 
{
	BastileKeyserverResults* self;
	self = BASTILE_KEYSERVER_RESULTS (base);
	return bastile_key_manager_store_get_selected_object (self->pv->view);
}

static void 
bastile_keyserver_results_set_selected (BastileViewer* base, BastileObject* value) 
{
	BastileKeyserverResults* self = BASTILE_KEYSERVER_RESULTS (base);
	GList* keys = NULL;

	if (value != NULL)
		keys = g_list_prepend (keys, value);

	bastile_viewer_set_selected_objects (BASTILE_VIEWER (self), keys);
	g_list_free (keys);;
	g_object_notify (G_OBJECT (self), "selected");
}


/**
* type: The type identifying this object
* n_props: Number of properties
* props: Properties
*
* Creates a new BastileKeyserverResults object, shows the resulting window
*
* Returns The BastileKeyserverResults object as GObject
**/
static GObject* 
bastile_keyserver_results_constructor (GType type, guint n_props, GObjectConstructParam *props) 
{
	BastileKeyserverResults *self = BASTILE_KEYSERVER_RESULTS (G_OBJECT_CLASS (bastile_keyserver_results_parent_class)->constructor(type, n_props, props));
	GtkActionGroup* actions;
	GtkTreeSelection *selection;
	GtkWindow *window;
	char* title;

	g_return_val_if_fail (self, NULL);	


	if (g_utf8_strlen (self->pv->search_string, -1) == 0) {
		title = g_strdup (_("Remote Keys"));
	} else {
		title = g_strdup_printf (_ ("Remote Keys Containing '%s'"), self->pv->search_string);
	}

	window = bastile_viewer_get_window (BASTILE_VIEWER (self));
	gtk_window_set_title (window, title);
	g_free (title);
	
	g_signal_connect (window, "delete-event", G_CALLBACK (on_delete_event), self);
	
	actions = gtk_action_group_new ("general");
	gtk_action_group_set_translation_domain (actions, GETTEXT_PACKAGE);
	gtk_action_group_add_actions (actions, GENERAL_ENTRIES, G_N_ELEMENTS (GENERAL_ENTRIES), self);
	bastile_viewer_include_actions (BASTILE_VIEWER (self), actions);

	actions = gtk_action_group_new ("keyserver");
	gtk_action_group_set_translation_domain (actions, GETTEXT_PACKAGE);
	gtk_action_group_add_actions (actions, SERVER_ENTRIES, G_N_ELEMENTS (SERVER_ENTRIES), self);
	bastile_viewer_include_actions (BASTILE_VIEWER (self), actions);

	/* init key list & selection settings */
	self->pv->view = GTK_TREE_VIEW (bastile_widget_get_widget (BASTILE_WIDGET (self), "key_list"));
	selection = gtk_tree_view_get_selection (self->pv->view);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
	g_signal_connect_object (selection, "changed", G_CALLBACK (on_view_selection_changed), self, 0);
	g_signal_connect_object (self->pv->view, "row-activated", G_CALLBACK (on_row_activated), self, 0);
	g_signal_connect_object (self->pv->view, "button-press-event", G_CALLBACK (on_key_list_button_pressed), self, 0);
	g_signal_connect_object (self->pv->view, "popup-menu", G_CALLBACK (on_key_list_popup_menu), self, 0);
	gtk_widget_realize (GTK_WIDGET (self->pv->view));
	
	/* Set focus to the current key list */
	gtk_widget_grab_focus (GTK_WIDGET (self->pv->view));
	
	/* To avoid flicker */
	bastile_viewer_ensure_updated (BASTILE_VIEWER (self));
	bastile_widget_show (BASTILE_WIDGET (BASTILE_VIEWER (self)));
	
	/* Our predicate for filtering keys */
	self->pv->pred.tag = g_quark_from_string ("openpgp");
	self->pv->pred.location = BASTILE_LOCATION_REMOTE;
	self->pv->pred.custom = (BastileObjectPredicateFunc)on_filter_objects;
	self->pv->pred.custom_target = self;
	
	/* Our set all nicely filtered */
	self->pv->objects = bastile_set_new_full (&self->pv->pred);
	self->pv->store = bastile_key_manager_store_new (self->pv->objects, self->pv->view);
	on_view_selection_changed (selection, self);
	
	return G_OBJECT (self);
}

/**
* self: The BastileKeyserverResults object to init
*
* Inits the results object
*
**/
static void
bastile_keyserver_results_init (BastileKeyserverResults *self)
{
	self->pv = G_TYPE_INSTANCE_GET_PRIVATE (self, BASTILE_TYPE_KEYSERVER_RESULTS, BastileKeyserverResultsPrivate);
}

/**
* obj: BastileKeyserverResults object
*
* Finalize the results object
*
**/
static void
bastile_keyserver_results_finalize (GObject *obj)
{
	BastileKeyserverResults *self = BASTILE_KEYSERVER_RESULTS (obj);

	g_free (self->pv->search_string);
	self->pv->search_string = NULL;
	
	if (self->pv->object_actions)
		g_object_unref (self->pv->object_actions);
	self->pv->object_actions = NULL;
	
	if (self->pv->objects)
		g_object_unref (self->pv->objects);
	self->pv->objects = NULL;
	
	if (self->pv->store)
		g_object_unref (self->pv->store);
	self->pv->store = NULL;
	
	if (self->pv->view)
		gtk_tree_view_set_model (self->pv->view, NULL);
	self->pv->view = NULL;
	
	G_OBJECT_CLASS (bastile_keyserver_results_parent_class)->finalize (obj);
}

/**
* obj: The BastileKeyserverResults object to set
* prop_id: Property to set
* value: Value of this property
* pspec: GParamSpec for the warning
*
* Supported properties are PROP_SEARCH and PROP_SELECTED
*
**/
static void
bastile_keyserver_results_set_property (GObject *obj, guint prop_id, const GValue *value, 
                           GParamSpec *pspec)
{
	BastileKeyserverResults *self = BASTILE_KEYSERVER_RESULTS (obj);
	const gchar* str;
	
	switch (prop_id) {
	case PROP_SEARCH:
		/* Many key servers ignore spaces at the beginning and end, so we do too */
		str = g_value_get_string (value);
		if (!str)
			str = "";
		self->pv->search_string = g_strstrip (g_utf8_casefold (str, -1));
		break;
		
	case PROP_SELECTED:
		bastile_viewer_set_selected (BASTILE_VIEWER (self), g_value_get_object (value));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

/**
* obj: The BastileKeyserverResults object to get properties from
* prop_id: the ide of the property to get
* value: Value returned
* pspec: GParamSpec for the warning
*
* Supported properties: PROP_SEARCH and PROP_SELECTED
*
**/
static void
bastile_keyserver_results_get_property (GObject *obj, guint prop_id, GValue *value, 
                           GParamSpec *pspec)
{
	BastileKeyserverResults *self = BASTILE_KEYSERVER_RESULTS (obj);
	
	switch (prop_id) {
	case PROP_SEARCH:
		g_value_set_string (value, bastile_keyserver_results_get_search (self));
		break;
	case PROP_SELECTED:
		g_value_set_object (value, bastile_viewer_get_selected (BASTILE_VIEWER (self)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

/**
* klass: The BastileKeyserverResultsClass
*
* Inits the class
*
**/
static void
bastile_keyserver_results_class_init (BastileKeyserverResultsClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    
	bastile_keyserver_results_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (BastileKeyserverResultsPrivate));

	gobject_class->constructor = bastile_keyserver_results_constructor;
	gobject_class->finalize = bastile_keyserver_results_finalize;
	gobject_class->set_property = bastile_keyserver_results_set_property;
	gobject_class->get_property = bastile_keyserver_results_get_property;
    
	BASTILE_VIEWER_CLASS (klass)->get_selected_objects = bastile_keyserver_results_get_selected_objects;
	BASTILE_VIEWER_CLASS (klass)->set_selected_objects = bastile_keyserver_results_set_selected_objects;
	BASTILE_VIEWER_CLASS (klass)->get_selected = bastile_keyserver_results_get_selected;
	BASTILE_VIEWER_CLASS (klass)->set_selected = bastile_keyserver_results_set_selected;
	
	g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_SEARCH, 
	         g_param_spec_string ("search", "search", "search", NULL, 
	                              G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_override_property (G_OBJECT_CLASS (klass), PROP_SELECTED, "selected");
}

/* -----------------------------------------------------------------------------
 * PUBLIC 
 */

/**
 * bastile_keyserver_results_show:
 * @op: The search operation
 * @parent: A GTK window as parent (or NULL)
 * @search_text: The test to search for
 *
 * Creates a search results window and adds the operation to it's progress status.
 *
 */
void 
bastile_keyserver_results_show (BastileOperation* op, GtkWindow* parent, const char* search_text) 
{
	BastileKeyserverResults* res;
	GtkWindow *window;
	
	g_return_if_fail (BASTILE_IS_OPERATION (op));
	g_return_if_fail (parent == NULL || GTK_IS_WINDOW (parent));
	g_return_if_fail (search_text != NULL);
	
	res = g_object_new (BASTILE_TYPE_KEYSERVER_RESULTS, "name", "keyserver-results", "search", search_text, NULL);
	
	/* Destorys itself with destroy */
	g_object_ref_sink (res);
	
	if (parent != NULL) {
		window = GTK_WINDOW (bastile_widget_get_toplevel (BASTILE_WIDGET (res)));
		gtk_window_set_transient_for (window, parent);
	}

	bastile_progress_status_set_operation (BASTILE_WIDGET (res), op);
}

/**
 * bastile_keyserver_results_get_search:
 * @self:  BastileKeyserverResults object to get the search string from
 *
 *
 *
 * Returns: The search string in from the results
 */
const gchar* 
bastile_keyserver_results_get_search (BastileKeyserverResults* self) 
{
	g_return_val_if_fail (BASTILE_IS_KEYSERVER_RESULTS (self), NULL);
	return self->pv->search_string;
}
