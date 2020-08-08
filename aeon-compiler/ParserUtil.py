import ply.lex as lex
import os
import sys
import re

import inspect
import copy

class MyUtil:
    @staticmethod
    def exitWithError(msg):
        print(msg)
        sys.exit(1)


    @staticmethod
    def generateArgToken(tokens):
        if len(tokens) == 1:
            return tokens[0]

        new_token_value = ''
        tok  = copy.copy(tokens[0])
        for i in range(len(tokens)):
            new_token_value = new_token_value+tokens[i].value

        tok.type = 'NAME'
        tok.value = new_token_value
        return tok
    @staticmethod
    def printInfo(str):
        print(str)

    @staticmethod
    def extractMethodArgs(tokens, arg_pos, args):
        print("MyUtil::extractMethodArgs begin")
        cur_pos = arg_pos
        error_msg = 'Fail to parse for method arguments'
        if cur_pos >= len(tokens) or tokens[cur_pos].type != 'OPEN_PAREN':
            MyUtil.exitWithError(error_msg)

        tmp_tokens = []
        paren_depth = 0

        while cur_pos < len(tokens):
            push_flag = True
            tok = tokens[cur_pos]
            print("type=%s, value=%s" % (tok.type, tok.value))
            if tok.type == 'OPEN_PAREN':
                paren_depth = paren_depth + 1
                if paren_depth == 1:
                    push_flag = False
            elif tok.type == 'CLOSE_PAREN':
                paren_depth = paren_depth - 1
                if paren_depth == 0:
                    push_flag = False
                    if len(tmp_tokens) > 0:
                        newTok = MyUtil.generateArgToken(tmp_tokens)
                        args.append(newTok)
                    tmp_tokens = []
                    cur_pos = cur_pos + 1
                    break
            elif tok.type == 'COMMA':
                if paren_depth == 1:
                    push_flag = False
                    if len(tmp_tokens) >0:
                        newTok = MyUtil.generateArgToken(tmp_tokens)
                        args.append(newTok)
                    tmp_tokens = []
                

            if push_flag:
                tmp_tokens.append(tok)

            cur_pos = cur_pos + 1
        print("MyUtil::extractMethodArgs end")
        return cur_pos

    @staticmethod
    def extractContextNameFromSquareBracket(tokens, pos):
        #print "MyUtil::extractContextNameFromSquareBracket"
        error_msg = 'Fail to extract context name from suqare bracket'
        if ( pos + 2 >= len(tokens) or tokens[pos].type != 'OPEN_SQUARE_BRACKET' or tokens[pos+1].type != 'NAME'
            or tokens[pos+2].type != 'CLOSE_SQUARE_BRACKET'):
            MyUtil.exitWithError(error_msg)

        return tokens[pos+1].value

    @staticmethod
    def checkContextMethodDefinition(tokens, pos):
        #print "MyUtil::checkContextMethodDefinition"
        if pos < 2 :
            return False
        min_len = pos + 4
        if len(tokens) < min_len:
            return False

        tok1 = tokens[pos-1]
        tok2 = tokens[pos]
        tok3 = tokens[pos+1]
        tok4 = tokens[pos+2]

        if tok1.type == 'NAME' and tok2.type == 'COLON' and tok3.type == 'COLON' and tok4.type == 'NAME':
            return True
        else:
            return False 

    @staticmethod
    def extractStringVar(strVar):
        #print "MyUtil::extractStringVar"
        matchStr = re.match(r'"([^"\\]*|\\.*)"', strVar, re.M | re.I)
        if not matchStr:
            MyUtil.exitWithError('Fail to extract string from string variable')
        rStr = matchStr.group(1)
        return rStr

    @staticmethod
    def generateContextMethodInvocationName( prefix, contextType, methodName ):
        #print 'MyUtil::generateContextMethodInvocationName'

        output_name = ''
        if prefix == '':
            output_name = 'sync_'+contextType+'_'+methodName
        elif prefix == 'async':
            output_name = 'broadcast_async_' + contextType+'_'+methodName
        else:
            output_name = 'async_event_' + contextType+'_'+methodName

        return output_name

    @staticmethod
    def isActorclass(type):
        if type in ["int", "float", "double"]:
            return False
        else:
            return True

class TokenStack:
    def __init__(self):
        self.items = []

    def push_back(self, item):
        l = len(self.items)
        if l>0 and item.type=='Code' and self.items[l-1].type=='Code':
            self.items[l-1].value = self.items[l-1].value + item.value 
        else:
            self.items.append(item)

    def popUntilOpenParen(self):
        l = len(self.items)
        toks = []

        paren_depth = 0

        while l>0:
            tok = self.items.pop()
            if tok.type == 'OPEN_PAREN':
                paren_depth = paren_depth-1
            elif tok.type == 'CLOSE_PAREN':
                paren_depth = paren_depth+1

            toks.append(tok)
            if paren_depth == 0:
                break
            l = len(self.items)

        toks.reverse()
        return toks

    def popUntilOpenSquareBracket(self):
        #print 'TokenStack::popUntilOpenSquareBracket'
        l = len(self.items)
        toks = []

        bracket_depth = 0

        while l>0:
            tok = self.items.pop()
            if tok.type == 'OPEN_SQUARE_BRACKET':
                bracket_depth = bracket_depth-1
            elif tok.type == 'CLOSE_SQUARE_BRACKET':
                bracket_depth = bracket_depth+1

            toks.append(tok)
            if bracket_depth == 0:
                break
            l = len(self.items)

        toks.reverse()
        return toks

    def popUntilCode(self):
        #print 'TokenStack::popUntilCode'
        l = len(self.items)
        toks = []

        while l>0:
            tok = self.items[l-1]
            if tok.type in [TokenType.CODE, 'EQUALS', 'TAB']:
                break
            else:
                ntok = copy.copy(tok)
                toks.append(ntok)
                self.items.pop()

            l = len(self.items)

        toks.reverse()

        return toks

    def end(self):
        l = len(self.items)

        if l == 0:
            tok = MyToken()
            return tok
        else:
            tok = self.items[l-1]
            return tok

    def pop(self):
        l = len(self.items)

        if l == 0:
            tok = MyToken()
            return tok
        else:
            tok = self.items[l-1]
            self.items.pop()
            return tok

    def getTokens(self):
        return self.items

    def lastNTokens( self, n ):
        toks = []
        l = len( self.items )

        i = 1
        while (l-i)>=0 and i<=n:
            toks.append( self.items[l-i] )
            i = i+1

        return toks



class TokenType:
    CODE = "CODE"
    ACTOR = "ACTOR"
    ACTOR_COLLECTION = "ACTOR_COLLECTION"
    ACTOR_COLLECTION_ITER = "ACTOR_COLLECTION_ITER"
    
class MyToken:
    def __init__(self):
        self.type = ''
        self.value = ''
        self.contextType = ''
        self.collectionType = ''
        self.actorID = ''
        self.isChildContext = False 

class MyConstants:
    ActorThisID = "_this_obj_id"