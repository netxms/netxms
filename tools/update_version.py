#!/usr/bin/python
import sys
import re
import os
import codecs

import xml.etree.ElementTree as ET

def fromatFilePath(filePath):
   dir_path = os.path.dirname(os.path.realpath(__file__))
   return "%s/../%s" % (dir_path, filePath)


def replaceVersionInJuraXML(version):
   fileName = fromatFilePath('src/java/netxms-jira-connector/pom.xml')
   ET.register_namespace('', "http://maven.apache.org/POM/4.0.0")
   tree = ET.parse(fileName)
   tree.find("{http://maven.apache.org/POM/4.0.0}version").text = version   
   tree.write(fileName, xml_declaration=True, encoding="UTF-8", method="xml")
   print("  File %s updated" % (fileName))
      
def updatePomVersion(fileName, version):
   fileName = fromatFilePath(fileName)
   cmd = "mvn -f %s versions:set -DnewVersion=%s" % (fileName, version)
   returned_value = os.system(cmd)
   if returned_value == 0:
      print("  File %s updated" % (fileName))
   else:
      print("  Error updating %s file" % (fileName))
      
   
def replaceInFile(fileName, sourceText, replaceText, encoding = "utf-8"):
   fileName = fromatFilePath(fileName)
   p = re.compile(sourceText, re.A)
   tmp_name = "tmp.txt"
   with codecs.open(fileName, 'r', encoding=encoding) as fi, \
     codecs.open(tmp_name, 'w', encoding=encoding) as fo:
      for line in fi:
         new_line = p.sub(replaceText, line)
         fo.write(new_line)
   os.remove(fileName)
   os.rename(tmp_name, fileName) 
   print("  File %s updated" % (fileName))

#check that version exists and is in correct format

