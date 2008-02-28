#include <corelib/array.h>
#include <corelib/str.h>

#define UDOC_IMPLEMENTATION
#include "ud_tag.h"

int ud_tag_by_name(const char *name, enum ud_tag *tag)
{
  unsigned int ind;
  for (ind = 0; ind < ud_num_tags; ++ind) {
    if (str_same(ud_tags_by_name[ind].ut_name, name)) {
      *tag = ud_tags_by_name[ind].ut_tag;
      return 1;
    }
  }
  return 0;
}

const char *ud_tag_name(enum ud_tag tag)
{
  unsigned int ind;
  for (ind = 0; ind < ud_num_tags; ++ind) {
    if (ud_tags_by_name[ind].ut_tag == tag)
      return ud_tags_by_name[ind].ut_name;
  }
  return 0;
}
