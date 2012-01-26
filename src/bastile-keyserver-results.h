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

#ifndef __BASTILE_KEYSERVER_RESULTS_H__
#define __BASTILE_KEYSERVER_RESULTS_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include <bastile-viewer.h>
#include <bastile-operation.h>
#include <bastile-object.h>

G_BEGIN_DECLS


#define BASTILE_TYPE_KEYSERVER_RESULTS              (bastile_keyserver_results_get_type ())
#define BASTILE_KEYSERVER_RESULTS(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_KEYSERVER_RESULTS, BastileKeyserverResults))
#define BASTILE_KEYSERVER_RESULTS_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_KEYSERVER_RESULTS, BastileKeyserverResultsClass))
#define BASTILE_IS_KEYSERVER_RESULTS(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_KEYSERVER_RESULTS))
#define BASTILE_IS_KEYSERVER_RESULTS_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_KEYSERVER_RESULTS))
#define BASTILE_KEYSERVER_RESULTS_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_KEYSERVER_RESULTS, BastileKeyserverResultsClass))

typedef struct _BastileKeyserverResults BastileKeyserverResults;
typedef struct _BastileKeyserverResultsClass BastileKeyserverResultsClass;
typedef struct _BastileKeyserverResultsPrivate BastileKeyserverResultsPrivate;

struct _BastileKeyserverResults {
	BastileViewer parent_instance;
	BastileKeyserverResultsPrivate *pv;
};

struct _BastileKeyserverResultsClass {
	BastileViewerClass parent_class;
};

GType            bastile_keyserver_results_get_type         (void);

void             bastile_keyserver_results_show             (BastileOperation* op, GtkWindow* parent, 
                                                              const char* search_text);

const gchar*     bastile_keyserver_results_get_search       (BastileKeyserverResults* self);


G_END_DECLS

#endif
