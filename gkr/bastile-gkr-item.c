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

#include <string.h>

#include <glib/gi18n.h>

#include "bastile-context.h"
#include "bastile-source.h"
#include "bastile-gtkstock.h"
#include "bastile-util.h"
#include "bastile-secure-memory.h"

#include "bastile-gkr.h"
#include "bastile-gkr-item.h"
#include "bastile-gkr-operation.h"

/* For mate-keyring secret type ids */
#ifdef WITH_PGP
#include "pgp/bastile-pgp.h"
#endif
#ifdef WITH_SSH
#include "ssh/bastile-ssh.h"
#endif 

/* XXX Copied from libmateui */
#define MATE_STOCK_AUTHENTICATION      "mate-stock-authentication"
#define MATE_STOCK_BOOK_OPEN           "mate-stock-book-open"
#define MATE_STOCK_BLANK               "mate-stock-blank"

enum {
    PROP_0,
    PROP_KEYRING_NAME,
    PROP_ITEM_ID,
    PROP_ITEM_INFO,
    PROP_ITEM_ATTRIBUTES,
    PROP_ITEM_ACL,
    PROP_HAS_SECRET,
    PROP_USE
};

struct _BastileGkrItemPrivate {
	gchar *keyring_name;
	guint32 item_id;
	
	gpointer req_info;
	MateKeyringItemInfo *item_info;
	
	gpointer req_attrs;
	MateKeyringAttributeList *item_attrs;
	
	gpointer req_acl;
	gboolean got_acl;
	GList *item_acl;

	gpointer req_secret;
	gchar *item_secret;
};

G_DEFINE_TYPE (BastileGkrItem, bastile_gkr_item, BASTILE_TYPE_OBJECT);

/* -----------------------------------------------------------------------------
 * INTERNAL HELPERS
 */

GType
boxed_item_info_type (void)
{
	static GType type = 0;
	if (!type)
		type = g_boxed_type_register_static ("MateKeyringItemInfo", 
		                                     (GBoxedCopyFunc)mate_keyring_item_info_copy,
		                                     (GBoxedFreeFunc)mate_keyring_item_info_free);
	return type;
}

GType
boxed_attributes_type (void)
{
	static GType type = 0;
	if (!type)
		type = g_boxed_type_register_static ("MateKeyringAttributeList", 
		                                     (GBoxedCopyFunc)mate_keyring_attribute_list_copy,
		                                     (GBoxedFreeFunc)mate_keyring_attribute_list_free);
	return type;
}

GType
boxed_acl_type (void)
{
	static GType type = 0;
	if (!type)
		type = g_boxed_type_register_static ("MateKeyringAcl", 
		                                     (GBoxedCopyFunc)mate_keyring_acl_copy,
		                                     (GBoxedFreeFunc)mate_keyring_acl_free);
	return type;
}

static gboolean 
received_result (BastileGkrItem *self, MateKeyringResult result)
{
	if (result == MATE_KEYRING_RESULT_CANCELLED)
		return FALSE;
	
	if (result == MATE_KEYRING_RESULT_OK)
		return TRUE;

	/* TODO: Implement so that we can display an error icon, along with some text */
	g_message ("failed to retrieve item %d from keyring %s: %s",
	           self->pv->item_id, self->pv->keyring_name, 
	           mate_keyring_result_to_message (result));
	return FALSE;
}

static void
received_item_info (MateKeyringResult result, MateKeyringItemInfo *info, gpointer data)
{
	BastileGkrItem *self = BASTILE_GKR_ITEM (data);
	self->pv->req_info = NULL;
	if (received_result (self, result))
		bastile_gkr_item_set_info (self, info);
}

static void
load_item_info (BastileGkrItem *self)
{
	/* Already in progress */
	if (!self->pv->req_info) {
		g_object_ref (self);
		self->pv->req_info = mate_keyring_item_get_info_full (self->pv->keyring_name,
		                                                       self->pv->item_id,
		                                                       MATE_KEYRING_ITEM_INFO_BASICS,
		                                                       received_item_info,
		                                                       self, g_object_unref);
	}
}

