#include <corelib/array.h>
#include <corelib/hashtable.h>
#include <corelib/str.h>
#include "ud_oht.h"

int
ud_oht_add(struct ud_ordered_ht *tab, const void *key,
                  unsigned long klen, const void *data)
{
  unsigned long index;

  if (ht_checkb(&tab->uht_ht, key, klen)) return 0;
  if (!array_cat(&tab->uht_array, data)) return -1;

  index = array_SIZE(&tab->uht_array) - 1;
  if (!ht_addb(&tab->uht_ht, key, klen, &index, sizeof(index))) {
    array_chop(&tab->uht_array, array_SIZE(&tab->uht_array) - 1);
    return -1;
  }
  return 1;
}

int
ud_oht_get(const struct ud_ordered_ht *tab, const void *key,
           unsigned long klen, void **rp, unsigned long *rind)
{
  unsigned long *index;
  unsigned long sz;
  void *ref;

  if (ht_getb(&tab->uht_ht, key, klen, (void *) &index, &sz)) {
    ref = array_index(&tab->uht_array, *index);
    if (ref) {
      *rind = *index;
      *rp = ref;
      return 1;
    }
  }
  return 0;
}

int
ud_oht_get_index(const struct ud_ordered_ht *tab, unsigned long index, void **rp)
{
  void *ref = array_index(&tab->uht_array, index);
  if (ref) {
    *rp = ref;
    return 1;
  } else
    return 0;
}

int
ud_oht_init(struct ud_ordered_ht *tab, unsigned int es)
{
  if (!ht_init(&tab->uht_ht)) return 0;
  if (!array_init(&tab->uht_array, 16, es)) { ht_free(&tab->uht_ht); return 0; }
  return 1;
}

unsigned long
ud_oht_size(const struct ud_ordered_ht *tab)
{
  return array_SIZE(&tab->uht_array);
}

void
ud_oht_free(struct ud_ordered_ht *tab)
{
  array_free(&tab->uht_array);
  ht_free(&tab->uht_ht);
}
