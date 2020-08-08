import ply.lex as lex
import os
import sys
import re

import inspect
import copy

import VariableParser
import ParserUtil
import MethodParser

class ActorclassParseStatus:
    NoneStatus = 0
    ToGetActorclassName = 1
    GetActorclassName = 2
    ActorclassVarMethodDeclaration = 4

class ActorclassMethodCallInfo:
    def __init__(self):
        self.call_prefix = ""
        self.method_name = ""
    

class ActorclassInfo:
    def __init__(self):
        self.actorclassName = ''
        self.variables = {}
        self.methods = {}
        self.childActorclasses = []

    def getVariables(self):
        return self.variables

    def addChildActorclasses(self, child_actorclasses ):
        for i in range(0, len(child_actorclasses)):
            if not child_actorclasses[i] in self.childActorclasses:
                self.childActorclasses.append( child_actorclasses[i] )
        
    def updateActorclassMethodImplementation(self, method_name, implementations):
        print("ActorclassDefinitionInfo::updateActorclassMethodImplementation")
        if method_name in self.methods.keys():
            self.methods[method_name].implementation = implementations

    def getOutputContextMethodName(self, method_name, method_type):
        # print "ContextDefinitionInfo::getOutputContextMethodName %s %s" % (method_name, method_type)
        return_method_name = ''
        if method_name in self.methods.keys():
            return_method_name = ''
            if method_type == 'event':
                return_method_name = "%s_event_%s_%s"%('async', self.methods[method_name].context_type, self.methods[method_name].define_name)
            elif method_type == 'async':
                return_method_name = "%s_async_%s_%s"%('broadcast', self.methods[method_name].context_type, self.methods[method_name].define_name)
            else:
                return_method_name = "%s_%s"%(self.methods[method_name].context_type, self.methods[method_name].define_name)
        return return_method_name

    def checkContextMethod(self, method_name):
        if method_name in self.methods.keys():
            return True
        else:
            return False
    
    def generateActorclassDefinition(self):
        print("ContextDefinitionInfo::generateContextDefinition: %s" % self.actorclassName)
        lines = []
        line = 'context %s<int ctxId> {' % self.actorclassName
        lines.append(line)

        for key, var in self.variables.items():
            line = var.generateVarDeclaration()
            if line != '':
                line = '\t'+line+';'
                lines.append(line)
        lines.append('}')
        return lines

    def generateEventAndAsyncMethodDefinition(self):
        print("ActorclassDefinitionInfo::generateEventAndAsyncMethodDefinition: %s" % self.actorclassName)
        lines = []
        for key, method in self.methods.items():
            m_lines = method.generateEventAndAsyncMethodDefinition(self.actorclassName)
            for i in range(len(m_lines)):
                lines.append(m_lines[i])
        return lines

    def generateSyncMethodDefinition(self):
        print("ActorclassDefinitionInfo::generateSyncMethodDefinition: %s" % self.actorclassName)
        lines = []
        for key, method in self.methods.items():
            m_lines = method.generateSyncMethodDefinition(self.actorclassName)
            for i in range(len(m_lines)):
                lines.append(m_lines[i])
        return lines

    def getMethodReturnType(self, methodName):
        # print 'ContextDefinitionInfo::getContextMethodReturnType'
        if not methodName in self.methods:
            print("Fail to find method %s" % methodName)
            return ""
        else:
            return self.methods[methodName].getReturnType()

    def getActorclassMethodArgs(self, methodName):
        if not methodName in self.methods:
            return []
        else:
            return self.methods[methodName].getArgs()

    def checkChildContext(self, varName):
        # print 'ContextDefinitionInfo::checkChildContext: ', self.vars.keys(), ' ', varName
        if varName in self.vars.keys():
            return True
        else:
            return False

    def getChildContext(self, varName):
        # print 'ContextDefinitionInfo::getChildContext'
        if varName in self.vars.keys() and self.vars[varName].type != VariableParser.VariableInfo.DefaultType:
            return self.vars[varName]
        else:
            var_info = VariableParser.VariableInfo()
            return var_info

    def markActorclassMethodCallType(self, prefix, methodName):
        if methodName in self.methods.keys():
            self.methods[methodName].markMethodCallType(prefix)


