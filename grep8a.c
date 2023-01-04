
/* Output the lines between BEG and LIM.  Deal with context.  */
static void
prtext (char *beg, char *lim)
{
  static bool used;	/* Avoid printing SEP_STR_GROUP before any output.  */
  char eol = eolbyte;

  if (!out_quiet && pending > 0)
    prpending (beg);

  char *p = beg;

  if (!out_quiet)
    {
      /* Deal with leading context.  */
      char const *bp = lastout ? lastout : bufbeg;
      intmax_t i;
      for (i = 0; i < out_before; ++i)
        if (p > bp)
          do
            --p;
          while (p[-1] != eol);

      /* Print the group separator unless the output is adjacent to
         the previous output in the file.  */
      if ((0 <= out_before || 0 <= out_after) && used
          && p != lastout && group_separator)
        {
          pr_sgr_start_if (sep_color);
          fputs_errno (group_separator);
          pr_sgr_end_if (sep_color);
          putchar_errno ('\n');
        }

      while (p < beg)
        {
          char *nl = rawmemchr (p, eol);
          nl++;
          prline (p, nl, SEP_CHAR_REJECTED);
          p = nl;
        }
    }

  intmax_t n;
  if (out_invert)
    {
      /* One or more lines are output.  */
      for (n = 0; p < lim && n < outleft; n++)
        {
          char *nl = rawmemchr (p, eol);
          nl++;
          if (!out_quiet)
            prline (p, nl, SEP_CHAR_SELECTED);
          p = nl;
        }
    }
  else
    {
      /* Just one line is output.  */
      if (!out_quiet)
        prline (beg, lim, SEP_CHAR_SELECTED);
      n = 1;
      p = lim;
    }

  after_last_match = bufoffset - (buflim - p);
  pending = out_quiet ? 0 : MAX (0, out_after);
  used = true;
  outleft -= n;
}

/* Replace all NUL bytes in buffer P (which ends at LIM) with EOL.
   This avoids running out of memory when binary input contains a long
   sequence of zeros, which would otherwise be considered to be part
   of a long line.  P[LIM] should be EOL.  */
static void
zap_nuls (char *p, char *lim, char eol)
{
  if (eol)
    while (true)
      {
        *lim = '\0';
        p += strlen (p);
        *lim = eol;
        if (p == lim)
          break;
        do
          *p++ = eol;
        while (!*p);
      }
}

/* Scan the specified portion of the buffer, matching lines (or
   between matching lines if OUT_INVERT is true).  Return a count of
   lines printed.  Replace all NUL bytes with NUL_ZAPPER as we go.  */
static intmax_t
grepbuf (char *beg, char const *lim)
{
  intmax_t outleft0 = outleft;
  char *endp;

  for (char *p = beg; p < lim; p = endp)
    {
      idx_t match_size;
      ptrdiff_t match_offset = execute (compiled_pattern, p, lim - p,
                                        &match_size, NULL);
      if (match_offset < 0)
        {
          if (!out_invert)
            break;
          match_offset = lim - p;
          match_size = 0;
        }
      char *b = p + match_offset;
      endp = b + match_size;
      /* Avoid matching the empty line at the end of the buffer. */
      if (!out_invert && b == lim)
        break;
      if (!out_invert || p < b)
        {
          char *prbeg = out_invert ? p : b;
          char *prend = out_invert ? b : endp;
          prtext (prbeg, prend);
          if (!outleft || done_on_match)
            {
              if (exit_on_match)
                exit (errseen ? exit_failure : EXIT_SUCCESS);
              break;
            }
        }
    }

  return outleft0 - outleft;
}

/* Search a given (non-directory) file.  Return a count of lines printed.
   Set *INEOF to true if end-of-file reached.  */
