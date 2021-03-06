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

#include "dfo.h"
#include "multi.h"

#define UDOC_IMPLEMENTATION
#include "udoc.h"
#include "ud_assert.h"
#include "ud_ref.h"
#include "ud_table.h"

static int
plain_put (struct buffer *out, const char *str, unsigned long len, void *data)
{
  return dfo_put (data, str, len);
}

static enum ud_tree_walk_stat
rp_tag_contents (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct ud_part *part_cur = (struct ud_part *) render_ctx->uc_part;
  struct ud_part *part_first = part_cur;
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

    /* increase indent */
    dfo_constrain (dfo, page_width,
      dfo->page_indent + ((part_cur->up_depth - part_first->up_depth) * 2));

    dfo_puts3 (dfo, part_cur->up_num_string, " ", part_cur->up_title);
    dfo_break_line (dfo);

    /* decrease indent */
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
rp_tag_footnote (struct udoc *ud, struct udr_ctx *render_ctx)
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
rp_fmt_footnote (char *cnum, unsigned long index)
{
  unsigned long len = 0;
  unsigned long pos = 0;
  char *str = cnum;

  pos = fmt_str (str, "["); len += pos; str += pos;
  pos = fmt_ulong (str, index); len += pos; str += pos;
  pos = fmt_str (str, "] "); len += pos; str += pos;
  return len;
}

static int
rp_footnotes (struct udoc *ud, struct udr_ctx *render_ctx)
{
  char cnum[FMT_ULONG + sizeof ("  [] ")];
  const unsigned long max = ud_oht_size (&ud->ud_footnotes);
  const unsigned long page_width = render_ctx->uc_opts->uo_page_width;
  unsigned long index = 0;
  unsigned long len = 0;
  unsigned long old_indent;
  struct ud_ref *u;
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;
  struct udr_ctx rtmp = *render_ctx;

  if (!max) goto END;

  dfo_puts (dfo, "--");
  dfo_break_line (dfo);
  dfo_break_line (dfo);

  dfo_constrain (dfo, page_width, dfo->page_indent + 2);
  for (index = 0; index < max; ++index) {
    ud_assert (ud_oht_get_index (&ud->ud_footnotes, index, (void *) &u));
    len = rp_fmt_footnote (cnum, index);
    cnum[len] = 0;
    dfo_put (dfo, cnum, len);
    dfo_break (dfo);

    old_indent = dfo->page_indent;
    dfo_constrain (dfo, page_width, len);

    rtmp.uc_tree_ctx = 0;
    rtmp.uc_flag_finish_file = 0;
    if (!ud_render_node (ud, &rtmp,
      &u->ur_list->unl_head->un_next->un_data.un_list)) return 0;

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
rp_tag_section (struct udoc *ud, struct udr_ctx *render_ctx)
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
rp_tag_link_ext (struct udoc *ud, struct udr_ctx *render_ctx)
{
  const struct ud_node *node = render_ctx->uc_tree_ctx->utc_state->utc_node;
  const char *link;
  const char *text = 0;
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;

  link = node->un_next->un_data.un_str;
  if (node->un_next)
    text = (node->un_next->un_next) ? node->un_next->un_next->un_data.un_str : 0;

  if (text)
    dfo_puts4 (dfo, text, " (", link, ")");
  else
    dfo_puts (dfo, link);

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rp_tag_link (struct udoc *ud, struct udr_ctx *render_ctx)
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
rp_tag_ref (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct ud_node *node = (struct ud_node *) render_ctx->uc_tree_ctx->utc_state->utc_node;
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;

  dfo_puts3 (dfo, "[ref: ", node->un_next->un_data.un_str, "]");
  dfo_break_line (dfo);
  dfo_break_line (dfo);

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rp_tag_render (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;

  if (!udr_print_file (ud, render_ctx,
    render_ctx->uc_tree_ctx->utc_state->utc_node->un_next->un_data.un_str,
    plain_put, dfo)) return UD_TREE_FAIL;

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rp_tag_date (struct udoc *ud, struct udr_ctx *render_ctx)
{
  char buf[CALTIME_FMT];
  struct caltime ct;
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;

  caltime_local (&ct, &ud->ud_time_start.sec, 0, 0);
  dfo_put (dfo, buf, caltime_fmt (buf, &ct));
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rp_tag_table (struct udoc *ud, struct udr_ctx *render_ctx)
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
  dfo_constrain (dfo, ud_table_advise_width (ud, &tab), 2);

  /* for each item in list, render list (row) */
  for (;;) {
    if (n->un_type == UDOC_TYPE_LIST) {
      ud_tryS (ud, dfo_columns_setup (dfo, tab.ut_cols, 2), UD_TREE_FAIL,
        "dfo_columns_setup", dfo_errorstr (dfo->error));

      rtmp.uc_tree_ctx = 0;
      rtmp.uc_flag_finish_file = 0;
      if (!ud_render_node (ud, &rtmp, &n->un_data.un_list)) return UD_TREE_FAIL;

      ud_tryS (ud, dfo_columns_end (dfo), UD_TREE_FAIL,
        "dfo_columns_end", dfo_errorstr (dfo->error));
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
rp_tag_table_row (struct udoc *ud, struct udr_ctx *render_ctx)
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
rp_tag_list (struct udoc *ud, struct udr_ctx *render_ctx)
{
  const struct ud_node *n = render_ctx->uc_tree_ctx->utc_state->utc_node;
  const unsigned long page_width = render_ctx->uc_opts->uo_page_width;
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;
  struct udr_ctx rtmp = *render_ctx;

  /* for each item in list, render list */
  for (;;) {
    if (n->un_type == UDOC_TYPE_LIST) {
      dfo_constrain (dfo, page_width, dfo->page_indent + 2);
      dfo_puts (dfo, "* ");

      rtmp.uc_tree_ctx = 0;
      rtmp.uc_flag_finish_file = 0;
      if (!ud_render_node (ud, &rtmp, &n->un_data.un_list)) return UD_TREE_FAIL;

      dfo_constrain (dfo, page_width, dfo->page_indent - 2);
      dfo_break (dfo);
    }
    if (n->un_next) n = n->un_next; else break;
  }
  dfo_break (dfo);
  return UD_TREE_STOP_LIST;
}

static enum ud_tree_walk_stat
rp_tag_para_verbatim (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;
  dfo_wrap_mode (dfo, DFO_WRAP_NONE);
  dfo_tran_disable (dfo, DFO_TRAN_RESPACE);
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rp_tag_para (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;
  const unsigned long page_width = render_ctx->uc_opts->uo_page_width;

  dfo_wrap_mode (dfo, DFO_WRAP_HYPH);
  dfo_tran_enable (dfo, DFO_TRAN_RESPACE);
  dfo_constrain (dfo, page_width, dfo->page_indent + 2);
  return UD_TREE_OK;
}

/* tables */

struct dispatch {
  enum ud_tag tag;
  enum ud_tree_walk_stat (*func) (struct udoc *, struct udr_ctx *);
};

static const struct dispatch tag_starts[] = {
  { UDOC_TAG_PARA,            rp_tag_para },
  { UDOC_TAG_PARA_VERBATIM,   rp_tag_para_verbatim },
  { UDOC_TAG_SECTION,         rp_tag_section },
  { UDOC_TAG_SUBSECTION,      rp_tag_section },
  { UDOC_TAG_REF,             rp_tag_ref },
  { UDOC_TAG_LINK,            rp_tag_link },
  { UDOC_TAG_LINK_EXT,        rp_tag_link_ext },
  { UDOC_TAG_TABLE,           rp_tag_table },
  { UDOC_TAG_CONTENTS,        rp_tag_contents },
  { UDOC_TAG_FOOTNOTE,        rp_tag_footnote },
  { UDOC_TAG_DATE,            rp_tag_date },
  { UDOC_TAG_RENDER,          rp_tag_render },
  { UDOC_TAG_RENDER_NOESCAPE, rp_tag_render },
  { UDOC_TAG_TABLE,           rp_tag_table },
  { UDOC_TAG_TABLE_ROW,       rp_tag_table_row },
  { UDOC_TAG_LIST,            rp_tag_list },
};
static const unsigned int tag_starts_size = sizeof (tag_starts)
                                          / sizeof (tag_starts[0]);
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
rp_file_init (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;
  const unsigned long page_width = render_ctx->uc_opts->uo_page_width;

  if (!dfo_init (dfo, &render_ctx->uc_out->uoc_buffer, 0, 0)) return UD_TREE_FAIL;

  ud_tryS (ud, dfo_constrain (dfo, page_width, 0), UD_TREE_FAIL, "dfo_constrain",
    dfo_errorstr (dfo->error));

  if (ud->ud_render_header)
    if (!udr_print_file (ud, render_ctx, ud->ud_render_header, plain_put, dfo))
      return UD_TREE_FAIL;

  if (render_ctx->uc_part->up_title) {
    dfo_constrain (dfo, page_width, 2);
    dfo_puts (dfo, render_ctx->uc_part->up_title);
    dfo_break_line (dfo);
    dfo_break_line (dfo);
    dfo_constrain (dfo, page_width, 0);
  }

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rp_symbol (struct udoc *ud, struct udr_ctx *render_ctx)
{
  const char *sym = render_ctx->uc_tree_ctx->utc_state->utc_list->unl_head->un_data.un_sym;
  enum ud_tag tag;

  if (render_ctx->uc_tree_ctx->utc_state->utc_list_pos == 0)
    if (ud_tag_by_name (sym, &tag))
      return dispatch (tag_starts, tag_starts_size, ud, render_ctx, tag);

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rp_string (struct udoc *ud, struct udr_ctx *render_ctx)
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
rp_list_end (struct udoc *ud, struct udr_ctx *render_ctx)
{
  const struct ud_tree_ctx *tc = render_ctx->uc_tree_ctx;
  enum ud_tag tag;
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;
  const unsigned long page_width = render_ctx->uc_opts->uo_page_width;

  if (!ud_tag_by_name (tc->utc_state->utc_list->unl_head->un_data.un_sym, &tag))
    return UD_TREE_OK;

  switch (tag) {
    case UDOC_TAG_PARA:
      dfo_break_line (dfo);
      dfo_break_line (dfo);
      dfo_constrain (dfo, page_width, dfo->page_indent - 2);
      break;
    case UDOC_TAG_PARA_VERBATIM:
      dfo_break_line (dfo);
      dfo_break_line (dfo);
      dfo_tran_enable (dfo, DFO_TRAN_RESPACE);
      dfo_wrap_mode (dfo, DFO_WRAP_HYPH);
      break;
    default:
      break;
  }
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rp_file_finish (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct dfo_put *dfo = &render_ctx->uc_out->uoc_dfo;

  if (!rp_footnotes (ud, render_ctx)) return UD_TREE_FAIL;
  if (ud->ud_render_footer)
    if (!udr_print_file (ud, render_ctx, ud->ud_render_footer, plain_put, dfo))
      return UD_TREE_FAIL;

  ud_tryS (ud, dfo_flush (dfo) != -1, UD_TREE_FAIL, "dfo_flush",
    dfo_errorstr (dfo->error));

  return UD_TREE_OK;
}

const struct ud_renderer ud_render_plain = {
  {
    .urf_file_init = rp_file_init,
    .urf_symbol = rp_symbol,
    .urf_string = rp_string,
    .urf_list_end = rp_list_end,
    .urf_file_finish = rp_file_finish,
  },
  {
    .ur_name = "plain",
    .ur_suffix = "txt",
    .ur_desc = "plain text output",
    .ur_part = 0,
  },
};
