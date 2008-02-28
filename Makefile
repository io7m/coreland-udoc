# auto generated - do not edit

default: all

all:\
UNIT_TESTS/t_fail_parse/failparse UNIT_TESTS/t_fail_part/failpart \
UNIT_TESTS/t_fail_render/failrender UNIT_TESTS/t_fail_valid/failvalid \
UNIT_TESTS/t_pass_parse/passparse UNIT_TESTS/t_pass_valid/passvalid \
UNIT_TESTS/t_token/t_token ctxt/ctxt.a deinstaller dfo.a inst-check \
inst-copy inst-dir inst-link installer instchk log.a multi.a token.a ud.a \
udoc udoc-conf 

# -- SYSDEPS start
flags-chrono:
	@echo SYSDEPS chrono-flags run create flags-chrono 
	@(cd SYSDEPS/modules/chrono-flags && ./run)
libs-chrono-S:
	@echo SYSDEPS chrono-libs-S run create libs-chrono-S 
	@(cd SYSDEPS/modules/chrono-libs-S && ./run)
flags-corelib:
	@echo SYSDEPS corelib-flags run create flags-corelib 
	@(cd SYSDEPS/modules/corelib-flags && ./run)
libs-corelib-S:
	@echo SYSDEPS corelib-libs-S run create libs-corelib-S 
	@(cd SYSDEPS/modules/corelib-libs-S && ./run)
_sysinfo.h:
	@echo SYSDEPS sysinfo run create _sysinfo.h 
	@(cd SYSDEPS/modules/sysinfo && ./run)


chrono-flags_clean:
	@echo SYSDEPS chrono-flags clean flags-chrono 
	@(cd SYSDEPS/modules/chrono-flags && ./clean)
chrono-libs-S_clean:
	@echo SYSDEPS chrono-libs-S clean libs-chrono-S 
	@(cd SYSDEPS/modules/chrono-libs-S && ./clean)
corelib-flags_clean:
	@echo SYSDEPS corelib-flags clean flags-corelib 
	@(cd SYSDEPS/modules/corelib-flags && ./clean)
corelib-libs-S_clean:
	@echo SYSDEPS corelib-libs-S clean libs-corelib-S 
	@(cd SYSDEPS/modules/corelib-libs-S && ./clean)
sysinfo_clean:
	@echo SYSDEPS sysinfo clean _sysinfo.h 
	@(cd SYSDEPS/modules/sysinfo && ./clean)


sysdeps_clean:\
chrono-flags_clean \
chrono-libs-S_clean \
corelib-flags_clean \
corelib-libs-S_clean \
sysinfo_clean \


# -- SYSDEPS end

UNIT_TESTS/t_assert.o:\
cc-compile UNIT_TESTS/t_assert.c UNIT_TESTS/t_assert.h 
	./cc-compile UNIT_TESTS/t_assert.c

UNIT_TESTS/t_fail_parse/failparse:\
cc-link UNIT_TESTS/t_fail_parse/failparse.ld \
UNIT_TESTS/t_fail_parse/failparse.o UNIT_TESTS/t_assert.o ud.a token.a log.a \
multi.a 
	./cc-link UNIT_TESTS/t_fail_parse/failparse \
	UNIT_TESTS/t_fail_parse/failparse.o UNIT_TESTS/t_assert.o ud.a \
	token.a log.a multi.a 

UNIT_TESTS/t_fail_parse/failparse.o:\
cc-compile UNIT_TESTS/t_fail_parse/failparse.c UNIT_TESTS/t_assert.h udoc.h \
log.h 
	./cc-compile UNIT_TESTS/t_fail_parse/failparse.c

UNIT_TESTS/t_fail_part/failpart:\
cc-link UNIT_TESTS/t_fail_part/failpart.ld UNIT_TESTS/t_fail_part/failpart.o \
UNIT_TESTS/t_assert.o ud.a token.a log.a multi.a 
	./cc-link UNIT_TESTS/t_fail_part/failpart \
	UNIT_TESTS/t_fail_part/failpart.o UNIT_TESTS/t_assert.o ud.a token.a \
	log.a multi.a 

