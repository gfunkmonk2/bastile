/*
 * Bastile
 *
 * Copyright (C) 2003 Jacob Perkins
 * Copyright (C) 2006 Stefan Walter
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

#include "bastile-combo-keys.h"
#include "bastile-mateconf.h"
#include "bastile-gtkstock.h"
#include "bastile-object-widget.h"
#include "bastile-set.h"
#include "bastile-util.h"

#include "pgp/bastile-gpgme-dialogs.h"
#include "pgp/bastile-gpgme-key-op.h"
#include "pgp/bastile-pgp-keysets.h"

#include <glib/gi18n.h>

G_MODULE_EXPORT gboolean
on_gpgme_sign_ok_clicked (BastileWidget *swidget, GtkWindow *parent)
{
    BastileSignCheck check;
    BastileSignOptions options = 0;
    BastileObject *signer;
    GtkWidget *w;
    gpgme_error_t err;
    BastileObject *to_sign;
    
    /* Figure out choice */
    check = SIGN_CHECK_NO_ANSWER;
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "sign-choice-not"));
    g_return_val_if_fail (w != NULL, FALSE);
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
        check = SIGN_CHECK_NONE;
    else {
        w = GTK_WIDGET (bastile_widget_get_widget (swidget, "sign-choice-casual"));
        g_return_val_if_fail (w != NULL, FALSE);
        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
            check = SIGN_CHECK_CASUAL;
        else {
            w = GTK_WIDGET (bastile_widget_get_widget (swidget, "sign-choice-careful"));
            g_return_val_if_fail (w != NULL, FALSE);
            if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
                check = SIGN_CHECK_CAREFUL;
        }
    }
    
    /* Local signature */
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "sign-option-local"));
    g_return_val_if_fail (w != NULL, FALSE);
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
        options |= SIGN_LOCAL;
    
    /* Revocable signature */
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "sign-option-revocable"));
    g_return_val_if_fail (w != NULL, FALSE);
    if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w))) 
        options |= SIGN_NO_REVOKE;
    
    /* Signer */
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "signer-select"));
    g_return_val_if_fail (w != NULL, FALSE);
    signer = bastile_combo_keys_get_active (GTK_COMBO_BOX (w));
    
    g_assert (!signer || (BASTILE_IS_GPGME_KEY (signer) && 
                          bastile_object_get_usage (BASTILE_OBJECT (signer)) == BASTILE_USAGE_PRIVATE_KEY));
    
    to_sign = g_object_get_data (G_OBJECT (swidget), "to-sign");
    if (BASTILE_IS_GPGME_UID (to_sign))
	    err = bastile_gpgme_key_op_sign_uid (BASTILE_GPGME_UID (to_sign), BASTILE_GPGME_KEY (signer), check, options);
    else if (BASTILE_IS_GPGME_KEY (to_sign))
	    err = bastile_gpgme_key_op_sign (BASTILE_GPGME_KEY (to_sign), BASTILE_GPGME_KEY (signer), check, options);
    else
	    g_assert (FALSE);
    

    if (!GPG_IS_OK (err)) {
        if (gpgme_err_code (err) == GPG_ERR_EALREADY) {
            w = gtk_message_dialog_new (parent, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
                                        _("This key was already signed by\n\"%s\""), bastile_object_get_label (signer));
            gtk_dialog_run (GTK_DIALOG (w));
            gtk_widget_destroy (w);
        } else
            bastile_gpgme_handle_error (err, _("Couldn't sign key"));
    }

    bastile_widget_destroy (swidget);
    
    return TRUE;
}

static void
keyset_changed (BastileSet *skset, GtkWidget *widget)
{
    if (bastile_set_get_count (skset) <= 1)
        gtk_widget_hide (widget);
    else
        gtk_widget_show (widget);
}

