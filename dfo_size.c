#define DFO_IMPLEMENTATION
#include "dfo.h"

unsigned int
dfo_size_linemax_check(unsigned int page_max, unsigned int page_ind,
                       unsigned int col_max, unsigned int col_space)
{
  unsigned int col_sp = (col_max > 1) ? (col_max * col_space) : 0;
  unsigned int col_avg = page_max / col_max;
  unsigned int col_ind = page_ind;

  if (!col_avg) return 0;
  if (col_sp > col_avg) return 0;
  if (col_ind > col_avg) return 0;
  if (col_sp + col_ind > col_avg) return 0;
  return col_avg - col_sp - col_ind;
}

int
dfo_size_check(struct dfo_put *dp, unsigned int page_max, unsigned int page_ind,
  unsigned int col_max, unsigned int col_space)
{
  if (!col_max) { dp->error = DFO_NO_COLUMNS; return 0; }
  if (!dfo_size_linemax_check(page_max, page_ind, col_max, col_space)) {
    dp->error = DFO_PAGE_TOO_SMALL;
    return 0;
  }
  return 1;
}

unsigned int
dfo_size_linemax(const struct dfo_put *dp)
{
  return dfo_size_linemax_check(dp->page_max, dp->page_indent,
                                dp->col_max, dp->col_space);
}

unsigned int
dfo_size_linepos(const struct dfo_put *dp, unsigned int line_max,
  const struct dfo_buffer *db)
{
  return (db->line_pos >= line_max) ? line_max - 1 : db->line_pos;
}
