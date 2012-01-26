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

#ifndef __BASTILE_UNKNOWN_H__
#define __BASTILE_UNKNOWN_H__

#include <gtk/gtk.h>

#include "bastile-object.h"
#include "bastile-unknown-source.h"

#define BASTILE_TYPE_UNKNOWN            (bastile_unknown_get_type ())
#define BASTILE_UNKNOWN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_UNKNOWN, BastileUnknown))
#define BASTILE_UNKNOWN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_UNKNOWN, BastileUnknownClass))
#define BASTILE_IS_UNKNOWN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_UNKNOWN))
#define BASTILE_IS_UNKNOWN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_UNKNOWN))
#define BASTILE_UNKNOWN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_UNKNOWN, BastileUnknownClass))

typedef struct _BastileUnknown BastileUnknown;
typedef struct _BastileUnknownClass BastileUnknownClass;

struct _BastileUnknown {
    BastileObject parent;

    /*< public >*/
    gchar *display;
};

struct _BastileUnknownClass {
    BastileObjectClass            parent_class;
};

GType                bastile_unknown_get_type         (void);

BastileUnknown*     bastile_unknown_new              (BastileUnknownSource *usrc,
                                                        GQuark id, 
                                                        const gchar *display);

#endif /* __BASTILE_UNKNOWN_H__ */