static intmax_t
grep (int fd, struct stat const *st, bool *ineof)
{
  intmax_t nlines, i;
  idx_t residue, save;
  char oldc;
  char *beg;
  char *lim;
  char eol = eolbyte;
  char nul_zapper = '\0';
  bool done_on_match_0 = done_on_match;
  bool out_quiet_0 = out_quiet;

  /* The value of NLINES when nulls were first deduced in the input;
     this is not necessarily the same as the number of matching lines
     before the first null.  -1 if no input nulls have been deduced.  */
  intmax_t nlines_first_null = -1;

  if (! reset (fd, st))
    return 0;

  totalcc = 0;
  lastout = 0;
  totalnl = 0;
  outleft = max_count;
  after_last_match = 0;
  pending = 0;
  skip_nuls = skip_empty_lines && !eol;
  encoding_error_output = false;

  nlines = 0;
  residue = 0;
  save = 0;

  if (! fillbuf (save, st))
    {
      suppressible_error (errno);
      return 0;
    }

  offset_width = 0;
  if (align_tabs)
    {
      /* Width is log of maximum number.  Line numbers are origin-1.  */
      intmax_t num = usable_st_size (st) ? st->st_size : INTMAX_MAX;
      num += out_line && num < INTMAX_MAX;
      do
        offset_width++;
      while ((num /= 10) != 0);
    }

  for (bool firsttime = true; ; firsttime = false)
    {
      if (nlines_first_null < 0 && eol && binary_files != TEXT_BINARY_FILES
          && (buf_has_nulls (bufbeg, buflim - bufbeg)
              || (firsttime && file_must_have_nulls (buflim - bufbeg, fd, st))))
        {
          if (binary_files == WITHOUT_MATCH_BINARY_FILES)
            return 0;
          if (!count_matches)
            done_on_match = out_quiet = true;
          nlines_first_null = nlines;
          nul_zapper = eol;
          skip_nuls = skip_empty_lines;
        }

      lastnl = bufbeg;
      if (lastout)
        lastout = bufbeg;

      beg = bufbeg + save;

      /* no more data to scan (eof) except for maybe a residue -> break */
      if (beg == buflim)
        {
          *ineof = true;
          break;
        }

      zap_nuls (beg, buflim, nul_zapper);

      /* Determine new residue (the length of an incomplete line at the end of
         the buffer, 0 means there is no incomplete last line).  */
      oldc = beg[-1];
      beg[-1] = eol;
      /* FIXME: use rawmemrchr if/when it exists, since we have ensured
         that this use of memrchr is guaranteed never to return NULL.  */
      lim = memrchr (beg - 1, eol, buflim - beg + 1);
      ++lim;
      beg[-1] = oldc;
      if (lim == beg)
        lim = beg - residue;
      beg -= residue;
      residue = buflim - lim;

      if (beg < lim)
        {
          if (outleft)
            nlines += grepbuf (beg, lim);
          if (pending)
            prpending (lim);
          if ((!outleft && !pending)
              || (done_on_match && MAX (0, nlines_first_null) < nlines))
            goto finish_grep;
        }

      /* The last OUT_BEFORE lines at the end of the buffer will be needed as
         leading context if there is a matching line at the begin of the
         next data. Make beg point to their begin.  */
      i = 0;
      beg = lim;
      while (i < out_before && beg > bufbeg && beg != lastout)
        {
          ++i;
          do
            --beg;
          while (beg[-1] != eol);
        }

      /* Detect whether leading context is adjacent to previous output.  */
      if (beg != lastout)
        lastout = 0;

      /* Handle some details and read more data to scan.  */
      save = residue + lim - beg;
      if (out_byte)
        totalcc = add_count (totalcc, buflim - bufbeg - save);
      if (out_line)
        nlscan (beg);
      if (! fillbuf (save, st))
        {
          suppressible_error (errno);
          goto finish_grep;
        }
    }
  if (residue)
    {
      *buflim++ = eol;
      if (outleft)
        nlines += grepbuf (bufbeg + save - residue, buflim);
      if (pending)
        prpending (buflim);
    }

 finish_grep:
  done_on_match = done_on_match_0;
  out_quiet = out_quiet_0;
  if (binary_files == BINARY_BINARY_FILES && !out_quiet
      && (encoding_error_output
          || (0 <= nlines_first_null && nlines_first_null < nlines)))
    error (0, 0, _("%s: binary file matches"), input_filename ());
  return nlines;
}

