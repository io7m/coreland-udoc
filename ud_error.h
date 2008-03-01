#ifndef UD_ERROR_H
#define UD_ERROR_H

#include <corelib/dstack.h>

#define UD_ERROR_DOCNAME_MAX 256
#define UD_ERROR_OP_MAX 32
#define UD_ERROR_EXTRA_MAX 256

struct udoc;
struct ud_err {
  const char *ue_func;
  char ue_doc[UD_ERROR_DOCNAME_MAX];
  char ue_op[UD_ERROR_OP_MAX];
  char ue_extra[UD_ERROR_EXTRA_MAX];
  int ue_errno_val;
  int ue_used_extra;
};

void ud_error_display(struct udoc *, const struct ud_err *);
void ud_error_fill(struct udoc *, struct ud_err *, const char *, const char *, const char *, int);
void ud_error_push(struct udoc *, const struct ud_err *);
void ud_try_fail(struct udoc *, const char *, const char *, const char *, int);

int ud_error_pop(struct dstack *, struct ud_err **);
unsigned long ud_error_size(const struct dstack *);

#define ud_try_ret(ud, eval, retval, op, extra, errval) \
if (!(eval)) { ud_try_fail((ud), __func__, (op), (extra), (errval)); return (retval); }

#define ud_try_goto(ud, eval, label, op, extra, errval) \
if (!(eval)) { ud_try_fail((ud), __func__, (op), (extra), (errval)); goto label; }

#define ud_try_sysS(ud, eval, retval, name, extra) \
ud_try_ret((ud),(eval),(retval),(name),(extra),errno)
#define ud_try_sys(ud, eval, retval, name) \
ud_try_ret((ud),(eval),(retval),(name),0,errno)
#define ud_tryS(ud, eval, retval, name, extra) \
ud_try_ret((ud),(eval),(retval),(name),(extra),0)
#define ud_try(ud, eval, retval, name) \
ud_try_ret((ud),(eval),(retval),(name),0,0)

#define ud_try_sys_jumpS(ud, eval, label, name, extra) \
ud_try_goto((ud),(eval),label,(name),(extra),errno)
#define ud_try_sys_jump(ud, eval, label, name) \
ud_try_goto((ud),(eval),label,(name),0,errno)
#define ud_try_jumpS(ud, eval, label, name, extra) \
ud_try_goto((ud),(eval),label,(name),(extra),0)
#define ud_try_jump(ud, eval, label, name) \
ud_try_goto((ud),(eval),label,(name),0,0)

#define ud_error(ud, str) \
do {                                                 \
  struct ud_err ue_tmp;                              \
  ud_error_fill(ud, &ue_tmp, __func__, (str), 0, 0); \
  ud_error_push(ud, &ue_tmp);                        \
} while (0);

#define ud_error_extra(ud, str1, str2) \
do {                                                       \
  struct ud_err ue_tmp;                                    \
  ud_error_fill(ud, &ue_tmp, __func__, (str1), (str2), 0); \
  ud_error_push(ud, &ue_tmp);                              \
} while (0);

#define ud_error_sys(ud, str) \
do {                                                     \
  struct ud_err ue_tmp;                                  \
  ud_error_fill(ud, &ue_tmp, __func__, (str), 0, errno); \
  ud_error_push(ud, &ue_tmp);                            \
} while (0);

#endif
