#include <corelib/bin.h>
#include <corelib/buffer.h>
#include <corelib/error.h>
#include <corelib/fmt.h>
#include <corelib/open.h>
#include <corelib/sstring.h>
#include <corelib/str.h>
#include <corelib/write.h>

#define UDOC_IMPLEMENTATION
#include "udoc.h"
#include "ud_assert.h"
#include "log.h"
#include "multi.h"

static int
r_output_open (struct udoc *ud, const struct ud_renderer *renderer,
  struct udr_output_ctx *out, const struct ud_part *uc_part)
{
  char cnum[FMT_ULONG];

  /* format filename based on part index and suffix */
  cnum[fmt_ulong (cnum, uc_part->up_index_cur)] = 0;
  sstring_init (&out->uoc_file, out->uoc_fbuf, sizeof (out->uoc_fbuf));
  sstring_cats3 (&out->uoc_file, cnum, ".", renderer->ur_data.ur_suffix);
  sstring_0 (&out->uoc_file);

  buffer_init (&out->uoc_buffer, (buffer_op) write, -1, out->uoc_cbuf, sizeof (out->uoc_cbuf));

  log_2x (LOG_NOTICE, "create ", out->uoc_file.s);

  out->uoc_buffer.fd = open_trunc (out->uoc_file.s);
  ud_try_sys_jump (ud, out->uoc_buffer.fd != -1, FAIL, "open");
  return 1;

  FAIL:
  close (out->uoc_buffer.fd);
  return 0;
}

static int
r_output_close (struct udoc *ud, struct udr_output_ctx *out)
{
  ud_try_sys (ud, buffer_flush (&out->uoc_buffer) != -1, 0, "write");
  ud_try_sys (ud, close (out->uoc_buffer.fd) != -1, 0, "close");
  return 1;
}

