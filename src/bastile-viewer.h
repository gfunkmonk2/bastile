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

#ifndef __BASTILE_VIEWER_H__
#define __BASTILE_VIEWER_H__

#include <glib-object.h>

#include "bastile-object.h"
#include "bastile-set.h"
#include "bastile-view.h"
#include "bastile-widget.h"

#define BASTILE_TYPE_VIEWER               (bastile_viewer_get_type ())
#define BASTILE_VIEWER(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_VIEWER, BastileViewer))
#define BASTILE_VIEWER_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_VIEWER, BastileViewerClass))
#define BASTILE_IS_VIEWER(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_VIEWER))
#define BASTILE_IS_VIEWER_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_VIEWER))
#define BASTILE_VIEWER_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_VIEWER, BastileViewerClass))

typedef struct _BastileViewer BastileViewer;
typedef struct _BastileViewerClass BastileViewerClass;
typedef struct _BastileViewerPrivate BastileViewerPrivate;
    
struct _BastileViewer {
	BastileWidget parent;
};

struct _BastileViewerClass {
	BastileWidgetClass parent;
    
	/* virtual -------------------------------------------------------- */
    
	GList* (*get_selected_objects) (BastileViewer* self);
	
	void (*set_selected_objects) (BastileViewer* self, GList* objects);
	
	BastileObject* (*get_selected_object_and_uid) (BastileViewer* self, guint* uid);
	
	BastileObject* (*get_selected) (BastileViewer* self);
	
	void (*set_selected) (BastileViewer* self, BastileObject* value);
	
	BastileSet* (*get_current_set) (BastileViewer* self);
    
	/* signals --------------------------------------------------------- */
    
	void (*signal)   (BastileViewer *viewer);
};

GType               bastile_viewer_get_type                        (void);

void                bastile_viewer_ensure_updated                  (BastileViewer* self);

void                bastile_viewer_include_actions                 (BastileViewer* self, 
                                                                     GtkActionGroup* actions);

GList*              bastile_viewer_get_selected_objects            (BastileViewer* self);

void                bastile_viewer_set_selected_objects            (BastileViewer* self, 
                                                                     GList* objects);

BastileObject*     bastile_viewer_get_selected_object_and_uid     (BastileViewer* self, 
                                                                     guint* uid);

void                bastile_viewer_show_context_menu               (BastileViewer* self, 
                                                                     guint button, 
                                                                     guint time);

void                bastile_viewer_show_properties                 (BastileViewer* self, 
                                                                     BastileObject* obj);

void                bastile_viewer_set_status                      (BastileViewer* self, 
                                                                     const char* text);

void                bastile_viewer_set_numbered_status             (BastileViewer* self, 
                                                                     const char* text, 
                                                                     gint num);

BastileObject*     bastile_viewer_get_selected                    (BastileViewer* self);

void                bastile_viewer_set_selected                    (BastileViewer* self, 
                                                                     BastileObject* value);

BastileSet*        bastile_viewer_get_current_set                 (BastileViewer* self);

GtkWindow*          bastile_viewer_get_window                      (BastileViewer* self);

void                bastile_viewer_register_ui                     (BastileViewer *self, 
                                                                     BastileObjectPredicate *pred,
                                                                     const gchar *uidef, 
                                                                     GtkActionGroup *actions);

void                bastile_viewer_register_commands               (BastileViewer *self, 
                                                                     BastileObjectPredicate *pred,
                                                                     BastileCommands *commands);

#endif /* __BASTILE_VIEWER_H__ */
