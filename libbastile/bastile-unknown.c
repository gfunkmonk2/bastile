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

#include "bastile-unknown.h"

#include "bastile-unknown-source.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (BastileUnknown, bastile_unknown, BASTILE_TYPE_OBJECT);

/* -----------------------------------------------------------------------------
 * OBJECT 
 */


static BastileOperation*           
bastile_unknown_delete (BastileObject *sobj)
{
    bastile_context_remove_object (SCTX_APP (), sobj);
    return bastile_operation_new_complete (NULL);
}

static void
bastile_unknown_init (BastileUnknown *self)
{

}

static void
bastile_unknown_class_init (BastileUnknownClass *klass)
{
	bastile_unknown_parent_class = g_type_class_peek_parent (klass);
	BASTILE_OBJECT_CLASS (klass)->delete = bastile_unknown_delete;
}

/* -----------------------------------------------------------------------------
 * PUBLIC METHODS
 */

BastileUnknown* 
bastile_unknown_new (BastileUnknownSource *source, GQuark id, const gchar *display)
{
	BastileUnknown *self;
    
	if (!display)
		display = _("Unavailable");
    
	self = g_object_new (BASTILE_TYPE_UNKNOWN, "source", source, 
	                     "label", display, "id", id, NULL);
	return self;
}
