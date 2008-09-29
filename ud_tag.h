#ifndef UD_TAG_H
#define UD_TAG_H

enum ud_tag {
  UDOC_TAG_UNKNOWN,
  UDOC_TAG_PARA,
  UDOC_TAG_PARA_VERBATIM,
  UDOC_TAG_TITLE,
  UDOC_TAG_SECTION,
  UDOC_TAG_SUBSECTION,
  UDOC_TAG_STYLE,
  UDOC_TAG_ITEM,
  UDOC_TAG_REF,
  UDOC_TAG_LINK,
  UDOC_TAG_LINK_EXT,
  UDOC_TAG_LIST,
  UDOC_TAG_TABLE,
  UDOC_TAG_TABLE_ROW,
  UDOC_TAG_ENCODING,
  UDOC_TAG_CONTENTS,
  UDOC_TAG_FOOTNOTE,
  UDOC_TAG_DATE,
  UDOC_TAG_RENDER_HEADER,
  UDOC_TAG_RENDER_FOOTER,
  UDOC_TAG_RENDER,
  UDOC_TAG_RENDER_NOESCAPE,

  /* always last */
  UDOC_TAG_LAST,
};

struct ud_tag_name {
  const char *ut_name;
  enum ud_tag ut_tag;
};

struct ud_tag_stack {
  struct array uts_sta;
};

#if defined(UDOC_IMPLEMENTATION)
int ud_tag_by_name (const char *, enum ud_tag *);
const char *ud_tag_name (enum ud_tag);
int ud_tag_stack_init (struct ud_tag_stack *);
void ud_tag_stack_free (struct ud_tag_stack *);
int ud_tag_stack_copy (struct ud_tag_stack *, const struct ud_tag_stack *);
int ud_tag_stack_push (struct ud_tag_stack *, enum ud_tag);
int ud_tag_stack_pop (struct ud_tag_stack *, enum ud_tag *);
int ud_tag_stack_peek (const struct ud_tag_stack *, enum ud_tag *);
int ud_tag_stack_above (const struct ud_tag_stack *, enum ud_tag);
const enum ud_tag *ud_tag_stack_index (const struct ud_tag_stack *, unsigned long);
unsigned long ud_tag_stack_size (const struct ud_tag_stack *);
extern const struct ud_tag_name ud_tags_by_name[];
extern const unsigned int ud_num_tags;
#endif

#endif