UNIT_TESTS/t_fail_part/failpart.o:\
cc-compile UNIT_TESTS/t_fail_part/failpart.c UNIT_TESTS/t_assert.h udoc.h \
log.h 
	./cc-compile UNIT_TESTS/t_fail_part/failpart.c

UNIT_TESTS/t_fail_render/failrender:\
cc-link UNIT_TESTS/t_fail_render/failrender.ld \
UNIT_TESTS/t_fail_render/failrender.o UNIT_TESTS/t_assert.o ud.a token.a \
log.a multi.a 
	./cc-link UNIT_TESTS/t_fail_render/failrender \
	UNIT_TESTS/t_fail_render/failrender.o UNIT_TESTS/t_assert.o ud.a \
	token.a log.a multi.a 

UNIT_TESTS/t_fail_render/failrender.o:\
cc-compile UNIT_TESTS/t_fail_render/failrender.c UNIT_TESTS/t_assert.h \
udoc.h log.h 
	./cc-compile UNIT_TESTS/t_fail_render/failrender.c

UNIT_TESTS/t_fail_valid/failvalid:\
cc-link UNIT_TESTS/t_fail_valid/failvalid.ld \
UNIT_TESTS/t_fail_valid/failvalid.o UNIT_TESTS/t_assert.o ud.a token.a log.a \
multi.a 
	./cc-link UNIT_TESTS/t_fail_valid/failvalid \
	UNIT_TESTS/t_fail_valid/failvalid.o UNIT_TESTS/t_assert.o ud.a \
	token.a log.a multi.a 

UNIT_TESTS/t_fail_valid/failvalid.o:\
cc-compile UNIT_TESTS/t_fail_valid/failvalid.c UNIT_TESTS/t_assert.h udoc.h \
log.h 
	./cc-compile UNIT_TESTS/t_fail_valid/failvalid.c

UNIT_TESTS/t_pass_parse/passparse:\
cc-link UNIT_TESTS/t_pass_parse/passparse.ld \
UNIT_TESTS/t_pass_parse/passparse.o UNIT_TESTS/t_assert.o ud.a token.a log.a \
multi.a 
	./cc-link UNIT_TESTS/t_pass_parse/passparse \
	UNIT_TESTS/t_pass_parse/passparse.o UNIT_TESTS/t_assert.o ud.a \
	token.a log.a multi.a 

UNIT_TESTS/t_pass_parse/passparse.o:\
cc-compile UNIT_TESTS/t_pass_parse/passparse.c UNIT_TESTS/t_assert.h udoc.h \
log.h 
	./cc-compile UNIT_TESTS/t_pass_parse/passparse.c

UNIT_TESTS/t_pass_valid/passvalid:\
cc-link UNIT_TESTS/t_pass_valid/passvalid.ld \
UNIT_TESTS/t_pass_valid/passvalid.o UNIT_TESTS/t_assert.o ud.a token.a log.a \
multi.a 
	./cc-link UNIT_TESTS/t_pass_valid/passvalid \
	UNIT_TESTS/t_pass_valid/passvalid.o UNIT_TESTS/t_assert.o ud.a \
	token.a log.a multi.a 

UNIT_TESTS/t_pass_valid/passvalid.o:\
cc-compile UNIT_TESTS/t_pass_valid/passvalid.c UNIT_TESTS/t_assert.h udoc.h \
log.h 
	./cc-compile UNIT_TESTS/t_pass_valid/passvalid.c

UNIT_TESTS/t_token/t_token:\
cc-link UNIT_TESTS/t_token/t_token.ld UNIT_TESTS/t_token/t_token.o \
UNIT_TESTS/t_assert.o token.a 
	./cc-link UNIT_TESTS/t_token/t_token UNIT_TESTS/t_token/t_token.o \
	UNIT_TESTS/t_assert.o token.a 

UNIT_TESTS/t_token/t_token.o:\
cc-compile UNIT_TESTS/t_token/t_token.c UNIT_TESTS/t_assert.h token.h 
	./cc-compile UNIT_TESTS/t_token/t_token.c

cc-compile:\
conf-cc conf-cctype conf-systype conf-cflags flags-corelib flags-chrono 