static gboolean
require_item_info (BastileGkrItem *self)
{
	if (!self->pv->item_info)
		load_item_info (self);
	return self->pv->item_info != NULL;
}

static void
received_item_secret (MateKeyringResult result, MateKeyringItemInfo *info, gpointer data)
{
	BastileGkrItem *self = BASTILE_GKR_ITEM (data);
	self->pv->req_secret = NULL;
	if (received_result (self, result)) {
		g_free (self->pv->item_secret);
		WITH_SECURE_MEM (self->pv->item_secret = mate_keyring_item_info_get_secret (info));
		g_object_notify (G_OBJECT (self), "has-secret");
	}
}

static void
load_item_secret (BastileGkrItem *self)
{
	/* Already in progress */
	if (!self->pv->req_secret) {
		g_object_ref (self);
		self->pv->req_secret = mate_keyring_item_get_info_full (self->pv->keyring_name,
		                                                         self->pv->item_id,
		                                                         MATE_KEYRING_ITEM_INFO_SECRET,
		                                                         received_item_secret,
		                                                         self, g_object_unref);
	}	
}

static gboolean
require_item_secret (BastileGkrItem *self)
{
	if (!self->pv->item_secret)
		load_item_secret (self);
	return self->pv->item_secret != NULL;
}

static void
received_item_attrs (MateKeyringResult result, MateKeyringAttributeList *attrs, gpointer data)
{
	BastileGkrItem *self = BASTILE_GKR_ITEM (data);
	self->pv->req_attrs = NULL;
	if (received_result (self, result))
		bastile_gkr_item_set_attributes (self, attrs);
}

static void
load_item_attrs (BastileGkrItem *self)
{
	/* Already in progress */
	if (!self->pv->req_attrs) {
		g_object_ref (self);
		self->pv->req_attrs = mate_keyring_item_get_attributes (self->pv->keyring_name,
		                                                         self->pv->item_id,
		                                                         received_item_attrs,
		                                                         self, g_object_unref);
	}	
}

static gboolean
require_item_attrs (BastileGkrItem *self)
{
	if (!self->pv->item_attrs)
		load_item_attrs (self);
	return self->pv->item_attrs != NULL;
}

static void
received_item_acl (MateKeyringResult result, GList *acl, gpointer data)
{
	BastileGkrItem *self = BASTILE_GKR_ITEM (data);
	self->pv->req_acl = NULL;
	if (received_result (self, result)) {
		self->pv->got_acl = TRUE;
		bastile_gkr_item_set_acl (self, acl);
	}
}

static void
load_item_acl (BastileGkrItem *self)
{
	/* Already in progress */
	if (!self->pv->req_acl) {
		g_object_ref (self);
		self->pv->req_acl = mate_keyring_item_get_acl (self->pv->keyring_name,
		                                                self->pv->item_id,
		                                                received_item_acl,
		                                                self, g_object_unref);
	}
}

static gboolean
require_item_acl (BastileGkrItem *self)
{
	if (!self->pv->got_acl)
		load_item_acl (self);
	return self->pv->got_acl;
}

static guint32
find_attribute_int (MateKeyringAttributeList *attrs, const gchar *name)
{
    guint i;
    
    if (!attrs)
        return 0;
    
    for (i = 0; i < attrs->len; i++) {
        MateKeyringAttribute *attr = &(mate_keyring_attribute_list_index (attrs, i));
        if (g_ascii_strcasecmp (name, attr->name) == 0 && 
            attr->type == MATE_KEYRING_ATTRIBUTE_TYPE_UINT32)
            return attr->value.integer;
    }
    
    return 0;
}


static gboolean
is_network_item (BastileGkrItem *self, const gchar *match)
{
	const gchar *protocol;
	
	if (!require_item_info (self))
		return FALSE;
	
	if (mate_keyring_item_info_get_type (self->pv->item_info) != MATE_KEYRING_ITEM_NETWORK_PASSWORD)
		return FALSE;
    
	if (!match)
		return TRUE;
    
	protocol = bastile_gkr_item_get_attribute (self, "protocol");
	return protocol && g_ascii_strncasecmp (protocol, match, strlen (match)) == 0;
}

