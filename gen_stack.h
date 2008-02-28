#ifndef GEN_STACK_H
#define GEN_STACK_H

#include <corelib/dstack.h>

/* type */
#define GEN_stack_type_declare(TYPE) \
struct TYPE ## _stack {              \
  struct dstack base;                \
  TYPE *ptr;                         \
}
#define GEN_stack_type_declare_wstruct(TYPE) \
struct TYPE ## _stack {                      \
  struct dstack base;                        \
  struct TYPE *ptr;                          \
}

/* stack_init() */
#define GEN_stack_init_declare(TYPE) \
int TYPE ## _stack_init(struct TYPE ## _stack *, unsigned long)
#define GEN_stack_init_define(TYPE) \
int TYPE ## _stack_init(struct TYPE ## _stack *a, unsigned long len) \
{                                                                    \
  if (!dstack_init(&a->base, len, sizeof(TYPE))) return 0;           \
  a->ptr = (TYPE *) dstack_data(&a->base);                           \
  return 1;                                                          \
}
#define GEN_stack_init_define_wstruct(TYPE) \
int TYPE ## _stack_init(struct TYPE ## _stack *a, unsigned long len) \
{                                                                    \
  if (!dstack_init(&a->base, len, sizeof(struct TYPE))) return 0;    \
  a->ptr = (struct TYPE *) dstack_data(&a->base);                    \
}

/* stack_free() */
#define GEN_stack_free_declare(TYPE) \
void TYPE ## _stack_free(struct TYPE ## _stack *)
#define GEN_stack_free_define(TYPE) \
void TYPE ## _stack_free(struct TYPE ## _stack *a) \
{                                                  \
  dstack_free(&a->base);                           \
  a->ptr = 0;                                      \
}

/* stack_size() */
#define GEN_stack_size_declare(TYPE) \
unsigned long TYPE ## _stack_size(struct TYPE ## _stack *)
#define GEN_stack_size_define(TYPE) \
unsigned long TYPE ## _stack_size(struct TYPE ## _stack *a) \
{                                                           \
  return dstack_size(&a->base);                             \
}

/* stack_bytes() */
#define GEN_stack_bytes_declare(TYPE) \
unsigned long TYPE ## _stack_bytes(struct TYPE ## _stack *)
#define GEN_stack_bytes_define(TYPE) \
unsigned long TYPE ## _stack_bytes(struct TYPE ## _stack *a) \
{                                                            \
  return dstack_bytes(&a->base);                             \
}

/* stack_push() */
#define GEN_stack_push_declare(TYPE) \
int TYPE ## _stack_push(struct TYPE ## _stack *, const TYPE *)
#define GEN_stack_push_declare_wstruct(TYPE) \
int TYPE ## _stack_push(struct TYPE ## _stack *, const struct TYPE *)
#define GEN_stack_push_define(TYPE) \
int TYPE ## _stack_push(struct TYPE ## _stack *a, const TYPE *x) \
{                                                                \
  if (!dstack_push(&a->base, (void *) x)) return 0;              \
  a->ptr = (TYPE *) dstack_data(&a->base);                       \
  return 1;                                                      \
}
#define GEN_stack_push_define_wstruct(TYPE) \
int TYPE ## _stack_push(struct TYPE ## _stack *a, const struct TYPE *x) \
{                                                                       \
  if (!dstack_push(&a->base, (void *) x)) return 0;                     \
  a->ptr = (struct TYPE *) dstack_data(&a->base);                       \
  return 1;                                                             \
}

/* stack_pop() */
#define GEN_stack_pop_declare(TYPE) \
int TYPE ## _stack_pop(struct TYPE ## _stack *, TYPE **)
#define GEN_stack_pop_declare_wstruct(TYPE) \
int TYPE ## _stack_pop(struct TYPE ## _stack *, struct TYPE **)
#define GEN_stack_pop_define(TYPE) \
int TYPE ## _stack_pop(struct TYPE ## _stack *a, TYPE **x) \
{                                                                 \
  return (dstack_pop(&a->base, (void *) x));                      \
}
#define GEN_stack_pop_define_wstruct(TYPE) \
int TYPE ## _stack_pop(struct TYPE ## _stack *a, struct TYPE **x) \
{                                                                 \
  return (dstack_pop(&a->base, (void *) x));                      \
}

