#include <stdio.h>
#include <stdlib.h>

#include "../t_assert.h"
#include "../../token.h"

struct tokenizer t;
enum token_type type;
char *tok;

char *file;
unsigned int lines;
unsigned int chars;

int
main(int argc, char *argv[])
{
  test_assert(argc == 4);

  file = argv[1];
  lines = atoi(argv[2]);
  chars = atoi(argv[3]);

  test_assert(token_init(&t) == 1);
  test_assert(token_open(&t, file) == 1);

  for (;;) {
    test_assert(token_next(&t, &tok, &type) == 1);
    switch (type) {
      case TOKEN_TYPE_SYMBOL:
        printf("SYMBOL\t\"%s\"\n", tok);
        break;
      case TOKEN_TYPE_STRING:
        printf("STRING\t\"%s\"\n", tok);
        break;
      case TOKEN_TYPE_PAREN_OPEN:
        printf("PAREN_OPEN\t\"%s\"\n", tok);
        break;
      case TOKEN_TYPE_PAREN_CLOSE:
        printf("PAREN_CLOSE\t\"%s\"\n", tok);
        break;
      case TOKEN_TYPE_EOF:
        printf("EOF\n");
        goto GOT_EOF;
        break;
      default:
        test_assert(!"token_next returned unknown token type");
        break;
    }
  }

  GOT_EOF:
  test_assert(t.line == lines);
  test_assert(t.chr == chars);
  return 0;
}
