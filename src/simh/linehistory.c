/*
 * vim: filetype=c:tabstop=4:ai:expandtab
 * SPDX-License-Identifier: BSD-2-Clause
 * scspell-id: 676f2224-f62a-11ec-baf3-80ee73e9b8e7
 *
 * -------------------------------------------------------------------------
 *
 * linehistory.c
 *
 * linehistory is forked from linenoise; the original version
 * is available from https://github.com/antirez/linenoise
 *
 * ------------------------------------------------------------------------
 *
 * Copyright (c) 2010-2014 Salvatore Sanfilippo <antirez@gmail.com>
 * Copyright (c) 2010-2013 Pieter Noordhuis <pcnoordhuis@gmail.com>
 * Copyright (c) 2020-2021 Jeffrey H. Johnson <trnsz@pobox.com>
 * Copyright (c) 2021-2024 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * -------------------------------------------------------------------------
 */

#if !defined(_POSIX_C_SOURCE)
# define _POSIX_C_SOURCE 200809L
#endif /* if !defined(_POSIX_C_SOURCE) */

#if !defined ( __MINGW32__ ) && !defined ( CROSS_MINGW32 ) && !defined ( CROSS_MINGW64 ) && !defined ( __MINGW64__ ) && !defined ( _MSC_VER ) && !defined ( _MSC_BUILD )

# if defined( __sun ) && defined( __SVR4 )
#  if !defined(__EXTENSIONS__)
#   define __EXTENSIONS__ 1
#  endif /* if !defined(__EXTENSIONS__) */
# endif /* if defined( __sun ) && defined( __SVR4 ) */
# include <termios.h>
# if defined( __sun ) && defined( __SVR4 )
#  include <sys/termiox.h>
# endif /* if defined( __sun ) && defined( __SVR4 ) */
# include "linehistory.h"
# if !defined(__NetBSD__)
#  include "../dps8/dps8.h"
# endif /* if !defined(__NetBSD__) */
# include <ctype.h>
# include <errno.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <strings.h>
# include <sys/ioctl.h>
# include <sys/stat.h>
# include <sys/types.h>
# include <unistd.h>

# undef FREE
# define FREE(p) do  \
  {                  \
    free((p));       \
    (p) = NULL;      \
  } while(0)

# if defined(TESTING)
#  undef realloc
#  undef FREE
#  define FREE(p) free(p)
#  define realloc trealloc
# endif /* if defined(TESTING) */

# define LINENOISE_DEFAULT_HISTORY_MAX_LEN  100
# define LINENOISE_MAX_LINE                4096

static char *unsupported_term[] = {
  "dumb", "cons25", "emacs", NULL
};

# if defined(LH_COMPLETION)
static linenoiseCompletionCallback *completionCallback = NULL;
# endif /* if defined(LH_COMPLETION) */

# if defined(LH_HINTS)
static linenoiseHintsCallback *hintsCallback           = NULL;
static linenoiseFreeHintsCallback *freeHintsCallback   = NULL;
# endif /* if defined(LH_HINTS) */

static struct termios orig_termios; /* In order to restore at exit.*/

# if defined(LH_MASKMODE)
static int maskmode          = 0;   /* Show "**" instead of input for passwords */
# endif /* if defined(LH_MASKMODE) */

static int rawmode           = 0;
static int mlmode            = 0;   /* Multi line mode. Default is single line. */
static int atexit_registered = 0;   /* Register atexit just 1 time.             */
static int history_max_len   = LINENOISE_DEFAULT_HISTORY_MAX_LEN;
static int history_len       = 0;
static char **history        = NULL;

/*
 * The linenoiseState structure represents the state during line editing.
 * We pass this state to functions implementing specific editing
 * functionalities.
 */

struct linenoiseState
{
  int    ifd;            /* Terminal stdin file descriptor.                  */
  int    ofd;            /* Terminal stdout file descriptor.                 */
  char   *buf;           /* Edited line buffer.                              */
  size_t buflen;         /* Edited line buffer size.                         */
  const  char *prompt;   /* Prompt to display.                               */
  size_t plen;           /* Prompt length.                                   */
  size_t pos;            /* Current cursor position.                         */
  size_t oldpos;         /* Previous refresh cursor position.                */
  size_t len;            /* Current edited line length.                      */
  size_t cols;           /* Number of columns in terminal.                   */
  size_t maxrows;        /* Maximum num of rows used so far (multiline mode) */
  int    history_index;  /* The history index we are currently editing.      */
};

enum KEY_ACTION
{
  KEY_NULL  = 0,  /* NULL      */
  CTRL_A    = 1,  /* Ctrl-A    */
  CTRL_B    = 2,  /* Ctrl-B    */
  CTRL_C    = 3,  /* Ctrl-C    */
  CTRL_D    = 4,  /* Ctrl-D    */
  CTRL_E    = 5,  /* Ctrl-E    */
  CTRL_F    = 6,  /* Ctrl-F    */
  CTRL_H    = 8,  /* Ctrl-H    */
  TAB       = 9,  /* Tab       */
  CTRL_K    = 11, /* Ctrl-K    */
  CTRL_L    = 12, /* Ctrl-L    */
  ENTER     = 13, /* Enter     */
  CTRL_N    = 14, /* Ctrl-N    */
  CTRL_P    = 16, /* Ctrl-P    */
  CTRL_T    = 20, /* Ctrl-T    */
  CTRL_U    = 21, /* Ctrl-U    */
  CTRL_W    = 23, /* Ctrl-W    */
  ESC       = 27, /* Escape    */
  BACKSPACE = 127 /* Backspace */
};

static void linenoiseAtExit(void);
static void refreshLine(struct linenoiseState *l);
size_t pstrlen(const char *s);

# if defined(LH_MASKMODE)

/*
 * Enable "mask mode". When it is enabled, instead of the input that
 * the user is typing, the terminal will just display a corresponding
 * number of asterisks, like "****". This is useful for passwords and other
 * secrets that should not be displayed.
 */

