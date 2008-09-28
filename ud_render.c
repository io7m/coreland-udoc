#include <corelib/bin.h>
#include <corelib/buffer.h>
#include <corelib/error.h>
#include <corelib/fmt.h>
#include <corelib/open.h>
#include <corelib/sstring.h>
#include <corelib/str.h>
#include <corelib/write.h>

#include <stdio.h>

#define UDOC_IMPLEMENTATION
#include "udoc.h"
#include "ud_assert.h"
#include "log.h"
#include "multi.h"

static int
r_output_open(struct udoc *ud, const struct ud_renderer *r,
  struct udr_output_ctx *out, const struct ud_part *uc_part)
{
  char cnum[FMT_ULONG];

  cnum[fmt_ulong(cnum, uc_part->up_index_cur)] = 0;
  sstring_init(&out->uoc_file, out->uoc_fbuf, sizeof(out->uoc_fbuf));
  sstring_cats3(&out->uoc_file, cnum, ".", r->ur_data.ur_suffix);
  sstring_0(&out->uoc_file);

  buffer_init(&out->uoc_buf, (buffer_op) write, -1, out->uoc_cbuf, sizeof(out->uoc_cbuf));

  log_2x(LOG_NOTICE, "create ", out->uoc_file.s);

  out->uoc_buf.fd = open_trunc(out->uoc_file.s);
  ud_try_sys_jump(ud, out->uoc_buf.fd != -1, FAIL, "open");
  return 1;

  FAIL:
  close(out->uoc_buf.fd);
  return 0;
}

static int
r_output_close(struct udoc *ud, struct udr_output_ctx *out)
{
  ud_try_sys(ud, buffer_flush(&out->uoc_buf) != -1, 0, "write");
  ud_try_sys(ud, close(out->uoc_buf.fd) != -1, 0, "close");
  return 1;
}

