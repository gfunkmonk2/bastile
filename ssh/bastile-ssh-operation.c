/*
 * Bastile
 *
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

#include <sys/wait.h>
#include <signal.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <glib/gi18n.h>

#include "bastile-ssh-operation.h"
#include "bastile-util.h"
#include "bastile-passphrase.h"

#include <mate-keyring.h>

#ifndef DEBUG_OPERATION_ENABLE
#if _DEBUG
#define DEBUG_OPERATION_ENABLE 1
#else
#define DEBUG_OPERATION_ENABLE 0
#endif
#endif

#if DEBUG_OPERATION_ENABLE
#define DEBUG_OPERATION(x)  g_printerr x
#else
#define DEBUG_OPERATION(x)
#endif

/* -----------------------------------------------------------------------------
 * DEFINITIONS
 */
 
typedef void (*ResultCallback) (BastileSSHOperation *sop);
typedef const gchar* (*PasswordCallback) (BastileSSHOperation *sop, const gchar* msg);

typedef struct _BastileSSHOperationPrivate {
    
    /* Data written to SSH */
    GString *sin;
    guint win;
    GIOChannel *iin;
    
    /* Data being read from SSH */
    GString *sout;
    guint wout;
    GIOChannel *iout;
    
    /* Data from SSH error */
    GString *serr;
    guint werr;
    GIOChannel *ierr;
    
    /* Process Information */
    GPid pid;
    guint wpid;
    
    /* Callback when ready to parse result */
    ResultCallback result_cb;
    
    /* Callback for password prompting */
    PasswordCallback password_cb;
    
    /* Prompt information */
    BastileObject *prompt_skey;
    GtkDialog *prompt_dialog;
    guint prompt_requests;
    
    /* bastile-ssh-askpass communication */
    GIOChannel *io_askpass;
    guint stag_askpass;
    int fds_askpass[2];

} BastileSSHOperationPrivate;

#define COMMAND_PASSWORD "PASSWORD "
#define COMMAND_PASSWORD_LEN   9

enum {
    PROP_0,
    PROP_KEY_SOURCE
};

#define BASTILE_SSH_OPERATION_GET_PRIVATE(obj)  \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), BASTILE_TYPE_SSH_OPERATION, BastileSSHOperationPrivate))

/* TODO: This is just nasty. Gotta get rid of these weird macros */
IMPLEMENT_OPERATION_PROPS(SSH, ssh)

    g_object_class_install_property (gobject_class, PROP_KEY_SOURCE,
        g_param_spec_object ("source", "SSH Key Source", "Key source this operation works on.", 
                             BASTILE_TYPE_SSH_SOURCE, G_PARAM_READABLE));

    g_type_class_add_private (gobject_class, sizeof (BastileSSHOperationPrivate));

END_IMPLEMENT_OPERATION_PROPS

/* -----------------------------------------------------------------------------
 * HELPERS
 */

static void 
watch_ssh_process (GPid pid, gint status, BastileSSHOperation *sop)
{
    BastileSSHOperationPrivate *pv = BASTILE_SSH_OPERATION_GET_PRIVATE (sop);
    
    DEBUG_OPERATION (("SSHOP: SSH process done\n"));

    /* Close off the askpass io channel etc... */
    if(pv->stag_askpass)
        g_source_remove (pv->stag_askpass);
    pv->stag_askpass = 0;
    
    if (bastile_operation_is_running (BASTILE_OPERATION (sop))) {

        /* Was killed */
        if (!WIFEXITED (status)) {
            bastile_operation_mark_done (BASTILE_OPERATION (sop), FALSE, 
                g_error_new (BASTILE_ERROR, 0, _("The SSH command was terminated unexpectedly.")));
            
        /* Command failed */
        } else if (WEXITSTATUS (status) != 0) {
            g_warning ("SSH command failed: (%d)", WEXITSTATUS (status));
            if (pv->serr->len)
                g_warning ("SSH error output: %s", pv->serr->str);
            bastile_operation_mark_done (BASTILE_OPERATION (sop), FALSE, 
                g_error_new_literal (BASTILE_ERROR, 0, pv->serr->len ? pv->serr->str : _("The SSH command failed.")));

        /* Successful completion */
        } else {
            
            /*
             * If a result callback is set (by one of our specific operations below)
             * then we let it setup the result. Otherwise use the output string.
             */
            if (pv->result_cb)
                (pv->result_cb) (sop);
            else
                bastile_operation_mark_result (BASTILE_OPERATION (sop), pv->sout->str, NULL);
    
            /* The result callback may have completed operation */
            if (bastile_operation_is_running (BASTILE_OPERATION (sop)))
                bastile_operation_mark_done (BASTILE_OPERATION (sop), FALSE, NULL);
        }
    }

    g_spawn_close_pid (pid);
    pv->pid = 0;
    pv->wpid = 0;
    
    
    if(pv->io_askpass)
        g_io_channel_unref (pv->io_askpass);
    pv->io_askpass = NULL;
    
    /* This watch holds a ref on the operation, release */
    g_object_unref (sop);
}

