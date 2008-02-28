#include <corelib/array.h>
#include <corelib/bin.h>
#include <corelib/fmt.h>
#include <corelib/hashtable.h>
#include <corelib/open.h>
#include <corelib/read.h>

#include "log.h"

#define UDOC_IMPLEMENTATION
#include "ud_tag.h"
#include "ud_tree.h"
#include "ud_valid.h"
#include "udoc.h"

#define UD_ARRAY_SIZEOF(x) (sizeof((x)) / sizeof((x)[0]))

static const enum ud_node_type rules_footnote[] = { UDOC_TYPE_LIST };
static const enum ud_node_type rules_style[] = { UDOC_TYPE_STRING };
static const enum ud_node_type rules_encoding[] = { UDOC_TYPE_STRING };
static const enum ud_node_type rules_ref[] = { UDOC_TYPE_STRING };
static const enum ud_node_type rules_title[] = { UDOC_TYPE_STRING };
static const enum ud_node_type rules_link[] = { UDOC_TYPE_STRING, UDOC_TYPE_STRING };
static const enum ud_node_type rules_link_ext[] = { UDOC_TYPE_STRING, UDOC_TYPE_STRING };
static const enum ud_node_type rules_render_header[] = { UDOC_TYPE_STRING };
static const enum ud_node_type rules_render[] = { UDOC_TYPE_STRING };
static const enum ud_node_type rules_render_footer[] = { UDOC_TYPE_STRING };
static const struct ud_markup_rule ud_markup_rules[] = {
  { UDOC_TAG_REF,             rules_ref, 1, 1 },
  { UDOC_TAG_TITLE,           rules_title, 1, 1 },
  { UDOC_TAG_LINK,            rules_link, 1, 2 },
  { UDOC_TAG_LINK_EXT,        rules_link_ext, 1, 2 },
  { UDOC_TAG_ENCODING,        rules_encoding, 1, 1 },
  { UDOC_TAG_STYLE,           rules_style, 1, 1 },
  { UDOC_TAG_RENDER_HEADER,   rules_render_header, 1, 1 },
  { UDOC_TAG_RENDER,          rules_render, 1, 1 },
  { UDOC_TAG_RENDER_NOESCAPE, rules_render, 1, 1 },
  { UDOC_TAG_RENDER_FOOTER,   rules_render_footer, 1, 1 },
  { UDOC_TAG_FOOTNOTE,        rules_footnote, 1, 1 },
};
static const unsigned int ud_markup_rules_size = UD_ARRAY_SIZEOF(ud_markup_rules);

/* tables of tags that are either forbidden or demanded as parents by tags */
static const enum ud_tag par_f_table[] = { UDOC_TAG_TABLE, UDOC_TAG_TABLE_ROW };
static const enum ud_tag par_f_table_row[] = { UDOC_TAG_TABLE_ROW };
static const enum ud_tag par_d_table_row[] = { UDOC_TAG_TABLE };
static const struct {
  enum ud_tag tag;
  const enum ud_tag *par_forbid;
  const unsigned int par_forbid_size;
  const enum ud_tag *par_demand;
  const unsigned int par_demand_size;
} ud_parent_rules[] = {
  { UDOC_TAG_TABLE,
    par_f_table, UD_ARRAY_SIZEOF(par_f_table), 0, 0 },
  { UDOC_TAG_TABLE_ROW,
    par_f_table_row, UD_ARRAY_SIZEOF(par_f_table_row),
    par_d_table_row, UD_ARRAY_SIZEOF(par_d_table_row), },
};
static const unsigned int ud_parent_rules_size = UD_ARRAY_SIZEOF(ud_parent_rules);

struct validate_ctx {
  enum {
    V_TOO_FEW_ARGS,
    V_TOO_MANY_ARGS,
    V_BAD_TYPE,
    V_SECTION_AT_START,
    V_ILLEGAL_PARENT,
    V_NO_DEMANDED_PARENTS
  } error;
  unsigned long arg;
  enum ud_tag tag;
};

static void
valid_error(struct udoc *doc, const struct ud_node *n, struct validate_ctx *vc)
{
  char ln[FMT_ULONG];
  char ns[FMT_ULONG];

  ln[fmt_ulong(ln, n->un_line_num)] = 0;
  ns[fmt_ulong(ns, vc->arg + 1)] = 0;
  switch (vc->error) {
    case V_TOO_FEW_ARGS:
      log_4x(LOG_ERROR, ln, ": ", n->un_data.un_sym, ": too few args");
      break;
    case V_TOO_MANY_ARGS:
      log_4x(LOG_ERROR, ln, ": ", n->un_data.un_sym, ": too many args");
      break;
    case V_BAD_TYPE:
      log_5x(LOG_ERROR, ln, ": ", n->un_data.un_sym, ": invalid type for argument ", ns);
      break;
    case V_SECTION_AT_START:
      log_2x(LOG_ERROR, ln, ": symbol at root cannot be \"section\"");
      break;
    case V_ILLEGAL_PARENT:
      log_5x(LOG_ERROR, ln, ": ", n->un_data.un_sym, ": cannot be child of ", ud_tag_name(vc->tag));
      break;
    case V_NO_DEMANDED_PARENTS:
      log_4x(LOG_ERROR, ln, ": ", n->un_data.un_sym, ": invalid parent");
      break;
    default:
      break;
  }
}

