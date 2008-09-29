#include <corelib/array.h>
#include <corelib/bin.h>
#include <corelib/fmt.h>
#include <corelib/hashtable.h>
#include <corelib/open.h>
#include <corelib/read.h>
#include <corelib/sstring.h>
#include <corelib/str.h>

#include "log.h"
#include "multi.h"

#define UDOC_IMPLEMENTATION
#include "ud_tag.h"
#include "ud_tree.h"
#include "udoc.h"

#define UD_ARRAY_SIZEOF(x) (sizeof ((x)) / sizeof ((x)[0]))

struct validate_ctx {
  enum {
    V_TOO_FEW_ARGS,
    V_TOO_MANY_ARGS,
    V_BAD_TYPE,
    V_SECTION_AT_START,
    V_ILLEGAL_PARENT,
    V_NO_DEMANDED_PARENTS,
    V_TOO_FEW_OF_TYPE,
    V_INVALID_ENCODING
  } error;
  unsigned long arg;
  enum ud_tag tag;
  enum ud_node_type type;
};

static void
validation_error (struct udoc *ud, const struct ud_node *n, struct validate_ctx *vc)
{
  char buf1[256];
  char buf2[256];
  char cnum[FMT_ULONG];
  struct sstring sb1;
  struct sstring sb2;

  sstring_init (&sb1, buf1, sizeof (buf1));
  sstring_init (&sb2, buf2, sizeof (buf2));

  sstring_catb (&sb1, cnum, fmt_ulong (cnum, n->un_line_num));
  sstring_cats (&sb1, ": ");
  sstring_cats (&sb1, n->un_data.un_sym);

  switch (vc->error) {
    case V_TOO_FEW_ARGS:
      sstring_cats (&sb2, "too few args");
      break;
    case V_TOO_MANY_ARGS:
      sstring_cats (&sb2, "too many args");
      break;
    case V_BAD_TYPE:
      cnum[fmt_ulong (cnum, vc->arg + 1)] = 0;
      sstring_cats2 (&sb2, "invalid type for argument ", cnum);
      break;
    case V_SECTION_AT_START:
      sstring_cats (&sb2, "symbol at root cannot be \"section\"");
      break;
    case V_ILLEGAL_PARENT:
      sstring_cats2 (&sb2, "cannot be child of ", ud_tag_name (vc->tag));
      break;
    case V_NO_DEMANDED_PARENTS:
      sstring_cats (&sb2, "invalid parent");
      break;
    case V_TOO_FEW_OF_TYPE:
      cnum[fmt_ulong (cnum, vc->arg)] = 0;
      sstring_cats5 (&sb2, "tag requires at least ", cnum,
        " argument of type \"", ud_node_type_name (vc->type), "\"");
      break;
    case V_INVALID_ENCODING:
      sstring_cpys (&sb1, ud->ud_encoding);
      sstring_cats (&sb2, "invalid encoding");
      break;
    default:
      break;
  }

  sstring_0 (&sb1);
  sstring_0 (&sb2);

  ud_error_extra (ud, sb1.s, sb2.s);
}

static void
validate_error (struct udoc *ud, struct ud_tree_ctx *ctx)
{
  validation_error (ud, ctx->utc_state->utc_node, ctx->utc_state->utc_user_data);
}

static enum ud_tree_walk_stat
validate_init (struct udoc *ud, struct ud_tree_ctx *ctx)
{
  struct validate_ctx *vc = ctx->utc_state->utc_user_data;
  enum ud_tag tag;

  /* to simplify part handling, root tag cannot be section or subsection */
  if (ud->ud_tree.ut_root.unl_head->un_type == UDOC_TYPE_SYMBOL) {
    if (ud_tag_by_name (ud->ud_tree.ut_root.unl_head->un_data.un_sym, &tag)) {
      if (tag == UDOC_TAG_SECTION || tag == UDOC_TAG_SUBSECTION) {
        vc->error = V_SECTION_AT_START;
        validation_error (ud, ud->ud_tree.ut_root.unl_head, vc);
        return UD_TREE_FAIL;
      }
    }
  }
  return UD_TREE_OK;
}

/*
 * check encoding
 */

static int
check_encoding (const struct udoc *ud, struct ud_tree_ctx *ctx)
{
  struct validate_ctx *vc = ctx->utc_state->utc_user_data;

  if (!str_same (ud->ud_encoding, "utf-8")) {
    vc->error = V_INVALID_ENCODING;
    return 0;
  }
  return 1;
}