static gboolean 
is_custom_display_name (BastileGkrItem *self, const gchar *display)
{
	const gchar *user;
	const gchar *server;
	const gchar *object;
	guint32 port;
	GString *generated;
	gboolean ret;
	
	if (require_item_info (self) && mate_keyring_item_info_get_type (self->pv->item_info) != 
					    MATE_KEYRING_ITEM_NETWORK_PASSWORD)
		return TRUE;
    
	if (!require_item_attrs (self) || !display)
		return TRUE;
    
	/* 
	 * For network passwords mate-keyring generates in a funky looking display 
	 * name that's generated from login credentials. We simulate that generating 
	 * here and return FALSE if it matches. This allows us to display user 
	 * customized display names and ignore the generated ones.
	 */
    
	user = bastile_gkr_item_get_attribute (self, "user");
	server = bastile_gkr_item_get_attribute (self, "server");
	object = bastile_gkr_item_get_attribute (self, "object");
	port = find_attribute_int (self->pv->item_attrs, "port");
    
	if (!server)
		return TRUE;
    
	generated = g_string_new (NULL);
	if (user != NULL)
		g_string_append_printf (generated, "%s@", user);
	g_string_append (generated, server);
	if (port != 0)
		g_string_append_printf (generated, ":%d", port);
	if (object != NULL)
		g_string_append_printf (generated, "/%s", object);

	ret = strcmp (display, generated->str) != 0;
	g_string_free (generated, TRUE);
    
	return ret;
}

static gchar*
calc_display_name (BastileGkrItem *self, gboolean always)
{
	const gchar *val;
	gchar *display;
    
	if (!require_item_info (self))
		return NULL;
	
	display = mate_keyring_item_info_get_display_name (self->pv->item_info);
    
	/* If it's customized by the application or user then display that */
	if (is_custom_display_name (self, display))
		return display;
    
	/* If it's a network item ... */
	if (mate_keyring_item_info_get_type (self->pv->item_info) == MATE_KEYRING_ITEM_NETWORK_PASSWORD) {
        
		/* HTTP usually has a the realm as the "object" display that */
		if (is_network_item (self, "http") && self->pv->item_attrs) {
			val = bastile_gkr_item_get_attribute (self, "object");
			if (val && val[0]) {
				g_free (display);
				return g_strdup (val);
			}
		}
        
		/* Display the server name as a last resort */
		if (always) {
			val = bastile_gkr_item_get_attribute (self, "server");
			if (val && val[0]) {
				g_free (display);
				return g_strdup (val);
			}
		}
	}
    
	return always ? display : NULL;
}

static gchar*
calc_network_item_markup (BastileGkrItem *self)
{
	const gchar *object;
	const gchar *user;
	const gchar *protocol;
	const gchar *server;
    
	gchar *uri = NULL;
	gchar *display = NULL;
	gchar *ret;
    
	server = bastile_gkr_item_get_attribute (self, "server");
	protocol = bastile_gkr_item_get_attribute (self, "protocol");
	object = bastile_gkr_item_get_attribute (self, "object");
	user = bastile_gkr_item_get_attribute (self, "user");

	if (!protocol)
		return NULL;
    
	/* The object in HTTP often isn't a path at all */
	if (is_network_item (self, "http"))
		object = NULL;
    
	display = calc_display_name (self, TRUE);
    
	if (server && protocol) {
		uri = g_strdup_printf ("  %s://%s%s%s/%s", 
		                       protocol, 
		                       user ? user : "",
		                       user ? "@" : "",
		                       server,
		                       object ? object : "");
	}
    
	ret = g_markup_printf_escaped ("%s<span foreground='#555555' size='small' rise='0'>%s</span>",
	                               display, uri ? uri : "");
	g_free (display);
	g_free (uri);
    
	return ret;
}

static gchar* 
calc_name_markup (BastileGkrItem *self)
{
	gchar *name, *markup = NULL;
	
	if (!require_item_info (self))
		return NULL;
    
	/* Only do our special markup for network passwords */
	if (is_network_item (self, NULL))
        markup = calc_network_item_markup (self);
    
	if (!markup) {
		name = calc_display_name (self, TRUE);
		markup = g_markup_escape_text (name, -1);
		g_free (name);
	}

	return markup;
}

