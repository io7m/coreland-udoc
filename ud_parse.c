#include <corelib/alloc.h>
#include <corelib/array.h>
#include <corelib/bin.h>
#include <corelib/error.h>
#include <corelib/fmt.h>
#include <corelib/str.h>
#include <corelib/sstring.h>

#include <unistd.h>

#include "multi.h"
#include "log.h"

#define UDOC_IMPLEMENTATION
#include "ud_assert.h"
#include "ud_tag.h"
#include "udoc.h"

static void 
syntax(struct udoc *ud, const char *s)
{
  char cnum[FMT_ULONG];
  struct ud_err ue;

  cnum[fmt_ulong(cnum, ud->ud_tok.line)] = 0;
  ud_error_fill(ud, &ue, "syntax", cnum, s, 0);
  ud_error_push(ud, &ue);
}

static int
symbol_check(struct udoc *ud, const char *s)
{
  unsigned int max = str_len (s);
  unsigned int pos;
  char ch;

  if (s[0] >= '0' && s[0] <= '9') {
    syntax(ud, "first character of symbol cannot be numeric");
    return 0;
  }

  for (pos = 0; pos < max; ++pos) {
    ch = s[pos];
    if (ch == '-') continue;
    if (ch == '_') continue;
    if (ch >= '0' && ch <= '9') continue;
    if (ch >= 'A' && ch <= 'Z') continue;
    if (ch >= 'a' && ch <= 'z') continue;
    syntax(ud, "invalid character in symbol (must be alphanumeric, '-' or '_')");
    return 0;
  }
  return 1;
}

static void 
ud_tree_reduce(struct udoc *ud, struct ud_node_list *list)
{
  struct ud_node *un;

  if (list->unl_size == 1) {
    un = list->unl_head;
    if (un->un_type == UDOC_TYPE_LIST) {
      log_1xf(LOG_DEBUG, "reduced list");
      list->unl_head = un->un_data.un_list.unl_head;
      dealloc_null((void *) &un);
    }
  }
}

static int 
ud_tree_include(struct udoc *ud, char *file, struct ud_node_list *list)
{
  struct udoc ud_new;
  struct udoc *udp;
  struct ud_node cur = {0, {0}, 0, 0};

  bin_zero(&ud_new, sizeof(ud_new));
  if (list->unl_size) {
    syntax(ud, "the include symbol can only appear at the start of a list");
    goto FAIL;
  }

  /* file has already been included before? */
  if (ud_get(ud, file, &udp)) {
    log_2xf(LOG_DEBUG, "include reused: ", file);
  } else {
    if (!ud_init(&ud_new)) goto FAIL;
    ud_new.ud_main_doc = ud->ud_main_doc;
    if (!ud_open(&ud_new, file)) goto FAIL;
    if (!ud_get(ud, file, &udp)) goto FAIL;
    if (!ud_parse(udp)) goto FAIL;
    if (!ud_close(udp)) goto FAIL;
  }

  ud_assert (udp->ud_name != 0);

  /* XXX: ??? */
  if (!udp->ud_nodes) {
    log_2xf(LOG_DEBUG, file, " is empty, ignoring");
    return 1;
  }

  /* insert node pointing to new/reused document */
  cur.un_line_num = ud->ud_tok.line;
  cur.un_type = UDOC_TYPE_INCLUDE;
  cur.un_data.un_list = udp->ud_tree.ut_root;
  ud_try_sys_jump(ud, ud_list_cat(list, &cur), FAIL, "ud_list_cat");

  /* insert node with name of included file */
  cur.un_line_num = ud->ud_tok.line;
  cur.un_type = UDOC_TYPE_STRING;
  ud_try_sys_jump(ud, str_dup(file, &cur.un_data.un_str), FAIL, "str_dup");
  ud_try_sys_jump(ud, ud_list_cat(list, &cur), FAIL, "ud_list_cat");

  ud->ud_nodes += 2;
  return 1;

FAIL:
  ud_free(&ud_new);
  return 0;
}

static int 
ud_tree_symbol(struct udoc *ud, char *symbol, struct ud_node_list *list)
{
  struct ud_node cur = {0, {0}, 0, 0};
  enum token_type par_type;
  char *par_token;

  if (!str_same(symbol, "include")) {
    if (!symbol_check(ud, symbol)) goto FAIL;
    cur.un_type = UDOC_TYPE_SYMBOL;
    cur.un_line_num = ud->ud_tok.line;
    ud_try_sys_jump(ud, str_dup(symbol, &cur.un_data.un_sym), FAIL, "str_dup");
    ud_try_sys_jump(ud, ud_list_cat(list, &cur), FAIL, "ud_list_cat");
    ++ud->ud_nodes;
  } else {
    if (!token_next(&ud->ud_tok, &par_token, &par_type)) {
      syntax(ud, "malformed document (premature end of file)");
      goto FAIL;
    }
    if (par_type != TOKEN_TYPE_STRING) {
      syntax(ud, "argument to include is not a string");
      goto FAIL;
    }
    ud_try_jump(ud, ud_tree_include(ud, par_token, list), FAIL, "ud_tree_include failed");
  }

  return 1;
FAIL:
  if (cur.un_data.un_sym) dealloc_null((void *) &cur.un_data.un_sym);
  return 0;
}

