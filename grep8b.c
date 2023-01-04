
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

/* Get the next non-digit option from ARGC and ARGV.
   Return -1 if there are no more options.
   Process any digit options that were encountered on the way,
   and store the resulting integer into *DEFAULT_CONTEXT.  */
static int
get_nondigit_option (int argc, char *const *argv, intmax_t *default_context)
{
  static int prev_digit_optind = -1;
  int this_digit_optind;
  bool was_digit;
  char buf[INT_BUFSIZE_BOUND (intmax_t) + 4];
  char *p = buf;
  int opt;

  was_digit = false;
  this_digit_optind = optind;
  while (true)
    {
      opt = getopt_long (argc, (char **) argv, short_options,
                         long_options, NULL);
      if (! c_isdigit (opt))
        break;

      if (prev_digit_optind != this_digit_optind || !was_digit)
        {
          /* Reset to start another context length argument.  */
          p = buf;
        }
      else
        {
          /* Suppress trivial leading zeros, to avoid incorrect
             diagnostic on strings like 00000000000.  */
          p -= buf[0] == '0';
        }

      if (p == buf + sizeof buf - 4)
        {
          /* Too many digits.  Append "..." to make context_length_arg
             complain about "X...", where X contains the digits seen
             so far.  */
          strcpy (p, "...");
          p += 3;
          break;
        }
      *p++ = opt;

      was_digit = true;
      prev_digit_optind = this_digit_optind;
      this_digit_optind = optind;
    }
  if (p != buf)
    {
      *p = '\0';
      context_length_arg (buf, default_context);
    }

  return opt;
}

/* Parse GREP_COLORS.  The default would look like:
     GREP_COLORS='ms=01;31:mc=01;31:sl=:cx=:fn=35:ln=32:bn=32:se=36'
   with boolean capabilities (ne and rv) unset (i.e., omitted).
   No character escaping is needed or supported.  */
static void
parse_grep_colors (void)
{
  const char *p;
  char *q;
  char *name;
  char *val;

  p = getenv ("GREP_COLORS"); /* Plural! */
  if (p == NULL || *p == '\0')
    return;

  /* Work off a writable copy.  */
  q = xstrdup (p);

  name = q;
  val = NULL;
  /* From now on, be well-formed or you're gone.  */
  for (;;)
    if (*q == ':' || *q == '\0')
      {
        char c = *q;
        struct color_cap const *cap;

        *q++ = '\0'; /* Terminate name or val.  */
        /* Empty name without val (empty cap)
         * won't match and will be ignored.  */
        for (cap = color_dict; cap->name; cap++)
          if (STREQ (cap->name, name))
            break;
        /* If name unknown, go on for forward compatibility.  */
        if (cap->var && val)
          *(cap->var) = val;
        if (cap->fct)
          cap->fct ();
        if (c == '\0')
          return;
        name = q;
        val = NULL;
      }
    else if (*q == '=')
      {
        if (q == name || val)
          return;
        *q++ = '\0'; /* Terminate name.  */
        val = q; /* Can be the empty string.  */
      }
    else if (val == NULL)
      q++; /* Accumulate name.  */
    else if (*q == ';' || c_isdigit (*q))
      q++; /* Accumulate val.  Protect the terminal from being sent crap.  */
    else
      return;
}

/* Return true if PAT (of length PATLEN) contains an encoding error.  */
static bool
contains_encoding_error (char const *pat, idx_t patlen)
{
  mbstate_t mbs = { 0 };
  ptrdiff_t charlen;

  for (idx_t i = 0; i < patlen; i += charlen)
    {
      charlen = mb_clen (pat + i, patlen - i, &mbs);
      if (charlen < 0)
        return true;
    }
  return false;
}

/* When ignoring case and (-E or -F or -G), then for each single-byte
   character I, ok_fold[I] is 1 if every case folded counterpart of I
   is also single-byte, and is -1 otherwise.  */
static signed char ok_fold[NCHAR];
static void
setup_ok_fold (void)
{
  for (int i = 0; i < NCHAR; i++)
    {
      wint_t wi = localeinfo.sbctowc[i];
      if (wi == WEOF)
        continue;

      int ok = 1;
      wchar_t folded[CASE_FOLDED_BUFSIZE];
      for (int n = case_folded_counterparts (wi, folded); 0 <= --n; )
        {
          char buf[MB_LEN_MAX];
          mbstate_t s = { 0 };
          if (wcrtomb (buf, folded[n], &s) != 1)
            {
              ok = -1;
              break;
            }
        }
      ok_fold[i] = ok;
    }
}

