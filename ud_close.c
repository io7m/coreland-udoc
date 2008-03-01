#include <corelib/close.h>
#include <corelib/error.h>
#include <corelib/open.h>
#include <corelib/str.h>

#define UDOC_IMPLEMENTATION
#include "ud_error.h"
#include "udoc.h"

int
ud_close(struct udoc *ud)
{
  if (ud->ud_dirfd_pwd != -1)
    ud_try_sys_jump(ud, close(ud->ud_dirfd_pwd) != -1, FAIL, "close");
  if (ud->ud_dirfd_src != -1)
    ud_try_sys_jump(ud, close(ud->ud_dirfd_src) != -1, FAIL, "close");
  if (ud->ud_dirfd_out != -1)
    ud_try_sys_jump(ud, close(ud->ud_dirfd_out) != -1, FAIL, "close");

  ud_try_sys_jump(ud, token_close(&ud->ud_tok) != -1, FAIL, "token_close");
  return 1;
  FAIL:
  return 0;
}
