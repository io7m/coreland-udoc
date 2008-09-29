#include <corelib/error.h>

#define DFO_IMPLEMENTATION
#include "dfo.h"

const char *
dfo_errorstr (enum dfo_error e)
{
  switch (e) {
    case DFO_PAGE_TOO_SMALL:
      return "page size or spacing settings result in zero sized columns";
    case DFO_NO_COLUMNS:
      return "number of columns must be non-zero";
    case DFO_TOO_SMALL_FOR_HYPH:
      return "column or page space is too small for hyphenation";
    default:
      return error_str (e);
  }
}
