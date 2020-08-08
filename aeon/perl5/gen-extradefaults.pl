#!/usr/bin/perl -w
# 
# gen-extradefaults.pl : part of the Mace toolkit for building distributed systems
# 
# Copyright (c) 2011, Charles Killian, Dejan Kostic, Ryan Braud, James W. Anderson, John Fisher-Ogden, Calvin Hubble, Duy Nguyen, Justin Burke, David Oppenheimer, Amin Vahdat, Adolfo Rodriguez, Sooraj Bhat
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in the
#      documentation and/or other materials provided with the distribution.
#    * Neither the names of the contributors, nor their associated universities 
#      or organizations may be used to endorse or promote products derived from
#      this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
# USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 
# ----END-OF-LEGAL-STUFF----

use strict;
use File::Basename;
use lib ((dirname($0) || "."), (dirname($0) || ".")."/../mace-extras/perl5");
use Mace::Compiler::GeneratedOn;

print Mace::Compiler::GeneratedOn::generatedOnHeader("Extension Specific Makefile Variables and Rules", "commentLinePrefix" => "#");

my $tgts = join(' ', map { '\1.'.$_.'.o' } @ARGV);

print "\n";
print q|
BASEDEPENDCCRULE=set -e; rm -f $@; $(CXX) -MM $(CXXFLAGS) $< > $@.$$$$; sed 's,\($*\)\.o[ :]*,|.$tgts.q| $@ : ,g' < $@.$$$$ > $@; rm -f $@.$$$$
BASEDEPENDCRULE=set -e; rm -f $@; $(CC) -MM $(CXXFLAGS) $< > $@.$$$$; sed 's,\($*\)\.o[ :]*,|.$tgts.q| $@ : ,g' < $@.$$$$ > $@; rm -f $@.$$$$
ifdef SHOW_COMMANDS
DEPENDCCRULE=$(BASEDEPENDCCRULE)
DEPENDCRULE=$(BASEDEPENDCRULE)
else
DEPENDCCRULE=$(ECHO) " DEP\t$@"; $(BASEDEPENDCCRULE)
DEPENDCRULE=$(ECHO) " DEP\t$@"; $(BASEDEPENDCRULE)
endif
|;

for my $ext (@ARGV) {

#$(PLAINAPPS) : % : %.o $(LIB) $(SERVICE_LIBFILES) $(OTHER_LIBFILES) $(EXTRA_DEP)
#	$(CXX) -pthread -o $@ $< $(LIBS) $(GLOBAL_LINK_FLAGS)

  print "\n# processing extension $ext\n";
  print "${ext}_OBJS = \$(BASE_OBJS:.o=.$ext.o)\n";
  print "${ext}_LINK_LIBS = \$(addsuffix .$ext,\$(addprefix -l,\$(LINK_LIBS)))\n";
  print "${ext}_LINK_LIBFILES = \$(addsuffix .$ext.a,\$(LINK_LIBFILESPRE))\n";
  print "%.$ext.o : %.cc\n" . torulestring("CXX", '$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $('.$ext.'_FLAGS) $< -o $@');
  print "%.$ext.o : %.c\n" . torulestring("CC", '$(CC) -c $(CPPFLAGS) $(CFLAGS) $('.$ext.'_FLAGS) $< -o $@');
  print "\$(LIBDIR)lib\$(LIBNAME).$ext.a : \$(${ext}_OBJS)\n".torulestring("AR", "ar rc \$@ \$^\n");
  print "${ext}_APPS = \$(addsuffix .${ext},\$(APPS))\n";
  print "${ext}_APP_OBJS = \$(addsuffix .${ext}.o,\$(APPS))\n";
  print "${ext}_APP_EOBJS = \$(addsuffix .${ext}.o,\$(APP_EOBJS))\n";
  print "\$(${ext}_APPS) : %.$ext : %.$ext.o \$(${ext}_APP_EOBJS) \$(${ext}_LINK_LIBFILES)\n".torulestring("LD", "\$(CXX) \$(GENERIC_LINK_FLAGS_PRE) \$(${ext}_LINK_FLAGS) -o \$@ \$< \$(${ext}_APP_EOBJS) \$(${ext}_LINK_LIBS) \$(GLOBAL_LINK_FLAGS) \$(GENERIC_LINK_FLAGS_POST)");
  print "CLEAN_LIBS+=\$(LIBDIR)lib\$(LIBNAME).$ext.a\n";
  print "CLEAN_OBJS+=\$(${ext}_OBJS) \$(${ext}_APP_OBJS) \$(${ext}_APP_EOBJS)\n";
  print "CLEAN_APPS+=\$(${ext}_APPS)\n";
  print qq|ifeq "\$(USE_$ext)" "1"\nTYPES+=${ext}\nALL_OBJS+=\$(${ext}_OBJS) \$(${ext}_APP_OBJS)\nALL_LIBS+=\$(LIBDIR)lib\$(LIBNAME).$ext.a\nALL_APPS+=\$(addsuffix .${ext},\$(APPS))\nendif\n|;

}

sub torulestring {
    my $type = shift;
    my $rule = shift;

    my $s = qq|\t\$(ECHO) " $type\\t\$@"\n\t\$(HIDE) $rule\n|;
    
    return $s;
}
