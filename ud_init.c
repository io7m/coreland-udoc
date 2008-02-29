#include <chrono/taia.h>

#include <corelib/alloc.h>
#include <corelib/bin.h>
#include <corelib/open.h>
#include <corelib/hashtable.h>

#define UDOC_IMPLEMENTATION
#include "ud_part.h"
#include "ud_oht.h"
#include "ud_ref.h"
#include "udoc.h"

int
ud_init(struct udoc *ud)
{
  bin_zero(ud, sizeof(*ud));
  if (!ud_oht_init(&ud->ud_parts, sizeof(struct ud_part))) goto FAIL;
  if (!ud_oht_init(&ud->ud_link_exts, sizeof(struct ud_ref))) goto FAIL;
  if (!ud_oht_init(&ud->ud_refs, sizeof(struct ud_ref))) goto FAIL;
  if (!ud_oht_init(&ud->ud_ref_names, sizeof(struct ud_ref))) goto FAIL;
  if (!ud_oht_init(&ud->ud_footnotes, sizeof(struct ud_ref))) goto FAIL;
  if (!ud_oht_init(&ud->ud_styles, sizeof(struct ud_ref))) goto FAIL;
  if (!dstack_init(&ud->ud_errors, 16, sizeof(struct ud_error))) goto FAIL;
  if (!token_init(&ud->ud_tok)) goto FAIL;

  ud->ud_dirfd_pwd = open_ro(".");
  if (ud->ud_dirfd_pwd == -1) goto FAIL;
  ud->ud_dirfd_src = -1;
  ud->ud_dirfd_out = -1;
  ud->ud_main_doc = ud;

  ht_init(&ud->ud_documents);
  taia_now(&ud->ud_time_start);

  return 1;
  FAIL:
  ud_free(ud);
  return 0;
}

int
ud_new(struct udoc **udp)
{
  struct udoc *ud;
  ud = alloc_zero(sizeof(*ud));
  return !!(*udp = ud);
}
