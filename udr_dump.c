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
    .urf_init = rd_init,
    .urf_file_init = 0,
    .urf_list = 0,
    .urf_symbol = rd_symbol,
    .urf_string = rd_string,
    .urf_list_end = rd_list_end,
    .urf_file_finish = 0,
    .urf_finish = rd_finish,
  },
  {
    .ur_name = "dump",
    .ur_suffix = "dump",
    .ur_desc = "dump various debugging and statistical output",
    .ur_part = 0,
  },
};
