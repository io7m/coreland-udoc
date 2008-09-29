#include <corelib/bin.h>

#define DFO_IMPLEMENTATION
#include "dfo.h"

int
dfo_init(struct dfo_put *dp, struct buffer *buf,
  const struct dfo_trans *tr, unsigned int nt)
{
  struct dfo_buffer db;
  unsigned int index;

  bin_zero(dp, sizeof(*dp));

  if (!dstring_init(&dp->buf_tmp, DFO_BLOCK_SIZE)) goto FAIL;
  if (!dstring_init(&dp->buf_input, DFO_BLOCK_SIZE)) goto FAIL;
  if (!array_init(&dp->col_bufs, 8, sizeof(struct dfo_buffer))) goto FAIL;
  for (index = 0; index < 8; ++index) {
    bin_zero(&db, sizeof(db));
    if (!dstring_init(&db.buf, DFO_BLOCK_SIZE)) goto FAIL;
    if (!array_cat(&dp->col_bufs, &db)) goto FAIL;
  }

  dp->out = buf;
  dp->ch_prev = 0;
  dp->col_cur = 0;
  dp->col_max = 1;
  dp->col_space = 0;
  dp->line_start = 1;
  dp->page_indent = 0;
  dp->page_max = 80;
  dp->tran_mode = DFO_TRAN_NONE;
  dp->trans = tr;
  dp->trans_size = nt;
  dp->user_data = 0;
  dp->wrap_mode = DFO_WRAP_NONE;
  return 1;

  FAIL:
  dfo_free(dp);
  return 0;
}
