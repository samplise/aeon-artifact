import ply.lex as lex
import os
import sys
import re

import inspect
import copy

import ParserUtil

class VariableParseStatus:
    NoneStatus = 0
    ToGetVariableType = 1
    ToGetVariableName = 2
    ToGetVariableValue = 3

class VariableInfo:
    InvalidType = 0
    DefaultType = 1
    ActorType = 2
    ActorCollectionType = 3
    ActorCollectionIterType = 4

    def __init__(self):
        self.type = VariableInfo.InvalidType
        self.varType = ""
        self.varName = ""
        self.value = ""
        
        self.special = ""
        self.const = ""

        self.collectionType = ""
        self.actorId = ''
        self.collectionPos = 0
        self.includedActorclasses = []
        self.actorclass = ""

        self.isChildActor = False

    def generateVarDeclaration(self):
        print('VariableInfo::generateVarDeclaration: ', self.varName)
        line = ''
        var_type = ''
        if self.type == VariableInfo.DefaultType:
            var_type = self.varType
        elif self.type == VariableInfo.ActorType:
            var_type = 'int'
        elif self.type == VariableInfo.ActorCollectionType:
            var_type = self.generateOutputContextCollectionType()
        elif self.type == VariableInfo.ActorCollectionIterType:
            var_type = self.generateOutputContextCollectionType()
            var_type = var_type+'::iterator'
        else:
            return line

        line = var_type+' '
        if self.const == 'const':
            line = 'const '+line

        if self.special != '':
            line = line+self.special

        line = line+self.varName
        return line

    def printInfo(self):
        if self.isContextType:
            self.contextVarType.printInfo()
        else:
            print(self.varType)

        # print 'variableInfo( varName=', self.varName, ', special=', self.special, ', const=', self.const, ')'

    def generateOutputContextCollectionType(self):
        # print 'VariableInfo::generateOutputContextCollectionType'
        output_type = ''
        if self.collectionType == 'map':
            output_type = 'map<'
            if self.collectionPos == 1:
                output_type = output_type + 'int, ' + self.contextId + '>'
            else:
                output_type = output_type + self.contextId + ', int>'
        else:
            output_type = self.varType

        return output_type


class VariableParser:
    @staticmethod
    def parseVariableDeclaration(tokens, actorclasses, actorclass_collection_vars):
        print("VariableParser::parseVariableDeclaration")
        var_info = VariableInfo()
        var_info.type = VariableInfo.DefaultType
        status = VariableParseStatus.ToGetVariableType
        
        for i in range(len(tokens)):
            tok = tokens[i]
            print("type=%s, value=%s" % ( tok.type, tok.value))
            if tok.value == "yield":
                continue

            if tok.type == "COLLECTION_TYPE" and tokens[i+1].value in actorclass_collection_vars:
                return actorclass_collection_vars[ tokens[i+1].value ]
            
            if status == VariableParseStatus.ToGetVariableValue:
                var_info.value = tok.value
            elif tok.type in ('ASTERISK', 'AMPERSTAND'):
                var_info.special = tok.value
            elif tok.type == 'NAME' or tok.type == 'COLLECTION_TYPE':
                if tok.value == 'configurable':
                    continue
                elif tok.value == 'const':
                    var_info.const = tok.value
                elif status == VariableParseStatus.ToGetVariableType:
                    var_info.varType = tok.value
                    if var_info.varType in actorclasses:
                        var_info.isChildActor = True
                        var_info.type = VariableInfo.ActorType
                        var_info.actorclass = var_info.varType
                    status = VariableParseStatus.ToGetVariableName
                elif status == VariableParseStatus.ToGetVariableName:
                    var_info.varName = tok.value
                    #print 'varable name: ', var_info.varName
                    if var_info.varName in actorclass_collection_vars:
                        var_actorclasses = actorclass_collection_vars[ var_info.varName ]
                        for j in range(0, len(var_actorclasses)):
                            if not var_actorclasses[j] in var_info.includedActorclasses:
                                var_info.includedActorclasses.append( var_actorclasses[j] )

                else:
                	ParserUtil.MyUtil.exitWithError('Syntax error here!')
            elif tok.type == 'EQUALS':
                status == VariableParseStatus.ToGetVariableValue
            
          
        #print 'type=', var_info.varType 
        return var_info

    @staticmethod
    def parseContextCollectionType( collection_type, context_types ):
        # print 'VariableParser::parseContextCollectionType'
        
        var_info = VariableInfo()
        m = re.search('.+::(.+)<.+>', collection_type)
        if not m:
            return var_info

        collectionType = m.group(1)
        if collectionType == 'vector' or collectionType == 'set' or collectionType == 'deque':
            m =  re.search('[a-z]+<(.+)>', collection_type)
            if not m:
                return var_info
            subType = m.group(1)
            # print subType
            if subType in context_types:
                var_info.type = VariableInfo.ContextCollectionType
                var_info.varType = subType
                var_info.collectionType = collectionType
        elif collectionType == 'map':
            m = re.search('map<(.+), (.+)>', collection_type)
            if not m:
                return var_info
            subType1 = m.group(1)
            subType2 = m.group(2)
            # print subType1, '; ', subType2

            if subType1 in context_types:
                var_info.type = VariableInfo.ContextCollectionType
                var_info.varType = subType1
                var_info.collectionType = collectionType
                var_info.contextObjPos = 1
                var_info.contextId = subType2
            elif subType2 in context_types:
                var_info.type = VariableInfo.ContextCollectionType
                var_info.varType = subType2
                var_info.collectionType = collectionType
                var_info.contextObjPos = 2
                var_info.contextId = subType1
        
        return var_info

    @staticmethod
    def analyzeActorclassType(varType, actorclasses):
        # print 'VariableParser::analyzeContextType'
        
        if varType in actorclasses:
            var_info = VariableInfo()
            var_info.type = VariableInfo.ActorType
            var_info.varType = varType
            return var_info
        else:
            return VariableParser.parseContextCollectionType( varType, actorclasses)