/* Return the number of bytes in the initial character of PAT, of size
   PATLEN, if Fcompile can handle that character.  Return -1 if
   Fcompile cannot handle it.  MBS is the multibyte conversion state.
   PATLEN must be nonzero.  */

static ptrdiff_t
fgrep_icase_charlen (char const *pat, idx_t patlen, mbstate_t *mbs)
{
  unsigned char pat0 = pat[0];

  /* If PAT starts with a single-byte character, Fcompile works if
     every case folded counterpart is also single-byte.  */
  if (localeinfo.sbctowc[pat0] != WEOF)
    return ok_fold[pat0];

  wchar_t wc;
  size_t wn = mbrtowc (&wc, pat, patlen, mbs);

  /* If PAT starts with an encoding error, Fcompile does not work.  */
  if (MB_LEN_MAX < wn)
    return -1;

  /* PAT starts with a multibyte character.  Fcompile works if the
     character has no case folded counterparts and toupper translates
     none of its encoding's bytes.  */
  wchar_t folded[CASE_FOLDED_BUFSIZE];
  if (case_folded_counterparts (wc, folded))
    return -1;
  for (idx_t i = wn; 0 < --i; )
    {
      unsigned char c = pat[i];
      if (toupper (c) != c)
        return -1;
    }
  return wn;
}

/* Return true if the -F patterns PAT, of size PATLEN, contain only
   single-byte characters that case-fold only to single-byte
   characters, or multibyte characters not subject to case folding,
   and so can be processed by Fcompile.  */

static bool
fgrep_icase_available (char const *pat, idx_t patlen)
{
  mbstate_t mbs = {0,};

  for (idx_t i = 0; i < patlen; )
    {
      int n = fgrep_icase_charlen (pat + i, patlen - i, &mbs);
      if (n < 0)
        return false;
      i += n;
    }

  return true;
}

/* Change the pattern *KEYS_P, of size *LEN_P, from fgrep to grep style.  */

void
fgrep_to_grep_pattern (char **keys_p, idx_t *len_p)
{
  idx_t len = *len_p;
  char *keys = *keys_p;
  mbstate_t mb_state = { 0 };
  char *new_keys = xnmalloc (len + 1, 2);
  char *p = new_keys;

  for (ptrdiff_t n; len; keys += n, len -= n)
    {
      n = mb_clen (keys, len, &mb_state);
      switch (n)
        {
        case -2:
          n = len;
          FALLTHROUGH;
        default:
          p = mempcpy (p, keys, n);
          break;

        case -1:
          memset (&mb_state, 0, sizeof mb_state);
          n = 1;
          FALLTHROUGH;
        case 1:
          switch (*keys)
            {
            case '$': case '*': case '.': case '[': case '\\': case '^':
              *p++ = '\\'; break;
            }
          *p++ = *keys;
          break;
        }
    }

  *p = '\n';
  free (*keys_p);
  *keys_p = new_keys;
  *len_p = p - new_keys;
}

/* If it is easy, convert the MATCHER-style patterns KEYS (of size
   *LEN_P) to -F style, update *LEN_P to a possibly-smaller value, and
   return F_MATCHER_INDEX.  If not, leave KEYS and *LEN_P alone and
   return MATCHER.  This function is conservative and sometimes misses
   conversions, e.g., it does not convert the -E pattern "(a|a|[aa])"
   to the -F pattern "a".  */

