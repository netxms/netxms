#!/usr/bin/python

import sys

if len(sys.argv) < 3:
	print "Usage: header2java.py file.h file.java"
	sys.exit(-1)

import re
reDefine = re.compile("#define\\s+(.*?)\\s+(.*)")

fInput = open(sys.argv[1], "r")
fOutput = open(sys.argv[2], "w")
for line in fInput.readlines():
	match = reDefine.search(line)
	if match:
		name = match.group(1).strip()
		value = match.group(2).strip()
		if len(value) > 0:
			fOutput.write("private static final int %s = %s;\n" % (name, value))
fInput.close()
fOutput.close()
