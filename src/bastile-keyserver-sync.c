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

#include <glib/gi18n.h>

#include "bastile-context.h"
#include "bastile-mateconf.h"
#include "bastile-keyserver-sync.h"
#include "bastile-object.h"
#include "bastile-progress.h"
#include "bastile-preferences.h"
#include "bastile-servers.h"
#include "bastile-transfer-operation.h"
#include "bastile-util.h"
#include "bastile-widget.h"
#include "bastile-windows.h"

static void 
sync_import_complete (BastileOperation *op, BastileSource *sksrc)
{
    GError *err = NULL;
    
    if (!bastile_operation_is_successful (op)) {
        bastile_operation_copy_error (op, &err);
        bastile_util_handle_error (err, _("Couldn't publish keys to server"), 
                                    bastile_mateconf_get_string (PUBLISH_TO_KEY));
        g_clear_error (&err);
    }    
}

static void
sync_export_complete (BastileOperation *op, BastileSource *sksrc)
{
    GError *err = NULL;
    gchar *keyserver;

    if (!bastile_operation_is_successful (op)) {
        g_object_get (sksrc, "key-server", &keyserver, NULL);

        bastile_operation_copy_error (op, &err);
        bastile_util_handle_error (err, _("Couldn't retrieve keys from server: %s"), 
                                    keyserver);
        g_clear_error (&err);
        g_free (keyserver);
    }    
}

G_MODULE_EXPORT void
on_sync_ok_clicked (GtkButton *button, BastileWidget *swidget)
{
    GList *keys;
    
    keys = (GList*)g_object_get_data (G_OBJECT (swidget), "publish-keys");
    keys = g_list_copy (keys);
    
    bastile_widget_destroy (swidget);
   
    bastile_keyserver_sync (keys);
    g_list_free (keys);
}

G_MODULE_EXPORT void
on_sync_configure_clicked (GtkButton *button, BastileWidget *swidget)
{
    bastile_preferences_show (GTK_WINDOW (bastile_widget_get_widget (swidget, swidget->name)), "keyserver-tab");
}

static void
update_message (BastileWidget *swidget)
{
    GtkWidget *w, *w2, *sync_button;
    gchar *t;
    
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "publish-message"));
    w2 = GTK_WIDGET (bastile_widget_get_widget (swidget, "sync-message"));
    sync_button = GTK_WIDGET (bastile_widget_get_widget (swidget, "sync-button"));

    t = bastile_mateconf_get_string (PUBLISH_TO_KEY);
    if (t && t[0]) {
        gtk_widget_show (w);
        gtk_widget_hide (w2);
        
        gtk_widget_set_sensitive (sync_button, TRUE);
    } else {
        gtk_widget_hide (w);
        gtk_widget_show (w2);
        
        gtk_widget_set_sensitive (sync_button, FALSE);
    }
    g_free (t);
}

static void
mateconf_notify (MateConfClient *client, guint id, MateConfEntry *entry, gpointer data)
{
    BastileWidget *swidget;

    if (g_str_equal (PUBLISH_TO_KEY, mateconf_entry_get_key (entry))) {
        swidget = BASTILE_WIDGET (data);
        update_message (swidget);
    }
}

static void
unhook_notification (GtkWidget *widget, gpointer data)
{
    guint notify_id = GPOINTER_TO_INT (data);
    bastile_mateconf_unnotify (notify_id);
}

/**
 * bastile_keyserver_sync_show
 * @keys: The keys to synchronize
 * 
 * Shows a synchronize window.
 * 
 * Returns the new window.
 **/
GtkWindow*
bastile_keyserver_sync_show (GList *keys, GtkWindow *parent)
{
    BastileWidget *swidget;
    GtkWindow *win;
    GtkWidget *w;
    guint n, notify_id;
    gchar *t;
    
    swidget = bastile_widget_new_allow_multiple ("keyserver-sync", parent);
    g_return_val_if_fail (swidget != NULL, NULL);
    
    win = GTK_WINDOW (bastile_widget_get_widget (swidget, swidget->name));
    
    /* The details message */
    n = g_list_length (keys);
    t = g_strdup_printf (ngettext ("<b>%d key is selected for synchronizing</b>", 
                                   "<b>%d keys are selected for synchronizing</b>", n), n);
    
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "detail-message"));
    g_return_val_if_fail (swidget != NULL, win);
    gtk_label_set_markup (GTK_LABEL (w), t);
    g_free (t);
            
    /* The right help message */
    update_message (swidget);
    notify_id = bastile_mateconf_notify (PUBLISH_TO_KEY, mateconf_notify, swidget);
    g_signal_connect (win, "destroy", G_CALLBACK (unhook_notification), 
                      GINT_TO_POINTER (notify_id));

    keys = g_list_copy (keys);
    g_return_val_if_fail (!keys || BASTILE_IS_OBJECT (keys->data), win);
    g_object_set_data_full (G_OBJECT (swidget), "publish-keys", keys, 
                            (GDestroyNotify)g_list_free);
    
    return win;
}

void
bastile_keyserver_sync (GList *keys)
{
    BastileSource *sksrc;
    BastileSource *lsksrc;
    BastileMultiOperation *mop;
    BastileOperation *op;
    gchar *keyserver;
    GSList *ks, *l;
    GList *k;
    GSList *keyids = NULL;
    
    if (!keys)
        return;
    
    g_assert (BASTILE_IS_OBJECT (keys->data));
    
    /* Build a keyid list */
    for (k = keys; k; k = g_list_next (k)) 
        keyids = g_slist_prepend (keyids, 
                    GUINT_TO_POINTER (bastile_object_get_id (BASTILE_OBJECT (k->data))));

    mop = bastile_multi_operation_new ();

    /* And now synchronizing keys from the servers */
    ks = bastile_servers_get_uris ();
    
    for (l = ks; l; l = g_slist_next (l)) {
        
        sksrc = bastile_context_remote_source (SCTX_APP (), (const gchar*)(l->data));

        /* This can happen if the URI scheme is not supported */
        if (sksrc == NULL)
            continue;
        
        lsksrc = bastile_context_find_source (SCTX_APP (), 
                        bastile_source_get_tag (sksrc), BASTILE_LOCATION_LOCAL);
        
        if (lsksrc) {
            op = bastile_transfer_operation_new (_("Synchronizing keys"), sksrc, lsksrc, keyids);
            g_return_if_fail (op != NULL);

            bastile_multi_operation_take (mop, op);
            bastile_operation_watch (op, (BastileDoneFunc) sync_export_complete, sksrc, NULL, NULL);
        }
    }
    
    bastile_util_string_slist_free (ks);
    
    /* Publishing keys online */    
    keyserver = bastile_mateconf_get_string (PUBLISH_TO_KEY);
    if (keyserver && keyserver[0]) {
        
        sksrc = bastile_context_remote_source (SCTX_APP (), keyserver);

        /* This can happen if the URI scheme is not supported */
        if (sksrc != NULL) {

            op = bastile_context_transfer_objects (SCTX_APP (), keys, sksrc);
            g_return_if_fail (sksrc != NULL);

            bastile_multi_operation_take (mop, op);
            bastile_operation_watch (op, (BastileDoneFunc) sync_import_complete, sksrc, NULL, NULL);

        }
    }

    g_slist_free (keyids);
    g_free (keyserver);
    
    /* Show the progress window if necessary */
    bastile_progress_show (BASTILE_OPERATION (mop), _("Synchronizing keys..."), FALSE);
    g_object_unref (mop);
}
