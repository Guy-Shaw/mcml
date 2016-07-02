/*
 * Filename: filev-probe.c
 * Library: libcscript
 * Brief: Visit and check every file name in a filename vector
 *
 * Description:
 *   Check each filename in a given filename vector.
 *   Ensure that each file exists.
 *   Do not stop check after the first error.
 *   Rather, keep validating all of the given filenames,
 *   but prevent repeated error messages due to common problems.
 *   For example, do not complain twice about two files when
 *   the true cause of the problem is common to both files.
 *   For example, a directory which is an ancestor common
 *   for two files may not exist or may have permission problems.
 *   The error is reported only once for the common ancestor directory.
 *
 * Copyright (C) 2015-2016 Guy Shaw
 * Written by Guy Shaw <gshaw@acm.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cscript.h>
#include <unistd.h>     // Import F_OK

extern bool debug;

int
fname_probe(const char *fname)
{
    int rv;

    if (fname[0] == '-' && !fname[1]) {
        return (0);
    }

    rv = access(fname, F_OK);
    if (rv != 0) {
        eputchar('\'');
        fshow_fname(errprint_fh, fname);
        eputchar('\'');
        eprint(" does not exist.\n");
    }
    return (rv);
}

int
filev_probe(size_t filec, char **filev)
{
    int any_rv;
    uint_t i;

    any_rv = 0;
    for (i = 0; i < filec; ++i) {
        int rv;

        rv = fname_probe(filev[i]);
        if (rv != 0) {
            any_rv = rv;
        }
    }

    return (any_rv);
}
