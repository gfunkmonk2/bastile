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

#include "bastile-gkr-keyring-commands.h"

#include "bastile-gkr.h"
#include "bastile-gkr-keyring.h"
#include "bastile-gkr-dialogs.h"
#include "bastile-gkr-source.h"

#include "common/bastile-registry.h"

#include "bastile-source.h"
#include "bastile-util.h"

#include <glib/gi18n.h>

struct _BastileGkrKeyringCommandsPrivate {
	GtkAction *action_lock;
	GtkAction *action_unlock;
	GtkAction *action_default;
};

G_DEFINE_TYPE (BastileGkrKeyringCommands, bastile_gkr_keyring_commands, BASTILE_TYPE_COMMANDS);

static const char* UI_KEYRING = ""\
"<ui>"\
"	<popup name='KeyPopup'>"\
"		<menuitem action='keyring-default'/>"\
"		<menuitem action='keyring-lock'/>"\
"		<menuitem action='keyring-unlock'/>"\
"		<menuitem action='keyring-password'/>"\
"	</popup>"\
"</ui>";

static BastileObjectPredicate keyring_predicate = { 0, };

static void on_view_selection_changed (BastileView *view, gpointer user_data);

/* -----------------------------------------------------------------------------
 * INTERNAL 
 */

static void
on_refresh_done (BastileOperation *op, gpointer user_data)
{
	BastileCommands *self = user_data;
	BastileView *view;

	g_return_if_fail (BASTILE_IS_COMMANDS (self));

	view = bastile_commands_get_view (self);
	if (view != NULL)
		on_view_selection_changed (view, self);

	g_object_unref (self);
}

static void
refresh_all_keyrings (BastileCommands *self)
{
	BastileOperation *op;

	op = bastile_source_load (BASTILE_SOURCE (bastile_gkr_source_default ()));
	
	/* 
	 * HACK: Major hack alert. This whole area needs some serious refactoring,
	 * so for now we're just going to let any viewers listen in on this
	 * operation like so:
	 */
	g_signal_emit_by_name (bastile_context_for_app (), "refreshing", op);

	bastile_operation_watch (op, on_refresh_done, g_object_ref (self), NULL, NULL);
	g_object_unref (op);
}

/**
 * on_gkr_add_keyring:
 * @action:
 * @unused: ignored
 *
 * calls the function that shows the add keyring window
 */
static void
on_gkr_add_keyring (GtkAction *action, gpointer unused)
{
	g_return_if_fail (GTK_IS_ACTION (action));
	bastile_gkr_add_keyring_show (NULL);
}

static void
on_gkr_add_item (GtkAction *action, gpointer unused)
{
	g_return_if_fail (GTK_IS_ACTION (action));
	bastile_gkr_add_item_show (NULL);
}

static const GtkActionEntry ENTRIES_NEW[] = {
	{ "gkr-add-keyring", "folder", N_("Password Keyring"), "",
	  N_("Used to store application and network passwords"), G_CALLBACK (on_gkr_add_keyring) },
	{ "gkr-add-item", "emblem-readonly", N_("Stored Password"), "",
	  N_("Safely store a password or secret."), G_CALLBACK (on_gkr_add_item) }
};

static void
on_keyring_unlock_done (MateKeyringResult result, gpointer user_data)
{
	BastileCommands *self = BASTILE_COMMANDS (user_data);
	BastileView *view;

	if (result != MATE_KEYRING_RESULT_OK &&
	    result != MATE_KEYRING_RESULT_DENIED &&
	    result != MATE_KEYRING_RESULT_CANCELLED) {
		view = bastile_commands_get_view (self);
		bastile_util_show_error (GTK_WIDGET (bastile_view_get_window (view)),
		                          _("Couldn't unlock keyring"),
		                          mate_keyring_result_to_message (result));
	}
	
	refresh_all_keyrings (self);
}