static gint
calc_item_type (BastileGkrItem *self)
{
	if (!require_item_info (self))
		return -1;
	return mate_keyring_item_info_get_type (self->pv->item_info);
}

/* -----------------------------------------------------------------------------
 * OBJECT 
 */

static void
bastile_gkr_item_realize (BastileObject *obj)
{
	BastileGkrItem *self = BASTILE_GKR_ITEM (obj);
	const gchar *description;
	gchar *display;
	gchar *markup;
	gchar *identifier;
	const gchar *icon;

	if (is_network_item (self, "http")) 
		description = _("Web Password");
	else if (is_network_item (self, NULL)) 
		description = _("Network Password");
	else
		description = _("Password");

	display = calc_display_name (self, TRUE);
	markup = calc_name_markup(self);
	identifier = g_strdup_printf ("%u", self->pv->item_id);

	/* We use a pointer so we don't copy the string every time */
	switch (calc_item_type (self))
	{
	case MATE_KEYRING_ITEM_GENERIC_SECRET:
		icon = MATE_STOCK_AUTHENTICATION;
		break;
	case MATE_KEYRING_ITEM_NETWORK_PASSWORD:
		icon = is_network_item (self, "http") ? BASTILE_THEMED_WEBBROWSER : GTK_STOCK_NETWORK;
		break;
	case MATE_KEYRING_ITEM_NOTE:
		icon = MATE_STOCK_BOOK_OPEN;
		break;
	default:
		icon = MATE_STOCK_BLANK;
		break;
	}
	
	g_object_set (self,
		      "label", display,
		      "icon", icon,
		      "markup", markup,
		      "identifier", identifier,
		      "description", description,
		      "flags", 0,
		      NULL);
	
	g_free (display);
	g_free (markup);
	g_free (identifier);
	
	g_object_notify (G_OBJECT (self), "has-secret");
	g_object_notify (G_OBJECT (self), "use");
	
	BASTILE_OBJECT_CLASS (bastile_gkr_item_parent_class)->realize (obj);
}

static void
bastile_gkr_item_refresh (BastileObject *obj)
{
	BastileGkrItem *self = BASTILE_GKR_ITEM (obj);
	
	if (self->pv->item_info)
		load_item_info (self);
	if (self->pv->item_attrs)
		load_item_attrs (self);
	if (self->pv->got_acl)
		load_item_acl (self);
	if (self->pv->item_secret)
		load_item_secret (self);

	BASTILE_OBJECT_CLASS (bastile_gkr_item_parent_class)->refresh (obj);
}

static BastileOperation*
bastile_gkr_item_delete (BastileObject *obj)
{
	BastileGkrItem *self = BASTILE_GKR_ITEM (obj);
	return bastile_gkr_operation_delete_item (self);
}

static void
bastile_gkr_item_init (BastileGkrItem *self)
{
	self->pv = G_TYPE_INSTANCE_GET_PRIVATE (self, BASTILE_TYPE_GKR_ITEM, BastileGkrItemPrivate);
	g_object_set (self, "usage", BASTILE_USAGE_CREDENTIALS, "tag", BASTILE_GKR_TYPE, "location", BASTILE_LOCATION_LOCAL, NULL);
}

static GObject* 
bastile_gkr_item_constructor (GType type, guint n_props, GObjectConstructParam *props) 
{
	GObject *obj = G_OBJECT_CLASS (bastile_gkr_item_parent_class)->constructor (type, n_props, props);
	BastileGkrItem *self = NULL;
	GQuark id;
	
	if (obj) {
		self = BASTILE_GKR_ITEM (obj);
		
		id = bastile_gkr_item_get_cannonical (self->pv->keyring_name, 
		                                        self->pv->item_id);
		g_object_set (self, "id", id, 
		              "usage", BASTILE_USAGE_CREDENTIALS, NULL); 
	}
	
	return obj;
}

