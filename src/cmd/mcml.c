/*
 * Filename: src/cmd/mcml.c
 * Project: mcml
 * Brief: Multi-Column Markup Language
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <ctype.h>
    // Import isprint()
#include <err.h>
    // Import err()
#include <errno.h>
    // Import var errno
#include <stdbool.h>
    // Import type bool
    // Import constant false
    // Import constant true
#include <stddef.h>
    // Import constant NULL
#include <stdio.h>
    // Import type FILE
    // Import fclose()
    // Import fopen()
    // Import fprintf()
    // Import fputc()
    // Import fputs()
    // Import snprintf()
    // Import var stdin
    // Import var stdout
#include <stdlib.h>
    // Import exit()
    // Import free()
#include <string.h>
    // Import strdup()
#include <unistd.h>
    // Import getopt_long()
    // Import optarg()
    // Import opterr()
    // Import optind()
    // Import optopt()
    // Import type size_t

#include <cscript.h>

extern int mc(FILE *f, size_t nelem, const char **elemv, size_t llen, size_t indent, bool horizontal);

const char *program_path;
const char *program_name;

size_t filec;               // Count of elements in filev
char **filev;               // Non-option elements of argv

bool verbose = false;
bool debug   = false;

static char *cmdpfx;
static size_t width      = 0;
static size_t indent     = 4;
static bool   horizontal = false;
static bool   each_file  = false;

FILE *errprint_fh = NULL;
FILE *dbgprint_fh = NULL;

static struct option long_options[] = {
    {"help",           no_argument,       0,  'h'},
    {"version",        no_argument,       0,  'V'},
    {"verbose",        no_argument,       0,  'v'},
    {"debug",          no_argument,       0,  'd'},
    {"each-file",      no_argument,       0,  'E'},
    {"horizontal",     no_argument,       0,  'H'},
    {"cmd",            required_argument, 0,  'c'},
    {"width",          required_argument, 0,  'w'},
    {"indent",         required_argument, 0,  'i'},
    {0, 0, 0, 0}
};

static const char usage_text[] =
    "Options:\n"
    "  --help|-h|-?         Show this help message and exit\n"
    "  --version            Show version information and exit\n"
    "  --verbose|-v         verbose\n"
    "  --debug|-d           debug\n"
    "  --each-file|-E       Break and print after each file\n"
    "  --horizontal|-H      Print elements along rows first\n"
    "  --cmd|c <str>        Command prefix\n"
    "  --width|w <n>        display width (AKA line length)\n"
    "  --indnt|i <n>        Indentation (number of spaces)\n"
    "\n"
    "The only command is \"category\".  A new category\n"
    "causes mcml to break, print all data collected so far,\n"
    "print the category header, then start fresh with the\n"
    "new category data.\n"
    "If no command prefix is specified, then all lines are\n"
    "just plain data.\n"
    ;

static const char version_text[] =
    "0.1\n"
    ;

static const char copyright_text[] =
    "Copyright (C) 2016 Guy Shaw\n"
    "Written by Guy Shaw\n"
    ;

static const char license_text[] =
    "License GPLv3+: GNU GPL version 3 or later"
    " <http://gnu.org/licenses/gpl.html>.\n"
    "This is free software: you are free to change and redistribute it.\n"
    "There is NO WARRANTY, to the extent permitted by law.\n"
    ;

static void
fshow_program_version(FILE *f)
{
    fputs(version_text, f);
    fputc('\n', f);
    fputs(copyright_text, f);
    fputc('\n', f);
    fputs(license_text, f);
    fputc('\n', f);
}

static void
show_program_version(void)
{
    fshow_program_version(stdout);
}

static void
usage(void)
{
    eprintf("usage: %s [ <options> ]\n", program_name);
    eprint(usage_text);
}

static inline bool
is_long_option(const char *s)
{
    return (s[0] == '-' && s[1] == '-');
}

static inline char *
vischar_r(char *buf, size_t sz, int c)
{
    if (isprint(c)) {
        buf[0] = c;
        buf[1] = '\0';
    }
    else {
        snprintf(buf, sz, "\\x%02x", c);
    }
    return (buf);
}

static char **mc_elemv;
static size_t mc_nelem;
static size_t mc_sz;

static void
mc_add_element(const char *elem)
{
    if (mc_sz == 0) {
        size_t bytes;

        mc_sz = 1024;
        bytes = mc_sz * sizeof (*mc_elemv);
        mc_elemv = (char **) guard_malloc(bytes);
        mc_nelem = 0;
    }

    if (mc_nelem >= mc_sz) {
        size_t bytes;

        mc_sz += 1024;
        bytes = mc_sz * sizeof (*mc_elemv);
        mc_elemv = (char **) guard_realloc(mc_elemv, bytes);
    }

    mc_elemv[mc_nelem] = strdup(elem);
    ++mc_nelem;
}

void
mc_reset(void)
{
    size_t i;

    for (i = 0; i < mc_nelem; ++i) {
        free(mc_elemv[i]);
    }
    mc_nelem = 0;
}

void
mc_flush(void)
{
    if (mc_nelem == 0) {
        return;
    }

    mc(stdout, mc_nelem, (const char **)mc_elemv, width, indent, horizontal);
    mc_reset();
}

// ################ linebuf

struct linebuf {
    FILE   *f;
    char   *buf;
    void   *sgl;
    size_t siz;
    size_t len;
    int    err;
    bool   eof;
};

typedef struct linebuf linebuf_t;

extern linebuf_t *linebuf_new(void);
extern void linebuf_init(linebuf_t *lbuf, FILE *f);
extern void linebuf_sgl_new(linebuf_t *lbuf);
extern void linebuf_free(linebuf_t *lbuf);
extern char *sgl_fgetline(linebuf_t *lbuf, int endl);

static void
fgetline(linebuf_t *lbuf)
{
    char *rbuf;

    // Free up any resources leftover from the last time
    // this line buffer was used.
    //
    // In particlur, free up any segments of a scatter/gather list
    // and the line buffer.

    linebuf_free(lbuf);

    linebuf_sgl_new(lbuf);

    rbuf = sgl_fgetline(lbuf, '\n');
    (void)rbuf;
}

/*
 * Is |pfx| a prefix of |s|?  Yes or no?
 * If |pfx| and |s| are equal strings, that counts as a yes.
 */

