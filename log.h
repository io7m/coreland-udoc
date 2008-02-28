#ifndef LOG_H
#define LOG_H

#define LOG_FATAL  1
#define LOG_ERROR  2
#define LOG_WARN   3
#define LOG_NOTICE 4
#define LOG_INFO   5
#define LOG_DEBUG  6

void log_8x_core(unsigned int, const char *,
                 const char *, const char *, const char *, const char *,
                 const char *, const char *, const char *, const char *);
void log_8sys_core(unsigned int, const char *,
                   const char *, const char *, const char *, const char *,
                   const char *, const char *, const char *, const char *);
void log_die8x_core(unsigned int, int, const char *,
                    const char *, const char *, const char *, const char *,
                    const char *, const char *, const char *, const char *);
void log_die8sys_core(unsigned int, int, const char *,
                      const char *, const char *, const char *, const char *,
                      const char *, const char *, const char *, const char *);

void log_fd(int);
void log_progname(const char *);
void log_level(unsigned int);
void log_more(void);
void log_less(void);
void log_callback(void (*)(const char *, unsigned long));
void log_lock_callback(void (*)(void *), void *);
void log_unlock_callback(void (*)(void *), void *);
int log_exit_register(void (*)(void));

#define log_8sys(lev,x1,x2,x3,x4,x5,x6,x7,x8) \
log_8sys_core((lev),0,(x1),(x2),(x3),(x4),(x5),(x6),(x7),(x8))
#define log_7sys(lev,x1,x2,x3,x4,x5,x6,x7) \
log_8sys_core((lev),0,(x1),(x2),(x3),(x4),(x5),(x6),(x7),0)
#define log_6sys(lev,x1,x2,x3,x4,x5,x6) \
log_8sys_core((lev),0,(x1),(x2),(x3),(x4),(x5),(x6),0,0)
#define log_5sys(lev,x1,x2,x3,x4,x5) \
log_8sys_core((lev),0,(x1),(x2),(x3),(x4),(x5),0,0,0)
#define log_4sys(lev,x1,x2,x3,x4) \
log_8sys_core((lev),0,(x1),(x2),(x3),(x4),0,0,0,0)
#define log_3sys(lev,x1,x2,x3) \
log_8sys_core((lev),0,(x1),(x2),(x3),0,0,0,0,0)
#define log_2sys(lev,x1,x2) \
log_8sys_core((lev),0,(x1),(x2),0,0,0,0,0,0)
#define log_1sys(lev,x1) \
log_8sys_core((lev),0,(x1),0,0,0,0,0,0,0)

#define log_8x(lev,x1,x2,x3,x4,x5,x6,x7,x8) \
log_8x_core((lev),0,(x1),(x2),(x3),(x4),(x5),(x6),(x7),(x8))
#define log_7x(lev,x1,x2,x3,x4,x5,x6,x7) \
log_8x_core((lev),0,(x1),(x2),(x3),(x4),(x5),(x6),(x7),0)
#define log_6x(lev,x1,x2,x3,x4,x5,x6) \
log_8x_core((lev),0,(x1),(x2),(x3),(x4),(x5),(x6),0,0)
#define log_5x(lev,x1,x2,x3,x4,x5) \
log_8x_core((lev),0,(x1),(x2),(x3),(x4),(x5),0,0,0)
#define log_4x(lev,x1,x2,x3,x4) \
log_8x_core((lev),0,(x1),(x2),(x3),(x4),0,0,0,0)
#define log_3x(lev,x1,x2,x3) \
log_8x_core((lev),0,(x1),(x2),(x3),0,0,0,0,0)
#define log_2x(lev,x1,x2) \
log_8x_core((lev),0,(x1),(x2),0,0,0,0,0,0)
#define log_1x(lev,x1) \
log_8x_core((lev),0,(x1),0,0,0,0,0,0,0)

