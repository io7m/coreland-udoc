#include <corelib/scan.h>

#define DFO_IMPLEMENTATION
#include "dfo.h"

/* convert groups of whitespace to a single space */

int
dfo_tran_respace(struct dfo_put *dp, struct dfo_buffer *dbuf, struct dstring *in)
{
  const char *str = in->s;
  unsigned long len = in->len;
  unsigned long pos;

  dstring_trunc(&dp->buf_tmp);
  for (;;) {
    /* scan non-whitespace word */
    if (!len) break;
    pos = scan_notcharset(str, dfo_whitespace);
    if (!dstring_catb(&dp->buf_tmp, str, pos)) return 0;
    str += pos;
    len -= pos;

    /* scan whitespace word */
    if (!len) break;
    pos = scan_charset(str, dfo_whitespace);
    if (!dstring_catb(&dp->buf_tmp, " ", 1)) return 0;
    str += pos;
    len -= pos;
  }

  if (!dstring_copy(in, &dp->buf_tmp)) return 0;
  dstring_0(in);
  return 1;
}

/* convert characters from the given table */

int
dfo_tran_conv(struct dfo_put *dp, struct dfo_buffer *dbuf, struct dstring *in)
{
  unsigned int pos;
  unsigned int tr;
  unsigned int ok;
  char ch;

  dstring_trunc(&dp->buf_tmp);
  for (pos = 0; pos < in->len; ++pos) {
    ch = in->s[pos];
    ok = 0;
    for (tr = 0; tr < dp->trans_size; ++tr)
      if (ch == dp->trans[tr].ch) {
        if (!dstring_cats(&dp->buf_tmp, dp->trans[tr].func(dp)))
          return 0;
        ok = 1;
      }
    if (!ok)
      if (!dstring_catb(&dp->buf_tmp, &ch, 1)) return 0;
    dp->ch_prev = ch;
  }

  if (!dstring_copy(in, &dp->buf_tmp)) return 0;
  dstring_0(in);
  return 1;
}

int
dfo_tran_flatten(struct dfo_put *dp, struct dstring *in)
{
  const char *str = in->s;
  unsigned long len = in->len;
  unsigned long pos;

  dstring_trunc(&dp->buf_tmp);

  str = in->s;
  len = in->len;
  for (pos = 0; pos < len; ++pos) {
    if (in->s[pos] != '\n') {
      if (!dstring_catb(&dp->buf_tmp, &in->s[pos], 1)) return 0;
    } else
      if (!dstring_catb(&dp->buf_tmp, " ", 1)) return 0;
  }

  if (!dstring_copy(in, &dp->buf_tmp)) return 0;
  dstring_trunc(&dp->buf_tmp);
  return 1;
}

void
dfo_tran_enable(struct dfo_put *dp, unsigned int t)
{
  dp->tran_mode |= t;
}

void
dfo_tran_disable(struct dfo_put *dp, unsigned int t)
{
  dp->tran_mode &= dp->tran_mode ^ t;
}
