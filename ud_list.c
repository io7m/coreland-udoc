#include <corelib/alloc.h>
#include <corelib/dstack.h>
#include <corelib/hashtable.h>

#define UDOC_IMPLEMENTATION
#include "ud_tag.h"
#include "ud_tree.h"

int
ud_node_new(struct ud_node **udp)
{
  struct ud_node *un = alloc_zero(sizeof(*un));
  if (!un) return 0;
  return !!(*udp = un);
}

int
ud_list_cat(struct ud_node_list *list, const struct ud_node *un)
{
  struct ud_node *np;
  if (!ud_node_new(&np)) return 0;

  *np = *un;

  if (!list->head) {
    list->head = np;
    list->tail = 0;
    goto END;
  }
  if (!list->tail) {
    list->head->next = np;
    list->tail = np;
    goto END;
  }
  if (list->tail) {
    list->tail->next = np;
    list->tail = np;
    goto END;
  }

END:
  ++list->size;
  return 1;
}

unsigned long
ud_list_len(const struct ud_node *un)
{
  unsigned long len = 0;
  for (;;) {
    if (!un) return len;
    un = un->next;
    ++len;
  }
}
