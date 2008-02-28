#ifndef UD_TREE_H
#define UD_TREE_H

#include "ud_tag.h"

struct udoc;

enum ud_node_type {
  UDOC_TYPE_SYMBOL,
  UDOC_TYPE_STRING,
  UDOC_TYPE_LIST,
  UDOC_TYPE_INCLUDE,
};

struct ud_node_list {
  struct ud_node *unl_head;
  struct ud_node *unl_tail;
  unsigned long unl_size;
};

struct ud_node {
  enum ud_node_type un_type;
  union {
    char *un_str;
    char *un_sym;
    struct ud_node_list un_list;
  } un_data;
  struct ud_node *un_next;
  unsigned long un_line_num;
};

/* section of document */
struct ud_section {
  const struct ud_node_list *us_root;
  struct array us_footnotes;
  unsigned long us_depth;    /* how many section tags present above this one */
};

/* base tree structure */
struct ud_tree {
  struct ud_node_list ut_root;
};

/* status enumeration for tree walking */
enum ud_tree_walk_stat {
  UD_TREE_FAIL,               /* stop processing, call error, return fail */
  UD_TREE_OK,                 /* continue processing */
  UD_TREE_STOP,               /* stop processing, return success */
  UD_TREE_STOP_LIST,          /* do not continue current list */
};

/* context structure for tree walking */
struct ud_tree_ctx;
struct ud_tree_ctx_funcs {
  enum ud_tree_walk_stat (*utcf_init)(struct udoc *, struct ud_tree_ctx *);
  enum ud_tree_walk_stat (*utcf_list)(struct udoc *, struct ud_tree_ctx *);
  enum ud_tree_walk_stat (*utcf_include)(struct udoc *, struct ud_tree_ctx *);
  enum ud_tree_walk_stat (*utcf_symbol)(struct udoc *, struct ud_tree_ctx *);
  enum ud_tree_walk_stat (*utcf_string)(struct udoc *, struct ud_tree_ctx *);
  enum ud_tree_walk_stat (*utcf_list_end)(struct udoc *, struct ud_tree_ctx *);
  enum ud_tree_walk_stat (*utcf_finish)(struct udoc *, struct ud_tree_ctx *);
  void (*utcf_error)(struct udoc *, struct ud_tree_ctx *);
};
struct ud_tree_ctx_state {
  unsigned long utc_list_pos;
  unsigned long utc_list_depth;
  const struct ud_node *utc_node;
  const struct ud_node_list *utc_list;
  struct ud_tag_stack utc_tag_stack;
  void *utc_user_data;
};
struct ud_tree_ctx {
  const struct ud_tree_ctx_funcs *utc_funcs;
  struct ud_tree_ctx_state *utc_state;
};

#if defined(UDOC_IMPLEMENTATION)
int ud_tree_walk(struct udoc *, struct ud_tree_ctx *);
int ud_list_cat(struct ud_node_list *, const struct ud_node *);
unsigned long ud_list_len(const struct ud_node *);
#endif

#endif
