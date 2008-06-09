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

#define UDOC_IMPLEMENTATION
#include "udoc.h"
#include "ud_assert.h"
#include "ud_ref.h"

struct tex_ctx {
  unsigned int verbatim;
  char ch_cur;
  char ch_prev;
};

static const char tex_header[] =
"\\setupinteraction[state=start]\n"
"\\placebookmarks[chapter,section,section,subsection,subsubsection][chapter,section]\n"
"\\setupinteractionscreen[option=bookmark]\n"
"\\setuphead[chapter][number=no]\n"
"\\setuphead[section][number=no]\n"
"\\setuphead[subsection][number=no]\n"
"\\setuphead[subsubsection][number=no]\n"
;

static int
tex_escape_put(struct buffer *out, const char *str, unsigned long len, void *ctx)
{
  struct tex_ctx *tc = ctx;

  while (len) {
    tc->ch_cur = *str;
    switch (tc->ch_cur) {
      case '$': buffer_puts(out, "\\$"); break;
      case '#': buffer_puts(out, "\\#"); break;
      case '&': buffer_puts(out, "\\&"); break;
      case '%': buffer_puts(out, "\\%"); break;
      case '_': buffer_puts(out, "\\_"); break;
      case '^': buffer_puts(out, "\\^{}"); break;
      case '~': buffer_puts(out, "\\~{}"); break;
      case '{': buffer_puts(out, "$\\{$"); break;
      case '}': buffer_puts(out, "$\\}$"); break;
      case '|': buffer_puts(out, "$|$"); break;
      case '\\': buffer_puts(out, "$\\backslash$"); break;
      case '\n':
        buffer_puts(out, (tc->ch_prev == '\n' && tc->verbatim) ? "\\crlf\n" : "\n");
        break;
      case ' ':
        if (tc->verbatim) {
          buffer_puts(out, "\\ ");
        } else {
          if (tc->ch_prev != ' ' && tc->ch_prev != '\n')
            buffer_puts(out, " ");
        }
        break;
      default:
        buffer_put(out, &tc->ch_cur, 1);
        break;
    }
    ++str;
    --len;
    tc->ch_prev = tc->ch_cur;
  }
  return 1;
}

static int
tex_escape_puts(struct buffer *out, const char *str, void *data)
{
  return tex_escape_put(out, str, str_len(str), data);
}

static int
rt_literal_start(struct udr_ctx *u)
{
  struct buffer *buf = &u->uc_out->uoc_buf;

  buffer_puts(buf, "{\\tt\n");
  buffer_puts(buf, "\\obeyspaces\n");
  buffer_puts(buf, "\\startlines\n");

  ((struct tex_ctx *) u->uc_user_data)->verbatim = 1;
  return 1;
}

static int
rt_literal_end(struct udr_ctx *u)
{
  buffer_puts(&u->uc_out->uoc_buf, "\\stoplines}\n");

  ((struct tex_ctx *) u->uc_user_data)->verbatim = 0;
  return 1;
}

/*
 * tags
 */

static enum ud_tree_walk_stat
rt_tag_contents(struct udoc *ud, struct udr_ctx *rc)
{
  return 1;
}

