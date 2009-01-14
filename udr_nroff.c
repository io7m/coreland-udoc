#include <chrono/caltime.h>
#include <chrono/taia.h>

#include <corelib/alloc.h>
#include <corelib/buffer.h>
#include <corelib/dstring.h>
#include <corelib/error.h>
#include <corelib/fmt.h>
#include <corelib/open.h>
#include <corelib/read.h>
#include <corelib/scan.h>
#include <corelib/sstring.h>
#include <corelib/str.h>

#include "multi.h"
#include "log.h"

#define UDOC_IMPLEMENTATION
#include "udoc.h"
#include "ud_assert.h"
#include "ud_ref.h"
#include "ud_table.h"

#define PAGE_WIDTH 80

struct nroff_ctx {
  unsigned int in_footnote;
  unsigned int in_list;
  unsigned int in_table;
  unsigned int in_verbatim;
};

static const char *
nr_t_bslash (struct dfo_put *dp) { return "\\\\"; }
static const struct dfo_trans nr_trans[] = {
  { '\\', nr_t_bslash },
};
static const unsigned int nr_trans_size = sizeof (nr_trans) / sizeof (nr_trans[0]);
static int
nroff_put (struct buffer *out, const char *str, unsigned long len, void *data)
{
  return dfo_put (data, str, len);
}

/* do not output nroff directives inside tags that may have already done so */
static int
nroff_here (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct nroff_ctx *nc = render_ctx->uc_user_data;
  return (!nc->in_footnote && !nc->in_list && !nc->in_table);
}

static void
nroff_literal_start (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;
  dfo_break_line (dfo);
  dfo_puts (dfo, ".nh"); /* disable hyphenation */
  dfo_break_line (dfo);
  dfo_puts (dfo, ".na"); /* disable margin adjustment */
  dfo_break_line (dfo);
  dfo_puts (dfo, ".nf"); /* disable filling */
  dfo_break_line (dfo);
}

static void
nroff_literal_end (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;
  dfo_break_line (dfo);
  dfo_puts (dfo, ".hy"); /* enable hyphenation */
  dfo_break_line (dfo);
  dfo_puts (dfo, ".ad b"); /* enable margin adjustment ('b' == both) */
  dfo_break_line (dfo);
  dfo_puts (dfo, ".fi"); /* enable filling */
  dfo_break_line (dfo);
}

static int
rn_literal_start (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;
  struct nroff_ctx *nc = render_ctx->uc_user_data;

  if (!nc->in_verbatim && nroff_here (ud, render_ctx)) {
    dfo_wrap_mode (dfo, DFO_WRAP_NONE);
    dfo_tran_disable (dfo, DFO_TRAN_RESPACE);
    nroff_literal_start (ud, render_ctx);
    nc->in_verbatim = 1;
  }
  return 1;
}

static int
rn_literal_end (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;
  struct nroff_ctx *nc = render_ctx->uc_user_data;

  if (nc->in_verbatim && nroff_here (ud, render_ctx)) {
    nroff_literal_end (ud, render_ctx);
    dfo_wrap_mode (dfo, DFO_WRAP_SOFT);
    dfo_tran_enable (dfo, DFO_TRAN_RESPACE);
    nc->in_verbatim = 0;
  }
  return 1;
}

