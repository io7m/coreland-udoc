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
x_t_lessthan(struct dfo_put *dp) { return "&lt;"; }
static const char *
x_t_greaterthan(struct dfo_put *dp) { return "&gt;"; }
static const char *
x_t_amp(struct dfo_put *dp) { return "&amp;"; }
static const struct dfo_trans x_trans[] = {
  { '<', x_t_lessthan },
  { '>', x_t_greaterthan },
  { '&', x_t_amp },
};
static const unsigned int x_trans_size = sizeof(x_trans) / sizeof(x_trans[0]);

#if 0
static int
x_put(struct buffer *out, const char *str, unsigned long len, void *data)
{
  return dfo_put(data, str, len);
}
static int
x_puts(struct buffer *out, const char *str, unsigned long len, void *data)
{
  return dfo_puts(data, str);
}
#endif

/* misc xhtml state */

struct xhtml_ctx {
  struct dstring dstr;  
};

static int
x_escape_put(struct buffer *out, const char *str, unsigned long len, void *data)
{
  unsigned long pos;
  char ch;

  for (;;) {
    pos = scan_notcharsetn(str, "<>&", len);
    if (pos) buffer_put(out, str, pos);
    str += pos;
    len -= pos;
    if (!len) break;
    ch = *str;
    switch (ch) {
      case '<': buffer_put(out, "&lt;", 4); break;
      case '>': buffer_put(out, "&gt;", 4); break;
      case '&': buffer_put(out, "&amp;", 5); break;
      default: break;
    }
    ++str;
    --len;
    if (!len) break;
  }
  return 1;
}

static int
x_escape_puts(struct buffer *out, const char *str, void *data)
{
  return x_escape_put(out, str, str_len(str), data);
}

/* text */

static enum ud_tree_walk_stat
x_string(struct udoc *ud, struct udr_ctx *r)
{
  x_escape_puts(&r->uc_out->uoc_buf, r->uc_tree_ctx->utc_state->utc_node->un_data.un_str, 0);
  return UD_TREE_OK;
}

/*
 * tag handling
 */

static int
x_tag_collect_css(struct dstring *buf, struct udr_ctx *rc)
{
  const struct ud_node *n = rc->uc_tree_ctx->utc_state->utc_list->unl_head;
  dstring_trunc(buf);
  for (;;) {
    if (n->un_next) n = n->un_next; else break;
    if (n->un_type == UDOC_TYPE_SYMBOL) {
      if (!dstring_cats(buf, n->un_data.un_sym)) return 0;
      if (!dstring_cats(buf, " ")) return 0;
    }
  }
  if (!buf->len) return 1;
  if (buf->s[buf->len - 1] == ' ') dstring_chop(buf, buf->len - 1);
  dstring_0(buf);
  return 1;
}

