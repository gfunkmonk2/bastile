/*
 * Bastile
 *
 * Copyright (C) 2004-2005 Stefan Walter
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

#include "bastile-context.h"
#include "bastile-dns-sd.h"
#include "bastile-mateconf.h"
#include "bastile-keyserver-results.h"
#include "bastile-preferences.h"
#include "bastile-servers.h"
#include "bastile-util.h"
#include "bastile-widget.h"

/**
 * SECTION:bastile-keyserver-search
 * @short_description: Contains the functions to start a search for keys on a
 * keyserver.
 **/

/**
 * KeyserverSelection:
 * @names: A list of keyserver names
 * @uris: A list of keyserver URIs
 * @all: TRUE if all keyservers are selected
 **/
typedef struct _KeyserverSelection {
    GSList *names;
    GSList *uris;
    gboolean all;
} KeyserverSelection;


/* Selection Retrieval ------------------------------------------------------ */

/**
 * widget: CHECK_BUTTON widget to read
 * selection: will be updated depending on the state of the widget
 *
 * Adds the name/uri of the checked widget to the selection
 *
 **/
static void
get_checks (GtkWidget *widget, KeyserverSelection *selection)
{
    if (GTK_IS_CHECK_BUTTON (widget)) {
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
            /* Pull in the selected name and uri */
            selection->names = g_slist_prepend (selection->names, 
                    g_strdup (gtk_button_get_label (GTK_BUTTON (widget))));
            selection->uris = g_slist_prepend (selection->uris, 
                    g_strdup (g_object_get_data (G_OBJECT (widget), "keyserver-uri")));
        } else {
            /* Note that not all checks are selected */
            selection->all = FALSE;
        }
    }
}

/**
 * swidget: the window/main widget
 *
 * extracts all keyservers in the sub-widgets "key-server-list" and
 * "shared-keys-list" and fills a KeyserverSelection structure.
 *
 * returns the selection
 **/
static KeyserverSelection*
get_keyserver_selection (BastileWidget *swidget)
{
    KeyserverSelection *selection;
    GtkWidget *w;
    
    selection = g_new0(KeyserverSelection, 1);
    selection->all = TRUE;
    
    /* Key servers */
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "key-server-list"));
    g_return_val_if_fail (w != NULL, selection);
    gtk_container_foreach (GTK_CONTAINER (w), (GtkCallback)get_checks, selection);

    /* Shared Key */
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "shared-keys-list"));
    g_return_val_if_fail (w != NULL, selection);
    gtk_container_foreach (GTK_CONTAINER (w), (GtkCallback)get_checks, selection);    
    
    return selection;
}

/**
 * selection: The selection to free
 *
 * All data (string lists, structures) are freed
 *
 **/
static void
free_keyserver_selection (KeyserverSelection *selection)
{
    if (selection) {
        bastile_util_string_slist_free (selection->uris);
        bastile_util_string_slist_free (selection->names);
        g_free (selection);
    }
}

/**
 * widget: a CHECK_BUTTON
 * checked: out- TRUE if the button is active, stays the same else.
 *
 **/
static void
have_checks (GtkWidget *widget, gboolean *checked)
{
    if (GTK_IS_CHECK_BUTTON (widget)) {
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
            *checked = TRUE;
    }
}

/**
 * swidget: sub widgets in here will be  checked
 *
 * returns TRUE if at least one of the CHECK_BUTTONS in "key-server-list" or
 * "shared-keys-list" is TRUE
 **/
static gboolean
have_keyserver_selection (BastileWidget *swidget)
{
    GtkWidget *w;
    gboolean checked = FALSE;
    
    /* Key servers */
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "key-server-list"));
    g_return_val_if_fail (w != NULL, FALSE);
    gtk_container_foreach (GTK_CONTAINER (w), (GtkCallback)have_checks, &checked);

    /* Shared keys */
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "shared-keys-list"));
    g_return_val_if_fail (w != NULL, FALSE);
    gtk_container_foreach (GTK_CONTAINER (w), (GtkCallback)have_checks, &checked);        
    
    return checked;
}

/**
 * on_keyserver_search_control_changed:
 * @widget: ignored
 * @swidget: main widget
 *
 *
 * Enables the "search" button if the edit-field contains text and at least a
 * server is selected
 */
G_MODULE_EXPORT void
on_keyserver_search_control_changed (GtkWidget *widget, BastileWidget *swidget)
{
    gboolean enabled = TRUE;
    GtkWidget *w;
    gchar *text;
    
    /* Need to have at least one key server selected ... */
    if (!have_keyserver_selection (swidget))
        enabled = FALSE;
    
    /* ... and some search text */
    else {
        w = GTK_WIDGET (bastile_widget_get_widget (swidget, "search-text"));
        text = gtk_editable_get_chars (GTK_EDITABLE (w), 0, -1);
        if (!text || !text[0])
            enabled = FALSE;
        g_free (text);
    }
        
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "search"));
    gtk_widget_set_sensitive (w, enabled);
}

