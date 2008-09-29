#define DFO_IMPLEMENTATION
#include "dfo.h"

const struct dfo_buffer *
dfo_current_buf (const struct dfo_put *dfo)
{
  return array_index (&dfo->col_bufs, dfo->col_cur);
}
