#include <corelib/alloc.h>
#include <corelib/bin.h>
#include <corelib/dstack.h>
#include <corelib/hashtable.h>
#include <corelib/error.h>
#include <corelib/sstring.h>
#include <corelib/str.h>

#include "log.h"

#define UDOC_IMPLEMENTATION
#include "udoc.h"
#include "ud_assert.h"
#include "ud_error.h"

void
ud_error_display(struct udoc *ud, const struct ud_err *ue)
{
  if (ue->ue_used_extra) {
    if (ue->ue_errno_val)
      log_7sys(LOG_ERROR, ue->ue_doc, ": ", ue->ue_func, ": ", ue->ue_op, ": ", ue->ue_extra);
    else
      log_7x(LOG_ERROR, ue->ue_doc, ": ", ue->ue_func, ": ", ue->ue_op, ": ", ue->ue_extra);
  } else {
    if (ue->ue_errno_val)
      log_5sys(LOG_ERROR, ue->ue_doc, ": ", ue->ue_func, ": ", ue->ue_op);
    else
      log_5x(LOG_ERROR, ue->ue_doc, ": ", ue->ue_func, ": ", ue->ue_op);
  }
}

void
ud_error_fill(struct udoc *ud, struct ud_err *ue, const char *func,
  const char *op, const char *extra, int ev)
{
  struct sstring sstr_dn = sstring_INIT(ue->ue_doc);
  struct sstring sstr_op = sstring_INIT(ue->ue_op);
  struct sstring sstr_ex = sstring_INIT(ue->ue_extra);

  bin_zero(ue, sizeof(*ue));

  ue->ue_func = func;
  ue->ue_errno_val = ev;

  ud_assert (ud->ud_cur_doc->ud_name != 0);

  sstring_cpys(&sstr_dn, ud->ud_cur_doc->ud_name);
  sstring_0(&sstr_dn);

  sstring_cpys(&sstr_op, op);
  sstring_0(&sstr_op);

  if (extra) {
    sstring_cpys(&sstr_ex, extra);
    sstring_0(&sstr_ex);
    ue->ue_used_extra = 1;
  }
}

void
ud_error_push(struct udoc *ud, const struct ud_err *ue)
{
  if (!ud->ud_main_doc) goto FAIL;
  if (!dstack_push(&ud->ud_main_doc->ud_errors, (struct ud_err *) ue)) goto FAIL;
  return;

  FAIL:
  log_1sys(LOG_ERROR, "could not push error onto stack. displaying immediately");
  ud_error_display(ud, ue);
}

void
ud_try_fail(struct udoc *ud, const char *func, const char *op, const char *extra, int ev)
{
  struct ud_err ue;
  ud_error_fill(ud, &ue, func, op, extra, ev);
  ud_error_push(ud, &ue);
}

int
ud_error_pop(struct dstack *ds, struct ud_err **ue)
{
  return dstack_pop(ds, (void *) ue);
}

unsigned long
ud_error_size(const struct dstack *ds)
{
  return dstack_SIZE(ds);
}

