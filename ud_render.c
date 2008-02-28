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
r_output_open(const struct ud_renderer *r, struct udr_output_ctx *out,
  const struct ud_part *uc_part)
{
  char cnum[FMT_ULONG];

  sstring_init(&out->uoc_file, out->uoc_fbuf, sizeof(out->uoc_fbuf));
  sstring_trunc(&out->uoc_file);
  sstring_catb(&out->uoc_file, cnum, fmt_ulong(cnum, uc_part->up_index_cur));
  sstring_catb(&out->uoc_file, ".", 1);
  sstring_cats(&out->uoc_file, r->ur_data.ur_suffix);
  sstring_0(&out->uoc_file);

  buffer_init(&out->uoc_buf, (buffer_op) write, -1, out->uoc_cbuf, sizeof(out->uoc_cbuf));

  log_2x(LOG_NOTICE, "create ", out->uoc_file.s);

  out->uoc_buf.fd = open_trunc(out->uoc_file.s);
  if (out->uoc_buf.fd == -1) {
    ud_error_pushsys(&ud_errors, "open");
    goto FAIL;
  }
  return 1;

  FAIL:
  close(out->uoc_buf.fd);
  return 0;
}

static int
r_output_close(struct udr_output_ctx *out)
{
  if (buffer_flush(&out->uoc_buf) == -1) {
    ud_error_pushsys(&ud_errors, "write");
    return 0;
  }
  if (close(out->uoc_buf.fd) == -1) {
    ud_error_pushsys(&ud_errors, "close");
    return 0;
  }
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

  ret = (r->uc_render->ur_funcs.urf_init)
       ? r->uc_render->ur_funcs.urf_init(ud, r) : UD_TREE_OK;
  if (ret != UD_TREE_OK) return ret;

  if (r->uc_part->up_list == tctx->utc_state->utc_list)
    return (r->uc_render->ur_funcs.urf_file_init) ?
            r->uc_render->ur_funcs.urf_file_init(ud, r) : UD_TREE_OK;

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
  struct udr_ctx *r = tctx->utc_state->utc_user_data;
  struct udr_ctx rtmp;
  struct ud_part *uc_part;
  unsigned long ind = 0;
  enum ud_tag tag;

  if (tctx->utc_state->utc_list_pos == 0) {
    if (!ud_tag_by_name(tctx->utc_state->utc_node->un_data.un_sym, &tag)) return UD_TREE_OK;
    switch (tag) {
      case UDOC_TAG_SECTION:
        if (ud_part_getfromnode(ud, tctx->utc_state->utc_node, &uc_part, &ind)) {
          if (uc_part != r->uc_part) {
            if (uc_part->up_flags & UD_PART_SPLIT) {
              log_1xf(LOG_DEBUG, "starting uc_part");
              if (fchdir(ud->ud_dirfd_out) == -1) {
                ud_error_pushsys(&ud_errors, "fchdir");
                return UD_TREE_FAIL;
              }
              if (!r_output_open(r->uc_render, &out, uc_part)) return UD_TREE_FAIL;
              if (fchdir(ud->ud_dirfd_src) == -1) {
                ud_error_pushsys(&ud_errors, "fchdir");
                return UD_TREE_FAIL;
              }
              rtmp = *r;
              rtmp.uc_tree_ctx = 0;
              if (!ud_render_node(ud, &rtmp, uc_part->up_list)) return UD_TREE_FAIL;
              if (!r_output_close(&out)) return UD_TREE_FAIL;
              return UD_TREE_STOP_LIST;
            }
          }
          /* save uc_part on stack */
          if (!ud_part_ind_stack_push(&r->uc_part_stack, &r->uc_part->up_index_cur)) {
            ud_error_pushsys(&ud_errors, "ud_part_ind_stack_push");
            return UD_TREE_FAIL;
          }
          r->uc_part = uc_part;
        } else {
          ud_error_push(&ud_errors, "ud_part_get", "could not get uc_part for node");
          return UD_TREE_FAIL;
        }
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
  return (r->uc_render->ur_funcs.urf_string)
        ? r->uc_render->ur_funcs.urf_string(ud, r) : UD_TREE_OK;
}

static enum ud_tree_walk_stat
r_list_end(struct udoc *ud, struct ud_tree_ctx *tctx)
{
  struct udr_ctx *r = tctx->utc_state->utc_user_data;
  unsigned long *ind;
  enum ud_tag tag;

  if (ud_tag_by_name(tctx->utc_state->utc_list->unl_head->un_data.un_sym, &tag))
    switch (tag) {
      case UDOC_TAG_SECTION:
        /* restore uc_part */
        ud_assert(ud_part_ind_stack_pop(&r->uc_part_stack, &ind));
        ud_assert(ud_oht_getind(&ud->ud_parts, *ind, (void *) &r->uc_part));
        break;
      default:
        break;
    }

  return (r->uc_render->ur_funcs.urf_list_end)
        ? r->uc_render->ur_funcs.urf_list_end(ud, r) : UD_TREE_OK;
}

static enum ud_tree_walk_stat
r_finish(struct udoc *ud, struct ud_tree_ctx *tctx)
{
  struct udr_ctx *r = tctx->utc_state->utc_user_data;
  enum ud_tree_walk_stat ret;

  if (r->uc_render->ur_funcs.urf_file_finish)
    if (r->uc_part->up_list == tctx->utc_state->utc_list)
      return r->uc_render->ur_funcs.urf_file_finish(ud, r);

  if (!r->uc_finish_once_done) {
    ret = (r->uc_render->ur_funcs.urf_finish_once)
         ? r->uc_render->ur_funcs.urf_finish_once(ud, r) : UD_TREE_OK;
    if (ret != UD_TREE_OK) return ret;
    r->uc_finish_once_done = 1;
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
ud_render_node(struct udoc *doc, struct udr_ctx *ctx,
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
  if (!ud_part_ind_stack_init(&rctx.uc_part_stack, ud_oht_size(&doc->ud_parts)))
    return 0;

  /* backend render is passed as user data to tree_ctx */
  tctx_state.utc_list = root;
  tctx_state.utc_user_data = &rctx;
  tctx.utc_funcs = &render_funcs;
  tctx.utc_state = &tctx_state;

  /* set tree_ctx */
  rctx.uc_tree_ctx = &tctx;
  ret = ud_tree_walk(doc, &tctx);

  /* the uc_part stack will only be empty if everything was successful */
  if (ret != UD_TREE_FAIL)
    ud_assert(ud_part_ind_stack_size(&rctx.uc_part_stack) == 0);

  ud_part_ind_stack_free(&rctx.uc_part_stack);
  return ret != UD_TREE_FAIL;
}

int
ud_render_doc(struct udoc *doc, const struct udr_opts *opts,
  const struct ud_renderer *r, const char *outdir)
{
  const struct ud_part *p;
  struct udr_output_ctx out;
  struct udr_ctx rctx;

  log_1xf(LOG_DEBUG, "rendering");

  if (fchdir(doc->ud_dirfd_pwd) == -1) {
    ud_error_pushsys(&ud_errors, "fchdir");
    return 0;
  }
  if (doc->ud_dirfd_out == -1) {
    doc->ud_dirfd_out = open_ro(outdir);
    if (doc->ud_dirfd_out == -1) {
      ud_error_pushsys(&ud_errors, "outdir");
      return 0;
    }
  }
  if (fchdir(doc->ud_dirfd_out) == -1) {
    ud_error_pushsys(&ud_errors, "fchdir");
    return 0;
  }
  if (!ud_oht_getind(&doc->ud_parts, 0, (void *) &p)) {
    ud_error_push(&ud_errors, "ud_oht_getind", "could not get root uc_part");
    return 0;
  }
  if (!r_output_open(r, &out, p)) return 0;
  if (fchdir(doc->ud_dirfd_src) == -1) {
    ud_error_pushsys(&ud_errors, "fchdir");
    return 0;
  }

  bin_zero(&rctx, sizeof(rctx));
  rctx.uc_tree_ctx = 0;
  rctx.uc_render = r;
  rctx.uc_part = p;
  rctx.uc_opts = opts;
  rctx.uc_out = &out;

  if (!ud_render_node(doc, &rctx, p->up_list)) return 0;
  if (!r_output_close(&out)) return 0;
  if (fchdir(doc->ud_dirfd_pwd) == -1) {
    ud_error_pushsys(&ud_errors, "fchdir");
    return 0;
  }
  return 1;
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

  if (fchdir(ud->ud_dirfd_src) == -1) {
    ud_error_pushsys(&ud_errors, "fchdir");
    goto END;
  }

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

  if (fchdir(ud->ud_dirfd_src) == -1) {
    ud_error_pushsys(&ud_errors, "fchdir");
    goto END;
  }

  ret = 1;

  END:
  return ret;
}

