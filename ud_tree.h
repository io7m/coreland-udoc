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
  struct ud_node *head;
  struct ud_node *tail;
  unsigned long size;
};

struct ud_node {
  enum ud_node_type type;
  union {
    char *str;
    char *sym;
    struct ud_node_list list;
  } data;
  struct ud_node *next;
  unsigned long line_num;
};

/* section of document */
struct ud_section {
  const struct ud_node_list *root;
  struct array footnotes;
  unsigned long depth;    /* how many section tags present above this one */
};

/* base tree structure */
struct ud_tree {
  struct ud_node_list root;
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
  enum ud_tree_walk_stat (*init)(struct udoc *, struct ud_tree_ctx *);
  enum ud_tree_walk_stat (*list)(struct udoc *, struct ud_tree_ctx *);
  enum ud_tree_walk_stat (*include)(struct udoc *, struct ud_tree_ctx *);
  enum ud_tree_walk_stat (*symbol)(struct udoc *, struct ud_tree_ctx *);
  enum ud_tree_walk_stat (*string)(struct udoc *, struct ud_tree_ctx *);
  enum ud_tree_walk_stat (*list_end)(struct udoc *, struct ud_tree_ctx *);
  enum ud_tree_walk_stat (*finish)(struct udoc *, struct ud_tree_ctx *);
  void (*error)(struct udoc *, struct ud_tree_ctx *);
};
struct ud_tree_ctx_state {
  unsigned long list_pos;
  unsigned long list_depth;
  const struct ud_node *node;
  const struct ud_node_list *list;
  struct ud_tag_stack tag_stack;
  void *user_data;
};
struct ud_tree_ctx {
  const struct ud_tree_ctx_funcs *funcs;
  struct ud_tree_ctx_state *state;
};

#if defined(UDOC_IMPLEMENTATION)
int ud_tree_walk(struct udoc *, struct ud_tree_ctx *);
int ud_list_cat(struct ud_node_list *, const struct ud_node *);
unsigned long ud_list_len(const struct ud_node *);
#endif

#endif
