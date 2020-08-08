#!/usr/bin/env python
# convert.py
# @Author:      Diwaker Gupta (diwaker@floatingsun.net)
# @Last Change: 2008-06-12

import os
import sys
import optparse

import re
# import matplotlib as _m
# import pylab as _p

r1 = re.compile("INSERT INTO ([^(]*).*VALUES \((.*)\)")
r2 = re.compile("CREATE TABLE (\w+) \(")

parser = optparse.OptionParser("%prog [options] args")
parser.add_option("--verbose", "-v", action = "store_true", dest = "verbose")

(options, args) = parser.parse_args()

input = open(args[0], 'r')
c = open("create.txt", "w")
copies = open("copies.txt", "w")

tables = {}

for line in input:
    m = r2.search(line)
    if m:
        c.write(line)
        copies.write("\copy %s from %s.txt\n" % (m.group(1), m.group(1)))

    m = r1.search(line)
    if m:
        table = m.group(1).strip()
        if not tables.has_key(table):
            tables[table] = open("%s.txt" % table, "w")
        vals = m.group(2).split(',')
        str = ""
        for v in vals:
            str += v.strip(" '") + "\t"
        str = str.strip()
        str += "\n"
        tables[table].write(str)

c.close()
copies.close()
for k in tables:
    tables[k].close()
