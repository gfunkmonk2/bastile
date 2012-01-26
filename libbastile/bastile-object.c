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

#include "bastile-object.h"

#include "bastile-context.h"
#include "bastile-source.h"

#include "string.h"

#include <glib/gi18n.h>

/**
 * _BastileObjectProps:
 * @PROP_0:
 * @PROP_CONTEXT:
 * @PROP_SOURCE:
 * @PROP_PREFERRED:
 * @PROP_PARENT:
 * @PROP_ID:
 * @PROP_TAG:
 * @PROP_LABEL:
 * @PROP_MARKUP:
 * @PROP_NICKNAME:
 * @PROP_DESCRIPTION:
 * @PROP_ICON:
 * @PROP_IDENTIFIER:
 * @PROP_LOCATION:
 * @PROP_USAGE:
 * @PROP_FLAGS:
 */
enum _BastileObjectProps {
	PROP_0,
	PROP_CONTEXT,
	PROP_SOURCE,
	PROP_PREFERRED,
	PROP_PARENT,
	PROP_ID,
	PROP_TAG,
	PROP_LABEL,
	PROP_MARKUP,
	PROP_NICKNAME,
	PROP_DESCRIPTION,
	PROP_ICON,
	PROP_IDENTIFIER,
	PROP_LOCATION,
	PROP_USAGE,
	PROP_FLAGS
};

/**
 * _BastileObjectPrivate:
 * @id: the id of the object
 * @tag: DBUS: "ktype"
 * @tag_explicit: If TRUE the tag will not be set automatically
 * @source: Where did the object come from ?
 * @context:
 * @preferred: Points to the object to prefer over this one
 * @parent: the object's parent
 * @children: a list of children the object has
 * @label: DBUS: "display-name"
 * @markup: Markup text
 * @markup_explicit: If TRUE the markup will not be set automatically
 * @nickname: DBUS: "simple-name"
 * @nickname_explicit: If TRUE the nickname will not be set automatically
 * @description:  Description text DBUS: "key-desc"
 * @description_explicit: If TRUE the description will not be set automatically
 * @icon: DBUS: "stock-id"
 * @identifier: DBUS: "key-id", "display-id", "raw-id"
 * @identifier_explicit:
 * @location: describes the loaction of the object (local, remte, invalid...)
 * @usage: DBUS: "etype"
 * @flags:
 * @realizing: set while the object is realizing
 * @realized: set as soon as the object is realized
 */
struct _BastileObjectPrivate {
	GQuark id;
	GQuark tag;
	gboolean tag_explicit;

	BastileSource *source;
	BastileContext *context;
	BastileObject *preferred;
	BastileObject *parent;
	GList *children;
	
    gchar *label;             
    gchar *markup;
    gboolean markup_explicit;
    gchar *nickname;
    gboolean nickname_explicit;
    gchar *description;
    gboolean description_explicit;
    gchar *icon;
    gchar *identifier;
    gboolean identifier_explicit;

    BastileLocation location;
    BastileUsage usage;
    guint flags;

	gboolean realized;
	gboolean realizing;
};

G_DEFINE_TYPE (BastileObject, bastile_object, G_TYPE_OBJECT);

/* -----------------------------------------------------------------------------
 * INTERNAL 
 */

/**
 * register_child:
 * @self: The parent
 * @child: The new child
 *
 *
 * Sets @child as a child of @self
 */
static void 
register_child (BastileObject* self, BastileObject* child) 
{
	g_assert (BASTILE_IS_OBJECT (self));
	g_assert (BASTILE_IS_OBJECT (child));
	g_assert (self != child);
	g_assert (child->pv->parent == NULL);
	
	child->pv->parent = self;
	self->pv->children = g_list_append (self->pv->children, child);
}

/**
 * unregister_child:
 * @self: The parent
 * @child: child to remove
 *
 *
 * removes @child from the children list in @self
 */
static void 
unregister_child (BastileObject* self, BastileObject* child) 
{
	g_assert (BASTILE_IS_OBJECT (self));
	g_assert (BASTILE_IS_OBJECT (child));
	g_assert (self != child);
	g_assert (child->pv->parent == self);
	
	child->pv->parent = NULL;
	self->pv->children = g_list_remove (self->pv->children, child);
}

/**
 * set_string_storage:
 * @value: The value to store
 * @storage: The datastore to write the value to
 *
 * When storing, new memory will be allocated and the value copied
 *
 * Returns: TRUE if the value has been set new, FALSE else
 */
