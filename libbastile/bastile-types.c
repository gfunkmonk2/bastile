/*
 * Bastile
 *
 * Copyright (C) 2008 Stefan Walter
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

#include "bastile-types.h"

GType bastile_location_get_type (void) 
{
	static GType bastile_location_type_id = 0;
	if (!bastile_location_type_id) {
		static const GEnumValue values[] = {
			{ BASTILE_LOCATION_INVALID, "BASTILE_LOCATION_INVALID", "invalid" }, 
			{ BASTILE_LOCATION_MISSING, "BASTILE_LOCATION_MISSING", "missing" }, 
			{ BASTILE_LOCATION_SEARCHING, "BASTILE_LOCATION_SEARCHING", "searching" }, 
			{ BASTILE_LOCATION_REMOTE, "BASTILE_LOCATION_REMOTE", "remote" }, 
			{ BASTILE_LOCATION_LOCAL, "BASTILE_LOCATION_LOCAL", "local" }, 
			{ 0, NULL, NULL }
		};
		bastile_location_type_id = g_enum_register_static ("BastileLocation", values);
	}
	return bastile_location_type_id;
}



GType bastile_usage_get_type (void) 
{
	static GType bastile_usage_type_id = 0;
	if (!bastile_usage_type_id) {
		static const GEnumValue values[] = {
			{ BASTILE_USAGE_NONE, "BASTILE_USAGE_NONE", "none" },
			{ BASTILE_USAGE_SYMMETRIC_KEY, "BASTILE_USAGE_SYMMETRIC_KEY", "symmetric-key" }, 
			{ BASTILE_USAGE_PUBLIC_KEY, "BASTILE_USAGE_PUBLIC_KEY", "public-key" }, 
			{ BASTILE_USAGE_PRIVATE_KEY, "BASTILE_USAGE_PRIVATE_KEY", "private-key" }, 
			{ BASTILE_USAGE_CREDENTIALS, "BASTILE_USAGE_CREDENTIALS", "credentials" }, 
			{ BASTILE_USAGE_IDENTITY, "BASTILE_USAGE_IDENTITY", "identity" }, 
			{ BASTILE_USAGE_OTHER, "BASTILE_USAGE_OTHER", "other" }, 
			{ 0, NULL, NULL }
		};
		bastile_usage_type_id = g_enum_register_static ("BastileUsage", values);
	}
	return bastile_usage_type_id;
}
