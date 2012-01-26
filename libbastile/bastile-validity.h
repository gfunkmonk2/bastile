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

/**
 * For parsing and noting key validity, trust, etc...
 */
 
#ifndef __BASTILE_VALIDITY_H__
#define __BASTILE_VALIDITY_H__

#include <gtk/gtk.h>
#include "cryptui.h"

typedef enum {
    BASTILE_VALIDITY_REVOKED =     CRYPTUI_VALIDITY_REVOKED,
    BASTILE_VALIDITY_DISABLED =    CRYPTUI_VALIDITY_DISABLED,
    BASTILE_VALIDITY_NEVER =       CRYPTUI_VALIDITY_NEVER,
    BASTILE_VALIDITY_UNKNOWN =     CRYPTUI_VALIDITY_UNKNOWN,
    BASTILE_VALIDITY_MARGINAL =    CRYPTUI_VALIDITY_MARGINAL,
    BASTILE_VALIDITY_FULL =        CRYPTUI_VALIDITY_FULL,
    BASTILE_VALIDITY_ULTIMATE =    CRYPTUI_VALIDITY_ULTIMATE
} BastileValidity;

const gchar*        bastile_validity_get_string    (BastileValidity validity);

#endif /* __BASTILE_VALIDITY_H__ */
