# 
# ContextParam.pm : part of the Mace toolkit for building distributed systems
# 
# Copyright (c) 2012, Wei-Chiu Chuang
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


package Mace::Compiler::ContextParam;

use strict;
use Data::Dumper;

use Class::MakeMethods::Template::Hash
    (
     'new' => 'new',
     'array_of_objects' => ["key" => { class => "Mace::Compiler::Param"}],
     'string' => 'className',
     );
sub toString {
    my $this = shift;
    my $numberof_keys =  $this->count_key();

    my $r = "";
    if( $numberof_keys == 0 ){

    }elsif( $numberof_keys == 1 ){

    }else{
        for my $key ( $this->key() ){
          $key->type->isConst( 0 );
          $key->type->isConst1( 0 );
          $key->type->isConst2( 0 );
          $key->type->isRef( 0 );
        }
        my $declareTypes = join("\n",map{ $_->toString(isConst=>0, isConst1=>0) . ";" } @{ $this->key() } );
        #my $constructorParams = join(",", map{ $_->toString(paramref=>1, nodefaults=>1  ) } @{ $this->key() });
        my $constructorParams = join(",", map{ $_->toString(paramref=>0, paramconst=>1, nodefaults=>1  ) } @{ $this->key() });
        my $copyParams = join(",", map{ "$_->{name}($_->{name} )" } @{ $this->key() });

        my $serializeFields  = join("", map{qq/mace::serialize(__str, &$_->{name});\n/} $this->key()  );
        my $deserializeFields  = join("", map{qq/serializedByteSize += mace::deserialize(__in, &$_->{name});\n/} $this->key() );
        my $serializeBody = qq/
            $serializeFields
        /;
        my $deserializeBody = qq/
          int serializedByteSize = 0;
          $deserializeFields
          return serializedByteSize;
        /;
        my $printParams = join(qq/<<","<</, map{qq/c.$_->{name}/} $this->key()  );
        my $compareParams = join("else ", map{
        qq/if( A.$_->{name} != B.$_->{name} ) return A.$_->{name} < B.$_->{name};
        /} @{ $this->key() } );

$r = qq/
class $this->{className}: public mace::Serializable {
public:
    $this->{className} (  )  { }
    $this->{className} ( $constructorParams ): $copyParams { }
    $declareTypes
    friend std::ostream& operator<<(std::ostream& os, const $this->{className} &c){
        os<<$printParams;
        return os;
    }
    void serialize(std::string& __str) const {
        $serializeBody
    }
    int deserialize(std::istream& __in)  throw (mace::SerializationException){
        $deserializeBody
    }
};
bool operator<( const $this->{className} & A, const $this->{className} & B ){
    $compareParams
    return false;
}
/;
    }

    return $r;
}
1;
