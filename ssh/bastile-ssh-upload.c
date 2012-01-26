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

#include <config.h>

#include <string.h>

#include <glib/gi18n.h>

#include "bastile-widget.h"
#include "bastile-util.h"
#include "bastile-operation.h"
#include "bastile-progress.h"
#include "bastile-gtkstock.h"

#include "ssh/bastile-ssh-source.h"
#include "ssh/bastile-ssh-key.h"
#include "ssh/bastile-ssh-operation.h"

static void 
upload_complete (BastileOperation *op, gpointer dummy)
{
    GError *err = NULL;
    
    if (!bastile_operation_is_successful (op)) {
        bastile_operation_copy_error (op, &err);
        bastile_util_handle_error (err, _("Couldn't configure Secure Shell keys on remote computer."));
        g_clear_error (&err);
    }    
}

G_MODULE_EXPORT void
on_upload_input_changed (GtkWidget *dummy, BastileWidget *swidget)
{
    GtkWidget *widget;
    const gchar *user, *host, *port;
    gchar *t = NULL;

    widget = GTK_WIDGET (bastile_widget_get_widget (swidget, "user-label"));
    user = gtk_entry_get_text (GTK_ENTRY (widget));
    g_return_if_fail (user && g_utf8_validate (user, -1, NULL));

    widget = GTK_WIDGET (bastile_widget_get_widget (swidget, "host-label"));
    host = gtk_entry_get_text (GTK_ENTRY (widget));
    g_return_if_fail (host && g_utf8_validate (host, -1, NULL));
    
    /* Take off port if necessary */
    port = strchr (host, ':');
    if (port) {
        
        /* Copy hostname out */
        g_assert (port >= host);
        host = t = g_strndup (host, port - host);
    }

    widget = GTK_WIDGET (bastile_widget_get_widget (swidget, "ok"));
    gtk_widget_set_sensitive (widget, host[0] && !bastile_util_string_is_whitespace (host) && 
                                      user[0] && !bastile_util_string_is_whitespace (user));
    
    /* Possibly allocated host */
    g_free (t);
}


static BastileOperation*
upload_via_source (const gchar *user, const gchar *host, const gchar *port, GList *keys)
{
    BastileMultiOperation *mop = NULL;
    BastileOperation *op = NULL;
    BastileSource *sksrc;
    BastileObject *object;
    GList *next;
    
    bastile_util_objects_sort (keys);
    g_assert (keys);
    
    while (keys) {
     
        /* Break off one set (same keysource) */
        next = bastile_util_objects_splice (keys);
        
        g_assert (BASTILE_IS_OBJECT (keys->data));
        object = BASTILE_OBJECT (keys->data);

        /* Upload via this key source */        
        sksrc = bastile_object_get_source (object);
        g_return_val_if_fail (sksrc != NULL, NULL);
        
        g_return_val_if_fail (BASTILE_IS_SSH_SOURCE (sksrc), NULL);

        /* If more than one operation start combining */
        if (op != NULL) {
            g_assert (mop == NULL);
            mop = bastile_multi_operation_new ();
            bastile_multi_operation_take (mop, op);
        }

        /* Start an upload process */
        op = bastile_ssh_operation_upload (BASTILE_SSH_SOURCE (sksrc), keys, user, host, port);
        g_return_val_if_fail (op != NULL, NULL);

        /* And combine if necessary */
        if (mop != NULL) {
            bastile_multi_operation_take (mop, op);
            op = NULL;
        }
        
        g_list_free (keys);
        keys = next;
    }
    
    return mop ? BASTILE_OPERATION (mop) : op;
}

static void
upload_keys (BastileWidget *swidget)
{
    BastileOperation *op;
    GtkWidget *widget;
    const gchar *cuser, *chost;
    gchar *user, *host, *port;
    GList *keys;

    keys = (GList*)g_object_steal_data (G_OBJECT (swidget), "upload-keys");
    g_return_if_fail (keys != NULL);

    widget = GTK_WIDGET (bastile_widget_get_widget (swidget, "user-label"));
    cuser = gtk_entry_get_text (GTK_ENTRY (widget));
    g_return_if_fail (cuser && g_utf8_validate (cuser, -1, NULL));
    
    widget = GTK_WIDGET (bastile_widget_get_widget (swidget, "host-label"));
    chost = (gchar*)gtk_entry_get_text (GTK_ENTRY (widget));
    g_return_if_fail (chost && g_utf8_validate (chost, -1, NULL));
    
    user = g_strdup (cuser);
    host = g_strdup (chost);

    /* Port is anything past a colon */
    port = strchr (host, ':');
    if (port) {
        *port = 0;
        port++;
        
        /* Trim and check */
        bastile_util_string_trim_whitespace (port);
        if (!port[0])
            port = NULL;
    }

    bastile_util_string_trim_whitespace (host);
    bastile_util_string_trim_whitespace (user);
    
    /* This frees |keys| */
    op = upload_via_source (user, host, port, keys);

    g_free (host);
    g_free (user);

    g_return_if_fail (op != NULL);
    bastile_operation_watch (op, (BastileDoneFunc)upload_complete, NULL, NULL, NULL);
    
    /* Show the progress window if necessary */
    bastile_progress_show (op, _("Configuring Secure Shell Keys..."), FALSE);
    g_object_unref (op);
}

/**
 * bastile_upload_show
 * @keys: Upload a certain set of SSH keys
 * 
 * Prompt a dialog to upload keys.
 **/
void
bastile_ssh_upload_prompt (GList *keys, GtkWindow *parent)
{
    BastileWidget *swidget;
    GtkWindow *win;
    GtkWidget *w;
    
    g_return_if_fail (keys != NULL);
    
    swidget = bastile_widget_new_allow_multiple ("ssh-upload", parent);
    g_return_if_fail (swidget != NULL);
    
    win = GTK_WINDOW (GTK_WIDGET (bastile_widget_get_widget (swidget, swidget->name)));

    /* Default to the users current name */
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "user-label"));
    gtk_entry_set_text (GTK_ENTRY (w), g_get_user_name ());
 
    /* Focus the host */
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "host-label"));
    gtk_widget_grab_focus (w);    

    keys = g_list_copy (keys);
    g_object_set_data_full (G_OBJECT (swidget), "upload-keys", keys, 
                            (GDestroyNotify)g_list_free);

    on_upload_input_changed (NULL, swidget);

    for (;;) {
        switch (gtk_dialog_run (GTK_DIALOG (win))) {
        case GTK_RESPONSE_HELP:
            /* TODO: Help */
            continue;
        case GTK_RESPONSE_ACCEPT:
            upload_keys (swidget);
            break;
        default:
            break;
        };
        
        break;
    }
    
    bastile_widget_destroy (swidget);
}
