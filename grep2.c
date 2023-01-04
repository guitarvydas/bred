
/* The group separator used when context is requested. */
static const char *group_separator = SEP_STR_GROUP;

/* The context and logic for choosing default --color screen attributes
   (foreground and background colors, etc.) are the following.
      -- There are eight basic colors available, each with its own
         nominal luminosity to the human eye and foreground/background
         codes (black [0 %, 30/40], blue [11 %, 34/44], red [30 %, 31/41],
         magenta [41 %, 35/45], green [59 %, 32/42], cyan [70 %, 36/46],
         yellow [89 %, 33/43], and white [100 %, 37/47]).
      -- Sometimes, white as a background is actually implemented using
         a shade of light gray, so that a foreground white can be visible
         on top of it (but most often not).
      -- Sometimes, black as a foreground is actually implemented using
         a shade of dark gray, so that it can be visible on top of a
         background black (but most often not).
      -- Sometimes, more colors are available, as extensions.
      -- Other attributes can be selected/deselected (bold [1/22],
         underline [4/24], standout/inverse [7/27], blink [5/25], and
         invisible/hidden [8/28]).  They are sometimes implemented by
         using colors instead of what their names imply; e.g., bold is
         often achieved by using brighter colors.  In practice, only bold
         is really available to us, underline sometimes being mapped by
         the terminal to some strange color choice, and standout best
         being left for use by downstream programs such as less(1).
      -- We cannot assume that any of the extensions or special features
         are available for the purpose of choosing defaults for everyone.
      -- The most prevalent default terminal backgrounds are pure black
         and pure white, and are not necessarily the same shades of
         those as if they were selected explicitly with SGR sequences.
         Some terminals use dark or light pictures as default background,
         but those are covered over by an explicit selection of background
         color with an SGR sequence; their users will appreciate their
         background pictures not be covered like this, if possible.
      -- Some uses of colors attributes is to make some output items
         more understated (e.g., context lines); this cannot be achieved
         by changing the background color.
      -- For these reasons, the grep color defaults should strive not
         to change the background color from its default, unless it's
         for a short item that should be highlighted, not understated.
      -- The grep foreground color defaults (without an explicitly set
         background) should provide enough contrast to be readable on any
         terminal with either a black (dark) or white (light) background.
         This only leaves red, magenta, green, and cyan (and their bold
         counterparts) and possibly bold blue.  */
/* The color strings used for matched text.
   The user can overwrite them using the GREP_COLORS environment variable.  */
static const char *selected_match_color = "01;31";	/* bold red */
static const char *context_match_color  = "01;31";	/* bold red */

/* Other colors.  Defaults look damn good.  */
static const char *filename_color = "35";	/* magenta */
static const char *line_num_color = "32";	/* green */
static const char *byte_num_color = "32";	/* green */
static const char *sep_color      = "36";	/* cyan */
static const char *selected_line_color = "";	/* default color pair */
static const char *context_line_color  = "";	/* default color pair */

/* Select Graphic Rendition (SGR, "\33[...m") strings.  */
/* Also Erase in Line (EL) to Right ("\33[K") by default.  */
/*    Why have EL to Right after SGR?
         -- The behavior of line-wrapping when at the bottom of the
            terminal screen and at the end of the current line is often
            such that a new line is introduced, entirely cleared with
            the current background color which may be different from the
            default one (see the boolean back_color_erase terminfo(5)
            capability), thus scrolling the display by one line.
            The end of this new line will stay in this background color
            even after reverting to the default background color with
            "\33[m', unless it is explicitly cleared again with "\33[K"
            (which is the behavior the user would instinctively expect
            from the whole thing).  There may be some unavoidable
            background-color flicker at the end of this new line because
            of this (when timing with the monitor's redraw is just right).
         -- The behavior of HT (tab, "\t") is usually the same as that of
            Cursor Forward Tabulation (CHT) with a default parameter
            of 1 ("\33[I"), i.e., it performs pure movement to the next
            tab stop, without any clearing of either content or screen
            attributes (including background color); try
               printf 'asdfqwerzxcv\rASDF\tZXCV\n'
            in a bash(1) shell to demonstrate this.  This is not what the
            user would instinctively expect of HT (but is ok for CHT).
            The instinctive behavior would include clearing the terminal
            cells that are skipped over by HT with blank cells in the
            current screen attributes, including background color;
            the boolean dest_tabs_magic_smso terminfo(5) capability
            indicates this saner behavior for HT, but only some rare
            terminals have it (although it also indicates a special
            glitch with standout mode in the Teleray terminal for which
            it was initially introduced).  The remedy is to add "\33K"
            after each SGR sequence, be it START (to fix the behavior
            of any HT after that before another SGR) or END (to fix the
            behavior of an HT in default background color that would
            follow a line-wrapping at the bottom of the screen in another
            background color, and to complement doing it after START).
            Piping grep's output through a pager such as less(1) avoids
            any HT problems since the pager performs tab expansion.

      Generic disadvantages of this remedy are:
         -- Some very rare terminals might support SGR but not EL (nobody
            will use "grep --color" on a terminal that does not support
            SGR in the first place).
         -- Having these extra control sequences might somewhat complicate
            the task of any program trying to parse "grep --color"
            output in order to extract structuring information from it.
      A specific disadvantage to doing it after SGR START is:
         -- Even more possible background color flicker (when timing
            with the monitor's redraw is just right), even when not at the
            bottom of the screen.
      There are no additional disadvantages specific to doing it after
      SGR END.

      It would be impractical for GNU grep to become a full-fledged
      terminal program linked against ncurses or the like, so it will
      not detect terminfo(5) capabilities.  */
static const char *sgr_start = "\33[%sm\33[K";
static const char *sgr_end   = "\33[m\33[K";

/* SGR utility functions.  */
static void
pr_sgr_start (char const *s)
{
  if (*s)
    print_start_colorize (sgr_start, s);
}
static void
pr_sgr_end (char const *s)
{
  if (*s)
    print_end_colorize (sgr_end);
}
static void
pr_sgr_start_if (char const *s)
{
  if (color_option)
    pr_sgr_start (s);
}
static void
pr_sgr_end_if (char const *s)
{
  if (color_option)
    pr_sgr_end (s);
}

struct color_cap
  {
    const char *name;
    const char **var;
    void (*fct) (void);
  };

static void
color_cap_mt_fct (void)
{
  /* Our caller just set selected_match_color.  */
  context_match_color = selected_match_color;
}