static int 
ud_tree_string(struct udoc *ud, const char *str, struct ud_node_list *list)
{
  struct ud_node cur = {0, {0}, 0, 0};

  if (!list->unl_size) {
    log_1xf(LOG_DEBUG, "inserting implicit para");
    cur.un_type = UDOC_TYPE_SYMBOL;
    cur.un_line_num = ud->ud_tok.line;
    ud_try_sys_jump(ud, str_dup("para", &cur.un_data.un_sym), FAIL, "str_dup");
    ud_try_sys_jump(ud, ud_list_cat(list, &cur), FAIL, "ud_list_cat");
    ++ud->ud_nodes;
  }
  cur.un_type = UDOC_TYPE_STRING;
  cur.un_line_num = ud->ud_tok.line;
  ud_try_sys_jump(ud, str_dup(str, &cur.un_data.un_str), FAIL, "str_dup");
  ud_try_sys_jump(ud, ud_list_cat(list, &cur), FAIL, "ud_list_cat");
  ++ud->ud_nodes;

  return 1;
FAIL:
  if (cur.un_data.un_sym) dealloc_null((void *) &cur.un_data.un_sym);
  return 0;
}

static int 
ud_tree_build(struct udoc *ud, struct ud_node_list *parent)
{
  struct ud_node cur = {0, {0}, 0, 0};
  struct ud_node_list cur_list = {0, 0, 0};
  enum token_type type;
  char cnum[FMT_ULONG];
  char *token;

  cnum[fmt_ulong(cnum, ud->ud_depth)] = 0;
  log_4xf(LOG_DEBUG, "processing ", ud->ud_name, " depth ", cnum);

  for (;;) {
    errno = 0;
    bin_zero(&cur, sizeof(cur));
    if (!token_next(&ud->ud_tok, &token, &type)) {
      if (!errno) syntax(ud, "malformed document (premature end of file)");
      goto FAIL;
    }
    switch (type) {
    case TOKEN_TYPE_SYMBOL:
      log_2xf(LOG_DEBUG, "symbol: ", token);
      if (!ud_tree_symbol(ud, token, &cur_list)) goto FAIL;
      break;
    case TOKEN_TYPE_STRING:
      log_2xf(LOG_DEBUG, "string: ", token);
      if (!ud_tree_string(ud, token, &cur_list)) goto FAIL;
      break;
    case TOKEN_TYPE_PAREN_OPEN:
      log_1xf(LOG_DEBUG, "paren open");
      ++ud->ud_depth;
      cur.un_type = UDOC_TYPE_LIST;
      if (!ud_tree_build(ud, &cur.un_data.un_list)) goto FAIL;
      if (cur.un_data.un_list.unl_size) {
        ud_try_sys_jump(ud, ud_list_cat(&cur_list, &cur), FAIL, "ud_list_cat");
        ++ud->ud_nodes;
      }
      break;
    case TOKEN_TYPE_PAREN_CLOSE:
      log_1xf(LOG_DEBUG, "paren close");
      if (!ud->ud_depth) {
        syntax(ud, "unbalanced parenthesis");
        goto FAIL;
      }
      --ud->ud_depth;
      if (!cur_list.unl_size) goto EMPTY;
      ud_tree_reduce(ud, &cur_list);
      goto SUCCESS;
    case TOKEN_TYPE_EOF:
      log_1xf(LOG_DEBUG, "eof");
      if (ud->ud_depth) {
        syntax(ud, "unbalanced parenthesis");
        goto FAIL;
      }
      goto SUCCESS;
    default:
      syntax(ud, "unknown token type");
      goto FAIL;
    }
  }

SUCCESS:
  log_2xf(LOG_DEBUG, "success depth ", cnum);

  *parent = cur_list;
  return 1;

FAIL:
  if (errno)
    log_2sysf(LOG_DEBUG, "fail depth ", cnum);
  else
    log_3xf(LOG_DEBUG, "fail depth ", cnum, ": parse error");
  return 0;

EMPTY:
  log_2xf(LOG_DEBUG, "empty depth ", cnum);
  return 1;
}

int 
ud_parse(struct udoc *ud)
{
  ud_try_sys_jump(ud, fchdir(ud->ud_dirfd_src) != -1, FAIL, "fchdir");
  if (!ud_tree_build(ud, &ud->ud_tree.ut_root)) goto FAIL;
  ud_try_sys_jump(ud, fchdir(ud->ud_dirfd_src) != -1, FAIL, "fchdir");
  return 1;
  FAIL:
  fchdir(ud->ud_dirfd_pwd);
  return 0;
}