/*
 * check number of args against spec.
 */

static int
check_num_args (struct udoc *ud, struct ud_tree_ctx *ctx, enum ud_tag tag)
{
  enum { ANY = -1 };
  static const struct {
    long min;
    long max;
  } rules[] = {
    [UDOC_TAG_CONTENTS]        = { .min = 0, .max = 0 },
    [UDOC_TAG_DATE]            = { .min = 0, .max = 0 },
    [UDOC_TAG_ENCODING]        = { .min = 1, .max = 1 },
    [UDOC_TAG_FOOTNOTE]        = { .min = 1, .max = 1 },
    [UDOC_TAG_ITEM]            = { .min = ANY, .max = ANY },
    [UDOC_TAG_LINK]            = { .min = 1, .max = 2 },
    [UDOC_TAG_LINK_EXT]        = { .min = 1, .max = 2 },
    [UDOC_TAG_LIST]            = { .min = 1, .max = ANY },
    [UDOC_TAG_PARA]            = { .min = ANY, .max = ANY },
    [UDOC_TAG_PARA_VERBATIM]   = { .min = ANY, .max = ANY },
    [UDOC_TAG_REF]             = { .min = 1, .max = 1 },
    [UDOC_TAG_RENDER]          = { .min = 1, .max = 1 },
    [UDOC_TAG_RENDER_FOOTER]   = { .min = 1, .max = 1 },
    [UDOC_TAG_RENDER_HEADER]   = { .min = 1, .max = 1 },
    [UDOC_TAG_RENDER_NOESCAPE] = { .min = 1, .max = 1 },
    [UDOC_TAG_SECTION]         = { .min = ANY, .max = ANY },
    [UDOC_TAG_STYLE]           = { .min = 1, .max = 1 },
    [UDOC_TAG_SUBSECTION]      = { .min = ANY, .max = ANY },
    [UDOC_TAG_TABLE]           = { .min = 1, .max = ANY },
    [UDOC_TAG_TABLE_ROW]       = { .min = 1, .max = ANY },
    [UDOC_TAG_TITLE]           = { .min = 1, .max = 1 },
  };
  const long len = ud_list_len (ctx->utc_state->utc_node) - 1;
  struct validate_ctx *vc = ctx->utc_state->utc_user_data;

  if (rules[tag].min == ANY) return 1;
  if (len < rules[tag].min && rules[tag].min != ANY) {
    vc->error = V_TOO_FEW_ARGS;
    return 0;
  }
  if (len > rules[tag].max && rules[tag].max != ANY) {
    vc->error = V_TOO_MANY_ARGS;
    return 0;
  }
  return 1;
}

/*
 * check that types occur in a given sequence.
 */

