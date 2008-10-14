#include <stdlib.h>
#include <corelib/str.h>

#include "../t_assert.h"
#include "../../udoc.h"
#include "../../log.h"

static const struct ud_renderer *renderers[] = {
  &ud_render_xhtml,
  &ud_render_context,
  &ud_render_plain,
  &ud_render_nroff,
  &ud_render_null,
};

struct udoc doc;
struct udoc_opts main_opts;
struct udr_opts r_opts = UDR_OPTS_INIT;

int
main(int argc, char *argv[])
{
  const struct ud_renderer *r = 0;
  unsigned long num;
  const char *r_name;
  const char *file;

  log_level(6);

  test_assert(argc == 4);

  file = argv[1];
  r_name = argv[2];
  main_opts.ud_split_thresh = atoi(argv[3]);

  for (num = 0; num < sizeof(renderers) / sizeof(renderers[0]); ++num)
    if (str_same(renderers[num]->ur_data.ur_name, argv[2])) {
      r = renderers[num];
      break;
    }
  test_assert(r != 0);

  test_assert(ud_init(&doc) == 1);

  doc.ud_opts = main_opts;

  test_assert(ud_open(&doc, argv[1]) == 1);
  test_assert(ud_parse(&doc) == 1);
  test_assert(ud_validate(&doc) == 1);
  test_assert(ud_partition(&doc) == 1);
  test_assert(ud_render_doc(&doc, &r_opts, r, ".") == 1);
  test_assert(ud_free(&doc) == 1);
  
  return 0;
}