static void 
on_keyring_unlock (GtkAction *action, BastileGkrKeyringCommands *self)
{
	BastileView *view;
	BastileGkrKeyring *keyring;
	GList *keys, *l;

	g_return_if_fail (BASTILE_IS_GKR_KEYRING_COMMANDS (self));
	g_return_if_fail (GTK_IS_ACTION (action));

	view = bastile_commands_get_view (BASTILE_COMMANDS (self));
	keys = bastile_view_get_selected_matching (view, &keyring_predicate);

	for (l = keys; l; l = g_list_next (l)) {
		keyring = BASTILE_GKR_KEYRING (l->data);
		g_return_if_fail (BASTILE_IS_GKR_KEYRING (l->data));
		mate_keyring_unlock (bastile_gkr_keyring_get_name (l->data), NULL, 
		                      on_keyring_unlock_done, g_object_ref (self), g_object_unref);
	}
	
	g_list_free (keys);
}

static void
on_keyring_lock_done (MateKeyringResult result, gpointer user_data)
{
	BastileCommands *self = BASTILE_COMMANDS (user_data);
	BastileView *view;

	if (result != MATE_KEYRING_RESULT_OK &&
	    result != MATE_KEYRING_RESULT_DENIED &&
	    result != MATE_KEYRING_RESULT_CANCELLED) {
		view = bastile_commands_get_view (self);
		bastile_util_show_error (GTK_WIDGET (bastile_view_get_window (view)),
		                          _("Couldn't lock keyring"),
		                          mate_keyring_result_to_message (result));
	}

	refresh_all_keyrings (self);
}

static void 
on_keyring_lock (GtkAction *action, BastileGkrKeyringCommands *self)
{
	BastileView *view;
	BastileGkrKeyring *keyring;
	GList *keyrings, *l;

	g_return_if_fail (BASTILE_IS_GKR_KEYRING_COMMANDS (self));
	g_return_if_fail (GTK_IS_ACTION (action));

	view = bastile_commands_get_view (BASTILE_COMMANDS (self));
	keyrings = bastile_view_get_selected_matching (view, &keyring_predicate);

	for (l = keyrings; l; l = g_list_next (l)) {
		keyring = BASTILE_GKR_KEYRING (l->data);
		g_return_if_fail (BASTILE_IS_GKR_KEYRING (l->data));
		mate_keyring_lock (bastile_gkr_keyring_get_name (l->data), 
		                    on_keyring_lock_done, g_object_ref (self), g_object_unref);
	}

	g_list_free (keyrings);
}

static void
on_set_default_keyring_done (MateKeyringResult result, gpointer user_data)
{
	BastileCommands *self = BASTILE_COMMANDS (user_data);
	BastileView *view;

	if (result != MATE_KEYRING_RESULT_OK &&
	    result != MATE_KEYRING_RESULT_DENIED &&
	    result != MATE_KEYRING_RESULT_CANCELLED) {
		view = bastile_commands_get_view (self);
		bastile_util_show_error (GTK_WIDGET (bastile_view_get_window (view)),
		                          _("Couldn't set default keyring"),
		                          mate_keyring_result_to_message (result));
	}
	
	refresh_all_keyrings (self);
}

static void
on_keyring_default (GtkAction *action, BastileGkrKeyringCommands *self)
{
	BastileView *view;
	GList *keys;

	g_return_if_fail (BASTILE_IS_GKR_KEYRING_COMMANDS (self));
	g_return_if_fail (GTK_IS_ACTION (action));

	view = bastile_commands_get_view (BASTILE_COMMANDS (self));
	keys = bastile_view_get_selected_matching (view, &keyring_predicate);

	if (keys) {
		mate_keyring_set_default_keyring (bastile_gkr_keyring_get_name (keys->data), 
		                                   on_set_default_keyring_done, g_object_ref (self), g_object_unref);
	}
	
	g_list_free (keys);

}

static void
on_change_password_done (MateKeyringResult result, gpointer user_data)
{
	BastileCommands *self = BASTILE_COMMANDS (user_data);
	BastileView *view;

	if (result != MATE_KEYRING_RESULT_OK &&
	    result != MATE_KEYRING_RESULT_DENIED &&
	    result != MATE_KEYRING_RESULT_CANCELLED) {
		view = bastile_commands_get_view (self);
		bastile_util_show_error (GTK_WIDGET (bastile_view_get_window (view)),
		                          _("Couldn't change keyring password"),
		                          mate_keyring_result_to_message (result));
	}
	
	refresh_all_keyrings (self);
}

