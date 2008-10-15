#include <corelib/buffer.h>
#include <corelib/get_opt.h>
#include <corelib/open.h>
#include <corelib/scan.h>
#include <corelib/str.h>
#include <corelib/syserr.h>

#include "ctxt.h"
#include "log.h"
#include "multi.h"
#include "udoc.h"

const char *usage_s = "[-hlV] [-L lev] file";
const char *help_s =
"   -L: log level (max 6, min 0, default 5)\n"
"   -l: suppress link integrity checking in validation.\n"
"   -V: version\n"
"   -h: this message";

struct udoc main_doc;
struct udoc_opts main_opts;

void
help (void)
{
  log_die3x (0, 0, usage_s, "\n", help_s);
}

void
usage (void)
{
  log_die1x (0, 111, usage_s);
}

void
die (const char *func)
{
  const unsigned long max = ud_error_size (&main_doc.ud_errors);
  unsigned long index;
  struct ud_err *ue;

  for (index = 0; index < max; ++index) {
    if (ud_error_pop (&main_doc.ud_errors, &ue))
      ud_error_display (&main_doc, ue);
  }
  log_die2x (LOG_FATAL, 112, func, " failed");
}

int
main (int argc, char *argv[])
{
  const char *file;
  unsigned long num;
  unsigned long flags = 0;
  int ch;

  log_progname ("udoc-valid");
  while ((ch = get_opt (argc, argv, "hlL:V")) != opteof)
    switch (ch) {
      case 'h': help ();
      case 'L':
        if (!scan_ulong (optarg, &num))
          log_die1x (LOG_FATAL, 111, "log level must be numeric");
        log_level (num);
        break;
      case 'l':
        flags |= UDOC_PART_NO_LINK_CHECK;
        break;
      case 'V':
        buffer_puts2 (buffer1, ctxt_version, "\n");
        if (buffer_flush (buffer1) == -1)
          log_die1sys (LOG_FATAL, 112, "write");
        return 0;
      default: usage ();
    }
  argc -= optind;
  argv += optind;

  if (argc < 1) usage ();

  file = argv[0];

  if (!ud_init (&main_doc)) die ("ud_init");
  main_doc.ud_opts = main_opts;
  if (!ud_open (&main_doc, file)) die ("opening");
  log_1xf (LOG_DEBUG, "parsing");
  if (!ud_parse (&main_doc)) die ("parsing");
  log_1xf (LOG_DEBUG, "validating");
  if (!ud_validate (&main_doc)) die ("validation");
  log_1xf (LOG_DEBUG, "partitioning");
  if (!ud_partition (&main_doc, flags)) die ("partitioning");
  if (!ud_close (&main_doc)) die ("closing");

  return 0;
}
