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

#include "bastile-context.h"
#include "bastile-mateconf.h"
#include "bastile-keyserver-control.h"
#include "bastile-servers.h"
#include "bastile-util.h"

#define UPDATING    "updating"

enum {
    PROP_0,
    PROP_MATECONF_KEY,
    PROP_NONE_OPTION
};

enum {
    COL_TEXT,
    COL_INFO
};

enum {
    OPTION_NONE,
    OPTION_SEPARATOR,
    OPTION_KEYSERVER
};

/* Forward declaration */
static void populate_combo (BastileKeyserverControl *combo, gboolean mateconf);

static void    bastile_keyserver_control_class_init      (BastileKeyserverControlClass *klass);
static void    bastile_keyserver_control_init            (BastileKeyserverControl *skc);
static GObject *bastile_keyserver_control_constructor    (GType type, guint n_construct_properties,
                                                           GObjectConstructParam *construct_params);
static void    bastile_keyserver_control_finalize        (GObject *gobject);
static void    bastile_keyserver_control_set_property    (GObject *object, guint prop_id,
                                                           const GValue *value, GParamSpec *pspec);
static void    bastile_keyserver_control_get_property    (GObject *object, guint prop_id,
                                                           GValue *value, GParamSpec *pspec);

G_DEFINE_TYPE(BastileKeyserverControl, bastile_keyserver_control, GTK_TYPE_COMBO_BOX)

