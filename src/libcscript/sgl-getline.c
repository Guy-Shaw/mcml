/*
 * Filename: sgl-getline.c
 * Library: libcscript
 * Brief: Read a single line of arbitrary size into a scatter/gather list.
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

#include <cscript.h>
#include <unistd.h>
#include <string.h>     // Import strdup()

#ifdef SGL_STRESS_TEST

#define SGL_SEGSIZE   4
#define SGL_PAGESIZE 16

#else

#define SGL_SEGSIZE    64
#define SGL_PAGESIZE 1024

#endif

struct sglsegment {
    char   *sbuf;
    size_t  slen;
};

typedef struct sglsegment sglsegment_t;

struct sgl {
    struct sgl    *sgl_next;
    sglsegment_t  sgl_segv[SGL_SEGSIZE];
};

typedef struct sgl sgl_t;

struct linebuf {
    FILE   *f;
    char   *buf;
    sgl_t  *sgl;
    size_t siz;
    size_t len;
    int    err;
    bool   eof;
};

typedef struct linebuf linebuf_t;

void
linebuf_init(linebuf_t *lbuf, FILE *f)
{
    lbuf->f = f;
    lbuf->buf = NULL;
    lbuf->siz = 0;
    lbuf->len = 0;
    lbuf->err = 0;
    lbuf->eof = false;
}

linebuf_t *
linebuf_new(void)
{
    return ((linebuf_t *)guard_calloc(1, sizeof (linebuf_t)));
}

// Allocate a new empty scatter/gather list.
//
static sgl_t *
sgl_new(void)
{
    sgl_t * sglp;

    sglp = (sgl_t *)guard_calloc(1, sizeof (sgl_t));
    return (sglp);
}

// Allocate a new scatter/gather list and associate it with a linebuf_t.
//
void
linebuf_sgl_new(linebuf_t *lbuf)
{
    sgl_t *sglp;

    if (lbuf->sgl != NULL) {
        abort();
    }

    sglp = sgl_new();
    lbuf->sgl = sglp;
}

// Prepare to append data to a scatter/gather list.
// Allocate a new page.
// Append a page to a scatter/gather list.
// If all segments in the linked list of segments are occupied,
// then allocate a new whole segment and append that to the linked
// list of segments.
//
// Append a new page to the last segment of the scatter/gather list.
//
static sglsegment_t *
sgl_append(sgl_t *sglp)
{
    sgl_t * new_sglp;
    sglsegment_t *sglv;
    unsigned int i;
    char *rbuf;

    if (sglp == NULL) {
        abort();
    }

    // Scan to the end of the linked list of sgl-segments.
    // If there are any available slots, they will be in the last segment.
    // If there are no vacancies, then a new sgl-segment will be appended
    // to the linked list of segments.
    while (sglp->sgl_next != NULL) {
        sglp = sglp->sgl_next;
    }

    // See if there is an empty slot in the last sgl segment
    // in the linked list of segments.
    sglv = &sglp->sgl_segv[0];
    for (i = 0; i < SGL_SEGSIZE; ++i) {
        if (sglv[i].sbuf == NULL) {
            rbuf = (char *)guard_malloc(SGL_PAGESIZE);
            sglv[i].sbuf = rbuf;
            return (&sglv[i]);
        }
    }

    new_sglp = (sgl_t *)guard_calloc(1, sizeof (sgl_t));
    sglp->sgl_next = new_sglp;
    rbuf = (char *)guard_malloc(SGL_PAGESIZE);
    sglv[0].sbuf = rbuf;
    return (&sglv[0]);
}


static void
sgl_str_build(char *rbuf, size_t sz, sgl_t *sglp)
{
    size_t rlen;

    rlen = 0;
    while (sglp != NULL) {
        sglsegment_t *sglv;
        unsigned int i;

        sglv = &sglp->sgl_segv[0];
        for (i = 0; i < SGL_SEGSIZE; ++i) {
            if (sglv[i].sbuf == NULL) {
                break;
            }
            if (rlen + sglv[i].slen > sz) {
                abort();
            }
            memcpy(rbuf + rlen, sglv[i].sbuf, sglv[i].slen);
            rlen += sglv[i].slen;
        }
        sglp = sglp->sgl_next;
    }

    rbuf[rlen] = '\0';
}

// XXX sgl_strlen() Is currently not used.

size_t
sgl_strlen(sgl_t *sglp)
{
    size_t llen;

    llen = 0;
    while (sglp != NULL) {
        sglsegment_t *sglv;
        unsigned int i;

        sglv = &sglp->sgl_segv[0];
        for (i = 0; i < SGL_SEGSIZE; ++i) {
            if (sglv[i].sbuf == NULL) {
                break;
            }
            llen += sglv[i].slen;
        }
        sglp = sglp->sgl_next;
    }

    return (llen);
}

// Free any pages contained in an sgl-segment.
//
static void
free_sgl_segment(sgl_t *sglp)
{
    sglsegment_t *sglv;
    unsigned int i;

    sglv = &sglp->sgl_segv[0];
    for (i = 0; i < SGL_SEGSIZE; ++i) {
        if (sglv[i].sbuf != NULL) {
            free(sglv[i].sbuf);
        }
    }
}

// Free an entire scatter/gather list.
// First, all pages in each sgl-segment; then the segment structure itself.
//
static void
free_sgl(sgl_t *sglp) {
    sgl_t *next_sglp;
 
    while (sglp != NULL) {
        next_sglp = sglp->sgl_next;
        free_sgl_segment(sglp);
        free(sglp);
        sglp = next_sglp;
    }
}

// Free all the dynamically allocated components of a linebuf_t,
// but not the linebuf_t struct itself.
//
// Also, set any pointers to freed memory to NULL.
//
void
linebuf_free(linebuf_t *lbuf)
{
    if (lbuf->buf != NULL) {
        free(lbuf->buf);
        lbuf->buf = NULL;
        lbuf->siz = 0;
        lbuf->len = 0;
    }

    if (lbuf->sgl != NULL) {
        free_sgl(lbuf->sgl);
        lbuf->sgl = NULL;
    }
}


// Read into a bounded buffer up to the given end-of-line character.
//
// That is, act pretty much like fgets(), work with any given
// end-of-line delimiter character -- not just '\n'.
//
// Note: This work _only_ for a single end-of-line character,
//       not for a multi-character sequence, let alone a regular expression.
//
// It is useful at least for reading "lines" ending in a nul-byte.
// so we can handle the output of utilities that generate list of file names,
// delimited by nul-byte endings.  Utilities like find, ls, xargs, sort,
// perl -0, etc.
//
static char *
fgets_endl(char *buf, size_t sz, FILE *f, int endl)
{
    size_t len;
    int chr;

    len = 0;
    while (len < sz - 1) {
        chr = fgetc(f);
        if (chr == EOF) {
            if (len == 0) {
                return (NULL);
            }
            else {
                if (buf[len - 1] != '\0') {
                    buf[len] = '\0';
                }
                return (buf);
            }
        }

        buf[len++] = chr;
        if (chr == endl && endl != '\0') {
            buf[len] = '\0';
            return (buf);
        }
    }

    buf[len] = '\0';
    return (buf);
}

// XXX Break into separate functions:
// XXX   One to read a line into a scatter/gather list,
// XXX   The other to convert a scatter/gather list into a single buffer.
//       sgl_finish()
char *
sgl_fgetline(linebuf_t *lbuf, int endl)
{
    char *rbuf;
    // XXX int err;
    sglsegment_t *sglv;
    size_t llen;
    size_t slen;

    dbg_printf("> %s\n", __FUNCTION__);

    // Free up any resources leftover from the last time
    // this line buffer was used.
    //
    // In particlur, free up any segments of a scatter/gather list
    // and the line buffer.
    //
    linebuf_free(lbuf);
    linebuf_sgl_new(lbuf);
    llen = 0;

    // Read one sgl-segment at a time.
    //
    while (true) {
        sglv = sgl_append(lbuf->sgl);
        if (endl == '\n') {
            rbuf = fgets(sglv->sbuf, SGL_PAGESIZE, lbuf->f);
        }
        else {
            rbuf = fgets_endl(sglv->sbuf, SGL_PAGESIZE, lbuf->f, endl);
        }
        if (rbuf == NULL) {
            lbuf->eof = feof(lbuf->f);
            break;
        }
        if (rbuf != sglv->sbuf) {
            abort();
        }
        slen = strlen(rbuf);
        if (slen > 0 && rbuf[slen - 1] == '\n') {
            // finished reading line
            --slen;
            rbuf[slen] = '\0';
            sglv->slen = slen;
            llen += slen;
            break;
        }
        sglv->slen = slen;
        llen += slen;
    }

    lbuf->buf = (char *)guard_malloc(llen + 1);
    sgl_str_build(lbuf->buf, llen + 1, lbuf->sgl);
    lbuf->len = llen;
    dbg_printf("< %s\n", __FUNCTION__);
    dbg_printf("    line: [%s]\n", lbuf->buf);
    dbg_printf("    len = %zu\n", lbuf->len);
    return (lbuf->buf);
}
