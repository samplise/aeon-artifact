# 
# ServiceClass.pm : part of the Mace toolkit for building distributed systems
# 
# Copyright (c) 2011, Charles Killian, James W. Anderson, Adolfo Rodriguez, Dejan Kostic
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
package Mace::Compiler::ServiceClass;

use strict;

use Mace::Compiler::Method;
use Mace::Compiler::AutoType;

use Class::MakeMethods::Template::Hash 
    (
     'new' => 'new',
     'string' => "name",
     'array_of_objects' => ["methods" => { class => "Mace::Compiler::Method" }],
     'array_of_objects' => ["constructors" => { class => "Mace::Compiler::Method" }],
     'array_of_objects' => ["auto_types" => { class => "Mace::Compiler::AutoType" }],
     'object' => ["destructor" => { class => "Mace::Compiler::Method" }],
     'array' => "superclasses",
     'array' => "handlers",
     'boolean' => "isServiceClass",
     'boolean' => "isHandler",
     'boolean' => "isAutoType",
     'boolean' => "isNamespace",
     'string --get_concat' => "pre",
     'string --get_concat' => "post",
     'string __get_concat' => "maceLiteral",
     'array' => "literals",
     );

sub className {
    my $this = shift;
    my $ext = "";
    if ($this->isHandler) {
	$ext = "Handler";
    }
    elsif ($this->isServiceClass()) {
	$ext = "ServiceClass";
    }
    return $this->name() . $ext;
} # className

sub allMethods {
    my $this = shift;
    my @r = ();
    if ($this->count_constructors()) {
# 	print "has " . $this->count_constructors() . " constructor(s)\n";
	push(@r, @{$this->constructors()});
    }
    if ($this->hasDestructor()) {
# 	print "has destructor!\n";
	push(@r, $this->destructor());
    }
    if ($this->count_methods()) {
# 	print "has " . $this->count_methods() . " method(s)\n";
	push(@r, @{$this->methods()});
    }
    return @r;
} # allMethods

sub hasDestructor {
    my $this = shift;
    return defined($this->destructor());
} # hasDestructor

sub toString {
    my $this = shift;
    my $r = "class " . $this->name();
    if ($this->isServiceClass or $this->isHandler()) {
	$r .= " [";
	if ($this->isServiceClass()) {
	    $r .= "ServiceClass";
	}
	if ($this->isHandler()) {
	    $r .= "Handler";
	}
	$r .= "]";
    }
    if ($this->count_superclasses()) {
	$r .= " extends " . join(", ", $this->superclasses());
    }
    $r .= "\n";
    for my $m ($this->allMethods()) {
	$r .= $m->toString() . "\n";
    }
    return $r;
} # toString

1;