static void
validate_error(struct udoc *doc, struct ud_tree_ctx *ctx)
{
  valid_error(doc, ctx->state->node, ctx->state->user_data);
}

static enum ud_tree_walk_stat
validate_init(struct udoc *doc, struct ud_tree_ctx *ctx)
{
  struct validate_ctx *vc = ctx->state->user_data;
  enum ud_tag tag;

  /* to simplify chunking, a symbol at the root cannot be 'section' */
  if (doc->ud_tree.ut_root.unl_head->un_type == UDOC_TYPE_SYMBOL) {
    if (ud_tag_by_name(doc->ud_tree.ut_root.unl_head->un_data.un_sym, &tag)) {
      if (tag == UDOC_TAG_SECTION) {
        vc->error = V_SECTION_AT_START;
        valid_error(doc, doc->ud_tree.ut_root.unl_head, vc);
        return UD_TREE_FAIL;
      }
    }
  }
  return UD_TREE_OK;
}

static int
check_list(struct udoc *doc, struct ud_tree_ctx *ctx, enum ud_tag tag)
{
  const struct ud_markup_rule *rule = 0;
  const struct ud_node *n = ctx->state->node;
  struct validate_ctx *vc = ctx->state->user_data;
  unsigned long len;
  unsigned int ind;
 
  for (ind = 0; ind < ud_markup_rules_size; ++ind) {
    if (tag == ud_markup_rules[ind].umr_tag) {
      rule = &ud_markup_rules[ind];
      break;
    }
  }
  if (!rule) return 1;

  len = ud_list_len(n);
  if (rule->umr_req_args)
    if (len < rule->umr_req_args + 1) { vc->error = V_TOO_FEW_ARGS; return 0; }
  if (rule->umr_max_args)
    if (len > rule->umr_max_args + 1) { vc->error = V_TOO_MANY_ARGS; return 0; }

  ind = 0;
  for (;;) {
    if (!n->un_next) break;
    if (rule->umr_arg_types)
      if (n->un_next->un_type != rule->umr_arg_types[ind]) {
        vc->error = V_BAD_TYPE;
        vc->arg = ind;
        return 0;
      }
    n = n->un_next;
    ++ind;
  }
  return 1;
}

static int
check_parents(struct udoc *doc, struct ud_tree_ctx *ctx, enum ud_tag tag)
{
  unsigned int r;
  unsigned int f;
  unsigned int got_par;
  struct validate_ctx *vc = ctx->state->user_data;

  for (r = 0; r < ud_parent_rules_size; ++r) {
    if (ud_parent_rules[r].tag == tag) {
      if (ud_parent_rules[r].par_demand_size) {
        got_par = 0;
        for (f = 0; f < ud_parent_rules[r].par_demand_size; ++f) {
          if (ud_tag_stack_above(&ctx->state->tag_stack,
                                ud_parent_rules[r].par_demand[f])) {
            got_par = 1;
            break;
          }
        }
        if (!got_par) {
          vc->error = V_NO_DEMANDED_PARENTS;
          return 0;
        }
      }
      for (f = 0; f < ud_parent_rules[r].par_forbid_size; ++f) {
        if (ud_tag_stack_above(&ctx->state->tag_stack,
                                ud_parent_rules[r].par_forbid[f])) {
          vc->error = V_ILLEGAL_PARENT;
          vc->tag = ud_parent_rules[r].par_forbid[f];
          return 0;
        }
      }
    }
  }
  return 1;
}

static enum ud_tree_walk_stat
validate_symbol(struct udoc *doc, struct ud_tree_ctx *ctx)
{
  char ln[FMT_ULONG];
  const struct ud_node *n = ctx->state->node;
  enum ud_tag tag;

  if (ctx->state->list_pos != 0) return UD_TREE_OK;
  if (!ud_tag_by_name(n->un_data.un_sym, &tag)) return UD_TREE_OK;
  if (!check_list(doc, ctx, tag)) return UD_TREE_FAIL;
  if (!check_parents(doc, ctx, tag)) return UD_TREE_FAIL;

  switch (tag) {
    case UDOC_TAG_ENCODING:
      if (doc->ud_encoding) {
        ln[fmt_ulong(ln, n->un_next->un_line_num)] = 0;
        log_4x(LOG_WARN, ln, ": ignored extra encoding tag (\"",
               n->un_next->un_data.un_str, "\")");
      } else
        doc->ud_encoding = n->un_next->un_data.un_str;
      break;
    default:
      break;
  }
  return UD_TREE_OK;
}

static const struct ud_tree_ctx_funcs validate_funcs = {
  validate_init,
  0,
  0,
  validate_symbol,
  0,
  0,
  0,
  validate_error,
};

int
ud_validate(struct udoc *ud)
{
  struct ud_tree_ctx ctx;
  struct ud_tree_ctx_state state;
  struct validate_ctx vc;

  if (!ud->ud_nodes) return 0; /* empty file is forbidden */

  bin_zero(&ctx, sizeof(ctx));
  bin_zero(&state, sizeof(state));
  bin_zero(&vc, sizeof(vc));

  state.list = &ud->ud_tree.ut_root;
  state.user_data = &vc;

  ctx.funcs = &validate_funcs;
  ctx.state = &state;
  return ud_tree_walk(ud, &ctx);
}