/* stack_peek() */
#define GEN_stack_peek_declare(TYPE) \
int TYPE ## _stack_peek(struct TYPE ## _stack *, TYPE **)
#define GEN_stack_peek_declare_wstruct(TYPE) \
int TYPE ## _stack_peek(struct TYPE ## _stack *, struct TYPE **)
#define GEN_stack_peek_define(TYPE) \
int TYPE ## _stack_peek(struct TYPE ## _stack *a, TYPE **x) \
{                                                                  \
  return (dstack_peek(&a->base, (void *) x));                      \
}
#define GEN_stack_peek_define_wstruct(TYPE) \
int TYPE ## _stack_peek(struct TYPE ## _stack *a, struct TYPE **x) \
{                                                                  \
  return (dstack_peek(&a->base, (void *) x));                      \
}

/* stack_data() */
#define GEN_stack_data_declare(TYPE) \
const TYPE *TYPE ## _stack_data(const struct TYPE ## _stack *)
#define GEN_stack_data_declare_wstruct(TYPE) \
const struct TYPE *TYPE ## _stack_data(const struct TYPE ## _stack *)
#define GEN_stack_data_define(TYPE) \
const TYPE *TYPE ## _stack_data(const struct TYPE ## _stack *a) \
{                                                               \
  return a->ptr;                                                \
}
#define GEN_stack_data_define_wstruct(TYPE) \
const struct TYPE *TYPE ## _stack_data(const struct TYPE ## _stack *a) \
{                                                                      \
  return a->ptr;                                                       \
}

/* global */
#define GEN_stack_declare(TYPE) \
GEN_stack_type_declare(TYPE); \
GEN_stack_init_declare(TYPE); \
GEN_stack_free_declare(TYPE); \
GEN_stack_push_declare(TYPE); \
GEN_stack_pop_declare(TYPE); \
GEN_stack_peek_declare(TYPE); \
GEN_stack_size_declare(TYPE); \
GEN_stack_bytes_declare(TYPE); \
GEN_stack_data_declare(TYPE)

#define GEN_stack_define(TYPE) \
GEN_stack_init_define(TYPE) \
GEN_stack_free_define(TYPE) \
GEN_stack_push_define(TYPE) \
GEN_stack_pop_define(TYPE) \
GEN_stack_peek_define(TYPE) \
GEN_stack_size_define(TYPE) \
GEN_stack_bytes_define(TYPE) \
GEN_stack_data_define(TYPE) \
static const int GEN_ ## TYPE ## _stack_defined = 1

#define GEN_stack_declare_wstruct(TYPE) \
GEN_stack_type_declare_wstruct(TYPE); \
GEN_stack_init_declare_wstruct(TYPE); \
GEN_stack_free_declare(TYPE); \
GEN_stack_push_declare_wstruct(TYPE); \
GEN_stack_pop_declare_wstruct(TYPE); \
GEN_stack_peek_declare_wstruct(TYPE); \
GEN_stack_size_declare(TYPE); \
GEN_stack_bytes_declare(TYPE); \
GEN_stack_data_declare_wstruct(TYPE)

#define GEN_stack_define_wstruct(TYPE) \
GEN_stack_init_define_wstruct(TYPE) \
GEN_stack_free_define(TYPE) \
GEN_stack_push_define_wstruct(TYPE) \
GEN_stack_pop_define_wstruct(TYPE) \
GEN_stack_peek_define_wstruct(TYPE) \
GEN_stack_size_define(TYPE) \
GEN_stack_bytes_define(TYPE) \
GEN_stack_data_define_wstruct(TYPE) \
static const int GEN_ ## TYPE ## _stack_defined_wstruct = 1

#endif
