#include <corelib/close.h>
#include "token.h"

int
token_close(struct tokenizer *t)
{
  if (close(t->buf.fd) == -1) return 0;
  t->buf.fd = -1;
  return 1;
}
