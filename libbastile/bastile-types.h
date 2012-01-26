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

#ifndef __BASTILE_TYPES_H__
#define __BASTILE_TYPES_H__

#include <glib.h>
#include <glib-object.h>

#include "libmatecryptui/cryptui.h"

G_BEGIN_DECLS


#define BASTILE_TYPE_LOCATION (bastile_location_get_type ())

#define BASTILE_TYPE_USAGE (bastile_usage_get_type ())

/* 
 * These types should never change. These values are exported via DBUS. In the 
 * case of a key being in multiple locations, the highest location always 'wins'.
 */
typedef enum  {
	BASTILE_LOCATION_INVALID = 0,
	BASTILE_LOCATION_MISSING = 10,
	BASTILE_LOCATION_SEARCHING = 20,
	BASTILE_LOCATION_REMOTE = 50,
	BASTILE_LOCATION_LOCAL = 100
} BastileLocation;

GType bastile_location_get_type (void);

/* Again, never change these values */
typedef enum  {
	BASTILE_USAGE_NONE = 0,
	BASTILE_USAGE_SYMMETRIC_KEY = 1,
	BASTILE_USAGE_PUBLIC_KEY = 2,
	BASTILE_USAGE_PRIVATE_KEY = 3,
	BASTILE_USAGE_CREDENTIALS = 4,
	BASTILE_USAGE_IDENTITY = 5,
	BASTILE_USAGE_OTHER = 10
} BastileUsage;

GType bastile_usage_get_type (void);

typedef enum {
	BASTILE_FLAG_IS_VALID =    CRYPTUI_FLAG_IS_VALID,
	BASTILE_FLAG_CAN_ENCRYPT = CRYPTUI_FLAG_CAN_ENCRYPT,
	BASTILE_FLAG_CAN_SIGN =    CRYPTUI_FLAG_CAN_SIGN,
	BASTILE_FLAG_EXPIRED =     CRYPTUI_FLAG_EXPIRED,
	BASTILE_FLAG_REVOKED =     CRYPTUI_FLAG_REVOKED,
	BASTILE_FLAG_DISABLED =    CRYPTUI_FLAG_DISABLED,
	BASTILE_FLAG_TRUSTED =     CRYPTUI_FLAG_TRUSTED,
	BASTILE_FLAG_EXPORTABLE =  CRYPTUI_FLAG_EXPORTABLE,
	BASTILE_FLAG_DELETABLE = 0x10000000
} BastileKeyFlags;

#define BASTILE_TAG_INVALID               0

G_END_DECLS

#endif
