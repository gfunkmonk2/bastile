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
 
#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

static FILE* bastile_link = NULL;
 
static gchar* 
askpass_command (const gchar *cmd, const gchar *arg)
{
    const gchar* env;
    gchar *t, *ret;
    int fd;
        
    /* Try an open the connection with bastile */
    if (!bastile_link) {
        env = g_getenv ("BASTILE_SSH_ASKPASS_FD");
        if (env == NULL)
            return NULL;
        g_unsetenv ("BASTILE_SSH_ASKPASS_FD");
        
        fd = strtol (env, &t, 10);
        if (*t) {
            g_warning ("fd received from bastile was not valid: %s", env);
            return NULL;
        }
        
        bastile_link = fdopen (fd, "r+b");
        if (!bastile_link) {
            g_warning ("couldn't open fd %d: %s", fd, strerror (errno));
            return NULL;
        }
        
        setvbuf(bastile_link, 0, _IONBF, 0);
    }
    
    /* Request a setting be sent */
    fprintf (bastile_link, "%s %s\n", cmd, arg ? arg : "");
    fflush (bastile_link);
    
    /* Read the setting */
    t = g_new0 (gchar, 512);
    ret = fgets (t, 512, bastile_link);
    
    /* Make sure it worked */
    if (ferror (bastile_link)) {
        g_warning ("error reading from bastile");
        fclose (bastile_link);
        bastile_link = NULL;
        g_free (t);
        return NULL;
    }
    
    return t;
}

int main (int argc, char* argv[])
{
    gchar *pass, *message, *p;

    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    /* Non buffered stdout */
    setvbuf(stdout, 0, _IONBF, 0);

    if (argc > 1)
        message = g_strjoinv (" ", argv + 1);
    else 
        message = g_strdup (_("Enter your Secure Shell passphrase:"));

    /* Check if we're being handed a password from bastile */
    pass = askpass_command ("PASSWORD", message);
    g_free (message);
    
    if (pass == NULL)
        return 1;
    
    if (write (1, pass, strlen (pass)) != strlen (pass))
	    g_warning ("couldn't write out password properly");
    for (p = pass; *p; p++) 
        *p = 0;
    g_free (pass);

    if (bastile_link)
        fclose (bastile_link);
    
    return 0;
}
