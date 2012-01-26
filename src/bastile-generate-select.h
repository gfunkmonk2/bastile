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

#ifndef __BASTILE_GENERATE_SELECT_H__
#define __BASTILE_GENERATE_SELECT_H__

#include <glib.h>
#include <glib-object.h>

#include "bastile-widget.h"

#define BASTILE_TYPE_GENERATE_SELECT             (bastile_generate_select_get_type ())
#define BASTILE_GENERATE_SELECT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_GENERATE_SELECT, BastileGenerateSelect))
#define BASTILE_GENERATE_SELECT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_GENERATE_SELECT, BastileGenerateSelectClass))
#define BASTILE_IS_GENERATE_SELECT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_GENERATE_SELECT))
#define BASTILE_IS_GENERATE_SELECT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_GENERATE_SELECT))
#define BASTILE_GENERATE_SELECT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_GENERATE_SELECT, BastileGenerateSelectClass))

typedef struct _BastileGenerateSelect BastileGenerateSelect;
typedef struct _BastileGenerateSelectClass BastileGenerateSelectClass;
typedef struct _BastileGenerateSelectPrivate BastileGenerateSelectPrivate;

struct _BastileGenerateSelect {
	BastileWidget parent_instance;
	BastileGenerateSelectPrivate *pv;
};

struct _BastileGenerateSelectClass {
	BastileWidgetClass parent_class;
};

void bastile_generate_select_show (GtkWindow* parent);
GType bastile_generate_select_get_type (void);


G_END_DECLS

#endif