static gboolean    
io_ssh_write (GIOChannel *source, GIOCondition condition, BastileSSHOperation *sop)
{
    BastileSSHOperationPrivate *pv = BASTILE_SSH_OPERATION_GET_PRIVATE (sop);
    GError *error = NULL;
    GIOStatus status;
    gsize written = 0;

    if (bastile_operation_is_running (BASTILE_OPERATION (sop)) && pv->sin) {
        DEBUG_OPERATION (("SSHOP: SSH ready for input\n"));
        
        status = g_io_channel_write_chars (pv->iin, pv->sin->str, pv->sin->len,
                                           &written, &error);
        switch (status) {
        case G_IO_STATUS_ERROR:
            bastile_operation_mark_done (BASTILE_OPERATION (sop), FALSE, error);
            break;
        case G_IO_STATUS_AGAIN:
            break;
        default:
            DEBUG_OPERATION (("SSHOP: Wrote %d bytes to SSH\n", (gint)written));
            g_string_erase (pv->sin, 0, written);
            break;
        }
    }
    
    if (pv->sin && !pv->sin->len) {
        DEBUG_OPERATION (("SSHOP: Finished writing SSH input\n"));
        g_string_free (pv->sin, TRUE);
        pv->sin = NULL;
    }
    
    if (!bastile_operation_is_running (BASTILE_OPERATION (sop)) || !pv->sin) {
        DEBUG_OPERATION (("SSHOP: Closing SSH input channel\n"));
        g_io_channel_unref (pv->iin);
        pv->iin = NULL;
        g_source_remove (pv->win);
        pv->win = 0;
        return FALSE;
    }
    
    return TRUE;
}

static gboolean    
io_ssh_read (GIOChannel *source, GIOCondition condition, BastileSSHOperation *sop)
{
    BastileSSHOperationPrivate *pv = BASTILE_SSH_OPERATION_GET_PRIVATE (sop);
    GError *error = NULL;
    gchar buf[128];
    GIOStatus status;
    gsize read = 0;
    GString *str;
    
    if (!bastile_operation_is_running (BASTILE_OPERATION (sop)))
        return TRUE;
    
    /* Figure out which buffer we're writing into */
    if (source == pv->iout) {
        str = pv->sout;
        DEBUG_OPERATION (("SSHOP: SSH output: "));    
    } else if(source == pv->ierr) {
        str = pv->serr;
        DEBUG_OPERATION (("SSHOP: SSH errout: "));
    } else
        g_assert_not_reached ();

    do {
        status = g_io_channel_read_chars (source, buf, sizeof (buf), &read, &error);
        switch (status) {
        case G_IO_STATUS_ERROR:
            bastile_operation_mark_done (BASTILE_OPERATION (sop), FALSE, error);
            break;
        case G_IO_STATUS_AGAIN:
            continue;
        case G_IO_STATUS_EOF:
            break;
        default:
            g_string_append_len (str, buf, read);
            DEBUG_OPERATION (("%s\n", str->str + (str->len - read)));
            break;
        }
    } while (read == sizeof (buf));
    
    return TRUE;
}

/* Communication with bastile-ssh-askpass */
static gboolean
askpass_handler (GIOChannel *source, GIOCondition condition, BastileSSHOperation *sop)
{
    BastileSSHOperationPrivate *pv = BASTILE_SSH_OPERATION_GET_PRIVATE (sop);
    gchar *string = NULL;
    gsize length;
    GError *err = NULL;
    gboolean ret = TRUE;
    const gchar *line;
    const gchar *result = NULL;

    if (condition & G_IO_IN) {
        
        /* Read 1 line from the io channel, including newline character */
        g_io_channel_read_line (source, &string, &length, NULL, &err);

        if (err != NULL) {
            g_critical ("couldn't read from bastile-ssh-askpass: %s", err->message);
            g_clear_error (&err);
            ret = FALSE;
        }
        
        /* Process the line */
        if (string && ret) {
            
            string[length] = 0;
            DEBUG_OPERATION (("SSHOP: bastile-ssh-askpass request: %s\n", string));
            
            if (g_ascii_strncasecmp (COMMAND_PASSWORD, string, COMMAND_PASSWORD_LEN) == 0) {
                line = g_strstrip (string + COMMAND_PASSWORD_LEN);
                
                /* Prompt for a password */
                if (pv->password_cb) {
                    result = (pv->password_cb) (sop, line);
                    
                    /* Cancelled prompt, cancel operation */
                    if (!result) {
                        if (bastile_operation_is_running (BASTILE_OPERATION (sop)))
                            bastile_operation_cancel (BASTILE_OPERATION (sop));
                        DEBUG_OPERATION (("SSHOP: password prompt cancelled\n"));
                        ret = FALSE;
                    }
                }
                
                pv->prompt_requests++;
            }
            
            if (ret) {
                /* And write the result back out to bastile-ssh-askpass */
                DEBUG_OPERATION (("SSHOP: bastile-ssh-askpass response: %s\n", result ? result : ""));
                if (result)
                    g_io_channel_write_chars (pv->io_askpass, result, strlen (result), &length, &err);
                if (err == NULL)
                    g_io_channel_write_chars (pv->io_askpass, "\n", 1, &length, &err);
                if (err == NULL)
                    g_io_channel_flush (pv->io_askpass, &err);
                if (err != NULL) {
                    g_critical ("couldn't read from bastile-ssh-askpass: %s", err->message);
                    g_clear_error (&err);
                    ret = FALSE;
                }
            }
        }
    }

    if (condition & G_IO_HUP)
        ret = FALSE;
        
    if (!ret) {
        if (pv->io_askpass)
            g_io_channel_unref (pv->io_askpass);
        pv->io_askpass = NULL;
        pv->stag_askpass = 0;
    }

    g_free (string);
    return ret;
}

