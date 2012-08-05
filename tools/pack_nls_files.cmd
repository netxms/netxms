@echo off
if not "x%NETXMS_SRC_BASE%" == "x" goto skip
set NETXMS_SRC_BASE=C:\Source\NetXMS
:skip
cd %NETXMS_SRC_BASE%
cd src\java\netxms-eclipse

del nxmc-messages.zip
dir /s /b messages*.properties | grep -v bin | cut -f 7- -d \ | zip -r -@ nxmc-messages.zip
dir /s /b bundle*.properties | grep -v bin | cut -f 7- -d \ | zip -r -@ nxmc-messages.zip
