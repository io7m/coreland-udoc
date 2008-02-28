#include <corelib/alloc.h>
#include <corelib/array.h>
#include <corelib/bin.h>
#include <corelib/error.h>
#include <corelib/fmt.h>
#include <corelib/hashtable.h>
#include <corelib/str.h>

#include "log.h"

#define UDOC_IMPLEMENTATION
#include "ud_oht.h"
#include "ud_ref.h"
#include "udoc.h"

int
ud_ref_add(struct ud_ordered_ht *tab, const struct ud_ref *ref)
{
  switch (ud_oht_add(tab, &ref->node, sizeof(&ref->node), ref)) {
    case 0:
      log_2xf(LOG_ERROR, "duplicate ref: ", ref->node->next->data.str);
      return 0;
    case -1:
      log_1sysf(LOG_ERROR, "ud_oht_add"); return 0;
    default: return 1;
  }
}

int
ud_ref_add_byname(struct ud_ordered_ht *tab, const char *key,
                   const struct ud_ref *ref)
{
  switch (ud_oht_add(tab, key, str_len(key), ref)) {
    case 0:
      log_2xf(LOG_ERROR, "duplicate ref: ", ref->node->next->data.str);
      return 0;
    case -1:
      log_1sysf(LOG_ERROR, "ud_oht_add");
      return 0;
    default: return 1;
  }
}
