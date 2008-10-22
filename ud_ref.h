#ifndef UD_REF_H
#define UD_REF_H

struct ud_ref {
  const struct ud_node_list *ur_list;
  const struct ud_node *ur_node;
  long ur_part_index;
};

#define UD_REF_PART_UNDEFINED (-1)

int ud_ref_add (struct ud_ordered_ht *, const struct ud_ref *);
int ud_ref_add_byname (struct ud_ordered_ht *, const char *, const struct ud_ref *);
int ud_ref_add_conditional (struct ud_ordered_ht *, const struct ud_ref *);

#endif
