#define DFO_IMPLEMENTATION
#include "dfo.h"

void
dfo_free(struct dfo_put *dp)
{
  struct dstring *ds;
  unsigned int ind;
  for (ind = 0; ind < array_SIZE(&dp->col_bufs); ++ind) {
    ds = array_index(&dp->col_bufs, ind);
    dstring_free(ds);
  }
  dstring_free(&dp->buf_tmp);
}