static enum ud_tree_walk_stat
rt_tag_footnote(struct udoc *ud, struct udr_ctx *rc)
{
  buffer_puts(&rc->uc_out->uoc_buf, "\\footnote[]{");
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rt_tag_section(struct udoc *ud, struct udr_ctx *rc)
{
  const char *st;
  unsigned long type = rc->uc_part->up_depth;
  struct buffer *buf = &rc->uc_out->uoc_buf;

  if (rc->uc_opts && rc->uc_opts->uo_split_hint) {
    if (type >= rc->uc_opts->uo_split_hint)
      type -= rc->uc_opts->uo_split_hint;
  }

  switch (type) {
    case 0:
    case 1: st = "\\chapter"; break;
    case 2: st = "\\section"; break;
    case 3: st = "\\subsection"; break;
    default: st = "\\subsubsection"; break;
  }

  buffer_puts4(buf, st, "{", rc->uc_part->up_num_string, ". ");
  if (rc->uc_part->up_title)
    tex_escape_puts(buf, rc->uc_part->up_title, rc->uc_user_data);
  buffer_puts(buf, "}\n\n");
  return 1;
}

static enum ud_tree_walk_stat
rt_tag_link_ext(struct udoc *ud, struct udr_ctx *rc)
{
  char cnum[FMT_ULONG];
  const struct ud_node *node = rc->uc_tree_ctx->utc_state->utc_node;
  struct ud_ref *ref;
  struct buffer *buf = &rc->uc_out->uoc_buf;
  unsigned long ind;
  unsigned long max;

  /* urls are numbered, so a linear search is necessary to work out the url id */
  /* XXX: this is not ideal... */
  max = ud_oht_size(&ud->ud_link_exts);
  for (ind = 0; ind < max; ++ind) {
    ud_assert(ud_oht_getind(&ud->ud_link_exts, ind, (void *) &ref));
    if (node == ref->ur_node) {
      buffer_puts(buf, "\\from[url_");
      buffer_put(buf, cnum, fmt_ulong(cnum, ind));
      buffer_puts(buf, "]");
      return 1;
    }
  }

  return 1;
}

static enum ud_tree_walk_stat
rt_tag_link(struct udoc *ud, struct udr_ctx *rc)
{
  const char *ref;
  const char *text;
  const struct ud_node *node = rc->uc_tree_ctx->utc_state->utc_node;
  struct buffer *buf = &rc->uc_out->uoc_buf;

  ref = node->un_next->un_data.un_str;
  text = (node->un_next->un_next) ? node->un_next->un_next->un_data.un_str : 0;

  if (text) {
    buffer_puts(buf, "(");
    tex_escape_puts(buf, text, &rc);
    buffer_puts(buf, ") ");
  }

  buffer_puts3(buf, "\\at{[page }{]}[r_", ref, "] ");
  return 1;
}

static enum ud_tree_walk_stat
rt_tag_ref(struct udoc *ud, struct udr_ctx *rc)
{
  const struct ud_node *n = rc->uc_tree_ctx->utc_state->utc_node;
  struct buffer *buf = &rc->uc_out->uoc_buf;

  buffer_puts3(buf, "\\pagereference[r_", n->un_next->un_data.un_str, "]\n");
  return 1;
}

static enum ud_tree_walk_stat
rt_tag_render_noescape(struct udoc *ud, struct udr_ctx *rc)
{
  if (!udr_print_file(ud, rc, rc->uc_tree_ctx->utc_state->utc_node->un_next->un_data.un_str, 0, 0))
    return UD_TREE_FAIL;

  return 1;
}

static enum ud_tree_walk_stat
rt_tag_render(struct udoc *ud, struct udr_ctx *rc)
{
  if (!udr_print_file(ud, rc, rc->uc_tree_ctx->utc_state->utc_node->un_next->un_data.un_str,
                          tex_escape_put, rc->uc_user_data)) return UD_TREE_FAIL;
  return 1;
}

static enum ud_tree_walk_stat
rt_tag_date(struct udoc *ud, struct udr_ctx *rc)
{
  char cbuf[CALTIME_FMT];
  struct caltime ct;
  struct buffer *buf = &rc->uc_out->uoc_buf;

  caltime_local(&ct, &ud->ud_time_start.sec, 0, 0);
  buffer_put(buf, cbuf, caltime_fmt(cbuf, &ct));
  return 1;
}

static enum ud_tree_walk_stat
rt_tag_table(struct udoc *ud, struct udr_ctx *rc)
{
  return UD_TREE_STOP_LIST;
}

static enum ud_tree_walk_stat
rt_tag_table_row(struct udoc *ud, struct udr_ctx *rc)
{
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rt_tag_list(struct udoc *ud, struct udr_ctx *rc)
{
  struct udr_ctx rtmp = *rc;
  const struct ud_node *n = rc->uc_tree_ctx->utc_state->utc_node;
  struct buffer *buf = &rc->uc_out->uoc_buf;

  buffer_puts(buf, "\\startitemize\n");

  /* for each item in list, render list */
  for (;;) {
    if (n->un_type == UDOC_TYPE_LIST) {
      buffer_puts(buf, "\\item ");
      rtmp.uc_tree_ctx = 0;
      if (!ud_render_node(ud, &rtmp, &n->un_data.un_list)) return UD_TREE_FAIL;
      buffer_puts(buf, "\n");
    }
    if (n->un_next) n = n->un_next; else break;
  }

  buffer_puts(buf, "\\stopitemize\n");
  return UD_TREE_STOP_LIST;
}

static enum ud_tree_walk_stat
rt_tag_para_verbatim(struct udoc *ud, struct udr_ctx *rc)
{
  rt_literal_start(rc);
  return UD_TREE_OK;
}

/*
 * tag ends
 */

static enum ud_tree_walk_stat
rt_tag_end_para_verbatim(struct udoc *ud, struct udr_ctx *rc)
{
  struct buffer *buf = &rc->uc_out->uoc_buf;
  buffer_puts(buf, "\n");
  rt_literal_end(rc);
  buffer_puts(buf, "\n");
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rt_tag_end_para(struct udoc *ud, struct udr_ctx *rc)
{
  buffer_puts(&rc->uc_out->uoc_buf, "\n\n");
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rt_tag_end_footnote(struct udoc *ud, struct udr_ctx *rc)
{
  buffer_puts(&rc->uc_out->uoc_buf, "} ");
  return UD_TREE_OK;
}

/*
 * tables
 */

struct dispatch {
  enum ud_tag tag;
  enum ud_tree_walk_stat (*func)(struct udoc *, struct udr_ctx *);
};

static const struct dispatch tag_starts[] = {
  { UDOC_TAG_SECTION,         rt_tag_section },
  { UDOC_TAG_REF,             rt_tag_ref },
  { UDOC_TAG_LINK,            rt_tag_link },
  { UDOC_TAG_LINK_EXT,        rt_tag_link_ext },
  { UDOC_TAG_TABLE,           rt_tag_table },
  { UDOC_TAG_CONTENTS,        rt_tag_contents },
  { UDOC_TAG_FOOTNOTE,        rt_tag_footnote },
  { UDOC_TAG_DATE,            rt_tag_date },
  { UDOC_TAG_RENDER,          rt_tag_render },
  { UDOC_TAG_RENDER_NOESCAPE, rt_tag_render_noescape },
  { UDOC_TAG_TABLE,           rt_tag_table },
  { UDOC_TAG_TABLE_ROW,       rt_tag_table_row },
  { UDOC_TAG_LIST,            rt_tag_list },
  { UDOC_TAG_PARA_VERBATIM,   rt_tag_para_verbatim },
};
static const unsigned int tag_starts_size = sizeof(tag_starts)
                                          / sizeof(tag_starts[0]);
static const struct dispatch tag_ends[] = {
  { UDOC_TAG_PARA_VERBATIM,   rt_tag_end_para_verbatim },
  { UDOC_TAG_PARA,            rt_tag_end_para },
  { UDOC_TAG_FOOTNOTE,        rt_tag_end_footnote },
};
static const unsigned int tag_ends_size = sizeof(tag_ends)
                                        / sizeof(tag_ends[0]);

static enum ud_tree_walk_stat
dispatch(const struct dispatch *tab, unsigned int tab_size,
         struct udoc *ud, struct udr_ctx *ctx, enum ud_tag tag)
{
  unsigned int ind;
  for (ind = 0; ind < tab_size; ++ind)
    if (tag == tab[ind].tag)
      return tab[ind].func(ud, ctx);
  return UD_TREE_OK;
}

/*
 * main rendering callbacks
 */

static enum ud_tree_walk_stat
rt_init_once(struct udoc *ud, struct udr_ctx *rc)
{
  if (!rc->uc_user_data)
    rc->uc_user_data = alloc_zero(sizeof(struct tex_ctx));
  return (rc->uc_user_data) ? UD_TREE_OK : UD_TREE_FAIL;
}

static enum ud_tree_walk_stat
rt_finish_once(struct udoc *ud, struct udr_ctx *rc)
{
  dealloc_null(&rc->uc_user_data);
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rt_file_init(struct udoc *ud, struct udr_ctx *rc)
{
  char cnum[FMT_ULONG];
  unsigned long ind;
  unsigned long max;
  struct ud_ref *ref;
  const struct ud_node *node;
  struct buffer *buf = &rc->uc_out->uoc_buf;

  /* udoc preamble */
  buffer_puts(buf, "% udoc preamble\n");
  buffer_puts(buf, tex_header);
  buffer_puts(buf, "\n");

  /* output \useURL list */
  buffer_puts(buf, "% urls\n");
  max = ud_oht_size(&ud->ud_link_exts);
  for (ind = 0; ind < max; ++ind) {
    ud_assert(ud_oht_getind(&ud->ud_link_exts, ind, (void *) &ref));
    node = ref->ur_node->un_next;
    buffer_puts(buf, "\\useURL[url_");
    buffer_put(buf, cnum, fmt_ulong(cnum, ind));
    buffer_puts3(buf, "][", node->un_data.un_str, "][][");
    if (node->un_next)
      buffer_puts2(buf, node->un_next->un_data.un_str, "]\n");
    else
      buffer_puts2(buf, node->un_data.un_str, "]\n");
  }
  buffer_puts(buf, "\n");

  /* output styles */
  buffer_puts(buf, "% styles\n");
  max = ud_oht_size(&ud->ud_styles);
  for (ind = 0; ind < max; ++ind) {
    ud_assert(ud_oht_getind(&ud->ud_styles, ind, (void *) &ref));
    if (!udr_print_file(ud, rc, ref->ur_node->un_next->un_data.un_str, 0, 0))
      return UD_TREE_FAIL;
  }

  buffer_puts(buf, "\n");
  buffer_puts(buf, "\\starttext\n");

  /* optional header */
  buffer_puts(buf, "% render-header\n");
  if (ud->ud_render_header)
    if (!udr_print_file(ud, rc, ud->ud_render_header, 0, 0))
      return UD_TREE_FAIL;

  /* document title */
  if (rc->uc_part->up_title) {
    buffer_puts(buf, "\\chapter{");
    buffer_puts2(buf, rc->uc_part->up_title, "}\n");
  }

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rt_symbol(struct udoc *ud, struct udr_ctx *rc)
{
  const char *sym = rc->uc_tree_ctx->utc_state->utc_list->unl_head->un_data.un_sym;
  enum ud_tag tag;

  if (rc->uc_tree_ctx->utc_state->utc_list_pos == 0)
    if (ud_tag_by_name(sym, &tag))
      return dispatch(tag_starts, tag_starts_size, ud, rc, tag);

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rt_string(struct udoc *ud, struct udr_ctx *rc)
{
  const struct ud_tree_ctx *tc = rc->uc_tree_ctx;
  struct buffer *buf = &rc->uc_out->uoc_buf;
  enum ud_tag tag;

  if (!ud_tag_by_name(tc->utc_state->utc_list->unl_head->un_data.un_sym, &tag)) return UD_TREE_OK;
  switch (tag) {
    case UDOC_TAG_TITLE:
    case UDOC_TAG_STYLE:
    case UDOC_TAG_REF:
    case UDOC_TAG_LINK:
    case UDOC_TAG_LINK_EXT:
    case UDOC_TAG_TABLE:
    case UDOC_TAG_ENCODING:
    case UDOC_TAG_CONTENTS:
    case UDOC_TAG_FOOTNOTE:
    case UDOC_TAG_RENDER_HEADER:
    case UDOC_TAG_RENDER_FOOTER:
    case UDOC_TAG_RENDER:
    case UDOC_TAG_RENDER_NOESCAPE:
      break;
    default:
      tex_escape_puts(buf, rc->uc_tree_ctx->utc_state->utc_node->un_data.un_str,
                           rc->uc_user_data);
      break;
  }
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rt_list_end(struct udoc *ud, struct udr_ctx *rc)
{
  const char *sym = rc->uc_tree_ctx->utc_state->utc_list->unl_head->un_data.un_sym;
  enum ud_tag tag;

  if (ud_tag_by_name(sym, &tag))
    return dispatch(tag_ends, tag_ends_size, ud, rc, tag);

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rt_file_finish(struct udoc *ud, struct udr_ctx *rc)
{
  struct buffer *buf = &rc->uc_out->uoc_buf;

  /* optional footer */
  buffer_puts(buf, "% render-footer\n");
  if (ud->ud_render_footer)
    if (!udr_print_file(ud, rc, ud->ud_render_footer, 0, 0))
      return UD_TREE_FAIL;

  buffer_puts(buf, "\\stoptext\n");
  return UD_TREE_OK;
}

const struct ud_renderer ud_render_context = {
  {
    .urf_init_once = rt_init_once,
    .urf_file_init = rt_file_init,
    .urf_list = 0,
    .urf_symbol = rt_symbol,
    .urf_string = rt_string,
    .urf_list_end = rt_list_end,
    .urf_file_finish = rt_file_finish,
    .urf_finish_once = rt_finish_once,
  },
  {
    .ur_name = "context",
    .ur_suffix = "tex",
    .ur_desc = "ConTeXt output",
    .ur_part = 0,
  },
};
