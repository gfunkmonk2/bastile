/*
 * Bastile
 *
 * Copyright (C) 2003 Jacob Perkins
 * Copyright (C) 2004,2005 Stefan Walter
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

#include "config.h"

#include "bastile-context.h"
#include "bastile-util.h"
#include "bastile-libdialogs.h"
#include "bastile-gtkstock.h"
#include "bastile-secure-memory.h"

#include "bastile-key-manager.h"

#include "common/bastile-cleanup.h"
#include "common/bastile-registry.h"

#ifdef WITH_PGP
#include "pgp/bastile-pgp-module.h"
#endif

#ifdef WITH_SSH
#include "ssh/bastile-ssh-module.h"
#endif

#ifdef WITH_PKCS11
#include "pkcs11/bastile-pkcs11-module.h"
#endif

#include "gkr/bastile-gkr-module.h"

#include <locale.h>
#include <stdlib.h>
  
#include <glib/gi18n.h>

static gboolean show_version = FALSE;

/* Initializes context and preferences, then loads key manager */
int
main (int argc, char **argv)
{
    static GOptionEntry options[] = {
        { "version", 'v', 0, G_OPTION_ARG_NONE, &show_version, N_("Version of this application"), NULL },
        { NULL, 0, 0, 0, NULL, NULL, NULL }
    };
    GError *error = NULL;
    int ret = 0;

    g_thread_init (NULL);

    bastile_secure_memory_init ();
    
#ifdef ENABLE_NLS
    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
#endif

    if (!gtk_init_with_args (&argc, &argv, _("Encryption Key Manager"), options, GETTEXT_PACKAGE, &error)) {
        g_printerr ("bastile: %s\n", error->message);
        g_error_free (error);
        exit (1);
    }

    if (show_version) {
        g_print ("%s\n", PACKAGE_STRING);
        g_print ("GNUPG: %s (%d.%d.%d)\n", GNUPG, GPG_MAJOR, GPG_MINOR, GPG_MICRO);
        exit (1);
    }

    /* Insert Icons into Stock */ 
    bastile_gtkstock_init ();
    
    /* Make the default BastileContext */
    bastile_context_new (BASTILE_CONTEXT_APP);

    /* Initialize the various components */
#ifdef WITH_PGP
    bastile_pgp_module_init ();
#endif
#ifdef WITH_SSH
    bastile_ssh_module_init ();
#endif
#ifdef WITH_PKCS11
    bastile_pkcs11_module_init ();
#endif
    bastile_gkr_module_init ();

    bastile_key_manager_show ();

    /* Start the refreshing of the keys */
    bastile_context_refresh_auto (NULL);

    g_signal_connect_after (SCTX_APP (), "destroy", gtk_main_quit, NULL);
    
    gtk_main ();


    bastile_cleanup_perform ();
    return ret;
}
