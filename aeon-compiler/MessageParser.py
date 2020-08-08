import ply.lex as lex
import os
import sys
import re

import inspect
import copy

import StructureParser
import VariableParser
import ParserUtil

class MessageDefinitionInfo:
    def __init__(self):
        self.message_name = ''
        self.vars = {}

    def generateMessageDefinition(self):
        # print "MessageDefinitionInfo::generateMessageDefinition: %s" % self.message_name
        lines = []
        line = '%s {' % self.message_name
        lines.append(line)

        for key, var in self.vars.iteritems():
            line = var.generateVarDeclaration()
            if line != '':
                line = '\t'+line+';'
                lines.append(line)

        lines.append('}')

        return lines
    

class MessageDefinitionParser:
    @staticmethod
    def parse(tokens):
        # print "MessageDefinitionParser::parse"
        current_parse_status = StructureParser.StructParseStatus.NoneStatus
        message_info = MessageDefinitionInfo()

        line = []
        
        for i in range(len(tokens)):
            tok = tokens[i]
            # print tok.value

            if current_parse_status == StructureParser.StructParseStatus.NoneStatus:
                if tok.type == 'NAME' and tok.value == 'message':
                    current_parse_status = StructureParser.StructParseStatus.ToGetStructName
            elif current_parse_status == StructureParser.StructParseStatus.ToGetStructName:
                if tok.type == 'NAME':
                    message_info.message_name = tok.value
                    current_parse_status = StructureParser.StructParseStatus.GetStructName
            elif current_parse_status == StructureParser.StructParseStatus.GetStructName:
                if tok.type == 'OPEN_BRACE':
                    current_parse_status = StructureParser.StructParseStatus.VarDefinition
            elif current_parse_status == StructureParser.StructParseStatus.VarDefinition:
                if tok.type == 'SEMI_COLON':
                    context_types = []
                    var_info = VariableParser.VariableParser.parseVariableDeclaration(line, context_types)
                    message_info.vars[var_info.varName] = var_info
                    line = []
                else:
                    line.append(tok)
        return message_info