static gboolean
set_string_storage (const gchar *value, gchar **storage)
{
	g_assert (storage);

	if (value == NULL)
		value = "";
	
	if (!value || !*storage || !g_str_equal (value, *storage)) {
		g_free (*storage);
		*storage = g_strdup (value);
		return TRUE;
	}
	
	return FALSE;
}

/**
 * take_string_storage:
 * @value: The value to store
 * @storage: The datastore to write the value to
 *
 * When storing, the pointer to value is used. No new memory will be allocated
 *
 * Returns: TRUE if the value has been set new, FALSE else
 */
static gboolean
take_string_storage (gchar *value, gchar **storage)
{
	g_assert (storage);
	
	if (value == NULL)
		value = g_strdup ("");
	
	if (!value || !*storage || !g_str_equal (value, *storage)) {
		g_free (*storage);
		*storage = value;
		return TRUE;
	}
	
	g_free (value);
	return FALSE;
}

/**
 * recalculate_id:
 * @self: object to calculate for
 *
 * recalculates tag and identifier from id
 *
 */
static void
recalculate_id (BastileObject *self)
{
	const gchar *str;
	const gchar *at;
	GQuark tag;
	gchar *result;
	gsize len;

	/* No id, clear any tag and auto generated identifer */
	if (!self->pv->id) {
		if (!self->pv->tag_explicit) {
			self->pv->tag = 0;
			g_object_notify (G_OBJECT (self), "tag");
		}
			
		if (!self->pv->identifier_explicit) {
			if (set_string_storage ("", &self->pv->identifier))
				g_object_notify (G_OBJECT (self), "identifier");
		}
		
	/* Have hande, configure tag and auto generated identifer */
	} else {
		str = g_quark_to_string (self->pv->id);
		len = strlen (str);
		at = strchr (str, ':');
		
		if (!self->pv->tag_explicit) {
			result = g_strndup (str, at ? at - str : len);
			tag = g_quark_from_string (result);
			g_free (result);
			
			if (tag != self->pv->tag) {
				self->pv->tag = tag;
				g_object_notify (G_OBJECT (self), "tag");
			}
		}
		
		if (!self->pv->identifier_explicit) {
			if (set_string_storage (at ? at + 1 : "", &self->pv->identifier))
				g_object_notify (G_OBJECT (self), "identifier");
		}
	}
}

/**
 * recalculate_label:
 * @self: object to recalculate label for
 *
 * Recalculates nickname and markup from the label
 *
 */
static void
recalculate_label (BastileObject *self)
{
	if (!self->pv->markup_explicit) {
		if (take_string_storage (g_markup_escape_text (self->pv->label ? self->pv->label : "", -1),
		                         &self->pv->markup))
			g_object_notify (G_OBJECT (self), "markup");
	}

	if (!self->pv->nickname_explicit) {
		if (set_string_storage (self->pv->label, &self->pv->nickname))
			g_object_notify (G_OBJECT (self), "nickname");
	}
}

/**
 * recalculate_usage:
 * @self: The #BastileObject to recalculate the usage decription for
 *
 * Basing on the usage identifiere this function will calculate a usage
 * description and store it in the object.
 *
 */
static void
recalculate_usage (BastileObject *self)
{
	const gchar *desc;
	
	if (!self->pv->description_explicit) {
		
		switch (self->pv->usage) {
		case BASTILE_USAGE_SYMMETRIC_KEY:
			desc = _("Symmetric Key");
			break;
		case BASTILE_USAGE_PUBLIC_KEY:
			desc = _("Public Key");
			break;
		case BASTILE_USAGE_PRIVATE_KEY:
			desc = _("Private Key");
			break;
		case BASTILE_USAGE_CREDENTIALS:
			desc = _("Credentials");
			break;
		case BASTILE_USAGE_IDENTITY:
		    /*
		     * Translators: "This object is a means of storing items such as 
		     * name, email address, etc. that make up one's digital identity.
		     */
			desc = _("Identity");
			break;
		default:
			desc = "";
			break;
		}
		
		if (set_string_storage (desc, &self->pv->description))
			g_object_notify (G_OBJECT (self), "description");
	}
}


/* -----------------------------------------------------------------------------
 * OBJECT 
 */

/**
 * bastile_object_init:
 * @self: The object to initialise
 *
 * Initialises the object with default data
 *
 */
