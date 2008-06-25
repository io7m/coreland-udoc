#include <corelib/alloc.h>
#include <corelib/array.h>
#include <corelib/bin.h>
#include <corelib/error.h>
#include <corelib/fmt.h>
#include <corelib/hashtable.h>
#include <corelib/sstack.h>
#include <corelib/str.h>

#include "gen_stack.h"
#include "log.h"

#define UDOC_IMPLEMENTATION
#include "ud_assert.h"
#include "ud_oht.h"
#include "ud_tag.h"
#include "ud_part.h"
#include "ud_ref.h"
#include "udoc.h"

struct part_ctx {
  struct dstring dstr;
  struct ud_part_ind_stack ind_stack; /* stack of integer indices to ud_parts */
  struct hashtable links; /* table of links for reference checking */
};

static void
part_ctx_free(struct udoc *ud, struct part_ctx *p)
{
  dstring_free(&p->dstr);
  ud_part_ind_stack_free(&p->ind_stack);
  ht_free(&p->links);
}

static int
part_ctx_init(struct udoc *ud, struct part_ctx *p)
{
  bin_zero(p, sizeof(*p));
  ud_try_sys_jump(ud, dstring_init(&p->dstr, 256), FAIL, "dstring_init"); 
  ud_try_sys_jump(ud, ud_part_ind_stack_init(&p->ind_stack, 32), FAIL, "stack_init"); 
  ud_try_sys_jump(ud, ht_init(&p->links), FAIL, "hashtable_init"); 
  return 1;

  FAIL:
  part_ctx_free(ud, p);
  return 0;
}

static int
part_add(struct udoc *ud, struct part_ctx *pctx, unsigned long flags,
  const struct ud_node_list *list, const struct ud_node *node)
{
  char cnum[FMT_ULONG];
  struct ud_part p_new;
  struct ud_part *p_cur = 0;
  struct ud_part *p_up = 0;
  struct ud_part *pp_new = 0;
  int flag_part = 0;
  int flag_strdup = 0;

  bin_zero(&p_new, sizeof(p_new));
  if (ud_part_getcur(ud, &p_cur)) {
    p_new.up_file = p_cur->up_file;
    p_new.up_index_cur = p_cur->up_index_cur + 1;
    if (flags & UD_PART_SPLIT)
      p_new.up_file = p_new.up_index_cur;
    p_new.up_index_prev = p_cur->up_index_cur;
    p_cur->up_index_next = p_new.up_index_cur;
  }
  p_new.up_depth = ud_part_ind_stack_size(&pctx->ind_stack);
  p_new.up_node = node;
  p_new.up_list = list;
  p_new.up_flags = flags;

  /* get parent and add part */
  if (ud_part_getprev_up(ud, &p_new, &p_up))
    p_new.up_index_parent = p_up->up_index_cur;

  ud_try_sys_jump(ud, ud_part_add(ud, &p_new), FAIL, "part_add");
  flag_part = 1;

  /* get pointer to added part */
  ud_assert(ud_part_getcur(ud, &pp_new));

  /* work out string for use in table of contents, etc */
  ud_try_sys_jump(ud, ud_part_num_fmt(ud, pp_new, &pctx->dstr), FAIL, "num_fmt");
  ud_try_sys_jump(ud, str_dup(pctx->dstr.s, (char **) &pp_new->up_num_string), FAIL, "str_dup");
  flag_strdup = 1;
 
  /* push part onto stack */
  ud_try_sys_jump(ud, ud_part_ind_stack_push(&pctx->ind_stack,
    &p_new.up_index_cur), FAIL, "stack_push");

  /* chatter */
  cnum[fmt_ulong(cnum, p_new.up_index_cur)] = 0;
  log_2xf(LOG_DEBUG, "part ", cnum);
  if (flags & UD_PART_SPLIT) {
    cnum[fmt_ulong(cnum, p_new.up_file)] = 0;
    log_2xf(LOG_DEBUG, "part split ", cnum);
  }
  return UD_TREE_OK;

  FAIL:
  if (flag_part) flag_part = 1; /* TODO: remove part, fix links... */
  if (flag_strdup) dealloc_null(&pp_new->up_num_string);
  return UD_TREE_FAIL;
}

