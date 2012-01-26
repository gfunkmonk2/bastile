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

#ifndef __BASTILE_KEY_MANAGER_H__
#define __BASTILE_KEY_MANAGER_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "bastile-viewer.h"

G_BEGIN_DECLS


#define BASTILE_TYPE_KEY_MANAGER 	      (bastile_key_manager_get_type ())
#define BASTILE_KEY_MANAGER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_KEY_MANAGER, BastileKeyManager))
#define BASTILE_KEY_MANAGER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_KEY_MANAGER, BastileKeyManagerClass))
#define BASTILE_IS_KEY_MANAGER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_KEY_MANAGER))
#define BASTILE_IS_KEY_MANAGER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_KEY_MANAGER))
#define BASTILE_KEY_MANAGER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_KEY_MANAGER, BastileKeyManagerClass))

typedef struct _BastileKeyManager BastileKeyManager;
typedef struct _BastileKeyManagerClass BastileKeyManagerClass;
typedef struct _BastileKeyManagerPrivate BastileKeyManagerPrivate;

struct _BastileKeyManager {
	BastileViewer parent_instance;
	BastileKeyManagerPrivate *pv;
};

struct _BastileKeyManagerClass {
	BastileViewerClass parent_class;
};

/* 
 * The various tabs. If adding a tab be sure to fix 
 * logic throughout this file. 
 */

GtkWindow*       bastile_key_manager_show         (void);
GType            bastile_key_manager_get_type     (void);


G_END_DECLS

#endif
