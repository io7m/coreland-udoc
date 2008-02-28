#include <corelib/array.h>
#include <corelib/hashtable.h>
#include <corelib/str.h>
#include "ud_oht.h"

int
ud_oht_add(struct ud_ordered_ht *tab, const void *key,
                  unsigned long klen, const void *data)
{
  unsigned long ind;

  if (ht_checkb(&tab->ht, key, klen)) return 0;
  if (!array_cat(&tab->array, data)) return -1;

  ind = array_size(&tab->array) - 1;
  if (!ht_addb(&tab->ht, key, klen, &ind, sizeof(ind))) {
    array_chop(&tab->array, array_size(&tab->array) - 1);
    return -1;
  }
  return 1;
}

int
ud_oht_get(const struct ud_ordered_ht *tab, const void *key,
           unsigned long klen, void **rp, unsigned long *rind)
{
  unsigned long *ind;
  unsigned long sz;
  void *ref;

  if (ht_getb(&tab->ht, key, klen, (void *) &ind, &sz)) {
    ref = array_index(&tab->array, *ind);
    if (ref) {
      *rind = *ind;
      *rp = ref;
      return 1;
    }
  }
  return 0;
}

int
ud_oht_getind(const struct ud_ordered_ht *tab, unsigned long ind, void **rp)
{
  void *ref = array_index(&tab->array, ind);
  if (ref) {
    *rp = ref;
    return 1;
  } else
    return 0;
}

int
ud_oht_init(struct ud_ordered_ht *tab, unsigned int es)
{
  if (!ht_init(&tab->ht)) return 0;
  if (!array_init(&tab->array, 16, es)) { ht_free(&tab->ht); return 0; }
  return 1;
}

unsigned long
ud_oht_size(const struct ud_ordered_ht *tab)
{
  return array_SIZE(&tab->array);
}

void
ud_oht_free(struct ud_ordered_ht *tab)
{
  array_free(&tab->array);
  ht_free(&tab->ht);
}
