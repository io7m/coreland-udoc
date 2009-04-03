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

struct part_context {
  struct dstring dstr;
  struct ud_part_index_stack index_stack; /* stack of integer indices to ud_parts */
  struct hashtable links;                 /* table of links for reference checking */
};

struct part_userdata {
  struct part_context part_context;
  unsigned long flags;
};

static int
part_want_link_check (const struct part_userdata *pd)
{
  return !(pd->flags & UDOC_PART_NO_LINK_CHECK);
}

static void
part_context_free (struct udoc *ud, struct part_context *p)
{
  dstring_free (&p->dstr);
  ud_part_index_stack_free (&p->index_stack);
  ht_free (&p->links);
}

static int
part_context_init (struct udoc *ud, struct part_context *p)
{
  bin_zero (p, sizeof (*p));
  ud_try_sys_jump (ud, dstring_init (&p->dstr, 256), FAIL, "dstring_init"); 
  ud_try_sys_jump (ud, ud_part_index_stack_init (&p->index_stack, 32), FAIL, "stack_init"); 
  ud_try_sys_jump (ud, ht_init (&p->links), FAIL, "hashtable_init"); 
  return 1;

  FAIL:
  part_context_free (ud, p);
  return 0;
}

static int
part_add (struct udoc *ud, struct part_context *pctx, unsigned long flags,
  const struct ud_node_list *list, const struct ud_node *node)
{
  char cnum[FMT_ULONG];
  struct ud_part p_new;
  struct ud_part *p_cur = 0;
  struct ud_part *p_up = 0;
  struct ud_part *pp_new = 0;
  int flag_part = 0;
  int flag_strdup = 0;

  bin_zero (&p_new, sizeof (p_new));
  if (ud_part_getcur (ud, &p_cur)) {
    p_new.up_file = p_cur->up_file;
    p_new.up_index_cur = p_cur->up_index_cur + 1;
    if (flags & UD_PART_SPLIT)
      p_new.up_file = p_new.up_index_cur;
    p_new.up_index_prev = p_cur->up_index_cur;
    p_cur->up_index_next = p_new.up_index_cur;
  }
  p_new.up_depth = ud_part_index_stack_size (&pctx->index_stack);
  p_new.up_node = node;
  p_new.up_list = list;
  p_new.up_flags = flags;

  /* get parent and add part */
  if (ud_part_getprev_up (ud, &p_new, &p_up))
    p_new.up_index_parent = p_up->up_index_cur;

  ud_try_sys_jump (ud, ud_part_add (ud, &p_new), FAIL, "part_add");
  flag_part = 1;

  /* get pointer to added part */
  ud_assert (ud_part_getcur (ud, &pp_new));

  /* work out string for use in table of contents, etc */
  ud_try_sys_jump (ud, ud_part_num_fmt (ud, pp_new, &pctx->dstr), FAIL, "num_fmt");
  ud_try_sys_jump (ud, str_dup (pctx->dstr.s, (char **) &pp_new->up_num_string),
    FAIL, "str_dup");
  flag_strdup = 1;
 
  /* push part onto stack */
  ud_try_sys_jump (ud, ud_part_index_stack_push (&pctx->index_stack,
    &p_new.up_index_cur), FAIL, "stack_push");

  /* chatter */
  cnum[fmt_ulong (cnum, p_new.up_index_cur)] = 0;
  log_2xf (LOG_DEBUG, "part ", cnum);
  if (flags & UD_PART_SPLIT) {
    cnum[fmt_ulong (cnum, p_new.up_file)] = 0;
    log_2xf (LOG_DEBUG, "part split ", cnum);
  }
  return UD_TREE_OK;

  FAIL:
  if (flag_part) flag_part = 1; /* TODO: remove part, fix links... */
  if (flag_strdup) dealloc_null ((void *) &pp_new->up_num_string);
  return UD_TREE_FAIL;
}

static int
part_section (struct udoc *ud, struct part_context *pctx,
  const struct ud_node_list *list, const struct ud_node *node)
{
  char cnum[FMT_ULONG];
  unsigned long flags = 0;
  const unsigned long size = ud_part_index_stack_size (&pctx->index_stack);

  cnum[fmt_ulong (cnum, size)] = 0;
  log_2xf (LOG_DEBUG, "part stack size ", cnum);

  if (size - 1 < ud->ud_opts.ud_split_thresh) {
    flags |= UD_PART_SPLIT;
  } else
    log_1xf (LOG_DEBUG, "split threshold exceeded - not splitting");

  if (!part_add (ud, pctx, flags, list, node)) return UD_TREE_FAIL;
  return UD_TREE_OK;
}

