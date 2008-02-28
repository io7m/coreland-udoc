#ifndef UD_ASSERT_H
#define UD_ASSERT_H

void ud_assert_core(const char *, unsigned long, const char *);
void ud_assert_core_s(const char *, unsigned long, const char *, const char *);

#ifdef UD_ASSERT_DISABLE
#define ud_assert(e) (e)
#define ud_assert(e,s) (e)
#else
#define ud_assert(e) \
  ((e) ? (void) 0 : ud_assert_core(__func__, __LINE__, #e))
#define ud_assert_s(e,s) \
  ((e) ? (void) 0 : ud_assert_core_s(__func__, __LINE__, #e, (s)))
#endif

#endif
