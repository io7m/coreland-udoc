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
  unsigned long split_thresh;
};

struct udoc {
  struct ud_tree tree;
  struct tokenizer tok;
  const char *name;
  const char *encoding;
  const char *render_header;
  const char *render_footer;
  unsigned long nodes;
  unsigned long depth;
  struct ud_ordered_ht link_exts;
  struct ud_ordered_ht parts;
  struct ud_ordered_ht refs;
  struct ud_ordered_ht ref_names;
  struct ud_ordered_ht footnotes;
  struct ud_ordered_ht styles;
  struct udoc_opts opts;
  int dirfd_pwd;
  int dirfd_src;
  int dirfd_out;
  struct taia time_start;
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