cc-link:\
conf-ld conf-ldtype conf-systype conf-ldflags libs-chrono-S libs-chrono-C \
libs-corelib-S libs-corelib-C 

cc-slib:\
conf-systype 

conf-cctype:\
conf-systype conf-cc mk-cctype 
	./mk-cctype > conf-cctype.tmp && mv conf-cctype.tmp conf-cctype

conf-ldtype:\
conf-systype conf-ld conf-cctype mk-ldtype 
	./mk-ldtype > conf-ldtype.tmp && mv conf-ldtype.tmp conf-ldtype

conf-sosuffix:\
mk-sosuffix 
	./mk-sosuffix > conf-sosuffix.tmp && mv conf-sosuffix.tmp \
	conf-sosuffix

conf-systype:\
mk-systype 
	./mk-systype > conf-systype.tmp && mv conf-systype.tmp conf-systype

ctxt/bindir.c: mk-ctxt conf-bindir
	rm -f ctxt/bindir.c
	./mk-ctxt ctxt_bindir < conf-bindir > ctxt/bindir.c

ctxt/bindir.o:\
cc-compile ctxt/bindir.c 
	./cc-compile ctxt/bindir.c

ctxt/ctxt.a:\
cc-slib ctxt/ctxt.sld ctxt/bindir.o ctxt/dlibdir.o ctxt/incdir.o \
ctxt/repos.o ctxt/slibdir.o ctxt/version.o 
	./cc-slib ctxt/ctxt ctxt/bindir.o ctxt/dlibdir.o ctxt/incdir.o \
	ctxt/repos.o ctxt/slibdir.o ctxt/version.o 

ctxt/dlibdir.c: mk-ctxt conf-dlibdir
	rm -f ctxt/dlibdir.c
	./mk-ctxt ctxt_dlibdir < conf-dlibdir > ctxt/dlibdir.c

ctxt/dlibdir.o:\
cc-compile ctxt/dlibdir.c 
	./cc-compile ctxt/dlibdir.c

ctxt/incdir.c: mk-ctxt conf-incdir
	rm -f ctxt/incdir.c
	./mk-ctxt ctxt_incdir < conf-incdir > ctxt/incdir.c

ctxt/incdir.o:\
cc-compile ctxt/incdir.c 
	./cc-compile ctxt/incdir.c

ctxt/repos.c: mk-ctxt conf-repos
	rm -f ctxt/repos.c
	./mk-ctxt ctxt_repos < conf-repos > ctxt/repos.c

ctxt/repos.o:\
cc-compile ctxt/repos.c 
	./cc-compile ctxt/repos.c

ctxt/slibdir.c: mk-ctxt conf-slibdir
	rm -f ctxt/slibdir.c
	./mk-ctxt ctxt_slibdir < conf-slibdir > ctxt/slibdir.c

ctxt/slibdir.o:\
cc-compile ctxt/slibdir.c 
	./cc-compile ctxt/slibdir.c

ctxt/version.c: mk-ctxt VERSION
	rm -f ctxt/version.c
	./mk-ctxt ctxt_version < VERSION > ctxt/version.c

ctxt/version.o:\
cc-compile ctxt/version.c 
	./cc-compile ctxt/version.c

deinstaller:\
cc-link deinstaller.ld deinstaller.o insthier.o install_core.o \
install_error.o ctxt/ctxt.a 
	./cc-link deinstaller deinstaller.o insthier.o install_core.o \
	install_error.o ctxt/ctxt.a 

deinstaller.o:\
cc-compile deinstaller.c install.h 
	./cc-compile deinstaller.c

dfo.a:\
cc-slib dfo.sld dfo_column.o dfo_cons.o dfo_cur.o dfo_err.o dfo_free.o \
dfo_init.o dfo_put.o dfo_size.o dfo_tran.o dfo_wrap.o dfo_ws.o 
	./cc-slib dfo dfo_column.o dfo_cons.o dfo_cur.o dfo_err.o \
	dfo_free.o dfo_init.o dfo_put.o dfo_size.o dfo_tran.o dfo_wrap.o \
	dfo_ws.o 

dfo_column.o:\
cc-compile dfo_column.c dfo.h 
	./cc-compile dfo_column.c

dfo_cons.o:\
cc-compile dfo_cons.c dfo.h 
	./cc-compile dfo_cons.c

dfo_cur.o:\
cc-compile dfo_cur.c dfo.h 
	./cc-compile dfo_cur.c

dfo_err.o:\
cc-compile dfo_err.c dfo.h 
	./cc-compile dfo_err.c

dfo_free.o:\
cc-compile dfo_free.c dfo.h 
	./cc-compile dfo_free.c

dfo_init.o:\
cc-compile dfo_init.c dfo.h 
	./cc-compile dfo_init.c

dfo_put.o:\
cc-compile dfo_put.c dfo.h 
	./cc-compile dfo_put.c

dfo_size.o:\
cc-compile dfo_size.c dfo.h 
	./cc-compile dfo_size.c

dfo_tran.o:\
cc-compile dfo_tran.c dfo.h 
	./cc-compile dfo_tran.c

dfo_wrap.o:\
cc-compile dfo_wrap.c dfo.h 
	./cc-compile dfo_wrap.c

dfo_ws.o:\
cc-compile dfo_ws.c dfo.h 
	./cc-compile dfo_ws.c

inst-check:\
cc-link inst-check.ld inst-check.o install_error.o 
	./cc-link inst-check inst-check.o install_error.o 

inst-check.o:\
cc-compile inst-check.c install.h 
	./cc-compile inst-check.c

inst-copy:\
cc-link inst-copy.ld inst-copy.o install_error.o 
	./cc-link inst-copy inst-copy.o install_error.o 

inst-copy.o:\
cc-compile inst-copy.c install.h 
	./cc-compile inst-copy.c

inst-dir:\
cc-link inst-dir.ld inst-dir.o install_error.o 
	./cc-link inst-dir inst-dir.o install_error.o 

inst-dir.o:\
cc-compile inst-dir.c install.h 
	./cc-compile inst-dir.c

inst-link:\
cc-link inst-link.ld inst-link.o install_error.o 
	./cc-link inst-link inst-link.o install_error.o 

inst-link.o:\
cc-compile inst-link.c install.h 
	./cc-compile inst-link.c

install_core.o:\
cc-compile install_core.c install.h 
	./cc-compile install_core.c

install_error.o:\
cc-compile install_error.c install.h 
	./cc-compile install_error.c

installer:\
cc-link installer.ld installer.o insthier.o install_core.o install_error.o \
ctxt/ctxt.a 
	./cc-link installer installer.o insthier.o install_core.o \
	install_error.o ctxt/ctxt.a 

installer.o:\
cc-compile installer.c install.h 
	./cc-compile installer.c

instchk:\
cc-link instchk.ld instchk.o insthier.o install_core.o install_error.o \
ctxt/ctxt.a 
	./cc-link instchk instchk.o insthier.o install_core.o \
	install_error.o ctxt/ctxt.a 

instchk.o:\
cc-compile instchk.c install.h 
	./cc-compile instchk.c

insthier.o:\
cc-compile insthier.c ctxt.h install.h 
	./cc-compile insthier.c

log.a:\
cc-slib log.sld log.o 
	./cc-slib log log.o 

log.o:\
cc-compile log.c log.h 
	./cc-compile log.c

mk-cctype:\
conf-cc conf-systype 

mk-ctxt:\
mk-mk-ctxt 
	./mk-mk-ctxt

mk-ldtype:\
conf-cctype conf-systype 

mk-mk-ctxt:\
conf-cc 

mk-sosuffix:\
conf-systype 

mk-systype:\
conf-cc 

multi.a:\
cc-slib multi.sld multiput.o multicats.o 
	./cc-slib multi multiput.o multicats.o 

multicats.o:\
cc-compile multicats.c multi.h 
	./cc-compile multicats.c

multiput.o:\
cc-compile multiput.c multi.h 
	./cc-compile multiput.c

tok_init.o:\
cc-compile tok_init.c token.h 
	./cc-compile tok_init.c

tok_next.o:\
cc-compile tok_next.c token.h 
	./cc-compile tok_next.c

