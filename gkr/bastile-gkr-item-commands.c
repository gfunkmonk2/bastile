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

#include "bastile-gkr-item-commands.h"

#include "bastile-gkr.h"
#include "bastile-gkr-item.h"
#include "bastile-gkr-dialogs.h"

#include "common/bastile-registry.h"

#include "bastile-source.h"
#include "bastile-util.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (BastileGkrItemCommands, bastile_gkr_item_commands, BASTILE_TYPE_COMMANDS);

static BastileObjectPredicate commands_predicate = { 0, };

/* -----------------------------------------------------------------------------
 * INTERNAL 
 */

/* -----------------------------------------------------------------------------
 * OBJECT 
 */

static void 
bastile_gkr_item_commands_show_properties (BastileCommands* base, BastileObject* object) 
{
	GtkWindow *window;

	g_return_if_fail (BASTILE_IS_OBJECT (object));
	g_return_if_fail (bastile_object_get_tag (object) == BASTILE_GKR_TYPE);

	window = bastile_view_get_window (bastile_commands_get_view (base));
	if (G_OBJECT_TYPE (object) == BASTILE_TYPE_GKR_ITEM) 
		bastile_gkr_item_properties_show (BASTILE_GKR_ITEM (object), window); 
	
	else
		g_return_if_reached ();
}

static BastileOperation* 
bastile_gkr_item_commands_delete_objects (BastileCommands* base, GList* objects) 
{
	gchar *prompt;
	GtkWidget *parent;
	gboolean ret;
	guint num;

	num = g_list_length (objects);
	if (num == 0)
		return NULL;

	if (num == 1) {
		prompt = g_strdup_printf (_ ("Are you sure you want to delete the password '%s'?"), 
		                          bastile_object_get_label (BASTILE_OBJECT (objects->data)));
	} else {
		prompt = g_strdup_printf (ngettext ("Are you sure you want to delete %d password?", 
		                                    "Are you sure you want to delete %d passwords?", 
		                                    num), num);
	}
	
	parent = GTK_WIDGET (bastile_view_get_window (bastile_commands_get_view (base)));
	ret = bastile_util_prompt_delete (prompt, parent);
	g_free (prompt);
	
	if (!ret)
		return NULL;

	return bastile_source_delete_objects (objects);
}

static GObject* 
bastile_gkr_item_commands_constructor (GType type, guint n_props, GObjectConstructParam *props) 
{
	GObject *obj = G_OBJECT_CLASS (bastile_gkr_item_commands_parent_class)->constructor (type, n_props, props);
	BastileCommands *base = NULL;
	BastileView *view;
	
	if (obj) {
		base = BASTILE_COMMANDS (obj);
		view = bastile_commands_get_view (base);
		g_return_val_if_fail (view, NULL);
		bastile_view_register_commands (view, &commands_predicate, base);
	}
	
	return obj;
}

static void
bastile_gkr_item_commands_init (BastileGkrItemCommands *self)
{

}

static void
bastile_gkr_item_commands_class_init (BastileGkrItemCommandsClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	BastileCommandsClass *cmd_class = BASTILE_COMMANDS_CLASS (klass);
	
	bastile_gkr_item_commands_parent_class = g_type_class_peek_parent (klass);

	gobject_class->constructor = bastile_gkr_item_commands_constructor;

	cmd_class->show_properties = bastile_gkr_item_commands_show_properties;
	cmd_class->delete_objects = bastile_gkr_item_commands_delete_objects;
	
	/* Setup the predicate that matches our commands */
	commands_predicate.type = BASTILE_TYPE_GKR_ITEM;

	/* Register this class as a commands */
	bastile_registry_register_type (bastile_registry_get (), BASTILE_TYPE_GKR_ITEM_COMMANDS, 
	                                 BASTILE_GKR_TYPE_STR, "commands", NULL, NULL);
}

/* -----------------------------------------------------------------------------
 * PUBLIC 
 */