static int
try_fgrep_pattern (int matcher, char *keys, idx_t *len_p)
{
  int result = matcher;
  idx_t len = *len_p;
  char *new_keys = ximalloc (len + 1);
  char *p = new_keys;
  char const *q = keys;
  mbstate_t mb_state = { 0 };

  while (len != 0)
    {
      switch (*q)
        {
        case '$': case '*': case '.': case '[': case '^':
          goto fail;

        case '(': case '+': case '?': case '{': case '|':
          /* There is no "case ')'" here, as "grep -E ')'" acts like
             "grep -E '\)'".  */
          if (matcher != G_MATCHER_INDEX)
            goto fail;
          break;

        case '\\':
          if (1 < len)
            switch (q[1])
              {
              case '\n':
              case 'B': case 'S': case 'W': case'\'': case '<':
              case 'b': case 's': case 'w': case '`': case '>':
              case '1': case '2': case '3': case '4':
              case '5': case '6': case '7': case '8': case '9':
                goto fail;

              case '(': case '+': case '?': case '{': case '|':
                /* Pass '\)' to GEAcompile so it can complain.  Otherwise,
                   "grep '\)'" would act like "grep ')'" while "grep '.*\)'
                   would be an error.  */
              case ')':
                if (matcher == G_MATCHER_INDEX)
                  goto fail;
                FALLTHROUGH;
              default:
                q++, len--;
                break;
              }
          break;
        }

      ptrdiff_t clen = (match_icase
                        ? fgrep_icase_charlen (q, len, &mb_state)
                        : mb_clen (q, len, &mb_state));
      if (clen < 0)
        goto fail;
      p = mempcpy (p, q, clen);
      q += clen;
      len -= clen;
    }

  if (*len_p != p - new_keys)
    {
      *len_p = p - new_keys;
      char *keys_end = mempcpy (keys, new_keys, p - new_keys);
      *keys_end = '\n';
    }
  result = F_MATCHER_INDEX;

 fail:
  free (new_keys);
  return result;
}

