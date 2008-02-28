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
  if (!ud_oht_init(&ud->parts, sizeof(struct ud_part))) return 0;
  if (!ud_oht_init(&ud->link_exts, sizeof(struct ud_ref))) return 0;
  if (!ud_oht_init(&ud->refs, sizeof(struct ud_ref))) return 0;
  if (!ud_oht_init(&ud->ref_names, sizeof(struct ud_ref))) return 0;
  if (!ud_oht_init(&ud->footnotes, sizeof(struct ud_ref))) return 0;
  if (!ud_oht_init(&ud->styles, sizeof(struct ud_ref))) return 0;
  ud->dirfd_pwd = open_ro(".");
  if (ud->dirfd_pwd == -1) return 0;
  ud->dirfd_src = -1;
  ud->dirfd_out = -1;

  taia_now(&ud->time_start);
  return token_init(&ud->tok);
}

int
ud_new(struct udoc **udp)
{
  struct udoc *ud;
  ud = alloc_zero(sizeof(*ud));
  return !!(*udp = ud);
}