static int
check_type_sequenced (struct udoc *ud, struct ud_tree_ctx *ctx, enum ud_tag tag)
{
  static const enum ud_node_type seq_encoding[] = { UDOC_TYPE_STRING };
  static const enum ud_node_type seq_footnote[] = { UDOC_TYPE_LIST };
  static const enum ud_node_type seq_link[] = { UDOC_TYPE_STRING, UDOC_TYPE_STRING };
  static const enum ud_node_type seq_link_ext[] = { UDOC_TYPE_STRING, UDOC_TYPE_STRING };
  static const enum ud_node_type seq_ref[] = { UDOC_TYPE_STRING };
  static const enum ud_node_type seq_render[] = { UDOC_TYPE_STRING };
  static const enum ud_node_type seq_style[] = { UDOC_TYPE_STRING };
  static const enum ud_node_type seq_title[] = { UDOC_TYPE_STRING };
  static const struct {
    unsigned int size;
    const enum ud_node_type *seq;
  } rules[] = {
    [UDOC_TAG_CONTENTS]        = { 0, 0 },
    [UDOC_TAG_DATE]            = { 0, 0 },
    [UDOC_TAG_ENCODING]        = { UD_ARRAY_SIZEOF (seq_encoding), seq_encoding },
    [UDOC_TAG_FOOTNOTE]        = { UD_ARRAY_SIZEOF (seq_footnote), seq_footnote },
    [UDOC_TAG_ITEM]            = { 0, 0 },
    [UDOC_TAG_LINK]            = { UD_ARRAY_SIZEOF (seq_link), seq_link },
    [UDOC_TAG_LINK_EXT]        = { UD_ARRAY_SIZEOF (seq_link_ext), seq_link_ext },
    [UDOC_TAG_LIST]            = { 0, 0 },
    [UDOC_TAG_PARA]            = { 0, 0 },
    [UDOC_TAG_PARA_VERBATIM]   = { 0, 0 },
    [UDOC_TAG_REF]             = { UD_ARRAY_SIZEOF (seq_ref), seq_ref },
    [UDOC_TAG_RENDER]          = { UD_ARRAY_SIZEOF (seq_render), seq_render },
    [UDOC_TAG_RENDER_FOOTER]   = { UD_ARRAY_SIZEOF (seq_render), seq_render },
    [UDOC_TAG_RENDER_HEADER]   = { UD_ARRAY_SIZEOF (seq_render), seq_render },
    [UDOC_TAG_RENDER_NOESCAPE] = { UD_ARRAY_SIZEOF (seq_render), seq_render },
    [UDOC_TAG_SECTION]         = { 0, 0 },
    [UDOC_TAG_STYLE]           = { UD_ARRAY_SIZEOF (seq_style), seq_style },
    [UDOC_TAG_SUBSECTION]      = { 0, 0 },
    [UDOC_TAG_TABLE]           = { 0, 0 },
    [UDOC_TAG_TABLE_ROW]       = { 0, 0 },
    [UDOC_TAG_TITLE]           = { UD_ARRAY_SIZEOF (seq_title), seq_title },
  };
  const long len = ud_list_len (ctx->utc_state->utc_node) - 1;
  const struct ud_node *n = ctx->utc_state->utc_node->un_next;
  const enum ud_node_type *seq = rules[tag].seq;
  struct validate_ctx *vc = ctx->utc_state->utc_user_data;
  long list_pos;

  if (seq) {
    for (list_pos = 0; list_pos < len; ++list_pos) {
      if (seq[list_pos] != n->un_type) {
        vc->error = V_BAD_TYPE;
        vc->arg = list_pos;
        return 0;
      }
      n = n->un_next;
    }
  }
  return 1;
}

/*
 * check that parameter types are in a given set.
 */

static int
check_type_set (struct udoc *ud, struct ud_tree_ctx *ctx, enum ud_tag tag)
{
  static const enum ud_node_type set_encoding[] = { UDOC_TYPE_STRING };
  static const enum ud_node_type set_footnote[] = { UDOC_TYPE_LIST };
  static const enum ud_node_type set_link[] = { UDOC_TYPE_STRING };
  static const enum ud_node_type set_link_ext[] = { UDOC_TYPE_STRING };
  static const enum ud_node_type set_list[] = { UDOC_TYPE_LIST, UDOC_TYPE_SYMBOL };
  static const enum ud_node_type set_ref[] = { UDOC_TYPE_STRING };
  static const enum ud_node_type set_render[] = { UDOC_TYPE_STRING };
  static const enum ud_node_type set_style[] = { UDOC_TYPE_STRING };
  static const enum ud_node_type set_table[] = { UDOC_TYPE_LIST, UDOC_TYPE_SYMBOL };
  static const enum ud_node_type set_table_row[] = { UDOC_TYPE_LIST, UDOC_TYPE_SYMBOL };
  static const enum ud_node_type set_title[] = { UDOC_TYPE_STRING };
  static const struct {
    unsigned int size;
    const enum ud_node_type *set;
  } rules[] = {
    [UDOC_TAG_CONTENTS]        = { 0, 0 },
    [UDOC_TAG_DATE]            = { 0, 0 },
    [UDOC_TAG_ENCODING]        = { UD_ARRAY_SIZEOF (set_encoding), set_encoding },
    [UDOC_TAG_FOOTNOTE]        = { UD_ARRAY_SIZEOF (set_footnote), set_footnote },
    [UDOC_TAG_ITEM]            = { 0, 0 },
    [UDOC_TAG_LINK]            = { UD_ARRAY_SIZEOF (set_link), set_link },
    [UDOC_TAG_LINK_EXT]        = { UD_ARRAY_SIZEOF (set_link_ext), set_link_ext },
    [UDOC_TAG_LIST]            = { UD_ARRAY_SIZEOF (set_list), set_list },
    [UDOC_TAG_PARA]            = { 0, 0 },
    [UDOC_TAG_PARA_VERBATIM]   = { 0, 0 },
    [UDOC_TAG_REF]             = { UD_ARRAY_SIZEOF (set_ref), set_ref },
    [UDOC_TAG_RENDER]          = { UD_ARRAY_SIZEOF (set_render), set_render },
    [UDOC_TAG_RENDER_FOOTER]   = { UD_ARRAY_SIZEOF (set_render), set_render },
    [UDOC_TAG_RENDER_HEADER]   = { UD_ARRAY_SIZEOF (set_render), set_render },
    [UDOC_TAG_RENDER_NOESCAPE] = { UD_ARRAY_SIZEOF (set_render), set_render },
    [UDOC_TAG_SECTION]         = { 0, 0 },
    [UDOC_TAG_STYLE]           = { UD_ARRAY_SIZEOF (set_style), set_style },
    [UDOC_TAG_SUBSECTION]      = { 0, 0 },
    [UDOC_TAG_TABLE]           = { UD_ARRAY_SIZEOF (set_table), set_table },
    [UDOC_TAG_TABLE_ROW]       = { UD_ARRAY_SIZEOF (set_table_row), set_table_row },
    [UDOC_TAG_TITLE]           = { UD_ARRAY_SIZEOF (set_title), set_title },
  };
  const long len = ud_list_len (ctx->utc_state->utc_node) - 1;
  struct validate_ctx *vc = ctx->utc_state->utc_user_data;
  const enum ud_node_type *set = rules[tag].set;
  const struct ud_node *n = ctx->utc_state->utc_node->un_next;
  unsigned long rule_pos;
  long list_pos;

  if (set) {
    for (list_pos = 0; list_pos < len; ++list_pos) {
      int in_set = 0;
      for (rule_pos = 0; rule_pos < rules[tag].size; ++rule_pos)
        if (set[rule_pos] == n->un_type) {
          in_set = 1;
          break;
        }
      if (!in_set) {
        vc->error = V_BAD_TYPE;
        vc->arg = list_pos;
        return 0;
      }
      n = n->un_next;
    }
  }
  return 1;
}

