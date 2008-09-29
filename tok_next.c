#include "token.h"

#define STATE_DEFAULT 0x0000
#define STATE_STRING  0x0001
#define STATE_ESCAPE  0x0002
#define STATE_COMMENT 0x0004

#define STATE_SET(s) (t->state |= (s))
#define STATE_UNSET(s) (t->state &= ~ (s))
#define STATE_ISSET(s) (t->state & (s))

static int
chars_get (struct tokenizer *t)
{
  if (t->ch_next) {
    t->ch_cur = t->ch_next;
    switch (buffer_get (&t->buf, &t->ch_next, 1)) {
      case 0: t->ch_next = 0; break;
      case -1: return -1;
      default: ++t->chr;
    }
  } else {
    switch (buffer_get (&t->buf, &t->ch_cur, 1)) {
      case 0: return 0;
      case -1: return -1;
      default: ++t->chr;
    }
    switch (buffer_get (&t->buf, &t->ch_next, 1)) {
      case 0: t->ch_next = 0; break;
      case -1: return -1;
      default: ++t->chr;
    }
  }
  return 1;
}

int
token_next (struct tokenizer *t, char **tok, enum token_type *type)
{
  enum token_type etype = TOKEN_TYPE_SYMBOL;

  dstring_trunc (&t->tok);
  for (;;) {
    switch (chars_get (t)) {
      case 0:
        if (STATE_ISSET (STATE_STRING)) return 0;
        if (STATE_ISSET (STATE_ESCAPE)) return 0;
        if (STATE_ISSET (STATE_COMMENT)) return 0;
        etype = TOKEN_TYPE_EOF;
        goto END;
      case -1: return 0;
      default: break;
    }
    switch (t->ch_cur) {
      case ';':
        if (STATE_ISSET (STATE_STRING) || STATE_ISSET (STATE_ESCAPE)) {
          if (!dstring_catb (&t->tok, &t->ch_cur, 1)) return 0;
          break;
        }
        if (STATE_ISSET (STATE_COMMENT)) break;
        STATE_SET (STATE_COMMENT);
        break;
      case '\n':
        if (STATE_ISSET (STATE_COMMENT)) {
          STATE_UNSET (STATE_COMMENT);
          break;
        }
        t->line++;
      case ' ':
      case '\t':
        if (STATE_ISSET (STATE_COMMENT)) break;
        if (STATE_ISSET (STATE_ESCAPE)) STATE_UNSET (STATE_ESCAPE);
        if (STATE_ISSET (STATE_STRING)) {
          if (!dstring_catb (&t->tok, &t->ch_cur, 1)) return 0;
        } else {
          if (t->tok.len) goto END;
        }
        break;
      case '"':
        if (STATE_ISSET (STATE_COMMENT)) break;
        if (STATE_ISSET (STATE_STRING)) {
          if (STATE_ISSET (STATE_ESCAPE)) {
            if (!dstring_catb (&t->tok, &t->ch_cur, 1)) return 0;
            STATE_UNSET (STATE_ESCAPE);
          } else {
            STATE_UNSET (STATE_STRING);
            goto END;
          }
        } else {
          STATE_SET (STATE_STRING);
          etype = TOKEN_TYPE_STRING;
        }
        break;
      case '\\':
        if (STATE_ISSET (STATE_COMMENT)) break;
        if (STATE_ISSET (STATE_STRING)) {
          if (STATE_ISSET (STATE_ESCAPE)) {
            if (!dstring_catb (&t->tok, &t->ch_cur, 1)) return 0;
            STATE_UNSET (STATE_ESCAPE);
          } else {
            STATE_SET (STATE_ESCAPE);
          }
        }
        break;
      case '(':
        if (STATE_ISSET (STATE_COMMENT)) break;
        if (!dstring_catb (&t->tok, &t->ch_cur, 1)) return 0;
        if (!STATE_ISSET (STATE_STRING)) {
          etype = TOKEN_TYPE_PAREN_OPEN;
          goto END;
        }
        break;
      case ')':
        if (STATE_ISSET (STATE_COMMENT)) break;
        if (!dstring_catb (&t->tok, &t->ch_cur, 1)) return 0;
        if (!STATE_ISSET (STATE_STRING)) {
          etype = TOKEN_TYPE_PAREN_CLOSE;
          goto END;
        }
        break;
      default:
        if (STATE_ISSET (STATE_COMMENT)) break;
        if (STATE_ISSET (STATE_ESCAPE)) STATE_UNSET (STATE_ESCAPE);
        if (!dstring_catb (&t->tok, &t->ch_cur, 1)) return 0;
        switch (t->ch_next) {
          case ')':
          case '(':
          case '"':
            if (!STATE_ISSET (STATE_STRING)) goto END;
          default: break;
        }
        break;
    }
  }

END:
  dstring_0 (&t->tok);
  *tok = t->tok.s;
  *type = etype;
  return 1;
}
