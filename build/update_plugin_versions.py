#!/usr/bin/env python
#vim: ts=4 sw=4 expandtab

import sys
import os
import collections
import re

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
    print("Processing " + manifestFile)
    tags = readManifestFile(manifestFile)
    if tags.get("Bundle-Vendor", "") not in ["netxms.org", "Raden Solutions"]:
        print("Skipping (wrong vendor)")
        return
    try:
        output = ""
        with open(manifestFile) as f:
            for line in f.readlines():
                if line.startswith("Bundle-Version:"):
                    output += "Bundle-Version: %s\n" % (sys.argv[2])
                else:
                    m = re.search("(.*) ([a-zA-Z0-9.]+);bundle-version=\"[0-9.]+\"(.*)", line)
                    if m and (m.group(2).startswith("org.netxms.ui") or m.group(2).startswith("org.netxms.webui") or m.group(2).startswith("com.radensolutions")):
                        output += "%s %s;bundle-version=\"%s\"%s\n" % (m.group(1), m.group(2), sys.argv[2], m.group(3))
                    else:
                        output += line
        with open(manifestFile, 'w') as f:
            f.write(output)
    except IOError:
        print("I/O error")

def main():
    for (root, dirs, files) in os.walk(sys.argv[1]):
       if "build" in root or ".metadata" in root or "target" in root or "export" in root:
          continue
       if "MANIFEST.MF" in files:
         processDirectory(root)

if len(sys.argv) == 3:
    main()
else:
    print("Usage: ./update_plugin_versions.py <source root> <version>")
