#ifndef UD_ERROR_H
#define UD_ERROR_H

#include <corelib/sstring.h>

#define UD_ERROR_DOCNAME_MAX 256
#define UD_ERROR_OP_MAX 32
#define UD_ERROR_EXTRA_MAX 256

struct ud_error {
  struct udoc *ue_doc;
  const char *ue_func;
  char ue_op_buf[UD_ERROR_OP_MAX];
  char ue_extra_buf[UD_ERROR_EXTRA_MAX];
  struct sstring ue_op;
  struct sstring ue_extra;
  int ue_errno_val;
};

void ud_error_display(struct udoc *, const struct ud_error *);
void ud_error_fill(struct udoc *, struct ud_error *, const char *, const char *, const char *, int);
void ud_error_push(struct udoc *, const struct ud_error *);
void ud_try_fail(struct udoc *, const char *, const char *, const char *, int);

int ud_error_pop(struct dstack *, struct ud_error **);
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

#endif