static void
bastile_gkr_item_get_property (GObject *object, guint prop_id,
                                GValue *value, GParamSpec *pspec)
{
	BastileGkrItem *self = BASTILE_GKR_ITEM (object);
    
	switch (prop_id) {
	case PROP_KEYRING_NAME:
		g_value_set_string (value, bastile_gkr_item_get_keyring_name (self));
		break;
	case PROP_ITEM_ID:
		g_value_set_uint (value, bastile_gkr_item_get_item_id (self));
		break;
	case PROP_ITEM_INFO:
		g_value_set_boxed (value, bastile_gkr_item_get_info (self));
		break;
	case PROP_ITEM_ATTRIBUTES:
		g_value_set_boxed (value, bastile_gkr_item_get_attributes (self));
		break;
	case PROP_ITEM_ACL:
		g_value_set_boxed (value, bastile_gkr_item_get_acl (self));
		break;
	case PROP_HAS_SECRET:
		g_value_set_boolean (value, self->pv->item_secret != NULL);
		break;
	case PROP_USE:
		g_value_set_uint (value, bastile_gkr_item_get_use (self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;		
	}
}

static void
bastile_gkr_item_set_property (GObject *object, guint prop_id, const GValue *value, 
                                GParamSpec *pspec)
{
	BastileGkrItem *self = BASTILE_GKR_ITEM (object);
    
	switch (prop_id) {
	case PROP_KEYRING_NAME:
		g_return_if_fail (self->pv->keyring_name == NULL);
		self->pv->keyring_name = g_value_dup_string (value);
		break;
	case PROP_ITEM_ID:
		g_return_if_fail (self->pv->item_id == 0);
		self->pv->item_id = g_value_get_uint (value);
		break;
	case PROP_ITEM_INFO:
		bastile_gkr_item_set_info (self, g_value_get_boxed (value));
		break;
	case PROP_ITEM_ATTRIBUTES:
		bastile_gkr_item_set_attributes (self, g_value_get_boxed (value));
		break;
	case PROP_ITEM_ACL:
		bastile_gkr_item_set_acl (self, g_value_get_boxed (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
bastile_gkr_item_finalize (GObject *gobject)
{
	BastileGkrItem *self = BASTILE_GKR_ITEM (gobject);
	
	g_free (self->pv->keyring_name);
	self->pv->keyring_name = NULL;
    
	if (self->pv->item_info)
		mate_keyring_item_info_free (self->pv->item_info);
	self->pv->item_info = NULL;
	g_assert (self->pv->req_info == NULL);
    
	if (self->pv->item_attrs)
		mate_keyring_attribute_list_free (self->pv->item_attrs);
	self->pv->item_attrs = NULL;
	g_assert (self->pv->req_attrs == NULL);

	mate_keyring_acl_free (self->pv->item_acl);
	self->pv->item_acl = NULL;
	self->pv->got_acl = FALSE;
	g_assert (self->pv->req_acl == NULL);

	g_free (self->pv->item_secret);
	self->pv->item_secret = NULL;
	g_assert (self->pv->req_secret == NULL);
	
	G_OBJECT_CLASS (bastile_gkr_item_parent_class)->finalize (gobject);
}

static void
bastile_gkr_item_class_init (BastileGkrItemClass *klass)
{
	GObjectClass *gobject_class;
	BastileObjectClass *bastile_class;
    
	bastile_gkr_item_parent_class = g_type_class_peek_parent (klass);
	gobject_class = G_OBJECT_CLASS (klass);
	
	gobject_class->constructor = bastile_gkr_item_constructor;
	gobject_class->finalize = bastile_gkr_item_finalize;
	gobject_class->set_property = bastile_gkr_item_set_property;
	gobject_class->get_property = bastile_gkr_item_get_property;
	
	bastile_class = BASTILE_OBJECT_CLASS (klass);
	bastile_class->realize = bastile_gkr_item_realize;
	bastile_class->refresh = bastile_gkr_item_refresh;
	bastile_class->delete = bastile_gkr_item_delete;
	
	g_type_class_add_private (klass, sizeof (BastileGkrItemPrivate));
    
	g_object_class_install_property (gobject_class, PROP_KEYRING_NAME,
	        g_param_spec_string("keyring-name", "Keyring Name", "Keyring this item is in", 
	                            "", G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (gobject_class, PROP_ITEM_ID,
	        g_param_spec_uint ("item-id", "Item ID", "MATE Keyring Item ID", 
	                           0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (gobject_class, PROP_ITEM_INFO,
                g_param_spec_boxed ("item-info", "Item Info", "MATE Keyring Item Info",
                                    boxed_item_info_type (),  G_PARAM_READWRITE));
                              
	g_object_class_install_property (gobject_class, PROP_ITEM_ATTRIBUTES,
                g_param_spec_boxed ("item-attributes", "Item Attributes", "MATE Keyring Item Attributes",
                                    boxed_attributes_type (), G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_ITEM_ACL,
                g_param_spec_boxed ("item-acl", "Item ACL", "MATE Keyring Item ACL",
                                    boxed_acl_type (),  G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_USE,
	        g_param_spec_uint ("use", "Use", "Item is used for", 
	                           0, G_MAXUINT, 0, G_PARAM_READABLE));

	g_object_class_install_property (gobject_class, PROP_HAS_SECRET,
	        g_param_spec_boolean ("has-secret", "Has Secret", "Secret has been loaded", 
	                              FALSE, G_PARAM_READABLE));
}

/* -----------------------------------------------------------------------------
 * PUBLIC 
 */

BastileGkrItem* 
bastile_gkr_item_new (BastileSource *source, const gchar *keyring_name, guint32 item_id)
{
	return g_object_new (BASTILE_TYPE_GKR_ITEM, "source", source, 
	                     "item-id", item_id, "keyring-name", keyring_name, NULL);
}

guint32
bastile_gkr_item_get_item_id (BastileGkrItem *self)
{
	g_return_val_if_fail (BASTILE_IS_GKR_ITEM (self), 0);
	return self->pv->item_id;
}

const gchar*
bastile_gkr_item_get_keyring_name (BastileGkrItem *self)
{
	g_return_val_if_fail (BASTILE_IS_GKR_ITEM (self), NULL);
	return self->pv->keyring_name;
}

MateKeyringItemInfo*
bastile_gkr_item_get_info (BastileGkrItem *self)
{
	g_return_val_if_fail (BASTILE_IS_GKR_ITEM (self), NULL);
	require_item_info (self);
	return self->pv->item_info;
}

void
bastile_gkr_item_set_info (BastileGkrItem *self, MateKeyringItemInfo* info)
{
	GObject *obj;
	
	g_return_if_fail (BASTILE_IS_GKR_ITEM (self));
	
	if (self->pv->item_info)
		mate_keyring_item_info_free (self->pv->item_info);
	if (info)
		self->pv->item_info = mate_keyring_item_info_copy (info);
	else
		self->pv->item_info = NULL;
	
	obj = G_OBJECT (self);
	g_object_freeze_notify (obj);
	bastile_gkr_item_realize (BASTILE_OBJECT (self));
	g_object_notify (obj, "item-info");
	g_object_notify (obj, "use");
	
	/* Get the secret out of the item info, if not already loaded */
	if (!self->pv->item_secret && self->pv->item_info && !self->pv->req_secret) {
		WITH_SECURE_MEM (self->pv->item_secret = mate_keyring_item_info_get_secret (self->pv->item_info));
		g_object_notify (obj, "has-secret");
	}
		
	g_object_thaw_notify (obj);
}

gboolean
bastile_gkr_item_has_secret (BastileGkrItem *self)
{
	g_return_val_if_fail (BASTILE_IS_GKR_ITEM (self), FALSE);
	return self->pv->item_secret != NULL;
}

const gchar*
bastile_gkr_item_get_secret (BastileGkrItem *self)
{
	g_return_val_if_fail (BASTILE_IS_GKR_ITEM (self), NULL);
	require_item_secret (self);
	return self->pv->item_secret;
}

MateKeyringAttributeList*
bastile_gkr_item_get_attributes (BastileGkrItem *self)
{
	g_return_val_if_fail (BASTILE_IS_GKR_ITEM (self), NULL);
	require_item_attrs (self);
	return self->pv->item_attrs;
}

void
bastile_gkr_item_set_attributes (BastileGkrItem *self, MateKeyringAttributeList* attrs)
{
	GObject *obj;
	
	g_return_if_fail (BASTILE_IS_GKR_ITEM (self));
	
	if (self->pv->item_attrs)
		mate_keyring_attribute_list_free (self->pv->item_attrs);
	if (attrs)
		self->pv->item_attrs = mate_keyring_attribute_list_copy (attrs);
	else
		self->pv->item_attrs = NULL;
	
	obj = G_OBJECT (self);
	g_object_freeze_notify (obj);
	bastile_gkr_item_realize (BASTILE_OBJECT (self));
	g_object_notify (obj, "item-attributes");
	g_object_notify (obj, "use");
	g_object_thaw_notify (obj);
}

const gchar*
bastile_gkr_item_get_attribute (BastileGkrItem *self, const gchar *name)
{
	g_return_val_if_fail (BASTILE_IS_GKR_ITEM (self), NULL);
	if (!require_item_attrs (self))
		return NULL;
	return bastile_gkr_find_string_attribute (self->pv->item_attrs, name);
}

const gchar*
bastile_gkr_find_string_attribute (MateKeyringAttributeList *attrs, const gchar *name)
{
	guint i;
	
	g_return_val_if_fail (attrs, NULL);
	g_return_val_if_fail (name, NULL);

	for (i = 0; i < attrs->len; i++) {
		MateKeyringAttribute *attr = &(mate_keyring_attribute_list_index (attrs, i));
		if (g_ascii_strcasecmp (name, attr->name) == 0 && 
				attr->type == MATE_KEYRING_ATTRIBUTE_TYPE_STRING)
			return attr->value.string;
	}
	    
	return NULL;	
}

GList*
bastile_gkr_item_get_acl (BastileGkrItem *self)
{
	g_return_val_if_fail (BASTILE_IS_GKR_ITEM (self), NULL);
	require_item_acl (self);
	return self->pv->item_acl;
}

void
bastile_gkr_item_set_acl (BastileGkrItem *self, GList* acl)
{
	GObject *obj;
	
	g_return_if_fail (BASTILE_IS_GKR_ITEM (self));
	
	if (self->pv->item_acl)
		mate_keyring_acl_free (self->pv->item_acl);
	if (acl)
		self->pv->item_acl = mate_keyring_acl_copy (acl);
	else
		self->pv->item_acl = NULL;
	
	obj = G_OBJECT (self);
	g_object_freeze_notify (obj);
	bastile_gkr_item_realize (BASTILE_OBJECT (self));
	g_object_notify (obj, "item-acl");
	g_object_thaw_notify (obj);
}

GQuark
bastile_gkr_item_get_cannonical (const gchar *keyring_name, guint32 item_id)
{
	gchar *buf = g_strdup_printf ("%s:%s-%08X", BASTILE_GKR_STR, keyring_name, item_id);
	GQuark id = g_quark_from_string (buf);
	g_free (buf);
	return id;
}

BastileGkrUse
bastile_gkr_item_get_use (BastileGkrItem *self)
{
	const gchar *val;
    
	/* Network passwords */
	if (require_item_info (self)) {
		if (mate_keyring_item_info_get_type (self->pv->item_info) == MATE_KEYRING_ITEM_NETWORK_PASSWORD) {
			if (is_network_item (self, "http"))
				return BASTILE_GKR_USE_WEB;
			return BASTILE_GKR_USE_NETWORK;
		}
	}
    
	if (require_item_attrs (self)) {
		val = bastile_gkr_item_get_attribute (self, "bastile-key-type");
		if (val) {
#ifdef WITH_PGP
			if (strcmp (val, BASTILE_PGP_TYPE_STR) == 0)
				return BASTILE_GKR_USE_PGP;
#endif
#ifdef WITH_SSH
			if (strcmp (val, BASTILE_SSH_TYPE_STR) == 0)
				return BASTILE_GKR_USE_SSH;
#endif
		}
	}
    
	return BASTILE_GKR_USE_OTHER;
}