int
main (int argc, char **argv)
{
  char *keys = NULL;
  idx_t keycc = 0, keyalloc = 0;
  int matcher = -1;
  int opt;
  int prev_optind, last_recursive;
  intmax_t default_context;
  FILE *fp;
  exit_failure = EXIT_TROUBLE;
  initialize_main (&argc, &argv);

  /* Which command-line options have been specified for filename output.
     -1 for -h, 1 for -H, 0 for neither.  */
  int filename_option = 0;

  eolbyte = '\n';
  filename_mask = ~0;

  max_count = INTMAX_MAX;

  /* The value -1 means to use DEFAULT_CONTEXT. */
  out_after = out_before = -1;
  /* Default before/after context: changed by -C/-NUM options */
  default_context = -1;
  /* Changed by -o option */
  only_matching = false;

  /* Internationalization. */
#if defined HAVE_SETLOCALE
  setlocale (LC_ALL, "");
#endif
#if defined ENABLE_NLS
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
#endif

  init_localeinfo (&localeinfo);

  atexit (clean_up_stdout);
  c_stack_action (NULL);

  last_recursive = 0;

  pattern_table = hash_initialize (0, 0, hash_pattern, compare_patterns, 0);
  if (!pattern_table)
    xalloc_die ();

  while (prev_optind = optind,
         (opt = get_nondigit_option (argc, argv, &default_context)) != -1)
    switch (opt)
      {
      case 'A':
        context_length_arg (optarg, &out_after);
        break;

      case 'B':
        context_length_arg (optarg, &out_before);
        break;

      case 'C':
        /* Set output match context, but let any explicit leading or
           trailing amount specified with -A or -B stand. */
        context_length_arg (optarg, &default_context);
        break;

      case 'D':
        if (STREQ (optarg, "read"))
          devices = READ_DEVICES;
        else if (STREQ (optarg, "skip"))
          devices = SKIP_DEVICES;
        else
          die (EXIT_TROUBLE, 0, _("unknown devices method"));
        break;

      case 'E':
        matcher = setmatcher ("egrep", matcher);
        break;

      case 'F':
        matcher = setmatcher ("fgrep", matcher);
        break;

      case 'P':
        matcher = setmatcher ("perl", matcher);
        break;

      case 'G':
        matcher = setmatcher ("grep", matcher);
        break;

      case 'X': /* undocumented on purpose */
        matcher = setmatcher (optarg, matcher);
        break;

      case 'H':
        filename_option = 1;
        break;

      case 'I':
        binary_files = WITHOUT_MATCH_BINARY_FILES;
        break;

      case 'T':
        align_tabs = true;
        break;

      case 'U':
        if (O_BINARY)
          binary = true;
        break;

      case 'u':
        /* Obsolete option; it had no effect; FIXME: remove in 2023  */
        error (0, 0, _("warning: --unix-byte-offsets (-u) is obsolete"));
        break;

      case 'V':
        show_version = true;
        break;

      case 'a':
        binary_files = TEXT_BINARY_FILES;
        break;

      case 'b':
        out_byte = true;
        break;

      case 'c':
        count_matches = true;
        break;

      case 'd':
        directories = XARGMATCH ("--directories", optarg,
                                 directories_args, directories_types);
        if (directories == RECURSE_DIRECTORIES)
          last_recursive = prev_optind;
        break;

      case 'e':
        {
          idx_t cc = strlen (optarg);
          ptrdiff_t shortage = keycc - keyalloc + cc + 1;
          if (0 < shortage)
            pattern_array = keys = xpalloc (keys, &keyalloc, shortage, -1, 1);
          char *keyend = mempcpy (keys + keycc, optarg, cc);
          *keyend = '\n';
          keycc = update_patterns (keys, keycc, keycc + cc + 1, "");
        }
        break;

      case 'f':
        {
          if (STREQ (optarg, "-"))
            {
              if (binary)
                xset_binary_mode (STDIN_FILENO, O_BINARY);
              fp = stdin;
            }
          else
            {
              fp = fopen (optarg, binary ? "rb" : "r");
              if (!fp)
                die (EXIT_TROUBLE, errno, "%s", optarg);
            }
          idx_t newkeycc = keycc, cc;
          for (;; newkeycc += cc)
            {
              ptrdiff_t shortage = newkeycc - keyalloc + 2;
              if (0 < shortage)
                pattern_array = keys = xpalloc (keys, &keyalloc,
                                                shortage, -1, 1);
              cc = fread (keys + newkeycc, 1, keyalloc - (newkeycc + 1), fp);
              if (cc == 0)
                break;
            }
          int err = errno;
          if (!ferror (fp))
            {
              err = 0;
              if (fp == stdin)
                clearerr (fp);
              else if (fclose (fp) != 0)
                err = errno;
            }
          if (err)
            die (EXIT_TROUBLE, err, "%s", optarg);
          /* Append final newline if file ended in non-newline. */
          if (newkeycc != keycc && keys[newkeycc - 1] != '\n')
            keys[newkeycc++] = '\n';
          keycc = update_patterns (keys, keycc, newkeycc, optarg);
        }
        break;

      case 'h':
        filename_option = -1;
        break;

      case 'i':
      case 'y':			/* For old-timers . . . */
        match_icase = true;
        break;

      case NO_IGNORE_CASE_OPTION:
        match_icase = false;
        break;

      case 'L':
        /* Like -l, except list files that don't contain matches.
           Inspired by the same option in Hume's gre. */
        list_files = LISTFILES_NONMATCHING;
        break;

      case 'l':
        list_files = LISTFILES_MATCHING;
        break;

      case 'm':
        switch (xstrtoimax (optarg, 0, 10, &max_count, ""))
          {
          case LONGINT_OK:
          case LONGINT_OVERFLOW:
            break;

          default:
            die (EXIT_TROUBLE, 0, _("invalid max count"));
          }
        break;

      case 'n':
        out_line = true;
        break;

      case 'o':
        only_matching = true;
        break;

      case 'q':
        exit_on_match = true;
        exit_failure = 0;
        break;

      case 'R':
        fts_options = basic_fts_options | FTS_LOGICAL;
        FALLTHROUGH;
      case 'r':
        directories = RECURSE_DIRECTORIES;
        last_recursive = prev_optind;
        break;

      case 's':
        suppress_errors = true;
        break;

      case 'v':
        out_invert = true;
        break;

      case 'w':
        wordinit ();
        match_words = true;
        break;

      case 'x':
        match_lines = true;
        break;

      case 'Z':
        filename_mask = 0;
        break;

      case 'z':
        eolbyte = '\0';
        break;

      case BINARY_FILES_OPTION:
        if (STREQ (optarg, "binary"))
          binary_files = BINARY_BINARY_FILES;
        else if (STREQ (optarg, "text"))
          binary_files = TEXT_BINARY_FILES;
        else if (STREQ (optarg, "without-match"))
          binary_files = WITHOUT_MATCH_BINARY_FILES;
        else
          die (EXIT_TROUBLE, 0, _("unknown binary-files type"));
        break;

      case COLOR_OPTION:
        if (optarg)
          {
            if (!c_strcasecmp (optarg, "always")
                || !c_strcasecmp (optarg, "yes")
                || !c_strcasecmp (optarg, "force"))
              color_option = 1;
            else if (!c_strcasecmp (optarg, "never")
                     || !c_strcasecmp (optarg, "no")
                     || !c_strcasecmp (optarg, "none"))
              color_option = 0;
            else if (!c_strcasecmp (optarg, "auto")
                     || !c_strcasecmp (optarg, "tty")
                     || !c_strcasecmp (optarg, "if-tty"))
              color_option = 2;
            else
              show_help = 1;
          }
        else
          color_option = 2;
        break;

      case EXCLUDE_OPTION:
      case INCLUDE_OPTION:
        for (int cmd = 0; cmd < 2; cmd++)
          {
            if (!excluded_patterns[cmd])
              excluded_patterns[cmd] = new_exclude ();
            add_exclude (excluded_patterns[cmd], optarg,
                         ((opt == INCLUDE_OPTION ? EXCLUDE_INCLUDE : 0)
                          | exclude_options (cmd)));
          }
        break;
      case EXCLUDE_FROM_OPTION:
        for (int cmd = 0; cmd < 2; cmd++)
          {
            if (!excluded_patterns[cmd])
              excluded_patterns[cmd] = new_exclude ();
            if (add_exclude_file (add_exclude, excluded_patterns[cmd],
                                  optarg, exclude_options (cmd), '\n')
                != 0)
              die (EXIT_TROUBLE, errno, "%s", optarg);
          }
        break;

      case EXCLUDE_DIRECTORY_OPTION:
        strip_trailing_slashes (optarg);
        for (int cmd = 0; cmd < 2; cmd++)
          {
            if (!excluded_directory_patterns[cmd])
              excluded_directory_patterns[cmd] = new_exclude ();
            add_exclude (excluded_directory_patterns[cmd], optarg,
                         exclude_options (cmd));
          }
        break;

      case GROUP_SEPARATOR_OPTION:
        group_separator = optarg;
        break;

      case LINE_BUFFERED_OPTION:
        line_buffered = true;
        break;

      case LABEL_OPTION:
        label = optarg;
        break;

      case 0:
        /* long options */
        break;

      default:
        usage (EXIT_TROUBLE);
        break;

      }

  if (show_version)
    {
      version_etc (stdout, getprogname (), PACKAGE_NAME, VERSION,
                   (char *) NULL);
      puts (_("Written by Mike Haertel and others; see\n"
              "<https://git.sv.gnu.org/cgit/grep.git/tree/AUTHORS>."));
      return EXIT_SUCCESS;
    }

  if (show_help)
    usage (EXIT_SUCCESS);

  if (keys)
    {
      if (keycc == 0)
        {
          /* No keys were specified (e.g. -f /dev/null).  Match nothing.  */
          out_invert ^= true;
          match_lines = match_words = false;
          keys[keycc++] = '\n';
        }
    }
  else if (optind < argc)
    {
      /* If a command-line regular expression operand starts with '\-',
         skip the '\'.  This suppresses a stray-backslash warning if a
         script uses the non-POSIX "grep '\-x'" to avoid treating
         '-x' as an option.  */
      char const *pat = argv[optind++];
      bool skip_bs = (matcher != F_MATCHER_INDEX
                      && pat[0] == '\\' && pat[1] == '-');

      /* Make a copy so that it can be reallocated or freed later.  */
      pattern_array = keys = xstrdup (pat + skip_bs);
      idx_t patlen = strlen (keys);
      keys[patlen] = '\n';
      keycc = update_patterns (keys, 0, patlen + 1, "");
    }
  else
    usage (EXIT_TROUBLE);

  /* Strip trailing newline from keys.  */
  keycc--;

  hash_free (pattern_table);

  bool possibly_tty = false;
  struct stat tmp_stat;
  if (! exit_on_match && fstat (STDOUT_FILENO, &tmp_stat) == 0)
    {
      if (S_ISREG (tmp_stat.st_mode))
        out_stat = tmp_stat;
      else if (S_ISCHR (tmp_stat.st_mode))
        {
          struct stat null_stat;
          if (stat ("/dev/null", &null_stat) == 0
              && SAME_INODE (tmp_stat, null_stat))
            dev_null_output = true;
          else
            possibly_tty = true;
        }
    }

  /* POSIX says -c, -l and -q are mutually exclusive.  In this
     implementation, -q overrides -l and -L, which in turn override -c.  */
  if (exit_on_match | dev_null_output)
    list_files = LISTFILES_NONE;
  if ((exit_on_match | dev_null_output) || list_files != LISTFILES_NONE)
    {
      count_matches = false;
      done_on_match = true;
    }
  out_quiet = count_matches | done_on_match;

  if (out_after < 0)
    out_after = default_context;
  if (out_before < 0)
    out_before = default_context;

  /* If it is easy to see that matching cannot succeed (e.g., 'grep -f
     /dev/null'), fail without reading the input.  */
  if ((max_count == 0
       || (keycc == 0 && out_invert && !match_lines && !match_words))
      && list_files != LISTFILES_NONMATCHING)
    return EXIT_FAILURE;

  if (color_option == 2)
    color_option = possibly_tty && should_colorize () && isatty (STDOUT_FILENO);
  init_colorize ();

  if (color_option)
    {
      /* Legacy.  */
      char *userval = getenv ("GREP_COLOR");
      if (userval != NULL && *userval != '\0')
        for (char *q = userval; *q == ';' || c_isdigit (*q); q++)
          if (!q[1])
            {
              selected_match_color = context_match_color = userval;
              break;
            }

      /* New GREP_COLORS has priority.  */
      parse_grep_colors ();

      /* Warn if GREP_COLOR has an effect, since it's deprecated.  */
      if (selected_match_color == userval || context_match_color == userval)
        error (0, 0, _("warning: GREP_COLOR='%s' is deprecated;"
                       " use GREP_COLORS='mt=%s'"),
               userval, userval);
    }

  initialize_unibyte_mask ();

  if (matcher < 0)
    matcher = G_MATCHER_INDEX;

  if (matcher == F_MATCHER_INDEX
      || matcher == E_MATCHER_INDEX || matcher == G_MATCHER_INDEX)
    {
      if (match_icase)
        setup_ok_fold ();

      /* In a single-byte locale, switch from -F to -G if it is a single
         pattern that matches words, where -G is typically faster.  In a
         multibyte locale, switch if the patterns have an encoding error
         (where -F does not work) or if -i and the patterns will not work
         for -iF.  */
      if (matcher == F_MATCHER_INDEX)
        {
          if (! localeinfo.multibyte
              ? n_patterns == 1 && match_words
              : (contains_encoding_error (keys, keycc)
                 || (match_icase && !fgrep_icase_available (keys, keycc))))
            {
              fgrep_to_grep_pattern (&pattern_array, &keycc);
              keys = pattern_array;
              matcher = G_MATCHER_INDEX;
            }
        }
      /* With two or more patterns, if -F works then switch from either -E
         or -G, as -F is probably faster then.  */
      else if (1 < n_patterns)
        matcher = try_fgrep_pattern (matcher, keys, &keycc);
    }

  execute = matchers[matcher].execute;
  compiled_pattern =
    matchers[matcher].compile (keys, keycc, matchers[matcher].syntax,
                               only_matching | color_option);
  /* We need one byte prior and one after.  */
  char eolbytes[3] = { 0, eolbyte, 0 };
  idx_t match_size;
  skip_empty_lines = ((execute (compiled_pattern, eolbytes + 1, 1,
                                &match_size, NULL) == 0)
                      == out_invert);

  int num_operands = argc - optind;
  out_file = (filename_option == 0 && num_operands <= 1
              ? - (directories == RECURSE_DIRECTORIES)
              : 0 <= filename_option);

  if (binary)
    xset_binary_mode (STDOUT_FILENO, O_BINARY);

  /* Prefer sysconf for page size, as getpagesize typically returns int.  */
#ifdef _SC_PAGESIZE
  long psize = sysconf (_SC_PAGESIZE);
#else
  long psize = getpagesize ();
#endif
  if (! (0 < psize && psize <= (IDX_MAX - uword_size) / 2))
    abort ();
  pagesize = psize;
  bufalloc = ALIGN_TO (INITIAL_BUFSIZE, pagesize) + pagesize + uword_size;
  buffer = ximalloc (bufalloc);

  if (fts_options & FTS_LOGICAL && devices == READ_COMMAND_LINE_DEVICES)
    devices = READ_DEVICES;

  char *const *files;
  if (0 < num_operands)
    {
      files = argv + optind;
    }
  else if (directories == RECURSE_DIRECTORIES && 0 < last_recursive)
    {
      static char *const cwd_only[] = { (char *) ".", NULL };
      files = cwd_only;
      omit_dot_slash = true;
    }
  else
    {
      static char *const stdin_only[] = { (char *) "-", NULL };
      files = stdin_only;
    }

  bool status = true;
  do
    status &= grep_command_line_arg (*files++);
  while (*files != NULL);

  return errseen ? EXIT_TROUBLE : status;
}
