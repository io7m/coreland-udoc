#define DFO_IMPLEMENTATION
#include "dfo.h"

int
dfo_columns (struct dfo_put *dp, unsigned int n, unsigned int col_space)
{
  struct dfo_buffer db;
  unsigned int num;

  if (!dfo_size_check (dp, dp->page_max, dp->page_indent, n, col_space)) return 0;
  if (array_SIZE (&dp->col_bufs) < n) {
    num = n - array_SIZE (&dp->col_bufs);
    while (num--) {
      db.write_pos = 0;
      db.line_pos = 0;
      if (!dstring_init (&db.buf, DFO_BLOCK_SIZE)) return 0;
      if (!array_cat (&dp->col_bufs, &db)) return 0;
    }
  }

  dp->col_space = col_space;
  dp->col_max = n;
  dp->col_cur = -1; /* next call to columns_start () makes this 0 */
  return 1;
}

int
dfo_columns_start (struct dfo_put *dp)
{
  struct dfo_buffer *buf;

  if (dp->col_cur + 1 == dp->col_max) return 0;
  ++dp->col_cur;

  buf = (struct dfo_buffer *) dfo_current_buf (dp);
  buf->line_pos = 0;
  return 1;
}

int
dfo_columns_end (struct dfo_put *dp)
{
  if (dfo_flush (dp) == -1) return 0;
  dp->col_max = 1;
  dp->col_cur = 0;
  return 1;
}