/* Initial Selection -------------------------------------------------------- */

/**
 * widget: a toggle button
 * names: a list of names
 *
 * If the label of the toggle button is in the list of names it will be
 * set active
 *
 **/
static void
select_checks (GtkWidget *widget, GSList *names)
{
    if (GTK_IS_CHECK_BUTTON (widget)) {

        gchar *t = g_utf8_casefold (gtk_button_get_label (GTK_BUTTON (widget)), -1);
        gboolean checked = !names || 
                    g_slist_find_custom (names, t, (GCompareFunc)g_utf8_collate);
        g_free (t);
        
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), checked);
    }
}

/**
 * swidget: the main widget
 *
 * Reads key servers from mateconf and updates the UI content.
 *
 **/
static void
select_inital_keyservers (BastileWidget *swidget)
{
    GSList *l, *names;
    GtkWidget *w;
    
    names = bastile_mateconf_get_string_list (LASTSERVERS_KEY);

    /* Close the expander if all servers are selected */
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "search-where"));
    g_return_if_fail (w != NULL);
    gtk_expander_set_expanded (GTK_EXPANDER (w), names != NULL);
    
    /* We do case insensitive matches */    
    for (l = names; l; l = g_slist_next (l)) {
        gchar *t = g_utf8_casefold (l->data, -1);
        g_free (l->data);
        l->data = t;
    }
    
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "key-server-list"));
    g_return_if_fail (w != NULL);
    gtk_container_foreach (GTK_CONTAINER (w), (GtkCallback)select_checks, names);

    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "shared-keys-list"));
    g_return_if_fail (w != NULL);
    gtk_container_foreach (GTK_CONTAINER (w), (GtkCallback)select_checks, names);
}

/* Populating Lists --------------------------------------------------------- */

/**
 * widget: a check button
 * unchecked: a hash table containing the state of the servers
 *
 * If the button is not checked, the hash table entry associate with it will be
 * replaced with ""
 *
 **/
static void
remove_checks (GtkWidget *widget, GHashTable *unchecked)
{
    if (GTK_IS_CHECK_BUTTON (widget)) {
    
        if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) 
            g_hash_table_replace (unchecked, 
                g_strdup (gtk_button_get_label (GTK_BUTTON (widget))), "");
        
        gtk_widget_destroy (widget);
    }
}

/**
* swidget: the main widget
* box: the GTK_CONTAINER with the checkboxes
* uris: the uri list of the keyservers
* names: the keyserver names
*
* Updates the box and adds checkboxes containing names/uris. The check-status
* of already existing check boxes is not changed.
**/
static void
populate_keyserver_list (BastileWidget *swidget, GtkWidget *box, GSList *uris, 
                         GSList *names)
{
    GtkContainer *cont = GTK_CONTAINER (box);
    GHashTable *unchecked;
    gboolean any = FALSE;
    GtkWidget *check;
    GSList *l, *n;
    
    /* Remove all checks, and note which ones were unchecked */
    unchecked = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
    gtk_container_foreach (cont, (GtkCallback)remove_checks, unchecked);
    
    /* Now add the new ones back */
    for (l = uris, n = names; l && n; l = g_slist_next (l), n = g_slist_next (n)) {
        any = TRUE;

        /* A new checkbox with this the name as the label */        
        check = gtk_check_button_new_with_label ((const gchar*)n->data);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), 
                                      g_hash_table_lookup (unchecked, (gchar*)n->data) == NULL);
        g_signal_connect (check, "toggled", G_CALLBACK (on_keyserver_search_control_changed), swidget);
        gtk_widget_show (check);

        /* Save URI and set it as the tooltip */
        g_object_set_data_full (G_OBJECT (check), "keyserver-uri", g_strdup ((gchar*)l->data), g_free);
        gtk_widget_set_tooltip_text (check, (gchar*)l->data);
        
        gtk_container_add (cont, check);
    }

    g_hash_table_destroy (unchecked);   

    /* Only display the container if we had some checks */
    if (any)
        gtk_widget_show (box);
    else
        gtk_widget_hide (box);
}

/**
* client: ignored
* id: ignored
* entry: used only for initial test
* swidget: the main BastileWidget
*
* refreshes the "key-server-list". It reads the data from MateConf
**/
static void
refresh_keyservers (MateConfClient *client, guint id, MateConfEntry *entry, BastileWidget *swidget)
{
    GSList *keyservers, *names;
    GtkWidget *w;

	if (entry && !g_str_equal (KEYSERVER_KEY, mateconf_entry_get_key (entry)))
        return;

    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "key-server-list"));
    g_return_if_fail (w != NULL);

    keyservers = bastile_servers_get_uris ();
    names = bastile_servers_get_names ();
    populate_keyserver_list (swidget, w, keyservers, names);
    
    bastile_util_string_slist_free (keyservers);
    bastile_util_string_slist_free (names);        
}

