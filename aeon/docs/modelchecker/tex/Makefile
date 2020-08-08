# This makefile compiles all the tex files in a directory
# it defaults to ps, but pdf or dvi can be specified

IGNORE = %-include.tex

texfiles   := $(filter-out $(IGNORE),$(wildcard *.tex))
dotfiles   := $(wildcard *.dot)
exts	   := ps pdf aux log out dvi bbl blg nav snm toc lof lot
latex_perl  = perl -ne '$s=1,print if /^!/ or $s;END{die "errors\n" if $s}'
define tex_deps_perl
perl -ne 'push @d,$1 if/\\(?:include|input)(?:.tex)?{(.+?)}/;\
END{$,=" ";print @d}'
endef

.PHONY: ps pdf all clean dot
.SUFFIXES: .ps .pdf .dvi .tex .dot .eps
.SECONDARY: 
# all must be the first rule
all: $(texfiles:.tex=.ps)
pdf: $(texfiles:.tex=.pdf)
dot: $(dotfiles:.dot=.eps)

%.eps: %.dot
	@echo 'Converting $* to eps.'
	@dot -Tps2 -o$@ $<

%.pdf: %.ps
	@echo 'Converting $* to pdf.'
	@ps2pdf -dMaxSubsetPct=100 -dPDFSETTINGS=/prepress $< >/dev/null

%.ps: %.dvi
	@echo 'Converting $* to ps.'
	@dvips -t letter -o $@ $< -q

%.dvi: %.tex
	@echo 'Creating the dvi for $*.'
	@latex $* </dev/null | $(value latex_perl)
	@if test -e $*.bib; then \
		echo '  Creating the bbl file.'; \
		bibtex $* >/dev/null; fi
	@for i in 1 2; do \
		if egrep -i -q 'undefined|No file $*.(toc|lof|lot)' $*.log; \
		then \
			echo '  Catching undefined references/citations.'; \
			latex $* > /dev/null ; fi; done

clean:
	@rm -vf $(foreach ext,$(exts),$(texfiles:.tex=.$(ext)))
	@rm -vf *~ $(dotfiles:.dot=.eps)

define compute_deps
$(1:.tex=.dvi): $(addsuffix .tex,$(shell $(value tex_deps_perl) $(1)))
endef
$(foreach file,$(texfiles),$(eval $(call compute_deps,$(file))))