tok_open.o:\
cc-compile tok_open.c token.h 
	./cc-compile tok_open.c

token.a:\
cc-slib token.sld tok_init.o tok_next.o tok_open.o 
	./cc-slib token tok_init.o tok_next.o tok_open.o 

ud.a:\
cc-slib ud.sld ud_assert.o ud_data.o ud_error.o ud_free.o ud_init.o \
ud_list.o ud_oht.o ud_open.o ud_parse.o ud_part.o ud_ref.o ud_render.o \
ud_table.o ud_tag.o ud_tag_st.o ud_tree.o ud_valid.o udr_context.o \
udr_dump.o udr_nroff.o udr_plain.o udr_xhtml.o 
	./cc-slib ud ud_assert.o ud_data.o ud_error.o ud_free.o ud_init.o \
	ud_list.o ud_oht.o ud_open.o ud_parse.o ud_part.o ud_ref.o \
	ud_render.o ud_table.o ud_tag.o ud_tag_st.o ud_tree.o ud_valid.o \
	udr_context.o udr_dump.o udr_nroff.o udr_plain.o udr_xhtml.o 

ud_assert.o:\
cc-compile ud_assert.c ud_assert.h log.h 
	./cc-compile ud_assert.c

ud_data.o:\
cc-compile ud_data.c ud_tag.h 
	./cc-compile ud_data.c

ud_error.o:\
cc-compile ud_error.c log.h ud_error.h 
	./cc-compile ud_error.c

ud_free.o:\
cc-compile ud_free.c udoc.h 
	./cc-compile ud_free.c

ud_init.o:\
cc-compile ud_init.c ud_part.h ud_oht.h ud_ref.h udoc.h 
	./cc-compile ud_init.c

ud_list.o:\
cc-compile ud_list.c ud_tag.h ud_tree.h 
	./cc-compile ud_list.c

ud_oht.o:\
cc-compile ud_oht.c ud_oht.h 
	./cc-compile ud_oht.c

ud_open.o:\
cc-compile ud_open.c ud_tag.h udoc.h 
	./cc-compile ud_open.c

ud_parse.o:\
cc-compile ud_parse.c multi.h log.h ud_tag.h udoc.h 
	./cc-compile ud_parse.c

ud_part.h:\
gen_stack.h 

ud_part.o:\
cc-compile ud_part.c gen_stack.h log.h ud_assert.h ud_oht.h ud_tag.h \
ud_part.h ud_ref.h udoc.h 
	./cc-compile ud_part.c

ud_ref.o:\
cc-compile ud_ref.c log.h ud_oht.h ud_ref.h udoc.h 
	./cc-compile ud_ref.c

ud_render.h:\
ud_part.h dfo.h 

ud_render.o:\
cc-compile ud_render.c udoc.h ud_assert.h log.h multi.h 
	./cc-compile ud_render.c

ud_table.o:\
cc-compile ud_table.c udoc.h ud_table.h 
	./cc-compile ud_table.c

ud_tag.o:\
cc-compile ud_tag.c ud_tag.h 
	./cc-compile ud_tag.c

ud_tag_st.o:\
cc-compile ud_tag_st.c ud_tag.h 
	./cc-compile ud_tag_st.c

ud_tree.h:\
ud_tag.h 

ud_tree.o:\
cc-compile ud_tree.c log.h ud_assert.h ud_tag.h ud_tree.h udoc.h 
	./cc-compile ud_tree.c

ud_valid.o:\
cc-compile ud_valid.c log.h ud_tag.h ud_tree.h ud_valid.h udoc.h 
	./cc-compile ud_valid.c

udoc:\
cc-link udoc.ld udoc.o ud.a token.a log.a multi.a dfo.a 
	./cc-link udoc udoc.o ud.a token.a log.a multi.a dfo.a 

udoc-conf:\
cc-link udoc-conf.ld udoc-conf.o ctxt/ctxt.a 
	./cc-link udoc-conf udoc-conf.o ctxt/ctxt.a 

udoc-conf.o:\
cc-compile udoc-conf.c ctxt.h _sysinfo.h 
	./cc-compile udoc-conf.c

