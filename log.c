#include <stdlib.h> /* atexit() */

#include <corelib/buffer.h>
#include <corelib/error.h>
#include <corelib/str.h>
#include <corelib/write.h>

#include "log.h"

typedef void vfp(void);
static vfp *exit_funcs[32];
static unsigned int exit_funcs_num = 0;
static int atexit_done = 0;

static char obuf[BUFFER_OUTSIZE];
static struct buffer logbuf = buffer_INIT(write, 2, obuf, sizeof(obuf));

static void (*cb_log)(const char *, unsigned long) = 0;
static void (*cb_lock)(void *) = 0;
static void *cb_lock_data = 0;
static void (*cb_unlock)(void *) = 0;
static void *cb_unlock_data = 0;

static const char *progname = 0;
static unsigned int level_current = LOG_INFO;

static const struct {
  const char *str;
  unsigned int lev;
} level_tab[] = {
  { 0, 0 },
  { "fatal", LOG_FATAL },
  { "error", LOG_ERROR },
  { "warn", LOG_WARN },
  { "notice", LOG_NOTICE },
  { "info", LOG_INFO },
  { "debug", LOG_DEBUG },
};

static void log_call_exits(void)
{
  unsigned int i;
  for (i = 0; i < exit_funcs_num; ++i) exit_funcs[i]();
}
static void log_exit(int e) { _exit(e); }
static void log_core(unsigned int lev, int sys, const char *func,
                     const char *s1, const char *s2,
                     const char *s3, const char *s4,
                     const char *s5, const char *s6,
                     const char *s7, const char *s8)
{
  if (lev <= level_current) {
    if (progname) {
      buffer_puts(&logbuf, progname);
      buffer_put(&logbuf, ": ", 2);
    }
    if (level_tab[lev].str) {
      buffer_puts(&logbuf, level_tab[lev].str);
      buffer_put(&logbuf, ": ", 2);
    }
    if (func) {
      buffer_puts(&logbuf, func);
      buffer_put(&logbuf, ": ", 2);
    }
    if (s1) buffer_puts(&logbuf, s1);
    if (s2) buffer_puts(&logbuf, s2);
    if (s3) buffer_puts(&logbuf, s3);
    if (s4) buffer_puts(&logbuf, s4);
    if (s5) buffer_puts(&logbuf, s5);
    if (s6) buffer_puts(&logbuf, s6);
    if (s7) buffer_puts(&logbuf, s7);
    if (s8) buffer_puts(&logbuf, s8);
    if (sys) {
      buffer_put(&logbuf, ": ", 2);
      buffer_puts(&logbuf, error_str(errno));
    }
    buffer_put(&logbuf, "\n", 1);
    buffer_flush(&logbuf);
  }
}

/* public */

int log_exit_register(void (*func)(void))
{
  if (exit_funcs_num < sizeof(exit_funcs) / sizeof(exit_funcs[0])) {
    exit_funcs[exit_funcs_num] = func; ++exit_funcs_num;
  } else
    return 0;
  if (!atexit_done)
    if (atexit(log_call_exits) == -1) return 0;
  return 1;
}
void log_more(void) { if (level_current < LOG_DEBUG) ++level_current; }
void log_less(void) { if (level_current > LOG_FATAL) --level_current; }

void log_callback(void (*cb)(const char *, unsigned long)) { cb_log = cb; }
void log_lock_callback(void (*cb)(void *), void *data) { cb_lock = cb; cb_lock_data = data; }
void log_unlock_callback(void (*cb)(void *), void *data) { cb_unlock = cb; cb_unlock_data = data; }

void log_level(unsigned int lev) { if (lev <= LOG_DEBUG) level_current = lev; }
void log_progname(const char *p) { progname = p; }
void log_fd(int fd) { logbuf.fd = fd; }
void log_8x_core(unsigned int lev, const char *func,
                 const char *s1, const char *s2, const char *s3, const char *s4,
                 const char *s5, const char *s6, const char *s7, const char *s8)
{
  log_core(lev, 0, func, s1, s2, s3, s4, s5, s6, s7, s8);
}
void log_8sys_core(unsigned int lev, const char *func,
                   const char *s1, const char *s2, const char *s3, const char *s4,
                   const char *s5, const char *s6, const char *s7, const char *s8)
{
  log_core(lev, 1, func, s1, s2, s3, s4, s5, s6, s7, s8);
}
void log_die8x_core(unsigned int lev, int e, const char *func,
                    const char *s1, const char *s2, const char *s3, const char *s4,
                    const char *s5, const char *s6, const char *s7, const char *s8)
{
  log_core(lev, 0, func, s1, s2, s3, s4, s5, s6, s7, s8);
  log_call_exits();
  log_exit(e);
}
void log_die8sys_core(unsigned int lev, int e, const char *func,
                      const char *s1, const char *s2, const char *s3, const char *s4,
                      const char *s5, const char *s6, const char *s7, const char *s8)
{
  log_core(lev, 1, func, s1, s2, s3, s4, s5, s6, s7, s8);
  log_call_exits();
  log_exit(e);
}
