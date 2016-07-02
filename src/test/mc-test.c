/*
 * Filename: src/test/mc-test.c
 * Project: mcml / libmc
 * Brief: Hard-coded test case for the libmc function, mc()
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

#include <stdbool.h>
    // Import type bool
    // Import constant false
    // Import constant true
#include <stdio.h>
    // Import type FILE
    // Import printf()
    // Import var stdout
#include <unistd.h>
    // Import type size_t

extern int mc(FILE *f, size_t nelem, const char **elemv, size_t llen, bool horizontal);

static const char *names[] = {
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "a-medium-length-line",
    "10",
    "20",
    "30",
    "40",
    "big-fucking-long-name",
    "another-long-name",
};


int
main()
{
    size_t nelem = sizeof (names) / sizeof (*names);

    printf("Test vertical.\n");
    mc(stdout, nelem, names, 80, false);

    printf("Test horizontal.\n");
    mc(stdout, nelem, names, 80, true);
    return (0);
}
