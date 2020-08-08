#
# Param.pm : part of the Mace toolkit for building distributed systems
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
package Mace::Compiler::Param;

use strict;

use Mace::Compiler::Type;
use Mace::Util qw(:all);

use Class::MakeMethods::Template::Hash 
    (
     'new' => 'new',
     'string' => "name",
     'object' => ["type" => { class => "Mace::Compiler::Type"}],
     'object' => ["typeSerial" => { class => "Mace::Compiler::Type"}],
     'boolean' => "hasDefault",
     'boolean' => "hasExpression",
     'string' => "default",
     'string' => "expression",
     'array' => 'arraySizes',
     'string' => 'filename',
     'number' => 'line',
     'string' => 'printNodePointer',
     'boolean' => "shouldLog",
     'array_of_objects' => ["typeOptions" => { class => "Mace::Compiler::TypeOption" }],
     'hash --get_set_items' => 'flags',
     );

sub toString {
#known accepted flags (passes through all):
#  noline
#  noid
#  nodefaults
#  paramconst
#  paramref
#  notype
    my $this = shift;
    my %args = @_;
    my $r = "";
    if ((not $args{noline}) and defined $this->filename() and $this->filename() ne "") {
        my $line = $this->line();
        my $file = $this->filename();
        $r .= qq{\n#line $line "$file"\n};
    }
    if($this->type() and !$args{notype}) {
        if ($this->flags('mutable') and $args{mutable}) {
            $r .= 'mutable ';
        }
        $r .= $this->type()->toString(%args) . " ";
    }
    $r .= $this->name() if(!$args{noid});
    if (scalar($this->arraySizes())) {
        $r .= join("", map { "[" . $_ . "]" } $this->arraySizes());
    }
    if ($this->hasExpression()) {
        $r .= $this->expression();
    }
    if (!$this->hasExpression() && $this->hasDefault() && !$args{nodefaults}) {
        $r .= " = " . $this->default();
    }
    if ((not $args{noline}) and defined $this->filename() and $this->filename() ne "") {
        $r .= qq{\n// __INSERT_LINE_HERE__ \n};
    }
    return $r;
}				# toString

sub validateTypeOptions {
    my $this = shift;
    my $config = shift;
    my $name;
    my $value;
    my %fieldoptions;
    $this->flags('serialize', 1);
    $this->flags('dump', 1);
    $this->flags('dumpHash', 0);
    $this->flags('state', 1);
    $this->flags('onChange', 0);
    $this->flags('reset', 1);
    $this->flags('mutable', 0);
    $this->flags('notComparable', 0);
    foreach my $option ($this->typeOptions()) {
	Mace::Compiler::Globals::error('bad_type_option', $option->file(), $option->line(), "Cannot define option ".$option->name()." for field ".$this->name()." more than once!", $option->file()) unless(++$fieldoptions{$option->name()} == 1);

	if ($option->name() eq 'serialize') {
        # we can serialize state variables now. So serialize option should be ok. Therefore, commenting this out
	    # Mace::Compiler::Globals::error('bad_type_option', $option->file(), $option->line(), "cannot define serialize here") if(!$config->{serialize});
	    while (($name, $value) = each(%{$option->options()})) {
		if ($name eq 'no') {
		    $this->flags('serialize', 0);
		}
		# elsif (! $name eq 'yes') {
		#     Mace::Compiler::Globals::error('bad_type_option', $option->file(), $option->line(), "Invalid option with name $name and value $value");
		# }
	    }
	}
	elsif ($option->name() eq 'mutable') {
	    while (($name, $value) = each(%{$option->options()})) {
		if ($name eq 'yes') {
		    $this->flags('mutable', 1);
		}
		elsif ($name ne 'no') {
		    Mace::Compiler::Globals::error('bad_type_option', $option->file(), $option->line(), "Invalid option with name reset and value $value");
		}
	    }
	}
	elsif ($option->name() eq 'notComparable') {
	    while (($name, $value) = each(%{$option->options()})) {
		if ($name eq 'yes') {
		    $this->flags('notComparable', 1);
		}
		elsif ($name ne 'no') {
		    Mace::Compiler::Globals::error('bad_type_option', $option->file(), $option->line(), "Invalid option with name reset and value $value");
		}
	    }
	}
	elsif ($option->name() eq 'dump') {
	    if (!defined($option->options("state"))) {
		$option->options("state", "");
	    }
	    if ($option->options("state") eq "no") {
		$this->flags("state", 0);
	    }
	    elsif ($option->options("state") ne "yes") {
		$this->flags("state", $option->options("state"));
	    }
	    delete $option->options()->{"state"};
	    while (($name, $value) = each(%{$option->options()})) {
		if ($name eq 'no') {
		    $this->flags('dump', 0);
		}
		elsif ($name eq 'hash') {
		    $this->flags("dumpHash", 1);
		}
		elsif ($name ne 'yes') {
		    #           Mace::Compiler::Globals::error('bad_type_option', $option->line(), "Invalid option with name $name and value $value");
		    $this->flags("dump", $name);
		}
	    }
	}
	elsif ($option->name() eq 'printNode') {
	    if (defined($option->options("pointer"))) {
		$this->printNodePointer($option->options("pointer"));
	    }
	    else {
		Mace::Compiler::Globals::error('bad_type_option', $option->file(), $option->line(), "pointer is the only supported printNode attribute");
	    }
	}
	elsif ($option->name() eq 'reset') {
	    while (($name, $value) = each(%{$option->options()})) {
		if ($name eq 'no') {
		    $this->flags('reset', 0);
		}
		elsif ($name ne 'yes') {
		    Mace::Compiler::Globals::error('bad_type_option', $option->file(), $option->line(), "Invalid option with name reset and value $value");
		}
	    }
	}
	elsif ($option->name() eq 'onChange') {
	    Mace::Compiler::Globals::error('bad_type_option', $option->file(), $option->line(), "onChange no longer a supported attribute");
            next;
	    # Mace::Compiler::Globals::error('bad_type_option', $option->line(), "cannot define onChange here") unless($config->{'onChange'});
	    # $this->flags('onChange','var');
	    # my $hasFunction = 0;
	    # while (($name, $value) = each(%{$option->options()})) {
	    #     if ($name eq 'type') {
	    #         if ($value eq 'collection') {
	    #     	Mace::Compiler::Globals::warning('unimplemented', $option->line(), "onChange for collection not yet implemented!");
	    #     	$this->flags('onChange','collection');
	    #         }
	    #         else {
	    #     	Mace::Compiler::Globals::error('bad_type_option', $option->line(), "value $value for sub-option 'type' invalid") unless($value eq 'var');
	    #         }
	    #     }
	    #     elsif ($name eq 'function') {
	    #         $this->flags('onChangeFunc',$value);
	    #         $hasFunction = 1;
	    #     }
	    #     else {
	    #         Mace::Compiler::Globals::error('bad_type_option', $option->line(), "Invalid option with name $name and value $value");
	    #     }
	    # }
	    # Mace::Compiler::Globals::error('bad_type_option', $option->line(), "Required sub-option 'function' not defined for option 'onChange'") unless($hasFunction);
	}
	elsif ($option->name() eq 'periodic_action') {
	    Mace::Compiler::Globals::warning('unimplemented', $option->file(), $option->line(), "option periodic_action not yet supported");
	    Mace::Compiler::Globals::error('bad_type_option', $option->file(), $option->line(), "cannot define periodic_action here") unless($config->{'periodic_action'});
	    $this->flags('periodic_action', $this->flags('periodic_action')+1);
	}
	elsif ($option->name() eq 'fail_detect') {
	    Mace::Compiler::Globals::warning('unimplemented', $option->file(), $option->line(), "option fail_detect not yet supported");
	}
	elsif ($option->name() eq 'track') {
	    Mace::Compiler::Globals::warning('unimplemented', $option->file(), $option->line(), "option track not yet supported");
	}
	else {
	    Mace::Compiler::Globals::error('bad_type_option', $option->file(), $option->line(), "Invalid option ".$option->name());
	}
    }
}
    
sub getDefault {
    my $this = shift;
    if ($this->hasDefault()) {
	return $this->default();
    }
    elsif ($this->type()->type() =~ /\*$/) {
	return "NULL";
    }
    return $this->type()->name() . "()";
}

sub toPrintNode {
    my $this = shift;
    my $pr = shift;
    my $name = $this->name();
    my $type = $this->type()->name();
    if ($this->flags("dump")) {
	if ($this->count_arraySizes()) {
	    my $node = "__" . $name . "Printer";
	    my $r = "mace::PrintNode $node($name, $type);\n";
	    my $head ="";
	    my $tail = "";
	    my $item = $name;
	    my $iname = "";
	    my $depth = 0;
	    for my $size ($this->arraySizes()) {
		$head .= "for (int i$depth = 0; i$depth < $size; i$depth++) {\n";
		$item .= "[i$depth]";
		$iname .= qq("[" + StrUtil::toString(i$depth) + "]");
		$tail .= "}\n";
		$depth++;
	    }
	    $r .= "$head mace::printItem($node, $iname, &($item)); $tail";
	    $r .= "$pr.addChild($node);\n";
	    return $r;
	}
	else {
	    return qq{mace::printItem($pr, "$name", &($name));};
	}
    }
    elsif ($this->printNodePointer()) {
	return qq{mace::printItem($pr, "$name", } . $this->printNodePointer() . ");";
    }
    return "";
}

sub toPrint {
    my $this = shift;
    my $out = shift;
    my $doxml = shift || 0;
    if ($this->flags('dump')) {
	my $name = $this->name();
	my $head = "";
	my $tail = "";
        my $item = "";
	if ($this->flags('dump') ne "1") {
	    $item .= "(" . $this->flags("dump") . ")";
	}
	$item .= $this->name();
	my $depth = 0;
	for my $size ($this->arraySizes()) {
            if ($doxml) {
                $head .= qq/$out << "<array>";
                            for(int i$depth = 0; i$depth < $size; i$depth++) {
                              $out << " ";
                            /;
                $item .= "[i$depth]";
                $tail .= qq/}
                            $out << "<\/array>";
                            /;
            }
            else {
                $head .= qq/$out << "[";
                            for(int i$depth = 0; i$depth < $size; i$depth++) {
                              $out << " ";
                            /;
                $item .= "[i$depth]";
                $tail .= qq/}
                            $out << "]";
                            /;
            }
	}
        if ($doxml) {
            return qq/$out << "<$name>"; $head { std::string __s; mace::printXml(__s, &($item), ($item)); $out << __s; } $tail; $out << "<\/$name>"; /;
        }
	elsif ($this->flags("dumpHash")) {
	    return qq/hash_string _hasher; $out << "$name=(hash)" << _hasher($item); $tail/;
	}
	else {
            return qq/$out << "$name="; $head mace::printItem($out, &($item)); $tail/;
        }
    }
    return '';
}

sub toPrintState {
    my $this = shift;
    my $out = shift;
    if ($this->flags('state')) {
	my $name = $this->name();
	my $head = "";
	my $tail = "";
        my $item = "";
	if ($this->flags('state') ne "1") {
	    $item .= "(" . $this->flags("state") . ")";
	}
	$item .= $this->name();
	my $depth = 0;
	for my $size ($this->arraySizes()) {
	    $head .= qq/$out << "[";
                        for(int i$depth = 0; i$depth < $size; i$depth++) {
                          $out << " ";
                        /;
	    $item .= "[i$depth]";
	    $tail .= qq/}
                        $out << "]";
                        /;
	}
	return qq/$out << "$name="; $head mace::printState($out, &($item), ($item)); $tail/;
    }
    return '';
}

sub toSerialize {
    my $this = shift;
    my $str = shift;
    my %opt = @_;
    if ($this->flags('serialize')) {
	my $name = $this->name();
	my $s;
	if ((not $opt{noline}) and defined $this->filename() and $this->filename() ne "") {
	    my $line = $this->line();
	    my $file = $this->filename();
	    $s .= qq{\n#line $line "$file"\n};
	}
	$s .= qq{mace::serialize($str, &$name);};
	if ((not $opt{noline}) and defined $this->filename() and $this->filename() ne "") {
	    $s .= qq{\n// __INSERT_LINE_HERE__ \n};
	}
	return $s;
    }
    return '';
}

sub toDeserialize {
    my $this = shift;
    my $str = shift;
    my %opt = @_;
    my $prefix = "";
    if($opt{prefix}) { $prefix = $opt{prefix}; }
    if ($this->flags('serialize')) {
	my $name = $this->name();
	my $idprefix = "";
	if ($opt{idprefix}) {
	    $idprefix = $opt{idprefix};
	}
	my $s;
	if ((not $opt{noline}) and defined $this->filename() and $this->filename() ne "") {
	    my $line = $this->line();
	    my $file = $this->filename();
	    $s .= qq{\n#line $line "$file"\n};
	}
	$s .= qq{$prefix mace::deserialize($str, &$idprefix$name);};
	if ((not $opt{noline}) and defined $this->filename() and $this->filename() ne "") {
	    $s .= qq{\n// __INSERT_LINE_HERE__ \n};
	}
	return $s;
    }
    return '';
}

sub eq {
    my $this = shift;
    my $other = shift;
    return $this->type()->eq($other->type());
} # eq

1;
