#ifndef BASTILESSHDIALOGS_H_
#define BASTILESSHDIALOGS_H_

#include <gtk/gtk.h>

#include "bastile-ssh-key.h"
#include "bastile-ssh-source.h"

void        bastile_ssh_upload_prompt         (GList *keys,
                                                GtkWindow *parent);

void        bastile_ssh_key_properties_show   (BastileSSHKey *skey,
                                                GtkWindow *parent);

void        bastile_ssh_generate_show         (BastileSSHSource *sksrc,
                                                GtkWindow *parent);

void        bastile_ssh_generate_register     (void);

#endif /*BASTILESSHDIALOGS_H_*/
