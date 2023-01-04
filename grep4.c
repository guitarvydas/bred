
/* Unless requested, diagnose an error about the input file.  */
static void
suppressible_error (int errnum)
{
  if (! suppress_errors)
    error (0, errnum, "%s", input_filename ());
  errseen = true;
}

/* If there has already been a write error, don't bother closing
   standard output, as that might elicit a duplicate diagnostic.  */
static void
clean_up_stdout (void)
{
  if (! stdout_errno)
    close_stdout ();
}

/* A cast to TYPE of VAL.  Use this when TYPE is a pointer type, VAL
   is properly aligned for TYPE, and 'gcc -Wcast-align' cannot infer
   the alignment and would otherwise complain about the cast.  */
#if 4 < __GNUC__ + (6 <= __GNUC_MINOR__)
# define CAST_ALIGNED(type, val)                           \
    ({ __typeof__ (val) val_ = val;                        \
       _Pragma ("GCC diagnostic push")                     \
       _Pragma ("GCC diagnostic ignored \"-Wcast-align\"") \
       (type) val_;                                        \
       _Pragma ("GCC diagnostic pop")                      \
    })
#else
# define CAST_ALIGNED(type, val) ((type) (val))
#endif

/* An unsigned type suitable for fast matching.  */
typedef uintmax_t uword;
static uword const uword_max = UINTMAX_MAX;
enum { uword_size = sizeof (uword) }; /* For when a signed size is wanted.  */

struct localeinfo localeinfo;

/* A mask to test for unibyte characters, with the pattern repeated to
   fill a uword.  For a multibyte character encoding where
   all bytes are unibyte characters, this is 0.  For UTF-8, this is
   0x808080....  For encodings where unibyte characters have no discerned
   pattern, this is all 1s.  The unsigned char C is a unibyte
   character if C & UNIBYTE_MASK is zero.  If the uword W is the
   concatenation of bytes, the bytes are all unibyte characters
   if W & UNIBYTE_MASK is zero.  */
static uword unibyte_mask;

static void
initialize_unibyte_mask (void)
{
  /* For each encoding error I that MASK does not already match,
     accumulate I's most significant 1 bit by ORing it into MASK.
     Although any 1 bit of I could be used, in practice high-order
     bits work better.  */
  unsigned char mask = 0;
  int ms1b = 1;
  for (int i = 1; i <= UCHAR_MAX; i++)
    if ((localeinfo.sbclen[i] != 1) & ! (mask & i))
      {
        while (ms1b * 2 <= i)
          ms1b *= 2;
        mask |= ms1b;
      }

  /* Now MASK will detect any encoding-error byte, although it may
     cry wolf and it may not be optimal.  Build a uword-length mask by
     repeating MASK.  */
  unibyte_mask = uword_max / UCHAR_MAX * mask;
}

/* Skip the easy bytes in a buffer that is guaranteed to have a sentinel
   that is not easy, and return a pointer to the first non-easy byte.
   The easy bytes all have UNIBYTE_MASK off.  */
static char const * _GL_ATTRIBUTE_PURE
skip_easy_bytes (char const *buf)
{
  /* Search a byte at a time until the pointer is aligned, then a
     uword at a time until a match is found, then a byte at a time to
     identify the exact byte.  The uword search may go slightly past
     the buffer end, but that's benign.  */
  char const *p;
  uword const *s;
  for (p = buf; (uintptr_t) p % uword_size != 0; p++)
    if (to_uchar (*p) & unibyte_mask)
      return p;
  for (s = CAST_ALIGNED (uword const *, p); ! (*s & unibyte_mask); s++)
    continue;
  for (p = (char const *) s; ! (to_uchar (*p) & unibyte_mask); p++)
    continue;
  return p;
}

/* Return true if BUF, of size SIZE, has an encoding error.
   BUF must be followed by at least uword_size bytes,
   the first of which may be modified.  */
static bool
buf_has_encoding_errors (char *buf, idx_t size)
{
  if (! unibyte_mask)
    return false;

  mbstate_t mbs = { 0 };
  ptrdiff_t clen;

  buf[size] = -1;
  for (char const *p = buf; (p = skip_easy_bytes (p)) < buf + size; p += clen)
    {
      clen = imbrlen (p, buf + size - p, &mbs);
      if (clen < 0)
        return true;
    }

  return false;
}


