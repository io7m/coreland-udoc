#include "../t_assert.h"
#include "../../udoc.h"
#include "../../log.h"

void
errors (struct udoc *doc)
{
  const unsigned long max = ud_error_size (&doc->ud_errors);
  unsigned long ind;
  struct ud_err *ue;

  for (ind = 0; ind < max; ++ind)
    if (ud_error_pop (&doc->ud_errors, &ue))
      ud_error_display (doc, ue);
}

int
main (int argc, char *argv[])
{
  struct udoc doc;

  log_level (6);

  test_assert (argc == 2);
  test_assert (ud_init (&doc) == 1);
  test_assert (ud_open (&doc, argv[1]) == 1);
  test_assert (ud_parse (&doc) == 1);
  test_assert (ud_validate (&doc) == 1);
  test_assert (ud_partition (&doc, 0) == 0);

  errors (&doc);

  test_assert (ud_free (&doc) == 1);
  return 0;
}