static bool
grepdirent (FTS *fts, FTSENT *ent, bool command_line)
{
  bool follow;
  command_line &= ent->fts_level == FTS_ROOTLEVEL;

  if (ent->fts_info == FTS_DP)
    return true;

  if (!command_line
      && skipped_file (ent->fts_name, false,
                       (ent->fts_info == FTS_D || ent->fts_info == FTS_DC
                        || ent->fts_info == FTS_DNR)))
    {
      fts_set (fts, ent, FTS_SKIP);
      return true;
    }

  filename = ent->fts_path;
  if (omit_dot_slash && filename[1])
    filename += 2;
  follow = (fts->fts_options & FTS_LOGICAL
            || (fts->fts_options & FTS_COMFOLLOW && command_line));

  switch (ent->fts_info)
    {
    case FTS_D:
      if (directories == RECURSE_DIRECTORIES)
        return true;
      fts_set (fts, ent, FTS_SKIP);
      break;

    case FTS_DC:
      if (!suppress_errors)
        error (0, 0, _("%s: warning: recursive directory loop"), filename);
      return true;

    case FTS_DNR:
    case FTS_ERR:
    case FTS_NS:
      suppressible_error (ent->fts_errno);
      return true;

    case FTS_DEFAULT:
    case FTS_NSOK:
      if (skip_devices (command_line))
        {
          struct stat *st = ent->fts_statp;
          struct stat st1;
          if (! st->st_mode)
            {
              /* The file type is not already known.  Get the file status
                 before opening, since opening might have side effects
                 on a device.  */
              int flag = follow ? 0 : AT_SYMLINK_NOFOLLOW;
              if (fstatat (fts->fts_cwd_fd, ent->fts_accpath, &st1, flag) != 0)
                {
                  suppressible_error (errno);
                  return true;
                }
              st = &st1;
            }
          if (is_device_mode (st->st_mode))
            return true;
        }
      break;

    case FTS_F:
    case FTS_SLNONE:
      break;

    case FTS_SL:
    case FTS_W:
      return true;

    default:
      abort ();
    }

  return grepfile (fts->fts_cwd_fd, ent->fts_accpath, follow, command_line);
}

/* True if errno is ERR after 'open ("symlink", ... O_NOFOLLOW ...)'.
   POSIX specifies ELOOP, but it's EMLINK on FreeBSD and EFTYPE on NetBSD.  */
static bool
open_symlink_nofollow_error (int err)
{
  if (err == ELOOP || err == EMLINK)
    return true;
#ifdef EFTYPE
  if (err == EFTYPE)
    return true;
#endif
  return false;
}

static bool
grepfile (int dirdesc, char const *name, bool follow, bool command_line)
{
  int oflag = (O_RDONLY | O_NOCTTY
               | (IGNORE_DUPLICATE_BRANCH_WARNING
                  (binary ? O_BINARY : 0))
               | (follow ? 0 : O_NOFOLLOW)
               | (skip_devices (command_line) ? O_NONBLOCK : 0));
  int desc = openat_safer (dirdesc, name, oflag);
  if (desc < 0)
    {
      if (follow || ! open_symlink_nofollow_error (errno))
        suppressible_error (errno);
      return true;
    }
  return grepdesc (desc, command_line);
}

/* Read all data from FD, with status ST.  Return true if successful,
   false (setting errno) otherwise.  */
static bool
drain_input (int fd, struct stat const *st)
{
  ssize_t nbytes;
  if (S_ISFIFO (st->st_mode) && dev_null_output)
    {
#ifdef SPLICE_F_MOVE
      /* Should be faster, since it need not copy data to user space.  */
      nbytes = splice (fd, NULL, STDOUT_FILENO, NULL,
                       INITIAL_BUFSIZE, SPLICE_F_MOVE);
      if (0 <= nbytes || errno != EINVAL)
        {
          while (0 < nbytes)
            nbytes = splice (fd, NULL, STDOUT_FILENO, NULL,
                             INITIAL_BUFSIZE, SPLICE_F_MOVE);
          return nbytes == 0;
        }
#endif
    }
  while ((nbytes = safe_read (fd, buffer, bufalloc)))
    if (nbytes == SAFE_READ_ERROR)
      return false;
  return true;
}