static void
ssh_child_setup (gpointer user_data)
{
    BastileSSHOperationPrivate *pv = (BastileSSHOperationPrivate*)user_data;
    gchar buf[15];

    /* No terminal for this process */
    setsid ();
    
    g_setenv ("SSH_ASKPASS", EXECDIR "bastile-ssh-askpass", FALSE);

    /* We do screen scraping so we need locale C */
    if (g_getenv ("LC_ALL"))
        g_setenv ("LC_ALL", "C", TRUE);
    g_setenv ("LANG", "C", TRUE);
    
    /* Let child know which fd it is */
    if (pv->fds_askpass[1] != -1) {
        snprintf (buf, sizeof (buf), "%d", pv->fds_askpass[1]);
        g_setenv ("BASTILE_SSH_ASKPASS_FD", buf, TRUE);
    }
    
    /* Child doesn't need this stuff */
    if (pv->fds_askpass[0] != -1)
        close(pv->fds_askpass[0]);
}

static void
ssh_sync_child_setup (gpointer user_data)
{
    /* No terminal for this process */
    setsid ();
}

static const gchar*
get_algorithm_text (guint algo)
{
    switch (algo) {
    case SSH_ALGO_DSA:
        return "dsa";
    case SSH_ALGO_RSA:
        return "rsa";
    default:
        g_return_val_if_reached (NULL);
        break;
    }
}

static gchar*
escape_shell_arg (const gchar *arg)
{
    guint len = 2;
    const gchar *t;
    gchar *e, *escaped;
    
    for (t = arg; *t; t++) {
        if (*t == '\'')
            len += 3;
        ++len;
    }
    
    escaped = g_new0 (gchar, len + 1);
    escaped[0] = '\'';
    
    for (t = arg, e = escaped + 1; *t; t++) {
        if (*t == '\'') {
            strcpy (e, "'\''");
            e += 4;
        } else {
            *e = *t;
            e++;
        }
    }

    g_assert (e < escaped + len);
    *e = '\'';
    
    return escaped;
}

static const gchar* 
prompt_passphrase (BastileSSHOperation *sop, const gchar* title, const gchar* message,
                   const gchar* check, gboolean confirm)
{
    BastileSSHOperationPrivate *pv = BASTILE_SSH_OPERATION_GET_PRIVATE (sop);
    const gchar *display;
    gchar *msg;
    
    if (pv->prompt_dialog)
        gtk_widget_destroy (GTK_WIDGET (pv->prompt_dialog));
    
    if (pv->prompt_skey)
        display = bastile_object_get_label (pv->prompt_skey);
    else 
        display = _("Secure Shell key");
    msg = g_strdup_printf (message, display);

    pv->prompt_dialog = bastile_passphrase_prompt_show (title, msg, _("Passphrase:"), 
                                                         check, confirm);
    g_free (msg);
    
    /* Run and check if cancelled? */
    if (gtk_dialog_run (pv->prompt_dialog) != GTK_RESPONSE_ACCEPT) {
        gtk_widget_destroy (GTK_WIDGET (pv->prompt_dialog));
        pv->prompt_dialog = NULL;
        return NULL;
    }
    
    gtk_widget_hide (GTK_WIDGET (pv->prompt_dialog));
    return bastile_passphrase_prompt_get (pv->prompt_dialog);
}

static const gchar* 
prompt_password (BastileSSHOperation *sop, const gchar* title, const gchar* message,
                 const gchar* check, gboolean confirm)
{
    BastileSSHOperationPrivate *pv = BASTILE_SSH_OPERATION_GET_PRIVATE (sop);
    const gchar *display;
    gchar *msg;
    
    if (pv->prompt_dialog)
        gtk_widget_destroy (GTK_WIDGET (pv->prompt_dialog));
    
    if (pv->prompt_skey)
        display = bastile_object_get_label (pv->prompt_skey);
    else 
        display = g_strdup (_("Secure Shell key"));
    msg = g_strdup_printf (message, display);

    pv->prompt_dialog = bastile_passphrase_prompt_show (title, msg, _("Password:"), 
                                                         check, confirm);
    g_free (msg);
    
    /* Run and check if cancelled? */
    if (gtk_dialog_run (pv->prompt_dialog) != GTK_RESPONSE_ACCEPT) {
        gtk_widget_destroy (GTK_WIDGET (pv->prompt_dialog));
        pv->prompt_dialog = NULL;
        return NULL;
    }
    
    gtk_widget_hide (GTK_WIDGET (pv->prompt_dialog));
    return bastile_passphrase_prompt_get (pv->prompt_dialog);
}

/* -----------------------------------------------------------------------------
 * OBJECT 
 */

