import ply.lex as lex
import os
import sys
import re

import inspect
import copy

import VariableParser
import ParserUtil
import MessageParser

class MethodParseStatus:
    NoneStatus = 0
    ToGetReturnType = 1
    ToGetMethodType = 2
    ToGetMethodName = 3
    ParseMethodArgsDeclaration = 4
    GetMethodDeclarationInfo = 5
    MethodImplementation = 6

class ActorclassMethodInfo:
    def __init__(self):
        self.valid = False
        self.returnType = ''
        self.actorclass = ''
        self.methodType = ''
        self.definedName = ''
        self.generatedName = ''
        self.arguments = []

        self.eventTag = False
        self.asyncTag = False
        self.syncTag = False

        self.implementation = []

    def getReturnType(self):
        return self.returnType

    def getArgs(self):
        return self.arguments

    def markMethodCallType(self, prefix):
        if prefix == 'event':
            self.eventTag = True
        elif prefix == 'async':
            self.asyncTag = True
        else:
            self.syncTag = True

    def generateEventAndAsyncMethodDefinition(self, actorclass):
        print("ActorclassMethodInfo::generateEventAndAsyncMethodDefinition: %s::%s" % (actorclass, self.definedName))
        lines = []

        if self.eventTag:
            line = 'async [%s<%s>] event_%s_%s(const int& %s' % (actorclass, ParserUtil.MyConstants.ActorThisID, actorclass, self.definedName, ParserUtil.MyConstants.ActorThisID)
            for i in range(len(self.arguments)):
                arg = self.arguments[i]
                line = line+', '+arg.generateVarDeclaration()
            line = line + ') [locking=ownership] {'
            lines.append(line) 

            for i in range(len(self.implementation)):
                lines.append(self.implementation[i])
            # lines.append('}')

        if self.asyncTag:
            line = 'broadcast [%s<%s>] async_%s_%s(const int& %s' % (actorclass, ParserUtil.MyConstants.ActorThisID, actorclass, self.definedName, ParserUtil.MyConstants.ActorThisID)
            for i in range(len(self.arguments)):
                arg = self.arguments[i]
                line = line+', '+arg.generateVarDeclaration()
            line = line + ') {'
            lines.append(line) 

            for i in range(len(self.implementation)):
                lines.append(self.implementation[i])
            # lines.append('}') 
        
        return lines

    def generateMainMethodDefinition(self):
        lines = []
        
        line = "async [Root] initRoot() [locking=ownership] {"
        lines.append(line)


        for i in range(len(self.implementation)):
            lines.append(self.implementation[i])
        # lines.append('}')

        return lines


    def generateUpcallDefinition(self):
        # print "ContextMethodInfo::generateUpcallDefinition"
        lines = []
        if self.method_type == 'upcall':
            line = 'upcall delver('
            for i in range(0, len(self.arguments)):
                arg = self.arguments[i]
                if i==0: 
                    line = line+arg.generateVarDeclaration()
                else:
                    line = line+', '+arg.generateVarDeclaration()
            line = line + ') {'
            lines.append(line)
            for i in range(len(self.implementation)):
                lines.append(self.implementation[i])
            lines.append('}')
        return lines

    def generateSyncMethodDefinition(self, actorclass):
        print("ActorclassMethodInfo::generateSyncMethodDefinition: %s::%s" % (actorclass, self.definedName))
        lines = []

        if self.syncTag:
            line = '[%s<%s>] %s sync_%s_%s(const int& %s' % (actorclass, ParserUtil.MyConstants.ActorThisID, self.returnType, actorclass, self.definedName, ParserUtil.MyConstants.ActorThisID)
            for i in range(len(self.arguments)):
                arg = self.arguments[i]
                line = line+', '+arg.generateVarDeclaration()
            line = line + ') {'
            lines.append(line) 

            for i in range(len(self.implementation)):
                lines.append(self.implementation[i])
            # lines.append('}')
        return lines

class MethodBlockInfo:
    def __init__(self):
        self.vars = {}

    def insertVarInfo(self, varName, varInfo):
        #print "Insert var=%s type=%s context=%s id=%s" % (varName, varInfo.varType, varInfo.contextType, varInfo.contextObjId)
        self.vars[varName] = varInfo

    def getVarInfo(self, varName):
        # print "MethodBlockInfo::getVarInfo %s" % varName
        if varName in self.vars.keys():
            varInfo = self.vars[varName]
            return varInfo
        else:
            varInfo = VariableParser.VariableInfo()
            return varInfo  

    def updateVarInfo(self, varName, varInfo):
        if varName in self.vars.keys():
            #print 'Update var=%s context=%s objId=%s' % (varName, varInfo.contextType, varInfo.contextObjId)
            self.vars[varName] = varInfo
            return True
        else:
            return False

class ActorclassMethodParserInfo:
    def __init__(self):
        self.block_infos = []
        self.brace_depth = 0
        self.paren_depth = 0
        self.current_block_info = MethodBlockInfo()

        self.context_type = ''
        self.method_name = ''

        self.actorclass_vars = {}

        self.nextVariable = 0
    
    def nextVariableName(self):
        var_name = '%s_%s_%d' % (self.context_type, self.method_name, self.nextVariable)
        self.nextVariable = self.nextVariable+1
        return var_name

    def createMethodBlock(self):
        self.block_infos.append(self.current_block_info)
        self.current_block_info = MethodBlockInfo()

    def popMethodBlock(self):
        if len(self.block_infos) > 0:
            self.current_block_info = self.block_infos.pop()

    def getVarInfo(self, varName):
        # print("ActorclassMethodParser::getVarInfo %s" % varName)
        
        varInfo = self.current_block_info.getVarInfo(varName)
        if varInfo.type == VariableParser.VariableInfo.InvalidType:
            j = len(self.block_infos) - 1
            while j >= 0:
                varInfo = self.block_infos[j].getVarInfo(varName)
                if varInfo.type != VariableParser.VariableInfo.InvalidType:
                    break
                j = j - 1
        
        if varInfo.type == VariableParser.VariableInfo.InvalidType and varName in self.actorclass_vars:
            return self.actorclass_vars[varName]
        return varInfo

    def updateVarInfo(self, varName, varInfo):
        succ = self.current_block_info.updateVarInfo(varName, varInfo)
        if not succ:
            j = len(self.block_infos) - 1
            while (j >= 0):
                block_info = self.block_infos[j]
                succ = block_info.updateVarInfo(varName, varInfo)
                if succ:
                    break
                j = j - 1
        return succ

    def insertVarInfo(self, varName, varInfo):
        self.current_block_info.insertVarInfo(varName, varInfo)

