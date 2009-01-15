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

#define UDOC_IMPLEMENTATION
#include "ud_assert.h"
#include "udoc.h"
#include "ud_ref.h"
#include "log.h"
#include "multi.h"

static const char xhtml_header[] =
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML Basic 1.1//EN\"\n"
"\"http://www.w3.org/TR/xhtml-basic/xhtml-basic11.dtd\">\n";

/* character escapes */
static const char *
x_t_lessthan (struct dfo_put *dp) { return "&lt;"; }
static const char *
x_t_greaterthan (struct dfo_put *dp) { return "&gt;"; }
static const char *
x_t_amp (struct dfo_put *dp) { return "&amp;"; }
static const struct dfo_trans x_trans[] = {
  { '<', x_t_lessthan },
  { '>', x_t_greaterthan },
  { '&', x_t_amp },
};
static const unsigned int x_trans_size = sizeof (x_trans) / sizeof (x_trans[0]);

#if 0
static int
x_put (struct buffer *out, const char *str, unsigned long len, void *data)
{
  return dfo_put (data, str, len);
}
static int
x_puts (struct buffer *out, const char *str, unsigned long len, void *data)
{
  return dfo_puts (data, str);
}
#endif

/* misc xhtml state */

struct xhtml_ctx {
  struct dstring dstr;  
};

static int
x_escape_put (struct buffer *out, const char *str, unsigned long len, void *data)
{
  unsigned long pos;
  char ch;

  for (;;) {
    pos = scan_notcharsetn (str, "<>&", len);
    if (pos) buffer_put (out, str, pos);
    str += pos;
    len -= pos;
    if (!len) break;
    ch = *str;
    switch (ch) {
      case '<': buffer_put (out, "&lt;", 4); break;
      case '>': buffer_put (out, "&gt;", 4); break;
      case '&': buffer_put (out, "&amp;", 5); break;
      default: break;
    }
    ++str;
    --len;
    if (!len) break;
  }
  return 1;
}

static int
x_escape_puts (struct buffer *out, const char *str, void *data)
{
  return x_escape_put (out, str, str_len (str), data);
}

/* text */

static enum ud_tree_walk_stat
x_string (struct udoc *ud, struct udr_ctx *render_ctx)
{
  x_escape_puts (&render_ctx->uc_out->uoc_buffer,
    render_ctx->uc_tree_ctx->utc_state->utc_node->un_data.un_str, 0);
  return UD_TREE_OK;
}

/*
 * tag handling
 */

static int
x_tag_collect_css (struct dstring *buf, struct udr_ctx *render_ctx)
{
  const struct ud_node *n = render_ctx->uc_tree_ctx->utc_state->utc_list->unl_head;
  dstring_trunc (buf);
  for (;;) {
    if (n->un_next) n = n->un_next; else break;
    if (n->un_type == UDOC_TYPE_SYMBOL) {
      if (!dstring_cats (buf, n->un_data.un_sym)) return 0;
      if (!dstring_cats (buf, " ")) return 0;
    }
  }
  if (!buf->len) return 1;
  if (buf->s[buf->len - 1] == ' ') dstring_chop (buf, buf->len - 1);
  dstring_0 (buf);
  return 1;
}

enum {
  TAG_NEWLINE = 0x0001,
};

