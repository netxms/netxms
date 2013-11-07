; NetXMS SDK installation script
; Copyright (c)

[Setup]
AppName=NetXMS SDK
AppVerName=NetXMS SDK 1.1.10
AppVersion=1.1.10
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
OutputBaseFilename=netxms-sdk-1.1.10

[Files]
; Header files
Source: "..\..\include\base64.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\dbdrv.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\geolocation.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\netxms-regex.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\netxms-version.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\netxms_getopt.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\netxms_isc.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\netxms_maps.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\netxmsdb.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\nms_agent.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\nms_common.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\nms_cscp.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\nms_threads.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\nms_util.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\nxclapi.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\nxconfig.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\nxcpapi.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\nxdbapi.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\nxevent.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\nxlog.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\nxlpapi.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\nxmbapi.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\nxqueue.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\nxsl.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\nxsl_classes.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\nxsnmp.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\nxtools.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\rwlock.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\unicode.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\include\uuid.h"; DestDir: "{app}\include"; Flags: ignoreversion;
Source: "..\..\src\server\include\nddrv.h"; DestDir: "{app}\include\server"; Flags: ignoreversion;
Source: "..\..\src\server\include\nms_actions.h"; DestDir: "{app}\include\server"; Flags: ignoreversion;
Source: "..\..\src\server\include\nms_alarm.h"; DestDir: "{app}\include\server"; Flags: ignoreversion;
Source: "..\..\src\server\include\nms_core.h"; DestDir: "{app}\include\server"; Flags: ignoreversion;
Source: "..\..\src\server\include\nms_dcoll.h"; DestDir: "{app}\include\server"; Flags: ignoreversion;
Source: "..\..\src\server\include\nms_events.h"; DestDir: "{app}\include\server"; Flags: ignoreversion;
Source: "..\..\src\server\include\nms_locks.h"; DestDir: "{app}\include\server"; Flags: ignoreversion;
Source: "..\..\src\server\include\nms_objects.h"; DestDir: "{app}\include\server"; Flags: ignoreversion;
Source: "..\..\src\server\include\nms_pkg.h"; DestDir: "{app}\include\server"; Flags: ignoreversion;
Source: "..\..\src\server\include\nms_script.h"; DestDir: "{app}\include\server"; Flags: ignoreversion;
Source: "..\..\src\server\include\nms_topo.h"; DestDir: "{app}\include\server"; Flags: ignoreversion;
Source: "..\..\src\server\include\nms_users.h"; DestDir: "{app}\include\server"; Flags: ignoreversion;
Source: "..\..\src\server\include\nxcore_jobs.h"; DestDir: "{app}\include\server"; Flags: ignoreversion;
Source: "..\..\src\server\include\nxcore_logs.h"; DestDir: "{app}\include\server"; Flags: ignoreversion;
Source: "..\..\src\server\include\nxcore_situations.h"; DestDir: "{app}\include\server"; Flags: ignoreversion;
Source: "..\..\src\server\include\nxmodule.h"; DestDir: "{app}\include\server"; Flags: ignoreversion;
Source: "..\..\src\server\include\nxsrvapi.h"; DestDir: "{app}\include\server"; Flags: ignoreversion;
; x86 libraries
Source: "..\..\release\libexpat.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\libnetxms.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\libnetxmsw.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\libnxcl.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\libnxclw.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\libnxdb.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\libnxdbw.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\libnxlp.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\libnxmap.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\libnxmapw.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\libnxmb.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\libnxsl.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\libnxsnmp.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\libnxsrv.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\libtre.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\nxcore.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\nxsqlite.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\..\release\nxzlib.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
; x64 libraries
Source: "..\..\x64\release\libexpat.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\libnetxms.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\libnetxmsw.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\libnxcl.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\libnxclw.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\libnxdb.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\libnxdbw.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\libnxlp.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\libnxmap.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\libnxmapw.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\libnxmb.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\libnxsl.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\libnxsnmp.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\libnxsrv.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\libtre.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\nxcore.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\nxsqlite.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
Source: "..\..\x64\release\nxzlib.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
; Java API
Source: "..\..\src\java\netxms-base\target\netxms-base-1.1.10.jar"; DestDir: "{app}\java"; Flags: ignoreversion;
Source: "..\..\src\java\netxms-base\target\netxms-base-1.1.10-javadoc.jar"; DestDir: "{app}\java"; Flags: ignoreversion;
Source: "..\..\src\java\netxms-client\target\netxms-client-1.1.10.jar"; DestDir: "{app}\java"; Flags: ignoreversion;
Source: "..\..\src\java\netxms-client\target\netxms-client-1.1.10-javadoc.jar"; DestDir: "{app}\java"; Flags: ignoreversion;
Source: "..\..\src\java\netxms-client-api\target\netxms-client-api-1.1.10.jar"; DestDir: "{app}\java"; Flags: ignoreversion;
Source: "..\..\src\java\netxms-client-api\target\netxms-client-api-1.1.10-javadoc.jar"; DestDir: "{app}\java"; Flags: ignoreversion;
; Documentation
Source: "..\..\doc\internal\event_code_ranges.txt"; DestDir: "{app}\doc"; Flags: ignoreversion;
Source: "..\..\doc\internal\nxcp_command_ranges.txt"; DestDir: "{app}\doc"; Flags: ignoreversion;
Source: "..\..\doc\internal\unicode.txt"; DestDir: "{app}\doc"; Flags: ignoreversion;
;Source: "..\..\doc\manuals\nxcp_reference.pdf"; DestDir: "{app}\doc"; Flags: ignoreversion;
; Examples
Source: "..\samples\C\samples.sln"; DestDir: "{app}\samples"; Flags: ignoreversion;
Source: "..\samples\C\subagent\sample-subagent.cpp"; DestDir: "{app}\samples\C\subagent"; Flags: ignoreversion;
Source: "..\samples\C\subagent\sample-subagent.vcproj"; DestDir: "{app}\samples\C\subagent"; Flags: ignoreversion;
Source: "..\samples\Java\get-dci-from-server\pom.xml"; DestDir: "{app}\samples\Java\get-dci-from-server"; Flags: ignoreversion;
Source: "..\samples\Java\get-dci-from-server\src\*"; DestDir: "{app}\samples\Java\get-dci-from-server\src"; Flags: ignoreversion recursesubdirs;
; OpenSSL headers and libs
Source: "..\openssl\include\*.h"; DestDir: "{app}\include\openssl"; Flags: ignoreversion;
Source: "..\openssl\include\*.c"; DestDir: "{app}\include\openssl"; Flags: ignoreversion;
Source: "..\openssl\lib\x86\*.lib"; DestDir: "{app}\lib\x86"; Flags: ignoreversion;
Source: "..\openssl\lib\x64\*.lib"; DestDir: "{app}\lib\x64"; Flags: ignoreversion;