class ActorclassMethodParser:
    @staticmethod
    def parseMethodDeclaration(line, actorclassName, actorclasses):
        print("ActorclassMethodParser::parseMethodDeclaration")
        method_info = ActorclassMethodInfo()
        status = MethodParseStatus.ToGetReturnType

        arg_tokens = []
        method_info.actorclass = actorclassName
        method_info.valid = True
        method_info.methodType = 'routine'
        paren_depth = 0

        for i in range(len(line)):
            tok = line[i]
            # print tok.value

            if tok.type == 'OPEN_PAREN':
                paren_depth = paren_depth + 1
            elif tok.type == 'CLOSE_PAREN':
                paren_depth = paren_depth - 1

            if status == MethodParseStatus.ToGetReturnType:
                if tok.type == 'NAME' or tok.type == 'COLLECTION_TYPE':
                    method_info.returnType = tok.value
                    status = MethodParseStatus.ToGetMethodName
                else:
                    continue
            elif status == MethodParseStatus.ToGetMethodName:
                if tok.type == 'NAME':
                    method_info.definedName = tok.value
                    status = MethodParseStatus.ParseMethodArgsDeclaration
            elif status == MethodParseStatus.ParseMethodArgsDeclaration:
                if tok.type == 'OPEN_PAREN':
                    pass
                elif tok.type == 'COMMA' and paren_depth == 1 and len(arg_tokens)>0:
                    actorclass_collection_vars = { }
                    variableInfo = VariableParser.VariableParser.parseVariableDeclaration(arg_tokens, actorclasses, actorclass_collection_vars)
                    arg_tokens = []
                    method_info.arguments.append(variableInfo)
                elif tok.type == 'CLOSE_PAREN' and paren_depth == 0 and len(arg_tokens)>0:
                    actorclass_collection_vars = { }
                    variableInfo = VariableParser.VariableParser.parseVariableDeclaration(arg_tokens, actorclasses, actorclass_collection_vars)
                    arg_tokens = []
                    method_info.arguments.append(variableInfo)
                else:
                    arg_tokens.append(tok)
        # print "Get Actorclass Method: type=%s name=%s" % (method_info.methodType, method_info.definedName)
        return method_info

    
    @staticmethod
    def parseActorclassMethodDefinition(program_parser, implementations):
        print("ActorclassMethodParser::parseActorclassMethodDefinition begin: ", program_parser.defined_actorclasses)
        tokens = program_parser.tokens
        status = MethodParseStatus.NoneStatus

        parser_info = ActorclassMethodParserInfo()

        pos = 0
        
        line = []

        while pos < len(tokens):
            tok = tokens[pos]
            print("type=%s, value=%s, pos=%d" % (tok.type, tok.value, pos))

            if tok.type == 'OPEN_BRACE':
                parser_info.brace_depth = parser_info.brace_depth + 1
            elif tok.type == 'CLOSE_BRACE':
                parser_info.brace_depth = parser_info.brace_depth - 1
            elif tok.type == 'OPEN_PAREN':
                parser_info.paren_depth = parser_info.paren_depth + 1
            elif tok.type == 'CLOSE_PAREN':
                parser_info.paren_depth = parser_info.paren_depth - 1

            if status == MethodParseStatus.NoneStatus:
                if tok.type == 'COLON' and ParserUtil.MyUtil.checkContextMethodDefinition(tokens, pos):
                    parser_info.context_type = tokens[pos-1].value
                    parser_info.method_name = tokens[pos+2].value
                    
                    parser_info.actorclass_vars = program_parser.getActorclassVariables(parser_info.context_type)
                    
                    if program_parser.checkActorclassMethodDeclaration(parser_info.context_type, parser_info.method_name):
                        status = MethodParseStatus.GetMethodDeclarationInfo 
                    else:
                        ParserUtil.MyUtil.exitWithError('Fail to find method %s:%s' % (parser_info.context_type, parser_info.method_name) ) 
                elif tok.type == 'OPEN_BRACE':
                    if tokens[pos-3].value == "main":
                        parser_info.context_type = "main"
                        parser_info.method_name = "main"
                        status = MethodParseStatus.MethodImplementation
            elif status == MethodParseStatus.GetMethodDeclarationInfo:
                if tok.type == 'OPEN_BRACE':
                    args = program_parser.getActorclassMethodArgs( parser_info.context_type, parser_info.method_name )
                    for i in range(0, len(args)):
                        if args[i].type == VariableParser.VariableInfo.ActorType or args[i].type == VariableParser.VariableInfo.ActorCollectionType:
                            print('Insert arg: %s of type %s' % (args[i].varName, args[i].varType))
                            parser_info.insertVarInfo(args[i].varName, args[i])                            
                    status = MethodParseStatus.MethodImplementation
            elif status == MethodParseStatus.MethodImplementation:
                line.append(tok)
                if tok.type == 'OPEN_BRACE' or tok.type == 'CLOSE_BRACE':
                    new_lines = ActorclassMethodParser.parseImplementationLine(parser_info, program_parser, line)
                    for j in range(0, len(new_lines)):
                        implementations.append(new_lines[j])

                    line = []
                elif tok.type == 'SEMI_COLON' and parser_info.paren_depth == 0:
                    new_lines = ActorclassMethodParser.parseImplementationLine(parser_info, program_parser, line)
                    for j in range(0, len(new_lines)):
                        implementations.append(new_lines[j])

                    line = []
            pos = pos + 1

        print("ActorclassMethodParser::parseActorclassMethodDefinition end")
        return parser_info

    @staticmethod
    def parseImplementationLine(parser_info, program_parser, input_tokens):
        print("ActorclassMethodParser::parseImplementationLine begin")
        line = []

        for i in range(0, len(input_tokens)):
            tok = ParserUtil.MyToken()
            tok.type = input_tokens[i].type
            tok.value = input_tokens[i].value

            if tok.value == "this":
                tok.type = ParserUtil.TokenType.ACTOR
                tok.value = ParserUtil.MyConstants.ActorThisID
                tok.contextType = parser_info.context_type
                tok.isChildContext = True

            line.append(tok)
        
        prefix_lines = []
        post_lines = []
        output_lines = []

        tok_stack = ParserUtil.TokenStack()
        pos = 0

        n_tab = 0
        is_line_head = True
        while pos < len(line):
            tok = line[pos]
            print("type=%s, value=%s, pos=%d" % ( tok.type, tok.value, pos))

            if tok.type == "TAB" and is_line_head:
                n_tab = n_tab + 1
            else:
                is_line_head = False
        
            if tok.type == 'CLOSE_BRACE':
                parser_info.popMethodBlock()
                pos = pos + 1
                tok_stack.push_back( tok )
            elif tok.type == 'CLOSE_PAREN':
                tok_stack.push_back( tok )
                pos = pos + 1

                toks = tok_stack.popUntilOpenParen()
                new_toks = ActorclassMethodParser.parseTokens(parser_info, program_parser, toks)
                for i in range(0, len(new_toks)):
                    tok_stack.push_back( new_toks[i] )
            elif tok.type == 'COLLECTION_TYPE':
                new_tok = copy.copy(tok)
                # pos = ContextMethodParser.parseContextCollectionOrIterDeclare( parser_info, program_parser, line, pos, new_tok )
                tok_stack.push_back(new_tok)
                # print 'COLLECTION_TYPE: ', new_tok.value
            elif tok.type == 'CLOSE_SQUARE_BRACKET':
                tok_stack.push_back( tok )
                pos = pos + 1

                toks = tok_stack.popUntilOpenSquareBracket()
                new_tok = ActorclassMethodParser.parseToksInSquareBrackets(toks)
                last_tok = tok_stack.end()
                tok_stack.pop()

                print(last_tok.type, ", ", last_tok.contextType)

                last_tok.value = last_tok.value + new_tok.value
                if last_tok.type == ParserUtil.TokenType.ACTOR_COLLECTION:
                    last_tok.type = ParserUtil.TokenType.ACTOR
                else:    
                    last_tok.type = ParserUtil.TokenType.CODE
                tok_stack.push_back( last_tok )    
            elif tok.type in ['EQUALS', 'PLUS', 'MINUS', 'DIVIDE', 'ASTERISK', 'OPEN_PAREN']:
                toks = tok_stack.popUntilCode()
                new_toks = ActorclassMethodParser.parseTokens(parser_info, program_parser, toks)
                for i in range(0, len(new_toks) ):
                    if new_toks[i].type == 'NAME':
                        new_toks[i].type = ParserUtil.TokenType.CODE
                    tok_stack.push_back( new_toks[i] )
                tok_stack.push_back(tok)
                pos = pos + 1
            elif tok.type == 'DOT':
                toks = tok_stack.popUntilCode()
                new_toks = ActorclassMethodParser.parseTokens(parser_info, program_parser, toks)
                for i in range(0, len(new_toks) ):
                    tok_stack.push_back( new_toks[i] )

                toks = tok_stack.getTokens()
                for i in range(len(toks)):
                    print(toks[i].type, ": ", toks[i].value)

                last_tok = tok_stack.end()
                if last_tok.type == ParserUtil.TokenType.ACTOR:
                    tok_stack.pop()

                    prefix_tok = tok_stack.end()
                    prefix = ''

                    if prefix_tok.value == 'async' or prefix_tok.value == 'event':
                        tok_stack.pop()
                        prefix = prefix_tok.value
                    
                    new_tok = ParserUtil.MyToken()
                    pos = ActorclassMethodParser.parseActorMethodCall( parser_info, program_parser, line, pos, last_tok, prefix, new_tok )
                    tok_stack.push_back( new_tok )
                elif last_tok.type == ParserUtil.TokenType.ACTOR_COLLECTION and last_tok.isChildContext:
                    new_tok = ParserUtil.MyToken()
                    pos = ActorclassMethodParser.parseActorCollectionMethod( parser_info, program_parser, line, pos, last_tok, prefix_lines, post_lines, new_tok )
                    tok_stack.pop()
                    tok_stack.push_back( new_tok )
                else:
                    tok_stack.push_back( tok )
                    pos = pos + 1
            elif tok.type == 'NAME':
                if tok.value in ['if', 'while', 'for']:
                    parser_info.createMethodBlock()
                    tok_stack.push_back(tok)
                    pos = pos + 1
                elif tok.value in program_parser.defined_actorclasses:
                    new_tok = ParserUtil.MyToken()
                    pos = ActorclassMethodParser.parseActorclassDeclaration( parser_info, line, pos, new_tok )
                    tok_stack.push_back( new_tok )
                elif tok.value == 'getActor':
                    new_tok = ParserUtil.MyToken()
                    pos = ActorclassMethodParser.parseGetActorAPI( program_parser, line, pos, new_tok)
                    tok_stack.push_back(new_tok)
                elif tok.value == "createActor":
                    new_tok = ParserUtil.MyToken()
                    pos = ActorclassMethodParser.parseCreateActorAPI( parser_info, line, pos, new_tok, prefix_lines)
                    tok_stack.push_back(new_tok)

                    if parser_info.method_name == "main":
                        cname = parser_info.nextVariableName()
                        impl_line = "mace::string %s = generateActorName(\"%s\", %s);" % (cname, new_tok.contextType, new_tok.value)
                        post_lines.append(impl_line)

                        impl_line = "createNewOwnership(\"Root\", %s);" % cname
                        post_lines.append(impl_line)
                elif tok.value == 'NULL':
                    last_toks = tok_stack.lastNTokens(3)
                    if len(last_toks) == 3:
                        if last_toks[0].type == 'EQUALS' and last_toks[1].type == 'EQUALS' and last_toks[2].type == ParserUtil.TokenType.ACTOR:
                            tok.value = '-1'
                    tok_stack.push_back(tok)
                    pos = pos+1
                else:
                    var_info = parser_info.getVarInfo(tok.value)
                   
                    if var_info.type == VariableParser.VariableInfo.ActorType:
                        tok.type = ParserUtil.TokenType.ACTOR
                        tok.contextType = var_info.varType
                        tok.isChildContext = var_info.isChildActor
                    elif var_info.type == VariableParser.VariableInfo.ActorCollectionType:
                        tok.type = ParserUtil.TokenType.ACTOR_COLLECTION
                        tok.contextType = var_info.actorclass
                        tok.collectionType = var_info.collectionType
                        tok.isChildContext = var_info.isChildActor
                    elif var_info.type == VariableParser.VariableInfo.ActorCollectionIterType:
                        tok.type = ParserUtil.TokenType.ACTOR_COLLECTION_ITER
                        tok.contextType = var_info.actorclass
                        tok.collectionType = var_info.collectionType
                        tok.isChildContext = var_info.isChildActor   
                    
                    # else:
                    #     child_var_info = program_parser.getChildContext( parser_info.context_type, tok.value )
                    #     if child_var_info.type == VariableParser.VariableInfo.ContextType:
                    #         tok.type = ParserUtil.TokenType.ACTOR
                    #         tok.contextType = child_var_info.varType
                    #         tok.isChildContext = True
                    #     elif child_var_info.type == VariableParser.VariableInfo.ContextCollectionType:
                    #         #print 'Child CollectionType: ', child_var_info.collectionType
                    #         tok.type = ParserUtil.TokenType.ACTOR_COLLECTION
                    #         tok.contextType = child_var_info.varType
                    #         tok.collectionType = child_var_info.collectionType
                    #         tok.isChildContext = True 
                    
                    tok_stack.push_back(tok)
                    pos = pos + 1
            else:
                tok_stack.push_back(tok)
                pos = pos + 1
                
        
        new_tokens = tok_stack.getTokens()
        output_line = ''
        for i in range(0, len(new_tokens) ):
            tok = new_tokens[i]
            print(tok.type, ": ", tok.value)
            output_line = output_line + tok.value
            if tok.type == 'EQUALS' and new_tokens[i-1].type == ParserUtil.TokenType.ACTOR and new_tokens[i-1].isChildContext:
                post_line = ''
                if new_tokens[i+1].value == 'NULL':
                    pname = parser_info.nextVariableName()
                    post_line = "mace::string %s = generateActorName(\"%s\", %s);" % (pname, parser_info.context_type, ParserUtil.MyConstants.ActorThisID)
                    post_lines.append(post_line)

                    cname = parser_info.nextVariableName()
                    post_line = "mace::string %s = generateActorName(\"%s\", %s);" % (cname, new_tokens[i-1].contextType, new_tokens[i-1].value)
                    post_lines.append(post_line)

                    post_line = 'removeOwnership(%s, %s);' % (pname, cname)
                    post_lines.append(post_line)
                    new_tokens[i+1].value = '-1'
                else:
                    pname = parser_info.nextVariableName()
                    post_line = "mace::string %s = generateActorName(\"%s\", %s);" % (pname, parser_info.context_type, ParserUtil.MyConstants.ActorThisID)
                    post_lines.append(post_line)
                    
                    cname = parser_info.nextVariableName()
                    post_line = "mace::string %s = generateActorName(\"%s\", %s);" % (cname, new_tokens[i-1].contextType, new_tokens[i-1].value)
                    post_lines.append(post_line)
                    
                    post_line = 'createNewOwnership(%s, %s);' % (pname, cname)
                    post_lines.append(post_line)
        
        tabs = ""
        for i in range(0, n_tab):
            tabs = tabs + "\t"

        for i in range(0, len(prefix_lines) ):
            output_lines.append( tabs + prefix_lines[i] )
        output_lines.append(output_line)
        for i in range(0, len(post_lines) ):
            output_lines.append( tabs + post_lines[i] )
        print("ActorclassMethodParser::parseImplementationLine end")
        for i in range(0, len(output_lines)):
            print(output_lines[i])
        return output_lines        

    @staticmethod    
    def parseActorMethodCall( parser_info, program_parser, line, pos, last_tok, prefix, new_tok ):
        print("ActorclassMethodParser::parseActorMethodCall begin")
        
        new_method_name = ParserUtil.MyUtil.generateContextMethodInvocationName( prefix, last_tok.contextType, line[pos+1].value )
        program_parser.markActorclassMethodCallType( prefix, last_tok.contextType, line[pos+1].value )
        
        arg_pos = pos + 2
        args = []
        arg_pos = ParserUtil.MyUtil.extractMethodArgs(line, arg_pos, args)
        
        str_line = ''
        str_line = str_line + new_method_name
        str_line = str_line + '('
        str_line = str_line + last_tok.value
        for arg in args:
            str_line = str_line + ', '
            str_line = str_line + arg.value

        str_line = str_line + ')'
        new_tok.value = str_line
        new_tok.type = ParserUtil.TokenType.CODE

        return_type = program_parser.getActorclassMethodReturnType(last_tok.contextType, line[pos+1].value)
        var_info = VariableParser.VariableParser.analyzeActorclassType(return_type, program_parser.defined_actorclasses)
        if prefix == '':
            if var_info.type == VariableParser.VariableInfo.ActorType:
                new_tok.type = ParserUtil.TokenType.ACTOR
                new_tok.contextType = return_type
            elif var_info.type == VariableParser.VariableInfo.ActorCollectionType:
                new_tok.type = ParserUtil.TokenType.ACTOR_COLLECTION
                new_tok.contextType = var_info.varType
                new_tok.collectionType = var_info.collectionType
            elif var_info.type == VariableParser.VariableInfo.ActorCollectionIterType:
                new_tok.type = 'ContextCollectionIter'
                new_tok.contextType = var_info.varType
                new_tok.collectionType = var_info.collectionType
        
        pos = arg_pos    
        print("ActorclassMethodParser::parseActorMethodCall end")
        return pos
        

    @staticmethod
    def parseGetActorAPI(program_parser, line, pos, new_tok):
        print("ActorclassMethodParser::parseGetActor begin")
        
        if pos+6 >= len(line) or line[pos+1].type != 'SMALL' or line[pos+2].type != 'NAME' or line[pos+3].type != 'LARGE':
            ParserUtil.MyUtil.exitWithError('Wrong for getActorAPI')

        new_tok.type = ParserUtil.TokenType.ACTOR
        new_tok.contextType = line[pos+2].value
        new_tok.value = line[pos+5].value
                
        pos = pos + 7
        return pos

    @staticmethod
    def parseCreateActorAPI(parser_info, line, pos, new_tok, prefix_lines):
        print("ActorclassMethodParser::parseCreateActorAPI begin")
        
        if pos+5 >= len(line) or line[pos+1].type != 'SMALL' or line[pos+2].type != 'NAME' or line[pos+3].type != 'LARGE':
            ParserUtil.MyUtil.exitWithError('Wrong for createActor API')

        if line[pos+5].type == 'CLOSE_PAREN':
            temp_var_name = parser_info.nextVariableName()
            impl_line = "int %s = createNewContext(\"%s\");" % (temp_var_name, line[pos+2].value)
            prefix_lines.append(impl_line)
            
            new_tok.type = ParserUtil.TokenType.ACTOR
            new_tok.value = temp_var_name
            new_tok.contextType = line[pos+2].value

            pos = pos + 6                      
        else:
            new_tok.type = ParserUtil.TokenType.ACTOR
            new_tok.contextType = line[pos+2].value
            new_tok.value = line[pos+5].value
                            
            pos = pos + 7
        print("ActorclassMethodParser::parseCreateActorAPI end")
        return pos

    @staticmethod
    def parseTokens(parser_info, program_parser, toks):
        print("ActorclassMethodParser::parseTokens begin")
        if len(toks) == 0 or len(toks) == 1:
            print("ActorclassMethodParser::parseTokens len=%d" % len(toks))
            return toks
        tokens = toks
        _continue = True

        output_tokens = []
        while _continue:
            _continue = False
            new_tokens = []
            i = 0
            while i < len(tokens):
                curr_tok = tokens[i]
                print("type=%s, value=%s, pos=%d" % ( curr_tok.type, curr_tok.value, i))
                if curr_tok.type == 'NAME':
                    varInfo = parser_info.getVarInfo(curr_tok.value)
                    if varInfo.type == VariableParser.VariableInfo.ActorType:
                        curr_tok.type = ParserUtil.TokenType.ACTOR
                        curr_tok.contextType = varInfo.varType
                        curr_tok.isChildContext = varInfo.isChildActor
                        _continue = True
                    elif curr_tok.type == VariableParser.VariableInfo.ActorCollectionType:
                        curr_tok.type = ParserUtil.TokenType.ACTOR_COLLECTION
                        curr_tok.contextType = varInfo.varType
                        curr_tok.collectionType = varInfo.collectionType
                        curr_tok.isChildContext = varInfo.isChildActor
                        _continue = True
                    elif curr_tok.type == VariableParser.VariableInfo.ActorCollectionIterType:
                        curr_tok.type = ParserUtil.TokenType.ACTOR_COLLECTION_ITER
                        curr_tok.contextType = varInfo.varType
                        curr_tok.collectionType = varInfo.collectionType  
                        _continue = True
                    
                    new_tokens.append(curr_tok)
                    i = i+1
                elif curr_tok.type == 'ASTERISK':
                    if i+1 < len(tokens) and tokens[i+1].type == ParserUtil.TokenType.ACTOR_COLLECTION_ITER:
                        curr_tok.type = ParserUtil.TokenType.ACTOR
                        curr_tok.value = '*'+tokens[i+1].value
                        i = i+2
                        _continue = True
                    else:
                        i = i+1
                    new_tokens.append( curr_tok )
                elif curr_tok.type == 'REFERENCE':
                    nl = len(new_tokens)
                    if new_tokens[nl-1].type == 'ContextCollectionIter' and new_tokens[nl-1].collectionType == 'map':
                        new_tokens[nl-1].type = ParserUtil.TokenType.ACTOR
                        new_tokens[nl-1].value = new_tokens[nl-1].value+'->'+tokens[i+1].value
                        i = i+2
                        _continue = True
                        # print new_tokens[nl-1].isChildContext
                    else:
                        new_tokens.append(curr_tok)
                        i = i+1
                elif curr_tok.type == 'PLUS':
                    nl = len(new_tokens)
                    if nl>1 and new_tokens[nl-1].type == 'ContextCollectionType' and i+1 < len(tokens) and tokens[i+1].type == 'PLUS':
                        new_tokens[nl-1].value = new_tokens[nl-1].value + '++'
                        i = i+2
                        _continue = True
                    else:
                        new_tokens.append( curr_tok )
                        i = i+1
                elif curr_tok.type == 'MINUS':
                    nl = len(new_tokens)
                    if nl>1 and new_tokens[nl-1].type == 'ContextCollectionType' and i+1 < len(tokens) and tokens[i+1].type == 'MINUS':
                        new_tokens[nl-1].value = new_tokens[nl-1].value + '--'
                        i = i+2
                        _continue = True
                    else:
                        new_tokens.append( curr_tok )
                        i = i+1    
                elif curr_tok.type == 'OPEN_PAREN':
                    if i+2<len(tokens) and tokens[i+2].type == 'CLOSE_PAREN':
                        curr_tok.value = '('+tokens[i+1].value+')'
                        curr_tok.type = tokens[i+1].type
                        curr_tok.contextType = tokens[i+1].contextType
                        curr_tok.collectionType = tokens[i+1].collectionType
                        curr_tok.isChildContext = tokens[i+1].isChildContext
                        _continue = True

                        new_tokens.append(curr_tok)
                        i = i+3
                    else:
                        new_tokens.append(curr_tok)
                        i = i+1
                else:
                    new_tokens.append(curr_tok)
                    i = i+1


            if not _continue:
                output_tokens = new_tokens
            else:
                tokens = copy.copy(new_tokens)

        tokens_with_space = []

        for i in range(len(output_tokens)):
            tokens_with_space.append(output_tokens[i])
            if output_tokens[i].type == "NAME" and i+1 < len(output_tokens) and output_tokens[i+1].type == "NAME":
                tok = ParserUtil.MyToken()
                tok.type = "SPACE"
                tok.value = " "
                tokens_with_space.append(tok)

        curr_tok = tokens_with_space[0]
        return_toks = []
        for i in range(1, len(tokens_with_space) ):
            tok = tokens_with_space[i]
            # print(tok.type, ": ", tok.value)
            if tok.type == ParserUtil.TokenType.ACTOR or tok.type == ParserUtil.TokenType.ACTOR_COLLECTION or tok.type == 'ContextCollectionType':
                return_toks.append(curr_tok)
                curr_tok = tok
            elif tok.type == 'NAME':
                if curr_tok.type == 'NAME':
                    curr_tok.value = curr_tok.value + ' ' + tok.value
                    curr_tok.type = ParserUtil.TokenType.CODE
                elif curr_tok.type == ParserUtil.TokenType.ACTOR or curr_tok.type == ParserUtil.TokenType.ACTOR_COLLECTION or curr_tok.type == 'ContextCollectionType':
                    return_toks.append( curr_tok)
                    curr_tok = tok
                    curr_tok.type = ParserUtil.TokenType.CODE
                else:
                    curr_tok.value = curr_tok.value + tok.value
            else:
                if curr_tok.type in [ParserUtil.TokenType.ACTOR, ParserUtil.TokenType.ACTOR_COLLECTION, 'ContextCollectionType']:
                    return_toks.append( curr_tok)
                    curr_tok = tok
                else:
                    curr_tok.value = curr_tok.value + tok.value
                    curr_tok.type = ParserUtil.TokenType.CODE

        return_toks.append(curr_tok)

        print("ActorclassMethodParser::parseTokens end")
        return return_toks

    @staticmethod
    def parseContextCollectionOrIterDeclare( parser_info, program_parser, line, pos, new_tok ):
        # print "ContextMethodParser::parseContextCollectionOrIterDeclare" 
        collection_type_tok = line[pos]
        
        context_types = program_parser.getContextTypes()
        var_info = VariableParser.VariableParser.parseContextCollectionType( collection_type_tok.value, context_types )
        
        if var_info.type != VariableParser.VariableInfo.ContextCollectionType:
            pos = pos + 1
            return pos

        new_pos = pos+1
        new_tok.type = ParserUtil.TokenType.CODE
        if pos+3 < len(line) and line[pos+1].type == 'COLON' and line[pos+2].type == 'COLON':
            new_pos = pos+4
            var_info.type = VariableParser.VariableInfo.ContextCollectionIterType
            new_tok.value = var_info.generateOutputContextCollectionType() + '::' + line[pos+3].value
        
        while new_pos < len(line):
            tok = line[new_pos]
            if tok.type == 'NAME':
                var_info.varName = tok.value
                parser_info.insertVarInfo( tok.value, var_info )
                new_tok.value = new_tok.value + ' ' + tok.value
                new_pos = new_pos + 1
            elif tok.type == 'COMMA':
                new_tok.value = new_tok.value+','
                new_pos = new_pos + 1
            else:
                if tok.type == 'EQUALS' and var_info.type == VariableParser.VariableInfo.ContextCollectionIterType:
                    if program_parser.checkChildContext(parser_info.context_type, line[new_pos+1].value) and line[new_pos+3].value in ['begin', 'rbegin']:
                        new_tok.value = new_tok.value+'='+line[new_pos+1].value+'.'+line[new_pos+3].value+'()'
                        new_pos = new_pos + 6
                        var_info.isChildContext = True
                        parser_info.updateVarInfo( var_info.varName, var_info )
                
                pos = new_pos
                break
        return pos 

    @staticmethod
    def parseActorclassDeclaration( parser_info, line, pos, new_tok ):
        print("ActorclassMethodParser::parseActorclassDeclaration begin") 
        new_pos = pos+1
        new_tok.type = ParserUtil.TokenType.CODE
        new_tok.value = 'int'

        var_info = VariableParser.VariableInfo()
        var_info.type = VariableParser.VariableInfo.ActorType
        var_info.varType = line[pos].value
        
        while new_pos < len(line):
            tok = line[new_pos]
            if tok.type == 'NAME':
                var_info.varName = tok.value
                parser_info.insertVarInfo( tok.value, var_info )
                new_tok.value = new_tok.value + ' ' + tok.value
                new_pos = new_pos + 1
            elif tok.type == 'COMMA':
                new_tok.value = new_tok.value+','
                new_pos = new_pos + 1
            else:
                pos = new_pos
                break
        print('ActorclassMethodParser::parseActorclassDeclaration: ', new_tok.value)
        return pos

    @staticmethod
    def parseToksInSquareBrackets(toks):
        # print 'ContextMethodParser::parseToksInSquareBrackets'

        new_tok = ParserUtil.MyToken()
        new_tok.type = ParserUtil.TokenType.CODE
        new_tok.value = ''

        for i in range(0, len(toks) ):
            new_tok.value = new_tok.value + toks[i].value

        return new_tok

    @staticmethod
    def parseArgTokens( parser_info, program_parser, line ):
        print("ActorclassMethodParser::parseArgTokens begin")
        
        tok_stack = ParserUtil.TokenStack()
        pos = 0
        while pos < len(line):
            tok = line[pos]
            print("%s: %s" % (tok.type, tok.value))
            if tok.type == 'CLOSE_SQUARE_BRACKET':
                tok_stack.push_back( tok )
                pos = pos + 1

                toks = tok_stack.popUntilOpenSquareBracket()
                new_tok = ActorclassMethodParser.parseToksInSquareBrackets(toks)
                last_tok = tok_stack.end()
                tok_stack.pop()

                last_tok.value = last_tok.value + new_tok.value
                if last_tok.type == ParserUtil.TokenType.ACTOR_COLLECTION:
                    last_tok.type = ParserUtil.TokenType.ACTOR
                else:    
                    last_tok.type = ParserUtil.TokenType.CODE
                tok_stack.push_back( new_tok )    
            elif tok.type == 'CLOSE_PAREN':
                tok_stack.push_back( tok )
                pos = pos + 1

                toks = tok_stack.popUntilOpenParen()
                new_toks = ActorclassMethodParser.parseTokens(parser_info, program_parser, toks)
                for i in range(0, len(new_toks)):
                    tok_stack.push_back( new_toks[i] )
            elif tok.type == 'PLUS' or tok.type == 'MINUS' or tok.type == 'DIVIDE' or tok.type == 'ASTERISK':
                toks = tok_stack.popUntilCode()
                new_toks = ActorclassMethodParser.parseTokens(parser_info, program_parser, toks)
                for i in range(0, len(new_toks) ):
                    tok_stack.push_back( new_toks[i] )
                tok_stack.push_back(tok)
                pos = pos + 1
            elif tok.type == 'DOT':
                toks = tok_stack.popUntilCode()
                new_toks = ActorclassMethodParser.parseTokens(parser_info, program_parser, toks)
                for i in range(0, len(new_toks) ):
                    tok_stack.push_back( new_toks[i] )
                last_tok = tok_stack.end()
                if last_tok.type == 'Context':
                    tok_stack.pop()
                    prefix = ''

                    new_tok = ParserUtil.MyToken()                   
                    pos = ActorclassMethodParser.analyzeContextMethod( parser_info, program_parser, line, pos, last_tok, prefix, new_tok )
                    tok_stack.push_back( new_tok )
                else:
                    tok_stack.push_back( tok )
                    pos = pos + 1
            elif tok.type == 'NAME':
                var_info = parser_info.getVarInfo(tok.value)
                if var_info.type == VariableParser.VariableInfo.ActorType:
                    tok.type = ParserUtil.TokenType.ACTOR
                    tok.contextType = var_info.varType
                elif var_info.type == VariableParser.VariableInfo.ActorCollectionType:
                    tok.type = ParserUtil.TokenType.ACTOR_COLLECTION
                    tok.contextType = var_info.actorclass
                    tok.collectionType = var_info.collectionType
                elif var_info.type == VariableParser.VariableInfo.ActorCollectionIterType:     
                    tok.type = 'ContextCollectionIter'
                    tok.contextType = var_info.varType
                    tok.collectionType = var_info.collectionType
                
                tok_stack.push_back(tok)
                pos = pos + 1
                
        new_toks = tok_stack.getTokens()
        new_tok = ParserUtil.MyToken()

        new_tok.type = ParserUtil.TokenType.CODE
        new_tok.value = ''
        for i in range(0, len(new_toks)):
            new_tok.value = new_tok.value+new_toks[i].value
            if new_toks[i].type in [ParserUtil.TokenType.ACTOR, ParserUtil.TokenType.ACTOR_COLLECTION, 'ContextCollectionIter']:
                new_tok.type = new_toks[i].type
                new_tok.contextType = new_toks[i].contextType
                new_tok.collectionType = new_toks[i].collectionType
        print("ActorclassMethodParser::parseArgTokens %s" % new_tok.value)
        return new_tok        

    @staticmethod
    def extractMethodArgs( parser_info, program_parser, tokens, arg_pos, args):
        print("ActorclassMethodParser::extractMethodArgs begin")
        cur_pos = arg_pos
        error_msg = 'Fail to parse for method arguments'
        if cur_pos >= len(tokens) or tokens[cur_pos].type != 'OPEN_PAREN':
            ParserUtil.exitWithError(error_msg)

        tmp_tokens = []
        paren_depth = 0

        while cur_pos < len(tokens):
            push_flag = True
            tok = tokens[cur_pos]
            print("%s: %s" % (tok.type, tok.value))
            if tok.type == 'OPEN_PAREN':
                paren_depth = paren_depth + 1
                if paren_depth == 1:
                    push_flag = False
            elif tok.type == 'CLOSE_PAREN':
                paren_depth = paren_depth - 1
                if paren_depth == 0:
                    push_flag = False
                    if len(tmp_tokens) > 0:
                        newTok = ActorclassMethodParser.parseArgTokens( parser_info, program_parser, tmp_tokens)
                        args.append(newTok)
                    tmp_tokens = []
                    arg_pos = cur_pos + 1
                    break
            elif tok.type == 'COMMA':
                if paren_depth == 1:
                    push_flag = False
                    if len(tmp_tokens) > 0:
                        newTok = ActorclassMethodParser.parseArgTokens( parser_info, program_parser, tmp_tokens)
                        args.append(newTok)
                    tmp_tokens = []
            if push_flag:
                tmp_tokens.append(tok)

            cur_pos = cur_pos + 1

        print("ActorclassMethodParser::extractMethodArgs end")
        return arg_pos

    @staticmethod
    def parseActorCollectionMethod( parser_info, program_parser, line, pos, last_tok, prefix_lines, post_lines, new_tok ):
        print("ActorclassMethodParser::parseActorCollectionMethod begin")

        cur_pos = pos + 1
        method_name = line[cur_pos].value
        cur_pos = cur_pos + 1

        new_tok.type = ParserUtil.TokenType.CODE
        new_tok.value = last_tok.value + '.' + method_name + '('

        args = []
        cur_pos = ActorclassMethodParser.extractMethodArgs( parser_info, program_parser, line, cur_pos, args )
        for i in range(0, len(args) ):
            if i == 0:
                new_tok.value = new_tok.value + args[i].value
            else:
                new_tok.value = new_tok.value+', '+args[i].value
        new_tok.value = new_tok.value+')'
        
        if last_tok.collectionType == 'vector':
            if method_name == 'push_back':
                print('narg=', len(args), ' contextType=', args[0].contextType)
                if len(args) != 1 or args[0].contextType == '':
                    ParserUtil.MyUtil.exitWithError('Error in vector::push_back')
                pname = parser_info.nextVariableName()
                post_line = "mace::string %s = generateActorName(\"%s\", %s);" % (pname, parser_info.context_type, ParserUtil.MyConstants.ActorThisID)
                post_lines.append(post_line)

                cname = parser_info.nextVariableName()
                post_line = "mace::string %s = generateActorName(\"%s\", %s);" % (cname, args[0].contextType, args[0].value)
                post_lines.append(post_line)

                post_line = 'createNewOwnership( %s, %s);' % (pname, cname)
                post_lines.append(post_line)
            elif method_name == 'pop_back':
                var_back_name = parser_info.nextVariableName()
                prefix_line = 'int %s = %s.back();' % ( var_back_name, line[pos-1].value )
                post_line = 'removeOwnership( getMyContextName(), %s[%s] );' % (line[pos-1].contextType, var_back_name)

                prefix_lines.append(prefix_line)
                post_lines.append(post_line)
                
            elif method_name == 'insert':
                if len(args) != 2 or args[1].contextType == '':
                    ParserUtil.MyUtil.exitWithError('Error in vector::insert')
                post_line = 'createNewOwnership( getMyContextName(), %s[%s]);' % (args[1].contextType, args[1].value )
                post_lines.append(post_line)
            elif method_name == 'erase':
                if len(args) != 1 or args[0].contextType == '':
                    ParserUtil.MyUtil.exitWithError('Error in vector::erase')
                post_line = 'removeOwnership( getMyContextName(), %s[*%s]);' % (args[1].contextType, args[1].value )
                post_lines.append(post_line)
        elif last_tok.collectionType == 'deque':
            if method_name == 'push_back' or method_name == 'push_front':
                if len(args) != 1 or args[0].contextType == '':
                    ParserUtil.MyUtil.exitWithError('Error in deque::push_back')
                post_line = 'createNewOwnership( getMyContextName(), %s[%s]);' % (args[0].contextType, args[0].value )
                post_lines.append(post_line)
            elif method_name == 'pop_back':
                var_back_name = parser_info.nextVariable
                prefix_line = 'int %s = %s.back();' % ( var_back_name, line[pos-1].value )
                post_line = 'removeOwnership( getMyContextName(), %s[%s] );' % (line[pos-1].contextType, var_back_name)

                prefix_lines.append(prefix_line)
                post_lines.append(post_line)
            elif method_name == 'pop_front':
                var_back_name = parser_info.nextVariable
                prefix_line = 'int %s = %s.front();' % ( var_back_name, line[pos-1].value )
                post_line = 'removeOwnership( getMyContextName(), %s[%s] );' % (line[pos-1].contextType, var_back_name)

                prefix_lines.append(prefix_line)
                post_lines.append(post_line)
            elif method_name == 'insert':
                if len(args) != 2 or args[1].contextType == '':
                    ParserUtil.MyUtil.exitWithError('Error in deque::insert')
                post_line = 'createNewOwnership( getMyContextName(), %s[%s]);' % (args[1].contextType, args[1].value )
                post_lines.append(post_line)
            elif method_name == 'erase':
                if len(args) != 1 or args[0].contextType == '':
                    ParserUtil.MyUtil.exitWithError('Error in deque::erase')
                post_line = 'removeOwnership( getMyContextName(), %s[*%s])' % (args[1].contextType, args[1].value )
                post_lines.append(post_line) 
        elif last_tok.collectionType == 'set':
            if method_name == 'insert':
                if len(args) != 1 or args[0].contextType == '':
                    ParserUtil.MyUtil.exitWithError('Error in set::insert')
                post_line = 'createNewOwnership( getMyContextName(), %s[%s]);' % (args[1].contextType, args[1].value )
                post_lines.append(post_line)
            elif method_name == 'erase':
                if len(args) != 1 or args[0].contextType == '':
                    ParserUtil.MyUtil.exitWithError('Error in vector::erase')
                post_line = ''
                if args[0].type == 'ContextCollectionIter':
                    post_line = 'removeOwnership( getMyContextName(), %s[*%s]);' % (args[0].contextType, args[0].value )
                else:
                    post_line = 'removeOwnership( getMyContextName(), %s[%s]);' % (args[0].contextType, args[0].value )
                post_lines.append(post_line)
        elif last_tok.collectionType == 'map':
            if method_name == 'erase':
                if len(args) != 1 or args[0].contextType == '':
                    ParserUtil.MyUtil.exitWithError('Error in vector::erase')
                post_line = ''
                if args[0].type == 'ContextCollectionIter':
                    post_line = 'removeOwnership( getMyContextName(), %s[%s] );' % (args[0].contextType, args[0].value+'->second' )
                else:
                    post_line = 'removeOwnership( getMyContextName(), %s[ %s[%s] ] );' % (args[0].contextType, last_tok.value, args[0].value )
                
                post_lines.append(post_line)

        pos = cur_pos
        print("ActorclassMethodParser::parseActorCollectionMethod end")
        return pos

        



