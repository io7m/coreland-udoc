#include <corelib/exit.h>
#include <corelib/fmt.h>

#include "ud_assert.h"
#include "log.h"

static void
ud_assert_die(int e)
{
  _exit(e);
}

void
ud_assert_core(const char *func, unsigned long line, const char *str)
{
  char cnum[FMT_ULONG];
  cnum[fmt_ulong(cnum, line)] = 0;
  log_5x(LOG_FATAL, func, ": ", cnum, ": assertion failed: ", str);
  ud_assert_die(112);
}

void
ud_assert_core_s(const char *func, unsigned long line,
  const char *str, const char *exs)
{
  char cnum[FMT_ULONG];
  cnum[fmt_ulong(cnum, line)] = 0;
  log_7x(LOG_FATAL, func, ": ", cnum, ": assertion failed: ", str, ": ", exs);
  ud_assert_die(112);
}
