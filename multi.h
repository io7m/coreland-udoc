#ifndef MULTI_H
#define MULTI_H

/* convenient 8 arg routines */

#include <corelib/buffer.h>

int buffer_puts8 (struct buffer *,
                 const char *, const char *, const char *, const char *,
                 const char *, const char *, const char *, const char *);

#define buffer_puts7(b,s1,s2,s3,s4,s5,s6,s7) \
buffer_puts8 ((b),(s1),(s2),(s3),(s4),(s5),(s6),(s7),0)
#define buffer_puts6(b,s1,s2,s3,s4,s5,s6) \
buffer_puts8 ((b),(s1),(s2),(s3),(s4),(s5),(s6),0,0)
#define buffer_puts5(b,s1,s2,s3,s4,s5) \
buffer_puts8 ((b),(s1),(s2),(s3),(s4),(s5),0,0,0)
#define buffer_puts4(b,s1,s2,s3,s4) \
buffer_puts8 ((b),(s1),(s2),(s3),(s4),0,0,0,0)
#define buffer_puts3(b,s1,s2,s3) \
buffer_puts8 ((b),(s1),(s2),(s3),0,0,0,0,0)
#define buffer_puts2(b,s1,s2) \
buffer_puts8 ((b),(s1),(s2),0,0,0,0,0,0)

#include <corelib/sstring.h>

unsigned long sstring_cats8(struct sstring *,
                            const char *, const char *, const char *, const char *,
                            const char *, const char *, const char *, const char *);

#define sstring_cats7(ss,s1,s2,s3,s4,s5,s6,s7) \
sstring_cats8 ((ss),(s1),(s2),(s3),(s4),(s5),(s6),(s7),0)
#define sstring_cats6(ss,s1,s2,s3,s4,s5,s6) \
sstring_cats8 ((ss),(s1),(s2),(s3),(s4),(s5),(s6),0,0)
#define sstring_cats5(ss,s1,s2,s3,s4,s5) \
sstring_cats8 ((ss),(s1),(s2),(s3),(s4),(s5),0,0,0)
#define sstring_cats4(ss,s1,s2,s3,s4) \
sstring_cats8((ss),(s1),(s2),(s3),(s4),0,0,0,0)
#define sstring_cats3(ss,s1,s2,s3) \
sstring_cats8((ss),(s1),(s2),(s3),0,0,0,0,0)
#define sstring_cats2(ss,s1,s2) \
sstring_cats8((ss),(s1),(s2),0,0,0,0,0,0)

#endif