static void
bastile_keyserver_control_class_init (BastileKeyserverControlClass *klass)
{
    GObjectClass *gobject_class;
    
    gobject_class = G_OBJECT_CLASS (klass);
    
    gobject_class->constructor = bastile_keyserver_control_constructor;
    gobject_class->set_property = bastile_keyserver_control_set_property;
    gobject_class->get_property = bastile_keyserver_control_get_property;
    gobject_class->finalize = bastile_keyserver_control_finalize;
  
    g_object_class_install_property (gobject_class, PROP_NONE_OPTION,
            g_param_spec_string ("none-option", "No key option", "Puts in an option for 'no key server'",
                                  NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property (gobject_class, PROP_MATECONF_KEY,
            g_param_spec_string ("mateconf-key", "MateConf key", "MateConf key to read/write selection",
                                  NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    
}

static void
keyserver_changed (GtkComboBox *widget, BastileKeyserverControl *skc)
{
    /* If currently updating (see populate_combo) ignore */
    if (g_object_get_data (G_OBJECT (skc), UPDATING) != NULL)
        return;
    
    if (skc->mateconf_key) {
        gchar *t = bastile_keyserver_control_selected (skc);
        bastile_mateconf_set_string (skc->mateconf_key, t ? t : "");
        g_free (t);
    }
}

static void
mateconf_notify (MateConfClient *client, guint id, MateConfEntry *entry, gpointer data)
{
    BastileKeyserverControl *skc = BASTILE_KEYSERVER_CONTROL (data);   
    const gchar *key = mateconf_entry_get_key (entry);

    if (g_str_equal (KEYSERVER_KEY, key))
        populate_combo (skc, FALSE);
    else if (skc->mateconf_key && g_str_equal (skc->mateconf_key, key))
        populate_combo (skc, TRUE);
}

static gint
compare_func (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data)
{
    gchar *desc_a, *desc_b;
    gint info_a, info_b;
    gint retval;

    gtk_tree_model_get (model, a, COL_TEXT, &desc_a, COL_INFO, &info_a, -1);
    gtk_tree_model_get (model, b, COL_TEXT, &desc_b, COL_INFO, &info_b, -1);

    if (info_a != info_b)
        retval = info_a - info_b;
    else if (info_a == OPTION_KEYSERVER)
        retval = g_utf8_collate (desc_a, desc_b);
    else
        retval = 0;

    g_free (desc_a);
    g_free (desc_b);

    return retval;
}

static gboolean
separator_func (GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
    gint info;

    gtk_tree_model_get (model, iter, COL_INFO, &info, -1);
     
    return info == OPTION_SEPARATOR;
}

static void    
bastile_keyserver_control_init (BastileKeyserverControl *skc)
{
    GtkCellRenderer *renderer;

    renderer = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (skc), renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (skc), renderer,
                                    "text", COL_TEXT,
                                    NULL);
    gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (skc),
                                          (GtkTreeViewRowSeparatorFunc) separator_func,
                                          NULL, NULL);
}

static GObject *
bastile_keyserver_control_constructor (GType type, guint n_construct_properties,
                                        GObjectConstructParam *construct_params)
{
    GObject *object;
    BastileKeyserverControl *skc;

    object = G_OBJECT_CLASS (bastile_keyserver_control_parent_class)->constructor
                                (type, n_construct_properties, construct_params);
    skc = BASTILE_KEYSERVER_CONTROL (object);

    populate_combo (skc, TRUE);
    g_signal_connect (skc, "changed", G_CALLBACK (keyserver_changed), skc);
    skc->notify_id_list = bastile_mateconf_notify (KEYSERVER_KEY, mateconf_notify, skc);
    if (skc->mateconf_key)
        skc->notify_id = bastile_mateconf_notify (skc->mateconf_key, mateconf_notify, skc);

    return object;
}

static void
bastile_keyserver_control_set_property (GObject *object, guint prop_id,
                                         const GValue *value, GParamSpec *pspec)
{
    BastileKeyserverControl *control = BASTILE_KEYSERVER_CONTROL (object);
    
    switch (prop_id) {
    case PROP_MATECONF_KEY:
        control->mateconf_key = g_value_dup_string (value);
        break;
        
    case PROP_NONE_OPTION:
        control->none_option = g_value_dup_string (value);
        break;
        
    default:
        break;
    }
}

static void
bastile_keyserver_control_get_property (GObject *object, guint prop_id,
                                         GValue *value, GParamSpec *pspec)
{
    BastileKeyserverControl *control = BASTILE_KEYSERVER_CONTROL (object);
    
    switch (prop_id) {
        case PROP_NONE_OPTION:
            g_value_set_string (value, control->none_option);
            break;

        case PROP_MATECONF_KEY:
            g_value_set_string (value, control->mateconf_key);
            break;
        
        default:
            break;
    }
}

static void
bastile_keyserver_control_finalize (GObject *gobject)
{
    BastileKeyserverControl *skc = BASTILE_KEYSERVER_CONTROL (gobject);
    
    if (skc->notify_id >= 0) {
        bastile_mateconf_unnotify (skc->notify_id);
        skc->notify_id = 0;
    }

    if (skc->notify_id_list >= 0) {
        bastile_mateconf_unnotify (skc->notify_id_list);
        skc->notify_id_list = 0;
    }

    g_free (skc->mateconf_key);

    G_OBJECT_CLASS (bastile_keyserver_control_parent_class)->finalize (gobject);
}

static void
populate_combo (BastileKeyserverControl *skc, gboolean mateconf)
{
    GtkComboBox *combo = GTK_COMBO_BOX (skc);
    GSList *l, *ks;
    gchar *chosen = NULL;
    gint chosen_info = OPTION_KEYSERVER;
    GtkListStore *store;
    GtkTreeIter iter, none_iter, chosen_iter;
    gboolean chosen_iter_set = FALSE;

    /* Get the appropriate selection */
    if (mateconf && skc->mateconf_key)
        chosen = bastile_mateconf_get_string (skc->mateconf_key);
    else {
        if (gtk_combo_box_get_active_iter (combo, &iter)) {
            gtk_tree_model_get (gtk_combo_box_get_model (combo), &iter,
                                COL_TEXT, &chosen,
                                COL_INFO, &chosen_info,
                                -1);
        }
    }

    /* Mark this so we can ignore events */
    g_object_set_data (G_OBJECT (skc), UPDATING, GINT_TO_POINTER (1));
    
    /* Remove old model, and create new one */
    gtk_combo_box_set_model (combo, NULL);

    store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);

    /* The all key servers option */
    if (skc->none_option) {
        gtk_list_store_insert_with_values (store, &none_iter, 0,
                                           COL_TEXT, skc->none_option,
                                           COL_INFO, OPTION_NONE,
                                           -1);
        /* And add a separator row */
        gtk_list_store_insert_with_values (store, &iter, 0,
                                           COL_TEXT, NULL,
                                           COL_INFO, OPTION_SEPARATOR,
                                           -1);
    }

    ks = bastile_servers_get_uris ();

    for (l = ks; l != NULL; l = g_slist_next (l)) {
        const gchar *keyserver = (const gchar *) l->data;

        g_assert (keyserver != NULL);
        gtk_list_store_insert_with_values (store, &iter, 0,
                                           COL_TEXT, keyserver,
                                           COL_INFO, OPTION_KEYSERVER,
                                           -1);
        if (chosen && g_strcmp0 (chosen, keyserver) == 0) {
            chosen_iter = iter;
            chosen_iter_set = TRUE;
        }
    }
    bastile_util_string_slist_free (ks);
    g_free (chosen);

    /* Turn on sorting after populating the store, since that's faster */
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (store), COL_TEXT,
                                     (GtkTreeIterCompareFunc) compare_func,
                                     NULL, NULL);
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store), COL_TEXT,
                                          GTK_SORT_ASCENDING);

    gtk_combo_box_set_model (combo, GTK_TREE_MODEL (store));

    /* Re-set the selected value, if it's still present in the model */
    if (chosen_iter_set)
        gtk_combo_box_set_active_iter (combo, &chosen_iter);
    else if (skc->none_option)
        gtk_combo_box_set_active_iter (combo, &none_iter);

    /* Done updating */
    g_object_set_data (G_OBJECT (skc), UPDATING, NULL);    
}

BastileKeyserverControl*  
bastile_keyserver_control_new (const gchar *mateconf_key, const gchar *none_option)
{
    return g_object_new (BASTILE_TYPE_KEYSERVER_CONTROL, 
                         "mateconf-key", mateconf_key, "none-option", none_option, NULL);
}

gchar *
bastile_keyserver_control_selected (BastileKeyserverControl *skc)
{
    GtkComboBox *combo = GTK_COMBO_BOX (skc);
    GtkTreeIter iter;
    gint info;
    gchar *server;

    if (!gtk_combo_box_get_active_iter (combo, &iter))
        return NULL;

    gtk_tree_model_get (gtk_combo_box_get_model (combo), &iter,
                        COL_TEXT, &server,
                        COL_INFO, &info,
                        -1);

    if (info == OPTION_KEYSERVER)
        return server;
        
    return NULL;
}
