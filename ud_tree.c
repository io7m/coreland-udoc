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
  const struct ud_node_list *cur_list = ctx->state->list;
  const struct ud_node *cur_node = cur_list->head;
  struct ud_tree_ctx_state *state = ctx->state;
  struct ud_tag_stack *tag_stack = &state->tag_stack;
  unsigned long saved_ssize = 0;
  unsigned long saved_pos = 0;
  enum ud_tree_walk_stat ret = UD_TREE_OK;
  enum ud_tag tag;

  saved_ssize = ud_tag_stack_size(tag_stack);
  saved_pos = state->list_pos;
  state->list_pos = 0;

  ns[fmt_ulong(ns, state->list_depth)] = 0;
  log_4xf(LOG_DEBUG, "processing ", doc->name, " depth ", ns);

  state->list_depth++;
  for (;;) {
    state->node = cur_node;
    ud_assert_s(cur_node->type <= UDOC_TYPE_INCLUDE, "unknown node type");
    switch (cur_node->type) {
      case UDOC_TYPE_SYMBOL:
        log_2xf(LOG_DEBUG, "symbol: ", cur_node->data.sym);
        if (state->list_pos == 0) {
          ud_tag_by_name(cur_node->data.sym, &tag);
          if (!ud_tag_stack_push(tag_stack, tag)) goto FAIL;
          ud_assert(ud_tag_stack_size(tag_stack) - 1 == saved_ssize);
        }
        if (ctx->funcs->symbol) {
          ret = ctx->funcs->symbol(doc, ctx);
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
        state->list = &cur_node->data.list;
        state->node = cur_node->data.list.head;
        if (ctx->funcs->list) {
          ret = ctx->funcs->list(doc, ctx);
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
        state->list = cur_list;
        state->node = cur_node;
        break;
      case UDOC_TYPE_STRING:
        log_3xf(LOG_DEBUG, "string: \"", cur_node->data.str, "\"");
        if (ctx->funcs->string) {
          ret = ctx->funcs->string(doc, ctx);
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
        state->list = &cur_node->data.list;
        state->node = cur_node->data.list.head;
        if (ctx->funcs->include) {
          ret = ctx->funcs->include(doc, ctx);
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
        state->list = cur_list;
        state->node = cur_node;
        break;
      default:
        break;
    }
    if (cur_node->next) cur_node = cur_node->next; else break;
    state->list_pos++;
  }

  END:
  state->list_depth--;
  log_1xf(LOG_DEBUG, "list: end");
  if (ret == UD_TREE_OK) {
    if (ctx->funcs->list_end) {
      ret = ctx->funcs->list_end(doc, ctx);
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
  state->list_pos = saved_pos;
  log_2xf(LOG_DEBUG, "success depth ", ns);
  return ret;

FAIL:
  if (ctx->funcs->error) ctx->funcs->error(doc, ctx);
  state->list_pos = saved_pos;
  if (errno)
    log_2sysf(LOG_DEBUG, "fail depth ", ns);
  else
    log_2xf(LOG_DEBUG, "fail depth ", ns);
  return ret;
}

int
ud_tree_walk(struct udoc *doc, struct ud_tree_ctx *ctx)
{
  if (!ud_tag_stack_init(&ctx->state->tag_stack)) return 0;
  if (ctx->funcs->init)
    switch (ctx->funcs->init(doc, ctx)) {
      case UD_TREE_FAIL: goto FAIL;
      case UD_TREE_OK: break;
      case UD_TREE_STOP:
      case UD_TREE_STOP_LIST: goto END;
      default: ud_assert_s(0, "unknown return value"); goto FAIL;
    }
  if (doc->nodes) {
    switch (ud_walk(doc, ctx)) {
      case UD_TREE_FAIL: goto FAIL;
      case UD_TREE_OK: break;
      case UD_TREE_STOP:
      case UD_TREE_STOP_LIST: goto END;
      default: ud_assert_s(0, "unknown return value"); goto FAIL;
    }
  }
  if (ctx->funcs->finish)
    switch (ctx->funcs->finish(doc, ctx)) {
      case UD_TREE_FAIL: goto FAIL;
      case UD_TREE_OK: break;
      case UD_TREE_STOP:
      case UD_TREE_STOP_LIST: goto END;
      default: ud_assert_s(0, "unknown return value"); goto FAIL;
    }

  END:
  ud_assert(ud_tag_stack_size(&ctx->state->tag_stack) == 0);
  ud_tag_stack_free(&ctx->state->tag_stack);
  return 1;

  FAIL:
  ud_tag_stack_free(&ctx->state->tag_stack);
  return 0;
}