static void
on_keyring_password (GtkAction *action, BastileGkrKeyringCommands *self)
{
	BastileView *view;
	BastileGkrKeyring *keyring;
	GList *keys, *l;

	g_return_if_fail (BASTILE_IS_GKR_KEYRING_COMMANDS (self));
	g_return_if_fail (GTK_IS_ACTION (action));

	view = bastile_commands_get_view (BASTILE_COMMANDS (self));
	keys = bastile_view_get_selected_matching (view, &keyring_predicate);

	for (l = keys; l; l = g_list_next (l)) {
		keyring = BASTILE_GKR_KEYRING (l->data);
		g_return_if_fail (BASTILE_IS_GKR_KEYRING (l->data));
		mate_keyring_change_password (bastile_gkr_keyring_get_name (l->data), NULL, NULL,
		                               on_change_password_done, g_object_ref (self), g_object_unref);
	}
	
	g_list_free (keys);
}

static const GtkActionEntry ENTRIES_KEYRING[] = {
	{ "keyring-lock", NULL, N_("_Lock"), "",
	  N_("Lock the password storage keyring so a master password is required to unlock it."), G_CALLBACK (on_keyring_lock) },
	{ "keyring-unlock", NULL, N_("_Unlock"), "",
	  N_("Unlock the password storage keyring with a master password so it is available for use."), G_CALLBACK (on_keyring_unlock) },
	{ "keyring-default", NULL, N_("_Set as default"), "",
	  N_("Applications usually store new passwords in the default keyring."), G_CALLBACK (on_keyring_default) },
	{ "keyring-password", NULL, N_("Change _Password"), "",
	  N_("Change the unlock password of the password storage keyring"), G_CALLBACK (on_keyring_password) }
};

static void
on_view_selection_changed (BastileView *view, gpointer user_data)
{
	BastileGkrKeyringCommands *self = user_data;
	MateKeyringInfo *info;
	gboolean locked = FALSE;
	gboolean unlocked = FALSE;
	gboolean can_default = FALSE;
	GList *keys, *l;
	
	g_return_if_fail (BASTILE_IS_VIEW (view));
	g_return_if_fail (BASTILE_IS_GKR_KEYRING_COMMANDS (self));
	
	keys = bastile_view_get_selected_matching (view, &keyring_predicate);
	for (l = keys; l; l = g_list_next (l)) {
		info = bastile_gkr_keyring_get_info (l->data);
		if (info != NULL) {
			if (mate_keyring_info_get_is_locked (info))
				locked = TRUE;
			else 
				unlocked = TRUE;
			if (!bastile_gkr_keyring_get_is_default (l->data))
				can_default = TRUE;
		}
	}
	
	g_list_free (keys);
	
	gtk_action_set_sensitive (self->pv->action_lock, unlocked);
	gtk_action_set_sensitive (self->pv->action_unlock, locked);
	gtk_action_set_sensitive (self->pv->action_default, can_default);
}

/* -----------------------------------------------------------------------------
 * OBJECT 
 */

static void 
bastile_gkr_keyring_commands_show_properties (BastileCommands* base, BastileObject* object) 
{
	GtkWindow *window;

	g_return_if_fail (BASTILE_IS_OBJECT (object));
	g_return_if_fail (bastile_object_get_tag (object) == BASTILE_GKR_TYPE);

	window = bastile_view_get_window (bastile_commands_get_view (base));
	if (G_OBJECT_TYPE (object) == BASTILE_TYPE_GKR_KEYRING) 
		bastile_gkr_keyring_properties_show (BASTILE_GKR_KEYRING (object), window);
	
	else
		g_return_if_reached ();
}

static BastileOperation* 
bastile_gkr_keyring_commands_delete_objects (BastileCommands* base, GList* objects) 
{
	BastileOperation *oper = NULL;
	gchar *prompt;
	
	if (!objects)
		return NULL;

	prompt = g_strdup_printf (_ ("Are you sure you want to delete the password keyring '%s'?"), 
	                          bastile_object_get_label (objects->data));

	if (bastile_util_prompt_delete (prompt, GTK_WIDGET (bastile_commands_get_window (base))))
		oper = bastile_object_delete (objects->data);
		
	g_free (prompt);
	
	return oper;
}

