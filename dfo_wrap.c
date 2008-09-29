#include <corelib/scan.h>

#define DFO_IMPLEMENTATION
#include "dfo.h"

static int
dfo_wrap_hard(struct dfo_put *dp, struct dfo_buffer *dbuf, struct dstring *in)
{
  unsigned long pos;
  unsigned long line_pos;
  unsigned long line_max;

  if (!dfo_tran_flatten(dp, in)) return 0;

  line_max = dfo_size_linemax(dp);
  line_pos = dfo_size_linepos(dp, line_max, dbuf);

  for (pos = 0; pos < in->len; ++pos) {
    if (line_pos >= line_max) {
      if (!dstring_catb(&dp->buf_tmp, "\n", 1)) return 0;
      line_pos = 0;
    }
    if (!dstring_catb(&dp->buf_tmp, &in->s[pos], 1)) return 0;
    ++line_pos;
  }

  /* update buffer */
  dbuf->line_pos = line_pos;

  if (!dstring_copy(in, &dp->buf_tmp)) return 0;
  dstring_0(in);
  return 1;
}

/* as with soft wrap but if a non-whitespace word is still to big to fit on a
 * line, hyphenate it.
 */

static int
dfo_wrap_hyph(struct dfo_put *dp, struct dfo_buffer *dbuf, struct dstring *in)
{
  const char *str;
  unsigned long len;
  unsigned long pos;
  unsigned long line_pos;
  unsigned long line_max;

  line_max = dfo_size_linemax(dp);
  line_pos = dfo_size_linepos(dp, line_max, dbuf);

  if (line_max <= 1) { dp->error = DFO_TOO_SMALL_FOR_HYPH; return 0; }
  if (!dfo_tran_flatten(dp, in)) return 0;

  str = in->s;
  len = in->len;
  for (;;) {
    /* scan non-whitespace word */
    if (!len) break;
    pos = scan_notcharset(str, dfo_whitespace);
    if (line_pos + pos >= line_max) {
      if (!dstring_catb(&dp->buf_tmp, "\n", 1)) return 0;
      line_pos = 0;
    }

    /* will word fit on the line? */
    if (pos < line_max) {
      if (!dstring_catb(&dp->buf_tmp, str, pos)) return 0;
      line_pos += pos;
      str += pos;
      len -= pos;
    } else {
      /* hyphenate */
      while (pos >= line_max) {
        if (!dstring_catb(&dp->buf_tmp, str, line_max - 1)) return 0;
        if (!dstring_catb(&dp->buf_tmp, "-\n", 2)) return 0;
        str += line_max - 1;
        len -= line_max - 1;
        pos -= line_max - 1;
      }
      if (!dstring_catb(&dp->buf_tmp, str, pos)) return 0;
      line_pos += pos;
      str += pos;
      len -= pos;
    }

    /* scan whitespace word (place on end of line if breaking) */
    if (!len) break;
    pos = scan_charset(str, dfo_whitespace);
    if (!dstring_catb(&dp->buf_tmp, str, pos)) return 0;
    if (line_pos + pos >= line_max) {
      if (!dstring_catb(&dp->buf_tmp, "\n", 1)) return 0;
      line_pos = 0;
    }
    line_pos += pos;
    str += pos;
    len -= pos;
  }

  /* update buffer */
  dbuf->line_pos = line_pos;

  if (!dstring_copy(in, &dp->buf_tmp)) return 0;
  dstring_0(in);
  return 1;
}

/* fit words on a line, do not split words if the length of a word is longer
 * than the maximum line length.
 */

static int
dfo_wrap_soft(struct dfo_put *dp, struct dfo_buffer *dbuf, struct dstring *in)
{
  const char *str;
  unsigned long len;
  unsigned long pos;
  unsigned long line_pos;
  unsigned long line_max;

  line_max = dfo_size_linemax(dp);
  line_pos = dfo_size_linepos(dp, line_max, dbuf);

  if (!dfo_tran_flatten(dp, in)) return 0;

  str = in->s;
  len = in->len;
  for (;;) {
    /* scan non-whitespace word */
    if (!len) break;
    pos = scan_notcharset(str, dfo_whitespace);
    if (line_pos + pos >= line_max) {
      if (!dstring_catb(&dp->buf_tmp, "\n", 1)) return 0;
      line_pos = 0;
    }
    if (!dstring_catb(&dp->buf_tmp, str, pos)) return 0;
    line_pos += pos;
    str += pos;
    len -= pos;

    /* scan whitespace word (place on end of line if breaking) */
    if (!len) break;
    pos = scan_charset(str, dfo_whitespace);
    if (!dstring_catb(&dp->buf_tmp, str, pos)) return 0;
    if (line_pos + pos >= line_max) {
      if (!dstring_catb(&dp->buf_tmp, "\n", 1)) return 0;
      line_pos = 0;
    }
    line_pos += pos;
    str += pos;
    len -= pos;
  }

  /* update buffer */
  dbuf->line_pos = line_pos;

  if (!dstring_copy(in, &dp->buf_tmp)) return 0;
  dstring_0(in);
  return 1;
}

static const struct {
  unsigned int val;
  int (*func)(struct dfo_put *, struct dfo_buffer *, struct dstring *);
} wrap_funcs[] = {
  { DFO_WRAP_SOFT, dfo_wrap_soft },
  { DFO_WRAP_HARD, dfo_wrap_hard },
  { DFO_WRAP_HYPH, dfo_wrap_hyph },
};

int
dfo_wrap(struct dfo_put *dp, struct dfo_buffer *db, struct dstring *ds)
{
  unsigned int index;

  for (index = 0; index < sizeof(wrap_funcs) / sizeof(wrap_funcs[0]); ++index) {
    if (wrap_funcs[index].val == dp->wrap_mode) {
      if (!wrap_funcs[index].func(dp, db, ds)) return 0;
      break;
    }
  }
  return 1;
}

void
dfo_wrap_mode(struct dfo_put *dp, unsigned int w)
{
  dp->wrap_mode = w;
}
