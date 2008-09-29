#define DFO_IMPLEMENTATION
#include "dfo.h"

int
dfo_constrain (struct dfo_put *dp, unsigned int page_max, unsigned int page_ind)
{
  if (!dfo_size_check (dp, page_max, page_ind, dp->col_max, dp->col_space)) return 0;
  dp->page_max = page_max;
  dp->page_indent = page_ind;
  return 1;
}
