#include <corelib/array.h>
#include <corelib/error.h>
#include <corelib/fmt.h>

#include "log.h"

#define UDOC_IMPLEMENTATION
#include "ud_assert.h"
#include "ud_tag.h"
#include "ud_tree.h"
#include "udoc.h"

static enum ud_tree_walk_stat
ud_walk(struct udoc *doc, struct ud_tree_ctx *ctx)
{
  char ns[FMT_ULONG];
  const struct ud_node_list *cur_list = ctx->utc_state->utc_list;
  const struct ud_node *cur_node = cur_list->unl_head;
  struct ud_tree_ctx_state *state = ctx->utc_state;
  struct ud_tag_stack *tag_stack = &state->utc_tag_stack;
  unsigned long saved_ssize = 0;
  unsigned long saved_pos = 0;
  enum ud_tree_walk_stat ret = UD_TREE_OK;
  enum ud_tag tag;

  saved_ssize = ud_tag_stack_size(tag_stack);
  saved_pos = state->utc_list_pos;
  state->utc_list_pos = 0;

  ns[fmt_ulong(ns, state->utc_list_depth)] = 0;
  log_4xf(LOG_DEBUG, "processing ", doc->ud_name, " depth ", ns);

  state->utc_list_depth++;
  for (;;) {
    state->utc_node = cur_node;
    ud_assert_s(cur_node->un_type <= UDOC_TYPE_INCLUDE, "unknown node type");
    switch (cur_node->un_type) {
      case UDOC_TYPE_SYMBOL:
        log_2xf(LOG_DEBUG, "symbol: ", cur_node->un_data.un_sym);
        if (state->utc_list_pos == 0) {
          ud_tag_by_name(cur_node->un_data.un_sym, &tag);
          if (!ud_tag_stack_push(tag_stack, tag)) goto FAIL;
          ud_assert(ud_tag_stack_size(tag_stack) - 1 == saved_ssize);
        }
        if (ctx->utc_funcs->utcf_symbol) {
          ret = ctx->utc_funcs->utcf_symbol(doc, ctx);
          ud_assert_s(ret <= UD_TREE_STOP_LIST, "symbol: unknown return value");
          switch (ret) {
            case UD_TREE_FAIL: goto FAIL;
            case UD_TREE_OK: break;
            case UD_TREE_STOP: log_1xf(LOG_DEBUG, "list: stop"); goto END;
            case UD_TREE_STOP_LIST:
              log_1xf(LOG_DEBUG, "list: stop_list"); ret = UD_TREE_OK; goto END;
            default: break;
          }
        }
        break;
      case UDOC_TYPE_LIST:
        log_1xf(LOG_DEBUG, "list");
        state->utc_list = &cur_node->un_data.un_list;
        state->utc_node = cur_node->un_data.un_list.unl_head;
        if (ctx->utc_funcs->utcf_list) {
          ret = ctx->utc_funcs->utcf_list(doc, ctx);
          ud_assert_s(ret <= UD_TREE_STOP_LIST, "list: unknown return value");
          switch (ret) {
            case UD_TREE_FAIL: goto FAIL;
            case UD_TREE_OK: break;
            case UD_TREE_STOP: log_1xf(LOG_DEBUG, "list: stop"); goto END;
            case UD_TREE_STOP_LIST:
              log_1xf(LOG_DEBUG, "list: stop_list"); goto END;
            default: break;
          }
        }
        ret = ud_walk(doc, ctx);
        ud_assert_s(ret <= UD_TREE_STOP_LIST, "list: unknown return value");
        switch (ret) {
          case UD_TREE_FAIL: goto FAIL;
          case UD_TREE_OK: break;
          case UD_TREE_STOP: log_1xf(LOG_DEBUG, "list: stop"); goto END;
          case UD_TREE_STOP_LIST:
            log_1xf(LOG_DEBUG, "list: stop_list"); goto END;
          default: break;
        }
        state->utc_list = cur_list;
        state->utc_node = cur_node;
        break;
      case UDOC_TYPE_STRING:
        log_3xf(LOG_DEBUG, "string: \"", cur_node->un_data.un_str, "\"");
        if (ctx->utc_funcs->utcf_string) {
          ret = ctx->utc_funcs->utcf_string(doc, ctx);
          ud_assert_s(ret <= UD_TREE_STOP_LIST, "string: unknown return value");
          switch (ret) {
            case UD_TREE_FAIL: goto FAIL;
            case UD_TREE_OK: break;
            case UD_TREE_STOP: log_1xf(LOG_DEBUG, "list: stop"); goto END;
            case UD_TREE_STOP_LIST:
              log_1xf(LOG_DEBUG, "list: stop_list"); goto END;
            default: break;
          }
        }
        break;
      case UDOC_TYPE_INCLUDE: 
        log_1xf(LOG_DEBUG, "include");
        state->utc_list = &cur_node->un_data.un_list;
        state->utc_node = cur_node->un_data.un_list.unl_head;
        if (ctx->utc_funcs->utcf_include) {
          ret = ctx->utc_funcs->utcf_include(doc, ctx);
          ud_assert_s(ret <= UD_TREE_STOP_LIST, "unknown return value");
          switch (ret) {
            case UD_TREE_FAIL: goto FAIL;
            case UD_TREE_OK: break;
            case UD_TREE_STOP: log_1xf(LOG_DEBUG, "list: stop"); goto END;
            case UD_TREE_STOP_LIST:
              log_1xf(LOG_DEBUG, "list: stop_list"); goto END;
            default: break;
          }
        }
        ret = ud_walk(doc, ctx);
        ud_assert_s(ret <= UD_TREE_STOP_LIST, "include: unknown return value");
        switch (ret) {
          case UD_TREE_FAIL: goto FAIL;
          case UD_TREE_OK: break;
          case UD_TREE_STOP: log_1xf(LOG_DEBUG, "list: stop"); goto END;
          case UD_TREE_STOP_LIST:
            log_1xf(LOG_DEBUG, "list: stop_list"); goto END;
          default: break;
        }
        state->utc_list = cur_list;
        state->utc_node = cur_node;
        break;
      default:
        break;
    }
    if (cur_node->un_next) cur_node = cur_node->un_next; else break;
    state->utc_list_pos++;
  }

  END:
  state->utc_list_depth--;
  log_1xf(LOG_DEBUG, "list: end");
  if (ret == UD_TREE_OK) {
    if (ctx->utc_funcs->utcf_list_end) {
      ret = ctx->utc_funcs->utcf_list_end(doc, ctx);
      ud_assert_s(ret <= UD_TREE_STOP_LIST, "list_end: unknown return value");
      switch (ret) {
        case UD_TREE_FAIL: goto FAIL;
        case UD_TREE_OK: break;
        case UD_TREE_STOP: log_1xf(LOG_DEBUG, "list: stop"); goto END;
        case UD_TREE_STOP_LIST:
          log_1xf(LOG_DEBUG, "list: stop_list"); ret = UD_TREE_OK; goto END;
        default: break;
      }
    }
  }

  if (saved_ssize < ud_tag_stack_size(tag_stack))
    ud_assert_s(ud_tag_stack_pop(tag_stack, &tag), "tag stack underflow");
  ud_assert(saved_ssize == ud_tag_stack_size(tag_stack));

  if (ret == UD_TREE_STOP_LIST) ret = UD_TREE_OK;
  state->utc_list_pos = saved_pos;
  log_2xf(LOG_DEBUG, "success depth ", ns);
  return ret;

FAIL:
  if (ctx->utc_funcs->utcf_error) ctx->utc_funcs->utcf_error(doc, ctx);
  state->utc_list_pos = saved_pos;
  if (errno)
    log_2sysf(LOG_DEBUG, "fail depth ", ns);
  else
    log_2xf(LOG_DEBUG, "fail depth ", ns);
  return ret;
}

