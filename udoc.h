#ifndef UDOC_H
#define UDOC_H

#include <chrono/taia.h>

#include <corelib/dstack.h>
#include <corelib/hashtable.h>
#include "token.h"

#include "ud_error.h"
#include "ud_oht.h"
#include "ud_tree.h"
#include "ud_render.h"

struct udoc_opts {
  unsigned long ud_split_thresh;
};

struct udoc {
  struct ud_tree ud_tree;
  struct tokenizer ud_tok;
  const char *ud_name;
  const char *ud_encoding;
  const char *ud_render_header;
  const char *ud_render_footer;
  unsigned long ud_nodes;
  unsigned long ud_depth;
  struct ud_ordered_ht ud_link_exts;
  struct ud_ordered_ht ud_parts;
  struct ud_ordered_ht ud_refs;
  struct ud_ordered_ht ud_ref_names;
  struct ud_ordered_ht ud_footnotes;
  struct ud_ordered_ht ud_styles;
  struct udoc_opts ud_opts;
  int ud_dirfd_pwd;
  int ud_dirfd_src;
  int ud_dirfd_out;
  struct taia ud_time_start;
};

int ud_new(struct udoc **);
int ud_init(struct udoc *);
int ud_open(struct udoc *, const char *);
int ud_parse(struct udoc *);
int ud_validate(struct udoc *);
int ud_partition(struct udoc *);
int ud_get(const char *, struct udoc **);
int ud_free(struct udoc *);

extern struct hashtable ud_documents;

#endif
