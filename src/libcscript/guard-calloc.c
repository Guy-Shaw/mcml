/*
 * Filename: guard-calloc.c
 * Library: libcscript
 * Brief: Wrapper around calloc() that complains and dies
 *
 * Copyright (C) 2016 Guy Shaw
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

#include <errno.h>
    // Import var errno
#include <stddef.h>
    // Import constant NULL
#include <stdio.h>
    // Import fprintf()
#include <stdlib.h>
    // Import exit()
    // Import calloc()

extern FILE *errprint_fh;

extern void eexplain_err(int err);

/**
 * @brief Allocate memory using calloc(), any error is fatal.
 * @param nelem  IN  How many elements to allocate.
 * @param size   IN  The size of each element.
 * @return pointer to the allocated memory.
 *
 * guard_calloc() is a wrapper around calloc() that complains and dies
 * if there is any error.  It never returns a NULL pointer.
 *
 */
void *
guard_calloc(size_t nelem, size_t size)
{
    void *mem;
    int err;

    mem = calloc(nelem, size);
    if (mem != NULL) {
        return (mem);
    }
    err = errno;
    fprintf(errprint_fh, "calloc(%zu, %#zx) failed\n", nelem, size);
    eexplain_err(err);
    exit(ENOMEM);
}
