; NetXMS SDK installation script
; Copyright (c)

[Setup]
AppName=NetXMS SDK
AppVerName=NetXMS SDK 3.5.152
AppVersion=3.5.152
AppPublisher=Raden Solutions
AppPublisherURL=http://www.radensolutions.com
AppSupportURL=http://www.radensolutions.com
AppUpdatesURL=http://www.netxms.org
DefaultDirName=C:\NetXMS\SDK
DefaultGroupName=NetXMS SDK
AllowNoIcons=yes
LicenseFile=..\..\LGPL.txt
Compression=lzma
SolidCompression=yes
LanguageDetectionMethod=none
OutputBaseFilename=netxms-sdk-3.5.152

[Files]
; Header files
Source: "..\..\build\netxms-build-tag.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\*.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\src\server\include\*.h"; DestDir: "{app}\include\server"; Flags: ignoreversion;
; x86 libraries
Source: "..\..\release\appagent.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\jansson.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\libethernetip.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\libexpat.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\libnetxms.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\libnxagent.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\libnxclient.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\libnxdb.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\libnxlp.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\libnxmb.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\libstrophe.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\nxsqlite.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\nxzlib.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
; x64 libraries
Source: "..\..\x64\release\appagent.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\jansson.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\libethernetip.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\libexpat.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\libnetxms.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\libnxagent.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\libnxclient.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\libnxdb.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\libnxdbmgr.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\libnxlp.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\libnxmb.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\libnxsl.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\libnxsnmp.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\libnxsrv.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\libstrophe.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\nxcore.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\nxsqlite.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\nxzlib.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
; Java API
Source: "..\..\src\agent\subagents\java\java\target\netxms-agent.jar"; DestDir: "{app}\java"; Flags: ignoreversion;
;Source: "..\..\src\agent\subagents\java\java\target\netxms-agent-javadoc.jar"; DestDir: "{app}\java"; Flags: ignoreversion;
Source: "..\..\src\java-common\netxms-base\target\netxms-base-3.5.152.jar"; DestDir: "{app}\java"; Flags: ignoreversion;
Source: "..\..\src\java-common\netxms-base\target\netxms-base-3.5.152-javadoc.jar"; DestDir: "{app}\java"; Flags: ignoreversion;
Source: "..\..\src\client\java\netxms-client\target\netxms-client-3.5.152.jar"; DestDir: "{app}\java"; Flags: ignoreversion;
Source: "..\..\src\client\java\netxms-client\target\netxms-client-3.5.152-javadoc.jar"; DestDir: "{app}\java"; Flags: ignoreversion;
; Documentation
Source: "..\..\doc\internal\event_code_ranges.txt"; DestDir: "{app}\doc"; Flags: ignoreversion;
Source: "..\..\doc\internal\nxcp_command_ranges.txt"; DestDir: "{app}\doc"; Flags: ignoreversion;
Source: "..\..\doc\internal\unicode.txt"; DestDir: "{app}\doc"; Flags: ignoreversion;
; Examples
Source: "..\samples\C\samples.sln"; DestDir: "{app}\samples"; Flags: ignoreversion;
Source: "..\samples\C\subagent\sample-subagent.cpp"; DestDir: "{app}\samples\C\subagent"; Flags: ignoreversion;
Source: "..\samples\C\subagent\sample-subagent.vcxproj"; DestDir: "{app}\samples\C\subagent"; Flags: ignoreversion;
Source: "..\samples\C\subagent\sample-subagent.vcxproj.filters"; DestDir: "{app}\samples\C\subagent"; Flags: ignoreversion;
Source: "..\samples\Java\get-dci-from-server\pom.xml"; DestDir: "{app}\samples\Java\get-dci-from-server"; Flags: ignoreversion;
Source: "..\samples\Java\get-dci-from-server\src\*"; DestDir: "{app}\samples\Java\get-dci-from-server\src"; Flags: ignoreversion recursesubdirs;
; OpenSSL headers and libs
Source: "..\openssl\include\openssl\*.h"; DestDir: "{app}\include\openssl"; Flags: ignoreversion;
Source: "..\openssl\include\openssl\*.c"; DestDir: "{app}\include\openssl"; Flags: ignoreversion;
Source: "..\openssl\lib\x86\*.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\openssl\lib\x64\*.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
