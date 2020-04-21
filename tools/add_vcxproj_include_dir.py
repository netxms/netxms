#!/usr/bin/env python

import sys
import os
import re

def updateProject(project, depth):
   print("Updating " + project + " at depth " + str(depth))
   replacement = "<AdditionalIncludeDirectories>"
   for i in range(0, depth):
      replacement = replacement + "..\\"
   replacement = replacement + sys.argv[1] + ";"
   try:
      f_in = open(project)
      f_out = open(project + ".new", "w")
      for line in f_in:
         f_out.write(line.replace("<AdditionalIncludeDirectories>", replacement))
      f_in.close()
      f_out.close()
      os.remove(project)
      os.rename(project + ".new", project)
      print("SUCCESS")
   except IOError:
      print("FAILED")

def getPathDepth(path):
   depth = 0
   tail = os.path.split(path)
   while tail[0] != "":
      tail = os.path.split(tail[0])
      depth = depth + 1
   return depth

def main():
   for (root, dirs, files) in os.walk("."):
      for f in files:
         if f.endswith(".vcxproj"):
            updateProject(os.path.join(root, f), getPathDepth(root))

if len(sys.argv) == 2:
    main()
else:
    print("Usage: add_vcxproj_include_dir.py <include dir>")
