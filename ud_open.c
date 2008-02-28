#include <corelib/array.h>
#include <corelib/dir_name.h>
#include <corelib/error.h>
#include <corelib/open.h>
#include <corelib/str.h>

#define UDOC_IMPLEMENTATION
#include "ud_tag.h"
#include "udoc.h"

int
ud_open(struct udoc *ud, const char *fn)
{
  char dir[DIR_NAME_MAX];

  ud->ud_name = fn;
  if (!dir_name_r(ud->ud_name, dir, sizeof(dir))) goto FAIL;
  ud->ud_dirfd_src = open_ro(dir);
  if (ud->ud_dirfd_src == -1) goto FAIL;

  if (!token_open(&ud->ud_tok, fn)) goto FAIL;
  switch (ht_addb(&ud_documents, fn, str_len(fn), ud, sizeof(struct udoc))) {
    case 0: break;
    case -1: goto FAIL;
    default: break;
  }
  return 1;

FAIL:
  ud_error_pushsys(&ud_errors, fn);
  return 0;
}

int
ud_get(const char *fn, struct udoc **dp)
{
  struct udoc *u;
  unsigned long sz;

  switch (ht_getb(&ud_documents, fn, str_len(fn), (void *) &u, &sz)) {
    case 0: return 0;
    default: *dp = u; return 1;
  }
}
