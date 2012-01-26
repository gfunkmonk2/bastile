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

#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE 4096

int main (int argc, char* argv[])
{
    unsigned char buf[BUF_SIZE];
	const char* fname;
    size_t len;
    FILE* f;

    fname = getenv ("BASTILE_IMAGE_FILE");
    if (!fname) {
        fprintf (stderr, "bastile-xloadimage: specify output file in $BASTILE_IMAGE_FILE\n");
        exit (2);
    }

    f = fopen (fname, "wb");
    if (!f) {
        fprintf (stderr, "bastile-xloadimage: couldn't open output file: %s", fname);
        exit (1);
    }

    do {
        len = fread (buf, sizeof(unsigned char), BUF_SIZE, stdin);
    } while (fwrite (buf, sizeof(unsigned char), len, f) == BUF_SIZE);

    if (ferror (f)) {
        fprintf (stderr, "bastile-xloadimage: couldn't write to output file: %s", fname);
        exit (1);
    }
        
    fclose(f);
    return 0;
}
