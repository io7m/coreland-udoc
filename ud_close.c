#include <corelib/close.h>
#include <corelib/error.h>
#include <corelib/open.h>
#include <corelib/str.h>

#define UDOC_IMPLEMENTATION
#include "udoc.h"

int
ud_close(struct udoc *ud)
{
  if (ud->ud_dirfd_pwd != -1)
    if (close(ud->ud_dirfd_pwd) == -1) return 0;
  if (ud->ud_dirfd_src != -1)
    if (close(ud->ud_dirfd_src) == -1) return 0;
  if (ud->ud_dirfd_out != -1)
    if (close(ud->ud_dirfd_out) == -1) return 0;

  if (token_close(&ud->ud_tok) == -1) return 0;
  return 1;
}