static int
part_section(struct udoc *ud, struct part_ctx *pctx,
  const struct ud_node_list *list, const struct ud_node *node)
{
  char cnum[FMT_ULONG];
  unsigned long flags = 0;
  const unsigned long size = ud_part_ind_stack_size(&pctx->ind_stack);

  cnum[fmt_ulong(cnum, size)] = 0;
  log_2xf(LOG_DEBUG, "part stack size ", cnum);

  if (size - 1 < ud->ud_opts.ud_split_thresh) {
    flags |= UD_PART_SPLIT;
  } else
    log_1xf(LOG_DEBUG, "split threshold exceeded - not splitting");

  if (!part_add(ud, pctx, flags, list, node)) return UD_TREE_FAIL;
  return UD_TREE_OK;
}

static int
part_subsection(struct udoc *ud, struct part_ctx *pctx,
  const struct ud_node_list *list, const struct ud_node *node)
{
  char cnum[FMT_ULONG];
  const unsigned long size = ud_part_ind_stack_size(&pctx->ind_stack);

  cnum[fmt_ulong(cnum, size)] = 0;
  log_2xf(LOG_DEBUG, "part stack size ", cnum);

  if (!part_add(ud, pctx, UD_PART_SUBSECT, list, node)) return UD_TREE_FAIL;
  return UD_TREE_OK;
}

static int
part_title(struct udoc *ud, struct part_ctx *pctx,
  struct ud_part *part, const struct ud_node *node)
{
  char cnum[FMT_ULONG];
  if (part->up_title) {
    cnum[fmt_ulong(cnum, node->un_next->un_line_num)] = 0;
    log_4x(LOG_WARN, ud->ud_cur_doc->ud_name, ": title on line ",
      cnum, " overwrites previous title");
  }
  part->up_title = node->un_next->un_data.un_str;
  log_2xf(LOG_DEBUG, "part title ", part->up_title);
  return UD_TREE_OK;
}

/*
 * link integrity checking.
 */

struct link_ctx {
  struct udoc *udoc;
  const struct ud_tree_ctx *tree_ctx;
  struct part_ctx *part_ctx;
  int ok;
};

static int
check_link(void *ku, unsigned long klen, void *du, unsigned long dlen, void *udata)
{
  char cnum[FMT_ULONG];
  const char *key = ku;
  struct link_ctx *link_ctx = udata;
  struct udoc *ud = link_ctx->udoc;
  const struct ud_tree_ctx *tree = link_ctx->tree_ctx;
  struct ud_ref *ref;
  struct dstring *buf = &link_ctx->part_ctx->dstr;
  unsigned long dummy;
  unsigned long line;
  struct ud_err ue;

  /* attempt to fetch ref from table */
  if (!ud_oht_get(&ud->ud_ref_names, key, str_len(key), (void *) &ref, &dummy)) {
    line = tree->utc_state->utc_node->un_line_num;
    cnum[fmt_ulong(cnum, line)] = 0;

    dstring_trunc(buf);
    dstring_cats(buf, "link points to undefined reference \"");
    dstring_cats(buf, key);
    dstring_cats(buf, "\"");
    dstring_0(buf);

    ud_error_fill(ud, &ue, "link", cnum, buf->s, 0);
    ud_error_push(ud, &ue);
    link_ctx->ok = 0;
  }
  return 1;
}

static int
check_links(struct udoc *ud, const struct ud_tree_ctx *ctx,
  struct part_ctx *pctx)
{
  struct link_ctx chk = { ud, ctx, pctx, 1 };
  ht_iter(&pctx->links, check_link, &chk);
  return chk.ok;
}

/*
 * top level callbacks
 */

static enum ud_tree_walk_stat
cb_part_init(struct udoc *ud, struct ud_tree_ctx *ctx)
{
  struct part_ctx *pctx = ctx->utc_state->utc_user_data;

  log_1xf(LOG_DEBUG, "adding root part");
  ud_try_jump(ud, part_ctx_init(ud, pctx), FAIL, "part_context_init");

  /* add root part - note that the root part is always 'split' */
  ud_try_jump(ud, part_add(ud, pctx, UD_PART_SPLIT, &ud->ud_tree.ut_root,
    ud->ud_tree.ut_root.unl_head), FAIL, "part_add");
  return UD_TREE_OK;

  FAIL:
  return UD_TREE_FAIL;
}

