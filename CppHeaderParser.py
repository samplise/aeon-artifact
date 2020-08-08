#!/usr/bin/python
#
# Author: Jashua R. Cloutier (contact via https://bitbucket.org/senex)
# Project: http://senexcanis.com/open-source/cppheaderparser/
#
# Copyright (C) 2011, Jashua R. Cloutier
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in
#   the documentation and/or other materials provided with the
#   distribution.
#
# * Neither the name of Jashua R. Cloutier nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.  Stories,
#   blog entries etc making reference to this project may mention the
#   name Jashua R. Cloutier in terms of project originator/creator etc.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
#
# The CppHeaderParser.py script is written in Python 2.4 and released to
# the open source community for continuous improvements under the BSD
# 2.0 new license, which can be found at:
#
#   http://www.opensource.org/licenses/bsd-license.php
#

import ply.lex as lex
import os
import sys
import re

import inspect
import copy

import ParserUtil
import MessageParser
import MethodParser
import StructureParser
import VariableParser
import ActorclassParser


def lineno():
    """Returns the current line number in our program."""
    return inspect.currentframe().f_back.f_lineno

version = __version__ = "2.4.3"

tokens = [
    'NUMBER',
    'FLOAT_NUMBER',
    'NAME',
    'OPEN_PAREN',
    'CLOSE_PAREN',
    'OPEN_BRACE',
    'CLOSE_BRACE',
    'OPEN_SQUARE_BRACKET',
    'CLOSE_SQUARE_BRACKET',
    'COLON',
    'SEMI_COLON',
    'COMMA',
    'TAB',
    'BACKSLASH',
    'PIPE',
    'PERCENT',
    'EXCLAMATION',
    'CARET',
    'COMMENT_SINGLELINE',
    'COMMENT_MULTILINE',
    'PRECOMP_MACRO',
    'PRECOMP_MACRO_CONT', 
    'ASTERISK',
    'AMPERSTAND',
    'EQUALS',
    'MINUS',
    'PLUS',  
    'DIVIDE', 
    'CHAR_LITERAL', 
    'STRING_LITERAL',
    'NEW_LINE',
    'SQUOTE',
    'REFERENCE',
    'DOT',
    'SMALL',
    'LARGE',
    'STRING',
]

t_ignore = " \r?@\f"
t_NUMBER = r'[0-9][0-9XxA-Fa-f]*'
t_FLOAT_NUMBER = r'[-+]?[0-9]*\.[0-9]+([eE][-+]?[0-9]+)?'
t_NAME = r'[A-Za-z_][A-Za-z0-9_]*'
t_OPEN_PAREN = r'\('
t_CLOSE_PAREN = r'\)'
t_OPEN_BRACE = r'{'
t_CLOSE_BRACE = r'}'
t_OPEN_SQUARE_BRACKET = r'\['
t_CLOSE_SQUARE_BRACKET = r'\]'
t_SEMI_COLON = r';'
t_COLON = r':'
t_COMMA = r','
t_TAB = r'\t'
t_BACKSLASH = r'\\'
t_PIPE = r'\|'
t_PERCENT = r'%'
t_CARET = r'\^'
t_EXCLAMATION = r'!'
t_PRECOMP_MACRO = r'\#.*'
t_PRECOMP_MACRO_CONT = r'.*\\\n'
t_ASTERISK = r'\*'
t_MINUS = r'\-'
t_PLUS = r'\+'
t_DIVIDE = r'/(?!/)'
t_AMPERSTAND = r'&'
t_EQUALS = r'='
t_CHAR_LITERAL = "'.'"
t_SQUOTE = "'"
t_REFERENCE = r'\->'
t_DOT = r'\.'
t_SMALL = r'<'
t_LARGE = r'>'
#found at http://wordaligned.org/articles/string-literals-and-regular-expressions
#TODO: This does not work with the string "bla \" bla"
t_STRING_LITERAL = r'"([^"\\]|\\.)*"'
#Found at http://ostermiller.org/findcomment.html
def t_NEWLINE(t):
    r'\n+'
    t.lexer.lineno += len(t.value)

def t_error(v):
    print(( "Lex error: ", v ))

