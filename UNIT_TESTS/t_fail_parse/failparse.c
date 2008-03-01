#include "../t_assert.h"
#include "../../udoc.h"
#include "../../log.h"

void
errors(struct udoc *doc)
{
  unsigned long ind;
  unsigned long max;
  struct ud_err *ue;

  max = ud_error_size(&doc->ud_errors);
  for (ind = 0; ind < max; ++ind) {
    if (ud_error_pop(&doc->ud_errors, &ue))
      ud_error_display(doc, ue);
  }
}

int
main(int argc, char *argv[])
{
  struct udoc doc;

  log_level(6);

  test_assert(argc == 2);
  test_assert(ud_init(&doc) == 1);
  test_assert(ud_open(&doc, argv[1]) == 1);
  test_assert(ud_parse(&doc) == 0);

  errors(&doc);

  test_assert(ud_free(&doc) == 1);
  return 0;
}