#define log_die8sys(lev,e,x1,x2,x3,x4,x5,x6,x7,x8) \
log_die8sys_core((lev),(e),0,(x1),(x2),(x3),(x4),(x5),(x6),(x7),(x8))
#define log_die7sys(lev,e,x1,x2,x3,x4,x5,x6,x7) \
log_die8sys_core((lev),(e),0,(x1),(x2),(x3),(x4),(x5),(x6),(x7),0)
#define log_die6sys(lev,e,x1,x2,x3,x4,x5,x6) \
log_die8sys_core((lev),(e),0,(x1),(x2),(x3),(x4),(x5),(x6),0,0)
#define log_die5sys(lev,e,x1,x2,x3,x4,x5) \
log_die8sys_core((lev),(e),0,(x1),(x2),(x3),(x4),(x5),0,0,0)
#define log_die4sys(lev,e,x1,x2,x3,x4) \
log_die8sys_core((lev),(e),0,(x1),(x2),(x3),(x4),0,0,0,0)
#define log_die3sys(lev,e,x1,x2,x3) \
log_die8sys_core((lev),(e),0,(x1),(x2),(x3),0,0,0,0,0)
#define log_die2sys(lev,e,x1,x2) \
log_die8sys_core((lev),(e),0,(x1),(x2),0,0,0,0,0,0)
#define log_die1sys(lev,e,x1) \
log_die8sys_core((lev),(e),0,(x1),0,0,0,0,0,0,0)

#define log_die8x(lev,e,x1,x2,x3,x4,x5,x6,x7,x8) \
log_die8x_core((lev),(e),0,(x1),(x2),(x3),(x4),(x5),(x6),(x7),(x8))
#define log_die7x(lev,e,x1,x2,x3,x4,x5,x6,x7) \
log_die8x_core((lev),(e),0,(x1),(x2),(x3),(x4),(x5),(x6),(x7),0)
#define log_die6x(lev,e,x1,x2,x3,x4,x5,x6) \
log_die8x_core((lev),(e),0,(x1),(x2),(x3),(x4),(x5),(x6),0,0)
#define log_die5x(lev,e,x1,x2,x3,x4,x5) \
log_die8x_core((lev),(e),0,(x1),(x2),(x3),(x4),(x5),0,0,0)
#define log_die4x(lev,e,x1,x2,x3,x4) \
log_die8x_core((lev),(e),0,(x1),(x2),(x3),(x4),0,0,0,0)
#define log_die3x(lev,e,x1,x2,x3) \
log_die8x_core((lev),(e),0,(x1),(x2),(x3),0,0,0,0,0)
#define log_die2x(lev,e,x1,x2) \
log_die8x_core((lev),(e),0,(x1),(x2),0,0,0,0,0,0)
#define log_die1x(lev,e,x1) \
log_die8x_core((lev),(e),0,(x1),0,0,0,0,0,0,0)

#define log_8sysf(lev,x1,x2,x3,x4,x5,x6,x7,x8) \
log_8sys_core((lev),__func__,(x1),(x2),(x3),(x4),(x5),(x6),(x7),(x8))
#define log_7sysf(lev,x1,x2,x3,x4,x5,x6,x7) \
log_8sys_core((lev),__func__,(x1),(x2),(x3),(x4),(x5),(x6),(x7),0)
#define log_6sysf(lev,x1,x2,x3,x4,x5,x6) \
log_8sys_core((lev),__func__,(x1),(x2),(x3),(x4),(x5),(x6),0,0)
#define log_5sysf(lev,x1,x2,x3,x4,x5) \
log_8sys_core((lev),__func__,(x1),(x2),(x3),(x4),(x5),0,0,0)
#define log_4sysf(lev,x1,x2,x3,x4) \
log_8sys_core((lev),__func__,(x1),(x2),(x3),(x4),0,0,0,0)
#define log_3sysf(lev,x1,x2,x3) \
log_8sys_core((lev),__func__,(x1),(x2),(x3),0,0,0,0,0)
#define log_2sysf(lev,x1,x2) \
log_8sys_core((lev),__func__,(x1),(x2),0,0,0,0,0,0)
#define log_1sysf(lev,x1) \
log_8sys_core((lev),__func__,(x1),0,0,0,0,0,0,0)