static int
part_subsection (struct udoc *ud, struct part_context *pctx,
  const struct ud_node_list *list, const struct ud_node *node)
{
  char cnum[FMT_ULONG];
  const unsigned long size = ud_part_index_stack_size (&pctx->index_stack);

  cnum[fmt_ulong (cnum, size)] = 0;
  log_2xf (LOG_DEBUG, "part stack size ", cnum);

  if (!part_add (ud, pctx, UD_PART_SUBSECT, list, node)) return UD_TREE_FAIL;
  return UD_TREE_OK;
}

static int
part_title (struct udoc *ud, struct part_context *pctx,
  struct ud_part *part, const struct ud_node *node)
{
  char cnum[FMT_ULONG];
  if (part->up_title) {
    cnum[fmt_ulong (cnum, node->un_next->un_line_num)] = 0;
    log_4x (LOG_WARN, ud->ud_cur_doc->ud_name, ": title on line ",
      cnum, " overwrites previous title");
  }
  part->up_title = node->un_next->un_data.un_str;
  log_2xf (LOG_DEBUG, "part title ", part->up_title);
  return UD_TREE_OK;
}

/*
 * link integrity checking.
 */

struct link_ctx {
  struct udoc *udoc;
  const struct ud_tree_ctx *tree_ctx;
  struct part_context *part_context;
  int ok;
};

static int
part_check_link (void *ku, unsigned long klen, void *du,
  unsigned long dlen, void *udata)
{
  char cnum[FMT_ULONG];
  const char *key = ku;
  struct link_ctx *link_ctx = udata;
  struct udoc *ud = link_ctx->udoc;
  const struct ud_tree_ctx *tree = link_ctx->tree_ctx;
  struct ud_ref *ref;
  struct dstring *buf = &link_ctx->part_context->dstr;
  unsigned long dummy;
  unsigned long line;
  struct ud_err ue;

  /* attempt to fetch ref from table */
  if (!ud_oht_get (&ud->ud_ref_names, key, klen, (void *) &ref, &dummy)) {
    line = tree->utc_state->utc_node->un_line_num;
    cnum[fmt_ulong (cnum, line)] = 0;

    dstring_trunc (buf);
    dstring_cats (buf, "link points to undefined reference \"");
    dstring_catb (buf, key, klen);
    dstring_cats (buf, "\"");
    dstring_0 (buf);

    ud_error_fill (ud, &ue, "link", cnum, buf->s, 0);
    ud_error_push (ud, &ue);
    link_ctx->ok = 0;
  }
  return 1;
}

static int
part_check_links (struct udoc *ud, const struct ud_tree_ctx *tree_ctx,
  struct part_context *pctx)
{
  struct link_ctx chk = { ud, tree_ctx, pctx, 1 };
  ht_iter (&pctx->links, part_check_link, &chk);
  return chk.ok;
}

/*
 * reference adding
 */

static int
part_ref_add_footnote (struct udoc *ud, const struct ud_ref *ref)
{
  return (ud_ref_add (&ud->ud_footnotes, ref) == 1);
}

static int
part_ref_add_style (struct udoc *ud, const struct ud_ref *ref)
{
  return (ud_ref_add (&ud->ud_styles, ref) == 1);
}

static int
part_ref_add_link_ext (struct udoc *ud, const struct ud_ref *ref)
{
  return (ud_ref_add_conditional (&ud->ud_link_exts, ref) == 1);
}

static int
part_ref_add_ref (struct udoc *ud, const struct ud_ref *ref)
{
  const char *ref_name = ref->ur_node->un_next->un_data.un_str;

  ud_tryS (ud, ud_ref_add_byname (&ud->ud_ref_names, ref_name, ref), 0,
    "ud_ref_add_byname", "could not add reference");
  return (ud_ref_add (&ud->ud_refs, ref) == 1);
}

/*
 * top level callbacks
 */

static enum ud_tree_walk_stat
cb_part_init (struct udoc *ud, struct ud_tree_ctx *tree_ctx)
{
  struct part_userdata *pdata = tree_ctx->utc_state->utc_user_data;
  struct part_context *pctx = &pdata->part_context;

  log_1xf (LOG_DEBUG, "adding root part");
  ud_try_jump (ud, part_context_init (ud, pctx),
    FAIL, "part_context_init");

  /* add root part - note that the root part is always 'split' */
  ud_try_jump (ud, part_add (ud, pctx, UD_PART_SPLIT, &ud->ud_tree.ut_root,
    ud->ud_tree.ut_root.unl_head), FAIL, "part_add");
  return UD_TREE_OK;

  FAIL:
  return UD_TREE_FAIL;
}