static bool
is_prefix(const char *pfx, const char *s)
{
    while (*pfx) {
        if (*pfx != *s) {
            return (false);
        }
        ++pfx;
        ++s;
    }
    return (true);
}

int
parse_mcml_command(const char *line, char **rcmd, char **rarg)
{
    static char *sv_cmd;
    static size_t sv_cmd_len;
    static size_t sv_cmd_chunk;
    static char *sv_arg;
    static size_t sv_arg_len;
    static size_t sv_arg_chunk;

    const char *cmd;
    const char *arg;
    const char *p;

    // Scan for command name, followed by whitespace or end of line

    p = line + strlen(cmdpfx);
    cmd = p;
    while (*p && !isspace(*p)) {
        ++p;
    }
    sv_cmd_len = p - cmd;
    sv_cmd_chunk = ((sv_cmd_len + 1 + 63) / 64) * 64;
    sv_cmd = (char *) realloc(sv_cmd, sv_cmd_chunk);
    memcpy(sv_cmd, cmd, sv_cmd_len);
    sv_cmd[sv_cmd_len] = '\0';

    // Scan for argument

    arg = p;
    if (*p) {
        while (*p && isspace(*p)) {
            ++p;
        }
        arg = p;
        while (*p) {
            ++p;
        }
    }
    sv_arg_len = p - arg;
    sv_arg_chunk = ((sv_arg_len + 1 + 63) / 64) * 64;
    sv_arg = (char *) realloc(sv_arg, sv_arg_chunk);
    memcpy(sv_arg, arg, sv_arg_len);
    sv_arg[sv_arg_len] = '\0';

    *rcmd = sv_cmd;
    *rarg = sv_arg;
    return (0);
}