#define log_8xf(lev,x1,x2,x3,x4,x5,x6,x7,x8) \
log_8x_core((lev),__func__,(x1),(x2),(x3),(x4),(x5),(x6),(x7),(x8))
#define log_7xf(lev,x1,x2,x3,x4,x5,x6,x7) \
log_8x_core((lev),__func__,(x1),(x2),(x3),(x4),(x5),(x6),(x7),0)
#define log_6xf(lev,x1,x2,x3,x4,x5,x6) \
log_8x_core((lev),__func__,(x1),(x2),(x3),(x4),(x5),(x6),0,0)
#define log_5xf(lev,x1,x2,x3,x4,x5) \
log_8x_core((lev),__func__,(x1),(x2),(x3),(x4),(x5),0,0,0)
#define log_4xf(lev,x1,x2,x3,x4) \
log_8x_core((lev),__func__,(x1),(x2),(x3),(x4),0,0,0,0)
#define log_3xf(lev,x1,x2,x3) \
log_8x_core((lev),__func__,(x1),(x2),(x3),0,0,0,0,0)
#define log_2xf(lev,x1,x2) \
log_8x_core((lev),__func__,(x1),(x2),0,0,0,0,0,0)
#define log_1xf(lev,x1) \
log_8x_core((lev),__func__,(x1),0,0,0,0,0,0,0)

#define log_die8sysf(lev,e,x1,x2,x3,x4,x5,x6,x7,x8) \
log_die8sys_core((lev),(e),__func__,(x1),(x2),(x3),(x4),(x5),(x6),(x7),(x8))
#define log_die7sysf(lev,e,x1,x2,x3,x4,x5,x6,x7) \
log_die8sys_core((lev),(e),__func__,(x1),(x2),(x3),(x4),(x5),(x6),(x7),0)
#define log_die6sysf(lev,e,x1,x2,x3,x4,x5,x6) \
log_die8sys_core((lev),(e),__func__,(x1),(x2),(x3),(x4),(x5),(x6),0,0)
#define log_die5sysf(lev,e,x1,x2,x3,x4,x5) \
log_die8sys_core((lev),(e),__func__,(x1),(x2),(x3),(x4),(x5),0,0,0)
#define log_die4sysf(lev,e,x1,x2,x3,x4) \
log_die8sys_core((lev),(e),__func__,(x1),(x2),(x3),(x4),0,0,0,0)
#define log_die3sysf(lev,e,x1,x2,x3) \
log_die8sys_core((lev),(e),__func__,(x1),(x2),(x3),0,0,0,0,0)
#define log_die2sysf(lev,e,x1,x2) \
log_die8sys_core((lev),(e),__func__,(x1),(x2),0,0,0,0,0,0)
#define log_die1sysf(lev,e,x1) \
log_die8sys_core((lev),(e),__func__,(x1),0,0,0,0,0,0,0)

#define log_die8xf(lev,e,x1,x2,x3,x4,x5,x6,x7,x8) \
log_die8x_core((lev),(e),__func__,(x1),(x2),(x3),(x4),(x5),(x6),(x7),(x8))
#define log_die7xf(lev,e,x1,x2,x3,x4,x5,x6,x7) \
log_die8x_core((lev),(e),__func__,(x1),(x2),(x3),(x4),(x5),(x6),(x7),0)
#define log_die6xf(lev,e,x1,x2,x3,x4,x5,x6) \
log_die8x_core((lev),(e),__func__,(x1),(x2),(x3),(x4),(x5),(x6),0,0)
#define log_die5xf(lev,e,x1,x2,x3,x4,x5) \
log_die8x_core((lev),(e),__func__,(x1),(x2),(x3),(x4),(x5),0,0,0)
#define log_die4xf(lev,e,x1,x2,x3,x4) \
log_die8x_core((lev),(e),__func__,(x1),(x2),(x3),(x4),0,0,0,0)
#define log_die3xf(lev,e,x1,x2,x3) \
log_die8x_core((lev),(e),__func__,(x1),(x2),(x3),0,0,0,0,0)
#define log_die2xf(lev,e,x1,x2) \
log_die8x_core((lev),(e),__func__,(x1),(x2),0,0,0,0,0,0)
#define log_die1xf(lev,e,x1) \
log_die8x_core((lev),(e),__func__,(x1),0,0,0,0,0,0,0)

#endif
