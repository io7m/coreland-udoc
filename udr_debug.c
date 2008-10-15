#include <corelib/buffer.h>
#include <corelib/fmt.h>

#define UDOC_IMPLEMENTATION
#include "ud_assert.h"
#include "ud_oht.h"
#include "ud_ref.h"
#include "udoc.h"

#include "log.h"

/* this renderer is intended to do pedantic checking, but
   hasn't yet been filled in */

static int rd_init_done;
static int rd_fini_done;

static void
rd_dump_footnotes (const struct udoc *doc, struct buffer *out)
{
  char cnum[FMT_ULONG];
  const unsigned long max = ud_oht_size (&doc->ud_footnotes);
  const struct ud_ref *ref;
  unsigned long index;

  buffer_puts (out, "-- footnotes\n");
  for (index = 0; index < max; ++index) {
    ud_oht_get_index (&doc->ud_footnotes, index, (void *) &ref);

    buffer_put (out, cnum, fmt_ulong (cnum, index));
    buffer_puts (out, " ");

    buffer_puts (out, "file:");
    buffer_put (out, cnum, fmt_ulong (cnum, ref->ur_part->up_file));
    buffer_puts (out, " ");

    buffer_puts (out, "title:\"");
    buffer_puts (out, (ref->ur_part->up_title) ? ref->ur_part->up_title : "(null)");
    buffer_puts (out, "\"\n");
  }
  buffer_flush (out);
}

static void
rd_dump_part (struct buffer *out, const struct ud_part *part)
{
  char cnum[FMT_ULONG];

  buffer_puts (out, "part ");

  buffer_puts (out, "file:");
  buffer_put (out, cnum, fmt_ulong (cnum, part->up_file));
  buffer_puts (out, " ");

  buffer_puts (out, "depth:");
  buffer_put (out, cnum, fmt_ulong (cnum, part->up_depth));
  buffer_puts (out, " ");

  buffer_puts (out, "prev:");
  buffer_put (out, cnum, fmt_ulong (cnum, part->up_index_prev));
  buffer_puts (out, " ");

  buffer_puts (out, "cur:");
  buffer_put (out, cnum, fmt_ulong (cnum, part->up_index_cur));
  buffer_puts (out, " ");

  buffer_puts (out, "next:");
  buffer_put (out, cnum, fmt_ulong (cnum, part->up_index_next));
  buffer_puts (out, " ");

  buffer_puts (out, "parent:");
  buffer_put (out, cnum, fmt_ulong (cnum, part->up_index_parent));
  buffer_puts (out, " ");

  buffer_puts (out, "flags:");
  buffer_put (out, cnum, fmt_ulongx (cnum, part->up_flags));
  buffer_puts (out, " ");

  buffer_put (out, "\n", 1);
  buffer_flush (out);
}

static void
rd_dump_stack (struct buffer *out, const struct ud_part_index_stack *stack)
{
  char cnum[FMT_ULONG];
  const ud_part_index *indices = ud_part_index_stack_data (stack);
  const unsigned long stack_size = ud_part_index_stack_size (stack);
  unsigned long stack_index;

  buffer_puts (out, "stack size:");
  buffer_put (out, cnum, fmt_ulong (cnum, stack_size));
  buffer_put (out, " (", 2);

  for (stack_index = 0; stack_index < stack_size; ++stack_index) {
    buffer_put (out, cnum, fmt_ulong (cnum, indices [stack_index]));
    buffer_put (out, " ", 1);
  }

  buffer_put (out, ")\n", 2);
  buffer_flush (out);
}

static void
rd_debug (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct buffer *out = &render_ctx->uc_out->uoc_buffer;
  rd_dump_part (out, render_ctx->uc_part);
  rd_dump_stack (out, &render_ctx->uc_part_stack);
}

static enum ud_tree_walk_stat
rd_init_once (struct udoc *ud, struct udr_ctx *render_ctx)
{
  ud_assert (rd_init_done == 0);
  rd_init_done = 1;

  log_1xf (LOG_DEBUG, 0);

  rd_dump_footnotes (ud, &render_ctx->uc_out->uoc_buffer);
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rd_init (struct udoc *ud, struct udr_ctx *render_ctx)
{
  log_1xf (LOG_DEBUG, 0);
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rd_file_init (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct buffer *out = &render_ctx->uc_out->uoc_buffer;

  log_1xf (LOG_DEBUG, 0);

  buffer_puts (out, "-- file_init\n");
  rd_debug (ud, render_ctx);

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rd_symbol (struct udoc *ud, struct udr_ctx *render_ctx)
{
  log_1xf (LOG_DEBUG, 0);
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rd_string (struct udoc *ud, struct udr_ctx *render_ctx)
{
  log_1xf (LOG_DEBUG, 0);
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rd_list (struct udoc *ud, struct udr_ctx *render_ctx)
{
  log_1xf (LOG_DEBUG, 0);
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rd_list_end (struct udoc *ud, struct udr_ctx *render_ctx)
{
  log_1xf (LOG_DEBUG, 0);
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rd_finish (struct udoc *ud, struct udr_ctx *render_ctx)
{
  log_1xf (LOG_DEBUG, 0);
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rd_file_finish (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct buffer *out = &render_ctx->uc_out->uoc_buffer;

  log_1xf (LOG_DEBUG, 0);

  buffer_puts (out, "-- file_finish\n");
  rd_debug (ud, render_ctx);
  rd_dump_footnotes (ud, out);

  buffer_puts (out, "-- eof\n");
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rd_finish_once (struct udoc *ud, struct udr_ctx *render_ctx)
{
  ud_assert (rd_fini_done == 0);
  rd_fini_done = 1;

  log_1xf (LOG_DEBUG, 0);
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
