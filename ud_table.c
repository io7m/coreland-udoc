#include <corelib/bin.h>

#define UDOC_IMPLEMENTATION
#include "udoc.h"
#include "ud_table.h"

static void
ud_row_measure (const struct ud_node_list *list, struct ud_table *udt)
{
  const struct ud_node *n = list->unl_head;
  unsigned long cols = 0;

  for (;;) {
    if (n->un_type == UDOC_TYPE_LIST) ++cols;
    if (n->un_next) n = n->un_next; else break;
  }

  udt->ut_cols = (cols > udt->ut_cols) ? cols : udt->ut_cols;
}

void
ud_table_measure (const struct ud_node_list *list, struct ud_table *udt)
{
  const struct ud_node *n = list->unl_head;

  for (;;) {
    if (n->un_type == UDOC_TYPE_LIST) {
      ++udt->ut_rows;
      ud_row_measure (&n->un_data.un_list, udt);
    }
    if (n->un_next) n = n->un_next; else break;
  }
}