/*
 * check that type T occurs at least N times.
 */

static int
check_type_occurences (struct udoc *ud, struct ud_tree_ctx *ctx, enum ud_tag tag)
{
  struct type_occur {
    enum ud_node_type type;
    unsigned int num;
  };
  static const struct type_occur occ_list[] = { { UDOC_TYPE_LIST, 1 } };
  static const struct type_occur occ_table[] = { { UDOC_TYPE_LIST, 1 } };
  static const struct type_occur occ_table_row[] = { { UDOC_TYPE_LIST, 1 } };
  static const struct {
    unsigned int size;
    const struct type_occur *occ;
  } rules[] = {
    [UDOC_TAG_CONTENTS]        = { 0, 0 },
    [UDOC_TAG_DATE]            = { 0, 0 },
    [UDOC_TAG_ENCODING]        = { 0, 0 },
    [UDOC_TAG_FOOTNOTE]        = { 0, 0 },
    [UDOC_TAG_ITEM]            = { 0, 0 },
    [UDOC_TAG_LINK]            = { 0, 0 },
    [UDOC_TAG_LINK_EXT]        = { 0, 0 },
    [UDOC_TAG_LIST]            = { UD_ARRAY_SIZEOF (occ_list), occ_list },
    [UDOC_TAG_PARA]            = { 0, 0 },
    [UDOC_TAG_PARA_VERBATIM]   = { 0, 0 },
    [UDOC_TAG_REF]             = { 0, 0 },
    [UDOC_TAG_RENDER]          = { 0, 0 },
    [UDOC_TAG_RENDER_FOOTER]   = { 0, 0 },
    [UDOC_TAG_RENDER_HEADER]   = { 0, 0 },
    [UDOC_TAG_RENDER_NOESCAPE] = { 0, 0 },
    [UDOC_TAG_SECTION]         = { 0, 0 },
    [UDOC_TAG_STYLE]           = { 0, 0 },
    [UDOC_TAG_SUBSECTION]      = { 0, 0 },
    [UDOC_TAG_TABLE]           = { UD_ARRAY_SIZEOF (occ_table), occ_table },
    [UDOC_TAG_TABLE_ROW]       = { UD_ARRAY_SIZEOF (occ_table_row), occ_table_row },
    [UDOC_TAG_TITLE]           = { 0, 0 },
  };
  const long len = ud_list_len (ctx->utc_state->utc_node) - 1;
  const struct type_occur *set = rules[tag].occ;
  struct validate_ctx *vc = ctx->utc_state->utc_user_data;
  unsigned long rule_pos;
  long list_pos;

  if (set) {
    for (rule_pos = 0; rule_pos < rules[tag].size; ++rule_pos) {
      const struct ud_node *n = ctx->utc_state->utc_node->un_next;
      unsigned long occur = 0;

      for (list_pos = 0; list_pos < len; ++list_pos) {
        if (set[rule_pos].type == n->un_type) ++occur;
        n = n->un_next;
      }
      if (occur < set[rule_pos].num) {
        vc->error = V_TOO_FEW_OF_TYPE;
        vc->arg = set[rule_pos].num;
        vc->type = set[rule_pos].type;
        return 0;
      }
    }
  }
  return 1;
}

