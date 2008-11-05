#ifndef DFO_H
#define DFO_H

/* Deluxe Formatted Output (tm) */

#include <corelib/array.h>
#include <corelib/dstring.h>
#include <corelib/buffer.h>
 
enum {
  DFO_BLOCK_SIZE = 4096,
};

enum {
  DFO_WRAP_NONE = 0x0000, /* don't wrap */
  DFO_WRAP_SOFT = 0x0001, /* wrap at line_max, don't split words */
  DFO_WRAP_HARD = 0x0002, /* wrap at line_max, breaking words to do so */
  DFO_WRAP_HYPH = 0x0003  /* hyphenate wrapping */
};

enum {
  DFO_TRAN_NONE    = 0x0000, /* no character transforms */
  DFO_TRAN_RESPACE = 0x0001, /* convert all whitespace to single space */
  DFO_TRAN_CONVERT = 0x0002  /* do character transforms */
};

enum dfo_error {
  DFO_PAGE_TOO_SMALL     = -1,
  DFO_NO_COLUMNS         = -2,
  DFO_TOO_SMALL_FOR_HYPH = -3
};

struct dfo_put;
struct dfo_trans {
  char ch;
  const char * (*func)(struct dfo_put *);
};
struct dfo_buffer {
  struct dstring buf;
  unsigned long write_pos;  /* used when flushing */
  unsigned int line_pos;
};

struct dfo_put {
  struct buffer *out;
  struct dstring buf_tmp;
  struct dstring buf_input;
  const struct dfo_trans *trans;
  unsigned int trans_size;
  unsigned int page_max;
  unsigned int page_indent;
  unsigned int wrap_mode;
  unsigned int tran_mode;
  unsigned int line_start;
  enum dfo_error error;
  void *user_data;
  struct array col_bufs;
  unsigned int col_padding;
  int col_max;
  int col_cur;
  char ch_prev;
};

int dfo_init (struct dfo_put *, struct buffer *, const struct dfo_trans *, unsigned int);
void dfo_free (struct dfo_put *);

int dfo_put (struct dfo_put *, const char *, unsigned long);
int dfo_puts (struct dfo_put *, const char *);
int dfo_puts8 (struct dfo_put *, const char *, const char *, const char *, const char *,
                                const char *, const char *, const char *, const char *);
int dfo_break (struct dfo_put *);
int dfo_break_line (struct dfo_put *);
int dfo_flush (struct dfo_put *);

int dfo_constrain (struct dfo_put *, unsigned int, unsigned int);

void dfo_wrap_mode (struct dfo_put *, unsigned int);
void dfo_tran_enable (struct dfo_put *, unsigned int);
void dfo_tran_disable (struct dfo_put *, unsigned int);

int dfo_columns_setup (struct dfo_put *, unsigned int, unsigned int);
int dfo_columns_start (struct dfo_put *);
int dfo_columns_end (struct dfo_put *);

const char *dfo_errorstr (enum dfo_error);

const struct dfo_buffer *dfo_current_buf (const struct dfo_put *);

#if defined (DFO_IMPLEMENTATION)
int dfo_size_check (struct dfo_put *, unsigned int, unsigned int, unsigned int, unsigned int);
unsigned int dfo_size_linemax (const struct dfo_put *);
unsigned int dfo_size_linepos (const struct dfo_put *, unsigned int, const struct dfo_buffer *);
unsigned int dfo_size_linemax_check (unsigned int, unsigned int, unsigned int, unsigned int);
int dfo_tran_flatten (struct dfo_put *, struct dstring *);
int dfo_tran_respace (struct dfo_put *, struct dfo_buffer *, struct dstring *);
int dfo_tran_conv (struct dfo_put *, struct dfo_buffer *, struct dstring *);
int dfo_wrap (struct dfo_put *, struct dfo_buffer *, struct dstring *);
extern const char *dfo_whitespace;
#endif

#define dfo_puts7(b,s1,s2,s3,s4,s5,s6,s7) \
dfo_puts8 ((b),(s1),(s2),(s3),(s4),(s5),(s6),(s7),0)
#define dfo_puts6(b,s1,s2,s3,s4,s5,s6) \
dfo_puts8 ((b),(s1),(s2),(s3),(s4),(s5),(s6),0,0)
#define dfo_puts5(b,s1,s2,s3,s4,s5) \
dfo_puts8 ((b),(s1),(s2),(s3),(s4),(s5),0,0,0)
#define dfo_puts4(b,s1,s2,s3,s4) \
dfo_puts8 ((b),(s1),(s2),(s3),(s4),0,0,0,0)
#define dfo_puts3(b,s1,s2,s3) \
dfo_puts8 ((b),(s1),(s2),(s3),0,0,0,0,0)
#define dfo_puts2(b,s1,s2) \
dfo_puts8 ((b),(s1),(s2),0,0,0,0,0,0)

#endif
