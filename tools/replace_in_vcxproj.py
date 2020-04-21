#!/usr/bin/env python

import sys
import os
import re

def updateProject(project):
   print("Updating " + project)
   try:
      f_in = open(project)
      f_out = open(project + ".new", "w")
      for line in f_in:
         f_out.write(line.replace(sys.argv[1], sys.argv[2]))
      f_in.close()
      f_out.close()
      os.remove(project)
      os.rename(project + ".new", project)
      print("SUCCESS")
   except IOError:
      print("FAILED")

def main():
   for (root, dirs, files) in os.walk("."):
      for f in files:
         if f.endswith(".vcxproj"):
            updateProject(os.path.join(root, f))

if len(sys.argv) == 3:
    main()
else:
    print("Usage: replace_in_vcxproj.py <text> <replacement>")
