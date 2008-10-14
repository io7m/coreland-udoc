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
  enum ud_tree_walk_stat (*urf_init_once)(struct udoc *, struct udr_ctx *);
  enum ud_tree_walk_stat (*urf_init)(struct udoc *, struct udr_ctx *);
  enum ud_tree_walk_stat (*urf_file_init)(struct udoc *, struct udr_ctx *);
  enum ud_tree_walk_stat (*urf_list)(struct udoc *, struct udr_ctx *);
  enum ud_tree_walk_stat (*urf_symbol)(struct udoc *, struct udr_ctx *);
  enum ud_tree_walk_stat (*urf_string)(struct udoc *, struct udr_ctx *);
  enum ud_tree_walk_stat (*urf_list_end)(struct udoc *, struct udr_ctx *);
  enum ud_tree_walk_stat (*urf_file_finish)(struct udoc *, struct udr_ctx *);
  enum ud_tree_walk_stat (*urf_finish)(struct udoc *, struct udr_ctx *);
  enum ud_tree_walk_stat (*urf_finish_once)(struct udoc *, struct udr_ctx *);
  void (*error)(struct udoc *, struct udr_ctx *);
};

struct udr_opts {
  unsigned long uo_split_hint;  /* original partition threshold, used as a hint to renderers */
  unsigned long uo_page_width;  /* only relevant for character-based renderers */
};

/* default values for structure */
#define UDR_OPTS_INIT {(0), (80)}

struct udr_output_ctx {
  char uoc_cbuf[BUFFER_OUTSIZE];
  char uoc_fbuf[256];
  struct buffer uoc_buffer;
  struct sstring uoc_file;
  struct dfo_put uoc_dfo;
};

struct udr_ctx {
  const struct ud_tree_ctx *uc_tree_ctx;
  const struct ud_renderer *uc_render;
  const struct ud_part *uc_part;
  const struct udr_opts *uc_opts;
  struct udr_output_ctx *uc_out;
  struct ud_part_index_stack uc_part_stack;
  void *uc_user_data;
  unsigned int uc_flag_init_once_done;
  unsigned int uc_flag_finish_file;
  unsigned int uc_flag_split;
  unsigned int uc_finish_once_refcount;
};

struct ud_renderer {
  const struct udr_funcs ur_funcs;
  struct {
    const char *ur_name;
    const char *ur_suffix;
    const char *ur_desc;
    int ur_part;
  } ur_data;
};

int ud_render_doc (struct udoc *, const struct udr_opts *, const struct ud_renderer *, const char *);
int ud_render_node (struct udoc *, struct udr_ctx *, const struct ud_node_list *);

#if defined(UDOC_IMPLEMENTATION)
int udr_print_file (struct udoc *, struct udr_ctx *, const char *,
  int (*)(struct buffer *, const char *, unsigned long, void *), void *);
#endif

extern const struct ud_renderer ud_render_context;
extern const struct ud_renderer ud_render_debug;
extern const struct ud_renderer ud_render_nroff;
extern const struct ud_renderer ud_render_null;
extern const struct ud_renderer ud_render_plain;
extern const struct ud_renderer ud_render_xhtml;

#endif
