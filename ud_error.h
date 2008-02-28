#ifndef UD_ERROR_H
#define UD_ERROR_H

struct ud_error {
  char *file;
  char *error;
};

#define ud_error_pushsys(ds,s) ud_error_push((ds),(s),error_str(errno))

int ud_error_init(struct dstack *);
int ud_error_push(struct dstack *, const char *, const char *);
int ud_error_pop(struct dstack *, struct ud_error *);
unsigned long ud_error_size(struct dstack *);
void ud_error_free(struct ud_error *);

extern struct dstack ud_errors;

#endif
