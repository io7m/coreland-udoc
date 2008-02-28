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
#include "ud_tag.h"
#include "udoc.h"

static void 
syntax(struct udoc *doc, const char *s)
{
  char buf[256];
  char cnum[FMT_ULONG];
  struct sstring sstr = sstring_INIT(buf);

  cnum[fmt_ulong(cnum, doc->tok.line)] = 0;
  sstring_cats4(&sstr, "syntax: ", cnum, ": ", s);
  sstring_0(&sstr);

  ud_error_push(&ud_errors, doc->name, sstr.s);
}

static void 
ud_tree_reduce(struct udoc *doc, struct ud_node_list *list)
{
  struct ud_node *un;

  if (list->size == 1) {
    un = list->head;
    if (un->type == UDOC_TYPE_LIST) {
      log_1xf(LOG_DEBUG, "reduced list");
      list->head = un->data.list.head;
      dealloc_null((void *) &un);
    }
  }
}

static int 
ud_tree_include(struct udoc *doc, char *file, struct ud_node_list *list)
{
  struct udoc ud_new;
  struct udoc *udp;
  struct ud_node cur = {0, {0}, 0, 0};

  if (list->size) {
    syntax(doc, "the include symbol can only appear at the start of a list");
    goto FAIL;
  }
  if (ud_get(file, &udp)) {
    log_2xf(LOG_INFO, "include reused: ", file);
  } else {
    if (!ud_init(&ud_new)) goto FAIL;
    if (!ud_open(&ud_new, file)) goto FAIL;
    if (!ud_get(file, &udp)) goto FAIL;
    if (!ud_parse(udp)) goto FAIL;
  }
  if (!udp->nodes) {
    log_2xf(LOG_DEBUG, file, " is empty, ignoring");
    return 1;
  }
  cur.line_num = doc->tok.line;
  cur.type = UDOC_TYPE_INCLUDE;
  cur.data.list = udp->tree.root;
  if (!ud_list_cat(list, &cur)) goto FAIL;
  ++doc->nodes;

  return 1;
FAIL:
  return 0;
}

static int 
ud_tree_symbol(struct udoc *doc, char *symbol, struct ud_node_list *list)
{
  struct ud_node cur = {0, {0}, 0, 0};
  enum token_type param_type;
  char *param_token;

  if (!str_same(symbol, "include")) {
    cur.type = UDOC_TYPE_SYMBOL;
    cur.line_num = doc->tok.line;
    if (!str_dup(symbol, &cur.data.sym)) goto FAIL;
    if (!ud_list_cat(list, &cur)) goto FAIL;
    ++doc->nodes;
  } else {
    if (!token_next(&doc->tok, &param_token, &param_type)) {
      if (!errno) syntax(doc, "malformed document (premature end of file)");
      goto FAIL;
    }
    if (param_type != TOKEN_TYPE_STRING) {
      syntax(doc, "argument to include is not a string");
      goto FAIL;
    }
    if (!ud_tree_include(doc, param_token, list)) goto FAIL;
  }

  return 1;
FAIL:
  return 0;
}

static int 
ud_tree_string(struct udoc *doc, const char *str, struct ud_node_list *list)
{
  struct ud_node cur = {0, {0}, 0, 0};

  if (!list->size) {
    log_1xf(LOG_DEBUG, "inserting implicit para");
    cur.type = UDOC_TYPE_SYMBOL;
    cur.line_num = doc->tok.line;
    if (!str_dup("para", &cur.data.sym)) goto FAIL;
    if (!ud_list_cat(list, &cur)) goto FAIL;
    ++doc->nodes;
  }
  cur.type = UDOC_TYPE_STRING;
  cur.line_num = doc->tok.line;
  if (!str_dup(str, &cur.data.str)) goto FAIL;
  if (!ud_list_cat(list, &cur)) goto FAIL;
  ++doc->nodes;

  return 1;
FAIL:
  return 0;
}

static int 
ud_tree_build(struct udoc *doc, struct ud_node_list *parent)
{
  struct ud_node cur = {0, {0}, 0, 0};
  struct ud_node_list cur_list = {0, 0, 0};
  enum token_type type;
  char cnum[FMT_ULONG];
  char *token;

  cnum[fmt_ulong(cnum, doc->depth)] = 0;
  log_4xf(LOG_DEBUG, "processing ", doc->name, " depth ", cnum);

  for (;;) {
    errno = 0;
    bin_zero(&cur, sizeof(cur));
    if (!token_next(&doc->tok, &token, &type)) {
      if (!errno) syntax(doc, "malformed document (premature end of file)");
      goto FAIL;
    }
    switch (type) {
    case TOKEN_TYPE_SYMBOL:
      log_2xf(LOG_DEBUG, "symbol: ", token);
      if (!ud_tree_symbol(doc, token, &cur_list)) goto FAIL;
      break;
    case TOKEN_TYPE_STRING:
      log_2xf(LOG_DEBUG, "string: ", token);
      if (!ud_tree_string(doc, token, &cur_list)) goto FAIL;
      break;
    case TOKEN_TYPE_PAREN_OPEN:
      log_1xf(LOG_DEBUG, "paren open");
      ++doc->depth;
      cur.type = UDOC_TYPE_LIST;
      if (!ud_tree_build(doc, &cur.data.list)) goto FAIL;
      if (cur.data.list.size) {
        if (!ud_list_cat(&cur_list, &cur)) goto FAIL;
        ++doc->nodes;
      }
      break;
    case TOKEN_TYPE_PAREN_CLOSE:
      log_1xf(LOG_DEBUG, "paren close");
      if (!doc->depth) {
        syntax(doc, "unbalanced parenthesis");
        goto FAIL;
      }
      --doc->depth;
      if (!cur_list.size) goto EMPTY;
      ud_tree_reduce(doc, &cur_list);
      goto SUCCESS;
    case TOKEN_TYPE_EOF:
      log_1xf(LOG_DEBUG, "eof");
      if (doc->depth) {
        syntax(doc, "unbalanced parenthesis");
        goto FAIL;
      }
      goto SUCCESS;
    default:
      syntax(doc, "unknown token type");
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
ud_parse(struct udoc *doc)
{
  if (fchdir(doc->dirfd_src) == -1) return 0;
  if (!ud_tree_build(doc, &doc->tree.root)) {
    fchdir(doc->dirfd_pwd);
    return 0;
  }
  return (fchdir(doc->dirfd_src) != -1);
}
