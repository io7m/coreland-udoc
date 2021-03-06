#include <corelib/buffer.h>
#include <corelib/fmt.h>
#include <corelib/get_opt.h>
#include <corelib/open.h>
#include <corelib/scan.h>
#include <corelib/str.h>
#include <corelib/syserr.h>

#include "ctxt.h"
#include "log.h"
#include "multi.h"
#include "udoc.h"

const char *usage_s = "[-hV] [-L lev] [-s threshold] [-r renderer] file";
const char *help_s =
"   -s: split threshold (default: 0 - no splitting)\n"
"   -r: select output renderer (default: udoc, ? to list available backends)\n"
"   -L: log level (max 6, min 0, default 5)\n"
"   -V: version\n"
"   -h: this message";

struct udoc main_doc;
struct udoc_opts main_opts;
struct udr_opts r_opts = UDR_OPTS_INIT;

static const struct ud_renderer *renderers[] = {
  &ud_render_xhtml,
  &ud_render_context,
  &ud_render_plain,
  &ud_render_nroff,
  &ud_render_null,
  &ud_render_debug,
};

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
  unsigned long index;
  unsigned long max;
  struct ud_err *ue;

  max = ud_error_size (&main_doc.ud_errors);
  for (index = 0; index < max; ++index) {
    if (ud_error_pop (&main_doc.ud_errors, &ue))
      ud_error_display (&main_doc, ue);
  }
  log_die2x (LOG_FATAL, 112, func, " failed");
}

void
files (const struct udoc *doc, const struct ud_renderer *r)
{
  char cnum[FMT_ULONG];
  struct ud_part *cur_part;
  const unsigned long max = ud_oht_size (&doc->ud_parts);
  unsigned long index;
  
  for (index = 0; index < max; ++index) {
    ud_oht_get_index (&doc->ud_parts, index, (void *) &cur_part);
    if (cur_part->up_flags & UD_PART_SPLIT) {
      cnum[fmt_ulong (cnum, cur_part->up_file)] = 0;
      buffer_puts4 (buffer1, cnum, ".", r->ur_data.ur_suffix, "\n");
    } 
  }
  if (buffer_flush (buffer1) == -1) log_die1sys (LOG_FATAL, 112, "write");
}

int main (int argc, char *argv[])
{
  const char *file;
  const struct ud_renderer *r = renderers[0];
  unsigned long num;
  int ch;

  log_progname ("udoc-files");
  while ((ch = get_opt (argc, argv, "hL:Vr:s:")) != opteof)
    switch (ch) {
      case 's':
        if (!scan_ulong (optarg, &main_opts.ud_split_thresh))
          log_die1x (LOG_FATAL, 111, "split threshold must be numeric");
        break;
      case 'h': help (); break;
      case 'L':
        if (!scan_ulong (optarg, &num))
          log_die1x (LOG_FATAL, 111, "log level must be numeric");
        log_level (num);
        break;
      case 'r':
        r = 0;
        if (str_same (optarg, "?")) {
          for (num = 0; num < sizeof (renderers) / sizeof (renderers[0]); ++num) {
            buffer_puts4 (buffer1, renderers[num]->ur_data.ur_name, ": ",
                                  renderers[num]->ur_data.ur_desc, "\n");
          }
          if (buffer_flush (buffer1) == -1)
            log_die1sys (LOG_FATAL, 112, "write");
          return 0;
        }
        for (num = 0; num < sizeof (renderers) / sizeof (renderers[0]); ++num) {
          if (str_same (optarg, renderers[num]->ur_data.ur_name)) {
            r = renderers[num];
            break;
          }
        }
        if (!r) log_die1x (LOG_FATAL, 111, "unknown renderer");
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

  /* save ud_split_thresh as hint */
  r_opts.uo_split_hint = main_opts.ud_split_thresh;

  /* some renderers don't support split output */
  if (!r->ur_data.ur_part && main_opts.ud_split_thresh) {
    log_1x (LOG_NOTICE, "this renderer does not support split output");
    log_1x (LOG_NOTICE, "split threshold set to 0");
    main_opts.ud_split_thresh = 0;
  }

  if (!ud_init (&main_doc)) die ("ud_init");
  main_doc.ud_opts = main_opts;
  if (!ud_open (&main_doc, file)) die ("opening");
  log_1xf (LOG_DEBUG, "parsing");
  if (!ud_parse (&main_doc)) die ("parsing");
  log_1xf (LOG_DEBUG, "validating");
  if (!ud_validate (&main_doc)) die ("validation");
  log_1xf (LOG_DEBUG, "partitioning");
  if (!ud_partition (&main_doc, 0)) die ("partitioning");

  files (&main_doc, r);

  if (!ud_close (&main_doc)) die ("closing");
  return 0;
}
