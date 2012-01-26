/* 
 * Bastile
 * 
 * Copyright (C) 2005 Stefan Walter
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
 
#ifndef __CRYPTUI_PRIV_H__
#define __CRYPTUI_PRIV_H__

#include <mateconf/mateconf.h>
#include <mateconf/mateconf-client.h>

/* 
 * Used internally by libmatecryptui. 
 */

#ifndef LIBMATECRYPTUI_BUILD
#error "This header shouldn't be included outside of the libmatecryptui build."
#endif

#include <mateconf/mateconf.h>

#include "cryptui.h"
#include "cryptui-defines.h"

/* cryptui.c ---------------------------------------------------------------- */

gboolean        _cryptui_mateconf_get_boolean          (const char *key);

gchar*          _cryptui_mateconf_get_string           (const char *key);

void            _cryptui_mateconf_set_string           (const char *key, const char *value);

guint           _cryptui_mateconf_notify               (const char *key, 
                                                     MateConfClientNotifyFunc notification_callback,
                                                     gpointer callback_data);

void            _cryptui_mateconf_notify_lazy          (const char *key, 
                                                     MateConfClientNotifyFunc notification_callback,
                                                     gpointer callback_data, gpointer lifetime);
                                                     
void            _cryptui_mateconf_unnotify             (guint notification_id);

/* cryptui-keyset.c --------------------------------------------------------- */

const gchar*    _cryptui_keyset_get_internal_keyid  (CryptUIKeyset *ckset,
                                                     const gchar *keyid);

#endif /* __CRYPTUI_PRIV_H__ */
