#include <corelib/sstring.h>
#include "multi.h"

unsigned long sstring_cats8 (struct sstring *ss,
                 const char *s1, const char *s2, const char *s3, const char *s4,
                 const char *s5, const char *s6, const char *s7, const char *s8)
{
  unsigned long r = 0;
  if (s1) r += sstring_cats (ss, s1);
  if (s2) r += sstring_cats (ss, s2);
  if (s3) r += sstring_cats (ss, s3);
  if (s4) r += sstring_cats (ss, s4);
  if (s5) r += sstring_cats (ss, s5);
  if (s6) r += sstring_cats (ss, s6);
  if (s7) r += sstring_cats (ss, s7);
  if (s8) r += sstring_cats (ss, s8);
  return r;
}
