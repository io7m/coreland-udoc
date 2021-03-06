#include "../t_assert.h"
#include "../../udoc.h"
#include "../../log.h"

int
main(int argc, char *argv[])
{
  struct udoc doc;

  log_level(6);

  test_assert(argc == 2);
  test_assert(ud_init(&doc) == 1);
  test_assert(ud_open(&doc, argv[1]) == 1);
  test_assert(ud_parse(&doc) == 1);
  test_assert(ud_free(&doc) == 1);
  
  return 0;
}