/**
* ssd: the BastileServiceDiscovery. List-data is read from there
* name: ignored
* swidget: The BastileWidget
*
* refreshes the "shared-keys-list"
*
**/
static void 
refresh_shared_keys (BastileServiceDiscovery *ssd, const gchar *name, BastileWidget *swidget)
{
    GSList *keyservers, *names;
    GtkWidget *w;
    
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "shared-keys-list"));
    g_return_if_fail (w != NULL);

    names = bastile_service_discovery_list (ssd);
    keyservers = bastile_service_discovery_get_uris (ssd, names);
    populate_keyserver_list (swidget, w, keyservers, names);

    bastile_util_string_slist_free (keyservers);
    bastile_util_string_slist_free (names);    
}

/* -------------------------------------------------------------------------- */
 
/**
 * on_keyserver_search_ok_clicked:
 * @button: ignored
 * @swidget: The BastileWidget to work with
 *
 * Extracts data, stores it in MateConf and starts a search using the entered
 * search data.
 *
 * This function gets the things done
 */
G_MODULE_EXPORT void
on_keyserver_search_ok_clicked (GtkButton *button, BastileWidget *swidget)
{
    BastileOperation *op;
    KeyserverSelection *selection;
    const gchar *search;
	GtkWidget *w;
            
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "search-text"));
    g_return_if_fail (w != NULL);
    
    /* Get search text and save it for next time */
    search = gtk_entry_get_text (GTK_ENTRY (w));
    g_return_if_fail (search != NULL && search[0] != 0);
    bastile_mateconf_set_string (LASTSEARCH_KEY, search);
    
    /* The keyservers to search, and save for next time */    
    selection = get_keyserver_selection (swidget);
    g_return_if_fail (selection->uris != NULL);
    bastile_mateconf_set_string_list (LASTSERVERS_KEY, 
                                    selection->all ? NULL : selection->names);
                                    
    op = bastile_context_search_remote (SCTX_APP(), search);
    if (op == NULL)
        return;
    
    /* Open the new result window */    
    bastile_keyserver_results_show (op, 
                                     GTK_WINDOW (bastile_widget_get_widget (swidget, swidget->name)),
                                     search);

    free_keyserver_selection (selection);
    bastile_widget_destroy (swidget);
}

/**
* widget: ignored
* swidget: the BastileWidget to remove the signals from
*
* Disconnects the added/removed signals
*
**/
static void
cleanup_signals (GtkWidget *widget, BastileWidget *swidget)
{
    BastileServiceDiscovery *ssd = bastile_context_get_discovery (SCTX_APP());
    g_signal_handlers_disconnect_by_func (ssd, refresh_shared_keys, swidget);
}


/**
 * bastile_keyserver_search_show:
 * @parent: the parent window to connect this window to
 *
 * Shows a remote search window.
 *
 * Returns: the new window.
 */
GtkWindow*
bastile_keyserver_search_show (GtkWindow *parent)
{
    BastileServiceDiscovery *ssd;
	BastileWidget *swidget;
    GtkWindow *win;
    GtkWidget *w;
    gchar *search;
        
	swidget = bastile_widget_new ("keyserver-search", parent);
	g_return_val_if_fail (swidget != NULL, NULL);
 
    win = GTK_WINDOW (bastile_widget_get_widget (swidget, swidget->name));

    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "search-text"));
    g_return_val_if_fail (w != NULL, win);

    search = bastile_mateconf_get_string (LASTSEARCH_KEY);
    if (search != NULL) {
        gtk_entry_set_text (GTK_ENTRY (w), search);
        gtk_editable_select_region (GTK_EDITABLE (w), 0, -1);
        g_free (search);
    }
   
	/*CHECK: unknown key:
	glade_xml_signal_connect_data (swidget->xml, "configure_clicked",
		                           G_CALLBACK (configure_clicked), swidget);
	*/

    /* The key servers to list */
    refresh_keyservers (NULL, 0, NULL, swidget);
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, swidget->name));
    bastile_mateconf_notify_lazy (KEYSERVER_KEY, (MateConfClientNotifyFunc)refresh_keyservers, 
                                swidget, GTK_WIDGET (win));
    
    /* Any shared keys to list */    
    ssd = bastile_context_get_discovery (SCTX_APP ());
    refresh_shared_keys (ssd, NULL, swidget);
    g_signal_connect (ssd, "added", G_CALLBACK (refresh_shared_keys), swidget);
    g_signal_connect (ssd, "removed", G_CALLBACK (refresh_shared_keys), swidget);
    g_signal_connect (win, "destroy", G_CALLBACK (cleanup_signals), swidget);
    
    select_inital_keyservers (swidget);
    on_keyserver_search_control_changed (NULL, swidget);       
    
    return win;
}
