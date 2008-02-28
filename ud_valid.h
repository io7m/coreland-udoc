#ifndef UD_VALID_H
#define UD_VALID_H

struct ud_markup_rule {
  enum ud_tag tag;
  const enum ud_node_type *arg_types;
  unsigned long req_args;
  unsigned long max_args;
};

#endif
