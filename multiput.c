#include <corelib/buffer.h>
#include "multi.h"

int buffer_puts8 (struct buffer *b,
                 const char *s1, const char *s2, const char *s3, const char *s4,
                 const char *s5, const char *s6, const char *s7, const char *s8)
{
  if (s1) buffer_puts (b, s1);
  if (s2) buffer_puts (b, s2);
  if (s3) buffer_puts (b, s3);
  if (s4) buffer_puts (b, s4);
  if (s5) buffer_puts (b, s5);
  if (s6) buffer_puts (b, s6);
  if (s7) buffer_puts (b, s7);
  if (s8) buffer_puts (b, s8);
  return 0;
}
