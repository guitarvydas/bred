
static void
color_cap_rv_fct (void)
{
  /* By this point, it was 1 (or already -1).  */
  color_option = -1;  /* That's still != 0.  */
}

static void
color_cap_ne_fct (void)
{
  sgr_start = "\33[%sm";
  sgr_end   = "\33[m";
}

/* For GREP_COLORS.  */
static const struct color_cap color_dict[] =
  {
    { "mt", &selected_match_color, color_cap_mt_fct }, /* both ms/mc */
    { "ms", &selected_match_color, NULL }, /* selected matched text */
    { "mc", &context_match_color,  NULL }, /* context matched text */
    { "fn", &filename_color,       NULL }, /* filename */
    { "ln", &line_num_color,       NULL }, /* line number */
    { "bn", &byte_num_color,       NULL }, /* byte (sic) offset */
    { "se", &sep_color,            NULL }, /* separator */
    { "sl", &selected_line_color,  NULL }, /* selected lines */
    { "cx", &context_line_color,   NULL }, /* context lines */
    { "rv", NULL,                  color_cap_rv_fct }, /* -v reverses sl/cx */
    { "ne", NULL,                  color_cap_ne_fct }, /* no EL on SGR_* */
    { NULL, NULL,                  NULL }
  };

/* Saved errno value from failed output functions on stdout.  */
static int stdout_errno;

static void
putchar_errno (int c)
{
  if (putchar (c) < 0)
    stdout_errno = errno;
}

static void
fputs_errno (char const *s)
{
  if (fputs (s, stdout) < 0)
    stdout_errno = errno;
}

static void _GL_ATTRIBUTE_FORMAT_PRINTF_STANDARD (1, 2)
printf_errno (char const *format, ...)
{
  va_list ap;
  va_start (ap, format);
  if (vfprintf (stdout, format, ap) < 0)
    stdout_errno = errno;
  va_end (ap);
}

static void
fwrite_errno (void const *ptr, idx_t size, idx_t nmemb)
{
  if (fwrite (ptr, size, nmemb, stdout) != nmemb)
    stdout_errno = errno;
}

static void
fflush_errno (void)
{
  if (fflush (stdout) != 0)
    stdout_errno = errno;
}

static struct exclude *excluded_patterns[2];
static struct exclude *excluded_directory_patterns[2];
/* Short options.  */
static char const short_options[] =
"0123456789A:B:C:D:EFGHIPTUVX:abcd:e:f:hiLlm:noqRrsuvwxyZz";

/* Non-boolean long options that have no corresponding short equivalents.  */
enum
{
  BINARY_FILES_OPTION = CHAR_MAX + 1,
  COLOR_OPTION,
  EXCLUDE_DIRECTORY_OPTION,
  EXCLUDE_OPTION,
  EXCLUDE_FROM_OPTION,
  GROUP_SEPARATOR_OPTION,
  INCLUDE_OPTION,
  LINE_BUFFERED_OPTION,
  LABEL_OPTION,
  NO_IGNORE_CASE_OPTION
};

/* Long options equivalences. */
static struct option const long_options[] =
{
  {"basic-regexp",    no_argument, NULL, 'G'},
  {"extended-regexp", no_argument, NULL, 'E'},
  {"fixed-regexp",    no_argument, NULL, 'F'},
  {"fixed-strings",   no_argument, NULL, 'F'},
  {"perl-regexp",     no_argument, NULL, 'P'},
  {"after-context", required_argument, NULL, 'A'},
  {"before-context", required_argument, NULL, 'B'},
  {"binary-files", required_argument, NULL, BINARY_FILES_OPTION},
  {"byte-offset", no_argument, NULL, 'b'},
  {"context", required_argument, NULL, 'C'},
  {"color", optional_argument, NULL, COLOR_OPTION},
  {"colour", optional_argument, NULL, COLOR_OPTION},
  {"count", no_argument, NULL, 'c'},
  {"devices", required_argument, NULL, 'D'},
  {"directories", required_argument, NULL, 'd'},
  {"exclude", required_argument, NULL, EXCLUDE_OPTION},
  {"exclude-from", required_argument, NULL, EXCLUDE_FROM_OPTION},
  {"exclude-dir", required_argument, NULL, EXCLUDE_DIRECTORY_OPTION},
  {"file", required_argument, NULL, 'f'},
  {"files-with-matches", no_argument, NULL, 'l'},
  {"files-without-match", no_argument, NULL, 'L'},
  {"group-separator", required_argument, NULL, GROUP_SEPARATOR_OPTION},
  {"help", no_argument, &show_help, 1},
  {"include", required_argument, NULL, INCLUDE_OPTION},
  {"ignore-case", no_argument, NULL, 'i'},
  {"no-ignore-case", no_argument, NULL, NO_IGNORE_CASE_OPTION},
  {"initial-tab", no_argument, NULL, 'T'},
  {"label", required_argument, NULL, LABEL_OPTION},
  {"line-buffered", no_argument, NULL, LINE_BUFFERED_OPTION},
  {"line-number", no_argument, NULL, 'n'},
  {"line-regexp", no_argument, NULL, 'x'},
  {"max-count", required_argument, NULL, 'm'},

