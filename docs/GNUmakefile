# DPS8M documentation: docs/GNUmakefile
# vim: filetype=make:tabstop=4:ai:cc=79:noexpandtab:list:listchars=tab\:\>\-
# SPDX-License-Identifier: MIT
# scspell-id: 09779b94-f632-11ec-8a12-80ee73e9b8e7
# Copyright (c) 2022-2023 The DPS8M Development Team

###############################################################################

.DEFAULT_GOAL := all

###############################################################################

.PHONY: docs all help
.NOTPARALLEL: docs all help
docs all help:
	@printf '%s\n' ""
	@printf '%s\n' "  ###############################################"
	@printf '%s\n' "  #  DPS8M Omnibus Documentation Build Targets  #"
	@printf '%s\n' "  ###############################################"
	@printf '%s\n' ""
	@$(MAKE) -C ".." "help"  |                                                \
      grep 'Makefile.doc)$$' |                                                \
        sed 's/(src\/Makefile.doc)//'
	@printf '%s\n' ""

###############################################################################

.PHONY: docspdf
.NOTPARALLEL: docspdf
docspdf:
	@$(MAKE) --no-print-directory -C ".." "docspdf"
	-@pdfinfo "dps8-omnibus.pdf" 2> "/dev/null" |                             \
      awk '/^Pages:/ { printf("# %s pages written.\n", $$2); }'               \
        2> "/dev/null" | grep '[0-9]' 2> "/dev/null" || true

###############################################################################

.PHONY: docsprt
.NOTPARALLEL: docsprt
docsprt:
	@env ONESIDE=1 $(MAKE) --no-print-directory -C ".." "docsprt"
	-@pdfinfo "dps8-omnibus.pdf" 2> "/dev/null" |                             \
      awk '/^Pages:/ { printf("# %s pages written.\n", $$2); }'               \
        2> "/dev/null" | grep '[0-9]' 2> "/dev/null" || true

###############################################################################

.PHONY: dokdocspdf
.NOTPARALLEL: dokdocspdf
dokdocspdf:
	@$(MAKE) --no-print-directory -C ".." "dokdocspdf"
	-@pdfinfo "dps8-omnibus.pdf" 2> "/dev/null" |                             \
      awk '/^Pages:/ { printf("# %s pages written.\n", $$2); }'               \
        2> "/dev/null" | grep '[0-9]' 2> "/dev/null" || true

###############################################################################

.PHONY: dokdocsprt
.NOTPARALLEL: dokdocsprt
dokdocsprt:
	@env ONESIDE=1 $(MAKE) --no-print-directory -C ".." "dokdocsprt"
	-@pdfinfo "dps8-omnibus.pdf" 2> "/dev/null" |                             \
      awk '/^Pages:/ { printf("# %s pages written.\n", $$2); }'               \
        2> "/dev/null" | grep '[0-9]' 2> "/dev/null" || true

###############################################################################

.PHONY: distclean superclean
.NOTPARALLEL: distclean superclean
distclean superclean: clean
	@rm -f ./cmdref.sav                > /dev/null  2>&1 || true
	@rm -f ./input.sav                 > /dev/null  2>&1 || true

###############################################################################

.PHONY: clean
.NOTPARALLEL: clean
clean:
	@rm -f  ./temp1.tmp                > /dev/null  2>&1 || true
	@rm -f  ./temp2.tmp                > /dev/null  2>&1 || true
	@rm -f  ./*.state                  > /dev/null  2>&1 || true
	@rm -f  ./.*.state                 > /dev/null  2>&1 || true
	@rm -f  ./yaml/docinfo-post.yml    > /dev/null  2>&1 || true
	@rm -f  ./*.pdf                    > /dev/null  2>&1 || true
	@rm -f  ./*.tmp                    > /dev/null  2>&1 || true
	@rm -f  ./stage*.zip               > /dev/null  2>&1 || true
	@rm -f  ./complete.out             > /dev/null  2>&1 || true
	@rm -f  ./md/commandref.md         > /dev/null  2>&1 || true
	@rm -f  ./md/commandref.out        > /dev/null  2>&1 || true
	@rm -rf ./temp.out                 > /dev/null  2>&1 || true
	@rm -rf ./s1epub3.tmp              > /dev/null  2>&1 || true
	@rm -rf ./s1temp.tmp               > /dev/null  2>&1 || true

###############################################################################

.PHONY: dep depend
.NOTPARALLEL: dep depend
dep depend:
	@true

###############################################################################

.PHONY: strip
.NOTPARALLEL: strip
strip:
	@true

###############################################################################

.PHONY: cleandep depclean cleandepend dependclean
.NOTPARALLEL: cleandep depclean cleandepend dependclean
cleandep depclean cleandepend dependclean:
	@true

###############################################################################

# Local Variables:
# mode: make
# tab-width: 4
# End:
