#ifndef TOKEN_H
#define TOKEN_H

#include <corelib/buffer.h>
#include <corelib/dstring.h>

enum token_type {
  TOKEN_TYPE_SYMBOL,
  TOKEN_TYPE_STRING,
  TOKEN_TYPE_PAREN_OPEN,
  TOKEN_TYPE_PAREN_CLOSE,
  TOKEN_TYPE_EOF,
};

struct tokenizer {
  char cbuf[BUFFER_INSIZE];
  struct buffer buf;
  struct dstring tok;
  unsigned long line;
  unsigned long chr;
  unsigned int state;
  char ch_cur;
  char ch_next;
};

int token_init (struct tokenizer *);
int token_open (struct tokenizer *, const char *);
int token_next (struct tokenizer *, char **, enum token_type *);
int token_close (struct tokenizer *);
void token_free (struct tokenizer *);

#endif
