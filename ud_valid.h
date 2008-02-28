#ifndef UD_VALID_H
#define UD_VALID_H

struct ud_markup_rule {
  enum ud_tag umr_tag;
  const enum ud_node_type *umr_arg_types;
  unsigned long umr_req_args;
  unsigned long umr_max_args;
};

#endif
