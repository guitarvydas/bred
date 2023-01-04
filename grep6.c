
/* Whether to output filenames.  1 means yes, 0 means no, and -1 means
   'grep -r PATTERN FILE' was used and it is not known yet whether
   FILE is a directory (which means yes) or not (which means no).  */
static int out_file;

static int filename_mask;	/* If zero, output nulls after filenames.  */
static bool out_quiet;		/* Suppress all normal output. */
static bool out_invert;		/* Print nonmatching stuff. */
static bool out_line;		/* Print line numbers. */
static bool out_byte;		/* Print byte offsets. */
static intmax_t out_before;	/* Lines of leading context. */
static intmax_t out_after;	/* Lines of trailing context. */
static bool count_matches;	/* Count matching lines.  */
static intmax_t max_count;	/* Max number of selected
                                   lines from an input file.  */
static bool line_buffered;	/* Use line buffering.  */
static char *label = NULL;      /* Fake filename for stdin */


/* Internal variables to keep track of byte count, context, etc. */
static intmax_t totalcc;	/* Total character count before bufbeg. */
static char const *lastnl;	/* Pointer after last newline counted. */
static char *lastout;		/* Pointer after last character output;
                                   NULL if no character has been output
                                   or if it's conceptually before bufbeg. */
static intmax_t outleft;	/* Maximum number of selected lines.  */
static intmax_t pending;	/* Pending lines of output.
                                   Always kept 0 if out_quiet is true.  */
static bool done_on_match;	/* Stop scanning file on first match.  */
static bool exit_on_match;	/* Exit on first match.  */
static bool dev_null_output;	/* Stdout is known to be /dev/null.  */
static bool binary;		/* Use binary rather than text I/O.  */

static void
nlscan (char const *lim)
{
  idx_t newlines = 0;
  for (char const *beg = lastnl; beg < lim; beg++)
    {
      beg = memchr (beg, eolbyte, lim - beg);
      if (!beg)
        break;
      newlines++;
    }
  totalnl = add_count (totalnl, newlines);
  lastnl = lim;
}

/* Print the current filename.  */
static void
print_filename (void)
{
  pr_sgr_start_if (filename_color);
  fputs_errno (input_filename ());
  pr_sgr_end_if (filename_color);
}

/* Print a character separator.  */
static void
print_sep (char sep)
{
  pr_sgr_start_if (sep_color);
  putchar_errno (sep);
  pr_sgr_end_if (sep_color);
}

/* Print a line number or a byte offset.  */
static void
print_offset (intmax_t pos, const char *color)
{
  pr_sgr_start_if (color);
  printf_errno ("%*"PRIdMAX, offset_width, pos);
  pr_sgr_end_if (color);
}

/* Print a whole line head (filename, line, byte).  The output data
   starts at BEG and contains LEN bytes; it is followed by at least
   uword_size bytes, the first of which may be temporarily modified.
   The output data comes from what is perhaps a larger input line that
   goes until LIM, where LIM[-1] is an end-of-line byte.  Use SEP as
   the separator on output.

   Return true unless the line was suppressed due to an encoding error.  */

static bool
print_line_head (char *beg, idx_t len, char const *lim, char sep)
{
  if (binary_files != TEXT_BINARY_FILES)
    {
      char ch = beg[len];
      bool encoding_errors = buf_has_encoding_errors (beg, len);
      beg[len] = ch;
      if (encoding_errors)
        {
          encoding_error_output = true;
          return false;
        }
    }

  if (out_file)
    {
      print_filename ();
      if (filename_mask)
        print_sep (sep);
      else
        putchar_errno (0);
    }

  if (out_line)
    {
      if (lastnl < lim)
        {
          nlscan (beg);
          totalnl = add_count (totalnl, 1);
          lastnl = lim;
        }
      print_offset (totalnl, line_num_color);
      print_sep (sep);
    }

  if (out_byte)
    {
      intmax_t pos = add_count (totalcc, beg - bufbeg);
      print_offset (pos, byte_num_color);
      print_sep (sep);
    }

  if (align_tabs && (out_file | out_line | out_byte) && len != 0)
    putchar_errno ('\t');

  return true;
}

static char *
print_line_middle (char *beg, char *lim,
                   const char *line_color, const char *match_color)
{
  idx_t match_size;
  ptrdiff_t match_offset;
  char *cur;
  char *mid = NULL;
  char *b;

  for (cur = beg;
       (cur < lim
        && 0 <= (match_offset = execute (compiled_pattern, beg, lim - beg,
                                         &match_size, cur)));
       cur = b + match_size)
    {
      b = beg + match_offset;

      /* Avoid matching the empty line at the end of the buffer. */
      if (b == lim)
        break;

      /* Avoid hanging on grep --color "" foo */
      if (match_size == 0)
        {
          /* Make minimal progress; there may be further non-empty matches.  */
          /* XXX - Could really advance by one whole multi-octet character.  */
          match_size = 1;
          if (!mid)
            mid = cur;
        }
      else
        {
          /* This function is called on a matching line only,
             but is it selected or rejected/context?  */
          if (only_matching)
            {
              char sep = out_invert ? SEP_CHAR_REJECTED : SEP_CHAR_SELECTED;
              if (! print_line_head (b, match_size, lim, sep))
                return NULL;
            }
          else
            {
              pr_sgr_start (line_color);
              if (mid)
                {
                  cur = mid;
                  mid = NULL;
                }
              fwrite_errno (cur, 1, b - cur);
            }

          pr_sgr_start_if (match_color);
          fwrite_errno (b, 1, match_size);
          pr_sgr_end_if (match_color);
          if (only_matching)
            putchar_errno (eolbyte);
        }
    }

  if (only_matching)
    cur = lim;
  else if (mid)
    cur = mid;

  return cur;
}
