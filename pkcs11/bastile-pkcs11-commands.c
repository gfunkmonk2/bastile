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

#include "bastile-pkcs11-commands.h"

#include "bastile-pkcs11.h"
#include "bastile-pkcs11-certificate.h"
#include "bastile-pkcs11-certificate-props.h"

#include "bastile-util.h"

#include "common/bastile-registry.h"

enum {
	PROP_0
};

struct _BastilePkcs11CommandsPrivate {
	GtkActionGroup *action_group;
};

static GQuark slot_certificate_window = 0; 

G_DEFINE_TYPE (BastilePkcs11Commands, bastile_pkcs11_commands, BASTILE_TYPE_COMMANDS);

#define BASTILE_PKCS11_COMMANDS_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), BASTILE_PKCS11_TYPE_COMMANDS, BastilePkcs11CommandsPrivate))

static BastileObjectPredicate commands_predicate = { 0, };

/* -----------------------------------------------------------------------------
 * INTERNAL 
 */

static void 
properties_response (GtkDialog *dialog, gint response_id, gpointer user_data)
{
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

/* -----------------------------------------------------------------------------
 * OBJECT 
 */

static void
bastile_pkcs11_commands_show_properties (BastileCommands *cmds, BastileObject *object)
{
	GtkWindow *window;
	gpointer previous;
	
	g_return_if_fail (BASTILE_PKCS11_IS_COMMANDS (cmds));
	g_return_if_fail (BASTILE_PKCS11_IS_CERTIFICATE (object));
	
	/* Try to show an already present window */
	previous = g_object_get_qdata (G_OBJECT (object), slot_certificate_window);
	if (GTK_IS_WINDOW (previous)) {
		window = GTK_WINDOW (previous);
		if (gtk_widget_get_visible (GTK_WIDGET (window))) {
			gtk_window_present (window);
			return;
		}
	}
	
	/* Create a new dialog for the certificate */
	window = GTK_WINDOW (bastile_pkcs11_certificate_props_new (GCR_CERTIFICATE (object)));
	gtk_window_set_transient_for (window, bastile_view_get_window (bastile_commands_get_view (cmds)));
	g_object_set_qdata (G_OBJECT (object), slot_certificate_window, window);
	gtk_widget_show (GTK_WIDGET (window));

	/* Close the window when we get a response */
	g_signal_connect (window, "response", G_CALLBACK (properties_response), NULL);
}

static BastileOperation*
bastile_pkcs11_commands_delete_objects (BastileCommands *cmds, GList *objects)
{
	gchar *prompt;
	const gchar *display;
	gboolean ret;
	guint num;
	
	g_return_val_if_fail (BASTILE_PKCS11_IS_COMMANDS (cmds), NULL);

	num = g_list_length (objects);
	
	if (num == 1) {
		display = bastile_object_get_label (BASTILE_OBJECT (objects->data));
		prompt = g_strdup_printf (_("Are you sure you want to delete the certificate '%s'?"), display);
	} else {
		prompt = g_strdup_printf (ngettext (
				"Are you sure you want to delete %d certificate?",
				"Are you sure you want to delete %d certificates?",
				num), num);
	}
	
	ret = bastile_util_prompt_delete (prompt, GTK_WIDGET (bastile_view_get_window (bastile_commands_get_view (cmds))));
	g_free (prompt);
	
	if (ret)
		return bastile_source_delete_objects (objects);
	else
		return NULL;
}

static GObject* 
bastile_pkcs11_commands_constructor (GType type, guint n_props, GObjectConstructParam *props) 
{
	GObject *obj = G_OBJECT_CLASS (bastile_pkcs11_commands_parent_class)->constructor (type, n_props, props);
	BastilePkcs11CommandsPrivate *pv;
	BastilePkcs11Commands *self = NULL;
	BastileCommands *base;
	BastileView *view;
	
	if (obj) {
		pv = BASTILE_PKCS11_COMMANDS_GET_PRIVATE (obj);
		self = BASTILE_PKCS11_COMMANDS (obj);
		base = BASTILE_COMMANDS (self);
	
		view = bastile_commands_get_view (base);
		g_return_val_if_fail (view, NULL);
		
		bastile_view_register_commands (view, &commands_predicate, base);
		bastile_view_register_ui (view, &commands_predicate, "", pv->action_group);
	}
	
	return obj;
}

static void
bastile_pkcs11_commands_init (BastilePkcs11Commands *self)
{
	BastilePkcs11CommandsPrivate *pv = BASTILE_PKCS11_COMMANDS_GET_PRIVATE (self);
	pv->action_group = gtk_action_group_new ("pkcs11");
}

static void
bastile_pkcs11_commands_dispose (GObject *obj)
{
	BastilePkcs11Commands *self = BASTILE_PKCS11_COMMANDS (obj);
	BastilePkcs11CommandsPrivate *pv = BASTILE_PKCS11_COMMANDS_GET_PRIVATE (self);
    
	if (pv->action_group)
		g_object_unref (pv->action_group);
	pv->action_group = NULL;
	
	G_OBJECT_CLASS (bastile_pkcs11_commands_parent_class)->dispose (obj);
}

static void
bastile_pkcs11_commands_finalize (GObject *obj)
{
	BastilePkcs11Commands *self = BASTILE_PKCS11_COMMANDS (obj);
	BastilePkcs11CommandsPrivate *pv = BASTILE_PKCS11_COMMANDS_GET_PRIVATE (self);
	
	g_assert (pv->action_group == NULL);

	G_OBJECT_CLASS (bastile_pkcs11_commands_parent_class)->finalize (obj);
}

static void
bastile_pkcs11_commands_set_property (GObject *obj, guint prop_id, const GValue *value, 
                           GParamSpec *pspec)
{
	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
bastile_pkcs11_commands_get_property (GObject *obj, guint prop_id, GValue *value, 
                                       GParamSpec *pspec)
{
	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
bastile_pkcs11_commands_class_init (BastilePkcs11CommandsClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	BastileCommandsClass *cmd_class = BASTILE_COMMANDS_CLASS (klass);
	
	bastile_pkcs11_commands_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (BastilePkcs11CommandsPrivate));

	gobject_class->constructor = bastile_pkcs11_commands_constructor;
	gobject_class->dispose = bastile_pkcs11_commands_dispose;
	gobject_class->finalize = bastile_pkcs11_commands_finalize;
	gobject_class->set_property = bastile_pkcs11_commands_set_property;
	gobject_class->get_property = bastile_pkcs11_commands_get_property;
    
	cmd_class->show_properties = bastile_pkcs11_commands_show_properties;
	cmd_class->delete_objects = bastile_pkcs11_commands_delete_objects;

	slot_certificate_window = g_quark_from_static_string ("bastile-pkcs11-commands-window");

	commands_predicate.type = BASTILE_PKCS11_TYPE_CERTIFICATE;
		
	/* Register this as a source of commands */
	bastile_registry_register_type (bastile_registry_get (), BASTILE_PKCS11_TYPE_COMMANDS, 
	                                 BASTILE_PKCS11_TYPE_STR, "commands", NULL);
}

/* -----------------------------------------------------------------------------
 * PUBLIC 
 */

BastilePkcs11Commands*
bastile_pkcs11_commands_new (void)
{
	return g_object_new (BASTILE_PKCS11_TYPE_COMMANDS, NULL);
}
