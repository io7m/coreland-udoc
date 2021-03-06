#include <corelib/array.h>
#include <corelib/hashtable.h>

#define UDOC_IMPLEMENTATION
#include "ud_tag.h"
#include "ud_tree.h"

/*
 * main tags list
 */

const struct ud_tag_name ud_tags_by_name[] = {
  { "?",               UDOC_TAG_UNKNOWN },
  { "para",            UDOC_TAG_PARA },
  { "para-verbatim",   UDOC_TAG_PARA_VERBATIM },
  { "item",            UDOC_TAG_ITEM },
  { "section",         UDOC_TAG_SECTION },
  { "subsection",      UDOC_TAG_SUBSECTION },
  { "ref",             UDOC_TAG_REF },
  { "link",            UDOC_TAG_LINK },
  { "link-ext",        UDOC_TAG_LINK_EXT },
  { "table",           UDOC_TAG_TABLE },
  { "t-row",           UDOC_TAG_TABLE_ROW },
  { "list",            UDOC_TAG_LIST },
  { "title",           UDOC_TAG_TITLE },
  { "style",           UDOC_TAG_STYLE },
  { "encoding",        UDOC_TAG_ENCODING },
  { "contents",        UDOC_TAG_CONTENTS },
  { "footnote",        UDOC_TAG_FOOTNOTE },
  { "date",            UDOC_TAG_DATE },
  { "render-header",   UDOC_TAG_RENDER_HEADER },
  { "render-footer",   UDOC_TAG_RENDER_FOOTER },
  { "render",          UDOC_TAG_RENDER },
  { "render-noescape", UDOC_TAG_RENDER_NOESCAPE },
};
const unsigned int ud_num_tags = sizeof (ud_tags_by_name)
                               / sizeof (ud_tags_by_name[0]);
/*
 * type names
 */

const struct ud_node_type_name ud_node_types_by_name[] = {
  { "symbol",  UDOC_TYPE_SYMBOL },
  { "string",  UDOC_TYPE_STRING },
  { "list",    UDOC_TYPE_LIST },
  { "include", UDOC_TYPE_INCLUDE },
};
const unsigned int ud_num_node_types = sizeof (ud_node_types_by_name)
                                     / sizeof (ud_node_types_by_name[0]);