static enum ud_tree_walk_stat
x_tag_generic(struct udoc *ud, struct udr_ctx *r,
              const char *tag, const char *def_attrib)
{
  struct xhtml_ctx *xc = r->uc_user_data;
  struct buffer *out = &r->uc_out->uoc_buf;

  if (!x_tag_collect_css(&xc->dstr, r)) return UD_TREE_FAIL;
  
  buffer_puts2(out, "<", tag);
  if (def_attrib) {
    if (xc->dstr.len)
      if (!dstring_catb(&xc->dstr, " ", 1)) return UD_TREE_FAIL;
    if (!dstring_cats(&xc->dstr, def_attrib)) return UD_TREE_FAIL;
    dstring_0(&xc->dstr);
  }
  if (xc->dstr.len)
    buffer_puts3(out, " class=\"", xc->dstr.s, "\"");

  buffer_puts(out, ">\n");
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
x_tag_end_generic(struct udoc *ud, struct udr_ctx *r,
              const char *tag, const char *def_attrib)
{
  buffer_puts3(&r->uc_out->uoc_buf, "</", tag, ">\n");
  return UD_TREE_OK;
}

/*
 * tag callbacks
 */

static enum ud_tree_walk_stat
x_tag_para(struct udoc *ud, struct udr_ctx *r)
{
  return x_tag_generic(ud, r, "p", 0);
}

static enum ud_tree_walk_stat
x_tag_para_verbatim(struct udoc *ud, struct udr_ctx *r)
{
  return x_tag_generic(ud, r, "pre", 0);
}

static enum ud_tree_walk_stat
x_tag_section(struct udoc *ud, struct udr_ctx *r)
{
  enum ud_tree_walk_stat ret;
  struct buffer *out = &r->uc_out->uoc_buf;
  struct ud_part *part;
  unsigned long ind;

  log_1xf(LOG_DEBUG, "section open");

  buffer_put(out, "\n", 1);
  buffer_puts(out, "<!--section_open-->\n");

  ret = x_tag_generic(ud, r, "div", "ud_section");
  if (ret == UD_TREE_OK)
    if (ud_part_getfromnode(ud, r->uc_tree_ctx->utc_state->utc_node, &part, &ind)) {
      buffer_puts5(out, "<h3>", part->up_num_string, " ", part->up_title, "</h3>\n");
      buffer_puts3(out, "<a name=\"sect_", part->up_num_string, "\"></a>\n");
    }

  return ret;
}

static enum ud_tree_walk_stat
x_tag_item(struct udoc *ud, struct udr_ctx *r)
{
  return x_tag_generic(ud, r, "span", 0);
}

static enum ud_tree_walk_stat
x_tag_ref(struct udoc *ud, struct udr_ctx *r)
{
  struct buffer *out = &r->uc_out->uoc_buf;
  const struct ud_node *n = r->uc_tree_ctx->utc_state->utc_node;

  buffer_puts3(out, "<a name=\"r_", n->un_next->un_data.un_str, "\"></a>\n");
  return 1;
}

static enum ud_tree_walk_stat
x_tag_link(struct udoc *ud, struct udr_ctx *r)
{
  char cnum[FMT_ULONG];
  struct buffer *out = &r->uc_out->uoc_buf;
  const struct ud_node *n = r->uc_tree_ctx->utc_state->utc_node;
  const char *refl;
  const char *text;
  unsigned long dummy;
  struct ud_ref *ref;

  refl = n->un_next->un_data.un_str;
  text = (n->un_next->un_next) ? n->un_next->un_next->un_data.un_str : refl;

  ud_tryS(ud, ud_oht_get(&ud->ud_ref_names, refl, str_len(refl), (void *) &ref, &dummy),
          UD_TREE_FAIL, "ud_oht_get", "could not get reference for link");

  /* only link to file if splitting */
  buffer_puts(out, "<a href=\"");
  if (ud->ud_opts.ud_split_thresh) {
    cnum[fmt_ulong(cnum, ref->ur_part->up_file)] = 0;
    buffer_puts3(out, cnum, ".", r->uc_render->ur_data.ur_suffix);
  }

  buffer_puts5(out, "#r_", refl, "\">", text, "</a>");
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
x_tag_link_ext(struct udoc *ud, struct udr_ctx *r)
{
  struct buffer *out = &r->uc_out->uoc_buf;
  const struct ud_node *n = r->uc_tree_ctx->utc_state->utc_node;
  const char *url;
  const char *text;

  url = n->un_next->un_data.un_str;
  text = (n->un_next->un_next) ? n->un_next->un_next->un_data.un_str : url;

  buffer_puts5(out, "<a href=\"", url, "\">", text, "</a>");
  return 1;
}

static enum ud_tree_walk_stat
x_tag_table(struct udoc *ud, struct udr_ctx *rc)
{
  const struct ud_node *n = rc->uc_tree_ctx->utc_state->utc_node;
  struct udr_ctx rtmp = *rc;

  if (x_tag_generic(ud, rc, "table", 0) == UD_TREE_FAIL) return UD_TREE_FAIL;

  /* for each item in list, render list (row) */
  for (;;) {
    if (n->un_type == UDOC_TYPE_LIST) {
      rtmp.uc_tree_ctx = 0;
      if (!ud_render_node(ud, &rtmp, &n->un_data.un_list)) return UD_TREE_FAIL;
    }
    if (n->un_next) n = n->un_next; else break;
  }

  if (x_tag_end_generic(ud, rc, "table", 0) == UD_TREE_FAIL) return UD_TREE_FAIL;
  return UD_TREE_STOP_LIST;
}

static enum ud_tree_walk_stat
x_tag_table_row(struct udoc *ud, struct udr_ctx *rc)
{
  const struct ud_node *n = rc->uc_tree_ctx->utc_state->utc_node;
  struct udr_ctx rtmp = *rc;

  if (x_tag_generic(ud, rc, "tr", 0) == UD_TREE_FAIL) return UD_TREE_FAIL;

  /* for each item in list, render list (cell) */
  for (;;) {
    if (n->un_type == UDOC_TYPE_LIST) {
      buffer_puts(&rc->uc_out->uoc_buf, "<td>");
      rtmp.uc_tree_ctx = 0;
      if (!ud_render_node(ud, &rtmp, &n->un_data.un_list)) return UD_TREE_FAIL;
      buffer_puts(&rc->uc_out->uoc_buf, "</td>\n");
    }
    if (n->un_next)
      n = n->un_next;
    else
      break;
  }

  if (x_tag_end_generic(ud, rc, "tr", 0) == UD_TREE_FAIL) return UD_TREE_FAIL;
  return UD_TREE_STOP_LIST;
}

static enum ud_tree_walk_stat
x_tag_list(struct udoc *ud, struct udr_ctx *rc)
{
  const struct ud_node *n = rc->uc_tree_ctx->utc_state->utc_node;
  struct udr_ctx rtmp = *rc;

  if (x_tag_generic(ud, rc, "ul", 0) == UD_TREE_FAIL) return UD_TREE_FAIL;

  /* for each item in list, render list */
  for (;;) {
    if (n->un_type == UDOC_TYPE_LIST) {
      buffer_puts(&rc->uc_out->uoc_buf, "<li>");
      rtmp.uc_tree_ctx = 0;
      if (!ud_render_node(ud, &rtmp, &n->un_data.un_list)) return UD_TREE_FAIL;
      buffer_puts(&rc->uc_out->uoc_buf, "</li>\n");
    }
    if (n->un_next) n = n->un_next; else break;
  }

  if (x_tag_end_generic(ud, rc, "ul", 0) == UD_TREE_FAIL) return UD_TREE_FAIL;
  return UD_TREE_STOP_LIST;
}

static enum ud_tree_walk_stat
x_tag_contents(struct udoc *ud, struct udr_ctx *r)
{
  char cnum[FMT_ULONG];
  struct buffer *out = &r->uc_out->uoc_buf;
  struct ud_part *part_cur = (struct ud_part *) r->uc_part;
  struct ud_part *part_first = 0; /* first part in current file */
  unsigned long max;
  unsigned long ind;
  unsigned long n;

  ud_part_getfirst_wdepth_noskip(ud, part_cur, &part_first);
  max = ud_oht_size(&ud->ud_parts);

  buffer_puts(out, "\n<div class=\"ud_toc\">\n");

  ind = part_cur->up_index_cur;
  for (;;) {
    ud_assert(ud_oht_getind(&ud->ud_parts, ind, (void *) &part_cur));
    if (part_cur->up_depth <= part_first->up_depth)
      if (part_cur != part_first) break;
    for (n = 0; n < part_cur->up_depth - part_first->up_depth; ++n)
      buffer_puts(out, "&nbsp; &nbsp; ");

    /* only link to file if splitting */
    buffer_puts(out, "<a href=\"");
    if (ud->ud_opts.ud_split_thresh) {
      buffer_put(out, cnum, fmt_ulong(cnum, part_cur->up_file));
      buffer_puts2(out, ".", r->uc_render->ur_data.ur_suffix);
    }
    buffer_puts2(out, "#sect_", part_cur->up_num_string);
    buffer_puts5(out, "\">", part_cur->up_num_string, ". ", part_cur->up_title, "</a><br/>\n");
    if (!part_cur->up_index_next) break;
    ind = part_cur->up_index_next;
  }
  buffer_puts(out, "</div>\n\n");

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
x_tag_footnote(struct udoc *ud, struct udr_ctx *r)
{
  char cnum[FMT_ULONG];
  unsigned long max;
  unsigned long ind;
  struct ud_ref *ref;
  struct buffer *out = &r->uc_out->uoc_buf;
  const struct ud_node_list *list = r->uc_tree_ctx->utc_state->utc_list;

  max = ud_oht_size(&ud->ud_footnotes);
  for (ind = 0; ind < max; ++ind) {
    ud_assert(ud_oht_getind(&ud->ud_footnotes, ind, (void *) &ref));
    if (list == ref->ur_list) {
      cnum[fmt_ulong(cnum, ind)] = 0;
      buffer_puts7(out, "[<a href=\"#fn_", cnum, "\" name=\"fnr_", cnum, "\">", cnum, "</a>]");
      break;
    }
  }

  return UD_TREE_STOP_LIST;
}

static enum ud_tree_walk_stat
x_tag_date(struct udoc *ud, struct udr_ctx *r)
{
  char buf[CALTIME_FMT];
  struct caltime ct;
  struct buffer *out = &r->uc_out->uoc_buf;

  caltime_local(&ct, &ud->ud_time_start.sec, 0, 0);
  buffer_put(out, buf, caltime_fmt(buf, &ct));
  buffer_put(out, "\n", 1);
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
x_tag_render(struct udoc *ud, struct udr_ctx *r)
{
  if (!udr_print_file(ud, r, r->uc_tree_ctx->utc_state->utc_node->un_next->un_data.un_str, x_escape_put, 0))
    return UD_TREE_FAIL;

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
x_tag_render_noescape(struct udoc *ud, struct udr_ctx *r)
{
  if (!udr_print_file(ud, r, r->uc_tree_ctx->utc_state->utc_node->un_next->un_data.un_str, 0, 0))
    return UD_TREE_FAIL;

  return UD_TREE_OK;
}

/* tag ends */

static enum ud_tree_walk_stat
x_tag_end_para(struct udoc *ud, struct udr_ctx *r)
{
  return x_tag_end_generic(ud, r, "p", 0);
}

static enum ud_tree_walk_stat
x_tag_end_para_verbatim(struct udoc *ud, struct udr_ctx *r)
{
  return x_tag_end_generic(ud, r, "pre", 0);
}

static enum ud_tree_walk_stat
x_tag_end_section(struct udoc *ud, struct udr_ctx *r)
{
  log_1xf(LOG_DEBUG, "section close");
  buffer_puts(&r->uc_out->uoc_buf, "\n<!--section_close-->\n");
  return x_tag_end_generic(ud, r, "div", 0);
}

static enum ud_tree_walk_stat
x_tag_end_item(struct udoc *ud, struct udr_ctx *r)
{
  return x_tag_end_generic(ud, r, "span", 0);
}

/* tables */

struct dispatch {
  enum ud_tag tag;
  enum ud_tree_walk_stat (*func)(struct udoc *, struct udr_ctx *);
};

static const struct dispatch tag_starts[] = {
  { UDOC_TAG_PARA,            x_tag_para },
  { UDOC_TAG_PARA_VERBATIM,   x_tag_para_verbatim },
  { UDOC_TAG_SECTION,         x_tag_section },
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
static const unsigned int tag_starts_size = sizeof(tag_starts)
                                          / sizeof(tag_starts[0]);
static const struct dispatch tag_ends[] = {
  { UDOC_TAG_PARA,          x_tag_end_para },
  { UDOC_TAG_PARA_VERBATIM, x_tag_end_para_verbatim },
  { UDOC_TAG_SECTION,       x_tag_end_section },
  { UDOC_TAG_ITEM,          x_tag_end_item },
};
static const unsigned int tag_ends_size = sizeof(tag_ends)
                                        / sizeof(tag_ends[0]);

static enum ud_tree_walk_stat
dispatch(const struct dispatch *tab, unsigned int tab_size,
  struct udoc *ud, struct udr_ctx *ctx, enum ud_tag tag)
{
  unsigned int ind;
  for (ind = 0; ind < tab_size; ++ind)
    if (tag == tab[ind].tag) return tab[ind].func(ud, ctx);
  return UD_TREE_OK;
}

static void
x_html_xml(struct buffer *out, const char *encoding)
{
  if (!encoding) encoding = "utf-8";

  buffer_puts(out, "<?xml version=\"1.0\" encoding=\"");
  buffer_puts(out, encoding);
  buffer_puts(out, "\"?>\n");
}

static void
x_html_doctype(struct buffer *out)
{
  buffer_puts(out, xhtml_header);
}

static void
x_navbar(struct udoc *ud, struct buffer *out, const struct ud_part *part_cur,
  const char *class)
{
  char cnum[FMT_ULONG];
  struct ud_part *part_prev = 0;
  struct ud_part *part_next = 0;
  struct ud_part *part_parent = 0;

  buffer_puts(out, "\n");
  buffer_puts(out, "<!--navbar-->\n");
  buffer_puts(out, "<div class=\"ud_navbar ");
  if (class)
    buffer_puts(out, class);
  buffer_puts(out, "\">\n");
  buffer_puts(out, "<a href=\"0.html\">top</a>\n");

  ud_oht_getind(&ud->ud_parts, part_cur->up_index_parent, (void *) &part_parent);
  ud_part_getnext_file(ud, part_cur, &part_next);
  ud_part_getprev_file(ud, part_cur, &part_prev);

  if (part_prev) {
    buffer_puts(out, "<a href=\"");
    buffer_put(out, cnum, fmt_ulong(cnum, part_prev->up_file));
    buffer_puts(out, ".html\">prev</a>\n");
  }
  if (part_parent) {
    buffer_puts(out, "<a href=\"");
    buffer_put(out, cnum, fmt_ulong(cnum, part_parent->up_file));
    buffer_puts(out, ".html\">up</a>\n");
  }
  if (part_next) {
    buffer_puts(out, "<a href=\"");
    buffer_put(out, cnum, fmt_ulong(cnum, part_next->up_file));
    buffer_puts(out, ".html\">next</a>\n");
  }
  buffer_puts(out, "</div>\n\n");
}

static int 
x_footnotes(struct udoc *ud, struct udr_ctx *rc)
{
  char cnum[FMT_ULONG];
  unsigned long ind;
  unsigned long max;
  unsigned long nfp = 0;
  struct ud_ref *note;
  struct buffer *out = &rc->uc_out->uoc_buf;
  struct udr_ctx rtmp = *rc;

  /* count footnotes for this file */
  max = ud_oht_size(&ud->ud_footnotes);
  for (ind = 0; ind < max; ++ind) {
    ud_assert(ud_oht_getind(&ud->ud_footnotes, ind, (void *) &note));
    if (note->ur_part->up_file == rc->uc_part->up_file) ++nfp;
  }

  /* print footnotes, if any */
  if (nfp) {
    buffer_puts(out, "\n<div class=\"ud_footnotes\">\n");
    buffer_puts(out, "<table class=\"ud_footnote_tab\">\n");
    for (ind = 0; ind < max; ++ind) {
      ud_assert(ud_oht_getind(&ud->ud_footnotes, ind, (void *) &note));
      if (note->ur_part->up_file == rc->uc_part->up_file) {
        buffer_puts(out, "<tr>\n");
        cnum[fmt_ulong(cnum, ind)] = 0;
        buffer_puts(out, "<td>");
        buffer_puts7(out, "[<a name=\"fn_", cnum, "\" href=\"#fnr_", cnum, "\">", cnum, "</a>]");
        buffer_puts(out, "</td>\n");
        buffer_puts(out, "<td>\n");
        rtmp.uc_tree_ctx = 0;
        if (!ud_render_node(ud, &rtmp,
          &note->ur_list->unl_head->un_next->un_data.un_list)) return 0;
        buffer_puts(out, "</td>\n");
        buffer_puts(out, "</tr>\n");
      }
    }
    buffer_puts(out, "</table>\n");
    buffer_puts(out, "</div>\n\n");
  }

  return 1;
}

/*
 * top level callbacks
 */

static enum ud_tree_walk_stat 
xhtm_init_once(struct udoc *ud, struct udr_ctx *rc)
{
  struct xhtml_ctx *xc;

  ud_assert(rc->uc_user_data == 0);
  rc->uc_user_data = alloc_zero(sizeof(struct xhtml_ctx));
  if (!rc->uc_user_data) return UD_TREE_FAIL;
  xc = rc->uc_user_data;
  if (!dstring_init(&xc->dstr, 128)) {
    dealloc_null(&rc->uc_user_data);
    return UD_TREE_FAIL;
  }
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat 
xhtm_file_init(struct udoc *ud, struct udr_ctx *rc)
{
  unsigned long max;
  unsigned long ind;
  struct ud_ref *ref;
  const struct ud_part *part = rc->uc_part;
  const struct ud_part *ppart = part;
  struct buffer *out = &rc->uc_out->uoc_buf;
  const char *title = 0;

  x_html_xml(out, ud->ud_encoding);
  x_html_doctype(out);

  buffer_puts(out, "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n");
  buffer_puts(out, "<head>\n");

  /* print style data */
  max = ud_oht_size(&ud->ud_styles);
  for (ind = 0; ind < max; ++ind) {
    ud_assert(ud_oht_getind(&ud->ud_styles, ind, (void *) &ref));
    buffer_puts(out, "<link rel=\"stylesheet\" type=\"text/css\" href=\"");
    buffer_puts(out, ref->ur_node->un_next->un_data.un_str);
    buffer_puts(out, ".css\"/>\n");
  }

  /* get title */
  for (;;) {
    title = ppart->up_title;
    if (!title) {
      if (ppart->up_index_parent == ppart->up_index_cur) break;
      ud_oht_getind(&ud->ud_parts, ppart->up_index_parent, (void *) &ppart);
    } else {
      buffer_puts(out, "<title>");
      x_escape_puts(out, title, 0);
      buffer_puts(out, "</title>\n");
      break;
    }
  }

  buffer_puts(out, "</head>\n<body>\n");

  /* render backend-specific header */
  if (ud->ud_render_header)
    if (!udr_print_file(ud, rc, ud->ud_render_header, 0, 0))
      return UD_TREE_FAIL;

  if (ud->ud_opts.ud_split_thresh)
    x_navbar(ud, out, part, "ud_navbar_head");

  buffer_puts(out, "<!--file_init-->\n");
  buffer_puts(out, "<div>\n");

  /* only print large title on first page */
  if (title && part->up_index_cur == 0)
    buffer_puts3(out, "<h2>", title, "</h2>\n");

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
xhtm_list(struct udoc *ud, struct udr_ctx *rc)
{
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
xhtm_symbol(struct udoc *ud, struct udr_ctx *rc)
{
  const char *sym = rc->uc_tree_ctx->utc_state->utc_list->unl_head->un_data.un_sym;
  enum ud_tag tag;

  if (rc->uc_tree_ctx->utc_state->utc_list_pos == 0)
    if (ud_tag_by_name(sym, &tag))
      return dispatch(tag_starts, tag_starts_size, ud, rc, tag);

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
xhtm_string(struct udoc *ud, struct udr_ctx *rc)
{
  const char *sym = rc->uc_tree_ctx->utc_state->utc_list->unl_head->un_data.un_sym;
  enum ud_tag tag;

  if (ud_tag_by_name(sym, &tag)) {
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
  if (!x_string(ud, rc)) return UD_TREE_FAIL;
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
xhtm_list_end(struct udoc *ud, struct udr_ctx *rc)
{
  const char *sym = rc->uc_tree_ctx->utc_state->utc_list->unl_head->un_data.un_sym;
  enum ud_tag tag;

  if (ud_tag_by_name(sym, &tag))
    return dispatch(tag_ends, tag_ends_size, ud, rc, tag);

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
xhtm_file_finish(struct udoc *ud, struct udr_ctx *rc)
{
  const struct ud_part *part = rc->uc_part;
  struct buffer *out = &rc->uc_out->uoc_buf;

  x_footnotes(ud, rc);

  buffer_puts(out, "\n<!--file_finish-->\n");
  buffer_puts(out, "</div>\n");

  if (ud->ud_opts.ud_split_thresh)
    x_navbar(ud, out, part, "ud_navbar_foot");

  /* render backend-specific footer */
  if (ud->ud_render_footer)
    if (!udr_print_file(ud, rc, ud->ud_render_footer, 0, 0)) return UD_TREE_FAIL;

  buffer_puts(out, "</body>\n</html>\n");

  ud_try_sys(ud, buffer_flush(out) != -1, UD_TREE_FAIL, "write");
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
xhtm_finish_once(struct udoc *ud, struct udr_ctx *rc)
{
  struct xhtml_ctx *xc = rc->uc_user_data;
  dstring_free(&xc->dstr);
  dealloc_null(&rc->uc_user_data);
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
