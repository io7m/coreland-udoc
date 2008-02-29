#include <corelib/array.h>
#include <corelib/close.h>
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
  struct hashtable *doc_hash = &ud->ud_main_doc->ud_documents;

  ud->ud_name = fn;
  ud_try_sys_jump(ud, dir_name_r(ud->ud_name, dir, sizeof(dir)), OPEN_FAIL, "dir_name_r");

  ud->ud_dirfd_src = open_ro(dir);
  ud_try_sys_jump(ud, ud->ud_dirfd_src != -1, OPEN_FAIL, dir);
  ud_try_sys_jump(ud, token_open(&ud->ud_tok, fn), OPEN_FAIL, fn);
  ud_try_sys_jump(ud, ht_addb(doc_hash, fn, str_len(fn), ud,
                      sizeof(struct udoc)) >= 0, OPEN_FAIL, "ht_addb");
  return 1;

  OPEN_FAIL:
  close(ud->ud_dirfd_src);
  token_close(&ud->ud_tok);
  return 0;
}

int
ud_get(const struct udoc *ud, const char *fn, struct udoc **dp)
{
  struct udoc *u;
  struct hashtable *doc_hash = &ud->ud_main_doc->ud_documents;
  unsigned long sz;

  switch (ht_getb(doc_hash, fn, str_len(fn), (void *) &u, &sz)) {
    case 0: return 0;
    default: *dp = u; return 1;
  }
}