  {"no-filename", no_argument, NULL, 'h'},
  {"no-group-separator", no_argument, NULL, GROUP_SEPARATOR_OPTION},
  {"no-messages", no_argument, NULL, 's'},
  {"null", no_argument, NULL, 'Z'},
  {"null-data", no_argument, NULL, 'z'},
  {"only-matching", no_argument, NULL, 'o'},
  {"quiet", no_argument, NULL, 'q'},
  {"recursive", no_argument, NULL, 'r'},
  {"dereference-recursive", no_argument, NULL, 'R'},
  {"regexp", required_argument, NULL, 'e'},
  {"invert-match", no_argument, NULL, 'v'},
  {"silent", no_argument, NULL, 'q'},
  {"text", no_argument, NULL, 'a'},
  {"binary", no_argument, NULL, 'U'},
  {"unix-byte-offsets", no_argument, NULL, 'u'},
  {"version", no_argument, NULL, 'V'},
  {"with-filename", no_argument, NULL, 'H'},
  {"word-regexp", no_argument, NULL, 'w'},
  {0, 0, 0, 0}
};

/* Define flags declared in grep.h. */
bool match_icase;
bool match_words;
bool match_lines;
char eolbyte;

/* For error messages. */
/* The input file name, or (if standard input) null or a --label argument.  */
static char const *filename;
/* Omit leading "./" from file names in diagnostics.  */
static bool omit_dot_slash;
static bool errseen;

/* True if output from the current input file has been suppressed
   because an output line had an encoding error.  */
static bool encoding_error_output;

enum directories_type
  {
    READ_DIRECTORIES = 2,
    RECURSE_DIRECTORIES,
    SKIP_DIRECTORIES
  };

/* How to handle directories.  */
static char const *const directories_args[] =
{
  "read", "recurse", "skip", NULL
};
static enum directories_type const directories_types[] =
{
  READ_DIRECTORIES, RECURSE_DIRECTORIES, SKIP_DIRECTORIES
};
ARGMATCH_VERIFY (directories_args, directories_types);

static enum directories_type directories = READ_DIRECTORIES;

enum { basic_fts_options = FTS_CWDFD | FTS_NOSTAT | FTS_TIGHT_CYCLE_CHECK };
static int fts_options = basic_fts_options | FTS_COMFOLLOW | FTS_PHYSICAL;

/* How to handle devices. */
static enum
  {
    READ_COMMAND_LINE_DEVICES,
    READ_DEVICES,
    SKIP_DEVICES
  } devices = READ_COMMAND_LINE_DEVICES;

static bool grepfile (int, char const *, bool, bool);
static bool grepdesc (int, bool);

static bool
is_device_mode (mode_t m)
{
  return S_ISCHR (m) || S_ISBLK (m) || S_ISSOCK (m) || S_ISFIFO (m);
}

static bool
skip_devices (bool command_line)
{
  return (devices == SKIP_DEVICES
          || ((devices == READ_COMMAND_LINE_DEVICES) & !command_line));
}

/* Return if ST->st_size is defined.  Assume the file is not a
   symbolic link.  */
static bool
usable_st_size (struct stat const *st)
{
  return S_ISREG (st->st_mode) || S_TYPEISSHM (st) || S_TYPEISTMO (st);
}

/* Lame substitutes for SEEK_DATA and SEEK_HOLE on platforms lacking them.
   Do not rely on these finding data or holes if they equal SEEK_SET.  */
#ifndef SEEK_DATA
enum { SEEK_DATA = SEEK_SET };
#endif
#ifndef SEEK_HOLE
enum { SEEK_HOLE = SEEK_SET };
#endif

/* True if lseek with SEEK_CUR or SEEK_DATA failed on the current input.  */
static bool seek_failed;
static bool seek_data_failed;

/* Functions we'll use to search. */
typedef void *(*compile_fp_t) (char *, idx_t, reg_syntax_t, bool);
typedef ptrdiff_t (*execute_fp_t) (void *, char const *, idx_t, idx_t *,
                                   char const *);
static execute_fp_t execute;
static void *compiled_pattern;

char const *
input_filename (void)
{
  if (!filename)
    filename = _("(standard input)");
  return filename;
}

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
