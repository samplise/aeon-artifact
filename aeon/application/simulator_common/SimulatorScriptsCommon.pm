# 
# SimulatorScriptsCommon.pm : part of the Mace toolkit for building distributed systems
# 
# Copyright (c) 2011, Karthik Nagaraj, Charles Killian
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
package SimulatorScriptsCommon;

use strict;

sub findSimulator {
  # Find the simulator executable by searching various places. Returns the path, else die.

  # Search Environment
  if (defined $ENV{"MACE_SIMULATOR"}
      and -x $ENV{"MACE_SIMULATOR"}) {
    return $ENV{"MACE_SIMULATOR"};
  }

  # Search PATH
  my $simulator_executable_name = "mace_simulator";
  my $path;
  foreach $path (split(/:/, $ENV{"PATH"})) {
    my $filename = $path . "/" . $simulator_executable_name;
    if (-x $filename) {
      return $filename;
    }
  }
  
  die "Cannot locate the Simulator! Try setting MACE_SIMULATOR environment or put mace_simulator in PATH.\n";
}

1;
