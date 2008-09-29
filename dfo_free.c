#define DFO_IMPLEMENTATION
#include "dfo.h"

void
dfo_free(struct dfo_put *dp)
{
  struct dstring *ds;
  unsigned int index;
  for (index = 0; index < array_SIZE(&dp->col_bufs); ++index) {
    ds = array_index(&dp->col_bufs, index);
    dstring_free(ds);
  }
  dstring_free(&dp->buf_tmp);
}
