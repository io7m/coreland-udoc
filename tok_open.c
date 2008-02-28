#include <corelib/open.h>
#include "token.h"

int
token_open(struct tokenizer *t, const char *file)
{
  t->buf.fd = open_ro(file);
  return (t->buf.fd != -1);
}
