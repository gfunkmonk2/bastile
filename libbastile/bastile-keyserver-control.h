/*
 * Bastile
 *
 * Copyright (C) 2005 Stefan Walter
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
 
#ifndef __BASTILE_KEYSERVER_CONTROL_H__
#define __BASTILE_KEYSERVER_CONTROL_H__

#include <gtk/gtk.h>

#define BASTILE_TYPE_KEYSERVER_CONTROL		(bastile_keyserver_control_get_type ())
#define BASTILE_KEYSERVER_CONTROL(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BASTILE_TYPE_KEYSERVER_CONTROL, BastileKeyserverControl))
#define BASTILE_KEYSERVER_CONTROL_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), BASTILE_TYPE_KEYSERVER_CONTROL, BastileKeyserverControlClass))
#define BASTILE_IS_KEYSERVER_CONTROL(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BASTILE_TYPE_KEYSERVER_CONTROL))
#define BASTILE_IS_KEYSERVER_CONTROL_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BASTILE_TYPE_KEYSERVER_CONTROL))
#define BASTILE_KEYSERVER_CONTROL_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), BASTILE_TYPE_KEYSERVER_CONTROL, BastileKeyserverControlClass))

typedef struct _BastileKeyserverControl BastileKeyserverControl;
typedef struct _BastileKeyserverControlClass BastileKeyserverControlClass;

/**
 * BastileKeyServerControl:
 * @parent_instance: The parent #GtkComboBox
 *
 * A control which allows you to select from a set
 * of keyservers.
 *
 * - Also displays shares for keys found via DNS-SD over the network.
 *
 * Properties:
 *   mateconf-key: (gchar*) The MateConf key to retrieve and set keyservers.
 *   none-option: (gchar*) Text to display for 'no key server'
 */

struct _BastileKeyserverControl {
    GtkComboBox parent_instance;
    
    /* <public> */
    gchar *mateconf_key;
    gchar *none_option;
    
    /* <private> */
    guint notify_id;
    guint notify_id_list;
    gboolean changed;
};

struct _BastileKeyserverControlClass {
	GtkComboBoxClass parent_class;
};

BastileKeyserverControl*   bastile_keyserver_control_new         (const gchar *mateconf_key,
                                                                    const gchar *none_option);

gchar*                      bastile_keyserver_control_selected    (BastileKeyserverControl *skc);

#endif /* __BASTILE_KEYSERVER_CONTROL_H__ */