static void 
bastile_ssh_operation_init (BastileSSHOperation *sop)
{
    BastileSSHOperationPrivate *pv = BASTILE_SSH_OPERATION_GET_PRIVATE (sop);
    
    pv->sout = g_string_new (NULL);
    pv->serr = g_string_new (NULL);

    /* The bastile-ssh-askpass pipes */
    if (socketpair (AF_UNIX, SOCK_STREAM, 0, pv->fds_askpass) == -1) {
        g_warning ("couldn't create pipes to communicate with bastile-ssh-askpass: %s",
                   strerror(errno));
        pv->fds_askpass[0] = -1;
        pv->fds_askpass[1] = -1;
    }
}

static void 
bastile_ssh_operation_set_property (GObject *gobject, guint prop_id, 
                                     const GValue *value, GParamSpec *pspec)
{
    
}

static void 
bastile_ssh_operation_get_property (GObject *gobject, guint prop_id, 
                                     GValue *value, GParamSpec *pspec)
{
    BastileSSHOperation *sop = BASTILE_SSH_OPERATION (gobject);
    
    switch (prop_id) {
    case PROP_KEY_SOURCE:
        g_value_set_object (value, sop->sksrc);
        break;
    }
}

static void 
bastile_ssh_operation_dispose (GObject *gobject)
{
    BastileOperation *op = BASTILE_OPERATION (gobject);
    BastileSSHOperationPrivate *pv = BASTILE_SSH_OPERATION_GET_PRIVATE (op);

    if (bastile_operation_is_running (op))
        bastile_ssh_operation_cancel (op);
    
    if (pv->prompt_dialog)
        gtk_widget_destroy (GTK_WIDGET (pv->prompt_dialog));
    pv->prompt_dialog = NULL;
    
    G_OBJECT_CLASS (ssh_operation_parent_class)->dispose (gobject);  
}

static void 
bastile_ssh_operation_finalize (GObject *gobject)
{
    BastileSSHOperation *sop = BASTILE_SSH_OPERATION (gobject);
    BastileSSHOperationPrivate *pv = BASTILE_SSH_OPERATION_GET_PRIVATE (sop);
    
    if (pv->win)
        g_source_remove (pv->win);
    if (pv->wout)
        g_source_remove (pv->wout);
    if (pv->werr)
        g_source_remove (pv->werr);

    if (pv->iin)
        g_io_channel_unref (pv->iin);
    if (pv->iout)
        g_io_channel_unref (pv->iout);
    if (pv->ierr)
        g_io_channel_unref (pv->ierr);
        
    if (pv->sin)
        g_string_free (pv->sin, TRUE);
    g_string_free (pv->sout, TRUE);
    g_string_free (pv->serr, TRUE);
    
    /* Close the sockets */
    if (pv->fds_askpass[0] != -1) 
        close (pv->fds_askpass[0]);
    pv->fds_askpass[0] = -1;

    if (pv->fds_askpass[1] != -1) 
        close (pv->fds_askpass[1]);
    pv->fds_askpass[1] = -1;
    
    g_assert (pv->prompt_dialog == NULL);

    /* watch_ssh_process always needs to have been called */
    g_assert (pv->pid == 0 && pv->wpid == 0);
    g_assert (pv->io_askpass == NULL && pv->stag_askpass == 0);
    
    G_OBJECT_CLASS (ssh_operation_parent_class)->finalize (gobject);  
}

static void 
bastile_ssh_operation_cancel (BastileOperation *operation)
{
    BastileSSHOperation *sop = BASTILE_SSH_OPERATION (operation);    
    BastileSSHOperationPrivate *pv = BASTILE_SSH_OPERATION_GET_PRIVATE (sop);

    bastile_operation_mark_done (operation, TRUE, NULL);

    if (pv->pid != 0)
        kill (pv->pid, SIGTERM);
}

/* -----------------------------------------------------------------------------
 * PUBLIC 
 */

