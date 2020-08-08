# Welcome

<mace@macesystems.org> / <http://www.macesystems.org>

Thanks for downloading the *ContextLattice* source package.  ContextLattice is an
elastic-oriented programming toolkit based on Mace, which is a toolkit and language
for building distributed systems.  Mace documentation can be found in the **docs/**
directory, and there is a short **README** about the modelchecker in the
**application/modelchecker** directory.  The License is available in the **LICENSE**
file, and is a BSD-style license.

Please email us to let us know if you're using ContextLattice and how, we'd love to hear
from you.


Quick Start:
- - -

ContextLattice uses *cmake* to configure it's build system.  Provided that you have
everything installed already, first create a directory for building ContextLattice.

Then, from within that directory, run `cmake [path-to-mace-checkout]`.  It will
check your configuration, and create the build system.  Then you can run `make`
to build ContextLattice.  If you want to change the configuration parameters, running
`make edit_cache` will open an editor for setting the build flags and other
configuration options.  `make help` will also show you a list of runnable make
commands.  See **docs/mace_HOWTO.pdf** for more details on using cmake.

If you wish to compile fewer services than those distributed, you can edit
**services/CMakeLists-services.txt**.  (You can also add new services here.) You
can also select applications by editing at **application/CMakeLists.txt**, and you
can add new applications anywhere using one of the sample CMakeLists.txt in the
application directories, and either setting up a new cmake project which uses
the ContextLattice project, or which is included in **mace/application/CMakeLists.txt**.

ContextLattice is compatible and tested on g++ 4.1.x to 4.7.x. But it is not tested on g++ 
versions before 3.4, and in particular has been known
to cause internal compiler segfaults with version 3.3.  g++ versions 4.0.x have
also been known, at least on Debian systems, to contain a (reported and
confirmed) bug which causes them to emit warnings when ContextLattice code is compiled
with ``-O2`` and ``-Wall``.  You can either turn off ``-Werror``, or compile with 3.4 or
4.1, 4.2, or higher.
