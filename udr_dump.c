#define UDOC_IMPLEMENTATION
#include "udoc.h"

static enum ud_tree_walk_stat
rd_init(struct udoc *ud, struct udr_ctx *rc)
{
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rd_symbol(struct udoc *ud, struct udr_ctx *rc)
{
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rd_string(struct udoc *ud, struct udr_ctx *rc)
{
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rd_list_end(struct udoc *ud, struct udr_ctx *rc)
{
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rd_finish(struct udoc *ud, struct udr_ctx *rc)
{
  return UD_TREE_OK;
}

const struct ud_renderer ud_render_dump = {
  {
    .init = rd_init,
    .file_init = 0,
    .list = 0,
    .symbol = rd_symbol,
    .string = rd_string,
    .list_end = rd_list_end,
    .file_finish = 0,
    .finish = rd_finish,
    .error = 0, 
  },
  {
    .name = "dump",
    .suffix = "dump",
    .desc = "dump various debugging and statistical output",
    .part = 0,
  },
};
