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

#include "bastile-cleanup.h"

typedef struct _BastileCleanup {
	GDestroyNotify notify;
	gpointer user_data;
} BastileCleanup; 

static GSList *registered_cleanups = NULL;

void    
bastile_cleanup_register (GDestroyNotify notify, gpointer user_data)
{
	BastileCleanup *cleanup = g_new0 (BastileCleanup, 1);
	
	g_assert (notify);
	cleanup->notify = notify;
	cleanup->user_data = user_data;
	
	/* Note we're reversing the order, so calls happen that way */
	registered_cleanups = g_slist_prepend (registered_cleanups, cleanup);
}

void
bastile_cleanup_unregister (GDestroyNotify notify, gpointer user_data)
{
	BastileCleanup *cleanup;
	GSList *l;
	
	for (l = registered_cleanups; l; l = g_slist_next (l)) {
		cleanup = (BastileCleanup*)l->data;
		if (cleanup->notify == notify && cleanup->user_data == user_data) {
			registered_cleanups = g_slist_remove (registered_cleanups, cleanup);
			break;
		}
	}
}


void    
bastile_cleanup_perform (void)
{
	GSList *cleanups, *l;
	BastileCleanup *cleanup;
	
	while (registered_cleanups) {
		
		/* 
		 * While performing cleanups, more cleanups may be registered.
		 * So swap out the list, and keep going until empty.
		 */
	 
		cleanups = registered_cleanups;
		registered_cleanups = NULL;
		
		for (l = cleanups; l; l = g_slist_next (l)) {
			cleanup = (BastileCleanup*)l->data;
			g_assert (cleanup->notify);
			
			(cleanup->notify) (cleanup->user_data);
			g_free (cleanup);
		}
		
		g_slist_free (cleanups);
	}
}
