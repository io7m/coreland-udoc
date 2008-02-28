#include <corelib/bin.h>

#define UDOC_IMPLEMENTATION
#include "udoc.h"
#include "ud_table.h"

static void
ud_row_measure(const struct ud_node_list *list, struct ud_table *udt)
{
  const struct ud_node *n = list->head;
  unsigned long cols = 0;

  for (;;) {
    if (n->type == UDOC_TYPE_LIST) ++cols;
    if (n->next)
      n = n->next;
    else
      break;
  }

  udt->cols = (cols > udt->cols) ? cols : udt->cols;
}

void
ud_table_measure(const struct ud_node_list *list, struct ud_table *udt)
{
  const struct ud_node *n = list->head;

  for (;;) {
    if (n->type == UDOC_TYPE_LIST) {
      ++udt->rows;
      ud_row_measure(&n->data.list, udt);
    }
    if (n->next)
      n = n->next;
    else
      break;
  }
}
