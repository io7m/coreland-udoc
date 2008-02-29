#define UDOC_IMPLEMENTATION
#include "ud_assert.h"
#include "udoc.h"

#include "log.h"

static int rd_init_done;
static int rd_fini_done;

static enum ud_tree_walk_stat
rd_init_once(struct udoc *ud, struct udr_ctx *rc)
{
  ud_assert(rd_init_done == 0);
  rd_init_done = 1;

  log_1xf(LOG_DEBUG, 0);
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rd_init(struct udoc *ud, struct udr_ctx *rc)
{
  log_1xf(LOG_DEBUG, 0);
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rd_file_init(struct udoc *ud, struct udr_ctx *rc)
{
  log_1xf(LOG_DEBUG, 0);
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rd_symbol(struct udoc *ud, struct udr_ctx *rc)
{
  log_1xf(LOG_DEBUG, 0);
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rd_string(struct udoc *ud, struct udr_ctx *rc)
{
  log_1xf(LOG_DEBUG, 0);
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rd_list(struct udoc *ud, struct udr_ctx *rc)
{
  log_1xf(LOG_DEBUG, 0);
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rd_list_end(struct udoc *ud, struct udr_ctx *rc)
{
  log_1xf(LOG_DEBUG, 0);
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rd_finish(struct udoc *ud, struct udr_ctx *rc)
{
  log_1xf(LOG_DEBUG, 0);
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rd_file_finish(struct udoc *ud, struct udr_ctx *rc)
{
  log_1xf(LOG_DEBUG, 0);
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rd_finish_once(struct udoc *ud, struct udr_ctx *rc)
{
  ud_assert(rd_fini_done == 0);
  rd_fini_done = 1;

  log_1xf(LOG_DEBUG, 0);
  return UD_TREE_OK;
}

const struct ud_renderer ud_render_debug = {
  {
    .urf_init = rd_init,
    .urf_init_once = rd_init_once,
    .urf_file_init = rd_file_init,
    .urf_list = rd_list,
    .urf_symbol = rd_symbol,
    .urf_string = rd_string,
    .urf_list_end = rd_list_end,
    .urf_file_finish = rd_file_finish,
    .urf_finish = rd_finish,
    .urf_finish_once = rd_finish_once,
  },
  {
    .ur_name = "debug",
    .ur_suffix = "debug",
    .ur_desc = "callbacks for debugging output",
    .ur_part = 1,
  },
};
