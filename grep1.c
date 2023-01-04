/* grep.c - main driver file for grep.
   Copyright (C) 1992, 1997-2002, 2004-2022 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

/* Written July 1992 by Mike Haertel.  */

#include <config.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <wchar.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdckdint.h>
#include <stdint.h>
#include <stdio.h>
#include "system.h"

#include "argmatch.h"
#include "c-ctype.h"
#include "c-stack.h"
#include "closeout.h"
#include "colorize.h"
#include "die.h"
#include "error.h"
#include "exclude.h"
#include "exitfail.h"
#include "fcntl-safer.h"
#include "fts_.h"
#include "getopt.h"
#include "getprogname.h"
#include "grep.h"
#include "hash.h"
#include "intprops.h"
#include "propername.h"
#include "safe-read.h"
#include "search.h"
#include "c-strcase.h"
#include "version-etc.h"
#include "xalloc.h"
#include "xbinary-io.h"
#include "xstrtol.h"

enum { SEP_CHAR_SELECTED = ':' };
enum { SEP_CHAR_REJECTED = '-' };
static char const SEP_STR_GROUP[] = "--";

/* When stdout is connected to a regular file, save its stat
   information here, so that we can automatically skip it, thus
   avoiding a potential (racy) infinite loop.  */
static struct stat out_stat;

/* if non-zero, display usage information and exit */
static int show_help;

/* Print the version on standard output and exit.  */
static bool show_version;

/* Suppress diagnostics for nonexistent or unreadable files.  */
static bool suppress_errors;

/* If nonzero, use color markers.  */
static int color_option;

/* Show only the part of a line matching the expression. */
static bool only_matching;

/* If nonzero, make sure first content char in a line is on a tab stop. */
static bool align_tabs;

/* Print width of line numbers and byte offsets.  Nonzero if ALIGN_TABS.  */
static int offset_width;

/* An entry in the PATLOC array saying where patterns came from.  */
struct patloc
  {
    /* Line number of the pattern in PATTERN_ARRAY.  Line numbers
       start at 0, and each pattern is terminated by '\n'.  */
    idx_t lineno;

    /* Input location of the pattern.  The FILENAME "-" represents
       standard input, and "" represents the command line.  FILELINE is
       origin-1 for files and is irrelevant for the command line.  */
    char const *filename;
    idx_t fileline;
  };

/* The array of pattern locations.  The concatenation of all patterns
   is stored in a single array, KEYS.  Given the invocation
   'grep -f <(seq 5) -f <(seq 6) -f <(seq 3)', there will initially be
   28 bytes in KEYS.  After duplicate patterns are removed, KEYS
   will have 12 bytes and PATLOC will be {0,x,1}, {10,y,1}
   where x, y and z are just place-holders for shell-generated names
   since and z is omitted as it contains only duplicates.  Sometimes
   removing duplicates will grow PATLOC, since each run of
   removed patterns not at a file start or end requires another
   PATLOC entry for the first non-removed pattern.  */
static struct patloc *patloc;
static idx_t patlocs_allocated, patlocs_used;

/* Pointer to the array of patterns, each terminated by newline.  */
static char *pattern_array;

/* The number of unique patterns seen so far.  */
static idx_t n_patterns;

/* Hash table of patterns seen so far.  */
static Hash_table *pattern_table;

/* Hash and compare newline-terminated patterns for textual equality.
   Patterns are represented by origin-1 offsets into PATTERN_ARRAY,
   cast to void *.  The origin-1 is so that the first pattern offset
   does not appear to be a null pointer when cast to void *.  */
