#define UDOC_IMPLEMENTATION
#include "udoc.h"

int
ud_free(struct udoc *ud)
{
  ud_oht_free(&ud->ud_parts);
  ud_oht_free(&ud->ud_link_exts);
  ud_oht_free(&ud->ud_refs);
  ud_oht_free(&ud->ud_ref_names);
  ud_oht_free(&ud->ud_footnotes);
  ud_oht_free(&ud->ud_styles);
  dstack_free(&ud->ud_errors);
  token_free(&ud->ud_tok);
  return 1;
}