class ActorclassParser:
    @staticmethod
    def parseVarMethodDeclaration(line, actorclass_info, actorclasses, actorclass_collection_vars):
        print("ActorclassParser::parseVarMethodDeclaration")
        l = len(line)
        if l == 0:
            return

        is_yield = False
        for i in range(len(line)):
            if line[i].value == "yield":
                is_yield = True
                break

        if line[l-1].type == 'CLOSE_PAREN':
            actorclassMethodInfo = MethodParser.ActorclassMethodParser.parseMethodDeclaration(line, actorclass_info.actorclassName, actorclasses)
            if actorclassMethodInfo.valid:
                actorclass_info.methods[actorclassMethodInfo.definedName] = actorclassMethodInfo
            else:
                ParserUtil.MyUtil.exitWithError('Fail to parse context method declaration!')
        elif line[l-1].type == 'NAME':
            variableInfo = VariableParser.VariableParser.parseVariableDeclaration(line, actorclasses, actorclass_collection_vars) 
            if variableInfo.type != VariableParser.VariableInfo.InvalidType:
                print('Add variable %s of type %s to actorclass(%s)' % (variableInfo.varName, variableInfo.varType, actorclass_info.actorclassName))
                if is_yield:
                    variableInfo.isChildActor = False
                else:
                    variableInfo.isChildActor = True
                    if variableInfo.actorclass != "" and not variableInfo.actorclass in actorclass_info.childActorclasses:
                        actorclass_info.childActorclasses.append(variableInfo.actorclass)
                        print("Add child %s to %s" % (variableInfo.actorclass, actorclass_info.actorclassName))
                actorclass_info.variables[variableInfo.varName] = variableInfo

    @staticmethod
    def parse(tokens, actorclasses, actorclass_collection_vars ):
        print("ActorclassParser::parse")
        current_parse_status = ActorclassParseStatus.NoneStatus
        actorclass_info = ActorclassInfo()

        for var, var_info in actorclass_collection_vars.items():
            if var_info.actorclass != "" and not var_info.actorclass in actorclass_info.childActorclasses:
                actorclass_info.childActorclasses.append(var_info.actorclass)
                print("Add child %s to %s" % (var_info.actorclass, actorclass_info.actorclassName))
        
        line = []
        paren_depth = 0
        brace_depth = 0
        for i in range(len(tokens)):
            tok = tokens[i]
            print("type=%s, value=%s" % ( tok.type, tok.value))

            if tok.type == 'OPEN_PAREN':
                paren_depth = paren_depth + 1
            elif tok.type == 'CLOSE_PAREN':
                paren_depth = paren_depth - 1
            elif tok.type == 'OPEN_BRACE':
                brace_depth = brace_depth + 1
            elif tok.type == 'CLOSE_BRACE':
                brace_depth = brace_depth - 1

            if current_parse_status == ActorclassParseStatus.NoneStatus:
                if tok.type == 'NAME' and tok.value == 'actorclass':
                    current_parse_status = ActorclassParseStatus.ToGetActorclassName
                else:
                	ParserUtil.MyUtil.exitWithError('Syntax error here!')
            elif current_parse_status == ActorclassParseStatus.ToGetActorclassName:
                if tok.type == 'NAME':
                    actorclass_info.actorclassName = tok.value
                    # print "Start to parse actorclasses %s" % tok.value
                    current_parse_status = ActorclassParseStatus.GetActorclassName
                else:
                	ParserUtil.MyUtil.exitWithError('Syntax error here!')
            elif current_parse_status == ActorclassParseStatus.GetActorclassName:
                if tok.type == 'OPEN_BRACE':
                    current_parse_status = ActorclassParseStatus.ActorclassVarMethodDeclaration
                else:
                	ParserUtil.MyUtil.exitWithError('Syntax error here!')
            elif current_parse_status == ActorclassParseStatus.ActorclassVarMethodDeclaration:
                if tok.type == 'SEMI_COLON' and paren_depth == 0:
                    ActorclassParser.parseVarMethodDeclaration(line, actorclass_info, actorclasses, actorclass_collection_vars)
                    line = []
                else:
                    line.append(tok)
            else:
            	ParserUtil.MyUtil.exitWithError('Syntax error here!')

        return actorclass_info