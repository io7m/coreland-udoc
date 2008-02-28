#include <corelib/read.h>
#include "token.h"

int
token_init(struct tokenizer *t)
{
  if (!dstring_init(&t->tok, 1024)) return 0;
  buffer_init(&t->buf, (buffer_op) read, -1, t->cbuf, sizeof(t->cbuf));
  t->line = 0;
  t->chr = 0;
  t->state = 0;
  t->ch_cur = 0;
  t->ch_next = 0;
  return 1;
}