lex.lex()
# Controls error_print
print_errors = 1
# Controls warning_print
print_warnings = 1
# Controls debug_print
debug = 0
# Controls trace_print
debug_trace = 0

def error_print(arg):
    if print_errors: print(("[%4d] %s"%(inspect.currentframe().f_back.f_lineno, arg)))

def warning_print(arg):
    if print_warnings: print(("[%4d] %s"%(inspect.currentframe().f_back.f_lineno, arg)))

def debug_print(arg):
    global debug
    if debug: print(("[%4d] %s"%(inspect.currentframe().f_back.f_lineno, arg)))

def trace_print(*arg):
    global debug_trace
    if debug_trace:
        sys.stdout.write("[%s] "%(inspect.currentframe().f_back.f_lineno))
        for a in arg: sys.stdout.write("%s "%a)
        sys.stdout.write("\n")

supportedAccessSpecifier = [
    'public',
    'protected', 
    'private'
]

#Symbols to ignore, usually special macros
ignoreSymbols = [
    'Q_OBJECT',
]

doxygenCommentCache = ""

#Track what was added in what order and at what depth
parseHistory = []

class CppParseError(Exception): pass

class ProgramParseStatus:
    NoneStatus = "ProgramNoneStatus"
    ToGetActorclassName = "ToGetActorclassName"
    ActorclassDefinition = "ActorclassDefinition"
    ActorclassMethodDefinition = "ActorclassMethodDefinition"
    MethodDefinition = "MethodDefinition"
    StructureDefinition = "StructureDefinition"
    RoutineDefinition = "RoutineDefinition"
    GetActorclassMethodName = "GetActorclassMethodName"
    MessageDefinition = "MessageDefinition"
    UpcallDefinition = "UpcallDefinition"
    ConfigVariable = "ConfigVariable"