static GObject* 
bastile_gkr_keyring_commands_constructor (GType type, guint n_props, GObjectConstructParam *props) 
{
	BastileGkrKeyringCommands *self = BASTILE_GKR_KEYRING_COMMANDS (G_OBJECT_CLASS (bastile_gkr_keyring_commands_parent_class)->constructor (type, n_props, props));
	GtkActionGroup *actions;
	BastileView *view;
	
	g_return_val_if_fail (BASTILE_IS_GKR_KEYRING_COMMANDS (self), NULL);
	
	view = bastile_commands_get_view (BASTILE_COMMANDS (self));
	g_return_val_if_fail (view, NULL);
	bastile_view_register_commands (view, &keyring_predicate, BASTILE_COMMANDS (self));

	actions = gtk_action_group_new ("gkr-keyring");
	gtk_action_group_set_translation_domain (actions, GETTEXT_PACKAGE);
	gtk_action_group_add_actions (actions, ENTRIES_KEYRING, G_N_ELEMENTS (ENTRIES_KEYRING), self);
	bastile_view_register_ui (view, &keyring_predicate, UI_KEYRING, actions);
	self->pv->action_lock = g_object_ref (gtk_action_group_get_action (actions, "keyring-lock"));
	self->pv->action_unlock = g_object_ref (gtk_action_group_get_action (actions, "keyring-unlock"));
	self->pv->action_default = g_object_ref (gtk_action_group_get_action (actions, "keyring-default"));
	g_object_unref (actions);
	
	/* Watch and wait for selection changes and diddle lock/unlock */ 
	g_signal_connect (view, "selection-changed", G_CALLBACK (on_view_selection_changed), self);

	return G_OBJECT (self);
}

static void
bastile_gkr_keyring_commands_init (BastileGkrKeyringCommands *self)
{
	self->pv = G_TYPE_INSTANCE_GET_PRIVATE (self, BASTILE_TYPE_GKR_KEYRING_COMMANDS, BastileGkrKeyringCommandsPrivate);
}

static void
bastile_gkr_keyring_commands_finalize (GObject *obj)
{
	BastileGkrKeyringCommands *self = BASTILE_GKR_KEYRING_COMMANDS (obj);

	g_object_unref (self->pv->action_lock);
	self->pv->action_lock = NULL;
	
	g_object_unref (self->pv->action_unlock);
	self->pv->action_unlock = NULL;
	
	g_object_unref (self->pv->action_default);
	self->pv->action_default = NULL;
	
	G_OBJECT_CLASS (bastile_gkr_keyring_commands_parent_class)->finalize (obj);
}

static void
bastile_gkr_keyring_commands_class_init (BastileGkrKeyringCommandsClass *klass)
{
	GtkActionGroup *actions;
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	BastileCommandsClass *cmd_class = BASTILE_COMMANDS_CLASS (klass);
	
	bastile_gkr_keyring_commands_parent_class = g_type_class_peek_parent (klass);

	gobject_class->constructor = bastile_gkr_keyring_commands_constructor;
	gobject_class->finalize = bastile_gkr_keyring_commands_finalize;

	cmd_class->show_properties = bastile_gkr_keyring_commands_show_properties;
	cmd_class->delete_objects = bastile_gkr_keyring_commands_delete_objects;
	
	g_type_class_add_private (gobject_class, sizeof (BastileGkrKeyringCommandsPrivate));

	/* Setup the predicate for these commands */
	keyring_predicate.type = BASTILE_TYPE_GKR_KEYRING;
	
	/* Register this class as a commands */
	bastile_registry_register_type (bastile_registry_get (), BASTILE_TYPE_GKR_KEYRING_COMMANDS, 
	                                 BASTILE_GKR_TYPE_STR, "commands", NULL, NULL);
	
	/* Register this as a generator */
	actions = gtk_action_group_new ("gkr-generate");
	gtk_action_group_set_translation_domain (actions, GETTEXT_PACKAGE);
	gtk_action_group_add_actions (actions, ENTRIES_NEW, G_N_ELEMENTS (ENTRIES_NEW), NULL);
	bastile_registry_register_object (NULL, G_OBJECT (actions), BASTILE_GKR_TYPE_STR, "generator", NULL);
}

/* -----------------------------------------------------------------------------
 * PUBLIC 
 */
