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
  struct dstring dstr1;
  struct ud_part_ind_stack ind_stack; /* stack of integer indices to ud_parts */
};

static int
part_add(struct udoc *doc, struct part_ctx *pctx, unsigned long flags,
  const struct ud_node_list *list, const struct ud_node *node)
{
  char cnum[FMT_ULONG];
  struct ud_part p_new;
  struct ud_part *p_cur = 0;
  struct ud_part *p_up = 0;
  struct ud_part *p_new_p = 0;

  bin_zero(&p_new, sizeof(p_new));
  if (ud_part_getcur(doc, &p_cur)) {
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
  if (ud_part_getprev_up(doc, &p_new, &p_up))
    p_new.up_index_parent = p_up->up_index_cur;
  if (!ud_part_add(doc, &p_new)) {
    ud_error_push(&ud_errors, "part_add", "part add failed");
    return 0;
  }

  /* get pointer to added part */
  ud_assert(ud_part_getcur(doc, &p_new_p));

  /* work out string for use in table of contents, etc */
  if (!ud_part_num_fmt(doc, p_new_p, &pctx->dstr1)) {
    ud_error_pushsys(&ud_errors, "ud_part_num_fmt");
    return UD_TREE_FAIL;
  }
  if (!str_dup(pctx->dstr1.s, (char **) &p_new_p->up_num_string)) {
    ud_error_pushsys(&ud_errors, "str_dup");
    return UD_TREE_FAIL;
  }
 
  /* push part onto stack */
  if (!ud_part_ind_stack_push(&pctx->ind_stack, &p_new.up_index_cur)) {
    ud_error_pushsys(&ud_errors, "stack_push");
    return UD_TREE_FAIL;
  }

  /* chatter */
  cnum[fmt_ulong(cnum, p_new.up_index_cur)] = 0;
  log_2xf(LOG_DEBUG, "part ", cnum);
  if (flags & UD_PART_SPLIT) {
    cnum[fmt_ulong(cnum, p_new.up_file)] = 0;
    log_2xf(LOG_DEBUG, "part split ", cnum);
  }
  return 1;
}

static int
part_section(struct udoc *doc, struct part_ctx *pctx,
  const struct ud_node_list *list, const struct ud_node *node)
{
  char cnum[FMT_ULONG];
  unsigned long flags = 0;
  unsigned long size = ud_part_ind_stack_size(&pctx->ind_stack);

  cnum[fmt_ulong(cnum, size)] = 0;
  log_2xf(LOG_DEBUG, "part stack size ", cnum);
  if (size - 1 < doc->ud_opts.ud_split_thresh) {
    flags |= UD_PART_SPLIT;
  } else
    log_1xf(LOG_DEBUG, "split threshold exceeded - not splitting");

  if (!part_add(doc, pctx, flags, list, node)) return UD_TREE_FAIL;
  return UD_TREE_OK;
}

static int
part_title(struct udoc *doc, struct part_ctx *pctx,
  struct ud_part *part, const struct ud_node *node)
{
  char cnum[FMT_ULONG];
  if (part->up_title) {
    cnum[fmt_ulong(cnum, node->un_next->un_line_num)] = 0;
    log_3x(LOG_WARN, "title on line ", cnum, " overwrites previous title");
  }
  part->up_title = node->un_next->un_data.un_str;
  log_2xf(LOG_DEBUG, "part title ", part->up_title);
  return UD_TREE_OK;
}

/*
 * top level callbacks
 */

static enum ud_tree_walk_stat
cb_part_init(struct udoc *doc, struct ud_tree_ctx *ctx)
{
  struct part_ctx *pctx = ctx->utc_state->utc_user_data;

  log_1xf(LOG_DEBUG, "adding root part");

  bin_zero(pctx, sizeof(*pctx));
  if (!ud_part_ind_stack_init(&pctx->ind_stack, 32)) {
    ud_error_pushsys(&ud_errors, "stack_init");
    goto FAIL;
  }

  /* add root part - note that the root part is always 'split' */
  if (!part_add(doc, pctx, UD_PART_SPLIT, &doc->ud_tree.ut_root,
                doc->ud_tree.ut_root.unl_head)) goto FAIL;

  return UD_TREE_OK;

  FAIL:
  ud_part_ind_stack_free(&pctx->ind_stack);
  dstring_free(&pctx->dstr1);
  return UD_TREE_FAIL;
}

static enum ud_tree_walk_stat
cb_part_symbol(struct udoc *doc, struct ud_tree_ctx *ctx)
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
    case UDOC_TAG_SECTION:
      return part_section(doc, pctx, ctx->utc_state->utc_list, ctx->utc_state->utc_node);
    case UDOC_TAG_TITLE:
    case UDOC_TAG_REF:
    case UDOC_TAG_FOOTNOTE:
    case UDOC_TAG_STYLE:
      ud_part_ind_stack_peek(&pctx->ind_stack, &ind);
      ud_oht_getind(&doc->ud_parts, *ind, (void *) &part);
      break;
    default:
      break;
  }

  ref.ur_list = ctx->utc_state->utc_list;
  ref.ur_node = ctx->utc_state->utc_node;
  ref.ur_part = part;
 
  switch (tag) {
    case UDOC_TAG_TITLE: return part_title(doc, pctx, part, ctx->utc_state->utc_node);
    case UDOC_TAG_REF:
      if (!ud_ref_add_byname(&doc->ud_ref_names, ref.ur_node->un_next->un_data.un_str, &ref)) {
        ud_error_push(&ud_errors, "ud_ref_add_byname", "could not add reference");
        return UD_TREE_FAIL;
      }
      tab = &doc->ud_refs;
      break;
    case UDOC_TAG_FOOTNOTE: tab = &doc->ud_footnotes; break;
    case UDOC_TAG_STYLE: tab = &doc->ud_styles; break;
    case UDOC_TAG_LINK_EXT: tab = &doc->ud_link_exts; break;
    case UDOC_TAG_RENDER_HEADER:
      if (doc->ud_render_header) {
        ln[fmt_ulong(ln, ctx->utc_state->utc_node->un_line_num)] = 0;
        log_2x(LOG_WARN, ln, ": render-header overrides previous tag");
      }
      doc->ud_render_header = ctx->utc_state->utc_node->un_next->un_data.un_str;
      break;
    case UDOC_TAG_RENDER_FOOTER:
      if (doc->ud_render_footer) {
        ln[fmt_ulong(ln, ctx->utc_state->utc_node->un_line_num)] = 0;
        log_2x(LOG_WARN, ln, ": render-footer overrides previous tag");
      }
      doc->ud_render_footer = ctx->utc_state->utc_node->un_next->un_data.un_str;
      break;
    default: break;
  }

  if (tab) {
    if (!ud_ref_add(tab, &ref)) {
      ud_error_push(&ud_errors, "ud_ref_add", "could not add reference");
      return UD_TREE_FAIL;
    }
  }
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
cb_part_list_end(struct udoc *doc, struct ud_tree_ctx *ctx)
{
  char cnum[FMT_ULONG];
  struct part_ctx *pctx = ctx->utc_state->utc_user_data;
  struct ud_node *first_sym;
  enum ud_tag tag;
  unsigned long *ind;

  first_sym = ctx->utc_state->utc_list->unl_head;
  if (!ud_tag_by_name(first_sym->un_data.un_sym, &tag)) return UD_TREE_OK;
  switch (tag) {
    case UDOC_TAG_SECTION:
      ud_part_ind_stack_pop(&pctx->ind_stack, &ind);
      cnum[fmt_ulong(cnum, ud_part_ind_stack_size(&pctx->ind_stack))] = 0;
      log_2xf(LOG_DEBUG, "part stack size ", cnum);
      break;
    default: break;
  }
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
cb_part_finish(struct udoc *doc, struct ud_tree_ctx *ctx)
{
  char cnum1[FMT_ULONG];
  char cnum2[FMT_ULONG];
  unsigned long ind;
  unsigned long max = ud_oht_size(&doc->ud_parts);
  unsigned long files = 0;
  struct ud_part *part;
  struct part_ctx *pctx = ctx->utc_state->utc_user_data;

  for (ind = 0; ind < max; ++ind)
    if (ud_oht_getind(&doc->ud_parts, ind, (void *) &part))
      if (part->up_flags & UD_PART_SPLIT && ind) ++files;

  cnum1[fmt_ulong(cnum1, ud_oht_size(&doc->ud_parts))] = 0;
  cnum2[fmt_ulong(cnum2, files + 1)] = 0;
  log_4xf(LOG_DEBUG, cnum1, " parts, ", cnum2, " files");

  ud_assert(ud_part_ind_stack_size(&pctx->ind_stack) == 1);
  ud_part_ind_stack_free(&pctx->ind_stack);
  dstring_free(&pctx->dstr1);
  return UD_TREE_OK;
}

static const struct ud_tree_ctx_funcs part_funcs = {
  cb_part_init,
  0,
  0,
  cb_part_symbol,
  0,
  cb_part_list_end,
  cb_part_finish,
  0,
};

int
ud_partition(struct udoc *doc)
{
  struct ud_tree_ctx ctx;
  struct ud_tree_ctx_state state;
  struct part_ctx pctx;

  bin_zero(&ctx, sizeof(ctx));
  bin_zero(&state, sizeof(state));
  bin_zero(&pctx, sizeof(pctx));

  state.utc_list = &doc->ud_tree.ut_root;
  state.utc_user_data = &pctx;

  ctx.utc_funcs = &part_funcs;
  ctx.utc_state = &state;

  switch (ud_tree_walk(doc, &ctx)) {
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
ud_part_getfromnode(struct udoc *doc, const struct ud_node *n,
  struct ud_part **ch, unsigned long *ind)
{
  return ud_oht_get(&doc->ud_parts, (void *) &n, sizeof(&n), (void *) ch, ind);
}

int
ud_part_getroot(struct udoc *doc, struct ud_part **ch)
{
  return ud_oht_getind(&doc->ud_parts, 0, (void *) ch);
}

int
ud_part_getcur(struct udoc *doc, struct ud_part **rcur)
{
  struct ud_part *cur;
  unsigned long max = ud_oht_size(&doc->ud_parts);
  if (!max) return 0;
  if (!ud_oht_getind(&doc->ud_parts, max - 1, (void *) &cur)) return 0;
  *rcur = cur;
  return 1;
}

int
ud_part_getprev(struct udoc *doc, const struct ud_part *cur,
  struct ud_part **prev)
{
  struct ud_part *cprev;
  unsigned long max = cur->up_index_cur;
  unsigned long ind;

  if (!max) return 0;
  for (ind = max - 1; ind; --ind) {
    if (ud_oht_getind(&doc->ud_parts, ind, (void *) &cprev)) {
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
ud_part_getfirst_wparent(struct udoc *doc, const struct ud_part *cur,
  struct ud_part **prev)
{
  struct ud_part *part_tmp = (struct ud_part *) cur;
  struct ud_part *part_first = (struct ud_part *) cur;

  for (;;) {
    if (!ud_oht_getind(&doc->ud_parts, part_tmp->up_index_prev, (void *) &part_tmp))
      break;
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
ud_part_getfirst_wdepth_noskip(struct udoc *doc, const struct ud_part *cur,
  struct ud_part **prev)
{
  struct ud_part *part_tmp = (struct ud_part *) cur;
  struct ud_part *part_first = (struct ud_part *) cur;

  for (;;) {
    if (!ud_part_getprev(doc, part_tmp, &part_tmp)) break;
    if (part_tmp->up_depth == cur->up_depth)
      part_first = part_tmp;
    else
      break;
  }

  *prev = part_first;
}

int
ud_part_getprev_up(struct udoc *doc, const struct ud_part *cur,
  struct ud_part **prev)
{
  struct ud_part *cprev;
  const struct ud_part *cur_walk = cur;
  for (;;) {
    if (!ud_part_getprev(doc, cur_walk, &cprev)) return 0;
    if (cprev->up_depth < cur->up_depth) { *prev = cprev; return 1; }
    cur_walk = cprev;
  }
  return 1;
}

int
ud_part_add(struct udoc *doc, const struct ud_part *ch)
{
  switch (ud_oht_add(&doc->ud_parts, &ch->up_node, sizeof(&ch->up_node), ch)) {
    case 0: log_1xf(LOG_ERROR, "duplicate part node!"); return 0;
    case -1: log_1sysf(LOG_ERROR, 0); return 0;
    default: return 1;
  }
}

int
ud_part_getnext_file(struct udoc *doc, const struct ud_part *cur,
  struct ud_part **next)
{
  struct ud_part *cnext;
  unsigned long max = ud_oht_size(&doc->ud_parts);
  unsigned long ind;

  for (ind = cur->up_index_cur; ind < max; ++ind) {
    if (ud_oht_getind(&doc->ud_parts, ind, (void *) &cnext)) {
      if (cnext->up_file != cur->up_file) {
        *next = cnext;
        return 1;
      }
    }
  }
  return 0;
}

int
ud_part_getprev_file(struct udoc *doc, const struct ud_part *cur,
  struct ud_part **prev)
{
  struct ud_part *cprev;
  unsigned long max = cur->up_index_cur;
  unsigned long ind;

  if (!max) return 0;
  for (ind = max - 1; ind + 1; --ind) {
    if (ud_oht_getind(&doc->ud_parts, ind, (void *) &cprev)) {
      if (cprev->up_file != cur->up_file) {
        *prev = cprev;
        return 1;
      }
    }
  }
  return 0;
}

static unsigned long
ud_part_offset_wparent(struct udoc *doc,
  const struct ud_part *from, const struct ud_part *to)
{
  unsigned long off = 0;
  struct ud_part *part_tmp = (struct ud_part *) from;

  for (;;) {
    if (part_tmp == to) break;
    if (!ud_oht_getind(&doc->ud_parts, part_tmp->up_index_next, (void *) &part_tmp))
      break;
    if (part_tmp->up_index_parent == from->up_index_parent) ++off;
  }
  return off;
}

int
ud_part_num_fmt(struct udoc *doc, const struct ud_part *part,
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
    if (!ud_oht_getind(&doc->ud_parts, part_cur->up_index_parent, (void *) &part_cur))
      break;
  }

  /* repeatedly pop parents and calculate offsets for numbering */
  dstring_trunc(ds);
  for (;;) {
    if (!sstack_pop(&ns, (void *) &num)) break;
    if (!ud_oht_getind(&doc->ud_parts, *num, (void *) &part_cur)) break;

    ud_part_getfirst_wparent(doc, part_cur, &part_first);
    off = ud_part_offset_wparent(doc, part_first, part_cur);

    /* annoying hack to allow root node to be 0 but to allow subsections
     * to be numbered from 1 instead of 0.
     */

    if (ds->len) ++off;
    if (!dstring_catb(ds, cnum, fmt_ulong(cnum, off))) return 0;
    if (!dstring_catb(ds, ".", 1)) return 0;
  }

  dstring_0(ds);
  return 1;
}

/* ud_part index stack */
GEN_stack_define(ud_part_ind);