/*
 * check any required parent tags.
 */

static int
check_parents_required (struct udoc *ud, struct ud_tree_ctx *ctx, enum ud_tag tag)
{
  static const enum ud_tag set_t_row[] = { UDOC_TAG_TABLE };
  static const struct {
    unsigned int size;
    const enum ud_tag *set;
  } rules[] = {
    [UDOC_TAG_CONTENTS]        = { 0, 0 },
    [UDOC_TAG_DATE]            = { 0, 0 },
    [UDOC_TAG_ENCODING]        = { 0, 0 },
    [UDOC_TAG_FOOTNOTE]        = { 0, 0 },
    [UDOC_TAG_ITEM]            = { 0, 0 },
    [UDOC_TAG_LINK]            = { 0, 0 },
    [UDOC_TAG_LINK_EXT]        = { 0, 0 },
    [UDOC_TAG_LIST]            = { 0, 0 },
    [UDOC_TAG_PARA]            = { 0, 0 },
    [UDOC_TAG_PARA_VERBATIM]   = { 0, 0 },
    [UDOC_TAG_REF]             = { 0, 0 },
    [UDOC_TAG_RENDER]          = { 0, 0 },
    [UDOC_TAG_RENDER_FOOTER]   = { 0, 0 },
    [UDOC_TAG_RENDER_HEADER]   = { 0, 0 },
    [UDOC_TAG_RENDER_NOESCAPE] = { 0, 0 },
    [UDOC_TAG_SECTION]         = { 0, 0 },
    [UDOC_TAG_STYLE]           = { 0, 0 },
    [UDOC_TAG_SUBSECTION]      = { 0, 0 },
    [UDOC_TAG_TABLE]           = { 0, 0 },
    [UDOC_TAG_TABLE_ROW]       = { UD_ARRAY_SIZEOF (set_t_row), set_t_row },
    [UDOC_TAG_TITLE]           = { 0, 0 },
  };
  const unsigned int set_size = rules[tag].size;
  const enum ud_tag *set = rules[tag].set;
  struct validate_ctx *vc = ctx->utc_state->utc_user_data;
  unsigned int rpos;
  int got_par = 0;

  /* check tag is descendent of required tag */
  if (set) {
    for (rpos = 0; rpos < set_size; ++rpos) {
      if (ud_tag_stack_above (&ctx->utc_state->utc_tag_stack, set[rpos])) {
        got_par = 1;
        break;
      }
    }
    if (!got_par) {
      vc->error = V_NO_DEMANDED_PARENTS;
      return 0;
    }
  }
  return 1;
}

/*
 * check any forbidden parent tags.
 */