/* Finish reading from FD, with status ST and where end-of-file has
   been seen if INEOF.  Typically this is a no-op, but when reading
   from standard input this may adjust the file offset or drain a
   pipe.  */

static void
finalize_input (int fd, struct stat const *st, bool ineof)
{
  if (fd == STDIN_FILENO
      && (outleft
          ? (!ineof
             && (seek_failed
                 || (lseek (fd, 0, SEEK_END) < 0
                     /* Linux proc file system has EINVAL (Bug#25180).  */
                     && errno != EINVAL))
             && ! drain_input (fd, st))
          : (bufoffset != after_last_match && !seek_failed
             && lseek (fd, after_last_match, SEEK_SET) < 0)))
    suppressible_error (errno);
}

static bool
grepdesc (int desc, bool command_line)
{
  intmax_t count;
  bool status = true;
  bool ineof = false;
  struct stat st;

  /* Get the file status, possibly for the second time.  This catches
     a race condition if the directory entry changes after the
     directory entry is read and before the file is opened.  For
     example, normally DESC is a directory only at the top level, but
     there is an exception if some other process substitutes a
     directory for a non-directory while 'grep' is running.  */
  if (fstat (desc, &st) != 0)
    {
      suppressible_error (errno);
      goto closeout;
    }

  if (desc != STDIN_FILENO && skip_devices (command_line)
      && is_device_mode (st.st_mode))
    goto closeout;

  if (desc != STDIN_FILENO && command_line
      && skipped_file (filename, true, S_ISDIR (st.st_mode) != 0))
    goto closeout;

  /* Don't output file names if invoked as 'grep -r PATTERN NONDIRECTORY'.  */
  if (out_file < 0)
    out_file = !!S_ISDIR (st.st_mode);

  if (desc != STDIN_FILENO
      && directories == RECURSE_DIRECTORIES && S_ISDIR (st.st_mode))
    {
      /* Traverse the directory starting with its full name, because
         unfortunately fts provides no way to traverse the directory
         starting from its file descriptor.  */

      FTS *fts;
      FTSENT *ent;
      int opts = fts_options & ~(command_line ? 0 : FTS_COMFOLLOW);
      char *fts_arg[2];

      /* Close DESC now, to conserve file descriptors if the race
         condition occurs many times in a deep recursion.  */
      if (close (desc) != 0)
        suppressible_error (errno);

      fts_arg[0] = (char *) filename;
      fts_arg[1] = NULL;
      fts = fts_open (fts_arg, opts, NULL);

      if (!fts)
        xalloc_die ();
      while ((ent = fts_read (fts)))
        status &= grepdirent (fts, ent, command_line);
      if (errno)
        suppressible_error (errno);
      if (fts_close (fts) != 0)
        suppressible_error (errno);
      return status;
    }
  if (desc != STDIN_FILENO
      && ((directories == SKIP_DIRECTORIES && S_ISDIR (st.st_mode))
          || ((devices == SKIP_DEVICES
               || (devices == READ_COMMAND_LINE_DEVICES && !command_line))
              && is_device_mode (st.st_mode))))
    goto closeout;

  /* If there is a regular file on stdout and the current file refers
     to the same i-node, we have to report the problem and skip it.
     Otherwise when matching lines from some other input reach the
     disk before we open this file, we can end up reading and matching
     those lines and appending them to the file from which we're reading.
     Then we'd have what appears to be an infinite loop that'd terminate
     only upon filling the output file system or reaching a quota.
     However, there is no risk of an infinite loop if grep is generating
     no output, i.e., with --silent, --quiet, -q.
     Similarly, with any of these:
       --max-count=N (-m) (for N >= 2)
       --files-with-matches (-l)
       --files-without-match (-L)
     there is no risk of trouble.
     For --max-count=1, grep stops after printing the first match,
     so there is no risk of malfunction.  But even --max-count=2, with
     input==output, while there is no risk of infloop, there is a race
     condition that could result in "alternate" output.  */
  if (!out_quiet && list_files == LISTFILES_NONE && 1 < max_count
      && S_ISREG (st.st_mode) && SAME_INODE (st, out_stat))
    {
      if (! suppress_errors)
        error (0, 0, _("%s: input file is also the output"), input_filename ());
      errseen = true;
      goto closeout;
    }

  count = grep (desc, &st, &ineof);
  if (count_matches)
    {
      if (out_file)
        {
          print_filename ();
          if (filename_mask)
            print_sep (SEP_CHAR_SELECTED);
          else
            putchar_errno (0);
        }
      printf_errno ("%" PRIdMAX "\n", count);
      if (line_buffered)
        fflush_errno ();
    }

  status = !count;

  if (list_files == LISTFILES_NONE)
    finalize_input (desc, &st, ineof);
  else if (list_files == (status ? LISTFILES_NONMATCHING : LISTFILES_MATCHING))
    {
      print_filename ();
      putchar_errno ('\n' & filename_mask);
      if (line_buffered)
        fflush_errno ();
    }

 closeout:
  if (desc != STDIN_FILENO && close (desc) != 0)
    suppressible_error (errno);
  return status;
}

