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

#include <glib/gi18n.h>

#include "bastile-ssh.h"
#include "bastile-ssh-commands.h"
#include "bastile-ssh-dialogs.h"

#include "common/bastile-registry.h"

#include "bastile-object.h"
#include "bastile-util.h"

enum {
	PROP_0
};

struct _BastileSshCommandsPrivate {
	GtkActionGroup* command_actions;
};

G_DEFINE_TYPE (BastileSshCommands, bastile_ssh_commands, BASTILE_TYPE_COMMANDS);

static const char* UI_DEFINITION = ""\
"<ui>"\
"	<menubar>"\
"		<menu name='Remote' action='remote-menu'>"\
"			<menuitem action='remote-ssh-upload'/>"\
"		</menu>"\
"	</menubar>"\
"	<popup name='KeyPopup'>"\
"		<menuitem action='remote-ssh-upload'/>"\
"	</popup>"\
"</ui>";

static BastileObjectPredicate commands_predicate = { 0, };

/* -----------------------------------------------------------------------------
 * INTERNAL 
 */

static void 
on_ssh_upload (GtkAction* action, BastileSshCommands* self) 
{
	BastileView *view;
	GList* ssh_keys;

	g_return_if_fail (BASTILE_IS_SSH_COMMANDS (self));
	g_return_if_fail (GTK_IS_ACTION (action));
	
	view = bastile_commands_get_view (BASTILE_COMMANDS (self));
	ssh_keys = bastile_view_get_selected_matching (view, &commands_predicate);
	if (ssh_keys == NULL)
		return;
	
	/* Indicate what we're actually going to operate on */
	bastile_view_set_selected_objects (view, ssh_keys);
	
	bastile_ssh_upload_prompt (ssh_keys, bastile_commands_get_window (BASTILE_COMMANDS (self)));
	g_list_free (ssh_keys);
}

static const GtkActionEntry COMMAND_ENTRIES[] = {
	{ "remote-ssh-upload", NULL, N_ ("Configure Key for _Secure Shell..."), "", 
		N_ ("Send public Secure Shell key to another machine, and enable logins using that key."), 
		G_CALLBACK (on_ssh_upload) }
};

/* -----------------------------------------------------------------------------
 * OBJECT 
 */

static void 
bastile_ssh_commands_show_properties (BastileCommands* base, BastileObject* obj) 
{
	g_return_if_fail (BASTILE_IS_OBJECT (obj));
	g_return_if_fail (bastile_object_get_tag (obj) == BASTILE_SSH_TYPE);
	g_return_if_fail (G_TYPE_FROM_INSTANCE (G_OBJECT (obj)) == BASTILE_SSH_TYPE_KEY);
	
	bastile_ssh_key_properties_show (BASTILE_SSH_KEY (obj), bastile_commands_get_window (base));
}

static BastileOperation*
bastile_ssh_commands_delete_objects (BastileCommands* base, GList* objects) 
{
	BastileOperation* op = NULL;
	guint num;
	gchar* prompt;

	num = g_list_length (objects);
	if (num == 0) {
		return NULL;
	} else if (num == 1) {
		prompt = g_strdup_printf (_("Are you sure you want to delete the secure shell key '%s'?"), 
		                          bastile_object_get_label (objects->data));
	} else {
		prompt = g_strdup_printf (_("Are you sure you want to delete %d secure shell keys?"), num);
	}
	
	if (bastile_util_prompt_delete (prompt, NULL))
		op = bastile_source_delete_objects (objects);
	
	g_free (prompt);
	return op;
}

static GObject* 
bastile_ssh_commands_constructor (GType type, guint n_props, GObjectConstructParam *props) 
{
	GObject *obj = G_OBJECT_CLASS (bastile_ssh_commands_parent_class)->constructor (type, n_props, props);
	BastileSshCommands *self = NULL;
	BastileCommands *base;
	BastileView *view;
	
	if (obj) {
		self = BASTILE_SSH_COMMANDS (obj);
		base = BASTILE_COMMANDS (obj);
	
		view = bastile_commands_get_view (BASTILE_COMMANDS (self));
		g_return_val_if_fail (view, NULL);
		
		self->pv->command_actions = gtk_action_group_new ("ssh");
		gtk_action_group_set_translation_domain (self->pv->command_actions, GETTEXT_PACKAGE);
		gtk_action_group_add_actions (self->pv->command_actions, COMMAND_ENTRIES, 
		                              G_N_ELEMENTS (COMMAND_ENTRIES), self);
		
		bastile_view_register_commands (view, &commands_predicate, base);
		bastile_view_register_ui (view, &commands_predicate, UI_DEFINITION, self->pv->command_actions);
	}
	
	return obj;
}

static void
bastile_ssh_commands_init (BastileSshCommands *self)
{
	self->pv = G_TYPE_INSTANCE_GET_PRIVATE (self, BASTILE_TYPE_SSH_COMMANDS, BastileSshCommandsPrivate);

}

static void
bastile_ssh_commands_dispose (GObject *obj)
{
	BastileSshCommands *self = BASTILE_SSH_COMMANDS (obj);
    
	if (self->pv->command_actions)
		g_object_unref (self->pv->command_actions);
	self->pv->command_actions = NULL;
	
	G_OBJECT_CLASS (bastile_ssh_commands_parent_class)->dispose (obj);
}

static void
bastile_ssh_commands_finalize (GObject *obj)
{
	BastileSshCommands *self = BASTILE_SSH_COMMANDS (obj);

	g_assert (!self->pv->command_actions);
	
	G_OBJECT_CLASS (bastile_ssh_commands_parent_class)->finalize (obj);
}

static void
bastile_ssh_commands_set_property (GObject *obj, guint prop_id, const GValue *value, 
                           GParamSpec *pspec)
{
	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
bastile_ssh_commands_get_property (GObject *obj, guint prop_id, GValue *value, 
                           GParamSpec *pspec)
{
	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
bastile_ssh_commands_class_init (BastileSshCommandsClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    
	bastile_ssh_commands_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (BastileSshCommandsPrivate));

	gobject_class->constructor = bastile_ssh_commands_constructor;
	gobject_class->dispose = bastile_ssh_commands_dispose;
	gobject_class->finalize = bastile_ssh_commands_finalize;
	gobject_class->set_property = bastile_ssh_commands_set_property;
	gobject_class->get_property = bastile_ssh_commands_get_property;
    
	BASTILE_COMMANDS_CLASS (klass)->show_properties = bastile_ssh_commands_show_properties;
	BASTILE_COMMANDS_CLASS (klass)->delete_objects = bastile_ssh_commands_delete_objects;

	commands_predicate.type = BASTILE_TYPE_SSH_KEY;
	commands_predicate.usage = BASTILE_USAGE_PRIVATE_KEY;
	
	/* Register this class as a commands */
	bastile_registry_register_type (bastile_registry_get (), BASTILE_TYPE_SSH_COMMANDS, 
	                                 BASTILE_SSH_TYPE_STR, "commands", NULL, NULL);
}

/* -----------------------------------------------------------------------------
 * PUBLIC 
 */

BastileSshCommands*
bastile_ssh_commands_new (void)
{
	return g_object_new (BASTILE_TYPE_SSH_COMMANDS, NULL);
}