static enum ud_tree_walk_stat
r_init(struct udoc *ud, struct ud_tree_ctx *tctx)
{
  struct udr_ctx *r = tctx->utc_state->utc_user_data;
  enum ud_tree_walk_stat ret;

  if (!r->uc_init_once_done) {
    ret = (r->uc_render->ur_funcs.urf_init_once)
         ? r->uc_render->ur_funcs.urf_init_once(ud, r) : UD_TREE_OK;
    if (ret != UD_TREE_OK) return ret;
    r->uc_init_once_done = 1;
  }

  /* will reach 0 at the last call to r_finish() */
  if (r->uc_render->ur_funcs.urf_finish_once)
    ++r->uc_finish_once_refcount;

  ret = (r->uc_render->ur_funcs.urf_init)
       ? r->uc_render->ur_funcs.urf_init(ud, r) : UD_TREE_OK;
  if (ret != UD_TREE_OK) return ret;

  if (r->uc_part->up_list == tctx->utc_state->utc_list) {
    log_1xf(LOG_DEBUG, "file init");
    return (r->uc_render->ur_funcs.urf_file_init) ?
            r->uc_render->ur_funcs.urf_file_init(ud, r) : UD_TREE_OK;
  }

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
r_list(struct udoc *ud, struct ud_tree_ctx *tctx)
{
  struct udr_ctx *r = tctx->utc_state->utc_user_data;
  return (r->uc_render->ur_funcs.urf_list)
        ? r->uc_render->ur_funcs.urf_list(ud, r) : UD_TREE_OK;
}

static enum ud_tree_walk_stat
r_include(struct udoc *ud, struct ud_tree_ctx *tctx)
{
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
r_symbol(struct udoc *ud, struct ud_tree_ctx *tctx)
{
  struct udr_output_ctx out;
  struct udr_ctx rtmp;
  struct udr_ctx *r = tctx->utc_state->utc_user_data;
  struct ud_part *part;
  const struct ud_node *cur_node;
  unsigned long ind = 0;
  enum ud_tag tag;

  if (tctx->utc_state->utc_list_pos == 0) {
    cur_node = tctx->utc_state->utc_node;
    if (!ud_tag_by_name(cur_node->un_data.un_sym, &tag)) return UD_TREE_OK;
    switch (tag) {
      case UDOC_TAG_SUBSECTION:
      case UDOC_TAG_SECTION:
        ud_assert_s(ud_part_getfromnode(ud, cur_node, &part, &ind),
          "could not get part for node");

        /* split? */
        if (part != r->uc_part) {
          if (part->up_flags & UD_PART_SPLIT) {
            log_1xf(LOG_DEBUG, "starting part split");

            /* switch to output dir, open output file, restore dir */
            ud_try_sys(ud, fchdir(ud->ud_dirfd_out) != -1, UD_TREE_FAIL, "fchdir");
            if (!r_output_open(ud, r->uc_render, &out, part)) return UD_TREE_FAIL;
            ud_try_sys(ud, fchdir(ud->ud_dirfd_src) != -1, UD_TREE_FAIL, "fchdir");

            /* save uc_part on stack (will be popped in r_finish) */
            ud_try_sys(ud, ud_part_ind_stack_push(&r->uc_part_stack,
              &part->up_index_cur), UD_TREE_FAIL, "stack_push");

            log_1xf(LOG_DEBUG, "pushed part");

            rtmp = *r;
            rtmp.uc_out = &out;
            rtmp.uc_tree_ctx = 0;
            rtmp.uc_part = part;
            rtmp.uc_finish_file = 1;

            /* tell current renderer that split is being handled */
            r->uc_split_flag = 1;

            if (!ud_render_node(ud, &rtmp, part->up_list)) return UD_TREE_FAIL;
            log_1xf(LOG_DEBUG, "finished part split");
            if (!r_output_close(ud, &out)) return UD_TREE_FAIL;
            return UD_TREE_STOP_LIST;
          }
        }

        /* save uc_part on stack */
        ud_try_sys(ud, ud_part_ind_stack_push(&r->uc_part_stack,
          &r->uc_part->up_index_cur), UD_TREE_FAIL, "stack_push");

        log_1xf(LOG_DEBUG, "pushed part");

        r->uc_part = part;
        break;
      default:
        break;
    }
  }
  return (r->uc_render->ur_funcs.urf_symbol)
        ? r->uc_render->ur_funcs.urf_symbol(ud, r) : UD_TREE_OK;
}

static enum ud_tree_walk_stat
r_string(struct udoc *ud, struct ud_tree_ctx *tctx)
{
  struct udr_ctx *r = tctx->utc_state->utc_user_data;

  /* only pass string to renderer if the first node was a symbol */
  if (tctx->utc_state->utc_list->unl_head->un_type == UDOC_TYPE_SYMBOL) {
    return (r->uc_render->ur_funcs.urf_string)
      ? r->uc_render->ur_funcs.urf_string(ud, r) : UD_TREE_OK;
  } else
    return UD_TREE_OK;
}

static enum ud_tree_walk_stat
r_list_end(struct udoc *ud, struct ud_tree_ctx *tctx)
{
  struct udr_ctx *r = tctx->utc_state->utc_user_data;
  unsigned long *ind_ptr;
  enum ud_tag tag;
  int section = 0;

  if (ud_tag_by_name(tctx->utc_state->utc_list->unl_head->un_data.un_sym, &tag))
    switch (tag) {
      case UDOC_TAG_SUBSECTION:
      case UDOC_TAG_SECTION:
        section = 1;
        ud_assert(ud_part_ind_stack_pop(&r->uc_part_stack, &ind_ptr));
        ud_assert(ud_oht_getind(&ud->ud_parts, *ind_ptr, (void *) &r->uc_part));
        log_1xf(LOG_DEBUG, "popped part");
     default:
        break;
    }

  if (section && r->uc_split_flag) {
    log_1xf(LOG_DEBUG, "section closed by previous render");
    r->uc_split_flag = 0;
    return UD_TREE_OK;
  }

  return (r->uc_render->ur_funcs.urf_list_end)
        ? r->uc_render->ur_funcs.urf_list_end(ud, r) : UD_TREE_OK;
}

static enum ud_tree_walk_stat
r_finish(struct udoc *ud, struct ud_tree_ctx *tctx)
{
  struct udr_ctx *r = tctx->utc_state->utc_user_data;
  enum ud_tree_walk_stat ret;

  if (r->uc_finish_file)
    if (r->uc_render->ur_funcs.urf_file_finish) {
      log_1xf(LOG_DEBUG, "file finish");
      return r->uc_render->ur_funcs.urf_file_finish(ud, r);
    }

  /* decrement refcount for finish_once(), if zero, call it */
  if (r->uc_render->ur_funcs.urf_finish_once) {
    --r->uc_finish_once_refcount;
    if (!r->uc_finish_once_refcount) {
      ret = r->uc_render->ur_funcs.urf_finish_once(ud, r);
      if (ret != UD_TREE_OK) return ret;
    }
  }

  return (r->uc_render->ur_funcs.urf_finish)
        ? r->uc_render->ur_funcs.urf_finish(ud, r) : UD_TREE_OK;
}

static const struct ud_tree_ctx_funcs render_funcs = {
  r_init,
  r_list,
  r_include,
  r_symbol,
  r_string,
  r_list_end,
  r_finish,
  0,
};

/* public */

int
ud_render_node(struct udoc *ud, struct udr_ctx *ctx,
  const struct ud_node_list *root)
{
  struct ud_tree_ctx tctx;
  struct ud_tree_ctx_state tctx_state;
  struct udr_ctx rctx;
  enum ud_tree_walk_stat ret;

  log_1xf(LOG_DEBUG, "rendering");
  ud_assert_s(ctx->uc_tree_ctx == 0, "passed tree_ctx is not null");

  /* new clean tree_ctx */
  bin_zero(&tctx, sizeof(tctx));
  bin_zero(&tctx_state, sizeof(tctx_state));

  rctx = *ctx;
  if (!ud_part_ind_stack_init(&rctx.uc_part_stack, ud_oht_size(&ud->ud_parts)))
    return 0;

  /* set flags and tree_ctx */
  rctx.uc_tree_ctx = &tctx;
  rctx.uc_split_flag = 0;

  /* backend render is passed as user data to tree_ctx */
  tctx_state.utc_list = root;
  tctx_state.utc_user_data = &rctx;
  tctx.utc_funcs = &render_funcs;
  tctx.utc_state = &tctx_state;

  /* walk */
  ret = ud_tree_walk(ud, &tctx);

  /* the uc_part stack will only be empty if everything was successful */
  if (ret != UD_TREE_FAIL)
    ud_assert(ud_part_ind_stack_size(&rctx.uc_part_stack) == 0);

  ud_part_ind_stack_free(&rctx.uc_part_stack);

  log_1xf(LOG_DEBUG, "rendering done");
  return ret != UD_TREE_FAIL;
}

int
ud_render_doc(struct udoc *ud, const struct udr_opts *opts,
  const struct ud_renderer *r, const char *outdir)
{
  const struct ud_part *p;
  struct udr_output_ctx out;
  struct udr_ctx rctx;

  log_1xf(LOG_DEBUG, "rendering");

  ud_try_sys_jump(ud, fchdir(ud->ud_dirfd_pwd) != -1, FAIL, "fchdir");

  if (ud->ud_dirfd_out == -1) {
    ud->ud_dirfd_out = open_ro(outdir);
    ud_try_sys_jump(ud, ud->ud_dirfd_out != -1, FAIL, outdir);
  }

  ud_try_sys_jump(ud, fchdir(ud->ud_dirfd_out) != -1, FAIL, "fchdir");
  ud_try_jumpS(ud, ud_oht_getind(&ud->ud_parts, 0, (void *) &p), FAIL,
    "ud_oht_getind", "could not get root part");
  if (!r_output_open(ud, r, &out, p)) goto FAIL;
  ud_try_sys_jump(ud, fchdir(ud->ud_dirfd_src) != -1, FAIL, "fchdir");

  bin_zero(&rctx, sizeof(rctx));
  rctx.uc_tree_ctx = 0;
  rctx.uc_render = r;
  rctx.uc_part = p;
  rctx.uc_opts = opts;
  rctx.uc_out = &out;
  rctx.uc_finish_file = 1;

  if (!ud_render_node(ud, &rctx, p->up_list)) return 0;
  if (!r_output_close(ud, &out)) return 0;

  ud_try_sys_jump(ud, fchdir(ud->ud_dirfd_pwd) != -1, FAIL, "fchdir");
  return 1;

  FAIL:
  return 0;
}

/*
 * insert an arbitrary file into a rendering in a possibly
 * backend-specific manner.
 */

int
udr_print_file(struct udoc *ud, struct udr_ctx *rc, const char *file,
  int (*put)(struct buffer *, const char *, unsigned long, void *), void *data)
{
  char fbuf[1024];
  char bbuf[1024];
  struct sstring sstr = sstring_INIT(fbuf);
  struct buffer in = buffer_INIT(read, -1, bbuf, sizeof(bbuf));
  unsigned long pos;
  long r;
  char *x;
  int ret = 0;

  ud_try_sys_jump(ud, fchdir(ud->ud_dirfd_pwd) != -1, END, "fchdir");

  /* if file does not contain '.' then add renderer suffix */
  sstring_trunc(&sstr);
  sstring_cats(&sstr, file);
  if (!str_rchar(file, '.', &pos))
    sstring_cats2(&sstr, ".", rc->uc_render->ur_data.ur_suffix);
  sstring_0(&sstr);

  in.fd = open_ro(sstr.s);
  if (in.fd != -1) {
    for (;;) {
      r = buffer_feed(&in);
      if (r == 0) break;
      if (r == -1) { log_2sys(LOG_ERROR, "read: ", sstr.s); goto END; }
      x = buffer_peek(&in);
      if (put)
        put(&rc->uc_out->uoc_buf, x, r, data);
      else
        buffer_put(&rc->uc_out->uoc_buf, x, r);
      buffer_seek(&in, r);
    }
  } else
    log_2sys(LOG_WARN, "open: ", sstr.s);

  if (in.fd != -1)
    if (close(in.fd) == -1)
      log_2sys(LOG_WARN, "close: ", sstr.s);

  ud_try_sys_jump(ud, fchdir(ud->ud_dirfd_src) != -1, END, "fchdir");
  ret = 1;

  END:
  return ret;
}

