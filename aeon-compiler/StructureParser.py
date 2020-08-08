import ply.lex as lex
import os
import sys
import re

import inspect
import copy

import VariableParser
import ParserUtil

class StructParseStatus:
    NoneStatus = 0
    ToGetStructName = 1
    GetStructName = 2
    VarDefinition = 3

class StructDefinitionInfo:
    def __init__(self):
        self.struct_name = ''
        self.vars = {}

    def generateStructDefinition(self):
        # print "StructDefinitionInfo::generateStructDefinition: %s" % self.struct_name
        lines = []
        line = '%s {' % self.struct_name
        lines.append(line)

        for key, var in self.vars.iteritems():
            line = var.generateVarDeclaration()
            if line != '':
                line = '\t'+line+';'
                lines.append(line)

        lines.append('}')

        return lines 
    

class StructDefinitionParser:
    @staticmethod
    def parse(tokens):
    	# print "StructDefinitionParser::parse Analyze struct definition"
        error_msg = 'Fail to parse struct!'
        current_parse_status = StructParseStatus.NoneStatus
        struct_info = StructDefinitionInfo()

        line = []
        for i in range(len(tokens)):
            tok = tokens[i]
            # print tok.value
            
            # if current_parse_status == StructParseStatus.NoneStatus:
            # 	if tok.type == 'NAME' and tok.value == 'struct':
            #     	current_parse_status = StructParseStatus.ToGetStructName
            #     else:
            #         ParserUtil.MyUtil.exitWithError(error_msg)
            # elif current_parse_status == StructParseStatus.ToGetStructName:
            # 	if tok.type == 'NAME':
            #         struct_info.struct_name = tok.value
            #         current_parse_status = StructParseStatus.GetStructName
            #     else:
            #         ParserUtil.MyUtil.exitWithError(error_msg)
            # elif current_parse_status == StructParseStatus.GetStructName:
            # 	if tok.type == 'OPEN_BRACE':
            #         current_parse_status = StructParseStatus.VarDefinition
            #     else:
            #         ParserUtil.MyUtil.exitWithError(error_msg)
            # elif current_parse_status == StructParseStatus.VarDefinition:
            #     if tok.type == 'SEMI_COLON':
            #         context_types = []
            #         var_info = VariableParser.VariableParser.parseVariableDeclaration(line, context_types)
            #         struct_info.vars[var_info.varName] = var_info
            #         line = []
            #     else:
            #         line.append(tok)

        return struct_info