#include <assert.h>

#include <corelib/bin.h>
#include <corelib/buffer.h>
#include <corelib/scan.h>
#include <corelib/str.h>

#define DFO_IMPLEMENTATION
#include "dfo.h"
 
/* concatenate n characters of whitespace onto ds. */

static int
dfo_catspace (struct dstring *ds, unsigned int n)
{
  while (n--) if (!dstring_catb (ds, " ", 1)) return 0;
  return 1;
}

/* concatenate input to output buffer, applying indent at each line */

static int
dfo_inbuf_copyout (struct dfo_put *dp, struct dfo_buffer *db, struct dstring *in)
{
  const char *str;
  unsigned long len;
  unsigned long pos;

  /* apply indent if in single column mode */
  if (dp->col_max == 1) {
    str = in->s;
    len = in->len;
    for (;;) {
      if (!len) break;
      if (dp->line_start) {
        if (!dfo_catspace (&db->buf, dp->page_indent)) return 0;
        dp->line_start = 0;
      }
      pos = scan_notcharset (str, "\n");
      if (!dstring_catb (&db->buf, str, pos)) return 0;
      if (str[pos] == '\n') {
        if (!dstring_catb (&db->buf, "\n", 1)) return 0;
        dp->line_start = 1;
      }
      str += pos;
      len -= pos;
      pos = scan_charsetn (str, "\n", 1);
      str += pos;
      len -= pos;
    }
  } else
    if (!dstring_cat (&db->buf, in)) return 0;

  dstring_0 (&db->buf);
  dstring_trunc (in);
  return 1;
}

/* process the input buffer
 * it's the responsibility of the upper levels to ensure that the end of the
 * buffer occurs at a sensible point. in the whitespace between words, or at
 * the end of input, for example.
 */

static int
dfo_inbuf_proc (struct dfo_put *dp)
{
  struct dfo_buffer *db = array_index (&dp->col_bufs, dp->col_cur);
  struct dstring *in = &dp->buf_input;

  if (!in->len) return 1;

  /* respace */
  if (dp->tran_mode & DFO_TRAN_RESPACE)
    if (!dfo_tran_respace (dp, db, in)) return 0;

  /* wrap */
  if (!dfo_wrap (dp, db, in)) return 0;

  /* convert characters */
  if (dp->tran_mode & DFO_TRAN_CONVERT)
    if (!dfo_tran_conv (dp, db, in)) return 0;

  if (!dfo_inbuf_copyout (dp, db, in)) return 0;
  return 1;
}

/* buffer input
 * process input buffer if it ends on whitespace.
 */

static int
dfo_inbuf_add (struct dfo_put *dp, const char *str, unsigned long len)
{
  if (!dstring_catb (&dp->buf_input, str, len)) return 0;
  dstring_0 (&dp->buf_input);

  /* process at appropriate break */
  switch (str[len - 1]) {
    case ' ':
    case '\t':
    case '\n':
    case '\r':
      if (!dfo_inbuf_proc (dp)) return 0;
      break;
    default:
      break;
  }

  /* set flag for eol */
  switch (str[len - 1]) {
    case '\n':
      dp->line_start = 1;
      break;
    default:
      break;
  }

  return 1;
}

static int
dfo_flush_single (struct dfo_put *dp)
{
  struct dfo_buffer *db = array_index (&dp->col_bufs, 0);
  const char *str;
  unsigned long pos;
  unsigned long len;

  str = db->buf.s;
  len = db->buf.len;
  for (;;) {
    if (!len) break;
    pos = scan_notcharset (str, "\n");
    buffer_put (dp->out, str, pos);
    if (str[pos] == '\n')
      buffer_put (dp->out, "\n", 1);
    str += pos;
    len -= pos;
    pos = scan_charsetn (str, "\n", 1);
    str += pos;
    len -= pos;
  }

  dstring_trunc (&db->buf);
  db->write_pos = 0;
  return buffer_flush (dp->out);
}

static int
dfo_flush_multicol (struct dfo_put *dp)
{
  unsigned long wlen;
  unsigned long pos;
  unsigned long len;
  struct dfo_buffer *db;
  struct dstring *db_out;
  struct dstring *line_buf;
  const char *str;
  unsigned int col_maxw;
  unsigned int buf_remain;
  int col;

  col_maxw = dfo_size_linemax (dp);
  line_buf = &dp->buf_tmp;

  for (;;) {
    buf_remain = 0;

    dstring_trunc (line_buf);
    for (col = 0; col < dp->col_max; ++col) {
      db = array_index (&dp->col_bufs, col);
      db_out = &db->buf;
      str = db_out->s + db->write_pos;
      len = db_out->len - db->write_pos;

      /* insert spacing/indent */
      if (!dfo_catspace (line_buf, (!col) ? dp->page_indent : dp->col_padding))
        return -1;

      /* print at most col_maxw chars */
      pos = scan_notcharset (str, "\n");
      wlen = (pos > col_maxw) ? col_maxw : pos;
      if (!dstring_catb (line_buf, str, wlen)) return -1;

      /* pad remaining space */
      if (pos < col_maxw)
        if (!dfo_catspace (line_buf, col_maxw - pos)) return -1;

      /* advance write pointer */
      str += wlen;
      db->write_pos += wlen;
      db->write_pos += scan_charsetn (str, "\n", 1);
      if (db->write_pos != db_out->len) buf_remain = 1;
    }

    /* output line */
    buffer_put (dp->out, line_buf->s, line_buf->len);
    buffer_put (dp->out, "\n", 1);

    if (!buf_remain) break;
  }

  /* empty all buffers */
  for (col = 0; col < dp->col_max; ++col) {
    db = array_index (&dp->col_bufs, col);
    db->write_pos = 0;
    dstring_trunc (&db->buf); 
  }
  return buffer_flush (dp->out);
}

int
dfo_put (struct dfo_put *dp, const char *str, unsigned long len)
{
  return dfo_inbuf_add (dp, str, len);
}

int
dfo_puts (struct dfo_put *dp, const char *str)
{
  return dfo_inbuf_add (dp, str, str_len (str));
}

int
dfo_flush (struct dfo_put *dp)
{
  if (!dfo_inbuf_proc (dp)) return -1;
  return (dp->col_max == 1) ? dfo_flush_single (dp) : dfo_flush_multicol (dp);
}

int
dfo_break (struct dfo_put *dp)
{
  return dfo_inbuf_proc (dp);
}

int
dfo_break_line (struct dfo_put *dp)
{
  struct dfo_buffer *db = array_index (&dp->col_bufs, dp->col_cur);
  if (!dfo_break (dp)) return 0;
  if (!dstring_catb (&db->buf, "\n", 1)) return 0;
  db->line_pos = 0;
  dp->line_start = 1;
  return 1;
}

int
dfo_puts8 (struct dfo_put *dp,
  const char *s1, const char *s2, const char *s3, const char *s4,
  const char *s5, const char *s6, const char *s7, const char *s8)
{
  if (s1) if (!dfo_puts (dp, s1)) return 0;
  if (s2) if (!dfo_puts (dp, s2)) return 0;
  if (s3) if (!dfo_puts (dp, s3)) return 0;
  if (s4) if (!dfo_puts (dp, s4)) return 0;
  if (s5) if (!dfo_puts (dp, s5)) return 0;
  if (s6) if (!dfo_puts (dp, s6)) return 0;
  if (s7) if (!dfo_puts (dp, s7)) return 0;
  if (s8) if (!dfo_puts (dp, s8)) return 0;
  return 1;
}
