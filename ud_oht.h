#ifndef UD_OHT_H
#define UD_OHT_H

#include <corelib/array.h>
#include <corelib/hashtable.h>

struct ud_ordered_ht {
  struct array array;
  struct hashtable ht;
};

#define UD_OHT_ZERO {ARRAY_ZERO,HT_ZERO}

int ud_oht_init(struct ud_ordered_ht *, unsigned int);
int ud_oht_add(struct ud_ordered_ht *, const void *, unsigned long, const void *);
int ud_oht_get(const struct ud_ordered_ht *, const void *, unsigned long, void **, unsigned long *);
int ud_oht_getind(const struct ud_ordered_ht *, unsigned long, void **);
unsigned long ud_oht_size(const struct ud_ordered_ht *);
void ud_oht_free(struct ud_ordered_ht *);

#endif
