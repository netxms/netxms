#!/usr/bin/python

import sys
import re
import fileinput

def replaceInFile(fileName, sourceText, replaceText):
   p = re.compile(sourceText, re.A)
   for line in fileinput.input(fileName, inplace=True):
      print(p.sub(replaceText, line), end='')
   fileinput.close()
   print("  File %s updated" % (fileName))

#chack that version exists and is in correct format

print("Vestion is updated to %s\n" % (sys.argv[1]))
print("Files updated: ")
replaceInFile('configure.ac', r'\[AC_INIT\(\[NetXMS\], \[(.*)\], \[bugs@netxms.org\]\)\]\)', r'[AC_INIT([NetXMS], [%s], [bugs@netxms.org])])' % sys.argv[1])
replaceInFile('include/netxms-version.h', r'#define NETXMS_VERSION_STRING(.*)"(.*)"(.*)', r'#define NETXMS_VERSION_STRING\1"%s"\3'  % sys.argv[1]) 

#line = "        [AC_INIT([NetXMS], [2.2.0], [bugs@netxms.org])])"
#print(re.sub("\[AC_INIT\(\[NetXMS\], \[(.*)\], \[bugs@netxms.org\]\)\]\)", "[AC_INIT([NetXMS], [%s], [bugs@netxms.org])])" % sys.argv[1] ,line))




#!/bin/bash

#if [ -z $1 ]; then 
#   echo "New version is not provided. First parameter should be new version."
#   exit 1 
#fi

#sed -i "s/\[AC_INIT(\[NetXMS\], \[.*\], \[bugs@netxms.org\])\])/[AC_INIT([NetXMS], [$1], [bugs@netxms.org])])/g" configure.ac
#sed -i "s/#define NETXMS_VERSION_STRING       _T(\".*\")/#define NETXMS_VERSION_STRING       _T(\"$1\")/g" include/netxms-version.h
#sed -i "s/#define NETXMS_VERSION_STRING       \".*\"/#define NETXMS_VERSION_STRING       \"$1\"/g" include/netxms-version.h
#sed -i '1!N; s|<version>.*</version>\n|<version>$1</version>  <name>netxms|g' src/libnxjava/java/base/netxms-base/pom.xml
