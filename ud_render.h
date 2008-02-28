#ifndef UD_RENDER_H
#define UD_RENDER_H

#include <corelib/buffer.h>
#include <corelib/sstring.h>
#include "ud_part.h"
#include "dfo.h"

struct udoc;

/* context structure for rendering */
struct udr_ctx;
struct udr_funcs {
  enum ud_tree_walk_stat (*init_once)(struct udoc *, struct udr_ctx *);
  enum ud_tree_walk_stat (*init)(struct udoc *, struct udr_ctx *);
  enum ud_tree_walk_stat (*file_init)(struct udoc *, struct udr_ctx *);
  enum ud_tree_walk_stat (*list)(struct udoc *, struct udr_ctx *);
  enum ud_tree_walk_stat (*symbol)(struct udoc *, struct udr_ctx *);
  enum ud_tree_walk_stat (*string)(struct udoc *, struct udr_ctx *);
  enum ud_tree_walk_stat (*list_end)(struct udoc *, struct udr_ctx *);
  enum ud_tree_walk_stat (*file_finish)(struct udoc *, struct udr_ctx *);
  enum ud_tree_walk_stat (*finish)(struct udoc *, struct udr_ctx *);
  enum ud_tree_walk_stat (*finish_once)(struct udoc *, struct udr_ctx *);
  void (*error)(struct udoc *, struct udr_ctx *);
};
struct udr_opts {
  unsigned long split_hint;  /* original partition threshold, used as a hint to renderers */
  unsigned int page_width;   /* only relevant for character-based renderers */
};
struct udr_output_ctx {
  char cbuf[BUFFER_OUTSIZE];
  char fbuf[256];
  struct buffer buf;
  struct sstring file;
  struct dfo_put dfo;
};
struct udr_ctx {
  const struct ud_tree_ctx *tree_ctx;
  const struct ud_renderer *render;
  const struct ud_part *part;
  const struct udr_opts *opts;
  struct udr_output_ctx *out;
  struct ud_part_ind_stack part_stack;
  void *user_data;
  unsigned int init_once_done;
  unsigned int finish_once_done;
};
struct ud_renderer {
  const struct udr_funcs funcs;
  struct {
    const char *name;
    const char *suffix;
    const char *desc;
    int part;
  } data;
};

int ud_render_doc(struct udoc *, const struct udr_opts *, const struct ud_renderer *, const char *);
int ud_render_node(struct udoc *, struct udr_ctx *, const struct ud_node_list *);

#if defined(UDOC_IMPLEMENTATION)
int udr_print_file(struct udoc *, struct udr_ctx *, const char *,
                   int (*)(struct buffer *, const char *, unsigned long, void *),
                   void *);
#endif

extern const struct ud_renderer ud_render_dump;
extern const struct ud_renderer ud_render_nroff;
extern const struct ud_renderer ud_render_plain;
extern const struct ud_renderer ud_render_context;
extern const struct ud_renderer ud_render_xhtml;

#endif
