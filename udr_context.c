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
  buffer_puts(&u->out->buf, "{\\tt\n");
  buffer_puts(&u->out->buf, "\\obeyspaces\n");
  buffer_puts(&u->out->buf, "\\startlines\n");

  ((struct tex_ctx *) u->user_data)->verbatim = 1;
  return 1;
}

static int
rt_literal_end(struct udr_ctx *u)
{
  buffer_puts(&u->out->buf, "\\stoplines}\n");

  ((struct tex_ctx *) u->user_data)->verbatim = 0;
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
  buffer_puts(&rc->out->buf, "\\footnote[]{");
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rt_tag_section(struct udoc *ud, struct udr_ctx *rc)
{
  const char *st;
  unsigned long type = rc->part->up_depth;

  if (rc->opts && rc->opts->udr_split_hint) {
    if (type >= rc->opts->udr_split_hint)
      type -= rc->opts->udr_split_hint;
  }

  switch (type) {
    case 0:
    case 1: st = "\\chapter"; break;
    case 2: st = "\\section"; break;
    case 3: st = "\\subsection"; break;
    default: st = "\\subsubsection"; break;
  }

  buffer_puts4(&rc->out->buf, st, "{", rc->part->up_num_string, " ");
  tex_escape_puts(&rc->out->buf, rc->part->up_title, rc->user_data);
  buffer_puts(&rc->out->buf, "}\n\n");
  return 1;
}

static enum ud_tree_walk_stat
rt_tag_link_ext(struct udoc *ud, struct udr_ctx *rc)
{
  char cnum[FMT_ULONG];
  const struct ud_node *node = rc->tree_ctx->state->node;
  struct ud_ref *ref;
  unsigned long ind;
  unsigned long max;

  /* urls are numbered, so a linear search is necessary to work out the url id */
  /* XXX: this is not ideal... */
  max = ud_oht_size(&ud->ud_link_exts);
  for (ind = 0; ind < max; ++ind) {
    ud_assert(ud_oht_getind(&ud->ud_link_exts, ind, (void *) &ref));
    if (node == ref->ur_node) {
      buffer_puts(&rc->out->buf, "\\from[url_");
      buffer_put(&rc->out->buf, cnum, fmt_ulong(cnum, ind));
      buffer_puts(&rc->out->buf, "]");
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
  const struct ud_node *node = rc->tree_ctx->state->node;

  ref = node->un_next->un_data.un_str;
  text = (node->un_next->un_next) ? node->un_next->un_next->un_data.un_str : 0;

  if (text) {
    buffer_puts(&rc->out->buf, "(");
    tex_escape_puts(&rc->out->buf, text, &rc);
    buffer_puts(&rc->out->buf, ") ");
  }

  buffer_puts3(&rc->out->buf, "\\at{[page }{]}[r_", ref, "] ");
  return 1;
}

static enum ud_tree_walk_stat
rt_tag_ref(struct udoc *ud, struct udr_ctx *rc)
{
  const struct ud_node *n = rc->tree_ctx->state->node;

  buffer_puts3(&rc->out->buf, "\\pagereference[r_", n->un_next->un_data.un_str, "]\n");
  return 1;
}

static enum ud_tree_walk_stat
rt_tag_render_noescape(struct udoc *ud, struct udr_ctx *rc)
{
  if (!udr_print_file(ud, rc, rc->tree_ctx->state->node->un_next->un_data.un_str, 0, 0))
    return UD_TREE_FAIL;

  return 1;
}

static enum ud_tree_walk_stat
rt_tag_render(struct udoc *ud, struct udr_ctx *rc)
{
  if (!udr_print_file(ud, rc, rc->tree_ctx->state->node->un_next->un_data.un_str,
                          tex_escape_put, rc->user_data)) return UD_TREE_FAIL;
  return 1;
}

static enum ud_tree_walk_stat
rt_tag_date(struct udoc *ud, struct udr_ctx *rc)
{
  char buf[CALTIME_FMT];
  struct caltime ct;
  struct buffer *out = &rc->out->buf;

  caltime_local(&ct, &ud->ud_time_start.sec, 0, 0);
  buffer_put(out, buf, caltime_fmt(buf, &ct));
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
  const struct ud_node *n = rc->tree_ctx->state->node;
  struct udr_ctx rtmp = *rc;

  buffer_puts(&rc->out->buf, "\\startitemize\n");

  /* for each item in list, render list */
  for (;;) {
    if (n->un_type == UDOC_TYPE_LIST) {
      buffer_puts(&rc->out->buf, "\\item ");
      rtmp.tree_ctx = 0;
      if (!ud_render_node(ud, &rtmp, &n->un_data.un_list)) return UD_TREE_FAIL;
      buffer_puts(&rc->out->buf, "\n");
    }
    if (n->un_next) n = n->un_next; else break;
  }

  buffer_puts(&rc->out->buf, "\\stopitemize\n");
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
  buffer_puts(&rc->out->buf, "\n");
  rt_literal_end(rc);
  buffer_puts(&rc->out->buf, "\n");
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rt_tag_end_para(struct udoc *ud, struct udr_ctx *rc)
{
  buffer_puts(&rc->out->buf, "\n\n");
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rt_tag_end_footnote(struct udoc *ud, struct udr_ctx *rc)
{
  buffer_puts(&rc->out->buf, "} ");
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
  if (!rc->user_data)
    rc->user_data = alloc_zero(sizeof(struct tex_ctx));
  return (rc->user_data) ? UD_TREE_OK : UD_TREE_FAIL;
}

static enum ud_tree_walk_stat
rt_finish_once(struct udoc *ud, struct udr_ctx *rc)
{
  dealloc_null(&rc->user_data);
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

  /* udoc preamble */
  buffer_puts(&rc->out->buf, "% udoc preamble\n");
  buffer_puts(&rc->out->buf, tex_header);
  buffer_puts(&rc->out->buf, "\n");

  /* output \useURL list */
  buffer_puts(&rc->out->buf, "% urls\n");
  max = ud_oht_size(&ud->ud_link_exts);
  for (ind = 0; ind < max; ++ind) {
    ud_assert(ud_oht_getind(&ud->ud_link_exts, ind, (void *) &ref));
    node = ref->ur_node->un_next;
    buffer_puts(&rc->out->buf, "\\useURL[url_");
    buffer_put(&rc->out->buf, cnum, fmt_ulong(cnum, ind));
    buffer_puts3(&rc->out->buf, "][", node->un_data.un_str, "][][");
    if (node->un_next)
      buffer_puts2(&rc->out->buf, node->un_next->un_data.un_str, "]\n");
    else
      buffer_puts2(&rc->out->buf, node->un_data.un_str, "]\n");
  }
  buffer_puts(&rc->out->buf, "\n");

  /* output styles */
  buffer_puts(&rc->out->buf, "% styles\n");
  max = ud_oht_size(&ud->ud_styles);
  for (ind = 0; ind < max; ++ind) {
    ud_assert(ud_oht_getind(&ud->ud_styles, ind, (void *) &ref));
    if (!udr_print_file(ud, rc, ref->ur_node->un_next->un_data.un_str, 0, 0))
      return UD_TREE_FAIL;
  }

  buffer_puts(&rc->out->buf, "\n");
  buffer_puts(&rc->out->buf, "\\starttext\n");

  /* optional header */
  buffer_puts(&rc->out->buf, "% render-header\n");
  if (ud->ud_render_header)
    if (!udr_print_file(ud, rc, ud->ud_render_header, 0, 0))
      return UD_TREE_FAIL;

  /* document title */
  if (rc->part->up_title) {
    buffer_puts(&rc->out->buf, "\\chapter{");
    buffer_puts2(&rc->out->buf, rc->part->up_title, "}\n");
  }

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rt_symbol(struct udoc *ud, struct udr_ctx *rc)
{
  const char *sym = rc->tree_ctx->state->list->unl_head->un_data.un_sym;
  enum ud_tag tag;

  if (rc->tree_ctx->state->list_pos == 0)
    if (ud_tag_by_name(sym, &tag))
      return dispatch(tag_starts, tag_starts_size, ud, rc, tag);

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rt_string(struct udoc *ud, struct udr_ctx *rc)
{
  const struct ud_tree_ctx *tc = rc->tree_ctx;
  enum ud_tag tag;

  if (!ud_tag_by_name(tc->state->list->unl_head->un_data.un_sym, &tag)) return UD_TREE_OK;
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
      tex_escape_puts(&rc->out->buf, rc->tree_ctx->state->node->un_data.un_str, rc->user_data);
      break;
  }
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rt_list_end(struct udoc *ud, struct udr_ctx *rc)
{
  const char *sym = rc->tree_ctx->state->list->unl_head->un_data.un_sym;
  enum ud_tag tag;

  if (ud_tag_by_name(sym, &tag))
    return dispatch(tag_ends, tag_ends_size, ud, rc, tag);

  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
rt_file_finish(struct udoc *ud, struct udr_ctx *rc)
{
  /* optional footer */
  buffer_puts(&rc->out->buf, "% render-footer\n");
  if (ud->ud_render_footer)
    if (!udr_print_file(ud, rc, ud->ud_render_footer, 0, 0))
      return UD_TREE_FAIL;

  buffer_puts(&rc->out->buf, "\\stoptext\n");
  return UD_TREE_OK;
}

const struct ud_renderer ud_render_context = {
  {
    .init_once = rt_init_once,
    .file_init = rt_file_init,
    .list = 0,
    .symbol = rt_symbol,
    .string = rt_string,
    .list_end = rt_list_end,
    .file_finish = rt_file_finish,
    .finish_once = rt_finish_once,
    .error = 0, 
  },
  {
    .name = "context",
    .suffix = "tex",
    .desc = "ConTeXt output",
    .part = 0,
  },
};