static enum ud_tree_walk_stat
rn_tag_contents (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct ud_part *part_cur = (struct ud_part *) render_ctx->uc_part;
  const struct ud_part *part_first = part_cur;
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;
  const unsigned long page_width = render_ctx->uc_opts->uo_page_width;
  unsigned long index = part_cur->up_index_cur;

  dfo_constrain (dfo, page_width, dfo->page_indent + 2);
  dfo_tran_disable (dfo, DFO_TRAN_RESPACE);
  dfo_wrap_mode (dfo, DFO_WRAP_NONE);

  for (;;) {
    ud_assert (ud_oht_get_index (&ud->ud_parts, index, (void *) &part_cur));
    if (part_cur->up_depth <= part_first->up_depth)
      if (part_cur != part_first) break;

    dfo_constrain (dfo, page_width,
      dfo->page_indent + ((part_cur->up_depth - part_first->up_depth) * 2));

    dfo_puts3 (dfo, part_cur->up_num_string, ". ", part_cur->up_title);
    dfo_break_line (dfo);

    dfo_constrain (dfo, page_width,
      dfo->page_indent - ((part_cur->up_depth - part_first->up_depth) * 2));

    if (part_cur->up_depth < part_first->up_depth) break;
    if (!part_cur->up_index_next) break;
    index = part_cur->up_index_next;
  }

  dfo_constrain (dfo, page_width, dfo->page_indent - 2);
  dfo_tran_enable (dfo, DFO_TRAN_RESPACE);
  dfo_wrap_mode (dfo, DFO_WRAP_HYPH);
  dfo_break_line (dfo);
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rn_tag_footnote (struct udoc *ud, struct udr_ctx *render_ctx)
{
  char cnum[FMT_ULONG];
  unsigned long max;
  unsigned long index;
  struct ud_ref *ref;
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;
  const struct ud_node_list *list = render_ctx->uc_tree_ctx->utc_state->utc_list;

  max = ud_oht_size (&ud->ud_footnotes);
  for (index = 0; index < max; ++index) {
    ud_assert (ud_oht_get_index (&ud->ud_footnotes, index, (void *) &ref));
    if (list == ref->ur_list) {
      cnum[fmt_ulong (cnum, index)] = 0;
      dfo_puts3 (dfo, "[", cnum, "]");
      break;
    }
  }
  return UD_TREE_STOP_LIST;
}

static unsigned long
rn_fmt_footnote (char *cnum, unsigned long index)
{
  unsigned long len = 0;
  unsigned long pos = 0;
  char *str = cnum;

  pos = fmt_str (str, "["); len += pos; str += pos;
  pos = fmt_ulong (str, index); len += pos; str += pos;
  pos = fmt_str (str, "] "); len += pos; str += pos;
  return len;
}

static enum ud_tree_walk_stat
rn_footnotes (struct udoc *ud, struct udr_ctx *render_ctx)
{
  char cnum[FMT_ULONG + sizeof ("  [] ")];
  const unsigned long max = ud_oht_size (&ud->ud_footnotes);
  unsigned long index = 0;
  unsigned long len = 0;
  struct ud_ref *u;
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;
  struct nroff_ctx *nc = render_ctx->uc_user_data;
  struct udr_ctx rtmp = *render_ctx;
  unsigned int old_indent;
  const unsigned long page_width = render_ctx->uc_opts->uo_page_width;

  if (!max) goto END;

  dfo_break_line (dfo);
  dfo_puts (dfo, "--");
  dfo_break_line (dfo);
  dfo_break_line (dfo);

  dfo_constrain (dfo, page_width, dfo->page_indent + 2);
  for (index = 0; index < max; ++index) {
    ud_assert (ud_oht_get_index (&ud->ud_footnotes, index, (void *) &u));
    len = rn_fmt_footnote (cnum, index);
    cnum[len] = 0;
    dfo_put (dfo, cnum, len);
    dfo_break (dfo);

    old_indent = dfo->page_indent;
    dfo_constrain (dfo, page_width, len);
    nc->in_footnote = 1;

    rtmp.uc_tree_ctx = 0;
    rtmp.uc_flag_finish_file = 0;
    if (!ud_render_node (ud, &rtmp,
      &u->ur_list->unl_head->un_next->un_data.un_list)) return 0;

    nc->in_footnote = 0;
    dfo_constrain (dfo, page_width, old_indent);

    dfo_break_line (dfo);
    dfo_break_line (dfo);
  }
  dfo_constrain (dfo, page_width, dfo->page_indent - 2);
  dfo_puts (dfo, "--");
  dfo_break_line (dfo);
  dfo_break_line (dfo);

  END:
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rn_tag_section (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;

  dfo_puts2 (dfo, render_ctx->uc_part->up_num_string, ".");
  if (render_ctx->uc_part->up_title)
    dfo_puts2 (dfo, " ", render_ctx->uc_part->up_title);

  dfo_break_line (dfo);
  dfo_break_line (dfo);
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rn_tag_link_ext (struct udoc *ud, struct udr_ctx *render_ctx)
{
  const struct ud_node *node = render_ctx->uc_tree_ctx->utc_state->utc_node;
  const char *link;
  const char *text = 0;
  const char *space;
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;
  const struct dfo_buffer *buf = dfo_current_buf (dfo);

  link = node->un_next->un_data.un_str;
  if (node->un_next)
    text = (node->un_next->un_next) ? node->un_next->un_next->un_data.un_str : 0;

  space = (buf->line_pos) ? " " : 0;
  if (text)
    dfo_puts5 (dfo, space, text, " (", link, ")");
  else
    dfo_puts2 (dfo, space, link);

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rn_tag_link (struct udoc *ud, struct udr_ctx *render_ctx)
{
  const char *ref;
  const char *text;
  const struct ud_node *node = render_ctx->uc_tree_ctx->utc_state->utc_node;
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;

  ref = node->un_next->un_data.un_str;
  text = (node->un_next->un_next) ? node->un_next->un_next->un_data.un_str : 0;

  if (text)
    dfo_puts5 (dfo, "[", text, " (", ref, ")]");
  else
    dfo_puts3 (dfo, "[", ref, "]");

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rn_tag_ref (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct ud_node *node = (struct ud_node *) render_ctx->uc_tree_ctx->utc_state->utc_node;
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;

  dfo_puts3 (dfo, "[ref: ", node->un_next->un_data.un_str, "]");
  dfo_break_line (dfo);
  dfo_break_line (dfo);

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rn_tag_render (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;

  if (!rn_literal_start (ud, render_ctx)) return UD_TREE_FAIL;

  if (!udr_print_file (ud, render_ctx,
    render_ctx->uc_tree_ctx->utc_state->utc_node->un_next->un_data.un_str,
    nroff_put, dfo)) return UD_TREE_FAIL;

  if (!rn_literal_end (ud, render_ctx)) return UD_TREE_FAIL;

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rn_tag_render_noescape (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;

  dfo_tran_disable (dfo, DFO_TRAN_CONVERT);

  if (!udr_print_file (ud, render_ctx,
    render_ctx->uc_tree_ctx->utc_state->utc_node->un_next->un_data.un_str,
    nroff_put, dfo))
    return UD_TREE_FAIL;

  dfo_tran_enable (dfo, DFO_TRAN_CONVERT);

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rn_tag_date (struct udoc *ud, struct udr_ctx *render_ctx)
{
  char buf[CALTIME_FMT];
  struct caltime ct;
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;

  caltime_local (&ct, &ud->ud_time_start.sec, 0, 0);
  dfo_put (dfo, buf, caltime_fmt (buf, &ct));
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rn_tag_table (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct ud_table tab;
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;
  struct udr_ctx rtmp = *render_ctx;
  const struct ud_node *n = render_ctx->uc_tree_ctx->utc_state->utc_node;
  const unsigned long page_width = render_ctx->uc_opts->uo_page_width;

  dfo_break_line (dfo);
  dfo_break_line (dfo);

  ud_tryS (ud, dfo_flush (dfo) != -1, UD_TREE_FAIL, "dfo_flush",
    dfo_errorstr (dfo->error));
  ud_table_measure (ud, render_ctx->uc_tree_ctx->utc_state->utc_list, &tab);

  dfo_constrain (dfo, page_width, 2);

  /* for each item in list, render list (row) */
  for (;;) {
    if (n->un_type == UDOC_TYPE_LIST) {
      ud_tryS (ud, dfo_columns_setup (dfo, tab.ut_cols, 2), UD_TREE_FAIL,
        "dfo_columns_setup", dfo_errorstr (dfo->error));

      rtmp.uc_tree_ctx = 0;
      rtmp.uc_flag_finish_file = 0;
      if (!ud_render_node (ud, &rtmp, &n->un_data.un_list)) return UD_TREE_FAIL;

      ud_tryS (ud, dfo_columns_end (dfo), UD_TREE_FAIL, "dfo_columns_end",
        dfo_errorstr (dfo->error));
    }
    if (n->un_next) n = n->un_next; else break;
  }

  ud_tryS (ud, dfo_flush (dfo) != -1, UD_TREE_FAIL, "dfo_flush",
    dfo_errorstr (dfo->error));

  dfo_break_line (dfo);
  dfo_constrain (dfo, page_width, 0);
  return UD_TREE_STOP_LIST;
}

static enum ud_tree_walk_stat
rn_tag_table_row (struct udoc *ud, struct udr_ctx *render_ctx)
{
  const struct ud_node *n = render_ctx->uc_tree_ctx->utc_state->utc_node;
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;
  struct udr_ctx rtmp = *render_ctx;

  /* for each item in list, render list (cell) */
  for (;;) {
    if (n->un_type == UDOC_TYPE_LIST) {
      dfo_columns_start (dfo);

      rtmp.uc_tree_ctx = 0;
      rtmp.uc_flag_finish_file = 0;
      if (!ud_render_node (ud, &rtmp, &n->un_data.un_list)) return UD_TREE_FAIL;

      dfo_break (dfo);
    }
    if (n->un_next) n = n->un_next; else break;
  }
  return UD_TREE_STOP_LIST;
}

static enum ud_tree_walk_stat
rn_tag_list (struct udoc *ud, struct udr_ctx *render_ctx)
{
  char cnum[FMT_ULONG];
  const struct ud_node *n = render_ctx->uc_tree_ctx->utc_state->utc_node;
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;
  struct nroff_ctx *nc = render_ctx->uc_user_data;
  struct udr_ctx rtmp = *render_ctx;
  unsigned int old_indent = dfo->page_indent;
  unsigned int alt = 0;
  const char *ch;
  const unsigned long page_width = render_ctx->uc_opts->uo_page_width;

  nc->in_list = 1;
  cnum[fmt_ulong (cnum, old_indent + 2)] = 0;

  alt = (old_indent + 2) % 4;
  ch = (alt) ? "* " : "o ";

  /* for each item in list, render list */
  dfo_constrain (dfo, page_width, 0);
  for (;;) {
    if (n->un_type == UDOC_TYPE_LIST) {
      dfo_puts2 (dfo, ".in ", cnum);
      dfo_break_line (dfo);
      dfo_puts2 (dfo, ch, " ");

      rtmp.uc_tree_ctx = 0;
      rtmp.uc_flag_finish_file = 0;
      if (!ud_render_node (ud, &rtmp, &n->un_data.un_list)) return UD_TREE_FAIL;

      dfo_break_line (dfo);
      dfo_puts (dfo, ".in 0");
      dfo_break_line (dfo);
    }
    if (n->un_next) n = n->un_next; else break;
  }
  dfo_break (dfo);
  dfo_constrain (dfo, page_width, old_indent);

  nc->in_list = 0;
  return UD_TREE_STOP_LIST;
}

static enum ud_tree_walk_stat
rn_tag_para (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;
 
  if (nroff_here (ud, render_ctx)) {
    dfo_puts (dfo, ".in 2");
    dfo_break_line (dfo);
  }
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rn_tag_para_verbatim (struct udoc *ud, struct udr_ctx *render_ctx)
{
  if (!rn_literal_start (ud, render_ctx)) return UD_TREE_FAIL;
  return UD_TREE_OK;
}

/* tag ends */

static enum ud_tree_walk_stat
rn_tag_end_para (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;

  if (nroff_here (ud, render_ctx)) {
    dfo_break_line (dfo);
    dfo_puts (dfo, ".in 0");
  }
  dfo_break_line (dfo);
  dfo_break_line (dfo);

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rn_tag_end_para_verbatim (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;
  if (!rn_literal_end (ud, render_ctx)) return UD_TREE_FAIL;
  dfo_break_line (dfo);
  dfo_break_line (dfo);
  return UD_TREE_OK;
}

/* tables */

struct dispatch {
  enum ud_tag tag;
  enum ud_tree_walk_stat (*func) (struct udoc *, struct udr_ctx *);
};

static const struct dispatch tag_starts[] = {
  { UDOC_TAG_PARA,            rn_tag_para },
  { UDOC_TAG_PARA_VERBATIM,   rn_tag_para_verbatim },
  { UDOC_TAG_SECTION,         rn_tag_section },
  { UDOC_TAG_SUBSECTION,      rn_tag_section },
  { UDOC_TAG_REF,             rn_tag_ref },
  { UDOC_TAG_LINK,            rn_tag_link },
  { UDOC_TAG_LINK_EXT,        rn_tag_link_ext },
  { UDOC_TAG_TABLE,           rn_tag_table },
  { UDOC_TAG_CONTENTS,        rn_tag_contents },
  { UDOC_TAG_FOOTNOTE,        rn_tag_footnote },
  { UDOC_TAG_DATE,            rn_tag_date },
  { UDOC_TAG_RENDER,          rn_tag_render },
  { UDOC_TAG_RENDER_NOESCAPE, rn_tag_render_noescape },
  { UDOC_TAG_TABLE,           rn_tag_table },
  { UDOC_TAG_TABLE_ROW,       rn_tag_table_row },
  { UDOC_TAG_LIST,            rn_tag_list },
};
static const unsigned int tag_starts_size = sizeof (tag_starts)
                                          / sizeof (tag_starts[0]);
static const struct dispatch tag_ends[] = {
  { UDOC_TAG_PARA,          rn_tag_end_para },
  { UDOC_TAG_PARA_VERBATIM, rn_tag_end_para_verbatim },
};
static const unsigned int tag_ends_size = sizeof (tag_ends)
                                        / sizeof (tag_ends[0]);

static enum ud_tree_walk_stat
dispatch (const struct dispatch *tab, unsigned int tab_size,
  struct udoc *ud, struct udr_ctx *ctx, enum ud_tag tag)
{
  unsigned int index;
  for (index = 0; index < tab_size; ++index)
    if (tag == tab[index].tag)
      return tab[index].func (ud, ctx);
  return UD_TREE_OK;
}

/*
 * main rendering callbacks
 */

static enum ud_tree_walk_stat
rn_symbol (struct udoc *ud, struct udr_ctx *render_ctx)
{
  const char *sym = render_ctx->uc_tree_ctx->utc_state->utc_list->unl_head->un_data.un_sym;
  enum ud_tag tag;

  if (render_ctx->uc_tree_ctx->utc_state->utc_list_pos == 0)
    if (ud_tag_by_name (sym, &tag))
      return dispatch (tag_starts, tag_starts_size, ud, render_ctx, tag);

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rn_string (struct udoc *ud, struct udr_ctx *render_ctx)
{
  const struct ud_tree_ctx *tc = render_ctx->uc_tree_ctx;
  enum ud_tag tag;
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;

  if (!ud_tag_by_name (tc->utc_state->utc_list->unl_head->un_data.un_sym, &tag))
    return UD_TREE_OK;

  switch (tag) {
    case UDOC_TAG_CONTENTS:
    case UDOC_TAG_ENCODING:
    case UDOC_TAG_FOOTNOTE:
    case UDOC_TAG_LINK:
    case UDOC_TAG_LINK_EXT:
    case UDOC_TAG_LIST:
    case UDOC_TAG_REF:
    case UDOC_TAG_RENDER:
    case UDOC_TAG_RENDER_FOOTER:
    case UDOC_TAG_RENDER_HEADER:
    case UDOC_TAG_RENDER_NOESCAPE:
    case UDOC_TAG_STYLE:
    case UDOC_TAG_TABLE:
    case UDOC_TAG_TABLE_ROW:
    case UDOC_TAG_TITLE:
      break;
    case UDOC_TAG_PARA_VERBATIM:
    case UDOC_TAG_PARA:
    default:
      dfo_puts (dfo, render_ctx->uc_tree_ctx->utc_state->utc_node->un_data.un_str);
      break;
  }
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rn_list_end (struct udoc *ud, struct udr_ctx *render_ctx)
{
  const char *sym = render_ctx->uc_tree_ctx->utc_state->utc_list->unl_head->un_data.un_sym;
  enum ud_tag tag;
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;

  if (ud_tag_by_name (sym, &tag))
    return dispatch (tag_ends, tag_ends_size, ud, render_ctx, tag);

  ud_tryS (ud, dfo_flush (dfo) != -1, UD_TREE_FAIL, "dfo_flush",
    dfo_errorstr (dfo->error));

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rn_file_init (struct udoc *ud, struct udr_ctx *render_ctx)
{
  char cnum[FMT_ULONG];
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;
  const unsigned long page_width = render_ctx->uc_opts->uo_page_width;

  render_ctx->uc_user_data = alloc_zero (sizeof (struct nroff_ctx));
  if (!render_ctx->uc_user_data) return UD_TREE_FAIL;

  if (!dfo_init (dfo, &render_ctx->uc_out->uoc_buffer, nr_trans, nr_trans_size))
    return UD_TREE_FAIL;

  ud_tryS (ud, dfo_constrain (dfo, page_width, 0), UD_TREE_FAIL, "dfo_constrain",
    dfo_errorstr (dfo->error));

  cnum[fmt_ulong (cnum, dfo->page_max)] = 0;
  dfo_puts2 (dfo, ".ll ", cnum);
  dfo_break_line (dfo);

  if (ud->ud_render_header)
    if (!udr_print_file (ud, render_ctx, ud->ud_render_header, nroff_put, dfo))
      return UD_TREE_FAIL;

  dfo_tran_enable (dfo, DFO_TRAN_RESPACE);
  dfo_wrap_mode (dfo, DFO_WRAP_SOFT);

  if (render_ctx->uc_part->up_title) {
    dfo_puts (dfo, ".in 2");
    dfo_break_line (dfo);
    dfo_puts (dfo, render_ctx->uc_part->up_title);
    dfo_break_line (dfo);
    dfo_puts (dfo, ".in 0");
    dfo_break_line (dfo);
    dfo_break_line (dfo);
  }
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rn_file_finish (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;

  if (!rn_footnotes (ud, render_ctx)) return UD_TREE_FAIL;
  if (ud->ud_render_footer)
    if (!udr_print_file (ud, render_ctx, ud->ud_render_footer, nroff_put, dfo))
      return UD_TREE_FAIL;

  ud_tryS (ud, dfo_flush (dfo) != -1, UD_TREE_FAIL, "dfo_flush",
    dfo_errorstr (dfo->error));

  dealloc_null (&render_ctx->uc_user_data);
  return UD_TREE_OK;
}

const struct ud_renderer ud_render_nroff = {
  {
    .urf_file_init = rn_file_init,
    .urf_symbol = rn_symbol,
    .urf_string = rn_string,
    .urf_list_end = rn_list_end,
    .urf_file_finish = rn_file_finish,
  },
  {
    .ur_name = "nroff",
    .ur_suffix = "nrf",
    .ur_desc = "nroff output",
    .ur_part = 0,
  },
};
