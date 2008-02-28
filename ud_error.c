#include <corelib/alloc.h>
#include <corelib/dstack.h>
#include <corelib/hashtable.h>
#include <corelib/error.h>
#include <corelib/str.h>

#include "log.h"

#define UDOC_IMPLEMENTATION
#include "ud_error.h"

struct dstack ud_errors;

static int
ud_error_fail(const char *file, const char *error)
{
  log_1sys(LOG_ERROR, "could not push error onto stack. displaying immediately");
  log_3x(LOG_ERROR, file, ": ", error);
  return 0;
}

int
ud_error_init(struct dstack *ds)
{
  return dstack_init(ds, 8, sizeof(struct ud_error));
}

int
ud_error_push(struct dstack *ds, const char *file, const char *error)
{
  struct ud_error ue = {0, 0};
  if (!str_dup(file, &ue.file)) goto FAIL;
  if (!str_dup(error, &ue.error)) goto FAIL;
  if (!dstack_push(ds, &ue)) goto FAIL;
  return 1;
  FAIL:
  ud_error_free(&ue);
  return ud_error_fail(file, error);
}

int
ud_error_pop(struct dstack *ds, struct ud_error *ue)
{
  struct ud_error *up;
  if (dstack_pop(ds, (void *) &up)) {
    *ue = *up;
    return 1;
  }
  return 0;
}

unsigned long
ud_error_size(struct dstack *ds)
{
  return dstack_SIZE(ds);
}

void
ud_error_free(struct ud_error *ue)
{
  dealloc_null((void *) &ue->file);
  dealloc_null((void *) &ue->error);
}