static enum ud_tree_walk_stat
r_init (struct udoc *ud, struct ud_tree_ctx *tree_ctx)
{
  struct udr_ctx *render_ctx = tree_ctx->utc_state->utc_user_data;
  enum ud_tree_walk_stat ret;

  /* has init_once callback been called? call if not */
  if (!render_ctx->uc_flag_init_once_done) {
    ret = (render_ctx->uc_render->ur_funcs.urf_init_once)
         ? render_ctx->uc_render->ur_funcs.urf_init_once (ud, render_ctx) : UD_TREE_OK;
    if (ret != UD_TREE_OK) return ret;
    render_ctx->uc_flag_init_once_done = 1;
  }

  /* increment for every init, decrement for every finish.
   * will reach 0 at the last call to r_finish ()
   */

  if (render_ctx->uc_render->ur_funcs.urf_finish_once)
    ++render_ctx->uc_finish_once_refcount;

  /* call init () callback */
  ret = (render_ctx->uc_render->ur_funcs.urf_init)
       ? render_ctx->uc_render->ur_funcs.urf_init (ud, render_ctx) : UD_TREE_OK;
  if (ret != UD_TREE_OK) return ret;

  /* at start of file? */
  if (render_ctx->uc_part->up_list == tree_ctx->utc_state->utc_list) {
    log_1xf (LOG_DEBUG, "file init");
    return (render_ctx->uc_render->ur_funcs.urf_file_init) ?
            render_ctx->uc_render->ur_funcs.urf_file_init (ud, render_ctx) : UD_TREE_OK;
  }

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
r_list (struct udoc *ud, struct ud_tree_ctx *tree_ctx)
{
  struct udr_ctx *render_ctx = tree_ctx->utc_state->utc_user_data;
  return (render_ctx->uc_render->ur_funcs.urf_list)
        ? render_ctx->uc_render->ur_funcs.urf_list (ud, render_ctx) : UD_TREE_OK;
}

static enum ud_tree_walk_stat
r_symbol (struct udoc *ud, struct ud_tree_ctx *tree_ctx)
{
  struct udr_output_ctx out;
  struct udr_ctx rtmp;
  struct udr_ctx *render_ctx = tree_ctx->utc_state->utc_user_data;
  struct ud_part *new_part;
  const struct ud_node *cur_node;
  unsigned long index = 0;
  enum ud_tag tag;

  if (tree_ctx->utc_state->utc_list_pos == 0) {
    cur_node = tree_ctx->utc_state->utc_node;
    if (!ud_tag_by_name (cur_node->un_data.un_sym, &tag)) return UD_TREE_OK;
    switch (tag) {
      case UDOC_TAG_SUBSECTION:
      case UDOC_TAG_SECTION:
        ud_assert_s (ud_part_getfromnode (ud, cur_node, &new_part, &index),
          "could not get part for node");

        /* split? */
        if ((new_part != render_ctx->uc_part) &&
            (new_part->up_flags & UD_PART_SPLIT)) {
          log_1xf (LOG_DEBUG, "starting part split");

          /* switch to output dir, open output file, restore dir */
          ud_try_sys (ud, fchdir (ud->ud_dirfd_out) != -1, UD_TREE_FAIL, "fchdir");
          if (!r_output_open (ud, render_ctx->uc_render, &out, new_part)) return UD_TREE_FAIL;
          ud_try_sys (ud, fchdir (ud->ud_dirfd_src) != -1, UD_TREE_FAIL, "fchdir");

          /* setup new rendering context */
          rtmp = *render_ctx;
          rtmp.uc_out = &out;
          rtmp.uc_tree_ctx = 0;
          rtmp.uc_part = new_part;
          rtmp.uc_flag_finish_file = 1;

          /* tell current renderer that split is being handled */
          render_ctx->uc_flag_split = 1;

          if (!ud_render_node (ud, &rtmp, new_part->up_list)) return UD_TREE_FAIL;
          log_1xf (LOG_DEBUG, "finished part split");
          if (!r_output_close (ud, &out)) return UD_TREE_FAIL;
          return UD_TREE_STOP_LIST;
        } else {
          /* push new part on stack */
          ud_try_sys (ud, ud_part_index_stack_push (&render_ctx->uc_part_stack,
            &render_ctx->uc_part->up_index_cur), UD_TREE_FAIL, "stack_push");

          log_1xf (LOG_DEBUG, "pushed part");
          render_ctx->uc_part = new_part;
        }
        break;
      default:
        break;
    }
  }
  return (render_ctx->uc_render->ur_funcs.urf_symbol)
        ? render_ctx->uc_render->ur_funcs.urf_symbol (ud, render_ctx) : UD_TREE_OK;
}

static enum ud_tree_walk_stat
r_string (struct udoc *ud, struct ud_tree_ctx *tree_ctx)
{
  struct udr_ctx *render_ctx = tree_ctx->utc_state->utc_user_data;

  /* only pass string to renderer if the first node was a symbol */
  if (tree_ctx->utc_state->utc_list->unl_head->un_type == UDOC_TYPE_SYMBOL) {
    return (render_ctx->uc_render->ur_funcs.urf_string)
      ? render_ctx->uc_render->ur_funcs.urf_string (ud, render_ctx) : UD_TREE_OK;
  } else
    return UD_TREE_OK;
}

static enum ud_tree_walk_stat
r_list_end (struct udoc *ud, struct ud_tree_ctx *tree_ctx)
{
  struct udr_ctx *render_ctx = tree_ctx->utc_state->utc_user_data;
  unsigned long *index_ptr;
  enum ud_tree_walk_stat retcode;
  enum ud_tag tag;
  int section = 0;

  if (ud_tag_by_name (tree_ctx->utc_state->utc_list->unl_head->un_data.un_sym, &tag))
    switch (tag) {
      case UDOC_TAG_SUBSECTION:
      case UDOC_TAG_SECTION:
        section = 1;
        break;
     default:
        break;
    }

  /* section was closed by previous rendering run, don't close twice */
  if (section && render_ctx->uc_flag_split) {
    log_1xf (LOG_DEBUG, "section closed by previous render");
    render_ctx->uc_flag_split = 0;
    return UD_TREE_OK;
  }

  /* call list_end if defined */
  retcode = (render_ctx->uc_render->ur_funcs.urf_list_end) ?
    render_ctx->uc_render->ur_funcs.urf_list_end (ud, render_ctx) : UD_TREE_OK;

  /* pop part from stack, if necessary */
  if (section) {
    ud_assert (ud_part_index_stack_size (&render_ctx->uc_part_stack) > 1);
    ud_assert (ud_part_index_stack_pop (&render_ctx->uc_part_stack, &index_ptr));
    ud_assert (ud_oht_get_index (&ud->ud_parts, *index_ptr, (void *) &render_ctx->uc_part));
    log_1xf (LOG_DEBUG, "popped part");
  }
  return retcode;
}

static enum ud_tree_walk_stat
r_finish (struct udoc *ud, struct ud_tree_ctx *tree_ctx)
{
  struct udr_ctx *render_ctx = tree_ctx->utc_state->utc_user_data;
  enum ud_tree_walk_stat ret;

  /* called once per file */
  if (render_ctx->uc_flag_finish_file)
    if (render_ctx->uc_render->ur_funcs.urf_file_finish) {
      log_1xf (LOG_DEBUG, "file finish");
      return render_ctx->uc_render->ur_funcs.urf_file_finish (ud, render_ctx);
    }

  /* decrement refcount for finish_once (), if zero, call it */
  if (render_ctx->uc_render->ur_funcs.urf_finish_once) {
    --render_ctx->uc_finish_once_refcount;
    if (!render_ctx->uc_finish_once_refcount) {
      ret = render_ctx->uc_render->ur_funcs.urf_finish_once (ud, render_ctx);
      if (ret != UD_TREE_OK) return ret;
    }
  }

  return (render_ctx->uc_render->ur_funcs.urf_finish)
        ? render_ctx->uc_render->ur_funcs.urf_finish (ud, render_ctx) : UD_TREE_OK;
}

static const struct ud_tree_ctx_funcs render_funcs = {
  r_init,
  r_list,
  0,
  r_symbol,
  r_string,
  r_list_end,
  r_finish,
  0,
};

/* public */

int
ud_render_node (struct udoc *ud, struct udr_ctx *cur_render_ctx,
  const struct ud_node_list *root)
{
  struct ud_tree_ctx tree_ctx;
  struct ud_tree_ctx_state tree_ctx_state;
  struct udr_ctx new_render_ctx;
  enum ud_tree_walk_stat ret;

  log_1xf (LOG_DEBUG, "rendering");

  /* sanity - ensure clean rendering context */
  ud_assert_s (cur_render_ctx->uc_tree_ctx == 0, "passed tree_ctx is not null");

  /* new clean tree_ctx */
  bin_zero (&tree_ctx, sizeof (tree_ctx));
  bin_zero (&tree_ctx_state, sizeof (tree_ctx_state));

  /* init render context and stack */
  new_render_ctx = *cur_render_ctx;
  ud_try_sys (ud, ud_part_index_stack_init (&new_render_ctx.uc_part_stack,
    ud_oht_size (&ud->ud_parts)), UD_TREE_FAIL, "ud_part_index_stack_init");

  /* push current part index onto stack */
  ud_try_sys (ud, ud_part_index_stack_push (&new_render_ctx.uc_part_stack,
    &new_render_ctx.uc_part->up_index_cur), UD_TREE_FAIL, "stack_push");

  /* set flags and tree_ctx */
  new_render_ctx.uc_tree_ctx = &tree_ctx;
  new_render_ctx.uc_flag_split = 0;

  /* setup new tree context based on given root list.
   * renderer backend is passed as user data to tree_ctx.
   */

  tree_ctx_state.utc_list = root;
  tree_ctx_state.utc_user_data = &new_render_ctx;
  tree_ctx.utc_funcs = &render_funcs;
  tree_ctx.utc_state = &tree_ctx_state;

  /* walk */
  ret = ud_tree_walk (ud, &tree_ctx);

  /* the uc_part stack will only be empty if everything was successful */
  if (ret != UD_TREE_FAIL) {
    ud_part_index *part_index;
    ud_assert (ud_part_index_stack_size (&new_render_ctx.uc_part_stack) == 1);
    ud_part_index_stack_peek (&new_render_ctx.uc_part_stack, &part_index);
    ud_assert (*part_index == new_render_ctx.uc_part->up_index_cur);
  }

  ud_part_index_stack_free (&new_render_ctx.uc_part_stack);

  log_1xf (LOG_DEBUG, "rendering done");
  return ret != UD_TREE_FAIL;
}

int
ud_render_doc (struct udoc *ud, const struct udr_opts *opts,
  const struct ud_renderer *renderer, const char *outdir)
{
  const struct ud_part *root_part;
  struct udr_output_ctx out;
  struct udr_ctx render_ctx;

  log_1xf (LOG_DEBUG, "rendering");

  /* switch to starting dir */
  ud_try_sys_jump (ud, fchdir (ud->ud_dirfd_pwd) != -1, FAIL, "fchdir");

  /* open output dir if necessary */
  if (ud->ud_dirfd_out == -1) {
    ud->ud_dirfd_out = open_ro (outdir);
    ud_try_sys_jump (ud, ud->ud_dirfd_out != -1, FAIL, outdir);
  }

  /* switch to output dir */
  ud_try_sys_jump (ud, fchdir (ud->ud_dirfd_out) != -1, FAIL, "fchdir");

  /* fetch document root part */
  ud_try_jumpS (ud, ud_oht_get_index (&ud->ud_parts, 0, (void *) &root_part), FAIL,
    "ud_oht_get_index", "could not get root part");

  /* open output document */
  if (!r_output_open (ud, renderer, &out, root_part)) goto FAIL;

  /* switch to source dir */
  ud_try_sys_jump (ud, fchdir (ud->ud_dirfd_src) != -1, FAIL, "fchdir");

  /* setup rendering context */
  bin_zero (&render_ctx, sizeof (render_ctx));
  render_ctx.uc_tree_ctx = 0;
  render_ctx.uc_render = renderer;
  render_ctx.uc_part = root_part;
  render_ctx.uc_opts = opts;
  render_ctx.uc_out = &out;
  render_ctx.uc_flag_finish_file = 1;

  /* start rendering */
  if (!ud_render_node (ud, &render_ctx, root_part->up_list)) return 0;
  if (!r_output_close (ud, &out)) return 0;

  /* switch back to starting dir */
  ud_try_sys_jump (ud, fchdir (ud->ud_dirfd_pwd) != -1, FAIL, "fchdir");
  return 1;

  FAIL:
  return 0;
}

/*
 * insert an arbitrary file into a rendering in a possibly
 * backend-specific manner.
 */

int
udr_print_file (struct udoc *ud, struct udr_ctx *rc, const char *file,
  int (*put) (struct buffer *, const char *, unsigned long, void *), void *data)
{
  char fbuf[1024];
  char bbuf[1024];
  struct sstring sstr = sstring_INIT (fbuf);
  struct buffer in = buffer_INIT (read, -1, bbuf, sizeof (bbuf));
  unsigned long pos;
  long r;
  char *x;
  int ret = 0;

  ud_try_sys_jump (ud, fchdir (ud->ud_dirfd_pwd) != -1, END, "fchdir");

  /* if file does not contain '.' then add renderer suffix */
  sstring_trunc (&sstr);
  sstring_cats (&sstr, file);
  if (!str_rchar (file, '.', &pos))
    sstring_cats2 (&sstr, ".", rc->uc_render->ur_data.ur_suffix);
  sstring_0 (&sstr);

  /* open input file and copy contents */
  in.fd = open_ro (sstr.s);
  if (in.fd != -1) {
    for (;;) {
      r = buffer_feed (&in);
      if (r == 0) break;
      if (r == -1) { log_2sys (LOG_ERROR, "read: ", sstr.s); goto END; }
      x = buffer_peek (&in);
      if (put)
        put (&rc->uc_out->uoc_buffer, x, r, data);
      else
        buffer_put (&rc->uc_out->uoc_buffer, x, r);
      buffer_seek (&in, r);
    }
  } else
    log_2sys (LOG_WARN, "open: ", sstr.s);

  if (in.fd != -1)
    if (close (in.fd) == -1)
      log_2sys (LOG_WARN, "close: ", sstr.s);

  ud_try_sys_jump (ud, fchdir (ud->ud_dirfd_src) != -1, END, "fchdir");
  ret = 1;

  END:
  return ret;
}

