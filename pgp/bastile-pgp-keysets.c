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

#include "config.h"

#include "bastile-mateconf.h"
#include "bastile-object.h"

#include "bastile-pgp-module.h"
#include "bastile-pgp-keysets.h"

/* -----------------------------------------------------------------------------
 * COMMON KEYSETS 
 */

static void
pgp_signers_mateconf_notify (MateConfClient *client, guint id, MateConfEntry *entry, 
                          BastileSet *skset)
{
    /* Default key changed, refresh */
    bastile_set_refresh (skset);
}

static gboolean 
pgp_signers_match (BastileObject *obj, gpointer data)
{
    BastileObject *defkey;
    
    if (!BASTILE_IS_OBJECT (obj))
	    return FALSE;
    
    defkey = bastile_context_get_default_key (SCTX_APP ());
    
    /* Default key overrides all, and becomes the only signer available*/
    if (defkey && bastile_object_get_id (obj) != bastile_object_get_id (defkey))
        return FALSE;
    
    return TRUE;
}

BastileSet*     
bastile_keyset_pgp_signers_new ()
{
    BastileObjectPredicate *pred = g_new0 (BastileObjectPredicate, 1);
    BastileSet *skset;
    
    pred->location = BASTILE_LOCATION_LOCAL;
    pred->tag = BASTILE_PGP;
    pred->usage = BASTILE_USAGE_PRIVATE_KEY;
    pred->flags = BASTILE_FLAG_CAN_SIGN;
    pred->nflags = BASTILE_FLAG_EXPIRED | BASTILE_FLAG_REVOKED | BASTILE_FLAG_DISABLED;
    pred->custom = pgp_signers_match;
    
    skset = bastile_set_new_full (pred);
    g_object_set_data_full (G_OBJECT (skset), "pgp-signers-predicate", pred, g_free);
    
    bastile_mateconf_notify_lazy (BASTILE_DEFAULT_KEY, 
                                (MateConfClientNotifyFunc)pgp_signers_mateconf_notify, 
                                skset, skset);
    return skset;
}