static int
check_parents_forbidden (struct udoc *ud, struct ud_tree_ctx *ctx, enum ud_tag tag)
{
  static const enum ud_tag set_table[] = { UDOC_TAG_TABLE, UDOC_TAG_TABLE_ROW };
  static const enum ud_tag set_t_row[] = { UDOC_TAG_TABLE_ROW };
  static const struct {
    unsigned int size;
    const enum ud_tag *set;
  } rules[] = {
    [UDOC_TAG_CONTENTS]        = { 0, 0 },
    [UDOC_TAG_DATE]            = { 0, 0 },
    [UDOC_TAG_ENCODING]        = { 0, 0 },
    [UDOC_TAG_FOOTNOTE]        = { 0, 0 },
    [UDOC_TAG_ITEM]            = { 0, 0 },
    [UDOC_TAG_LINK]            = { 0, 0 },
    [UDOC_TAG_LINK_EXT]        = { 0, 0 },
    [UDOC_TAG_LIST]            = { 0, 0 },
    [UDOC_TAG_PARA]            = { 0, 0 },
    [UDOC_TAG_PARA_VERBATIM]   = { 0, 0 },
    [UDOC_TAG_REF]             = { 0, 0 },
    [UDOC_TAG_RENDER]          = { 0, 0 },
    [UDOC_TAG_RENDER_FOOTER]   = { 0, 0 },
    [UDOC_TAG_RENDER_HEADER]   = { 0, 0 },
    [UDOC_TAG_RENDER_NOESCAPE] = { 0, 0 },
    [UDOC_TAG_SECTION]         = { 0, 0 },
    [UDOC_TAG_STYLE]           = { 0, 0 },
    [UDOC_TAG_SUBSECTION]      = { 0, 0 },
    [UDOC_TAG_TABLE]           = { UD_ARRAY_SIZEOF (set_table), set_table },
    [UDOC_TAG_TABLE_ROW]       = { UD_ARRAY_SIZEOF (set_t_row), set_t_row },
    [UDOC_TAG_TITLE]           = { 0, 0 },
  };
  const unsigned int set_size = rules[tag].size;
  const enum ud_tag *set = rules[tag].set;
  struct validate_ctx *vc = ctx->utc_state->utc_user_data;
  unsigned int rpos;

  /* check tag is not descendent of forbidden tag */
  if (set) {
    for (rpos = 0; rpos < set_size; ++rpos) {
      if (ud_tag_stack_above (&ctx->utc_state->utc_tag_stack, set[rpos])) {
        vc->error = V_ILLEGAL_PARENT;
        vc->tag = set[rpos];
        return 0;
      }
    }
  }
  return 1;
}

static enum ud_tree_walk_stat
validate_symbol (struct udoc *ud, struct ud_tree_ctx *ctx)
{
  char ln[FMT_ULONG];
  const struct ud_node *n = ctx->utc_state->utc_node;
  enum ud_tag tag;

  if (ctx->utc_state->utc_list_pos != 0) return UD_TREE_OK;
  if (!ud_tag_by_name (n->un_data.un_sym, &tag)) return UD_TREE_OK;

  if (!check_num_args (ud, ctx, tag)) return UD_TREE_FAIL;
  if (!check_type_sequenced (ud, ctx, tag)) return UD_TREE_FAIL;
  if (!check_type_set (ud, ctx, tag)) return UD_TREE_FAIL;
  if (!check_type_occurences (ud, ctx, tag)) return UD_TREE_FAIL;

  if (!check_parents_required (ud, ctx, tag)) return UD_TREE_FAIL;
  if (!check_parents_forbidden (ud, ctx, tag)) return UD_TREE_FAIL;

  switch (tag) {
    case UDOC_TAG_ENCODING:
      if (ud->ud_encoding) {
        ln[fmt_ulong (ln, n->un_next->un_line_num)] = 0;
        log_6x (LOG_WARN, ud->ud_cur_doc->ud_name, ": ", ln,
          ": ignored extra encoding tag (\"", n->un_next->un_data.un_str, "\")");
      } else
        ud->ud_encoding = n->un_next->un_data.un_str;
      break;
    default:
      break;
  }
  return UD_TREE_OK;
}

static enum ud_tree_walk_stat
validate_finish (struct udoc *ud, struct ud_tree_ctx *ctx)
{
  struct validate_ctx *vc = ctx->utc_state->utc_user_data;

  if (!ud->ud_encoding) ud->ud_encoding = "utf-8";
  if (!check_encoding (ud, ctx)) {
    validation_error (ud, ud->ud_tree.ut_root.unl_head, vc);
    return UD_TREE_FAIL;
  }
  return UD_TREE_OK;
}

static const struct ud_tree_ctx_funcs validate_funcs = {
  .utcf_init = validate_init,
  .utcf_symbol = validate_symbol,
  .utcf_finish = validate_finish,
  .utcf_error = validate_error,
};

int
ud_validate (struct udoc *ud)
{
  struct ud_tree_ctx ctx;
  struct ud_tree_ctx_state state;
  struct validate_ctx vc;

  /* empty file is forbidden */
  ud_try (ud, ud->ud_nodes, UD_TREE_FAIL, "file is empty");

  bin_zero (&ctx, sizeof (ctx));
  bin_zero (&state, sizeof (state));
  bin_zero (&vc, sizeof (vc));

  state.utc_list = &ud->ud_tree.ut_root;
  state.utc_user_data = &vc;

  ctx.utc_funcs = &validate_funcs;
  ctx.utc_state = &state;
  return ud_tree_walk (ud, &ctx) != UD_TREE_FAIL;
}