static bool
grep_command_line_arg (char const *arg)
{
  if (STREQ (arg, "-"))
    {
      filename = label;
      if (binary)
        xset_binary_mode (STDIN_FILENO, O_BINARY);
      return grepdesc (STDIN_FILENO, true);
    }
  else
    {
      filename = arg;
      return grepfile (AT_FDCWD, arg, true, true);
    }
}

_Noreturn void usage (int);
void
usage (int status)
{
  if (status != 0)
    {
      fprintf (stderr, _("Usage: %s [OPTION]... PATTERNS [FILE]...\n"),
               getprogname ());
      fprintf (stderr, _("Try '%s --help' for more information.\n"),
               getprogname ());
    }
  else
    {
      printf (_("Usage: %s [OPTION]... PATTERNS [FILE]...\n"), getprogname ());
      printf (_("Search for PATTERNS in each FILE.\n"));
      printf (_("\
Example: %s -i 'hello world' menu.h main.c\n\
PATTERNS can contain multiple patterns separated by newlines.\n\
\n\
Pattern selection and interpretation:\n"), getprogname ());
      printf (_("\
  -E, --extended-regexp     PATTERNS are extended regular expressions\n\
  -F, --fixed-strings       PATTERNS are strings\n\
  -G, --basic-regexp        PATTERNS are basic regular expressions\n\
  -P, --perl-regexp         PATTERNS are Perl regular expressions\n"));
  /* -X is deliberately undocumented.  */
      printf (_("\
  -e, --regexp=PATTERNS     use PATTERNS for matching\n\
  -f, --file=FILE           take PATTERNS from FILE\n\
  -i, --ignore-case         ignore case distinctions in patterns and data\n\
      --no-ignore-case      do not ignore case distinctions (default)\n\
  -w, --word-regexp         match only whole words\n\
  -x, --line-regexp         match only whole lines\n\
  -z, --null-data           a data line ends in 0 byte, not newline\n"));
      printf (_("\
\n\
Miscellaneous:\n\
  -s, --no-messages         suppress error messages\n\
  -v, --invert-match        select non-matching lines\n\
  -V, --version             display version information and exit\n\
      --help                display this help text and exit\n"));
      printf (_("\
\n\
Output control:\n\
  -m, --max-count=NUM       stop after NUM selected lines\n\
  -b, --byte-offset         print the byte offset with output lines\n\
  -n, --line-number         print line number with output lines\n\
      --line-buffered       flush output on every line\n\
  -H, --with-filename       print file name with output lines\n\
  -h, --no-filename         suppress the file name prefix on output\n\
      --label=LABEL         use LABEL as the standard input file name prefix\n\
"));
      printf (_("\
  -o, --only-matching       show only nonempty parts of lines that match\n\
  -q, --quiet, --silent     suppress all normal output\n\
      --binary-files=TYPE   assume that binary files are TYPE;\n\
                            TYPE is 'binary', 'text', or 'without-match'\n\
  -a, --text                equivalent to --binary-files=text\n\
"));
      printf (_("\
  -I                        equivalent to --binary-files=without-match\n\
  -d, --directories=ACTION  how to handle directories;\n\
                            ACTION is 'read', 'recurse', or 'skip'\n\
  -D, --devices=ACTION      how to handle devices, FIFOs and sockets;\n\
                            ACTION is 'read' or 'skip'\n\
  -r, --recursive           like --directories=recurse\n\
  -R, --dereference-recursive  likewise, but follow all symlinks\n\
"));
      printf (_("\
      --include=GLOB        search only files that match GLOB (a file pattern)"
                "\n\
      --exclude=GLOB        skip files that match GLOB\n\
      --exclude-from=FILE   skip files that match any file pattern from FILE\n\
      --exclude-dir=GLOB    skip directories that match GLOB\n\
"));
      printf (_("\
  -L, --files-without-match  print only names of FILEs with no selected lines\n\
  -l, --files-with-matches  print only names of FILEs with selected lines\n\
  -c, --count               print only a count of selected lines per FILE\n\
  -T, --initial-tab         make tabs line up (if needed)\n\
  -Z, --null                print 0 byte after FILE name\n"));
      printf (_("\
\n\
Context control:\n\
  -B, --before-context=NUM  print NUM lines of leading context\n\
  -A, --after-context=NUM   print NUM lines of trailing context\n\
  -C, --context=NUM         print NUM lines of output context\n\
"));
      printf (_("\
  -NUM                      same as --context=NUM\n\
      --group-separator=SEP  print SEP on line between matches with context\n\
      --no-group-separator  do not print separator for matches with context\n\
      --color[=WHEN],\n\
      --colour[=WHEN]       use markers to highlight the matching strings;\n\
                            WHEN is 'always', 'never', or 'auto'\n\
  -U, --binary              do not strip CR characters at EOL (MSDOS/Windows)\n\
\n"));
      printf (_("\
When FILE is '-', read standard input.  With no FILE, read '.' if\n\
recursive, '-' otherwise.  With fewer than two FILEs, assume -h.\n\
Exit status is 0 if any line is selected, 1 otherwise;\n\
if any error occurs and -q is not given, the exit status is 2.\n"));
      emit_bug_reporting_address ();
    }
  exit (status);
}