/* Return true if BUF, of size SIZE, has a null byte.
   BUF must be followed by at least one byte,
   which may be arbitrarily written to or read from.  */
static bool
buf_has_nulls (char *buf, idx_t size)
{
  buf[size] = 0;
  return strlen (buf) != size;
}

/* Return true if a file is known to contain null bytes.
   SIZE bytes have already been read from the file
   with descriptor FD and status ST.  */
static bool
file_must_have_nulls (idx_t size, int fd, struct stat const *st)
{
  /* If the file has holes, it must contain a null byte somewhere.  */
  if (SEEK_HOLE != SEEK_SET && !seek_failed
      && usable_st_size (st) && size < st->st_size)
    {
      off_t cur = size;
      if (O_BINARY || fd == STDIN_FILENO)
        {
          cur = lseek (fd, 0, SEEK_CUR);
          if (cur < 0)
            return false;
        }

      /* Look for a hole after the current location.  */
      off_t hole_start = lseek (fd, cur, SEEK_HOLE);
      if (0 <= hole_start)
        {
          if (lseek (fd, cur, SEEK_SET) < 0)
            suppressible_error (errno);
          if (hole_start < st->st_size)
            return true;
        }
    }

  return false;
}

/* Convert STR to a nonnegative integer, storing the result in *OUT.
   STR must be a valid context length argument; report an error if it
   isn't.  Silently ceiling *OUT at the maximum value, as that is
   practically equivalent to infinity for grep's purposes.  */
static void
context_length_arg (char const *str, intmax_t *out)
{
  switch (xstrtoimax (str, 0, 10, out, ""))
    {
    case LONGINT_OK:
    case LONGINT_OVERFLOW:
      if (0 <= *out)
        break;
      FALLTHROUGH;
    default:
      die (EXIT_TROUBLE, 0, "%s: %s", str,
           _("invalid context length argument"));
    }
}

/* Return the add_exclude options suitable for excluding a file name.
   If COMMAND_LINE, it is a command-line file name.  */
static int
exclude_options (bool command_line)
{
  return EXCLUDE_WILDCARDS | (command_line ? 0 : EXCLUDE_ANCHORED);
}

/* Return true if the file with NAME should be skipped.
   If COMMAND_LINE, it is a command-line argument.
   If IS_DIR, it is a directory.  */
static bool
skipped_file (char const *name, bool command_line, bool is_dir)
{
  struct exclude **pats;
  if (! is_dir)
    pats = excluded_patterns;
  else if (directories == SKIP_DIRECTORIES)
    return true;
  else if (command_line && omit_dot_slash)
    return false;
  else
    pats = excluded_directory_patterns;
  return pats[command_line] && excluded_file_name (pats[command_line], name);
}

/* Hairy buffering mechanism for grep.  The intent is to keep
   all reads aligned on a page boundary and multiples of the
   page size, unless a read yields a partial page.  */

static char *buffer;		/* Base of buffer. */
static idx_t bufalloc;		/* Allocated buffer size, counting slop. */
static int bufdesc;		/* File descriptor. */
static char *bufbeg;		/* Beginning of user-visible stuff. */
static char *buflim;		/* Limit of user-visible stuff. */
static idx_t pagesize;		/* alignment of memory pages */
static off_t bufoffset;		/* Read offset.  */
static off_t after_last_match;	/* Pointer after last matching line that
                                   would have been output if we were
                                   outputting characters. */
static bool skip_nuls;		/* Skip '\0' in data.  */
static bool skip_empty_lines;	/* Skip empty lines in data.  */
static intmax_t totalnl;	/* Total newline count before lastnl. */

/* Initial buffer size, not counting slop. */
enum { INITIAL_BUFSIZE = 96 * 1024 };

/* Return VAL aligned to the next multiple of ALIGNMENT.  VAL can be
   an integer or a pointer.  Both args must be free of side effects.  */
#define ALIGN_TO(val, alignment) \
  ((uintptr_t) (val) % (alignment) == 0 \
   ? (val) \
   : (val) + ((alignment) - (uintptr_t) (val) % (alignment)))

/* Add two numbers that count input bytes or lines, and report an
   error if the addition overflows.  */
static intmax_t
add_count (intmax_t a, idx_t b)
{
  intmax_t sum;
  if (ckd_add (&sum, a, b))
    die (EXIT_TROUBLE, 0, _("input is too large to count"));
  return sum;
}