static enum ud_tree_walk_stat
cb_part_symbol (struct udoc *ud, struct ud_tree_ctx *tree_ctx)
{
  char line_num[FMT_ULONG];
  struct part_userdata *pdata = tree_ctx->utc_state->utc_user_data;
  struct part_context *pctx = &pdata->part_context;
  const struct ud_node *cur_node = tree_ctx->utc_state->utc_node;
  const struct ud_node_list *cur_list = tree_ctx->utc_state->utc_list;
  struct ud_part *part = 0;
  long *part_index = 0;
  enum ud_tag tag;
  struct ud_ref ref;

  if (tree_ctx->utc_state->utc_list_pos != 0) return UD_TREE_OK;
  if (!ud_tag_by_name (cur_node->un_data.un_sym, &tag)) return UD_TREE_OK;
  
  switch (tag) {
    case UDOC_TAG_SUBSECTION:
      return part_subsection (ud, pctx, cur_list, cur_node);
    case UDOC_TAG_SECTION:
      return part_section (ud, pctx, cur_list, cur_node);
    case UDOC_TAG_TITLE:
    case UDOC_TAG_REF:
    case UDOC_TAG_FOOTNOTE:
    case UDOC_TAG_STYLE:
      ud_assert (ud_part_index_stack_peek (&pctx->index_stack, &part_index));
      break;
    case UDOC_TAG_LINK:
      {
        /* add link target to table for integrity checking */
        const char *link_target = cur_node->un_next->un_data.un_str;
        const unsigned long len = str_len (link_target);
        const unsigned long dummy = 1;

        if (part_want_link_check (pdata)) {
          if (ht_checks (&pctx->links, link_target) == 0)
            ud_try_sys (ud, ht_addb (&pctx->links, link_target, len, &dummy, sizeof (dummy)) == 1,
                UD_TREE_FAIL, "link_table_add");
        }
      }
      break;
    default:
      break;
  }

  /* fill in part reference */
  ud_assert_s (cur_list, "current list is null");
  ud_assert_s (cur_node, "current node is null");

  ref.ur_list = cur_list;
  ref.ur_node = cur_node;
  ref.ur_part_index = (part_index) ? *part_index : UD_REF_PART_UNDEFINED;
 
  /* fetch part, if defined */
  if (ref.ur_part_index != UD_REF_PART_UNDEFINED)
    ud_assert (part = (struct ud_part *) ud_part_get (ud, ref.ur_part_index));

  /* add references */
  switch (tag) {
    case UDOC_TAG_TITLE:
      ud_assert_s (part, "part is null");
      return part_title (ud, pctx, part, cur_node);
    case UDOC_TAG_REF:
      ud_try_sys (ud, part_ref_add_ref (ud, &ref),
        UD_TREE_FAIL, "part_ref_add_ref");
      break;
    case UDOC_TAG_FOOTNOTE:
      ud_try_sys (ud, part_ref_add_footnote (ud, &ref),
        UD_TREE_FAIL, "part_ref_add_footnote");
      break;
    case UDOC_TAG_STYLE:
      ud_try_sys (ud, part_ref_add_style (ud, &ref),
        UD_TREE_FAIL, "part_ref_add_style");
      break;
    case UDOC_TAG_LINK_EXT:
      ud_try_sys (ud, part_ref_add_link_ext (ud, &ref),
        UD_TREE_FAIL, "part_ref_add_link_ext");
      break;
    case UDOC_TAG_RENDER_HEADER:
      if (ud->ud_render_header) {
        line_num[fmt_ulong (line_num, cur_node->un_line_num)] = 0;
        log_4x (LOG_WARN, ud->ud_cur_doc->ud_name, ": ", line_num,
          ": render-header overrides previous tag");
      }
      ud->ud_render_header = cur_node->un_next->un_data.un_str;
      break;
    case UDOC_TAG_RENDER_FOOTER:
      if (ud->ud_render_footer) {
        line_num[fmt_ulong (line_num, cur_node->un_line_num)] = 0;
        log_4x (LOG_WARN, ud->ud_cur_doc->ud_name, ": ", line_num,
          ": render-footer overrides previous tag");
      }
      ud->ud_render_footer = cur_node->un_next->un_data.un_str;
      break;
    default: break;
  }
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
cb_part_list_end (struct udoc *ud, struct ud_tree_ctx *tree_ctx)
{
  char cnum[FMT_ULONG];
  struct part_userdata *pdata = tree_ctx->utc_state->utc_user_data;
  struct part_context *pctx = &pdata->part_context;
  struct ud_node *first_sym;
  enum ud_tag tag;
  long *index;

  first_sym = tree_ctx->utc_state->utc_list->unl_head;
  if (!ud_tag_by_name (first_sym->un_data.un_sym, &tag)) return UD_TREE_OK;
  switch (tag) {
    case UDOC_TAG_SUBSECTION:
    case UDOC_TAG_SECTION:
      ud_assert (ud_part_index_stack_pop (&pctx->index_stack, &index));
      cnum[fmt_ulong (cnum, ud_part_index_stack_size (&pctx->index_stack))] = 0;
      log_2xf (LOG_DEBUG, "part stack size ", cnum);
      break;
    default: break;
  }
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
cb_part_finish (struct udoc *ud, struct ud_tree_ctx *tree_ctx)
{
  char cnum1[FMT_ULONG];
  char cnum2[FMT_ULONG];
  const unsigned long max = ud_oht_size (&ud->ud_parts);
  unsigned long index;
  unsigned long files = 0;
  struct ud_part *part;
  struct part_userdata *pdata = tree_ctx->utc_state->utc_user_data;
  struct part_context *pctx = &pdata->part_context;
  enum ud_tree_walk_stat ret = UD_TREE_FAIL;

  /* check link integrity */
  if (part_want_link_check (pdata))
    if (!part_check_links (ud, tree_ctx, pctx)) goto END;

  /* count files */
  for (index = 0; index < max; ++index)
    if (ud_oht_get_index (&ud->ud_parts, index, (void *) &part))
      if (part->up_flags & UD_PART_SPLIT && index) ++files;

  cnum1[fmt_ulong (cnum1, ud_oht_size (&ud->ud_parts))] = 0;
  cnum2[fmt_ulong (cnum2, files + 1)] = 0;
  log_4xf (LOG_DEBUG, cnum1, " parts, ", cnum2, " files");

  ret = UD_TREE_OK;
  END:
  ud_assert (ud_part_index_stack_size (&pctx->index_stack) == 1);
  part_context_free (ud, pctx);
  return ret;
}

static const struct ud_tree_ctx_funcs part_funcs = {
  .utcf_init = cb_part_init,
  .utcf_symbol = cb_part_symbol,
  .utcf_list_end = cb_part_list_end,
  .utcf_finish = cb_part_finish,
};

int
ud_partition (struct udoc *ud, unsigned long flags)
{
  struct ud_tree_ctx tree_ctx;
  struct ud_tree_ctx_state state;
  struct part_userdata pdata;

  bin_zero (&tree_ctx, sizeof (tree_ctx));
  bin_zero (&state, sizeof (state));

  pdata.flags = flags;

  state.utc_list = &ud->ud_tree.ut_root;
  state.utc_user_data = &pdata;

  tree_ctx.utc_funcs = &part_funcs;
  tree_ctx.utc_state = &state;

  switch (ud_tree_walk (ud, &tree_ctx)) {
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

const struct ud_part *
ud_part_get (const struct udoc *ud, unsigned long index)
{
  void *ret = 0;
  ud_oht_get_index (&ud->ud_parts, index, (void *) &ret);
  return ret;
}

int
ud_part_getfromnode (const struct udoc *ud, const struct ud_node *n,
  struct ud_part **ch, unsigned long *index)
{
  return ud_oht_get (&ud->ud_parts, (void *) &n, sizeof (&n), (void *) ch, index);
}

int
ud_part_getroot (const struct udoc *ud, struct ud_part **ch)
{
  return ud_oht_get_index (&ud->ud_parts, 0, (void *) ch);
}

int
ud_part_getcur (const struct udoc *ud, struct ud_part **rcur)
{
  struct ud_part *cur;
  unsigned long max = ud_oht_size (&ud->ud_parts);
  if (!max) return 0;
  if (!ud_oht_get_index (&ud->ud_parts, max - 1, (void *) &cur)) return 0;
  *rcur = cur;
  return 1;
}

int
ud_part_getprev (const struct udoc *ud, const struct ud_part *cur,
  struct ud_part **prev)
{
  struct ud_part *cprev;
  unsigned long max = cur->up_index_cur;
  unsigned long index;

  if (!max) return 0;
  for (index = max - 1; index; --index) {
    if (ud_oht_get_index (&ud->ud_parts, index, (void *) &cprev)) {
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
ud_part_getfirst_wparent (const struct udoc *ud, const struct ud_part *cur,
  struct ud_part **prev)
{
  struct ud_part *part_tmp = (struct ud_part *) cur;
  struct ud_part *part_first = (struct ud_part *) cur;

  for (;;) {
    if (!ud_oht_get_index (&ud->ud_parts, part_tmp->up_index_prev, (void *) &part_tmp)) break;
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
ud_part_getfirst_wdepth_noskip (const struct udoc *ud, const struct ud_part *cur,
  struct ud_part **prev)
{
  struct ud_part *part_tmp = (struct ud_part *) cur;
  struct ud_part *part_first = (struct ud_part *) cur;

  for (;;) {
    if (!ud_part_getprev (ud, part_tmp, &part_tmp)) break;
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
ud_part_getprev_up (const struct udoc *ud, const struct ud_part *cur,
  struct ud_part **prev)
{
  struct ud_part *cprev;
  const struct ud_part *cur_walk = cur;
  for (;;) {
    if (!ud_part_getprev (ud, cur_walk, &cprev)) return 0;
    if (cprev->up_depth < cur->up_depth) { *prev = cprev; return 1; }
    cur_walk = cprev;
  }
}

int
ud_part_add (struct udoc *ud, const struct ud_part *ch)
{
  switch (ud_oht_add (&ud->ud_parts, &ch->up_node, sizeof (&ch->up_node), ch)) {
    case 0: log_1xf (LOG_ERROR, "duplicate part node!"); return 0;
    case -1: log_1sysf (LOG_ERROR, 0); return 0;
    default: return 1;
  }
}

/*
 * get closest next part from a different file
 */

int
ud_part_getnext_file (const struct udoc *ud, const struct ud_part *cur,
  struct ud_part **next)
{
  struct ud_part *cnext;
  unsigned long max = ud_oht_size (&ud->ud_parts);
  unsigned long index;

  for (index = cur->up_index_cur; index < max; ++index) {
    if (ud_oht_get_index (&ud->ud_parts, index, (void *) &cnext)) {
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
ud_part_getprev_file (const struct udoc *ud, const struct ud_part *cur,
  struct ud_part **prev)
{
  struct ud_part *cprev;
  unsigned long max = cur->up_index_cur;
  unsigned long index;

  if (!max) return 0;
  for (index = max - 1; index + 1; --index) {
    if (ud_oht_get_index (&ud->ud_parts, index, (void *) &cprev)) {
      if (cprev->up_file != cur->up_file) {
        *prev = cprev;
        return 1;
      }
    }
  }
  return 0;
}

static unsigned long
ud_part_offset_wparent (const struct udoc *ud,
  const struct ud_part *from, const struct ud_part *to)
{
  unsigned long off = 0;
  struct ud_part *part_tmp = (struct ud_part *) from;

  for (;;) {
    if (part_tmp == to) break;
    if (!ud_oht_get_index (&ud->ud_parts, part_tmp->up_index_next, (void *) &part_tmp)) break;
    if (part_tmp->up_index_parent == from->up_index_parent) ++off;
  }
  return off;
}

int
ud_part_num_fmt (const struct udoc *ud, const struct ud_part *part,
  struct dstring *ds)
{
  char cnum[FMT_ULONG];
  char sbuf[32 * sizeof (unsigned long)];
  struct sstack ns;
  struct ud_part *part_cur;
  struct ud_part *part_first;
  unsigned long *num;
  unsigned long off;

  sstack_init (&ns, sbuf, sizeof (sbuf), sizeof (unsigned long));

  /* push parent parts onto stack */
  part_cur = (struct ud_part *) part;
  for (;;) {
    if (!sstack_push (&ns, (void *) &part_cur->up_index_cur)) break;
    if (!part_cur->up_index_parent) break;
    if (!ud_oht_get_index (&ud->ud_parts, part_cur->up_index_parent, (void *) &part_cur)) break;
  }

  /* repeatedly pop parents and calculate offsets for numbering */
  dstring_trunc (ds);
  for (;;) {
    if (!sstack_pop (&ns, (void *) &num)) break;
    if (!ud_oht_get_index (&ud->ud_parts, *num, (void *) &part_cur)) break;

    ud_part_getfirst_wparent (ud, part_cur, &part_first);
    off = ud_part_offset_wparent (ud, part_first, part_cur);

    /* annoying hack to allow root node to be 0 but to allow subsections
     * to be numbered from 1 instead of 0.
     */

    if (ds->len) ++off;
    if (!dstring_catb (ds, cnum, fmt_ulong (cnum, off))) return 0;
    if (!dstring_catb (ds, ".", 1)) return 0;
  }

  /* remove possible trailing '.' */
  if (ds->s[ds->len - 1] == '.') dstring_chop (ds, ds->len - 1);

  dstring_0 (ds);
  return 1;
}

/* ud_part index stack */
GEN_stack_define (ud_part_index);