/* Pattern compilers and matchers.  */

static struct
{
  char name[12];
  int syntax; /* used if compile == GEAcompile */
  compile_fp_t compile;
  execute_fp_t execute;
} const matchers[] = {
  { "grep", RE_SYNTAX_GREP, GEAcompile, EGexecute },
  { "egrep", RE_SYNTAX_EGREP, GEAcompile, EGexecute },
  { "fgrep", 0, Fcompile, Fexecute, },
  { "awk", RE_SYNTAX_AWK, GEAcompile, EGexecute },
  { "gawk", RE_SYNTAX_GNU_AWK, GEAcompile, EGexecute },
  { "posixawk", RE_SYNTAX_POSIX_AWK, GEAcompile, EGexecute },
#if HAVE_LIBPCRE
  { "perl", 0, Pcompile, Pexecute, },
#endif
};
/* Keep these in sync with the 'matchers' table.  */
enum { E_MATCHER_INDEX = 1, F_MATCHER_INDEX = 2, G_MATCHER_INDEX = 0 };

/* Return the index of the matcher corresponding to M if available.
   MATCHER is the index of the previous matcher, or -1 if none.
   Exit in case of conflicts or if M is not available.  */
static int
setmatcher (char const *m, int matcher)
{
  for (int i = 0; i < sizeof matchers / sizeof *matchers; i++)
    if (STREQ (m, matchers[i].name))
      {
        if (0 <= matcher && matcher != i)
          die (EXIT_TROUBLE, 0, _("conflicting matchers specified"));
        return i;
      }

#if !HAVE_LIBPCRE
  if (STREQ (m, "perl"))
    die (EXIT_TROUBLE, 0,
         _("Perl matching not supported in a --disable-perl-regexp build"));
#endif
  die (EXIT_TROUBLE, 0, _("invalid matcher %s"), m);
}

