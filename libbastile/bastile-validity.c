/*
 * Bastile
 *
 * Copyright (C) 2003 Jacob Perkins
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

#include <glib/gi18n.h>

#include <string.h>

#include "bastile-validity.h"

/**
 * bastile_validity_get_string:
 * @validity: The validity ID
 *
 * Returns: A string describing the validity. Must not be freed
 */
const gchar*
bastile_validity_get_string (BastileValidity validity)
{
	switch (validity) {
		case BASTILE_VALIDITY_UNKNOWN:
			return _("Unknown");
		case BASTILE_VALIDITY_NEVER:
			return C_("Validity", "Never");
		case BASTILE_VALIDITY_MARGINAL:
			return _("Marginal");
		case BASTILE_VALIDITY_FULL:
			return _("Full");
		case BASTILE_VALIDITY_ULTIMATE:
			return _("Ultimate");
		case BASTILE_VALIDITY_DISABLED:
			return _("Disabled");
		case BASTILE_VALIDITY_REVOKED:
			return _("Revoked");
		default:
			return NULL;
	}
}
