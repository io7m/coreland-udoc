#ifndef UD_TABLE_H
#define UD_TABLE_H

struct ud_table {
  unsigned long rows;
  unsigned long cols;
};

void ud_table_measure(const struct ud_node_list *, struct ud_table *);

#endif