BastileOperation*
bastile_ssh_operation_new (BastileSSHSource *ssrc, const gchar *command, 
                            const gchar *input, gint length, BastileSSHKey *skey)
{
    BastileSSHOperationPrivate *pv;
    BastileSSHOperation *sop;
    GError *error = NULL;
    int argc, r;
    int fin, fout, ferr;
    char **argv;
    
    g_return_val_if_fail (BASTILE_IS_SSH_SOURCE (ssrc), NULL);
    g_return_val_if_fail (command && command[0], NULL);
    
    if (!g_shell_parse_argv (command, &argc, &argv, NULL)) {
        g_critical ("couldn't parse ssh command line: %s\n", command);
        g_return_val_if_reached (NULL);
    }
    
    sop = g_object_new (BASTILE_TYPE_SSH_OPERATION, NULL);
    pv = BASTILE_SSH_OPERATION_GET_PRIVATE (sop);

    sop->sksrc = ssrc;
    pv->prompt_skey = BASTILE_OBJECT (skey);
    
    DEBUG_OPERATION (("SSHOP: Executing SSH command: %s\n", command));
    
    /* And off we go to run the program */
    r = g_spawn_async_with_pipes (NULL, argv, NULL, 
                                  G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_LEAVE_DESCRIPTORS_OPEN 	, 
                                  ssh_child_setup, pv, &pv->pid, 
                                  input ? &fin : NULL, &fout, &ferr, &error);
    g_strfreev (argv);  
    
    if (!r)
        return bastile_operation_new_complete (error);
    
    /* Copy the input for later writing */
    if (input) {
        pv->sin = g_string_new_len (input, length == -1 ? strlen (input) : length);
        DEBUG_OPERATION (("SSHOP: Will send SSH input: %s", pv->sin->str));    
        
        fcntl (fin, F_SETFL, O_NONBLOCK | fcntl (fin, F_GETFL));
        pv->iin = g_io_channel_unix_new (fin);
        g_io_channel_set_encoding (pv->iin, NULL, NULL);
        g_io_channel_set_close_on_unref (pv->iin, TRUE);
        pv->win = g_io_add_watch (pv->iin, G_IO_OUT, (GIOFunc)io_ssh_write, sop);
    }
    
    /* Make all the proper IO Channels for the output/error */
    fcntl (fout, F_SETFL, O_NONBLOCK | fcntl (fout, F_GETFL));
    pv->iout = g_io_channel_unix_new (fout);
    g_io_channel_set_encoding (pv->iout, NULL, NULL);
    g_io_channel_set_close_on_unref (pv->iout, TRUE);
    pv->wout = g_io_add_watch (pv->iout, G_IO_IN, (GIOFunc)io_ssh_read, sop);
    
    fcntl (ferr, F_SETFL, O_NONBLOCK | fcntl (ferr, F_GETFL));
    pv->ierr = g_io_channel_unix_new (ferr);
    g_io_channel_set_encoding (pv->ierr, NULL, NULL);
    g_io_channel_set_close_on_unref (pv->ierr, TRUE);
    pv->werr = g_io_add_watch (pv->ierr, G_IO_IN, (GIOFunc)io_ssh_read, sop);
    
    /* Process watch */
    g_object_ref (sop); /* When the process ends, reference is released */
    pv->wpid = g_child_watch_add (pv->pid, (GChildWatchFunc)watch_ssh_process, sop);
    
    /* Setup askpass communication */
    if (pv->fds_askpass[0] != -1) {
        pv->io_askpass = g_io_channel_unix_new (pv->fds_askpass[0]);
        g_io_channel_set_close_on_unref (pv->io_askpass, TRUE);
        g_io_channel_set_encoding (pv->io_askpass, NULL, NULL);
        pv->stag_askpass = g_io_add_watch (pv->io_askpass, G_IO_IN | G_IO_HUP, 
                                           (GIOFunc)askpass_handler, sop);
        pv->fds_askpass[0] = -1; /* closed by io channel */
    }
    
    /* The other end of the pipe, close it */
    if (pv->fds_askpass[1] != -1) 
        close (pv->fds_askpass[1]);
    pv->fds_askpass[1] = -1;
    
    bastile_operation_mark_start (BASTILE_OPERATION (sop));
    
    return BASTILE_OPERATION (sop);
}

gchar*
bastile_ssh_operation_sync (BastileSSHSource *ssrc, const gchar *command, 
                             GError **error)
{
    GError *err = NULL;
    gchar *sout, *serr;
    gint status;
    gint argc;
    gchar **argv;
    gboolean r;
    
    g_assert (!error || !*error);
    
    /* We use this internally, so guarantee it exists */
    if (!error)
        error = &err;
    
    if (!g_shell_parse_argv (command, &argc, &argv, NULL)) {
        g_critical ("couldn't parse ssh command line: %s\n", command);
        return NULL;
    }
    
    DEBUG_OPERATION (("SSHOP: executing SSH command: %s\n", command));
    
    r = g_spawn_sync (NULL, argv, NULL, 0, ssh_sync_child_setup, NULL, 
                      &sout, &serr, &status, error);
    g_strfreev (argv);
    
    if (!r) {
        g_critical ("couldn't execute SSH command: %s (%s)", command, *error ? (*error)->message : "");
        return NULL;
    }
    
    if (!WIFEXITED (status)) {
        g_critical ("SSH command didn't exit properly: %s", command);
        g_set_error (error, BASTILE_ERROR, 0, "%s", _("The SSH command was terminated unexpectedly."));
        g_free (sout);
        g_free (serr);
        return NULL;
    }
    
    if (WEXITSTATUS (status) != 0) {
        g_set_error (error, BASTILE_ERROR, 0, "%s", _("The SSH command failed."));
        g_warning ("SSH command failed: %s (%d)", command, WEXITSTATUS (status));
        if (serr && serr[0])
            g_warning ("SSH error output: %s", serr);
        g_free (serr);
        g_free (sout);
        return NULL;
    }
    
    g_free (serr);
    return sout;
}

/* -----------------------------------------------------------------------------
 * UPLOAD KEY 
 */

static const gchar*
upload_password_cb (BastileSSHOperation *sop, const gchar* msg)
{
    DEBUG_OPERATION (("in upload_password_cb\n"));

    /* Just prompt over and over again */
    return prompt_password (sop, _("Remote Host Password"), msg, NULL, FALSE);
}