class AEONParser:
    IGNORE_NAMES = '__extension__'.split()

    ## Configurable variable #########################################################################################################
    def parseConfigVariable(self):
        print("AEONParser::parseConfigVariable")
        actorclasses = [ ]
        actorclass_collection_vars = { }
        var_info = VariableParser.VariableParser.parseVariableDeclaration(self.tokens, actorclasses, actorclass_collection_vars)
        self.configVariables[ var_info.varName ] = var_info
        self.tokens = [ ]

    ##################################################################################################################################
   
    def checkContextType(self, ctx_name):
        print("AEONParser::checkContextName: ", ctx_name)

        if ctx_name in self.actorclasses.keys():
            return True
        else:
            return False

    def checkChildContext(self, contextType, varName):
        print('AEONParser::checkChildContext')

        if contextType in self.actorclasses.keys():
            return self.actorclasses[contextType].checkChildContext(varName)
        else:
            return False

    def getChildContext(self, contextType, varName):
        print('AEONParser::checkChildContext')
        var_info = VariableParser.VariableInfo()
        if contextType in self.actorclasses.keys():
            return self.actorclasses[contextType].getChildContext(varName)
        else:
            return var_info

    def getActorclasses(self):
        return self.actorclasses.keys()

    def getContextInfo(self, ctx_name):
        print('AEONParser::getContextInfo: ', ctx_name)
        if ctx_name in self.actorclasses.keys():
            return self.actorclasses[ctx_name]
        else:
            ctxInfo = ActorclassParser.ActorclassInfo()
            return ctxInfo

    def getActorclassMethodReturnType(self, contextType, methodName):
        print("AEONParser::getActorclassMethodReturnType %s::%s" % (contextType, methodName))
        name = "%s::%s" % (contextType, methodName)
        if not name in self.actorclass_methods_returntype:
            ParserUtil.MyUtil.exitWithError("Fail to find %s" % name)
        else:
            return self.actorclass_methods_returntype[name]

    def getActorclassVariables(self, actorcalss):
        if not actorcalss in self.actorclasses.keys():
            return {}
        else:
            return self.actorclasses[actorcalss].getVariables()


    def getActorclassMethodArgs( self, actorclass, method_name ):
        if not actorclass in self.actorclasses.keys():
            print("Fail to find actorclass %s", actorclass)
            return []
        else:
            return self.actorclasses[actorclass].getActorclassMethodArgs(method_name)

    def checkContextMethodDefinition(self, ctx_type):
        print("AEONParser::checkContextMethodDefinition")
        pos = len(self.tokens) - 1
        if pos < 3:
            return False

        tok1 = self.tokens[pos]
        tok2 = self.tokens[pos-1]
        tok3 = self.tokens[pos-2]
        
        if tok1.type!='COLON' or tok2.type!='COLON' or tok3.type!='NAME':
            return False

        if self.checkContextType(ctx_type):
            return self.actorclasses[tok3.value].checkContextMethod(tok3.value)
        else:
            return False

    def checkMethodDefinition(self):
        pos = len(self.tokens) - 1
        if pos < 2 or pos > 3:
            return False

        tok = self.tokens[pos-1]
        if tok.type == 'NAME':
            return True
        else:
            return False

    ## Actorclasses #########################################################################################################
    def parseActorclassDefinition(self):
        print("AEONParser::parseActorclassDefinition")
        actorclass_info = ActorclassParser.ActorclassParser.parse(self.tokens, self.defined_actorclasses, self.actorclass_collection_vars )
        
        if actorclass_info.actorclassName in self.actorclassMethodCallInfos:
            call_infos = self.actorclassMethodCallInfos[actorclass_info.actorclassName]
            for i in range(0, len(call_infos)):
                actorclass_info.markActorclassMethodCallType( call_infos[i].call_prefix, call_infos[i].method_name)
        
        self.tokens = []
        self.actorclasses[ actorclass_info.actorclassName ] = actorclass_info

    def parseStructDefinition(self):
        print("AEONParser::parseStructDefinition: Analyze struct definition")
        structInfo = StructureParser.StructDefinitionParser.parse(self.tokens)
        #print "Get struct %s definition" % structInfo.struct_name
        self.structs[ structInfo.struct_name ] = structInfo
        self.tokens = []

    def parseMessageDefinition(self):
        print("AEONParser::parseMessageDefinition")
        msgInfo = MessageParser.MessageDefinitionParser.parse(self.tokens)
        #print "Get struct %s definition" % structInfo.struct_name
        self.messages[ msgInfo.message_name ] = msgInfo
        self.tokens = []

    def parseActorclassMethodDefinition(self):
        print("AEONParser::parseActorclassMethodDefinition")
        implementations = []
        parser_info = MethodParser.ActorclassMethodParser.parseActorclassMethodDefinition(self, implementations)
        if parser_info.context_type in self.actorclasses.keys():
            self.actorclasses[parser_info.context_type].updateActorclassMethodImplementation(parser_info.method_name, implementations)
        else:
            ParserUtil.MyUtil.exitWithError('Fail to parse actorclass method!!')
        self.tokens = []

    def parseMethodDefinition(self):
        print('AEONParser::parseMethodDefinition')
        implementations = []
        parser_info = MethodParser.ActorclassMethodParser.parseActorclassMethodDefinition(self, implementations)
        
        method_info = MethodParser.ActorclassMethodInfo()
        method_info.returnType = self.tokens[0].value
        method_info.definedName = parser_info.method_name
        method_info.implementation = implementations

        self.methods[method_info.definedName] = method_info
        self.tokens = []

    def parseUpcallDefinition(self):
        print('AEONParser::parseUpcallDefinition:')
        method_info = MethodParser.ContextMethodParser.parseUpcallDefinition(self)
        self.upcall_methods[method_info.define_name] = method_info
        self.tokens = []


    def getOutputContextMethodName(self, contextType, method_name, method_type):
        print("AEONParser::getOutputContextMethodName %s %s %s" % (contextType, method_name, method_type))
        return_method_name = ''
        if contextType in self.contexts.keys():
            return_method_name = self.contexts[contextType].getOutputContextMethodName(method_name, method_type)

        return return_method_name

    def addContextMethodType(self, contextType, method_name, method_type):
        if contextType in self.contexts.keys():
            self.contexts[contextType].addContextMethodType(method_name, method_type)

    def checkActorclassMethodDeclaration(self, actorclass_name, method_name):
        print("AEONParser::checkActorclassMethodDeclaration")
        if actorclass_name not in self.actorclasses.keys():
            print("Fail to find actorclass %s in defined actorclasses." % actorclass_name)
            print(self.actorclasses.keys)
            return False
        ctx_info = self.actorclasses[actorclass_name]
        return ctx_info.checkContextMethod(method_name)

    def markActorclassMethodCallType( self, prefix, contextType, methodName ):
        print("AEONParser::markActorclassMethodCallType actorclass=%s, prefix=%s, methodName=%s" % (contextType, prefix, methodName))
        if contextType in self.actorclasses.keys():
            self.actorclasses[contextType].markActorclassMethodCallType(prefix, methodName)
        else:
            call_info = ActorclassParser.ActorclassMethodCallInfo()
            call_info.call_prefix = prefix
            call_info.method_name = methodName

            if not contextType in self.actorclassMethodCallInfos:
                self.actorclassMethodCallInfos[contextType] = []

            self.actorclassMethodCallInfos[contextType].append(call_info)

    def checkActorclassCycleIter(self, name, children, decendants):
        if name in children:
            return True
        
        for i in range(len(children)):
            if self.checkActorclassCycleIter(name, decendants[children[i]], decendants):
                return True

        return False

    def checkActorclassCycle(self):
        decendants = {}
        anames = []

        for name, actorclass in self.actorclasses.items():
            decendants[name] = actorclass.childActorclasses
            print(name, ": ", decendants[name])
            if name in decendants[name]:
                decendants[name].remove(name)
            anames.append(name)

        for i in range(len(anames)):
            name = anames[i]
            dnames = decendants[name]
            if self.checkActorclassCycleIter(name, dnames, decendants):
                return True

        return False
         

    def generateAEONFile(self):
        print("AEONParser::generateAEONFile")
        filelines = []

        line = "service GameApp;\n\nprovides Null;"
        filelines.append(line)

        line = "services {\n\tTransport t;\n}"
        filelines.append(line)

        line = "constants {\n\n}"
        filelines.append(line)

        line = "constructor_parameters {\n\n}"
        filelines.append(line)

        # filelines.append('auto_type {')
        # for key, value in self.structures.items():
        #     lines = value.generateStructDefinition()
        #     for i in range(len(lines)):
        #         filelines.append(lines[i])
        # filelines.append('}')

        # filelines.append('messages {')
        # for key, value in self.messages.items():
        #     lines = value.generateMessageDefinition()
        #     for i in range(len(lines)):
        #         filelines.append(lines[i])
        # filelines.append('}')

        filelines.append('state_variables {\n')
        tline = "context Root {\n}"
        filelines.append(tline)

        for key, value in self.actorclasses.items():
            lines = value.generateActorclassDefinition()
            for i in range(len(lines)):
                filelines.append(lines[i])
        filelines.append('\n}')
        
        filelines.append('transitions {\n')
        tline = "downcall maceInit(){"
        filelines.append(tline)
        tline = "\tasync_initRoot();"
        filelines.append(tline)
        tline = "}"
        filelines.append(tline)

        for key, value in self.actorclasses.items():
            lines = value.generateEventAndAsyncMethodDefinition()
            for i in range(len(lines)):
                filelines.append(lines[i])

        lines = self.methods["main"].generateMainMethodDefinition()
        for i in range(len(lines)):
            filelines.append(lines[i]) 
        filelines.append('\n}')
        # for key, value in self.upcall_methods.iteritems():
        #     lines = value.generateUpcallDefinition()
        #     for i in range(len(lines)):
        #         filelines.append(lines[i])
        # filelines.append('}')

        filelines.append('routines {')
        for key, value in self.actorclasses.items():
            lines = value.generateSyncMethodDefinition()
            for i in range(len(lines)):
                filelines.append(lines[i])

        line = "[__null] mace::string generateActorName( const mace::string& ctxName, const uint32_t& id) {"
        filelines.append(line)
        line = "\tstd::ostringstream oss;\n\toss << ctxName <<\"[\"<<id<<\"]\";\n\treturn oss.str();\n}"
        filelines.append(line)

        filelines.append('}')

        ofile = open(self.oFileName, "w")
        for i in range(len(filelines)):
            # print(filelines[i])
            ofile.write(filelines[i] + "\n")

        ofile.close()

        


    def __init__(self, appFileName, oFileName):
        self.appFileName = os.path.expandvars(appFileName)
        self.oFileName = os.path.expandvars(oFileName)
        
        appFileStr = ""
                
        self.actorclasses = { }
        self.methods = {}
        self.structures = { }
        self.messages = { }
        self.routines = { }
        self.includeHeadFiles = [ ]
        self.configVariables = { }
        self.precompMacros = [ ]
        
        self.parsed_contexts = { }

        self.actorclassMethodCallInfos = { }
            
        if (len(self.appFileName)):
            fd = open(self.appFileName)
            appFileStr = "".join(fd.readlines())
            fd.close()  

        # Make sure supportedAccessSpecifier are sane
        for i in range(0, len(supportedAccessSpecifier)):
            if " " not in supportedAccessSpecifier[i]: continue
            supportedAccessSpecifier[i] = re.sub("[ ]+", " ", supportedAccessSpecifier[i]).strip()
        
        # Strip out template declarations
        appFileStr = re.sub("template[\t ]*<[^>]*>", "", appFileStr)

        # Change multi line #defines and expressions to single lines maintaining line nubmers
        # Based from http://stackoverflow.com/questions/2424458/regular-expression-to-match-cs-multiline-preprocessor-statements
        is_define = re.compile(r'[ \t\v]*#[Dd][Ee][Ff][Ii][Nn][Ee]')
        
        matches = re.findall(r'(?m)^(?:.*\\\r?\n)+.*$', appFileStr)
        for m in matches:
            #Keep the newlines so that linecount doesnt break
            num_newlines = len([a for a in m if a=="\n"])
            if is_define.match(m):
                new_m = m.replace("\n", "<CppHeaderParser_newline_temp_replacement>\\n")
            else:
                # Just expression taking up multiple lines, make it take 1 line for easier parsing
                new_m = m.replace("\\\n", " ")
            if (num_newlines > 0):
                new_m += "\n"*(num_newlines)
            appFileStr = appFileStr.replace(m, new_m)

        
        #Filter out Extern "C" statements.  These are order dependent
        matches = re.findall(re.compile(r'extern[\t ]+"[Cc]"[\t \n\r]*{', re.DOTALL), appFileStr)
        for m in matches:
            #Keep the newlines so that linecount doesnt break
            num_newlines = len([a for a in m if a=="\n"])
            appFileStr = appFileStr.replace(m, "\n" * num_newlines)        
        appFileStr = re.sub(r'extern[ ]+"[Cc]"[ ]*', "", appFileStr)
                
        #Filter out any ignore symbols that end with "()" to account for #define magic functions
        for ignore in ignoreSymbols:
            if not ignore.endswith("()"): continue
            while True:
                locStart = appFileStr.find(ignore[:-1])
                if locStart == -1:
                    break
                locEnd = None
                #Now walk till we find the last paren and account for sub parens
                parenCount = 1
                inQuotes = False
                for i in range(locStart + len(ignore) - 1, len(appFileStr)):
                    c = appFileStr[i]
                    if not inQuotes:
                        if c == "(":
                            parenCount += 1
                        elif c == ")":
                            parenCount -= 1
                        elif c == '"':
                            inQuotes = True
                        if parenCount == 0:
                            locEnd = i + 1
                            break
                    else:
                        if c == '"' and appFileStr[i-1] != '\\':
                            inQuotes = False
                        
                if locEnd:
                    #Strip it out but keep the linecount the same so line numbers are right
                    match_str = appFileStr[locStart:locEnd]
                    debug_print("Striping out '%s'"%match_str)
                    num_newlines = len([a for a in match_str if a=="\n"])
                    appFileStr = appFileStr.replace(appFileStr[locStart:locEnd], "\n"*num_newlines)

                
        self.braceDepth = 0
        lex.lex()
        lex.input(appFileStr)
        
        
        
        self.current_parse_status = ProgramParseStatus.NoneStatus
        self.tokens = []
        self.defined_actorclasses = []

        brace_depth = 0        
        paren_depth = 0

        small_large_count = 0

        name_space = ''
        name_space_n_colon = 0

        current_actorclass = ''
        current_method_name = ''

        program_tokens = []

        next_is_actorclass = False
        curr_actorclass = ""

        self.actorclass_methods_returntype = {}
        self.actorclass_collection_vars = {}

        # e.g., vector
        collection_type = ""
        # e.g., vector<Item>
        collection_var_type = ""
        # e.g., Item
        actorclass_in_collection_var_type = ""

        try:
            while True:
                tok = lex.token()
                if not tok: break

                if next_is_actorclass:
                    self.defined_actorclasses.append(tok.value)
                    curr_actorclass = tok.value
                    next_is_actorclass = False
                if tok.type == "NAME" and tok.value == "actorclass":
                    next_is_actorclass = True
                elif tok.type == "OPEN_BRACE":
                    brace_depth = brace_depth + 1
                elif tok.type == "CLOSE_BRACE":
                    brace_depth = brace_depth - 1
                    if brace_depth == 0:
                        curr_actorclass = ""
                elif tok.type == "OPEN_PAREN" and brace_depth == 1 and curr_actorclass != "":
                    l = len(program_tokens)
                    method_name = program_tokens[l-1].value
                    method_return_type = program_tokens[l-2].value
                    mname = "%s::%s" % (curr_actorclass, method_name)
                    self.actorclass_methods_returntype[mname] = method_return_type
                    print("Add %s with return type %s" % (mname, method_return_type))
                program_tokens.append(tok)
            
            print(self.defined_actorclasses)
            print(self.actorclass_methods_returntype)

            brace_depth = 0
            for k in range(0, len(program_tokens)):
                tok = program_tokens[k]
                print("type=%s, value=%s, status=%s" % ( tok.type, tok.value, self.current_parse_status))

                if collection_var_type != "" and small_large_count == 0 and actorclass_in_collection_var_type != "" and tok.type == 'NAME':
                    v = VariableParser.VariableInfo()
                    v.type = VariableParser.VariableInfo.ActorCollectionType
                    v.varType = collection_var_type
                    v.varName = tok.value
                    v.collectionType = collection_type
                    v.actorclass = actorclass_in_collection_var_type
                    v.isChildActor = True
                    
                    self.actorclass_collection_vars[ tok.value ] = v
                    print("Add actor collection %s of %s with %s" % (v.varName, v.varType, v.actorclass))

                    collection_type = ""
                    collection_var_type = ""
                    actorclass_in_collection_var_type = ""
                    
                if tok.type == 'PRECOMP_MACRO':
                    self.precompMacros.append(tok.value)
                    continue
                elif tok.type == 'NAME' and tok.value in self.IGNORE_NAMES: 
                    continue
                elif self.current_parse_status == ProgramParseStatus.NoneStatus and tok.type == 'SEMI_COLON':
                    continue

                # process collection type
                if tok.type == 'NAME' and collection_type == '' and tok.value in ['map', 'vector', 'deque', 'set']:
                    collection_type = tok.value
                    collection_var_type = "mace::" + tok.value
                    continue
                elif collection_type != '':
                    if tok.type == 'SMALL':
                        collection_var_type = collection_var_type + tok.value
                        small_large_count = small_large_count + 1
                        continue
                    elif tok.type == 'LARGE':
                        collection_var_type = collection_var_type + tok.value
                        small_large_count = small_large_count - 1
                        if small_large_count == 0:
                            tok.type = 'COLLECTION_TYPE'
                            tok.value = collection_var_type
                            if actorclass_in_collection_var_type == "":
                                collection_type = ""
                                collection_var_type = ""
                        else:
                            continue
                    elif tok.type == 'NAME' and tok.value in self.defined_actorclasses and self.current_parse_status == ProgramParseStatus.ActorclassDefinition:
                        collection_var_type = collection_var_type + "int"
                        actorclass_in_collection_var_type = tok.value
                        continue
                    else:
                        collection_type = collection_type+tok.value
                        continue

                # process types with namespace
                if tok.type == 'NAME' and tok.value in ['std', 'mace']:
                    name_space = tok.value
                    continue
                elif name_space != '' and tok.type == 'COLON':
                    name_space_n_colon = name_space_n_colon + 1
                    continue
                elif name_space_n_colon == 2 and tok.type == 'NAME':
                    tok.value = name_space + '::' + tok.value
                    print("Generated new token: type=%s, value=%s" % (tok.type, tok.value))
                    name_space = ''
                    name_space_n_colon = 0
                elif name_space_n_colon == 2 and tok.type == 'COLLECTION_TYPE':
                    tok.value = name_space + '::' + tok.value
                    print('Generated new token: type=%s, value=%s' % ( tok.type, tok.value))
                    name_space = ''
                    name_space_n_colon = 0                
                
                
                self.tokens.append(tok)
                
                if tok.type == 'OPEN_BRACE':
                    brace_depth = brace_depth + 1
                elif tok.type == 'CLOSE_BRACE':
                    brace_depth = brace_depth - 1
                elif tok.type == 'OPEN_PAREN':
                    paren_depth = paren_depth + 1
                elif tok.type == 'CLOSE_PAREN':
                    paren_depth = paren_depth - 1
                    
                if self.current_parse_status == ProgramParseStatus.NoneStatus:
                    if tok.type == 'NAME':
                        if tok.value == 'configurable':
                            self.current_parse_status = ProgramParseStatus.ConfigVariable
                        elif tok.value == 'actorclass':
                            self.current_parse_status = ProgramParseStatus.ToGetActorclassName
                        elif tok.value == 'struct':
                            self.current_parse_status = ProgramParseStatus.StructDefinition
                        elif tok.value == 'message':
                            self.current_parse_status = ProgramParseStatus.MessageDefinition
                        elif tok.value == 'upcall':
                            self.current_parse_status = ProgramParseStatus.UpcallDefinition
                        elif tok.value == "main":
                            self.current_parse_status = ProgramParseStatus.MethodDefinition
                    elif tok.type == 'COLON':
                        self.current_parse_status = ProgramParseStatus.GetActorclassMethodName
                    elif tok.type == 'OPEN_PAREN':
                        if paren_depth == 1 and self.checkMethodDefinition():
                            self.current_parse_status = ProgramParseStatus.MethodDefinition
                    else:
                        ParserUtil.MyUtil.exitWithError('Syntax error here!')
                elif self.current_parse_status == ProgramParseStatus.ToGetActorclassName:
                    if tok.type == 'NAME':
                        self.current_parse_status = ProgramParseStatus.ActorclassDefinition
                    else:
                       ParserUtil.MyUtil.exitWithError('Fail to get the name of actorclass!!') 
                elif self.current_parse_status == ProgramParseStatus.GetActorclassMethodName:
                    if tok.type == 'NAME':
                        self.current_method_name = tok.value
                        self.current_parse_status = ProgramParseStatus.ActorclassMethodDefinition
                elif self.current_parse_status == ProgramParseStatus.ConfigVariable:
                    if tok.type == 'SEMI_COLON':
                        self.parseConfigVariable()
                        self.current_parse_status = ProgramParseStatus.NoneStatus
                else:
                    if tok.type == 'CLOSE_BRACE' and brace_depth == 0:
                        if self.current_parse_status == ProgramParseStatus.ActorclassDefinition:
                            self.parseActorclassDefinition()
                            self.actorclass_collection_vars.clear()
                            self.current_parse_status = ProgramParseStatus.NoneStatus
                        elif self.current_parse_status == ProgramParseStatus.StructureDefinition:
                            self.parseStructDefinition()
                            self.current_parse_status = ProgramParseStatus.NoneStatus
                        elif self.current_parse_status == ProgramParseStatus.ActorclassMethodDefinition:
                            self.parseActorclassMethodDefinition()
                            self.current_parse_status = ProgramParseStatus.NoneStatus
                        elif self.current_parse_status == ProgramParseStatus.MethodDefinition:
                            self.parseMethodDefinition()
                            self.current_parse_status = ProgramParseStatus.NoneStatus
                        elif self.current_parse_status == ProgramParseStatus.MessageDefinition:
                            self.parseMessageDefinition()
                            self.current_parse_status = ProgramParseStatus.NoneStatus
                        elif self.current_parse_status == ProgramParseStatus.UpcallDefinition:
                            self.parseUpcallDefinition()
                            self.current_parse_status = ProgramParseStatus.NoneStatus

            if self.checkActorclassCycle():
                ParserUtil.MyUtil.exitWithError("There is cycle in actorclass definition!")
            else:
                print("There is no cycle!")
            
            self.generateAEONFile()


        except:
            raise CppParseError("Not able to parse on current line!")
