#!/usr/bin/python
import sys
sys.path = ["../"] + sys.path
import CppHeaderParser
try:
    aeonParser = CppHeaderParser.AEONParser("gameapp.cc", "GameApp.mac")
except CppHeaderParser.CppParseError as e:
    print(e)
    sys.exit(1)