void
linenoiseMaskModeEnable(void)
{
  maskmode = 1;
}

/* Disable mask mode. */
void
linenoiseMaskModeDisable(void)
{
  maskmode = 0;
}
# endif /* if defined(LH_MASKMODE) */

/* Set if to use or not the multi line mode. */
void
linenoiseSetMultiLine(int ml)
{
  mlmode = ml;
}

/*
 * Return true if the terminal name is in the list of terminals we know are
 * not able to understand basic escape sequences.
 */

static int
isUnsupportedTerm(void)
{
  char *term = getenv("TERM");
  int j;

  if (term == NULL)
  {
    return ( 0 );
  }

  for (j = 0; unsupported_term[j]; j++)
  {
    if (!strcasecmp(term, unsupported_term[j]))
    {
      return ( 1 );
    }
  }

  return ( 0 );
}

/* Raw mode */
static int
enableRawMode(int fd)
{
  struct termios raw;

  if (!isatty(STDIN_FILENO))
  {
    goto fatal;
  }

  if (!atexit_registered)
  {
    atexit(linenoiseAtExit);
    atexit_registered = 1;
  }

  if (tcgetattr(fd, &orig_termios) == -1)
  {
    goto fatal;
  }

  raw = orig_termios; /* modify the original mode */
  /* input modes: no break, no CR to NL, no parity check, no strip char,
   * no start/stop output control. */
  raw.c_iflag &= ~( BRKINT | ICRNL | INPCK | ISTRIP | IXON );
  /* control modes - set 8 bit chars */
  raw.c_cflag |= ( CS8 );
  /* local modes - echoing off, canonical off, no extended functions,
   * no signal chars (^Z,^C) */
  raw.c_lflag &= ~( ECHO | ICANON | IEXTEN | ISIG );
  /* control chars - set return condition: min number of bytes and timer.
   * We want read to return every single byte, without timeout. */
  raw.c_cc[VMIN]  = 1;
  raw.c_cc[VTIME] = 0; /* 1 byte, no timer */

  /* put terminal in raw mode after flushing */
# if defined(__ANDROID__)
#  define TCSA_TYPE TCSANOW
  (void)fflush(stdout);
  (void)fflush(stderr);
# else
#  define TCSA_TYPE TCSAFLUSH
# endif
  if (tcsetattr(fd, TCSA_TYPE, &raw) < 0)
  {
    goto fatal;
  }

  rawmode = 1;
  return ( 0 );

fatal:
  errno = ENOTTY;
  return ( -1 );
}

static void
disableRawMode(int fd)
{
  /* Don't even check the return value as it's too late. */
# if defined(__ANDROID__)
  (void)fflush(stdout);
  (void)fflush(stderr);
# endif
  if (rawmode && tcsetattr(fd, TCSA_TYPE, &orig_termios) != -1)
  {
    rawmode = 0;
  }
}

/*
 * Use the ESC [6n escape sequence to query the horizontal cursor position
 * and return it. On error -1 is returned, on success the position of the
 * cursor.
 */

static int
getCursorPosition(int ifd, int ofd)
{
  char     buf[32];
  int      cols, rows;
  unsigned int i = 0;

  /* Report cursor location */
  if (write(ofd, "\x1b[6n", 4) != 4)
  {
    return ( -1 );
  }

  /* Read the response: ESC [ rows ; cols R */
  while (i < sizeof ( buf ) - 1)
  {
    if (read(ifd, buf + i, 1) != 1)
    {
      break;
    }

    if (buf[i] == 'R')
    {
      break;
    }

    i++;
  }
  buf[i] = '\0';

  /* Parse it. */
  if (buf[0] != ESC || buf[1] != '[')
  {
    return ( -1 );
  }

  if (sscanf(buf + 2, "%d;%d", &rows, &cols) != 2)
  {
    return ( -1 );
  }

  return ( cols );
}