BastileOperation*  
bastile_ssh_operation_upload (BastileSSHSource *ssrc, GList *keys, 
                               const gchar *username, const gchar *hostname, const gchar *port)
{
    BastileSSHOperationPrivate *pv;
    BastileOperation *op;
    GMemoryOutputStream *output;
    gchar *data;
    size_t length;
    gchar *cmd;
    
    g_return_val_if_fail (keys != NULL, NULL);
    g_return_val_if_fail (username && username[0], NULL);
    g_return_val_if_fail (hostname && hostname[0], NULL);
    
    if (port && !port[0])
        port = NULL;
    
    	output = G_MEMORY_OUTPUT_STREAM (g_memory_output_stream_new (NULL, 0, g_realloc, NULL));
    	g_return_val_if_fail (output, NULL);
    	
    	/* Buffer for what we send to the server */
    	op = bastile_source_export (BASTILE_SOURCE (ssrc), keys, G_OUTPUT_STREAM (output));
    	g_return_val_if_fail (op != NULL, NULL);
    
    /* 
     * We happen to know that bastile_ssh_source_export always returns
     * completed operations, so we don't need to factor that in. If this
     * ever changes, then we need to recode this bit 
     */
    g_return_val_if_fail (!bastile_operation_is_running (op), NULL);
    
    /* Return any errors */
    if (!bastile_operation_is_successful (op)) {
        g_object_unref (output);
        return op;
    }
    
    /* Free the export operation */
    g_object_unref (op);

    /* 
     * This command creates the .ssh directory if necessary (with appropriate permissions) 
     * and then appends all input data onto the end of .ssh/authorized_keys
     */
    /* TODO: Important, we should handle the host checking properly */
    cmd = g_strdup_printf (SSH_PATH " '%s@%s' %s %s -o StrictHostKeyChecking=no "
                                    "\"umask 077; test -d .ssh || mkdir .ssh ; cat >> .ssh/authorized_keys\"", 
                           username, hostname, 
                           port ? "-p" : "", 
                           port ? port : "");
    
	data = g_memory_output_stream_get_data (output);
	length = strlen (data);
	g_object_unref (output);

	op = bastile_ssh_operation_new (ssrc, cmd, data, length, NULL);
    
	g_free (cmd);
	g_free (data);

    pv = BASTILE_SSH_OPERATION_GET_PRIVATE (op);
    pv->password_cb = upload_password_cb;

    return op;
}

/* -----------------------------------------------------------------------------
 * CHANGE PASSPHRASE 
 */

static const gchar*
change_password_cb (BastileSSHOperation *sop, const gchar* msg)
{
    BastileSSHOperationPrivate *pv = BASTILE_SSH_OPERATION_GET_PRIVATE (sop);
    const gchar *ret = NULL;
    gchar *lcase;

    lcase = g_strdup (msg ? msg : "");
    bastile_util_string_lower (lcase);
    
    DEBUG_OPERATION (("in change_password_cb\n"));
    
    /* Need the old passphrase */
    if (strstr (lcase, "old pass"))
        ret = prompt_passphrase (sop, _("Old Key Passphrase"), 
                _("Enter the old passphrase for: %s"), NULL, FALSE);
        
    /* Look for the new passphrase thingy */
    else if (strstr (lcase, "new pass"))
        ret = prompt_passphrase (sop, _("New Key Passphrase"), 
                _("Enter the new passphrase for: %s"), NULL, TRUE);
        
    /* Confirm the new passphrase, just send it again */
    else if (strstr (lcase, "again") && pv->prompt_dialog)
        ret = bastile_passphrase_prompt_get (pv->prompt_dialog);
        
    /* Something we don't understand */
    else
        ret = prompt_passphrase (sop, _("Enter Key Passphrase"), msg, NULL, FALSE);
    
    g_free (lcase);
    return ret;
}

static void
change_result_cb (BastileSSHOperation *sop)
{
	BastileSSHOperationPrivate *pv = BASTILE_SSH_OPERATION_GET_PRIVATE (sop);
	if (pv->prompt_skey)
		bastile_object_refresh (BASTILE_OBJECT (pv->prompt_skey));
}

BastileOperation*
bastile_ssh_operation_change_passphrase (BastileSSHKey *skey)
{
    BastileSSHOperationPrivate *pv;
    BastileSource *ssrc;
    BastileOperation *op;
    gchar *cmd;
    
    g_return_val_if_fail (BASTILE_IS_SSH_KEY (skey), NULL);
    g_return_val_if_fail (skey->keydata && skey->keydata->privfile, NULL);
    
    ssrc = bastile_object_get_source (BASTILE_OBJECT (skey));
    g_return_val_if_fail (BASTILE_IS_SSH_SOURCE (ssrc), NULL);
    
    cmd = g_strdup_printf (SSH_KEYGEN_PATH " -p -f '%s'", skey->keydata->privfile);
    op = bastile_ssh_operation_new (BASTILE_SSH_SOURCE (ssrc), cmd, NULL, -1, skey);
    g_free (cmd);
    
    pv = BASTILE_SSH_OPERATION_GET_PRIVATE (op);
    pv->password_cb = change_password_cb;
    pv->result_cb = change_result_cb;
    
    return op;
}

/* -----------------------------------------------------------------------------
 * KEY GENERATE OPERATION
 */ 

static void
generate_result_cb (BastileSSHOperation *sop)
{
    BastileSSHKey *skey;
    const char *filename;
    
    filename = g_object_get_data (G_OBJECT (sop), "filename");
    g_return_if_fail (filename != NULL);
    
    /* The result of the operation is the key we generated */
    skey = bastile_ssh_source_key_for_filename (sop->sksrc, filename);
    g_return_if_fail (BASTILE_IS_SSH_KEY (skey));
    
    bastile_operation_mark_result (BASTILE_OPERATION (sop), skey, NULL);
}

