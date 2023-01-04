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

/* Return true if BUF (of size SIZE) is all zeros.  */
static bool
all_zeros (char const *buf, idx_t size)
{
  for (char const *p = buf; p < buf + size; p++)
    if (*p)
      return false;
  return true;
}

/* Reset the buffer for a new file, returning false if we should skip it.
   Initialize on the first time through. */
static bool
reset (int fd, struct stat const *st)
{
  bufbeg = buflim = ALIGN_TO (buffer + 1, pagesize);
  bufbeg[-1] = eolbyte;
  bufdesc = fd;
  bufoffset = fd == STDIN_FILENO ? lseek (fd, 0, SEEK_CUR) : 0;
  seek_failed = bufoffset < 0;

  /* Assume SEEK_DATA fails if SEEK_CUR does.  */
  seek_data_failed = seek_failed;

  if (seek_failed)
    {
      if (errno != ESPIPE)
        {
          suppressible_error (errno);
          return false;
        }
      bufoffset = 0;
    }
  return true;
}

/* Read new stuff into the buffer, saving the specified
   amount of old stuff.  When we're done, 'bufbeg' points
   to the beginning of the buffer contents, and 'buflim'
   points just after the end.  Return false if there's an error.  */
static bool
fillbuf (idx_t save, struct stat const *st)
{
  char *readbuf;

  /* After BUFLIM, we need room for at least a page of data plus a
     trailing uword.  */
  idx_t min_after_buflim = pagesize + uword_size;

  if (min_after_buflim <= buffer + bufalloc - buflim)
    readbuf = buflim;
  else
    {
      char *newbuf;

      /* For data to be searched we need room for the saved bytes,
         plus at least a page of data to read.  */
      idx_t minsize = save + pagesize;

      /* Add enough room so that the buffer is aligned and has room
         for byte sentinels fore and aft, and so that a uword can
         be read aft.  */
      ptrdiff_t incr_min = minsize - bufalloc + min_after_buflim;

      if (incr_min <= 0)
        newbuf = buffer;
      else
        {
          /* Try not to allocate more memory than the file size indicates,
             as that might cause unnecessary memory exhaustion if the file
             is large.  However, do not use the original file size as a
             heuristic if we've already read past the file end, as most
             likely the file is growing.  */
          ptrdiff_t alloc_max = -1;
          if (usable_st_size (st))
            {
              off_t to_be_read = st->st_size - bufoffset;
              ptrdiff_t a;
              if (0 <= to_be_read
                  && !ckd_add (&a, to_be_read, save + min_after_buflim))
                alloc_max = MAX (a, bufalloc + incr_min);
            }

          newbuf = xpalloc (NULL, &bufalloc, incr_min, alloc_max, 1);
        }

      readbuf = ALIGN_TO (newbuf + 1 + save, pagesize);
      idx_t moved = save + 1;  /* Move the preceding byte sentinel too.  */
      memmove (readbuf - moved, buflim - moved, moved);
      if (0 < incr_min)
        {
          free (buffer);
          buffer = newbuf;
        }
    }

  bufbeg = readbuf - save;

  clear_asan_poison ();

  idx_t readsize = buffer + bufalloc - uword_size - readbuf;
  readsize -= readsize % pagesize;

  idx_t fillsize;
  bool cc = true;

  while (true)
    {
      fillsize = safe_read (bufdesc, readbuf, readsize);
      if (fillsize == SAFE_READ_ERROR)
        {
          fillsize = 0;
          cc = false;
        }
      bufoffset += fillsize;

      if (((fillsize == 0) | !skip_nuls) || !all_zeros (readbuf, fillsize))
        break;
      totalnl = add_count (totalnl, fillsize);

      if (SEEK_DATA != SEEK_SET && !seek_data_failed)
        {
          /* Solaris SEEK_DATA fails with errno == ENXIO in a hole at EOF.  */
          off_t data_start = lseek (bufdesc, bufoffset, SEEK_DATA);
          if (data_start < 0 && errno == ENXIO
              && usable_st_size (st) && bufoffset < st->st_size)
            data_start = lseek (bufdesc, 0, SEEK_END);

          if (data_start < 0)
            seek_data_failed = true;
          else
            {
              totalnl = add_count (totalnl, data_start - bufoffset);
              bufoffset = data_start;
            }
        }
    }

  buflim = readbuf + fillsize;

  /* Initialize the following word, because skip_easy_bytes and some
     matchers read (but do not use) those bytes.  This avoids false
     positive reports of these bytes being used uninitialized.  */
  memset (buflim, 0, uword_size);

  /* Mark the part of the buffer not filled by the read or set by
     the above memset call as ASAN-poisoned.  */
  asan_poison (buflim + uword_size, bufalloc - (buflim - buffer) - uword_size);

  return cc;
}

/* Flags controlling the style of output. */
static enum
{
  BINARY_BINARY_FILES,
  TEXT_BINARY_FILES,
  WITHOUT_MATCH_BINARY_FILES
} binary_files;		/* How to handle binary files.  */

/* Options for output as a list of matching/non-matching files */
static enum
{
  LISTFILES_NONE,
  LISTFILES_MATCHING,
  LISTFILES_NONMATCHING,
} list_files;
