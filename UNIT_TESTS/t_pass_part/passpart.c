#include <stdlib.h>
#include <stdio.h>

#include "../t_assert.h"
#include "../../udoc.h"
#include "../../log.h"

unsigned long
files(struct ud_ordered_ht *oht)
{
  struct ud_part *p;
  unsigned long ind;
  unsigned long max = ud_oht_size(oht);
  unsigned long nfiles = 0;

  for (ind = 0; ind < max; ++ind) {
    ud_oht_getind(oht, ind, (void *) &p);
    if (p->up_flags & UD_PART_SPLIT) ++nfiles;
  }
  return nfiles;
}

void
dump(struct udoc *doc)
{
  unsigned long max;
  unsigned long ind;
  struct ud_part *p;

  max = ud_oht_size(&doc->ud_parts);
  for (ind = 0; ind < max; ++ind) {
    ud_oht_getind(&doc->ud_parts, ind, (void *) &p);
    printf("part:%lu ", ind);
    printf("file:%lu ", p->up_file);
    printf("depth:%lu ", p->up_depth);
    printf("prev:%lu ", p->up_index_prev);
    printf("cur:%lu ", p->up_index_cur);
    printf("next:%lu ", p->up_index_next);
    printf("parent:%lu ", p->up_index_parent);
    printf("flags:%.8x ", p->up_flags);
    if (p->up_title)
      printf("title:\"%s\" ", p->up_title);
    else
      printf("title:(null) ");
    if (p->up_num_string)
      printf("num_string:\"%s\" ", p->up_num_string);
    else
      printf("num_string:(null) ");
    printf("\n");
  }
}

int
main(int argc, char *argv[])
{
  struct udoc doc;
  unsigned long num_parts;
  unsigned long num_files;
  unsigned long split;
  const char *file;

  log_level(6);

  test_assert(argc == 5);

  file = argv[1];
  split = atoi(argv[2]);
  num_parts = atoi(argv[3]);
  num_files = atoi(argv[4]);

  test_assert(ud_init(&doc) == 1);

  doc.ud_opts.ud_split_thresh = split;

  test_assert(ud_open(&doc, file) == 1);
  test_assert(ud_parse(&doc) == 1);
  test_assert(ud_validate(&doc) == 1);
  test_assert(ud_partition(&doc) == 1);

  test_assert(num_parts == ud_oht_size(&doc.ud_parts));
  test_assert(num_files == files(&doc.ud_parts));

  dump(&doc);

  test_assert(ud_free(&doc) == 1);
  return 0;
}