static const gchar*
generate_password_cb (BastileSSHOperation *sop, const gchar* msg)
{
    BastileSSHOperationPrivate *pv = BASTILE_SSH_OPERATION_GET_PRIVATE (sop);
    DEBUG_OPERATION (("in generate_password_cb\n"));

    /* If the first time then prompt */
    if (!pv->prompt_dialog) {
        return prompt_passphrase (sop, _("Passphrase for New Secure Shell Key"), 
                _("Enter a passphrase for your new Secure Shell key."), NULL, TRUE);
    }
    
    /* Otherwise return the entered passphrase */
    return bastile_passphrase_prompt_get (pv->prompt_dialog);
}

BastileOperation*
bastile_ssh_operation_generate (BastileSSHSource *src, const gchar *email, 
                                 guint type, guint bits)
{
    BastileSSHOperationPrivate *pv;
    BastileOperation *op;
    gchar *filename, *comment;
    const gchar *algo;
    gchar *cmd;
    
    filename = bastile_ssh_source_file_for_algorithm (src, type);
    g_return_val_if_fail (filename, NULL);
    
    comment = escape_shell_arg (email);
    
    algo = get_algorithm_text (type);
    g_return_val_if_fail (algo != NULL, NULL);
    
    /* Default number of bits */
    if (bits == 0)
        bits = 2048;
    
    cmd = g_strdup_printf (SSH_KEYGEN_PATH " -b '%d' -t '%s' -C %s -f '%s'",
                           bits, algo, comment, filename);
    g_free (comment);
    
    op = bastile_ssh_operation_new (BASTILE_SSH_SOURCE (src), cmd, NULL, -1, NULL);
    g_free (cmd);
    
    pv = BASTILE_SSH_OPERATION_GET_PRIVATE (op);
    pv->result_cb = generate_result_cb;
    pv->password_cb = generate_password_cb;
    g_object_set_data_full (G_OBJECT (op), "filename", filename, g_free);
    
    return op;
}

/* -----------------------------------------------------------------------------
 * IMPORT A PUBLIC KEY 
 */

BastileOperation*
bastile_ssh_operation_import_public (BastileSSHSource *ssrc, BastileSSHKeyData *data,
                                      const gchar* filename)
{
    BastileOperation *op;
    GError *err = NULL;
    
    g_return_val_if_fail (bastile_ssh_key_data_is_valid (data), NULL);
    g_return_val_if_fail (data->rawdata, NULL);
    g_return_val_if_fail (BASTILE_IS_SSH_SOURCE (ssrc), NULL);

    bastile_ssh_key_data_filter_file (filename, data, NULL, &err);
    
    op = bastile_operation_new_complete (err);
    bastile_operation_mark_result (op, g_strdup (data->fingerprint), g_free);
    return op;
}


/* -----------------------------------------------------------------------------
 * IMPORT A PRIVATE KEY 
 */ 

static const gchar*
import_password_cb (BastileSSHOperation *sop, const gchar* msg)
{
    const gchar *comment;
    const gchar *ret;
    gchar* message;
    
    /* Add the comment to the output */
    comment = (const gchar*)g_object_get_data (G_OBJECT (sop), "import-comment");
    if (comment)
        message = g_strdup_printf (_("Importing key: %s"), comment);
    else
        message = g_strdup (_("Importing key. Enter passphrase"));
    
    ret = prompt_passphrase (sop, _("Import Key"), message, NULL, FALSE);
    g_free (message);
    
    return ret;
}

static void
import_result_cb (BastileSSHOperation *sop)
{
    BastileSSHOperationPrivate *pv = BASTILE_SSH_OPERATION_GET_PRIVATE (sop);
    BastileSSHKeyData *keydata;
    const gchar *pubfile;
    const gchar *comment;
    GError *err = NULL;
    gsize pos;
    
    g_assert (bastile_operation_is_running (BASTILE_OPERATION (sop)));
    
    /* Only use the first line of the output */
    pos = strcspn (pv->sout->str, "\n\r");
    if (pos < pv->sout->len)
        g_string_erase (pv->sout, pos, -1);
    
    /* Parse the data so we can get the fingerprint */
    keydata = bastile_ssh_key_data_parse_line (pv->sout->str, -1);
    if (bastile_ssh_key_data_is_valid (keydata))
        bastile_operation_mark_result (BASTILE_OPERATION (sop),
                                        g_strdup (keydata->fingerprint), g_free);
    else
        g_warning ("couldn't parse imported private key fingerprint");
    bastile_ssh_key_data_free (keydata);
    
    /* Add the comment to the output */
    comment = (const gchar*)g_object_get_data (G_OBJECT (sop), "import-comment");
    if (comment) {
        g_string_append_c (pv->sout, ' ');
        g_string_append (pv->sout, comment);
    }
    
    /* The file to write to */
    pubfile = (const gchar*)g_object_get_data (G_OBJECT (sop), "import-file");
    g_assert (pubfile);
    
    if (!bastile_util_write_file_private (pubfile, pv->sout->str, &err))
        bastile_operation_mark_done (BASTILE_OPERATION (sop), FALSE, err);
}