udoc.h:\
token.h ud_error.h ud_oht.h ud_tree.h ud_render.h 

udoc.o:\
cc-compile udoc.c log.h multi.h udoc.h 
	./cc-compile udoc.c

udr_context.o:\
cc-compile udr_context.c multi.h udoc.h ud_assert.h ud_ref.h 
	./cc-compile udr_context.c

udr_dump.o:\
cc-compile udr_dump.c udoc.h 
	./cc-compile udr_dump.c

udr_nroff.o:\
cc-compile udr_nroff.c multi.h log.h udoc.h ud_assert.h ud_ref.h ud_table.h 
	./cc-compile udr_nroff.c

udr_plain.o:\
cc-compile udr_plain.c dfo.h multi.h udoc.h ud_assert.h ud_ref.h ud_table.h 
	./cc-compile udr_plain.c

udr_xhtml.o:\
cc-compile udr_xhtml.c ud_assert.h udoc.h ud_ref.h log.h multi.h 
	./cc-compile udr_xhtml.c

clean-all: sysdeps_clean tests_clean obj_clean 
clean: obj_clean
obj_clean: 
	rm -f UNIT_TESTS/t_assert.o UNIT_TESTS/t_fail_parse/failparse \
	UNIT_TESTS/t_fail_parse/failparse.o UNIT_TESTS/t_fail_part/failpart \
	UNIT_TESTS/t_fail_part/failpart.o \
	UNIT_TESTS/t_fail_render/failrender \
	UNIT_TESTS/t_fail_render/failrender.o \
	UNIT_TESTS/t_fail_valid/failvalid \
	UNIT_TESTS/t_fail_valid/failvalid.o \
	UNIT_TESTS/t_pass_parse/passparse \
	UNIT_TESTS/t_pass_parse/passparse.o \
	UNIT_TESTS/t_pass_valid/passvalid \
	UNIT_TESTS/t_pass_valid/passvalid.o UNIT_TESTS/t_token/t_token \
	UNIT_TESTS/t_token/t_token.o conf-cctype conf-ldtype conf-systype \
	ctxt/bindir.c ctxt/bindir.o ctxt/ctxt.a ctxt/dlibdir.c \
	ctxt/dlibdir.o ctxt/incdir.c ctxt/incdir.o ctxt/repos.c ctxt/repos.o \
	ctxt/slibdir.c ctxt/slibdir.o ctxt/version.c ctxt/version.o \
	deinstaller deinstaller.o dfo.a dfo_column.o dfo_cons.o dfo_cur.o \
	dfo_err.o dfo_free.o dfo_init.o dfo_put.o dfo_size.o dfo_tran.o \
	dfo_wrap.o dfo_ws.o inst-check inst-check.o inst-copy inst-copy.o \
	inst-dir inst-dir.o inst-link inst-link.o install_core.o \
	install_error.o installer installer.o instchk instchk.o insthier.o \
	log.a log.o mk-ctxt multi.a 
	rm -f multicats.o multiput.o tok_init.o tok_next.o tok_open.o \
	token.a ud.a ud_assert.o ud_data.o ud_error.o ud_free.o ud_init.o \
	ud_list.o ud_oht.o ud_open.o ud_parse.o ud_part.o ud_ref.o \
	ud_render.o ud_table.o ud_tag.o ud_tag_st.o ud_tree.o ud_valid.o \
	udoc udoc-conf udoc-conf.o udoc.o udr_context.o udr_dump.o \
	udr_nroff.o udr_plain.o udr_xhtml.o 

deinstall: deinstaller inst-check inst-copy inst-dir inst-link
	./deinstaller
deinstall-dryrun: deinstaller inst-check inst-copy inst-dir inst-link
	./deinstaller dryrun
install: installer inst-check inst-copy inst-dir inst-link postinstall
	./installer
	./postinstall

install-dryrun: installer inst-check inst-copy inst-dir inst-link
	./installer dryrun
install-check: instchk inst-check
	./instchk
tests:
	(cd UNIT_TESTS && make)
tests_clean:
	(cd UNIT_TESTS && make clean)
regen:
	cpj-genmk > Makefile.tmp && mv Makefile.tmp Makefile

