#ifndef UD_REF_H
#define UD_REF_H

struct ud_ref {
  const struct ud_node_list *list;
  const struct ud_node *node;
  const struct ud_part *part;
};

int ud_ref_add(struct ud_ordered_ht *, const struct ud_ref *);
int ud_ref_add_byname(struct ud_ordered_ht *, const char *, const struct ud_ref *);

#endif
