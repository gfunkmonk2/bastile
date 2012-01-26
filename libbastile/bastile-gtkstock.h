/**
 * @file gtkstock.h GTK+ Stock resources
 *
 * bastile
 *
 * Gaim is the legal property of its developers, whose names are too numerous
 * to list here.  Please refer to the COPYRIGHT file distributed with the gaim
 * source distribution.
 * 
 * Also (c) 2005 Adam Schreiber <sadam@clemson.edu> 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef _BASTILE_GTKSTOCK_H_
#define _BASTILE_GTKSTOCK_H_

#include <glib.h>

/* The default stock icons */
#define BASTILE_STOCK_KEY          "bastile-key"
#define BASTILE_STOCK_SECRET       "bastile-key-personal"
#define BASTILE_STOCK_KEY_SSH      "bastile-key-ssh"
#define BASTILE_STOCK_PERSON       "bastile-person"
#define BASTILE_STOCK_SIGN         "bastile-sign"
#define BASTILE_STOCK_SIGN_OK      "bastile-sign-ok"
#define BASTILE_STOCK_SIGN_BAD     "bastile-sign-bad"
#define BASTILE_STOCK_SIGN_UNKNOWN "bastile-sign-unknown"

#define BASTILE_THEMED_WEBBROWSER  "web-browser"
#define BASTILE_THEMED_FOLDER      "folder"

void    bastile_gtkstock_init          (void);

void    bastile_gtkstock_add_icons     (const gchar **icons);

#endif /* _BASTILE_GTKSTOCK_H_ */
