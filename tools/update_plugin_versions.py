#!/usr/bin/env python

import sys
import os
import collections
import re
from sets import Set

def readManifestFile(name):
    data = {}
    try:
        f = open(name)
        for line in f.readlines():
            line = line.rstrip("\r\n")
            s = line.split(":")
            if len(s) == 2:
                data[s[0].strip()] = s[1].strip()
    except IOError:
        data = {}
    return data

def processDirectory(directory):
   manifestFile = directory + "/MANIFEST.MF" 
   print "Processing " + manifestFile
   tags = readManifestFile(manifestFile)
   if not (tags.get("Bundle-Vendor", "") in Set(["netxms.org", "Raden Solutions"])):
      print "Skipping (wrong vendor)"
      return
   try:
      fIn = open(manifestFile)
      fOut = open(manifestFile + ".tmp", "w")
      for line in fIn.readlines():
         if line.startswith("Bundle-Version:"):
            fOut.write("Bundle-Version: " + sys.argv[2] + "\n")
         else:
            m = re.search("(.*) ([a-zA-Z0-9.]+);bundle-version=\"[0-9.]+\"(.*)", line)
            if m != None:
               if m.group(2).startswith("org.netxms.ui") or m.group(2).startswith("org.netxms.webui") or m.group(2).startswith("com.radensolutions"):
                  fOut.write(m.group(1) + " " + m.group(2) + ";bundle-version=\"" + sys.argv[2] + "\"" + m.group(3) + "\n")
               else:
                  fOut.write(line)
            else:
               fOut.write(line)
      fIn.close()
      fOut.close()
      os.rename(manifestFile + ".tmp", manifestFile)
   except IOError:
      print "I/O error"

def main():
    for (root, dirs, files) in os.walk(sys.argv[1]):
       if "build" in root or ".metadata" in root or "target" in root or "export" in root:
          continue
       if "MANIFEST.MF" in files:
         processDirectory(root)

if len(sys.argv) == 3:
    main()
else:
    print "Usage: ./update_plugin_versions.py <source root> <version>"