static enum ud_tree_walk_stat
cb_part_symbol(struct udoc *ud, struct ud_tree_ctx *ctx)
{
  char ln[FMT_ULONG];
  struct part_ctx *pctx = ctx->utc_state->utc_user_data;
  struct ud_part *part = 0;
  struct ud_ordered_ht *tab = 0;
  unsigned long *ind;
  enum ud_tag tag;
  struct ud_ref ref;

  if (ctx->utc_state->utc_list_pos != 0) return UD_TREE_OK;
  if (!ud_tag_by_name(ctx->utc_state->utc_node->un_data.un_sym, &tag)) return UD_TREE_OK;
  
  switch (tag) {
    case UDOC_TAG_SUBSECTION:
      return part_subsection(ud, pctx, ctx->utc_state->utc_list, ctx->utc_state->utc_node);
    case UDOC_TAG_SECTION:
      return part_section(ud, pctx, ctx->utc_state->utc_list, ctx->utc_state->utc_node);
    case UDOC_TAG_TITLE:
    case UDOC_TAG_REF:
    case UDOC_TAG_FOOTNOTE:
    case UDOC_TAG_STYLE:
      ud_assert(ud_part_ind_stack_peek(&pctx->ind_stack, &ind));
      ud_assert(ud_oht_getind(&ud->ud_parts, *ind, (void *) &part));
      break;
    case UDOC_TAG_LINK:
      {
        /* add link target to table for integrity checking */
        const char *link_target = ctx->utc_state->utc_node->un_next->un_data.un_str;
        const unsigned long len = str_len(link_target);
        const unsigned long dummy = 1;
        if (!ht_checks(&pctx->links, link_target))
          ud_try_sys(ud,
            ht_addb(&pctx->links, link_target, len, &dummy, sizeof(dummy) == 1),
              UD_TREE_FAIL, "link_table_add");
      }
      break;
    default:
      break;
  }

  ref.ur_list = ctx->utc_state->utc_list;
  ref.ur_node = ctx->utc_state->utc_node;
  ref.ur_part = part;
 
  switch (tag) {
    case UDOC_TAG_TITLE: return part_title(ud, pctx, part, ctx->utc_state->utc_node);
    case UDOC_TAG_REF:
      ud_tryS(ud, ud_ref_add_byname(&ud->ud_ref_names,
              ref.ur_node->un_next->un_data.un_str, &ref), UD_TREE_FAIL,
              "ud_ref_add_byname", "could not add reference");
      tab = &ud->ud_refs;
      break;
    case UDOC_TAG_FOOTNOTE: tab = &ud->ud_footnotes; break;
    case UDOC_TAG_STYLE: tab = &ud->ud_styles; break;
    case UDOC_TAG_LINK_EXT: tab = &ud->ud_link_exts; break;
    case UDOC_TAG_RENDER_HEADER:
      if (ud->ud_render_header) {
        ln[fmt_ulong(ln, ctx->utc_state->utc_node->un_line_num)] = 0;
        log_4x(LOG_WARN, ud->ud_cur_doc->ud_name, ": ", ln,
          ": render-header overrides previous tag");
      }
      ud->ud_render_header = ctx->utc_state->utc_node->un_next->un_data.un_str;
      break;
    case UDOC_TAG_RENDER_FOOTER:
      if (ud->ud_render_footer) {
        ln[fmt_ulong(ln, ctx->utc_state->utc_node->un_line_num)] = 0;
        log_4x(LOG_WARN, ud->ud_cur_doc->ud_name, ": ", ln,
          ": render-footer overrides previous tag");
      }
      ud->ud_render_footer = ctx->utc_state->utc_node->un_next->un_data.un_str;
      break;
    default: break;
  }

  if (tab)
    ud_tryS(ud, ud_ref_add(tab, &ref), UD_TREE_FAIL, "ud_ref_add", "could not add reference");

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
cb_part_list_end(struct udoc *ud, struct ud_tree_ctx *ctx)
{
  char cnum[FMT_ULONG];
  struct part_ctx *pctx = ctx->utc_state->utc_user_data;
  struct ud_node *first_sym;
  enum ud_tag tag;
  unsigned long *ind;

  first_sym = ctx->utc_state->utc_list->unl_head;
  if (!ud_tag_by_name(first_sym->un_data.un_sym, &tag)) return UD_TREE_OK;
  switch (tag) {
    case UDOC_TAG_SUBSECTION:
    case UDOC_TAG_SECTION:
      ud_assert(ud_part_ind_stack_pop(&pctx->ind_stack, &ind));
      cnum[fmt_ulong(cnum, ud_part_ind_stack_size(&pctx->ind_stack))] = 0;
      log_2xf(LOG_DEBUG, "part stack size ", cnum);
      break;
    default: break;
  }
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
cb_part_finish(struct udoc *ud, struct ud_tree_ctx *ctx)
{
  char cnum1[FMT_ULONG];
  char cnum2[FMT_ULONG];
  unsigned long ind;
  unsigned long max = ud_oht_size(&ud->ud_parts);
  unsigned long files = 0;
  struct ud_part *part;
  struct part_ctx *pctx = ctx->utc_state->utc_user_data;
  enum ud_tree_walk_stat ret = UD_TREE_FAIL;

  /* check link integrity */
  if (!check_links(ud, ctx, pctx)) goto END;

  /* count files */
  for (ind = 0; ind < max; ++ind)
    if (ud_oht_getind(&ud->ud_parts, ind, (void *) &part))
      if (part->up_flags & UD_PART_SPLIT && ind) ++files;

  cnum1[fmt_ulong(cnum1, ud_oht_size(&ud->ud_parts))] = 0;
  cnum2[fmt_ulong(cnum2, files + 1)] = 0;
  log_4xf(LOG_DEBUG, cnum1, " parts, ", cnum2, " files");

  ret = UD_TREE_OK;
  END:
  ud_assert(ud_part_ind_stack_size(&pctx->ind_stack) == 1);
  part_ctx_free(ud, pctx);
  return ret;
}

static const struct ud_tree_ctx_funcs part_funcs = {
  .utcf_init = cb_part_init,
  .utcf_symbol = cb_part_symbol,
  .utcf_list_end = cb_part_list_end,
  .utcf_finish = cb_part_finish,
};

int
ud_partition(struct udoc *ud)
{
  struct ud_tree_ctx ctx;
  struct ud_tree_ctx_state state;
  struct part_ctx pctx;

  bin_zero(&ctx, sizeof(ctx));
  bin_zero(&state, sizeof(state));

  state.utc_list = &ud->ud_tree.ut_root;
  state.utc_user_data = &pctx;

  ctx.utc_funcs = &part_funcs;
  ctx.utc_state = &state;

  switch (ud_tree_walk(ud, &ctx)) {
    case UD_TREE_STOP:
    case UD_TREE_OK:
      return 1;
    default:
      return 0;
  }
}

/*
 * part API
 */

int
ud_part_getfromnode(struct udoc *ud, const struct ud_node *n,
  struct ud_part **ch, unsigned long *ind)
{
  return ud_oht_get(&ud->ud_parts, (void *) &n, sizeof(&n), (void *) ch, ind);
}

int
ud_part_getroot(struct udoc *ud, struct ud_part **ch)
{
  return ud_oht_getind(&ud->ud_parts, 0, (void *) ch);
}

int
ud_part_getcur(struct udoc *ud, struct ud_part **rcur)
{
  struct ud_part *cur;
  unsigned long max = ud_oht_size(&ud->ud_parts);
  if (!max) return 0;
  if (!ud_oht_getind(&ud->ud_parts, max - 1, (void *) &cur)) return 0;
  *rcur = cur;
  return 1;
}

int
ud_part_getprev(struct udoc *ud, const struct ud_part *cur,
  struct ud_part **prev)
{
  struct ud_part *cprev;
  unsigned long max = cur->up_index_cur;
  unsigned long ind;

  if (!max) return 0;
  for (ind = max - 1; ind; --ind) {
    if (ud_oht_getind(&ud->ud_parts, ind, (void *) &cprev)) {
      *prev = cprev;
      return 1;
    }
  }
  return 0;
}

/*
 * find first node in list with the same parent as this node.
 */

void
ud_part_getfirst_wparent(struct udoc *ud, const struct ud_part *cur,
  struct ud_part **prev)
{
  struct ud_part *part_tmp = (struct ud_part *) cur;
  struct ud_part *part_first = (struct ud_part *) cur;

  for (;;) {
    if (!ud_oht_getind(&ud->ud_parts, part_tmp->up_index_prev, (void *) &part_tmp)) break;
    if (part_tmp->up_index_parent == cur->up_index_parent) part_first = part_tmp;
    if (!part_tmp->up_index_cur) break;
  }

  *prev = part_first;
}

/*
 * search back through nodes, stopping before the first node of a different
 * depth ("noskip").
 */

void
ud_part_getfirst_wdepth_noskip(struct udoc *ud, const struct ud_part *cur,
  struct ud_part **prev)
{
  struct ud_part *part_tmp = (struct ud_part *) cur;
  struct ud_part *part_first = (struct ud_part *) cur;

  for (;;) {
    if (!ud_part_getprev(ud, part_tmp, &part_tmp)) break;
    if (part_tmp->up_depth == cur->up_depth)
      part_first = part_tmp;
    else
      break;
  }

  *prev = part_first;
}

/*
 * find closest previous node of a lower depth (closer to the root)
 */

int
ud_part_getprev_up(struct udoc *ud, const struct ud_part *cur,
  struct ud_part **prev)
{
  struct ud_part *cprev;
  const struct ud_part *cur_walk = cur;
  for (;;) {
    if (!ud_part_getprev(ud, cur_walk, &cprev)) return 0;
    if (cprev->up_depth < cur->up_depth) { *prev = cprev; return 1; }
    cur_walk = cprev;
  }
  return 1;
}

int
ud_part_add(struct udoc *ud, const struct ud_part *ch)
{
  switch (ud_oht_add(&ud->ud_parts, &ch->up_node, sizeof(&ch->up_node), ch)) {
    case 0: log_1xf(LOG_ERROR, "duplicate part node!"); return 0;
    case -1: log_1sysf(LOG_ERROR, 0); return 0;
    default: return 1;
  }
}

/*
 * get closest next part from a different file
 */

int
ud_part_getnext_file(struct udoc *ud, const struct ud_part *cur,
  struct ud_part **next)
{
  struct ud_part *cnext;
  unsigned long max = ud_oht_size(&ud->ud_parts);
  unsigned long ind;

  for (ind = cur->up_index_cur; ind < max; ++ind) {
    if (ud_oht_getind(&ud->ud_parts, ind, (void *) &cnext)) {
      if (cnext->up_file != cur->up_file) {
        *next = cnext;
        return 1;
      }
    }
  }
  return 0;
}

/*
 * get closest previous part from a different file
 */

int
ud_part_getprev_file(struct udoc *ud, const struct ud_part *cur,
  struct ud_part **prev)
{
  struct ud_part *cprev;
  unsigned long max = cur->up_index_cur;
  unsigned long ind;

  if (!max) return 0;
  for (ind = max - 1; ind + 1; --ind) {
    if (ud_oht_getind(&ud->ud_parts, ind, (void *) &cprev)) {
      if (cprev->up_file != cur->up_file) {
        *prev = cprev;
        return 1;
      }
    }
  }
  return 0;
}

static unsigned long
ud_part_offset_wparent(struct udoc *ud,
  const struct ud_part *from, const struct ud_part *to)
{
  unsigned long off = 0;
  struct ud_part *part_tmp = (struct ud_part *) from;

  for (;;) {
    if (part_tmp == to) break;
    if (!ud_oht_getind(&ud->ud_parts, part_tmp->up_index_next, (void *) &part_tmp)) break;
    if (part_tmp->up_index_parent == from->up_index_parent) ++off;
  }
  return off;
}

int
ud_part_num_fmt(struct udoc *ud, const struct ud_part *part,
  struct dstring *ds)
{
  char cnum[FMT_ULONG];
  char sbuf[32 * sizeof(unsigned long)];
  struct sstack ns;
  struct ud_part *part_cur;
  struct ud_part *part_first;
  unsigned long *num;
  unsigned long off;

  sstack_init(&ns, sbuf, sizeof(sbuf), sizeof(unsigned long));

  /* push parent parts onto stack */
  part_cur = (struct ud_part *) part;
  for (;;) {
    if (!sstack_push(&ns, (void *) &part_cur->up_index_cur)) break;
    if (!part_cur->up_index_parent) break;
    if (!ud_oht_getind(&ud->ud_parts, part_cur->up_index_parent, (void *) &part_cur)) break;
  }

  /* repeatedly pop parents and calculate offsets for numbering */
  dstring_trunc(ds);
  for (;;) {
    if (!sstack_pop(&ns, (void *) &num)) break;
    if (!ud_oht_getind(&ud->ud_parts, *num, (void *) &part_cur)) break;

    ud_part_getfirst_wparent(ud, part_cur, &part_first);
    off = ud_part_offset_wparent(ud, part_first, part_cur);

    /* annoying hack to allow root node to be 0 but to allow subsections
     * to be numbered from 1 instead of 0.
     */

    if (ds->len) ++off;
    if (!dstring_catb(ds, cnum, fmt_ulong(cnum, off))) return 0;
    if (!dstring_catb(ds, ".", 1)) return 0;
  }

  /* remove possible trailing '.' */
  if (ds->s[ds->len - 1] == '.') dstring_chop (ds, ds->len - 1);

  dstring_0(ds);
  return 1;
}

/* ud_part index stack */
GEN_stack_define(ud_part_ind);