int
mcml_stream(const char *fname, FILE *f)
{
    (void)fname;    // We will use later for error reporting.

    linebuf_t *lbuf;

    lbuf = linebuf_new();
    linebuf_init(lbuf, f);
    while (true) {
        fgetline(lbuf);
        if (lbuf->eof) {
            break;
        }
        if (lbuf->len == 0) {
            continue;
        }

        // fprintf(stderr, "line=[%s]\n", lbuf->buf);

        if (cmdpfx != NULL && is_prefix(cmdpfx, lbuf->buf)) {
            char *cmd;
            char *arg;
            parse_mcml_command(lbuf->buf, &cmd, &arg);

            // fprintf(stderr, "cmd=[%s]\n", cmd);
            // fprintf(stderr, "arg=[%s]\n", arg);

            if (strcmp(cmd, "category") == 0) {
                mc_flush();
                fprintf(stdout, "%s:\n", arg);
                continue;
            }
        }
        mc_add_element(lbuf->buf);
    }
    linebuf_free(lbuf);
    free(lbuf);

    if (each_file) {
        mc_flush();
    }

    return (0);
}

int
filev_mcml(void)
{
    size_t fnr;

    for (fnr = 0; fnr < filec; ++fnr) {
        FILE *f;
        int rv;
        int close_rv;

        f = fopen(filev[fnr], "r");
        if (f == NULL) {
            int err = errno;
            fprintf(errprint_fh, "fopen('%s', \"r\") failed\n", filev[fnr]);
            fprintf(errprint_fh, "  errno=%d\n", err);
            return (err);
        }

        rv = mcml_stream(filev[fnr], f);
        close_rv = fclose(f);
        if (rv) {
            fprintf(errprint_fh, "mcml_stream('%s') failed.\n", filev[fnr]);
            return (rv);
        }
        if (close_rv) {
            fprintf(errprint_fh, "fclose('%s') failed.\n", filev[fnr]);
            return (rv);
        }
    }

    if (!each_file) {
        mc_flush();
    }

    return (0);
}

int
main(int argc, char **argv)
{
    extern char *optarg;
    extern int optind, opterr, optopt;
    int option_index;
    int err_count;
    int optc;
    int rv;

    set_eprint_fh();
    program_path = *argv;
    program_name = sname(program_path);
    option_index = 0;
    err_count = 0;
    opterr = 0;

    while (true) {
        int this_option_optind;

        if (err_count > 10) {
            eprintf("%s: Too many option errors.\n", program_name);
            break;
        }

        this_option_optind = optind ? optind : 1;
        optc = getopt_long(argc, argv, "+hVdvEHw:", long_options, &option_index);
        if (optc == -1) {
            break;
        }

        rv = 0;
        if (optc == '?' && optopt == '?') {
            optc = 'h';
        }

        switch (optc) {
        case 'V':
            show_program_version();
            exit(0);
            break;
        case 'h':
            fputs(usage_text, stdout);
            exit(0);
            break;
        case 'd':
            debug = true;
            set_debug_fh(NULL);
            break;
        case 'v':
            verbose = true;
            break;
        case 'H':
            horizontal = true;
            break;
        case 'E':
            each_file = true;
            break;
        case 'w':
            rv = parse_cardinal(&width, optarg);
            break;
        case 'c':
            cmdpfx = optarg;
            break;
        case '?':
            eprint(program_name);
            eprint(": ");
            if (is_long_option(argv[this_option_optind])) {
                eprintf("unknown long option, '%s'\n",
                    argv[this_option_optind]);
            }
            else {
                char chrbuf[10];
                eprintf("unknown short option, '%s'\n",
                    vischar_r(chrbuf, sizeof (chrbuf), optopt));
            }
            ++err_count;
            break;
        default:
            eprintf("%s: INTERNAL ERROR: unknown option, '%c'\n",
                program_name, optopt);
            exit(2);
            break;
        }
    }

    verbose = verbose || debug;

    if (optind < argc) {
        filec = (size_t) (argc - optind);
        filev = argv + optind;
    }
    else {
        filec = 0;
        filev = NULL;
    }

    if (verbose) {
        fshow_str_array(errprint_fh, filec, filev);
    }

    if (verbose && optind < argc) {
        eprintf("non-option ARGV-elements:\n");
        while (optind < argc) {
            eprintf("    %s\n", argv[optind]);
            ++optind;
        }
    }

    if (err_count != 0) {
        usage();
        exit(1);
    }

    if (width == 0) {
        width = 80;
    }

    if (filec == 0) {
        each_file = true;
        rv = mcml_stream("-", stdin);
    }
    else {
        rv = filev_probe(filec, filev);
        if (rv != 0) {
            exit(rv);
        }

        rv = filev_mcml();
    }

    if (rv != 0) {
        exit(rv);
    }

    exit(0);
}
