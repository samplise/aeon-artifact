include perlvars.mk

COMPILE_TARGETS = $(addprefix compile_,$(PERLBINS))

testcompile: $(COMPILE_TARGETS)

compile_% : 
	perl -cw $*