static void
bastile_object_init (BastileObject *self)
{
	self->pv = G_TYPE_INSTANCE_GET_PRIVATE (self, BASTILE_TYPE_OBJECT, 
	                                        BastileObjectPrivate);
	self->pv->label = g_strdup ("");
	self->pv->markup = g_strdup ("");
	self->pv->description = g_strdup ("");
	self->pv->icon = g_strdup ("gtk-missing-image");
	self->pv->identifier = g_strdup ("");
	self->pv->location = BASTILE_LOCATION_INVALID;
	self->pv->usage = BASTILE_USAGE_NONE;
}


/**
 * bastile_object_dispose:
 * @obj: A #BastileObject to dispose
 *
 * Before this object is disposed, all it's children get new parents
 *
 */
static void
bastile_object_dispose (GObject *obj)
{
	BastileObject *self = BASTILE_OBJECT (obj);
	BastileObject *parent;
	GList *l, *children;
	
	if (self->pv->context != NULL) {
		bastile_context_remove_object (self->pv->context, self);
		g_assert (self->pv->context == NULL);
	}
	
	if (self->pv->source != NULL) {
		g_object_remove_weak_pointer (G_OBJECT (self->pv->source), (gpointer*)&self->pv->source);
		self->pv->source = NULL;
	}
	
	if (self->pv->preferred != NULL) {
		g_object_remove_weak_pointer (G_OBJECT (self->pv->source), (gpointer*)&self->pv->preferred);
		self->pv->preferred = NULL;
	}

	/* 
	 * When an object is destroyed, we reparent all
	 * children to this objects parent. If no parent
	 * of this object, all children become root objects.
	 */

	parent = self->pv->parent;
	if (parent)
		g_object_ref (parent);

	children = g_list_copy (self->pv->children);
	for (l = children; l; l = g_list_next (l)) {
		g_return_if_fail (BASTILE_IS_OBJECT (l->data));
		bastile_object_set_parent (l->data, parent);
	}
	g_list_free (children);

	if (parent)
		g_object_unref (parent);
	
	g_assert (self->pv->children == NULL);
	
	/* Now remove this object from its parent */
	bastile_object_set_parent (self, NULL);
	
	G_OBJECT_CLASS (bastile_object_parent_class)->dispose (obj);	
}

/**
 * bastile_object_finalize:
 * @obj: #BastileObject to finalize
 *
 */
static void
bastile_object_finalize (GObject *obj)
{
	BastileObject *self = BASTILE_OBJECT (obj);
	
	g_assert (self->pv->source == NULL);
	g_assert (self->pv->preferred == NULL);
	g_assert (self->pv->parent == NULL);
	g_assert (self->pv->context == NULL);
	g_assert (self->pv->children == NULL);
	
	g_free (self->pv->label);
	self->pv->label = NULL;
	
	g_free (self->pv->markup);
	self->pv->markup = NULL;
	
	g_free (self->pv->nickname);
	self->pv->nickname = NULL;
	
	g_free (self->pv->description);
	self->pv->description = NULL;
	
	g_free (self->pv->icon);
	self->pv->icon = NULL;
	
	g_free (self->pv->identifier);
	self->pv->identifier = NULL;

	G_OBJECT_CLASS (bastile_object_parent_class)->finalize (obj);
}


/**
 * bastile_object_get_property:
 * @obj: The object to get the property for
 * @prop_id: The property requested
 * @value: out - the value as #GValue
 * @pspec: a #GParamSpec for the warning
 *
 * Returns: The property of the object @obj defined by the id @prop_id in @value.
 *
 */
static void
bastile_object_get_property (GObject *obj, guint prop_id, GValue *value, 
                           GParamSpec *pspec)
{
	BastileObject *self = BASTILE_OBJECT (obj);
	
	switch (prop_id) {
	case PROP_CONTEXT:
		g_value_set_object (value, bastile_object_get_context (self));
		break;
	case PROP_SOURCE:
		g_value_set_object (value, bastile_object_get_source (self));
		break;
	case PROP_PREFERRED:
		g_value_set_object (value, bastile_object_get_preferred (self));
		break;
	case PROP_PARENT:
		g_value_set_object (value, bastile_object_get_parent (self));
		break;
	case PROP_ID:
		g_value_set_uint (value, bastile_object_get_id (self));
		break;
	case PROP_TAG:
		g_value_set_uint (value, bastile_object_get_tag (self));
		break;
	case PROP_LABEL:
		g_value_set_string (value, bastile_object_get_label (self));
		break;
	case PROP_NICKNAME:
		g_value_set_string (value, bastile_object_get_nickname (self));
		break;
	case PROP_MARKUP:
		g_value_set_string (value, bastile_object_get_markup (self));
		break;
	case PROP_DESCRIPTION:
		g_value_set_string (value, bastile_object_get_description (self));
		break;
	case PROP_ICON:
		g_value_set_string (value, bastile_object_get_icon (self));
		break;
	case PROP_IDENTIFIER:
		g_value_set_string (value, bastile_object_get_identifier (self));
		break;
	case PROP_LOCATION:
		g_value_set_enum (value, bastile_object_get_location (self));
		break;
	case PROP_USAGE:
		g_value_set_enum (value, bastile_object_get_usage (self));
		break;
	case PROP_FLAGS:
		g_value_set_uint (value, bastile_object_get_flags (self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

/**
 * bastile_object_set_property:
 * @obj: Object to set the property in
 * @prop_id: the property to set
 * @value: the value to set
 * @pspec: To be used for warnings. #GParamSpec
 *
 * Sets a property in this object
 *
 */
static void
bastile_object_set_property (GObject *obj, guint prop_id, const GValue *value, 
                              GParamSpec *pspec)
{
	BastileObject *self = BASTILE_OBJECT (obj);
	BastileLocation loc;
	BastileUsage usage;
	guint flags;
	GQuark quark;
	
	switch (prop_id) {
	case PROP_CONTEXT:
		if (self->pv->context) 
			g_object_remove_weak_pointer (G_OBJECT (self->pv->context), (gpointer*)&self->pv->context);
		self->pv->context = BASTILE_CONTEXT (g_value_get_object (value));
		if (self->pv->context != NULL)
			g_object_add_weak_pointer (G_OBJECT (self->pv->context), (gpointer*)&self->pv->context);
		g_object_notify (G_OBJECT (self), "context");
		break;
	case PROP_SOURCE:
		bastile_object_set_source (self, BASTILE_SOURCE (g_value_get_object (value)));
		break;
	case PROP_PREFERRED:
		bastile_object_set_preferred (self, BASTILE_OBJECT (g_value_get_object (value)));
		break;
	case PROP_PARENT:
		bastile_object_set_parent (self, BASTILE_OBJECT (g_value_get_object (value)));
		break;
	case PROP_ID:
		quark = g_value_get_uint (value);
		if (quark != self->pv->id) {
			self->pv->id = quark;
			g_object_freeze_notify (obj);
			g_object_notify (obj, "id");
			recalculate_id (self);
			g_object_thaw_notify (obj);
		}
		break;
	case PROP_TAG:
		quark = g_value_get_uint (value);
		if (quark != self->pv->tag) {
			self->pv->tag = quark;
			self->pv->tag_explicit = TRUE;
			g_object_notify (obj, "tag");
		}
		break;
	case PROP_LABEL:
		if (set_string_storage (g_value_get_string (value), &self->pv->label)) {
			g_object_freeze_notify (obj);
			g_object_notify (obj, "label");
			recalculate_label (self);
			g_object_thaw_notify (obj);
		}
		break;
	case PROP_NICKNAME:
		if (set_string_storage (g_value_get_string (value), &self->pv->nickname)) {
			self->pv->nickname_explicit = TRUE;
			g_object_notify (obj, "nickname");
		}
		break;
	case PROP_MARKUP:
		if (set_string_storage (g_value_get_string (value), &self->pv->markup)) {
			self->pv->markup_explicit = TRUE;
			g_object_notify (obj, "markup");
		}
		break;
	case PROP_DESCRIPTION:
		if (set_string_storage (g_value_get_string (value), &self->pv->description)) {
			self->pv->description_explicit = TRUE;
			g_object_notify (obj, "description");
		}
		break;
	case PROP_ICON:
		if (set_string_storage (g_value_get_string (value), &self->pv->icon))
			g_object_notify (obj, "icon");
		break;
	case PROP_IDENTIFIER:
		if (set_string_storage (g_value_get_string (value), &self->pv->identifier)) {
			self->pv->identifier_explicit = TRUE;
			g_object_notify (obj, "identifier");
		}
		break;
	case PROP_LOCATION:
		loc = g_value_get_enum (value);
		if (loc != self->pv->location) {
			self->pv->location = loc;
			g_object_notify (obj, "location");
		}
		break;
	case PROP_USAGE:
		usage = g_value_get_enum (value);
		if (usage != self->pv->usage) {
			self->pv->usage = usage;
			g_object_freeze_notify (obj);
			g_object_notify (obj, "usage");
			recalculate_usage (self);
			g_object_thaw_notify (obj);
		}
		break;
	case PROP_FLAGS:
		flags = g_value_get_uint (value);
		if (BASTILE_OBJECT_GET_CLASS (obj)->delete)
			flags |= BASTILE_FLAG_DELETABLE;
		else 
			flags &= ~BASTILE_FLAG_DELETABLE;
		if (flags != self->pv->flags) {
			self->pv->flags = flags;
			g_object_notify (obj, "flags");
		}
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

/**
 * bastile_object_real_realize:
 * @self: The object
 *
 * To be overridden
 *
 */
static void 
bastile_object_real_realize (BastileObject *self)
{
	/* 
	 * We do nothing by default. It's up to the derived class
	 * to override this and realize themselves when called.
	 */
	
	self->pv->realized = TRUE;
}

/**
 * bastile_object_real_refresh:
 * @self: The object
 *
 * To be overridden
 *
 */
static void 
bastile_object_real_refresh (BastileObject *self)
{
	/* 
	 * We do nothing by default. It's up to the derived class
	 * to override this and refresh themselves when called.
	 */
}

/**
 * bastile_object_class_init:
 * @klass: the object class
 *
 * Initialises the object class
 *
 */
static void
bastile_object_class_init (BastileObjectClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    
	bastile_object_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (BastileObjectPrivate));

	gobject_class->dispose = bastile_object_dispose;
	gobject_class->finalize = bastile_object_finalize;
	gobject_class->set_property = bastile_object_set_property;
	gobject_class->get_property = bastile_object_get_property;
	
	klass->realize = bastile_object_real_realize;
	klass->refresh = bastile_object_real_refresh;
	
	g_object_class_install_property (gobject_class, PROP_SOURCE,
	           g_param_spec_object ("source", "Object Source", "Source the Object came from", 
	                                BASTILE_TYPE_SOURCE, G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_CONTEXT,
	           g_param_spec_object ("context", "Context for object", "Object that this context belongs to", 
	                                BASTILE_TYPE_CONTEXT, G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_PREFERRED,
	           g_param_spec_object ("preferred", "Preferred Object", "An object to prefer over this one", 
	                                BASTILE_TYPE_OBJECT, G_PARAM_READWRITE));
    
	g_object_class_install_property (gobject_class, PROP_PREFERRED,
	           g_param_spec_object ("parent", "Parent Object", "This object's parent in the tree.", 
	                                BASTILE_TYPE_OBJECT, G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class, PROP_ID,
	           g_param_spec_uint ("id", "Object ID", "This object's ID.", 
	                              0, G_MAXUINT, 0, G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class, PROP_TAG,
	           g_param_spec_uint ("tag", "Object Type Tag", "This object's type tag.", 
	                              0, G_MAXUINT, 0, G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class, PROP_LABEL,
	           g_param_spec_string ("label", "Object Display Label", "This object's displayable label.", 
	                                "", G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_NICKNAME,
	           g_param_spec_string ("nickname", "Object Short Name", "This object's short name.", 
	                                "", G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class, PROP_ICON,
	           g_param_spec_string ("icon", "Object Icon", "Stock ID for object.", 
	                                "", G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class, PROP_MARKUP,
	           g_param_spec_string ("markup", "Object Display Markup", "This object's displayable markup.", 
	                                "", G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class, PROP_DESCRIPTION,
	           g_param_spec_string ("description", "Object Description", "Description of the type of object", 
	                                "", G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class, PROP_IDENTIFIER,
	           g_param_spec_string ("identifier", "Object Identifier", "Displayable ID for the object.", 
	                                "", G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class, PROP_LOCATION,
	           g_param_spec_enum ("location", "Object Location", "Where the object is located.", 
	                              BASTILE_TYPE_LOCATION, BASTILE_LOCATION_INVALID, G_PARAM_READWRITE));
	
	g_object_class_install_property (gobject_class, PROP_USAGE,
	           g_param_spec_enum ("usage", "Object Usage", "How this object is used.", 
	                              BASTILE_TYPE_USAGE, BASTILE_USAGE_NONE, G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class, PROP_FLAGS,
	           g_param_spec_uint ("flags", "Object Flags", "This object's flags.", 
	                              0, G_MAXUINT, 0, G_PARAM_READWRITE));
}

/* -----------------------------------------------------------------------------
 * PUBLIC 
 */

/**
 * bastile_object_get_id:
 * @self: Object
 *
 * Returns: the id of the object @self
 */
GQuark
bastile_object_get_id (BastileObject *self)
{
	g_return_val_if_fail (BASTILE_IS_OBJECT (self), 0);
	return self->pv->id;
}

/**
 * bastile_object_get_tag:
 * @self: Object
 *
 * Returns: the tag of the object @self
 */
GQuark
bastile_object_get_tag (BastileObject *self)
{
	g_return_val_if_fail (BASTILE_IS_OBJECT (self), 0);
	if (!self->pv->tag)
		bastile_object_realize (self);
	return self->pv->tag;	
}

/**
 * bastile_object_get_source:
 * @self: Object
 *
 * Returns: the source of the object @self
 */
BastileSource*
bastile_object_get_source (BastileObject *self)
{
	g_return_val_if_fail (BASTILE_IS_OBJECT (self), NULL);
	return self->pv->source;	
}


/**
 * bastile_object_set_source:
 * @self: The object to set a new source for
 * @value: The source to set
 *
 * sets the source for the object
 */
void
bastile_object_set_source (BastileObject *self, BastileSource *value)
{
	g_return_if_fail (BASTILE_IS_OBJECT (self));
	
	if (value == self->pv->source)
		return;

	if (self->pv->source) 
		g_object_remove_weak_pointer (G_OBJECT (self->pv->source), (gpointer*)&self->pv->source);

	self->pv->source = value;
	if (self->pv->source != NULL)
		g_object_add_weak_pointer (G_OBJECT (self->pv->source), (gpointer*)&self->pv->source);

	g_object_notify (G_OBJECT (self), "source");
}

/**
 * bastile_object_get_context:
 * @self: Object
 *
 * Returns: the context of the object @self
 */
BastileContext*
bastile_object_get_context (BastileObject *self)
{
	g_return_val_if_fail (BASTILE_IS_OBJECT (self), NULL);
	return self->pv->context;
}

/**
 * bastile_object_get_preferred:
 * @self: Object
 *
 * Returns: the preferred of the object @self
 */
BastileObject*
bastile_object_get_preferred (BastileObject *self)
{
	g_return_val_if_fail (BASTILE_IS_OBJECT (self), NULL);
	return self->pv->preferred;
}

/**
 * bastile_object_set_preferred:
 * @self: the object to set the preferred object for
 * @value: the preferred object
 *
 *
 */
void
bastile_object_set_preferred (BastileObject *self, BastileObject *value)
{
	g_return_if_fail (BASTILE_IS_OBJECT (self));
	
	if (self->pv->preferred == value)
		return;

	if (self->pv->preferred)
		g_object_remove_weak_pointer (G_OBJECT (self->pv->preferred), (gpointer*)&self->pv->preferred);

	self->pv->preferred = value;
	if (self->pv->preferred != NULL)
		g_object_add_weak_pointer (G_OBJECT (self->pv->preferred), (gpointer*)&self->pv->preferred);

	g_object_notify (G_OBJECT (self), "preferred");
}

/**
 * bastile_object_get_parent:
 * @self: Object
 *
 * Returns: the parent of the object @self
 */
BastileObject*
bastile_object_get_parent (BastileObject *self)
{
	g_return_val_if_fail (BASTILE_IS_OBJECT (self), NULL);
	return self->pv->parent;
}

/**
 * bastile_object_set_parent:
 * @self: the child
 * @value: the parent
 *
 * register @value as the parent of @self:
 */
void
bastile_object_set_parent (BastileObject *self, BastileObject *value)
{
	g_return_if_fail (BASTILE_IS_OBJECT (self));
	g_return_if_fail (self->pv->parent != self);
	g_return_if_fail (value != self);
	
	if (value == self->pv->parent)
		return;
	
	/* Set the new parent/child relationship */
	if (self->pv->parent != NULL)
		unregister_child (self->pv->parent, self);

	if (value != NULL)
		register_child (value, self);
	
	g_assert (self->pv->parent == value);

	g_object_notify (G_OBJECT (self), "parent");
}

/**
 * bastile_object_get_children:
 * @self: Object
 *
 * Returns: the children of the object @self
 */
GList*
bastile_object_get_children (BastileObject *self)
{
	g_return_val_if_fail (BASTILE_IS_OBJECT (self), NULL);
	bastile_object_realize (self);
	return g_list_copy (self->pv->children);
}

/**
 * bastile_object_get_nth_child:
 * @self: Object
 * @index: the number of the child to return
 *
 * Returns: the child number @index
 */
BastileObject*
bastile_object_get_nth_child (BastileObject *self, guint index)
{
	g_return_val_if_fail (BASTILE_IS_OBJECT (self), NULL);
	return BASTILE_OBJECT (g_list_nth_data (self->pv->children, index));
}

/**
 * bastile_object_get_label:
 * @self: Object
 *
 * Returns: the label of the object @self
 */
const gchar*
bastile_object_get_label (BastileObject *self)
{
	g_return_val_if_fail (BASTILE_IS_OBJECT (self), NULL);
	bastile_object_realize (self);
	return self->pv->label;
}

/**
 * bastile_object_get_markup:
 * @self: Object
 *
 * Returns: the markup of the object @self
 */
const gchar*
bastile_object_get_markup (BastileObject *self)
{
	g_return_val_if_fail (BASTILE_IS_OBJECT (self), NULL);
	bastile_object_realize (self);
	return self->pv->markup;
}

/**
 * bastile_object_get_nickname:
 * @self: Object
 *
 * Returns: the nickname of the object @self
 */
const gchar*
bastile_object_get_nickname (BastileObject *self)
{
	g_return_val_if_fail (BASTILE_IS_OBJECT (self), NULL);
	bastile_object_realize (self);
	return self->pv->nickname;
}

/**
 * bastile_object_get_description:
 * @self: Object
 *
 * Returns: the description of the object @self
 */
const gchar*
bastile_object_get_description (BastileObject *self)
{
	g_return_val_if_fail (BASTILE_IS_OBJECT (self), NULL);
	bastile_object_realize (self);
	return self->pv->description;
}

/**
 * bastile_object_get_icon:
 * @self: Object
 *
 * Returns: the icon of the object @self
 */
const gchar*
bastile_object_get_icon (BastileObject *self)
{
	g_return_val_if_fail (BASTILE_IS_OBJECT (self), NULL);
	bastile_object_realize (self);
	return self->pv->icon;
}

/**
 * bastile_object_get_identifier:
 * @self: Object
 *
 * Returns: the identifier of the object @self
 */
const gchar*
bastile_object_get_identifier (BastileObject *self)
{
	g_return_val_if_fail (BASTILE_IS_OBJECT (self), NULL);
	bastile_object_realize (self);
	return self->pv->identifier;	
}

/**
 * bastile_object_get_location:
 * @self: Object
 *
 * Returns: the location of the object @self
 */
BastileLocation
bastile_object_get_location (BastileObject *self)
{
	g_return_val_if_fail (BASTILE_IS_OBJECT (self), BASTILE_LOCATION_INVALID);
	if (self->pv->location == BASTILE_LOCATION_INVALID)
		bastile_object_realize (self);
	return self->pv->location;
}

/**
 * bastile_object_get_usage:
 * @self: Object
 *
 * Returns: the usage of the object @self
 */
BastileUsage
bastile_object_get_usage (BastileObject *self)
{
	g_return_val_if_fail (BASTILE_IS_OBJECT (self), BASTILE_USAGE_NONE);
	if (self->pv->usage == BASTILE_USAGE_NONE)
		bastile_object_realize (self);
	return self->pv->usage;	
}

/**
 * bastile_object_get_flags:
 * @self: Object
 *
 * Returns: the flags of the object @self
 */
guint
bastile_object_get_flags (BastileObject *self)
{
	g_return_val_if_fail (BASTILE_IS_OBJECT (self), 0);
	bastile_object_realize (self);
	return self->pv->flags;	
}

/**
 * bastile_object_lookup_property:
 * @self: the object to look up the property
 * @field: the field to lookup
 * @value: the returned value
 *
 * Looks up the property @field in the object @self and returns it in @value
 *
 * Returns: TRUE if a property was found
 */
gboolean
bastile_object_lookup_property (BastileObject *self, const gchar *field, GValue *value)
{
	GParamSpec *spec;
	
	g_return_val_if_fail (BASTILE_IS_OBJECT (self), FALSE);
	g_return_val_if_fail (field, FALSE);
	g_return_val_if_fail (value, FALSE);
	
	spec = g_object_class_find_property (G_OBJECT_GET_CLASS (self), field);
	if (!spec) {
		/* Some name mapping to new style names */
		if (g_str_equal (field, "simple-name"))
			field = "nickname";
		else if (g_str_equal (field, "key-id"))
			field = "identifier";
		else if (g_str_equal (field, "display-name"))
			field = "label";
		else if (g_str_equal (field, "key-desc"))
			field = "description";
		else if (g_str_equal (field, "ktype"))
			field = "tag";
		else if (g_str_equal (field, "etype"))
			field = "usage";
		else if (g_str_equal (field, "display-id"))
			field = "identifier";
		else if (g_str_equal (field, "stock-id"))
			field = "icon";
		else if (g_str_equal (field, "raw-id"))
			field = "identifier";
		else 
			return FALSE;
	
		/* Try again */
		spec = g_object_class_find_property (G_OBJECT_GET_CLASS (self), field);
		if (!spec)
			return FALSE;
	}

	g_value_init (value, spec->value_type);
	g_object_get_property (G_OBJECT (self), field, value);
	return TRUE; 
}

/**
 * bastile_object_realize:
 * @self: the object to realize
 *
 *
 * Realizes an object. Calls the klass method
 */
void
bastile_object_realize (BastileObject *self)
{
	BastileObjectClass *klass;
	g_return_if_fail (BASTILE_IS_OBJECT (self));
	if (self->pv->realized)
		return;
	if (self->pv->realizing)
		return;
	klass = BASTILE_OBJECT_GET_CLASS (self);
	g_return_if_fail (klass->realize);
	self->pv->realizing = TRUE;
	(klass->realize) (self);
	self->pv->realizing = FALSE;
}

/**
 * bastile_object_refresh:
 * @self: object to refresh
 *
 * calls the class refresh function
 *
 */
void
bastile_object_refresh (BastileObject *self)
{
	BastileObjectClass *klass;
	g_return_if_fail (BASTILE_IS_OBJECT (self));
	klass = BASTILE_OBJECT_GET_CLASS (self);
	g_return_if_fail (klass->refresh);
	(klass->refresh) (self);
}

/**
 * bastile_object_delete:
 * @self: object to delete
 *
 * calls the class delete function
 *
 * Returns: NULL on error
 */
BastileOperation*
bastile_object_delete (BastileObject *self)
{
	BastileObjectClass *klass;
	g_return_val_if_fail (BASTILE_IS_OBJECT (self), NULL);
	klass = BASTILE_OBJECT_GET_CLASS (self);
	g_return_val_if_fail (klass->delete, NULL);
	return (klass->delete) (self);
}

/**
 * bastile_object_predicate_match:
 * @self: the object to test
 * @obj: The predicate to match
 *
 * matches a bastile object and a predicate
 *
 * Returns: FALSE if predicate does not match the #BastileObject, TRUE else
 */
gboolean 
bastile_object_predicate_match (BastileObjectPredicate *self, BastileObject* obj) 
{
	BastileObjectPrivate *pv;
	
	g_return_val_if_fail (BASTILE_IS_OBJECT (obj), FALSE);
	pv = obj->pv;
	
	bastile_object_realize (obj);
	
	/* Check all the fields */
	if (self->tag != 0 && self->tag != pv->tag)
		return FALSE;
	if (self->id != 0 && self->id != pv->id)
		return FALSE;
	if (self->type != 0 && self->type != G_OBJECT_TYPE (obj))
		return FALSE;
	if (self->location != 0 && self->location != pv->location)
		return FALSE;
	if (self->usage != 0 && self->usage != pv->usage) 
		return FALSE;
	if (self->flags != 0 && (self->flags & pv->flags) == 0) 
		return FALSE;
	if (self->nflags != 0 && (self->nflags & pv->flags) != 0)
		return FALSE;
	if (self->source != NULL && self->source != pv->source)
		return FALSE;

	/* And any custom stuff */
	if (self->custom != NULL && !self->custom (obj, self->custom_target)) 
		return FALSE;

	return TRUE;
}

/**
 * bastile_object_predicate_clear:
 * @self: The predicate to clean
 *
 * Clears a bastile predicate (#BastileObjectPredicate)
 */
void
bastile_object_predicate_clear (BastileObjectPredicate *self)
{
	memset (self, 0, sizeof (*self));
}
