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
 
/** 
 * A collection of functions for accessing mateconf. Adapted from libeel. 
 */
  
#include <glib.h>
#include <mateconf/mateconf.h>
#include <mateconf/mateconf-client.h>

#include <gtk/gtk.h>
#include "cryptui-defines.h"

#define ENCRYPTSELF_KEY BASTILE_DESKTOP_KEYS "/encrypt_to_self"
#define MULTI_EXTENSION_KEY BASTILE_DESKTOP_KEYS "/package_extension"
#define MULTI_SEPERATE_KEY BASTILE_DESKTOP_KEYS "/multi_seperate"
#define KEYSERVER_KEY BASTILE_DESKTOP_KEYS "/keyservers/all_keyservers"
#define AUTORETRIEVE_KEY BASTILE_DESKTOP_KEYS "/keyservers/auto_retrieve"
#define AUTOSYNC_KEY BASTILE_DESKTOP_KEYS "/keyservers/auto_sync"
#define LASTSEARCH_KEY BASTILE_DESKTOP_KEYS "/keyservers/search_text"
#define LASTSERVERS_KEY BASTILE_DESKTOP_KEYS "/keyservers/search_keyservers"
#define PUBLISH_TO_KEY BASTILE_DESKTOP_KEYS "/keyservers/publish_to"

#define BASTILE_SCHEMAS            "/apps/bastile"

#define WINDOW_SIZE                BASTILE_SCHEMAS "/windows/"

void            bastile_mateconf_disconnect        ();

void            bastile_mateconf_set_boolean       (const char         *key, 
                                                  gboolean           boolean_value);

gboolean        bastile_mateconf_get_boolean       (const char         *key);

void            bastile_mateconf_set_integer       (const char         *key, 
                                                  int                int_value);

int             bastile_mateconf_get_integer       (const char         *key);

void            bastile_mateconf_set_string        (const char         *key, 
                                                  const char         *string_value);

char*           bastile_mateconf_get_string        (const char         *key);

void            bastile_mateconf_set_string_list   (const char         *key, 
                                                  const GSList       *slist);

GSList*         bastile_mateconf_get_string_list   (const char         *key);

MateConfEntry*     bastile_mateconf_get_entry         (const char         *key);

guint           bastile_mateconf_notify            (const char         *key, 
                                                  MateConfClientNotifyFunc notification_callback,
                                                  gpointer           callback_data);

void            bastile_mateconf_notify_lazy       (const char         *key,
                                                  MateConfClientNotifyFunc notification_callback,
                                                  gpointer           callback_data,
                                                  gpointer           lifetime);

void            bastile_mateconf_unnotify          (guint              notification_id);
