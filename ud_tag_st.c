#include <corelib/array.h>

#define UDOC_IMPLEMENTATION
#include "ud_tag.h"

int
ud_tag_stack_init(struct ud_tag_stack *s)
{
  return array_init(&s->sta, 1, sizeof(enum ud_tag));
}

void
ud_tag_stack_free(struct ud_tag_stack *s)
{
  array_free(&s->sta);
}

int
ud_tag_stack_push(struct ud_tag_stack *s, enum ud_tag tag)
{
  return array_cat(&s->sta, &tag);
}

int
ud_tag_stack_copy(struct ud_tag_stack *s, const struct ud_tag_stack *t)
{
  return array_copy(&s->sta, &t->sta);
}

int
ud_tag_stack_peek(const struct ud_tag_stack *s, enum ud_tag *tag)
{
  enum ud_tag *rtag;
  if (!s->sta.u) return 0;
  rtag = array_index(&s->sta, s->sta.u - 1);
  *tag = *rtag;
  return 1;
}

int
ud_tag_stack_pop(struct ud_tag_stack *s, enum ud_tag *tag)
{
  if (ud_tag_stack_peek(s, tag)) {
    array_chop(&s->sta, s->sta.u - 1);
    return 1;
  } else
    return 0;
}

const enum ud_tag *
ud_tag_stack_index(const struct ud_tag_stack *s, unsigned long ind)
{
  return array_index(&s->sta, ind);
}

unsigned long
ud_tag_stack_size(const struct ud_tag_stack *s)
{
  return array_size(&s->sta);
}

int
ud_tag_stack_above(const struct ud_tag_stack *s, enum ud_tag tag)
{
  unsigned long ind;
  unsigned long max = array_size(&s->sta);
  const enum ud_tag *rtag = 0;

  if (max)
    for (ind = 0; ind < max - 1; ++ind) {
      rtag = ud_tag_stack_index(s, ind);
      if (*rtag == tag) return 1;
    }
  return 0;
}
