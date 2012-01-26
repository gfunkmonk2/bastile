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

#include "bastile-generate-select.h"

#include "common/bastile-object-list.h"
#include "common/bastile-registry.h"

typedef enum  {
	COLUMN_ICON,
	COLUMN_TEXT,
	COLUMN_ACTION,
	COLUMN_N_COLUMNS
} Column;

struct _BastileGenerateSelectPrivate {
	GtkListStore* store;
	GtkTreeView* view;
	GtkDialog* dialog;
	GList *action_groups;
};

G_DEFINE_TYPE (BastileGenerateSelect, bastile_generate_select, BASTILE_TYPE_WIDGET);

static const char* TEMPLATE = "<span size=\"larger\" weight=\"bold\">%s</span>\n%s";

/* -----------------------------------------------------------------------------
 * INTERNAL 
 */

static gboolean 
fire_selected_action (BastileGenerateSelect* self) 
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkAction *action;
	
	selection = gtk_tree_view_get_selection (self->pv->view);
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return FALSE;

	gtk_tree_model_get (GTK_TREE_MODEL (self->pv->store), &iter,
	                    COLUMN_ACTION, &action, -1);
	g_assert (action != NULL);

	gtk_action_activate (action);
	return TRUE;
}

static void
on_row_activated (GtkTreeView* view, GtkTreePath* path, GtkTreeViewColumn* col, BastileGenerateSelect* self) 
{
	g_return_if_fail (BASTILE_IS_GENERATE_SELECT (self));
	g_return_if_fail (GTK_IS_TREE_VIEW (view));
	g_return_if_fail (path != NULL);
	g_return_if_fail (GTK_IS_TREE_VIEW_COLUMN (col));
	
	if (fire_selected_action (self))
		bastile_widget_destroy (BASTILE_WIDGET (self));
}

static void 
on_response (GtkDialog* dialog, gint response, BastileGenerateSelect* self) 
{
	g_return_if_fail (BASTILE_IS_GENERATE_SELECT (self));
	g_return_if_fail (GTK_IS_DIALOG (dialog));
	
	if (response == GTK_RESPONSE_OK) 
		fire_selected_action (self);

	bastile_widget_destroy (BASTILE_WIDGET (self));
}

/* -----------------------------------------------------------------------------
 * OBJECT 
 */


static GObject* 
bastile_generate_select_constructor (GType type, guint n_props, GObjectConstructParam *props) 
{
	BastileGenerateSelect *self = BASTILE_GENERATE_SELECT (G_OBJECT_CLASS (bastile_generate_select_parent_class)->constructor(type, n_props, props));
	gchar *text, *icon;
	gchar *label, *tooltip;
	GtkCellRenderer *pixcell;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GList *l;
	
	g_return_val_if_fail (self, NULL);	

	self->pv->store = gtk_list_store_new (COLUMN_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, GTK_TYPE_ACTION);

	self->pv->action_groups = bastile_registry_object_instances (NULL, "generator", NULL);
	for (l = self->pv->action_groups; l; l = g_list_next (l)) {
		GList *k, *actions = gtk_action_group_list_actions (l->data);
		for (k = actions; k; k = g_list_next (k)) {
			
			g_object_get (k->data, "label", &label, "tooltip", &tooltip, "stock-id", &icon, NULL);
			text = g_strdup_printf (TEMPLATE, label, tooltip);

			gtk_list_store_append (self->pv->store, &iter);
			gtk_list_store_set (self->pv->store, &iter, 
			                    COLUMN_TEXT, text, 
			                    COLUMN_ICON, icon, 
				            COLUMN_ACTION, k->data, 
				            -1);
			
			g_free (text);
			g_free (label);
			g_free (icon);
			g_free (tooltip);
		}
		
		g_list_free (actions);
	}
	
	/* Hook it into the view */
	self->pv->view = GTK_TREE_VIEW (bastile_widget_get_widget (BASTILE_WIDGET (self), "keytype-tree"));
	g_return_val_if_fail (self->pv->view, NULL);
	
	pixcell = gtk_cell_renderer_pixbuf_new ();
	g_object_set (pixcell, "stock-size", GTK_ICON_SIZE_DND, NULL);
	gtk_tree_view_insert_column_with_attributes (self->pv->view, -1, "", pixcell, "stock-id", COLUMN_ICON, NULL);
	gtk_tree_view_insert_column_with_attributes (self->pv->view, -1, "", gtk_cell_renderer_text_new (), "markup", COLUMN_TEXT, NULL);
	gtk_tree_view_set_model (self->pv->view, GTK_TREE_MODEL (self->pv->store));

	/* Setup selection, select first item */
	selection = gtk_tree_view_get_selection (self->pv->view);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
	
	gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->pv->store), &iter);
	gtk_tree_selection_select_iter (selection, &iter);

	g_signal_connect (self->pv->view, "row-activated", G_CALLBACK (on_row_activated), self);

	self->pv->dialog = GTK_DIALOG (bastile_widget_get_toplevel (BASTILE_WIDGET (self)));
	g_signal_connect (self->pv->dialog, "response", G_CALLBACK (on_response), self);
	
	return G_OBJECT (self);
}

static void
bastile_generate_select_init (BastileGenerateSelect *self)
{
	self->pv = G_TYPE_INSTANCE_GET_PRIVATE (self, BASTILE_TYPE_GENERATE_SELECT, BastileGenerateSelectPrivate);
}

static void
bastile_generate_select_finalize (GObject *obj)
{
	BastileGenerateSelect *self = BASTILE_GENERATE_SELECT (obj);

	if (self->pv->store != NULL)
		g_object_unref (self->pv->store);
	self->pv->store = NULL;

	bastile_object_list_free (self->pv->action_groups);
	self->pv->action_groups = NULL;

	G_OBJECT_CLASS (bastile_generate_select_parent_class)->finalize (obj);
}

static void
bastile_generate_select_class_init (BastileGenerateSelectClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    
	bastile_generate_select_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (BastileGenerateSelectPrivate));

	gobject_class->constructor = bastile_generate_select_constructor;
	gobject_class->finalize = bastile_generate_select_finalize;

}

/* -----------------------------------------------------------------------------
 * PUBLIC 
 */

void 
bastile_generate_select_show (GtkWindow* parent) 
{
	BastileGenerateSelect* sel;
	
	g_return_if_fail (parent == NULL || GTK_IS_WINDOW (parent));
	
	sel = g_object_ref_sink (g_object_new (BASTILE_TYPE_GENERATE_SELECT, "name", "generate-select", NULL));
	if (parent != NULL)
		gtk_window_set_transient_for (GTK_WINDOW (sel->pv->dialog), parent);
}