/* Try to get the number of columns in terminal, or assume 80 */
static int
getColumns(int ifd, int ofd)
{
  struct winsize ws;

  if (ioctl(1, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
  {
    /* ioctl() failed. Try to query the terminal itself. */
    int start, cols;

    /* Get the initial position so we can restore it later. */
    start = getCursorPosition(ifd, ofd);
    if (start == -1)
    {
      goto failed;
    }

    /* Go to right margin and get position. */
    if (write(ofd, "\x1b[999C", 6) != 6)
    {
      goto failed;
    }

    cols = getCursorPosition(ifd, ofd);
    if (cols == -1)
    {
      goto failed;
    }

    /* Restore position. */
    if (cols > start)
    {
      char seq[32];
      (void)snprintf(seq, sizeof ( seq ), "\x1b[%dD", cols - start);
      if (write(ofd, seq, strlen(seq)) == -1) { /* Can't recover... */ }
    }

    return ( cols );
  }
  else
  {
    return ( ws.ws_col );
  }

failed:
  return ( 80 );
}

/* Clear the screen. Used to handle ctrl+l */
void
linenoiseClearScreen(void)
{
  if (write(STDOUT_FILENO, "\x1b[H\x1b[2J", 7) <= 0)
  { /* nothing to do, just to avoid warning. */
  }
}

# if defined(LH_COMPLETION)

/*
 * Beep, used for completion when there is nothing to complete or when all
 * the choices were already shown.
 */

static void
linenoiseBeep(void)
{
  (void)fflush(stdout);
  (void)fprintf(stderr, "\x7");
  (void)fflush(stderr);
}

/* Free a list of completion option populated by linenoiseAddCompletion(). */
static void
freeCompletions(const linenoiseCompletions *lc)
{
  size_t i;

  for (i = 0; i < lc->len; i++)
  {
    FREE(lc->cvec[i]);
  }

  if (lc->cvec != NULL)
  {
    FREE(lc->cvec);
  }
}

/*
 * This is an helper function for linenoiseEdit() and is called when the
 * user types the <tab> key in order to complete the string currently in the
 * input.
 *
 * The state of the editing is encapsulated into the pointed linenoiseState
 * structure as described in the structure definition.
 */

static int
completeLine(struct linenoiseState *ls)
{
  linenoiseCompletions lc = {
    0, NULL
  };
  int nread, nwritten;
  char c = 0;

  completionCallback(ls->buf, &lc);
  if (lc.len == 0)
  {
    linenoiseBeep();
  }
  else
  {
    size_t stop = 0, i = 0;

    while (!stop)
    {
      /* Show completion or original buffer */
      if (i < lc.len)
      {
        struct linenoiseState saved = *ls;

        ls->len = ls->pos = strlen(lc.cvec[i]);
        ls->buf = lc.cvec[i];
        refreshLine(ls);
        ls->len = saved.len;
        ls->pos = saved.pos;
        ls->buf = saved.buf;
      }
      else
      {
        refreshLine(ls);
      }

      nread = read(ls->ifd, &c, 1);
      if (nread <= 0)
      {
        freeCompletions(&lc);
        return ( -1 );
      }

      switch (c)
      {
      case 9: /* Tab */
        i = ( i + 1 ) % ( lc.len + 1 );
        if (i == lc.len)
        {
          linenoiseBeep();
        }

        stop = 1;
        break;

      default:
        /* Update buffer and return */
        if (i < lc.len)
        {
          nwritten = snprintf(ls->buf, ls->buflen, "%s", lc.cvec[i]);
          ls->len  = ls->pos = nwritten;
        }

        stop = 1;
        break;
      }
    }
  }

  freeCompletions(&lc);
  return ( c ); /* Return last read character */
}

/* Register a callback function to be called for tab-completion. */
void
linenoiseSetCompletionCallback(linenoiseCompletionCallback *fn)
{
  completionCallback = fn;
}

# endif /* if defined(LH_COMPLETION) */

# if defined(LH_HINTS)

/*
 * Register a hits function to be called to show hits to the user at the
 * right of the prompt.
 */

void
linenoiseSetHintsCallback(linenoiseHintsCallback *fn)
{
  hintsCallback = fn;
}

/*
 * Register a function to free the hints returned by the hints callback
 * registered with linenoiseSetHintsCallback().
 */

void
linenoiseSetFreeHintsCallback(linenoiseFreeHintsCallback *fn)
{
  freeHintsCallback = fn;
}

# endif /* if defined(LH_HINTS) */

# if defined(LH_COMPLETION)

/*
 * This function is used by the callback function registered by the user
 * in order to add completion options given the input string when the
 * user typed <tab>.
 */

void
linenoiseAddCompletion(linenoiseCompletions *lc, const char *str)
{
  size_t len = strlen(str);
  char *copy, **cvec;

  copy = malloc(len + 1);
  if (copy == NULL)
  {
    return;
  }

  memcpy(copy, str, len + 1);
  cvec = realloc(lc->cvec, sizeof ( char * ) * ( lc->len + 1 ));
  if (cvec == NULL)
  {
    FREE(copy);
    return;
  }

  lc->cvec            = cvec;
  lc->cvec[lc->len++] = copy;
}

# endif /* if defined(LH_COMPLETION) */

/*
 * We define a very simple "append buffer" structure, that is an heap
 * allocated string where we can append to. This is useful in order to
 * write all the escape sequences in a buffer and flush them to the standard
 * output in a single call, to avoid flickering effects.
 */

struct abuf
{
  char *b;
  int len;
};

static void
abInit(struct abuf *ab)
{
  ab->b = NULL;
  ab->len = 0;
}

static void
abAppend(struct abuf *ab, const char *s, int len)
{
  if (len <= 0)
  {
    return;
  }

  char *new = realloc(ab->b, 1 + ab->len + len);

  if (new == NULL)
  {
    return;
  }

  memcpy(new + ab->len, s, len);
  ab->b    = new;
  ab->len += len;
}

static void
abFree(const struct abuf *ab)
{
  free(ab->b); /* X-LINTED: FREE */
}

# if defined(LH_HINTS)

/*
 * Helper of refreshSingleLine() and refreshMultiLine() to show hints
 * to the right of the prompt.
 */

static void
refreshShowHints(struct abuf *ab, const struct linenoiseState *l, int plen)
{
  char seq[64];

  seq[0] = '\0';
  if (hintsCallback && plen + l->len < l->cols)
  {
    int color = -1, bold = 0;
    char *hint = hintsCallback(l->buf, &color, &bold);
    if (hint)
    {
      int hintlen    = strlen(hint);
      int hintmaxlen = l->cols - ( plen + l->len );
      if (hintlen > hintmaxlen)
      {
        hintlen = hintmaxlen;
      }

      if (bold == 1 && color == -1)
      {
        color = 37;
      }

      if (color != -1 || bold != 0)
      {
        (void)snprintf(seq, sizeof ( seq ), "\033[%d;%d;49m", bold, color);
      }
      else
      {
        seq[0] = '\0';
      }

      abAppend(ab, seq, strlen(seq));
      abAppend(ab, hint, hintlen);
      if (color != -1 || bold != 0)
      {
        abAppend(ab, "\033[0m", 4);
      }

      /* Call the function to free the hint returned. */
      if (freeHintsCallback)
      {
        freeHintsCallback(hint);
      }
    }
  }
}

# endif /* if defined(LH_HINTS) */

/*
 * Single line low level line refresh.
 *
 * Rewrite the currently edited line accordingly to the buffer content,
 * cursor position, and number of columns of the terminal.
 */

static void
refreshSingleLine(const struct linenoiseState *l)
{
  char   seq[64];
  size_t plen = pstrlen(l->prompt);
  int    fd   = l->ofd;
  char   *buf = l->buf;
  size_t len  = l->len;
  size_t pos  = l->pos;
  struct abuf ab;

  while (( plen + pos ) >= l->cols)
  {
    buf++;
    len--;
    pos--;
  }
  while (plen + len > l->cols)
  {
    len--;
  }

  abInit(&ab);
  /* Cursor to left edge */
  (void)snprintf(seq, sizeof ( seq ), "\r");
  abAppend(&ab, seq, strlen(seq));
  /* Write the prompt and the current buffer content */
  abAppend(&ab, l->prompt, strlen(l->prompt));
# if defined(LH_MASKMODE)
  if (maskmode == 1)
  {
    while (len--)
    {
      abAppend(&ab, "*", 1);
    }
  }
  else
  {
# endif /* if defined(LH_MASKMODE) */
  abAppend(&ab, buf, len);
# if defined(LH_MASKMODE)
}
# endif /* if defined(LH_MASKMODE) */
# if defined(LH_HINTS)
  /* Show hits if any. */
  refreshShowHints(&ab, l, plen);
# endif /* if defined(LH_HINTS) */
  /* Erase to right */
  (void)snprintf(seq, sizeof ( seq ), "\x1b[0K");
  abAppend(&ab, seq, strlen(seq));
  /* Move cursor to original position. */
  (void)snprintf(seq, sizeof ( seq ), "\r\x1b[%dC", (int)( pos + plen ));
  abAppend(&ab, seq, strlen(seq));
  if (write(fd, ab.b, ab.len) == -1)
  { /* Can't recover from write error. */
  }

  abFree(&ab);
}

/*
 * Multi line low level line refresh.
 *
 * Rewrite the currently edited line accordingly to the buffer content,
 * cursor position, and number of columns of the terminal.
 */

static void
refreshMultiLine(struct linenoiseState *l)
{
  char seq[64];
  int plen = strlen(l->prompt);
  int rows = ( plen + l->len + l->cols - 1 )
             / l->cols; /* rows used by current buf. */
  int rpos = ( plen + l->oldpos + l->cols ) / l->cols; /* cursor relative row. */
  int rpos2; /* rpos after refresh. */
  int col; /* colum position, zero-based. */
  int old_rows = l->maxrows;
  int fd = l->ofd, j;
  struct abuf ab;

  /* Update maxrows if needed. */
  if (rows > (int)l->maxrows)
  {
    l->maxrows = rows;
  }

  /*
   * First step: clear all the lines used before. To do so start by
   * going to the last row.
   */

  abInit(&ab);
  if (old_rows - rpos > 0)
  {
    (void)snprintf(seq, sizeof ( seq ), "\x1b[%dB", old_rows - rpos);
    abAppend(&ab, seq, strlen(seq));
  }

  /* Now for every row clear it, go up. */
  for (j = 0; j < old_rows - 1; j++)
  {
    (void)snprintf(seq, sizeof ( seq ), "\r\x1b[0K\x1b[1A");
    abAppend(&ab, seq, strlen(seq));
  }

  /* Clean the top line. */
  (void)snprintf(seq, sizeof ( seq ), "\r\x1b[0K");
  abAppend(&ab, seq, strlen(seq));

  /* Write the prompt and the current buffer content */
  abAppend(&ab, l->prompt, strlen(l->prompt));
# if defined(LH_MASKMODE)
  if (maskmode == 1)
  {
    unsigned int i;
    for (i = 0; i < l->len; i++)
    {
      abAppend(&ab, "*", 1);
    }
  }
  else
  {
# endif /* if defined(LH_MASKMODE) */
  abAppend(&ab, l->buf, l->len);
# if defined(LH_MASKMODE)
}
# endif /* if defined(LH_MASKMODE) */

# if defined(LH_HINTS)
  /* Show hits if any. */
  refreshShowHints(&ab, l, plen);
# endif /* if defined(LH_HINTS) */

  /*
   * If we are at the very end of the screen with our prompt, we need to
   * emit a newline and move the prompt to the first column.
   */

  if (l->pos && l->pos == l->len && ( l->pos + plen ) % l->cols == 0)
  {
    abAppend(&ab, "\n", 1);
    (void)snprintf(seq, sizeof ( seq ), "\r");
    abAppend(&ab, seq, strlen(seq));
    rows++;
    if (rows > (int)l->maxrows)
    {
      l->maxrows = rows;
    }
  }

  /* Move cursor to right position. */
  rpos2 = ( plen + l->pos + l->cols ) / l->cols;

  /* Go up till we reach the expected positon. */
  if (rows - rpos2 > 0)
  {
    (void)snprintf(seq, sizeof ( seq ), "\x1b[%dA", rows - rpos2);
    abAppend(&ab, seq, strlen(seq));
  }

  /* Set column. */
  col = ( plen + (int)l->pos ) % (int)l->cols;
  if (col)
  {
    (void)snprintf(seq, sizeof ( seq ), "\r\x1b[%dC", col);
  }
  else
  {
    (void)snprintf(seq, sizeof ( seq ), "\r");
  }

  abAppend(&ab, seq, strlen(seq));

  l->oldpos = l->pos;

  if (write(fd, ab.b, ab.len) == -1)
  { /* Can't recover from write error. */
  }

  abFree(&ab);
}

/*
 * Calls the two low level functions refreshSingleLine() or
 * refreshMultiLine() according to the selected mode.
 */

static void
refreshLine(struct linenoiseState *l)
{
  l->cols = getColumns(STDIN_FILENO, STDOUT_FILENO);
  if (mlmode)
  {
    refreshMultiLine(l);
  }
  else
  {
    refreshSingleLine(l);
  }
}

/*
 * Insert the character 'c' at cursor current position.
 * On error writing to the terminal -1 is returned, otherwise 0.
 */

int
linenoiseEditInsert(struct linenoiseState *l, char c)
{
  if (l->len < l->buflen)
  {
    if (l->len == l->pos)
    {
      l->buf[l->pos] = c;
      l->pos++;
      l->len++;
      l->buf[l->len] = '\0';
# if defined( LH_MASKMODE ) && defined( LH_HINTS )
      if (( !mlmode && l->plen + l->len < l->cols && !hintsCallback ))
      {
        /* Avoid a full update of the line in the trivial case. */
        char d = ( maskmode == 1 ) ? '*' : (char)c;
        if (write(l->ofd, &d, 1) == -1)
        {
          return ( -1 );
        }
      }
      else
      {
# endif /* if defined( LH_MASKMODE ) && defined( LH_HINTS ) */
      refreshLine(l);
# if defined( LH_MASKMODE ) && defined( LH_HINTS )
      }
# endif /* if defined( LH_MASKMODE ) && defined( LH_HINTS ) */
    }
    else
    {
      memmove(l->buf + l->pos + 1, l->buf + l->pos, l->len - l->pos);
      l->buf[l->pos] = c;
      l->len++;
      l->pos++;
      l->buf[l->len] = '\0';
      refreshLine(l);
    }
  }

  return ( 0 );
}

/* Move cursor on the left. */
void
linenoiseEditMoveLeft(struct linenoiseState *l)
{
  if (l->pos > 0)
  {
    l->pos--;
    refreshLine(l);
  }
}

/* Move cursor on the right. */
void
linenoiseEditMoveRight(struct linenoiseState *l)
{
  if (l->pos != l->len)
  {
    l->pos++;
    refreshLine(l);
  }
}

/* Move cursor to the end of the current word. */
void
linenoiseEditMoveWordEnd(struct linenoiseState *l)
{
  if (l->len == 0 || l->pos >= l->len)
  {
    return;
  }

  if (l->buf[l->pos] == ' ')
  {
    while (l->pos < l->len && l->buf[l->pos] == ' ')
    {
      ++l->pos;
    }
  }

  while (l->pos < l->len && l->buf[l->pos] != ' ')
  {
    ++l->pos;
  }
  refreshLine(l);
}

/* Move cursor to the start of the current word. */
void
linenoiseEditMoveWordStart(struct linenoiseState *l)
{
  if (l->len == 0)
  {
    return;
  }

  if (l->buf[l->pos - 1] == ' ')
  {
    --l->pos;
  }

  if (l->buf[l->pos] == ' ')
  {
    while (l->pos > 0 && l->buf[l->pos] == ' ')
    {
      --l->pos;
    }
  }

  while (l->pos > 0 && l->buf[l->pos - 1] != ' ')
  {
    --l->pos;
  }
  refreshLine(l);
}

/* Move cursor to the start of the line. */
void
linenoiseEditMoveHome(struct linenoiseState *l)
{
  if (l->pos != 0)
  {
    l->pos = 0;
    refreshLine(l);
  }
}

/* Move cursor to the end of the line. */
void
linenoiseEditMoveEnd(struct linenoiseState *l)
{
  if (l->pos != l->len)
  {
    l->pos = l->len;
    refreshLine(l);
  }
}

/*
 * Substitute the currently edited line with the next
 * or previous history entry as specified by 'dir'.
 */

# define LINENOISE_HISTORY_NEXT 0
# define LINENOISE_HISTORY_PREV 1
void
linenoiseEditHistoryNext(struct linenoiseState *l, int dir)
{
  if (history_len > 1)
  {

    /*
     * Update the current history entry before to
     * overwrite it with the next one.
     */

    FREE(history[history_len - 1 - l->history_index]);
    history[history_len - 1 - l->history_index] = strdup(l->buf);
    if (!history[history_len - 1 - l->history_index])
      {
        (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                       __func__, __FILE__, __LINE__);
# if defined(USE_BACKTRACE)
#  if defined(SIGUSR2)
        (void)raise(SIGUSR2);
        /*NOTREACHED*/ /* unreachable */
#  endif /* if defined(SIGUSR2) */
# endif /* if defined(USE_BACKTRACE) */
        abort();
      }
    /* Show the new entry */
    l->history_index += ( dir == LINENOISE_HISTORY_PREV ) ? 1 : -1;
    if (l->history_index < 0)
    {
      l->history_index = 0;
      return;
    }
    else if (l->history_index >= history_len)
    {
      l->history_index = history_len - 1;
      return;
    }

    strncpy(l->buf, history[history_len - 1 - l->history_index], l->buflen);
    l->buf[l->buflen - 1] = '\0';
    l->len = l->pos = strlen(l->buf);
    refreshLine(l);
  }
}

/*
 * Search a line in history that start with the same characters as the
 * currently edited line. Substitute the current line with this history
 */

# define LINENOISE_SEARCH_HISTORY_FORWARD 0
# define LINENOISE_SEARCH_HISTORY_REVERSE 1
void
linenoiseSearchInHistory(struct linenoiseState *l, int direction)
{
  if (history_len > 1)
  {

    /*
     * Update the current history entry before to
     * overwrite it with the next one.
     */

    FREE(history[history_len - 1 - l->history_index]);
    history[history_len - 1 - l->history_index] = strdup(l->buf);
    if (!history[history_len - 1 - l->history_index])
      {
        (void)fprintf (stderr, "\rFATAL: Out of memory! Aborting at %s[%s:%d]\r\n",
                       __func__, __FILE__, __LINE__);
# if defined(USE_BACKTRACE)
#  if defined(SIGUSR2)
        (void)raise(SIGUSR2);
        /*NOTREACHED*/ /* unreachable */
#  endif /* if defined(SIGUSR2) */
# endif /* if defined(USE_BACKTRACE) */
        abort();
      }

    /* Search new entry */
    int cnt;
    if (direction == LINENOISE_SEARCH_HISTORY_FORWARD)
    {
      cnt = history_len - 2 - l->history_index;
      for (; cnt >= 0; cnt--)
      {

        /*
         * Search a history entry that start with same
         * as the current line until the cursor position
         */

        if (strncmp(l->buf, history[cnt], l->pos) == 0)
        {
          strncpy(l->buf, history[cnt], l->buflen);
          l->buf[l->buflen - 1] = '\0';
          /* Don't change old cursor position */
          l->len = strlen(l->buf);

          /*
           * Set history index so that we can contiune
           * the search on this postiion
           */

          l->history_index = history_len - 1 - cnt;
          refreshLine(l);
          return;
        }
      }
    }
    else if (direction == LINENOISE_SEARCH_HISTORY_REVERSE)
    {
      cnt = history_len - l->history_index;
      for (; cnt < history_len; cnt++)
      {

        /*
         * Search a history entry that start with same
         * as the current line until the cursor position
         */

        if (strncmp(l->buf, history[cnt], l->pos) == 0)
        {
          strncpy(l->buf, history[cnt], l->buflen);
          l->buf[l->buflen - 1] = '\0';
          /* Don't change old cursor position */
          l->len = strlen(l->buf);

          /*
           * Set history index so that we can contiune
           * the search on this postiion
           */

          l->history_index = history_len - 1 - cnt;
          refreshLine(l);
          return;
        }
      }
    }
  }
}

/*
 * Delete the character at the right of the cursor without altering the cursor
 * position. Basically this is what happens with the "Delete" keyboard key.
 */

void
linenoiseEditDelete(struct linenoiseState *l)
{
  if (l->len > 0 && l->pos < l->len)
  {
    memmove(l->buf + l->pos, l->buf + l->pos + 1, l->len - l->pos - 1);
    l->len--;
    l->buf[l->len] = '\0';
    refreshLine(l);
  }
}

/* Backspace implementation. */
void
linenoiseEditBackspace(struct linenoiseState *l)
{
  if (l->pos > 0 && l->len > 0)
  {
    memmove(l->buf + l->pos - 1, l->buf + l->pos, l->len - l->pos);
    l->pos--;
    l->len--;
    l->buf[l->len] = '\0';
    refreshLine(l);
  }
}

/*
 * Delete the previous word, maintaining the
 * cursor at the start of the current word.
 */

void
linenoiseEditDeletePrevWord(struct linenoiseState *l)
{
  size_t old_pos = l->pos;
  size_t diff;

  while (l->pos > 0 && l->buf[l->pos - 1] == ' ')
  {
    l->pos--;
  }
  while (l->pos > 0 && l->buf[l->pos - 1] != ' ')
  {
    l->pos--;
  }
  diff = old_pos - l->pos;
  memmove(l->buf + l->pos, l->buf + old_pos, l->len - old_pos + 1);
  l->len -= diff;
  refreshLine(l);
}

/* Delete the next word, maintaining the cursor at the same position */
void
linenoiseEditDeleteNextWord(struct linenoiseState *l)
{
  size_t next_word_end = l->pos;

  while (next_word_end < l->len && l->buf[next_word_end] == ' ')
  {
    ++next_word_end;
  }
  while (next_word_end < l->len && l->buf[next_word_end] != ' ')
  {
    ++next_word_end;
  }
  memmove(l->buf + l->pos, l->buf + next_word_end, l->len - next_word_end);
  l->len -= next_word_end - l->pos;
  refreshLine(l);
}

/*
 * This function is the core of the line editing capability of linenoise.
 * It expects 'fd' to be already in "raw mode" so that every key pressed
 * will be returned ASAP to read().
 *
 * The resulting string is put into 'buf' when the user type enter, or
 * when ctrl+d is typed.
 *
 * The function returns the length of the current buffer.
 */

static int
linenoiseEdit(int stdin_fd, int stdout_fd, char *buf, size_t buflen,
              const char *prompt)
{
  struct linenoiseState l;

  /*
   * Populate the linenoise state that we pass to functions implementing
   * specific editing functionalities.
   */

  l.ifd           = stdin_fd;
  l.ofd           = stdout_fd;
  l.buf           = buf;
  l.buflen        = buflen;
  l.prompt        = prompt;
  l.plen          = pstrlen(prompt);
  l.oldpos        = l.pos = 0;
  l.len           = 0;
  l.cols          = getColumns(stdin_fd, stdout_fd);
  l.maxrows       = 0;
  l.history_index = 0;

  /* Buffer starts empty. */
  l.buf[0] = '\0';
  l.buflen--; /* Make sure there is always space for the nulterm */

  /*
   * The latest history entry is always our current
   * buffer, that initially is just an empty string.
   */

  (void)linenoiseHistoryAdd("");

  if (write(l.ofd, prompt, strlen(prompt)) == -1)
  {
    return ( -1 );
  }

  while (1)
  {
    signed char c;
    int    nread;
    char   seq[3];

    nread = read(l.ifd, &c, 1);
    if (nread <= 0)
    {
      return ( l.len );
    }

# if defined(LH_COMPLETION)

    /*
     * Only autocomplete when the callback is set. It returns < 0 when
     * there was an error reading from fd. Otherwise it will return the
     * character that should be handled next.
     */

    if (c == 9 && completionCallback != NULL)
    {
      int cint = completeLine(&l);
      /* Return on errors */
      if (cint < 0)
      {
        return ( l.len );
      }

      /* Read next character when 0 */
      if (cint == 0)
      {
        continue;
      }

      c = (char)cint;
    }

# endif /* if defined(LH_COMPLETION) */

    switch (c)
    {
    case 9:
      break;    /* johnsonjh - disable processing of tabs */

    case ENTER: /* Enter */
      history_len--;
      FREE(history[history_len]);
      if (mlmode)
      {
        linenoiseEditMoveEnd(&l);
      }

# if defined(LH_HINTS)
      if (hintsCallback)
      {

        /*
         * Force a refresh without hints to leave the previous
         * line as the user typed it after a newline.
         */

        linenoiseHintsCallback *hc = hintsCallback;
        hintsCallback              = NULL;
        refreshLine(&l);
        hintsCallback              = hc;
      }

# endif /* if defined(LH_HINTS) */
      return ((int)l.len );

    case CTRL_C: /* Ctrl-C */
      errno = EAGAIN;
      return ( -1 );

    case BACKSPACE: /* Backspace */
    case 8:         /* Ctrl-H */
      linenoiseEditBackspace(&l);
      break;

    case CTRL_D:     /* Ctrl-D, remove char at right of cursor, or */
      if (l.len > 0) /* if the line is empty, act as end-of-file.  */
      {
        linenoiseEditDelete(&l);
      }
      else
      {
        history_len--;
        FREE(history[history_len]);
        return ( -1 );
      }

      break;

    case CTRL_T: /* Ctrl-T */
      break;

    case CTRL_B: /* Ctrl-B */
      linenoiseEditMoveLeft(&l);
      break;

    case CTRL_F: /* Ctrl-F */
      linenoiseEditMoveRight(&l);
      break;

    case CTRL_P: /* Ctrl-P */
      linenoiseEditHistoryNext(&l, LINENOISE_HISTORY_PREV);
      break;

    case CTRL_N: /* Ctrl-N */
      linenoiseEditHistoryNext(&l, LINENOISE_HISTORY_NEXT);
      break;

    case ESC: /* Escape Sequence */

      /*
       * Read the next two bytes representing the escape sequence.
       * Use two calls to handle slow terminals returning the two
       * chars at different times.
       */

      if (read(l.ifd, seq, 1) == -1)
      {
        break;
      }

      if (read(l.ifd, seq + 1, 1) == -1)
      {
        break;
      }

      /* ESC [ sequences. */
      if (seq[0] == '[')
      {
        if (seq[1] >= '0' && seq[1] <= '9')
        {
          /* Extended escape, read additional byte. */
          if (read(l.ifd, seq + 2, 1) == -1)
          {
            break;
          }

          if (seq[2] == '~')
          {
            switch (seq[1])
            {
            case '3': /* Delete */
              linenoiseEditDelete(&l);
              break;

            case '6': /* Page Down */
              linenoiseSearchInHistory(
                &l,
                LINENOISE_SEARCH_HISTORY_REVERSE);
              break;

            case '5': /* Page Up */
              linenoiseSearchInHistory(
                &l,
                LINENOISE_SEARCH_HISTORY_FORWARD);
              break;
            }
          }
        }
        else
        {
          switch (seq[1])
          {
          case 'A': /* Up */
            linenoiseEditHistoryNext(&l, LINENOISE_HISTORY_PREV);
            break;

          case 'B': /* Down */
            linenoiseEditHistoryNext(&l, LINENOISE_HISTORY_NEXT);
            break;

          case 'C': /* Right */
            linenoiseEditMoveRight(&l);
            break;

          case 'D': /* Left */
            linenoiseEditMoveLeft(&l);
            break;

          case 'H': /* Home */
            linenoiseEditMoveHome(&l);
            break;

          case 'F': /* End */
            linenoiseEditMoveEnd(&l);
            break;
          }
        }
      }

      break;

    default:
      if (linenoiseEditInsert(&l, c))
      {
        return ( -1 );
      }

      break;

    case CTRL_U: /* Ctrl+U, delete the whole line. */
      buf[0] = '\0';
      l.pos  = l.len = 0;
      refreshLine(&l);
      break;

    case CTRL_K: /* Ctrl+K, delete from current to end of line. */
      buf[l.pos] = '\0';
      l.len      = l.pos;
      refreshLine(&l);
      break;

    case CTRL_A: /* Ctrl+A, go to the start of the line */
      linenoiseEditMoveHome(&l);
      break;

    case CTRL_E: /* Ctrl+E, go to the end of the line */
      linenoiseEditMoveEnd(&l);
      break;

    case CTRL_L: /* Ctrl+L, clear screen */
      linenoiseClearScreen();
      refreshLine(&l);
      break;

    case CTRL_W: /* Ctrl+W, delete previous word */
      linenoiseEditDeletePrevWord(&l);
      break;
    }
  }
# if defined(SUNLINT) || !defined(__SUNPRO_C) && !defined(__SUNPRO_CC)
 /*NOTREACHED*/ /* unreachable */
 return ( -1 );
# endif /* if defined(SUNLINT) || !defined(__SUNPRO_C) && !defined(__SUNPRO_CC) */
}

/*
 * This function calls the line editing function linenoiseEdit() using
 * the STDIN file descriptor set in raw mode.
 */

static int
linenoiseRaw(char *buf, size_t buflen, const char *prompt)
{
  int count;

  if (buflen == 0)
  {
    errno = EINVAL;
    return ( -1 );
  }

  if (enableRawMode(STDIN_FILENO) == -1)
  {
    return ( -1 );
  }

  count = linenoiseEdit(STDIN_FILENO, STDOUT_FILENO, buf, buflen, prompt);
  disableRawMode(STDIN_FILENO);
  (void)printf("\n");
  return ( count );
}

/*
 * This function is called when linenoise() is called with the standard
 * input file descriptor not attached to a TTY. So for example when the
 * program using linenoise is called in pipe or with a file redirected
 * to its standard input. In this case, we want to be able to return the
 * line regardless of its length (by default we are limited to 4k).
 */

static char *
linenoiseNoTTY(void)
{
  char *line = NULL;
  size_t len = 0, maxlen = 0;

  while (1)
  {
    if (len == maxlen)
    {
      if (maxlen == 0)
      {
        maxlen = 16;
      }

      maxlen *= 2;
      char *oldval = line;
      line = realloc(line, maxlen); //-V701
      if (line == NULL)
      {
        if (oldval)
        {
          FREE(oldval);
        }

        return ( NULL );
      }
    }

    int c = fgetc(stdin);
    if (c == EOF || c == '\n')
    {
      if (c == EOF && len == 0)
      {
        FREE(line);
        return ( NULL );
      }
      else
      {
        line[len] = '\0';
        return ( line );
      }
    }
    else
    {
      line[len] = c;
      len++;
    }
  }
# if defined(SUNLINT) || !defined(__SUNPRO_C) && !defined(__SUNPRO_CC)
 /*NOTREACHED*/ /* unreachable */
 return ( NULL );
# endif /* if defined(SUNLINT) || !defined(__SUNPRO_C) && !defined(__SUNPRO_CC) */
}

/*
 * The high level function that is the main API of the linenoise library.
 * This function checks if the terminal has basic capabilities, just checking
 * for a blacklist of stupid terminals, and later either calls the line
 * editing function or uses dummy fgets() so that you will be able to type
 * something even in the most desperate of the conditions.
 */

char *
linenoise(const char *prompt)
{
  char buf[LINENOISE_MAX_LINE];
  int count;

  if (!isatty(STDIN_FILENO))
  {

    /*
     * Not a tty: read from file / pipe. In this mode we don't want any
     * limit to the line size, so we call a function to handle that.
     */

    return ( linenoiseNoTTY());
  }
  else if (isUnsupportedTerm())
  {
    size_t len;

    (void)fflush(stderr);
    (void)printf("%s", prompt);
    (void)fflush(stdout);
    if (fgets(buf, LINENOISE_MAX_LINE, stdin) == NULL)
    {
      return ( NULL );
    }

    len = strlen(buf);
    while (len && ( buf[len - 1] == '\n' || buf[len - 1] == '\r' ))
    {
      len--;
      buf[len] = '\0';
    }
    return ( strdup(buf));
  }
  else
  {
    count = linenoiseRaw(buf, LINENOISE_MAX_LINE, prompt);
    if (count == -1)
    {
      return ( NULL );
    }

    return ( strdup(buf));
  }
}

/*
 * This is just a wrapper the user may want to call in order to make sure
 * the linenoise returned buffer is freed with the same allocator it was
 * created with. Useful when the main program is using an alternative
 * allocator.
 */

void
linenoiseFree(void *ptr)
{
  FREE(ptr);
}

/*
 * Free the history, but does not reset it. Only used when we have to
 * exit() to avoid memory leaks are reported by valgrind & co.
 */

static void
freeHistory(void)
{
  if (history)
  {
    int j;

    for (j = 0; j < history_len; j++)
    {
      FREE(history[j]);
    }

    FREE(history);
  }
}

/* At exit we'll try to fix the terminal to the initial conditions. */
static void
linenoiseAtExit(void)
{
  disableRawMode(STDIN_FILENO);
  freeHistory();
}

/*
 * This is the API call to add a new entry in the linenoise history.
 * It uses a fixed array of char pointers that are shifted (memmoved)
 * when the history max length is reached in order to remove the older
 * entry and make room for the new one, so it is not exactly suitable for huge
 * histories, but will work well for a few hundred of entries.
 */

int
linenoiseHistoryAdd(const char *line)
{
  char *linecopy;

  if (history_max_len == 0)
  {
    return ( 0 );
  }

  /* Initialization on first call. */
  if (history == NULL)
  {
    history = malloc(sizeof ( char * ) * history_max_len);
    if (history == NULL)
    {
      return ( 0 );
    }

    (void)memset(history, 0, ( sizeof ( char * ) * history_max_len ));
  }

  /* Don't add duplicated lines. */
  if (( history_len > 0 ) && ( !strcmp(history[history_len - 1], line)))
  {
    return ( 0 );
  }

  /*
   * Add an heap allocated copy of the line in the history.
   * If we reached the max length, remove the older line.
   */

  linecopy = strdup(line);
  if (!linecopy)
  {
    return ( 0 );
  }

  if (history_len == history_max_len)
  {
    FREE(history[0]);
    memmove(
      history,
      history + 1,
      sizeof ( char * ) * ( history_max_len - 1 ));
    history_len--;
  }

  history[history_len] = linecopy;
  history_len++;
  return ( 1 );
}

/*
 * Set the maximum length for the history. This function can be called even
 * if there is already some history, the function will make sure to retain
 * just the latest 'len' elements if the new history length value is smaller
 * than the amount of items already inside the history.
 */

int
linenoiseHistorySetMaxLen(int len)
{
  char **new;

  if (len < 1)
  {
    return ( 0 );
  }

  if (history)
  {
    int tocopy = history_len;

    new = malloc(sizeof ( char * ) * len);
    if (new == NULL)
    {
      return ( 0 );
    }

    /* If we can't copy everything, free the elements we'll not use. */
    if (len < tocopy)
    {
      int j;

      for (j = 0; j < tocopy - len; j++)
      {
        FREE(history[j]);
      }

      tocopy = len;
    }

    (void)memset(new, 0, sizeof ( char * ) * len);
    memcpy(
      new,
      history + ( history_len - tocopy ),
      sizeof ( char * ) * tocopy);
    FREE(history);
    history = new;
  }

  history_max_len = len;
  if (history_len > history_max_len)
  {
    history_len = history_max_len;
  }

  return ( 1 );
}

/* Calculate length of the prompt string as seen from a terminal. */
size_t
pstrlen(const char *s)
{
  size_t len = 0, i = 0;

  while (s[i] != '\0')
  {
    if (s[i] == '\033') //-V536
    {
      i = strpbrk(s + i, "m") - s + 1; //-V769
      continue;
    }

    len++;
    i++;
  }
  return ( len );
}

#endif /* if !defined ( __MINGW32__ ) && !defined ( CROSS_MINGW32 ) && !defined ( CROSS_MINGW64 ) && !defined ( __MINGW64__ ) && !defined ( _MSC_VER ) && !defined ( _MSC_BUILD ) */
