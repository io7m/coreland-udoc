#include "ctxt.h"
#include "install.h"

struct install_item insthier[] = {
  {INST_MKDIR, 0, 0, ctxt_bindir, 0, 0, 0755},
  {INST_MKDIR, 0, 0, ctxt_incdir, 0, 0, 0755},
  {INST_MKDIR, 0, 0, ctxt_dlibdir, 0, 0, 0755},
  {INST_MKDIR, 0, 0, ctxt_slibdir, 0, 0, 0755},
  {INST_MKDIR, 0, 0, ctxt_repos, 0, 0, 0755},
  {INST_COPY, "dfo_column.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "dfo_cons.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "dfo_cur.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "dfo_err.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "dfo_free.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "dfo_init.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "dfo_put.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "dfo_size.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "dfo_tran.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "dfo_wrap.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "dfo_ws.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "log.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "multicats.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "multiput.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "tok_close.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "tok_free.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "tok_init.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "tok_next.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "tok_open.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "ud_assert.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "ud_close.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "ud_data.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "ud_error.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "ud_free.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "ud_init.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "ud_list.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "ud_oht.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "ud_open.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "ud_parse.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "ud_part.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "ud_ref.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "ud_render.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "ud_table.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "ud_tag.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "ud_tag_st.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "ud_tree.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "ud_valid.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "udoc-conf.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "udoc.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "udr_context.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "udr_debug.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "udr_nroff.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "udr_null.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "udr_plain.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "udr_xhtml.c", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "dfo.h", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "dfo.h", 0, ctxt_incdir, 0, 0, 0644},
  {INST_COPY, "gen_stack.h", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "gen_stack.h", 0, ctxt_incdir, 0, 0, 0644},
  {INST_COPY, "log.h", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "log.h", 0, ctxt_incdir, 0, 0, 0644},
  {INST_COPY, "multi.h", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "multi.h", 0, ctxt_incdir, 0, 0, 0644},
  {INST_COPY, "token.h", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "token.h", 0, ctxt_incdir, 0, 0, 0644},
  {INST_COPY, "ud_assert.h", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "ud_assert.h", 0, ctxt_incdir, 0, 0, 0644},
  {INST_COPY, "ud_error.h", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "ud_error.h", 0, ctxt_incdir, 0, 0, 0644},
  {INST_COPY, "ud_oht.h", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "ud_oht.h", 0, ctxt_incdir, 0, 0, 0644},
  {INST_COPY, "ud_part.h", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "ud_part.h", 0, ctxt_incdir, 0, 0, 0644},
  {INST_COPY, "ud_ref.h", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "ud_ref.h", 0, ctxt_incdir, 0, 0, 0644},
  {INST_COPY, "ud_render.h", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "ud_render.h", 0, ctxt_incdir, 0, 0, 0644},
  {INST_COPY, "ud_table.h", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "ud_table.h", 0, ctxt_incdir, 0, 0, 0644},
  {INST_COPY, "ud_tag.h", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "ud_tag.h", 0, ctxt_incdir, 0, 0, 0644},
  {INST_COPY, "ud_tree.h", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "ud_tree.h", 0, ctxt_incdir, 0, 0, 0644},
  {INST_COPY, "udoc.h", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "udoc.h", 0, ctxt_incdir, 0, 0, 0644},
  {INST_COPY, "dfo.sld", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "dfo.a", "libdfo.a", ctxt_slibdir, 0, 0, 0644},
  {INST_COPY, "log.sld", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "log.a", "liblog.a", ctxt_slibdir, 0, 0, 0644},
  {INST_COPY, "multi.sld", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "multi.a", "libmulti.a", ctxt_slibdir, 0, 0, 0644},
  {INST_COPY, "token.sld", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "token.a", "libtoken.a", ctxt_slibdir, 0, 0, 0644},
  {INST_COPY, "ud.sld", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "ud.a", "libud.a", ctxt_slibdir, 0, 0, 0644},
  {INST_COPY, "udoc-conf.ld", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "udoc-conf", 0, ctxt_bindir, 0, 0, 0755},
  {INST_COPY, "udoc.ld", 0, ctxt_repos, 0, 0, 0644},
  {INST_COPY, "udoc", 0, ctxt_bindir, 0, 0, 0755},
};
unsigned long insthier_len = sizeof(insthier) / sizeof(struct install_item);
