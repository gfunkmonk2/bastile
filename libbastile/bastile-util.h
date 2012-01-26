/*
 * Bastile
 *
 * Copyright (C) 2003 Jacob Perkins
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
 
/**
 * A bunch of miscellaneous utility functions.
 */

#ifndef __BASTILE_UTIL_H__
#define __BASTILE_UTIL_H__

#include <gtk/gtk.h>
#include <time.h>

#include "bastile-context.h"

#ifdef WITH_SHARING
#include <avahi-client/client.h>
const AvahiPoll* bastile_util_dns_sd_get_poll ();
#endif

struct _BastileObject;

typedef enum {
    BASTILE_CRYPT_SUFFIX,
    BASTILE_SIG_SUFFIX,
} BastileSuffix;

typedef guint64 BastileVersion;

#define BASTILE_EXT_ASC ".asc"
#define BASTILE_EXT_SIG ".sig"
#define BASTILE_EXT_PGP ".pgp"
#define BASTILE_EXT_GPG ".gpg"

gchar*		bastile_util_get_date_string		    (const time_t		time);
gchar*		bastile_util_get_display_date_string   (const time_t		time);


#define BASTILE_ERROR  (bastile_util_error_domain ())

GQuark      bastile_util_error_domain ();

void        bastile_util_show_error            (GtkWidget          *parent,
                                                 const gchar        *heading,
                                                 const gchar        *message);
                                                 
void        bastile_util_handle_error          (GError*            err,
                                                 const char*        desc, ...);

gboolean    bastile_util_prompt_delete         (const gchar *text,
                                                 GtkWidget *parent);

guchar*     bastile_util_read_to_memory        (GInputStream *     input,
                                                 guint              *len);

guint       bastile_util_read_data_block       (GString            *buf, 
                                                 GInputStream*      input, 
                                                 const gchar        *start, 
                                                 const gchar*       end);

GMemoryInputStream*
            bastile_util_memory_input_string   (const gchar *string, gsize length);

gsize       bastile_util_memory_output_length  (GMemoryOutputStream *output);

gboolean    bastile_util_print_fd          (int fd, 
                                             const char* data);

gboolean    bastile_util_printf_fd         (int fd, 
                                             const char* data, ...);
                             
gchar*      bastile_util_filename_for_objects (GList *objects);
                                             
gboolean    bastile_util_uri_exists        (const gchar* uri);

gchar*      bastile_util_uri_unique        (const gchar* uri);

gchar*      bastile_util_uri_replace_ext   (const gchar *uri, 
                                             const gchar *ext);

const gchar* bastile_util_uri_get_last     (const gchar* uri);

const gchar* bastile_util_uri_split_last   (gchar* uri);

gboolean    bastile_util_uris_package      (const gchar* package, 
                                             const gchar** uris);

GQuark      bastile_util_detect_mime_type   (const gchar *mime);

GQuark      bastile_util_detect_data_type   (const gchar *data,
                                              guint length);

GQuark      bastile_util_detect_file_type   (const gchar *uri);

gboolean    bastile_util_write_file_private            (const gchar* filename,
                                                         const gchar* contents,
                                                         GError **err);

GtkDialog*  bastile_util_chooser_open_new              (const gchar *title,
                                                         GtkWindow *parent);

GtkDialog*  bastile_util_chooser_save_new              (const gchar *title,
                                                         GtkWindow *parent);

void        bastile_util_chooser_show_key_files        (GtkDialog *dialog);

void        bastile_util_chooser_show_archive_files    (GtkDialog *dialog);

void        bastile_util_chooser_set_filename_full     (GtkDialog *dialog, 
                                                         GList *objects);

void        bastile_util_chooser_set_filename          (GtkDialog *dialog, 
                                                         struct _BastileObject *object);

gchar*      bastile_util_chooser_open_prompt           (GtkDialog *dialog);

gchar*      bastile_util_chooser_save_prompt           (GtkDialog *dialog);

gboolean	bastile_util_check_suffix		(const gchar		*path,
                                             BastileSuffix     suffix);

gchar*		bastile_util_add_suffix		(const gchar        *path,
                                             BastileSuffix     suffix,
                                             const gchar        *prompt);

gchar*      bastile_util_remove_suffix     (const gchar        *path,
                                             const gchar        *prompt);

gchar**     bastile_util_strvec_dup        (const gchar        **vec);

guint       bastile_util_strvec_length       (const gchar      **vec);

GList*       bastile_util_objects_sort       (GList *objects);

GList*       bastile_util_objects_splice     (GList *objects);

gboolean    bastile_util_string_equals       (const gchar *s1, const gchar *s2);

gchar*      bastile_util_string_up_first     (const gchar *orig);

void        bastile_util_string_lower        (gchar *s);

GSList*     bastile_util_string_slist_free   (GSList *slist);

GSList*     bastile_util_string_slist_copy   (GSList *slist);

gboolean    bastile_util_string_slist_equal  (GSList *sl1, GSList *sl2);

gboolean    bastile_util_string_is_whitespace (const gchar *text);

void        bastile_util_string_trim_whitespace (gchar *text);

gchar*      bastile_util_hex_encode (gconstpointer value, gsize length);

void        bastile_util_determine_popup_menu_position  (GtkMenu *menu,
                                                           int *x,
                                                           int *y,
                                                           gboolean *push_in,
                                                           gpointer gdata);

BastileVersion bastile_util_parse_version   (const char *version);

#define bastile_util_version(a,b,c,d) ((BastileVersion)a << 48) + ((BastileVersion)b << 32) \
                                     + ((BastileVersion)c << 16) +  (BastileVersion)d

#define     bastile_util_wait_until(expr)                \
    while (!(expr)) {                                     \
        while (g_main_context_pending(NULL) && !(expr))   \
            g_main_context_iteration (NULL, FALSE);       \
        g_thread_yield ();                                \
    }

#ifdef _DEBUG
#define DBG_PRINT(x) g_printerr x
#else
#define DBG_PRINT(x)
#endif

#endif /* __BASTILE_UTIL_H__ */
