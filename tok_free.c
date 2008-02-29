#include <corelib/bin.h>
#include <corelib/dstring.h>
#include <corelib/read.h>
#include "token.h"

void
token_free(struct tokenizer *t)
{
  token_close(t);
  dstring_free(&t->tok);
  bin_zero(t, sizeof(*t));
}