BastileOperation*
bastile_ssh_operation_import_private (BastileSSHSource *ssrc, BastileSSHSecData *data,
                                       const gchar *filename)
{
    BastileSSHOperationPrivate *pv;
    BastileOperation *op;
    gchar *cmd, *privfile = NULL;
    GError *err = NULL;
    
    g_return_val_if_fail (data && data->rawdata, NULL);
    g_return_val_if_fail (BASTILE_IS_SSH_SOURCE (ssrc), NULL);
    
    /* No filename specified, make one up */
    if (!filename) {
        filename = privfile = bastile_ssh_source_file_for_algorithm (ssrc, data->algo);
        g_return_val_if_fail (privfile, NULL);
    }
    
    /* Write the private key into the file */
    if (!bastile_util_write_file_private (filename, data->rawdata, &err)) {
        g_free (privfile);
        return bastile_operation_new_complete (err);
    }
    
    /* Start command to generate public key */
    cmd = g_strdup_printf (SSH_KEYGEN_PATH " -y -f '%s'", privfile);
    op = bastile_ssh_operation_new (ssrc, cmd, NULL, 0, NULL);
    g_free (cmd);
    
    g_object_set_data_full (G_OBJECT (op), "import-file", 
                g_strdup_printf ("%s.pub", filename), g_free);
    g_object_set_data_full (G_OBJECT (op), "import-comment",
                g_strdup (data->comment), g_free);
    
    g_free (privfile);
    
    pv = BASTILE_SSH_OPERATION_GET_PRIVATE (op);
    pv->result_cb = import_result_cb;
    pv->password_cb = import_password_cb;
    
    return op;
}

/* -----------------------------------------------------------------------------
 * AUTHORIZE A PUBLIC KEY 
 */ 

BastileOperation*
bastile_ssh_operation_authorize (BastileSSHSource *ssrc, BastileSSHKey *skey,
                                  gboolean authorize)
{
    BastileSSHKeyData *keydata = NULL;
    GError *err = NULL;
    gchar* from = NULL;
    gchar* to = NULL;
    
    g_return_val_if_fail (BASTILE_IS_SSH_SOURCE (ssrc), NULL);
    g_return_val_if_fail (BASTILE_IS_SSH_KEY (skey), NULL);
    
    g_object_get (skey, "key-data", &keydata, NULL);
    g_return_val_if_fail (keydata, NULL);
    
    if (authorize) {
        to = bastile_ssh_source_file_for_public (ssrc, TRUE);
    } else {
        from = bastile_ssh_source_file_for_public (ssrc, TRUE);
        to = bastile_ssh_source_file_for_public (ssrc, FALSE);
    }
    
    /* Take it out of the from file, and put into the to file */
    if (!from || bastile_ssh_key_data_filter_file (from, NULL, keydata, &err))
        bastile_ssh_key_data_filter_file (to, keydata, NULL, &err);
    
    g_free (from);
    g_free (to);
    
    /* Just reload that one key */
    if (!err)
        bastile_object_refresh (BASTILE_OBJECT (skey));
    
    return bastile_operation_new_complete (err);
}

/* -----------------------------------------------------------------------------
 * RENAME COMMENT ON A KEY 
 */

static gboolean
change_raw_comment (BastileSSHKeyData *keydata, const gchar *newcomment)
{
    const gchar *x = keydata->rawdata;
    gchar *result;
    gchar **parts;
    
    g_assert (x);
    while (*x && g_ascii_isspace (*x))
        ++x;
    
    parts = g_strsplit_set (x, " ", 3);
    if (!parts[0] || !parts[1])
        return FALSE;
    
    result = g_strconcat (parts[0], " ", parts[1], " ", newcomment, NULL);
    g_strfreev(parts);
    
    g_free (keydata->rawdata);
    keydata->rawdata = result;
    return TRUE;
}

BastileOperation*
bastile_ssh_operation_rename (BastileSSHSource *ssrc, BastileSSHKey *skey,
                               const gchar *newcomment)
{
    BastileSSHKeyData *keydata;
    GError *err = NULL;
    
    g_return_val_if_fail (BASTILE_IS_SSH_SOURCE (ssrc), NULL);
    g_return_val_if_fail (BASTILE_IS_SSH_KEY (skey), NULL);
    g_assert (bastile_ssh_key_data_is_valid (skey->keydata));
    g_assert (skey->keydata->rawdata);
    
    keydata = bastile_ssh_key_data_dup (skey->keydata);
    
    if (!newcomment)
        newcomment = "";
    
    if (!change_raw_comment (keydata, newcomment ? newcomment : ""))
        g_return_val_if_reached (NULL);
    
    DEBUG_OPERATION (("renaming key to: %s", newcomment));
    
    /* Just part of a file for this key */
    if (keydata->partial) {
        g_assert (keydata->pubfile);
        bastile_ssh_key_data_filter_file (keydata->pubfile, keydata, keydata, &err);
        
    /* A full file for this key */
    } else {
        g_assert (keydata->pubfile);
        bastile_util_write_file_private (keydata->pubfile, keydata->rawdata, &err);
    }
    
    bastile_ssh_key_data_free (keydata);
    return bastile_operation_new_complete (err);
}