G_MODULE_EXPORT void
on_gpgme_sign_choice_toggled (GtkToggleButton *toggle, BastileWidget *swidget)
{
    GtkWidget *w;
    
    /* Figure out choice */
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "sign-choice-not"));
    g_return_if_fail (w != NULL);
    bastile_widget_set_visible (swidget, "sign-display-not", 
                             gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)));
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "sign-choice-casual"));
    g_return_if_fail (w != NULL);
    bastile_widget_set_visible (swidget, "sign-display-casual", 
                             gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)));
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "sign-choice-careful"));
    g_return_if_fail (w != NULL);
    bastile_widget_set_visible (swidget, "sign-display-careful", 
                             gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)));
}

static void
sign_internal (BastileObject *to_sign, GtkWindow *parent)
{
    BastileSet *skset;
    GtkWidget *w;
    gint response;
    BastileWidget *swidget;
    gboolean do_sign = TRUE;
    gchar *userid;

    /* Some initial checks */
    skset = bastile_keyset_pgp_signers_new ();
    
    /* If no signing keys then we can't sign */
    if (bastile_set_get_count (skset) == 0) {
        /* TODO: We should be giving an error message that allows them to 
           generate or import a key */
        bastile_util_show_error (NULL, _("No keys usable for signing"), 
                _("You have no personal PGP keys that can be used to indicate your trust of this key."));
        return;
    }

    swidget = bastile_widget_new ("sign", parent);
    g_return_if_fail (swidget != NULL);
    
    g_object_set_data_full (G_OBJECT (swidget), "to-sign", g_object_ref (to_sign), g_object_unref);

    /* ... Except for when calling this, which is messed up */
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "sign-uid-text"));
    g_return_if_fail (w != NULL);

    userid = g_markup_printf_escaped("<i>%s</i>", bastile_object_get_label (to_sign));
    gtk_label_set_markup (GTK_LABEL (w), userid);
    g_free (userid);
    
    /* Uncheck all selections */
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "sign-choice-not"));
    g_return_if_fail (w != NULL);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), FALSE);
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "sign-choice-casual"));
    g_return_if_fail (w != NULL);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), FALSE);
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "sign-choice-careful"));
    g_return_if_fail (w != NULL);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), FALSE);
    
    /* Initial choice */
    on_gpgme_sign_choice_toggled (NULL, swidget);
    
    /* Other question's default state */
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "sign-option-local"));
    g_return_if_fail (w != NULL);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), FALSE);
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "sign-option-revocable"));
    g_return_if_fail (w != NULL);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), TRUE);
    
    /* Signature area */
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "signer-frame"));
    g_return_if_fail (w != NULL);
    g_signal_connect (skset, "set-changed", G_CALLBACK (keyset_changed), w);
    keyset_changed (skset, w);

    /* Signer box */
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "signer-select"));
    g_return_if_fail (w != NULL);
    bastile_combo_keys_attach (GTK_COMBO_BOX (w), skset, NULL);

    /* Image */
    w = GTK_WIDGET (bastile_widget_get_widget (swidget, "sign-image"));
    g_return_if_fail (w != NULL);
    gtk_image_set_from_stock (GTK_IMAGE (w), BASTILE_STOCK_SIGN, GTK_ICON_SIZE_DIALOG);
    
    g_object_unref (skset);
    bastile_widget_show (swidget);
    
    while (do_sign) {
        response = gtk_dialog_run (GTK_DIALOG (bastile_widget_get_toplevel (swidget)));
        switch (response) {
        case GTK_RESPONSE_HELP:
            break;
        case GTK_RESPONSE_OK:
            do_sign = !on_gpgme_sign_ok_clicked (swidget, parent);
            break;
        default:
            do_sign = FALSE;
            bastile_widget_destroy (swidget);
            break;
        }
    }
}

void
bastile_gpgme_sign_prompt (BastileGpgmeKey *to_sign, GtkWindow *parent)
{
	sign_internal (BASTILE_OBJECT (to_sign), parent);
}

void
bastile_gpgme_sign_prompt_uid (BastileGpgmeUid *to_sign, GtkWindow *parent)
{
	sign_internal (BASTILE_OBJECT (to_sign), parent);
}
