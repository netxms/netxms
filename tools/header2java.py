#!/usr/bin/python

import sys

if len(sys.argv) < 3:
	print "Usage: header2java.py file.h file.java [access]"
	sys.exit(-1)

access = "public"
if len(sys.argv) > 3:
	access = sys.argv[3]

import re
reDefine = re.compile("#define\\s+(.*?)\\s+(.*)")

fInput = open(sys.argv[1], "r")
fOutput = open(sys.argv[2], "w")
for line in fInput.readlines():
	match = reDefine.search(line)
	if match:
		name = match.group(1).strip()
		value = match.group(2).strip()
		if value[0:3] == "_T(":
			value = value[3:-1]
		t = "int"
		if value.find('"') >= 0:
			t = "String"
		if len(value) > 0:
			fOutput.write("%s static final %s %s = %s;\n" % (access, t, name, value))
fInput.close()
fOutput.close()
