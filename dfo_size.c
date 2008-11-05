#define DFO_IMPLEMENTATION
#include "dfo.h"

unsigned int
dfo_size_linemax_check (unsigned int page_max, unsigned int page_indent,
  unsigned int num_columns, unsigned int col_padding)
{
  const unsigned int page_space = page_max - page_indent;

  if (!num_columns) return 0;
  if (num_columns > 1) {
    const unsigned int total_padding = col_padding * num_columns;
    const unsigned int column_average = page_max / num_columns;
    if (total_padding > page_space) return 0;
    if (!column_average) return 0;
    return column_average - col_padding;
  } else {
    if (page_indent > page_max) return 0;
    return page_space;
  }
}

int
dfo_size_check (struct dfo_put *dp, unsigned int page_max, unsigned int page_indent,
  unsigned int num_columns, unsigned int col_padding)
{
  if (!num_columns) { dp->error = DFO_NO_COLUMNS; return 0; }
  if (!dfo_size_linemax_check (page_max, page_indent, num_columns, col_padding)) {
    dp->error = DFO_PAGE_TOO_SMALL;
    return 0;
  }
  return 1;
}

unsigned int
dfo_size_linemax (const struct dfo_put *dp)
{
  return dfo_size_linemax_check (dp->page_max, dp->page_indent,
    dp->col_max, dp->col_padding);
}

unsigned int
dfo_size_linepos (const struct dfo_put *dp, unsigned int line_max,
  const struct dfo_buffer *db)
{
  return (db->line_pos >= line_max) ? line_max - 1 : db->line_pos;
}
