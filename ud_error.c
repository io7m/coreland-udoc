#include <corelib/alloc.h>
#include <corelib/dstack.h>
#include <corelib/hashtable.h>
#include <corelib/error.h>
#include <corelib/sstring.h>
#include <corelib/str.h>

#include "log.h"

#define UDOC_IMPLEMENTATION
#include "udoc.h"
#include "ud_error.h"

void
ud_error_display(struct udoc *ud, const struct ud_error *ue)
{
  if (ue->ue_extra.len) {
    if (ue->ue_errno_val)
      log_7x(LOG_ERROR, ue->ue_doc->ud_name, ": ", ue->ue_func, ": ", ue->ue_op.s, ": ", ue->ue_extra.s);
    else
      log_7sys(LOG_ERROR, ue->ue_doc->ud_name, ": ", ue->ue_func, ": ", ue->ue_op.s, ": ", ue->ue_extra.s);
  } else {
    if (ue->ue_errno_val)
      log_5x(LOG_ERROR, ue->ue_doc->ud_name, ": ", ue->ue_func, ": ", ue->ue_op.s);
    else
      log_5sys(LOG_ERROR, ue->ue_doc->ud_name, ": ", ue->ue_func, ": ", ue->ue_op.s);
  }
}

void
ud_error_fill(struct udoc *ud, struct ud_error *ue, const char *func,
  const char *op, const char *extra, int ev)
{
  ue->ue_doc = ud;
  ue->ue_func = func;
  ue->ue_errno_val = ev;

  sstring_init(&ue->ue_op, ue->ue_op_buf, sizeof(ue->ue_op_buf));
  sstring_init(&ue->ue_extra, ue->ue_extra_buf, sizeof(ue->ue_extra_buf));

  sstring_cats(&ue->ue_op, op); sstring_0(&ue->ue_op);
  if (extra) { sstring_cats(&ue->ue_extra, extra); sstring_0(&ue->ue_extra); }
}

void
ud_error_push(struct udoc *ud, const struct ud_error *ue)
{
  if (!dstack_push(&ud->ud_main_doc->ud_errors, &ue)) {
    log_1sys(LOG_ERROR, "could not push error onto stack. displaying immediately");
    ud_error_display(ud, ue);
  }
}

void
ud_try_fail(struct udoc *ud, const char *func, const char *op, const char *extra, int ev)
{
  struct ud_error ue;
  ud_error_fill(ud, &ue, func, op, extra, ev);
  ud_error_push(ud, &ue);
}

int
ud_error_pop(struct dstack *ds, struct ud_error **ue)
{
  return dstack_pop(ds, (void *) ue);
}

unsigned long
ud_error_size(const struct dstack *ds)
{
  return dstack_SIZE(ds);
}

