#ifndef UD_PART_H
#define UD_PART_H

#include <corelib/dstring.h>

enum ud_part_flags {
  UD_PART_SPLIT   = 0x0001,
  UD_PART_SUBSECT = 0x0002
};

struct udoc;
struct ud_part {
  const struct ud_node_list *up_list;
  const struct ud_node *up_node;
  unsigned long up_file;
  unsigned long up_depth;
  long up_index_prev;
  long up_index_cur;
  long up_index_next;
  long up_index_parent;
  unsigned int up_flags;
  const char *up_title;
  const char *up_num_string;
};

int ud_part_init (struct ud_part *);
int ud_part_getfromnode (const struct udoc *, const struct ud_node *, struct ud_part **, unsigned long *);
int ud_part_getroot (const struct udoc *, struct ud_part **);
int ud_part_getcur (const struct udoc *, struct ud_part **);
int ud_part_getprev (const struct udoc *, const struct ud_part *, struct ud_part **);
void ud_part_getfirst_wdepth_noskip (const struct udoc *, const struct ud_part *, struct ud_part **);
void ud_part_getfirst_wparent (const struct udoc *, const struct ud_part *, struct ud_part **);
int ud_part_getprev_up (const struct udoc *, const struct ud_part *, struct ud_part **);
int ud_part_getnext_file (const struct udoc *, const struct ud_part *, struct ud_part **);
int ud_part_getprev_file (const struct udoc *, const struct ud_part *, struct ud_part **);
int ud_part_num_fmt (const struct udoc *, const struct ud_part *, struct dstring *);

const struct ud_part *ud_part_get (const struct udoc *, unsigned long);
int ud_part_add (struct udoc *, const struct ud_part *);

void ud_part_dump (struct ud_part *);
void ud_parts_dump (struct udoc *);

#include "gen_stack.h"

/* ud_part index stack */
typedef long ud_part_index;

GEN_stack_declare(ud_part_index);

#endif
