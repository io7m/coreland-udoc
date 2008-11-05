#ifndef UD_TABLE_H
#define UD_TABLE_H

struct ud_table {
  unsigned long ut_rows;
  unsigned long ut_cols;
  unsigned long ut_char_width;
  unsigned long ut_longest_header;
};

void ud_table_measure (struct udoc *, const struct ud_node_list *, struct ud_table *);
unsigned long ud_table_advise_width (const struct udoc *, const struct ud_table *);

#endif