def main():
   print("Vestion is updated to %s\n" % (sys.argv[1]))
   print("Files updated: ")
   replaceInFile('configure.ac', r'\[AC_INIT\(\[NetXMS\], \[(.*)\], \[bugs@netxms.org\]\)\]\)', r'[AC_INIT([NetXMS], [%s], [bugs@netxms.org])])' % sys.argv[1])
   replaceInFile('include/netxms-version.h', r'#define NETXMS_VERSION_STRING(.*)"(.*)"(.*)', r'#define NETXMS_VERSION_STRING\1"%s"\3'  % sys.argv[1]) 
   updatePomVersion('src/libnxjava/java/base/netxms-base/pom.xml', sys.argv[1]) 
   replaceInFile('src/libnxjava/java/base/netxms-base/src/main/java/org/netxms/base/NXCommon.java', r'public static final String VERSION = ".*";', r'public static final String VERSION = "%s";' % sys.argv[1])
   updatePomVersion('src/libnxjava/java/bridge/pom.xml', sys.argv[1])
   updatePomVersion('src/client/java/netxms-client/pom.xml', sys.argv[1])
   updatePomVersion('src/client/nxshell/java/pom.xml', sys.argv[1])
   updatePomVersion('src/server/nxapisrv/java/pom.xml', sys.argv[1])
   replaceInFile('src/java/build/pack.sh', r'version=.*', r'version=%s'  % sys.argv[1]) 
   updatePomVersion('src/java/client/mobile-agent/pom.xml', sys.argv[1])
   replaceVersionInJuraXML(sys.argv[1])
   updatePomVersion('src/java/nxreporting/pom.xml', sys.argv[1])
   updatePomVersion('src/agent/subagents/java/java/pom.xml', sys.argv[1])
   updatePomVersion('src/agent/subagents/bind9/pom.xml', sys.argv[1])
   updatePomVersion('src/agent/subagents/jmx/pom.xml', sys.argv[1])
   updatePomVersion('src/agent/subagents/ubntlw/pom.xml', sys.argv[1])
   replaceInFile('src/agent/install/setup.iss', r'AppVerName=NetXMS Agent .*', r'AppVerName=NetXMS Agent %s'  % sys.argv[1], "iso-8859-1") 
   replaceInFile('src/agent/install/setup.iss', r'AppVersion=.*', r'AppVersion=%s'  % sys.argv[1], "iso-8859-1") 
   replaceInFile('src/agent/install/setup.iss', r'VersionInfoVersion=.*', r'VersionInfoVersion=%s'  % sys.argv[1], "iso-8859-1") 
   replaceInFile('src/agent/install/setup.iss', r'VersionInfoTextVersion=.*', r'VersionInfoTextVersion=%s'  % sys.argv[1], "iso-8859-1") 
   replaceInFile('src/agent/install/nxagent.iss', r'OutputBaseFilename=nxagent-.*', r'OutputBaseFilename=nxagent-%s'  % sys.argv[1]) 
   replaceInFile('src/agent/install/nxagent-x64.iss', r'OutputBaseFilename=nxagent-.*', r'OutputBaseFilename=nxagent-%s-x64'  % sys.argv[1]) 
   replaceInFile('src/java/netxms-eclipse/Product/nxmc.product', r'^Version .*', r'Version %s'  % sys.argv[1]) 
   replaceInFile('src/java/netxms-eclipse/Product/nxmc.product', r'application" version=".*" use', r'application" version="%s" use'  % sys.argv[1]) 
   replaceInFile('src/java/netxms-eclipse/Core/plugin.xml', r'Version .*&#x0A;', r'Version %s&#x0A;'  % sys.argv[1]) 
   replaceInFile('android/src/agent/app/build.gradle', r'org.netxms:netxms-mobile-agent:.*\'\)', r"org.netxms:netxms-mobile-agent:%s')"  % sys.argv[1]) 
   replaceInFile('android/src/agent/app/build.gradle', r'org.netxms:netxms-base:.*\'\)', r"org.netxms:netxms-base:%s')"  % sys.argv[1]) 
   replaceInFile('android/src/console/app/build.gradle', r'org.netxms:netxms-client:.*\'\)', r"org.netxms:netxms-client:%s')"  % sys.argv[1]) 
   replaceInFile('android/src/console/app/build.gradle', r'org.netxms:netxms-base:.*\'\)', r"org.netxms:netxms-base:%s')"  % sys.argv[1]) 
   replaceInFile('src/install/windows/webui-x64.iss', r'OutputBaseFilename=netxms-webui-.*', r'OutputBaseFilename=netxms-webui-%s-x64'  % sys.argv[1]) 
   replaceInFile('src/install/windows/setup-webui.iss', r'AppVerName=NetXMS WebUI .*', r'AppVerName=NetXMS WebUI %s'  % sys.argv[1]) 
   replaceInFile('src/install/windows/setup-webui.iss', r'AppVersion=.*', r'AppVersion=%s'  % sys.argv[1]) 
   replaceInFile('src/install/windows/setup.iss', r'AppVerName=NetXMS .*', r'AppVerName=NetXMS %s'  % sys.argv[1]) 
   replaceInFile('src/install/windows/setup.iss', r'AppVersion=.*', r'AppVersion=%s'  % sys.argv[1]) 
   replaceInFile('src/install/windows/netxms-x64.iss', r'OutputBaseFilename=netxms-.*', r'OutputBaseFilename=netxms-%s-x64'  % sys.argv[1]) 
   replaceInFile('src/install/windows/netxms-x64-minimal.iss', r'OutputBaseFilename=netxms-.*', r'OutputBaseFilename=netxms-%s-x64-minimal'  % sys.argv[1]) 
   replaceInFile('webui/webapp/Core/nxmc.warproduct', r'version=".*" us', r'version="%s" us'  % sys.argv[1]) 
   replaceInFile('webui/webapp/Product/nxmc.product', r'version=".*" us', r'version="%s" us'  % sys.argv[1]) 
   updatePomVersion('webui/webapp/Product/pom.xml', sys.argv[1])
   
if len(sys.argv) == 2:
    main()
else:
    print ("Usage: ./update_versions.py <version>")


