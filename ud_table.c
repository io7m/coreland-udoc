#include <corelib/bin.h>
#include <corelib/str.h>

#define UDOC_IMPLEMENTATION
#include "udoc.h"
#include "ud_table.h"

static enum ud_tree_walk_stat
ud_column_string (struct udoc *ud, struct ud_tree_ctx *tree_ctx)
{
  const struct ud_node *head = tree_ctx->utc_state->utc_list->unl_head;
  const struct ud_node *cur = tree_ctx->utc_state->utc_node;
  struct ud_table *udt = tree_ctx->utc_state->utc_user_data;
  enum ud_tag tag;
  unsigned long len;

  if (!ud_tag_by_name (head->un_data.un_sym, &tag)) return UD_TREE_OK;

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
    default:
      len = str_len (cur->un_data.un_str);
      udt->ut_char_width += len;
      if (udt->ut_longest_header < len) udt->ut_longest_header = len;
      break;
  }
  return UD_TREE_OK;
}

/*
 * calculate width of table in characters by summing the number of visible
 * characters in columns.
 */

static void
ud_row_width (struct udoc *ud, const struct ud_node_list *list,
  struct ud_table *udt)
{
  static const struct ud_tree_ctx_funcs measure_funcs = {
    .utcf_string = ud_column_string,
  };
  struct ud_tree_ctx tree_ctx;
  struct ud_tree_ctx_state state;

  bin_zero (&tree_ctx, sizeof (tree_ctx));
  bin_zero (&state, sizeof (state));

  state.utc_list = list;
  state.utc_user_data = udt;

  tree_ctx.utc_funcs = &measure_funcs;
  tree_ctx.utc_state = &state;

  ud_tree_walk (ud, &tree_ctx);
}

/*
 * calculate number of columns in row
 */

static void
ud_row_measure (struct udoc *ud, const struct ud_node_list *list,
  struct ud_table *udt)
{
  const struct ud_node *n = list->unl_head;
  unsigned long cols = 0;

  for (;;) {
    if (n->un_type == UDOC_TYPE_LIST) ++cols;
    if (n->un_next) n = n->un_next; else break;
  }

  udt->ut_cols = (cols > udt->ut_cols) ? cols : udt->ut_cols;
}

/*
 * calculate number of columns, rows and width in characters of table
 */

void
ud_table_measure (struct udoc *ud, const struct ud_node_list *list,
  struct ud_table *udt)
{
  const struct ud_node *n = list->unl_head;
  int top = 1;

  bin_zero (udt, sizeof (*udt));

  for (;;) {
    if (n->un_type == UDOC_TYPE_LIST) {
      ++udt->ut_rows;
      ud_row_measure (ud, &n->un_data.un_list, udt);
      if (top) {
        ud_row_width (ud, &n->un_data.un_list, udt);
        top = 0;
      }
    }
    if (n->un_next) n = n->un_next; else break;
  }
}

/*
 * guess the best possible width for formatting a character-based table.
 */

unsigned long
ud_table_advise_width (const struct udoc *ud, const struct ud_table *udt)
{
  const unsigned int padding = 2;
  return (udt->ut_longest_header * udt->ut_cols) + (udt->ut_cols * padding);
}
