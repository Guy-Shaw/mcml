/*
 * Filename: guard-realloc.c
 * Library: libcscript
 * Brief: Wrapper around realloc() that complains and dies
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
    // Import malloc()

extern FILE *errprint_fh;

extern void eexplain_err(int err);

/**
 * @brief Allocate memory using realloc(), any error is fatal.
 * @param mem    IN  A region of memory previously allocated by libc *alloc()
 * @param size   IN  The size of the region of memory to allocate.
 * @return pointer to the allocated memory.
 *
 * guard_malloc() is a wrapper around malloc() that complains and dies
 * if there is any error.  It never returns a NULL pointer.
 *
 */
void *
guard_realloc(void *mem, size_t size)
{
    void *new_mem;
    int err;

    new_mem = realloc(mem, size);
    if (new_mem != NULL) {
        return (new_mem);
    }
    err = errno;
    fprintf(errprint_fh, "realloc(_, %#zx) failed\n", size);
    eexplain_err(err);
    exit(ENOMEM);
}