static enum ud_tree_walk_stat
x_tag_generic (struct udoc *ud, struct udr_ctx *render_ctx,
  const char *tag, const char *def_attrib, unsigned long flags)
{
  struct xhtml_ctx *xc = render_ctx->uc_user_data;
  struct buffer *out = &render_ctx->uc_out->uoc_buffer;

  if (!x_tag_collect_css (&xc->dstr, render_ctx)) return UD_TREE_FAIL;
  
  buffer_puts2 (out, "<", tag);
  if (def_attrib) {
    if (xc->dstr.len)
      if (!dstring_catb (&xc->dstr, " ", 1)) return UD_TREE_FAIL;
    if (!dstring_cats (&xc->dstr, def_attrib)) return UD_TREE_FAIL;
    dstring_0 (&xc->dstr);
  }
  if (xc->dstr.len)
    buffer_puts3 (out, " class=\"", xc->dstr.s, "\"");

  buffer_put (out, ">", 1);

  if (flags & TAG_NEWLINE) buffer_put (out, "\n", 1);
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
x_tag_end_generic (struct udoc *ud, struct udr_ctx *render_ctx,
  const char *tag, const char *def_attrib, unsigned long flags)
{
  buffer_puts3 (&render_ctx->uc_out->uoc_buffer, "</", tag, ">");
  if (flags & TAG_NEWLINE) buffer_put (&render_ctx->uc_out->uoc_buffer, "\n", 1);
  return UD_TREE_OK;
}

/*
 * tag callbacks
 */

static enum ud_tree_walk_stat
x_tag_para (struct udoc *ud, struct udr_ctx *render_ctx)
{
  return x_tag_generic (ud, render_ctx, "p", 0, TAG_NEWLINE);
}

static enum ud_tree_walk_stat
x_tag_para_verbatim (struct udoc *ud, struct udr_ctx *render_ctx)
{
  return x_tag_generic (ud, render_ctx, "pre", 0, TAG_NEWLINE);
}

static enum ud_tree_walk_stat
x_tag_section (struct udoc *ud, struct udr_ctx *render_ctx)
{
  enum ud_tree_walk_stat ret;
  struct buffer *out = &render_ctx->uc_out->uoc_buffer;
  struct ud_part *part;
  unsigned long index;

  log_1xf (LOG_DEBUG, "section open");

  buffer_put (out, "\n", 1);
  buffer_puts (out, "<!--section_open-->\n");

  ret = x_tag_generic (ud, render_ctx, "div", "ud_section", TAG_NEWLINE);
  if (ret == UD_TREE_OK)
    if (ud_part_getfromnode (ud, render_ctx->uc_tree_ctx->utc_state->utc_node,
      &part, &index)) {
      buffer_puts3 (out, "<a name=\"sect_", part->up_num_string, "\"></a>\n");
      buffer_puts5 (out, "<h3>", part->up_num_string, " ", part->up_title, "</h3>\n");
    }

  return ret;
}

static enum ud_tree_walk_stat
x_tag_item (struct udoc *ud, struct udr_ctx *render_ctx)
{
  return x_tag_generic (ud, render_ctx, "span", 0, 0);
}

static enum ud_tree_walk_stat
x_tag_ref (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct buffer *out = &render_ctx->uc_out->uoc_buffer;
  const struct ud_node *n = render_ctx->uc_tree_ctx->utc_state->utc_node;

  buffer_puts3 (out, "<a name=\"r_", n->un_next->un_data.un_str, "\"></a>\n");
  return 1;
}

static enum ud_tree_walk_stat
x_tag_link (struct udoc *ud, struct udr_ctx *render_ctx)
{
  char cnum[FMT_ULONG];
  struct buffer *out = &render_ctx->uc_out->uoc_buffer;
  const struct ud_node *cur_node = render_ctx->uc_tree_ctx->utc_state->utc_node;
  const char *ref_link;
  const char *ref_text;
  unsigned long dummy;
  unsigned long ref_len;
  const struct ud_ref *ref;
  const struct ud_part *part;

  ref_link = cur_node->un_next->un_data.un_str;
  ref_text = (cur_node->un_next->un_next) ?
    cur_node->un_next->un_next->un_data.un_str : ref_link;
  ref_len = str_len (ref_link);

  ud_assert (ud_oht_get (&ud->ud_ref_names, ref_link, ref_len, (void *) &ref, &dummy));
  ud_assert (part = ud_part_get (ud, ref->ur_part_index));

  /* only link to file if splitting */
  buffer_puts (out, "<a href=\"");
  if (ud->ud_opts.ud_split_thresh) {
    cnum[fmt_ulong (cnum, part->up_file)] = 0;
    buffer_puts3 (out, cnum, ".", render_ctx->uc_render->ur_data.ur_suffix);
  }

  buffer_puts5 (out, "#r_", ref_link, "\">", ref_text, "</a>");
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
x_tag_link_ext (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct buffer *out = &render_ctx->uc_out->uoc_buffer;
  const struct ud_node *n = render_ctx->uc_tree_ctx->utc_state->utc_node;
  const char *url;
  const char *text;

  url = n->un_next->un_data.un_str;
  text = (n->un_next->un_next) ? n->un_next->un_next->un_data.un_str : url;

  buffer_puts5 (out, "<a href=\"", url, "\">", text, "</a>");
  return 1;
}

static enum ud_tree_walk_stat
x_tag_table (struct udoc *ud, struct udr_ctx *render_ctx)
{
  const struct ud_node *n = render_ctx->uc_tree_ctx->utc_state->utc_node;
  struct udr_ctx rtmp = *render_ctx;

  if (x_tag_generic (ud, render_ctx, "table", 0, TAG_NEWLINE) == UD_TREE_FAIL)
    return UD_TREE_FAIL;

  /* for each item in list, render list (row) */
  for (;;) {
    if (n->un_type == UDOC_TYPE_LIST) {
      rtmp.uc_tree_ctx = 0;
      rtmp.uc_flag_finish_file = 0;
      if (!ud_render_node (ud, &rtmp, &n->un_data.un_list)) return UD_TREE_FAIL;
    }
    if (n->un_next) n = n->un_next; else break;
  }

  if (x_tag_end_generic (ud, render_ctx, "table", 0, TAG_NEWLINE) == UD_TREE_FAIL)
    return UD_TREE_FAIL;

  return UD_TREE_STOP_LIST;
}

static enum ud_tree_walk_stat
x_tag_table_row (struct udoc *ud, struct udr_ctx *render_ctx)
{
  const struct ud_node *n = render_ctx->uc_tree_ctx->utc_state->utc_node;
  struct udr_ctx rtmp = *render_ctx;

  if (x_tag_generic (ud, render_ctx, "tr", 0, TAG_NEWLINE) == UD_TREE_FAIL)
    return UD_TREE_FAIL;

  /* for each item in list, render list (cell) */
  for (;;) {
    if (n->un_type == UDOC_TYPE_LIST) {
      buffer_puts (&render_ctx->uc_out->uoc_buffer, "<td>");
      rtmp.uc_tree_ctx = 0;
      rtmp.uc_flag_finish_file = 0;
      if (!ud_render_node (ud, &rtmp, &n->un_data.un_list)) return UD_TREE_FAIL;
      buffer_puts (&render_ctx->uc_out->uoc_buffer, "</td>\n");
    }
    if (n->un_next) n = n->un_next; else break;
  }

  if (x_tag_end_generic (ud, render_ctx, "tr", 0, TAG_NEWLINE) == UD_TREE_FAIL)
    return UD_TREE_FAIL;

  return UD_TREE_STOP_LIST;
}

static enum ud_tree_walk_stat
x_tag_list (struct udoc *ud, struct udr_ctx *render_ctx)
{
  const struct ud_node *n = render_ctx->uc_tree_ctx->utc_state->utc_node;
  struct udr_ctx rtmp = *render_ctx;

  if (x_tag_generic (ud, render_ctx, "ul", 0, TAG_NEWLINE) == UD_TREE_FAIL)
    return UD_TREE_FAIL;

  /* for each item in list, render list */
  for (;;) {
    if (n->un_type == UDOC_TYPE_LIST) {
      buffer_puts (&render_ctx->uc_out->uoc_buffer, "<li>");
      rtmp.uc_tree_ctx = 0;
      rtmp.uc_flag_finish_file = 0;
      if (!ud_render_node (ud, &rtmp, &n->un_data.un_list)) return UD_TREE_FAIL;
      buffer_puts (&render_ctx->uc_out->uoc_buffer, "</li>\n");
    }
    if (n->un_next) n = n->un_next; else break;
  }

  if (x_tag_end_generic (ud, render_ctx, "ul", 0, TAG_NEWLINE) == UD_TREE_FAIL)
    return UD_TREE_FAIL;

  return UD_TREE_STOP_LIST;
}

static int
x_appears_in_contents (struct udoc *ud,
  const struct udr_ctx *render_ctx, unsigned long depth_first,
  unsigned long depth_cur)
{
  const unsigned long threshold = render_ctx->uc_opts->uo_toc_threshold;
  const unsigned long diff = depth_cur - depth_first;

  if (threshold)
    return (diff < threshold);
  else
    return 1;
}

static enum ud_tree_walk_stat
x_tag_contents (struct udoc *ud, struct udr_ctx *render_ctx)
{
  char cnum[FMT_ULONG];
  struct buffer *out = &render_ctx->uc_out->uoc_buffer;
  struct ud_part *part_cur = (struct ud_part *) render_ctx->uc_part;
  struct ud_part *part_first = part_cur;
  unsigned long index;
  unsigned long n;

  buffer_puts (out, "\n<div class=\"ud_toc\">\n");

  index = part_cur->up_index_cur;
  for (;;) {
    ud_assert (ud_oht_get_index (&ud->ud_parts, index, (void *) &part_cur));
    if (part_cur->up_depth <= part_first->up_depth)
      if (part_cur != part_first) break;

    /* only place entry in toc if below threshold */
    if (x_appears_in_contents (ud, render_ctx,
      part_first->up_depth, part_cur->up_depth)) {

      /* indent */
      for (n = 0; n < part_cur->up_depth - part_first->up_depth; ++n)
        buffer_puts (out, "&nbsp; &nbsp; ");

      /* only link to file if splitting */
      buffer_puts (out, "<a href=\"");
      if (ud->ud_opts.ud_split_thresh) {
        buffer_put (out, cnum, fmt_ulong (cnum, part_cur->up_file));
        buffer_puts2 (out, ".", render_ctx->uc_render->ur_data.ur_suffix);
      }
      buffer_puts2 (out, "#sect_", part_cur->up_num_string);
      buffer_puts5 (out, "\">", part_cur->up_num_string, ". ",
        part_cur->up_title, "</a><br/>\n");
    }

    if (!part_cur->up_index_next) break;
    index = part_cur->up_index_next;
  }
  buffer_puts (out, "</div>\n\n");

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
x_tag_footnote (struct udoc *ud, struct udr_ctx *render_ctx)
{
  char cnum[FMT_ULONG];
  unsigned long max;
  unsigned long index;
  struct ud_ref *ref;
  struct buffer *out = &render_ctx->uc_out->uoc_buffer;
  const struct ud_node_list *list = render_ctx->uc_tree_ctx->utc_state->utc_list;

  max = ud_oht_size (&ud->ud_footnotes);
  for (index = 0; index < max; ++index) {
    ud_assert (ud_oht_get_index (&ud->ud_footnotes, index, (void *) &ref));
    if (list == ref->ur_list) {
      cnum[fmt_ulong (cnum, index)] = 0;
      buffer_puts7 (out, "[<a href=\"#fn_", cnum, "\" name=\"fnr_", cnum, "\">",
        cnum, "</a>]");
      break;
    }
  }

  return UD_TREE_STOP_LIST;
}

static enum ud_tree_walk_stat
x_tag_date (struct udoc *ud, struct udr_ctx *render_ctx)
{
  char buf[CALTIME_FMT];
  struct caltime ct;
  struct buffer *out = &render_ctx->uc_out->uoc_buffer;

  caltime_local (&ct, &ud->ud_time_start.sec, 0, 0);
  buffer_put (out, buf, caltime_fmt (buf, &ct));
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
x_tag_render (struct udoc *ud, struct udr_ctx *render_ctx)
{
  if (!udr_print_file (ud, render_ctx,
    render_ctx->uc_tree_ctx->utc_state->utc_node->un_next->un_data.un_str,
    x_escape_put, 0))
    return UD_TREE_FAIL;

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
x_tag_render_noescape (struct udoc *ud, struct udr_ctx *render_ctx)
{
  if (!udr_print_file (ud, render_ctx,
    render_ctx->uc_tree_ctx->utc_state->utc_node->un_next->un_data.un_str, 0, 0))
    return UD_TREE_FAIL;

  return UD_TREE_OK;
}

/* tag ends */

static enum ud_tree_walk_stat
x_tag_end_para (struct udoc *ud, struct udr_ctx *render_ctx)
{
  return x_tag_end_generic (ud, render_ctx, "p", 0, TAG_NEWLINE);
}

static enum ud_tree_walk_stat
x_tag_end_para_verbatim (struct udoc *ud, struct udr_ctx *render_ctx)
{
  return x_tag_end_generic (ud, render_ctx, "pre", 0, TAG_NEWLINE);
}

static enum ud_tree_walk_stat
x_tag_end_section (struct udoc *ud, struct udr_ctx *render_ctx)
{
  log_1xf (LOG_DEBUG, "section close");
  buffer_puts (&render_ctx->uc_out->uoc_buffer, "\n<!--section_close-->\n");
  return x_tag_end_generic (ud, render_ctx, "div", 0, TAG_NEWLINE);
}

static enum ud_tree_walk_stat
x_tag_end_item (struct udoc *ud, struct udr_ctx *render_ctx)
{
  return x_tag_end_generic (ud, render_ctx, "span", 0, 0);
}

/* tables */

struct dispatch {
  enum ud_tag tag;
  enum ud_tree_walk_stat (*func) (struct udoc *, struct udr_ctx *);
};

static const struct dispatch tag_starts[] = {
  { UDOC_TAG_PARA,            x_tag_para },
  { UDOC_TAG_PARA_VERBATIM,   x_tag_para_verbatim },
  { UDOC_TAG_SECTION,         x_tag_section },
  { UDOC_TAG_SUBSECTION,      x_tag_section },
  { UDOC_TAG_ITEM,            x_tag_item },
  { UDOC_TAG_REF,             x_tag_ref },
  { UDOC_TAG_LINK,            x_tag_link },
  { UDOC_TAG_LINK_EXT,        x_tag_link_ext },
  { UDOC_TAG_TABLE,           x_tag_table },
  { UDOC_TAG_CONTENTS,        x_tag_contents },
  { UDOC_TAG_FOOTNOTE,        x_tag_footnote },
  { UDOC_TAG_DATE,            x_tag_date },
  { UDOC_TAG_RENDER,          x_tag_render },
  { UDOC_TAG_RENDER_NOESCAPE, x_tag_render_noescape },
  { UDOC_TAG_TABLE,           x_tag_table },
  { UDOC_TAG_TABLE_ROW,       x_tag_table_row },
  { UDOC_TAG_LIST,            x_tag_list },
};
static const unsigned int tag_starts_size = sizeof (tag_starts)
                                          / sizeof (tag_starts[0]);
static const struct dispatch tag_ends[] = {
  { UDOC_TAG_PARA,          x_tag_end_para },
  { UDOC_TAG_PARA_VERBATIM, x_tag_end_para_verbatim },
  { UDOC_TAG_SECTION,       x_tag_end_section },
  { UDOC_TAG_SUBSECTION,    x_tag_end_section },
  { UDOC_TAG_ITEM,          x_tag_end_item },
};
static const unsigned int tag_ends_size = sizeof (tag_ends)
                                        / sizeof (tag_ends[0]);

static enum ud_tree_walk_stat
dispatch (const struct dispatch *tab, unsigned int tab_size,
  struct udoc *ud, struct udr_ctx *ctx, enum ud_tag tag)
{
  unsigned int index;
  for (index = 0; index < tab_size; ++index)
    if (tag == tab[index].tag) return tab[index].func (ud, ctx);
  return UD_TREE_OK;
}

static void
x_html_xml (struct buffer *out, const char *encoding)
{
  if (!encoding) encoding = "utf-8";

  buffer_puts (out, "<?xml version=\"1.0\" encoding=\"");
  buffer_puts (out, encoding);
  buffer_puts (out, "\"?>\n");
}

static void
x_html_doctype (struct buffer *out)
{
  buffer_puts (out, xhtml_header);
}

static void
x_navbar (struct udoc *ud, struct buffer *out, const struct ud_part *part_cur,
  const char *class)
{
  char cnum[FMT_ULONG];
  struct ud_part *part_prev = 0;
  struct ud_part *part_next = 0;
  struct ud_part *part_parent = 0;

  buffer_puts (out, "\n");
  buffer_puts (out, "<!--navbar-->\n");
  buffer_puts (out, "<div class=\"ud_navbar ");
  if (class)
    buffer_puts (out, class);
  buffer_puts (out, "\">\n");
  buffer_puts (out, "<a href=\"0.html\">top</a>\n");

  ud_oht_get_index (&ud->ud_parts, part_cur->up_index_parent, (void *) &part_parent);
  ud_part_getnext_file (ud, part_cur, &part_next);
  ud_part_getprev_file (ud, part_cur, &part_prev);

  if (part_prev) {
    buffer_puts (out, "<a href=\"");
    buffer_put (out, cnum, fmt_ulong (cnum, part_prev->up_file));
    buffer_puts (out, ".html\">prev</a>\n");
  }
  if (part_parent) {
    buffer_puts (out, "<a href=\"");
    buffer_put (out, cnum, fmt_ulong (cnum, part_parent->up_file));
    buffer_puts (out, ".html\">up</a>\n");
  }
  if (part_next) {
    buffer_puts (out, "<a href=\"");
    buffer_put (out, cnum, fmt_ulong (cnum, part_next->up_file));
    buffer_puts (out, ".html\">next</a>\n");
  }
  buffer_puts (out, "</div>\n\n");
}

static int 
x_footnotes (struct udoc *ud, struct udr_ctx *render_ctx)
{
  char cnum[FMT_ULONG];
  const unsigned long max = ud_oht_size (&ud->ud_footnotes);
  unsigned long index;
  unsigned long num_notes = 0;
  const struct ud_ref *note;
  const struct ud_part *part;
  struct buffer *out = &render_ctx->uc_out->uoc_buffer;
  struct udr_ctx rtmp = *render_ctx;

  /* count footnotes for this file */
  for (index = 0; index < max; ++index) {
    ud_assert (ud_oht_get_index (&ud->ud_footnotes, index, (void *) &note));
    ud_assert (part = ud_part_get (ud, note->ur_part_index));
    if (part->up_file == render_ctx->uc_part->up_file) ++num_notes;
  }

  /* print footnotes, if any */
  if (num_notes) {
    buffer_puts (out, "\n<div class=\"ud_footnotes\">\n");
    buffer_puts (out, "<table class=\"ud_footnote_tab\">\n");
    for (index = 0; index < max; ++index) {
      ud_assert (ud_oht_get_index (&ud->ud_footnotes, index, (void *) &note));
      ud_assert (part = ud_part_get (ud, note->ur_part_index));

      /* render if footnote belongs to this file */
      if (part->up_file == render_ctx->uc_part->up_file) {
        buffer_puts (out, "<tr>\n");
        cnum[fmt_ulong (cnum, index)] = 0;
        buffer_puts (out, "<td>");
        buffer_puts7 (out, "[<a name=\"fn_", cnum, "\" href=\"#fnr_", cnum, "\">", cnum, "</a>]");
        buffer_puts (out, "</td>\n");
        buffer_puts (out, "<td>\n");

        /* render footnote content */
        rtmp.uc_tree_ctx = 0;
        rtmp.uc_flag_finish_file = 0;
        if (!ud_render_node (ud, &rtmp,
          &note->ur_list->unl_head->un_next->un_data.un_list)) return 0;

        buffer_puts (out, "</td>\n");
        buffer_puts (out, "</tr>\n");
      }
    }
    buffer_puts (out, "</table>\n");
    buffer_puts (out, "</div>\n\n");
  }

  return 1;
}

/*
 * top level callbacks
 */

static enum ud_tree_walk_stat 
xhtm_init_once (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct xhtml_ctx *xc;

  ud_assert (render_ctx->uc_user_data == 0);
  render_ctx->uc_user_data = alloc_zero (sizeof (struct xhtml_ctx));
  if (!render_ctx->uc_user_data) return UD_TREE_FAIL;
  xc = render_ctx->uc_user_data;
  if (!dstring_init (&xc->dstr, 128)) {
    dealloc_null (&render_ctx->uc_user_data);
    return UD_TREE_FAIL;
  }
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat 
xhtm_file_init (struct udoc *ud, struct udr_ctx *render_ctx)
{
  unsigned long max;
  unsigned long index;
  struct ud_ref *ref;
  const struct ud_part *part = render_ctx->uc_part;
  const struct ud_part *ppart = part;
  struct buffer *out = &render_ctx->uc_out->uoc_buffer;
  const char *title = 0;

  x_html_xml (out, ud->ud_encoding);
  x_html_doctype (out);

  buffer_puts (out, "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n");
  buffer_puts (out, "<head>\n");

  /* print style data */
  max = ud_oht_size (&ud->ud_styles);
  for (index = 0; index < max; ++index) {
    ud_assert (ud_oht_get_index (&ud->ud_styles, index, (void *) &ref));
    buffer_puts (out, "<link rel=\"stylesheet\" type=\"text/css\" href=\"");
    buffer_puts (out, ref->ur_node->un_next->un_data.un_str);
    buffer_puts (out, ".css\"/>\n");
  }

  /* get title */
  for (;;) {
    title = ppart->up_title;
    if (!title) {
      if (ppart->up_index_parent == ppart->up_index_cur) break;
      ud_oht_get_index (&ud->ud_parts, ppart->up_index_parent, (void *) &ppart);
    } else {
      buffer_puts (out, "<title>");
      x_escape_puts (out, title, 0);
      buffer_puts (out, "</title>\n");
      break;
    }
  }

  buffer_puts (out, "</head>\n<body>\n");

  /* render backend-specific header */
  if (ud->ud_render_header)
    if (!udr_print_file (ud, render_ctx, ud->ud_render_header, 0, 0))
      return UD_TREE_FAIL;

  if (ud->ud_opts.ud_split_thresh)
    x_navbar (ud, out, part, "ud_navbar_head");

  buffer_puts (out, "<!--file_init-->\n");
  buffer_puts (out, "<div>\n");

  /* only print large title on first page */
  if (title && part->up_index_cur == 0)
    buffer_puts3 (out, "<h2>", title, "</h2>\n");

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
xhtm_list (struct udoc *ud, struct udr_ctx *render_ctx)
{
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
xhtm_symbol (struct udoc *ud, struct udr_ctx *render_ctx)
{
  const char *sym = render_ctx->uc_tree_ctx->utc_state->utc_list->unl_head->un_data.un_sym;
  enum ud_tag tag;

  if (render_ctx->uc_tree_ctx->utc_state->utc_list_pos == 0)
    if (ud_tag_by_name (sym, &tag))
      return dispatch (tag_starts, tag_starts_size, ud, render_ctx, tag);

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
xhtm_string (struct udoc *ud, struct udr_ctx *render_ctx)
{
  const char *sym = render_ctx->uc_tree_ctx->utc_state->utc_list->unl_head->un_data.un_sym;
  enum ud_tag tag;

  if (ud_tag_by_name (sym, &tag)) {
    switch (tag) {
      case UDOC_TAG_LINK:
      case UDOC_TAG_LINK_EXT:
      case UDOC_TAG_RENDER_HEADER:
      case UDOC_TAG_RENDER_FOOTER:
      case UDOC_TAG_RENDER:
      case UDOC_TAG_RENDER_NOESCAPE:
      case UDOC_TAG_STYLE:
      case UDOC_TAG_REF:
      case UDOC_TAG_TITLE:
      case UDOC_TAG_ENCODING:
      case UDOC_TAG_TABLE:
      case UDOC_TAG_TABLE_ROW:
      case UDOC_TAG_LIST:
        return UD_TREE_OK;
      default:
        break;
    }
  }
  if (!x_string (ud, render_ctx)) return UD_TREE_FAIL;
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
xhtm_list_end (struct udoc *ud, struct udr_ctx *render_ctx)
{
  const char *sym = render_ctx->uc_tree_ctx->utc_state->utc_list->unl_head->un_data.un_sym;
  enum ud_tag tag;

  if (ud_tag_by_name (sym, &tag))
    return dispatch (tag_ends, tag_ends_size, ud, render_ctx, tag);

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
xhtm_file_finish (struct udoc *ud, struct udr_ctx *render_ctx)
{
  const struct ud_part *part = render_ctx->uc_part;
  struct buffer *out = &render_ctx->uc_out->uoc_buffer;

  x_footnotes (ud, render_ctx);

  buffer_puts (out, "\n<!--file_finish-->\n");
  buffer_puts (out, "</div>\n");

  if (ud->ud_opts.ud_split_thresh)
    x_navbar (ud, out, part, "ud_navbar_foot");

  /* render backend-specific footer */
  if (ud->ud_render_footer)
    if (!udr_print_file (ud, render_ctx, ud->ud_render_footer, 0, 0))
      return UD_TREE_FAIL;

  buffer_puts (out, "</body>\n</html>\n");

  ud_try_sys (ud, buffer_flush (out) != -1, UD_TREE_FAIL, "write");
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
xhtm_finish_once (struct udoc *ud, struct udr_ctx *render_ctx)
{
  struct xhtml_ctx *xc = render_ctx->uc_user_data;
  dstring_free (&xc->dstr);
  dealloc_null (&render_ctx->uc_user_data);
  return UD_TREE_OK;
}
 
const struct ud_renderer ud_render_xhtml = {
  {
    .urf_init_once = xhtm_init_once,
    .urf_file_init = xhtm_file_init,
    .urf_list = xhtm_list,
    .urf_symbol = xhtm_symbol,
    .urf_string = xhtm_string,
    .urf_list_end = xhtm_list_end,
    .urf_file_finish = xhtm_file_finish,
    .urf_finish_once = xhtm_finish_once,
  },
  {
    .ur_name = "xhtml",
    .ur_suffix = "html",
    .ur_desc = "xhtml output",
    .ur_part = 1,
  },
};