int
ud_tree_walk(struct udoc *doc, struct ud_tree_ctx *ctx)
{
  if (!ud_tag_stack_init(&ctx->utc_state->utc_tag_stack)) return 0;
  if (ctx->utc_funcs->utcf_init)
    switch (ctx->utc_funcs->utcf_init(doc, ctx)) {
      case UD_TREE_FAIL: goto FAIL;
      case UD_TREE_OK: break;
      case UD_TREE_STOP:
      case UD_TREE_STOP_LIST: goto END;
      default: ud_assert_s(0, "unknown return value"); goto FAIL;
    }
  if (doc->ud_nodes) {
    switch (ud_walk(doc, ctx)) {
      case UD_TREE_FAIL: goto FAIL;
      case UD_TREE_OK: break;
      case UD_TREE_STOP:
      case UD_TREE_STOP_LIST: goto END;
      default: ud_assert_s(0, "unknown return value"); goto FAIL;
    }
  }
  if (ctx->utc_funcs->utcf_finish)
    switch (ctx->utc_funcs->utcf_finish(doc, ctx)) {
      case UD_TREE_FAIL: goto FAIL;
      case UD_TREE_OK: break;
      case UD_TREE_STOP:
      case UD_TREE_STOP_LIST: goto END;
      default: ud_assert_s(0, "unknown return value"); goto FAIL;
    }

  END:
  ud_assert(ud_tag_stack_size(&ctx->utc_state->utc_tag_stack) == 0);
  ud_tag_stack_free(&ctx->utc_state->utc_tag_stack);
  return 1;

  FAIL:
  ud_tag_stack_free(&ctx->utc_state->utc_tag_stack);
  return 0;
}