static size_t _GL_ATTRIBUTE_PURE
hash_pattern (void const *pat, size_t n_buckets)
{
  /* This uses the djb2 algorithm, except starting with a larger prime
     in place of djb2's 5381, if size_t is wide enough.  The primes
     are taken from the primeth recurrence sequence
     <https://oeis.org/A007097>.  h15, h32 and h64 are the largest
     sequence members that fit into 15, 32 and 64 bits, respectively.
     Since any H will do, hashing works correctly on oddball machines
     where size_t has some other width.  */
  uint_fast64_t h15 = 5381, h32 = 3657500101, h64 = 4123221751654370051;
  size_t h = h64 <= SIZE_MAX ? h64 : h32 <= SIZE_MAX ? h32 : h15;
  intptr_t pat_offset = (intptr_t) pat - 1;
  unsigned char const *s = (unsigned char const *) pattern_array + pat_offset;
  for ( ; *s != '\n'; s++)
    h = h * 33 ^ *s;
  return h % n_buckets;
}
static bool _GL_ATTRIBUTE_PURE
compare_patterns (void const *a, void const *b)
{
  intptr_t a_offset = (intptr_t) a - 1;
  intptr_t b_offset = (intptr_t) b - 1;
  char const *p = pattern_array + a_offset;
  char const *q = pattern_array + b_offset;
  for (; *p == *q; p++, q++)
    if (*p == '\n')
      return true;
  return false;
}

/* Update KEYS to remove duplicate patterns, and return the number of
   bytes in the resulting KEYS.  KEYS contains a sequence of patterns
   each terminated by '\n'.  The first DUPFREE_SIZE bytes are a
   sequence of patterns with no duplicates; SIZE is the total number
   of bytes in KEYS.  If some patterns past the first DUPFREE_SIZE
   bytes are not duplicates, update PATLOCS accordingly.  */
static idx_t
update_patterns (char *keys, idx_t dupfree_size, idx_t size,
                 char const *filename)
{
  char *dst = keys + dupfree_size;
  idx_t fileline = 1;
  int prev_inserted = 0;

  char const *srclim = keys + size;
  idx_t patsize;
  for (char const *src = keys + dupfree_size; src < srclim; src += patsize)
    {
      char const *patend = rawmemchr (src, '\n');
      patsize = patend + 1 - src;
      memmove (dst, src, patsize);

      intptr_t dst_offset_1 = dst - keys + 1;
      int inserted = hash_insert_if_absent (pattern_table,
                                            (void *) dst_offset_1, NULL);
      if (inserted)
        {
          if (inserted < 0)
            xalloc_die ();
          dst += patsize;

          /* Add a PATLOCS entry unless this input line is simply the
             next one in the same file.  */
          if (!prev_inserted)
            {
              if (patlocs_used == patlocs_allocated)
                patloc = xpalloc (patloc, &patlocs_allocated, 1, -1,
                                  sizeof *patloc);
              patloc[patlocs_used++]
                = (struct patloc) { .lineno = n_patterns,
                                    .filename = filename,
                                    .fileline = fileline };
            }
          n_patterns++;
        }

      prev_inserted = inserted;
      fileline++;
    }

  return dst - keys;
}

/* Map LINENO, the origin-0 line number of one of the input patterns,
   to the name of the file from which it came.  Return "-" if it was
   read from stdin, "" if it was specified on the command line.
   Set *NEW_LINENO to the origin-1 line number of PATTERN in the file,
   or to an unspecified value if PATTERN came from the command line.  */
char const * _GL_ATTRIBUTE_PURE
pattern_file_name (idx_t lineno, idx_t *new_lineno)
{
  idx_t i;
  for (i = 1; i < patlocs_used; i++)
    if (lineno < patloc[i].lineno)
      break;
  *new_lineno = lineno - patloc[i - 1].lineno + patloc[i - 1].fileline;
  return patloc[i - 1].filename;
}

#if HAVE_ASAN
/* Record the starting address and length of the sole poisoned region,
   so that we can unpoison it later, just before each following read.  */
static void const *poison_buf;
static idx_t poison_len;

static void
clear_asan_poison (void)
{
  if (poison_buf)
    __asan_unpoison_memory_region (poison_buf, poison_len);
}

static void
asan_poison (void const *addr, idx_t size)
{
  poison_buf = addr;
  poison_len = size;

  __asan_poison_memory_region (poison_buf, poison_len);
}
#else
static void clear_asan_poison (void) { }
static void asan_poison (void const volatile *addr, idx_t size) { }
#endif
