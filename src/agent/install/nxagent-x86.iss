; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
#include "setup.iss"
OutputBaseFilename=nxagent-{#VersionString}-x86

[Files]
; Installer helpers
Source: "..\..\..\Release\nxreload.exe"; DestDir: "{tmp}"; Flags: dontcopy signonce
; Agent
Source: "..\..\..\Release\libethernetip.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\libnetxms.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\libnxagent.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\libnxdb.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\libnxjava.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\libnxlp.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\libnxsde.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\libnxsnmp.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\appagent.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\nxagentd.exe"; DestDir: "{app}\bin"; BeforeInstall: RenameOldFile; Flags: ignoreversion signonce
Source: "..\..\..\Release\nxcrashsrv.exe"; DestDir: "{app}\bin"; BeforeInstall: RenameOldFile; Flags: ignoreversion signonce
Source: "..\..\..\Release\nxsagent.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\bind9.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\db2.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\dbquery.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\devemu.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\filemgr.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\informix.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\java.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\logwatch.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\mqtt.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\mysql.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\netsvc.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\oracle.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\pgsql.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\ping.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\sms.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\ssh.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\ups.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\wineventsync.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\winnt.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\winperf.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\wmi.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\db2.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\informix.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\mariadb.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\mssql.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\mysql.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\odbc.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\oracle.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\pgsql.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\sqlite.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\contrib\nxagentd.conf-dist"; DestDir: "{app}\etc"; Flags: ignoreversion
Source: "..\..\..\Release\libpng.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\nxsqlite.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\jansson.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\install\files\windows\x86\jq.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\install\files\windows\x86\libcurl.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\install\files\windows\x86\libexpat.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\install\files\windows\x86\modbus.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\install\files\windows\x86\pcre.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\install\files\windows\x86\pcre16.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\install\files\windows\x86\smartctl.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\install\files\windows\x86\unzip.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\install\files\windows\x86\zlib.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\install\files\windows\x86\openssl-3\capi.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\install\files\windows\x86\openssl-3\libcrypto-3.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\install\files\windows\x86\openssl-3\libmosquitto.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\install\files\windows\x86\openssl-3\libssh.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\install\files\windows\x86\openssl-3\libssl-3.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\install\files\windows\x86\openssl-3\openssl.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\install\files\windows\x86\CRT\*"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\..\java-common\netxms-base\target\netxms-base-{#VersionString}.jar"; DestDir: "{app}\lib\java"; Flags: ignoreversion
Source: "..\..\libnxjava\java\target\netxms-java-bridge-{#VersionString}.jar"; DestDir: "{app}\lib\java"; Flags: ignoreversion
Source: "..\subagents\java\java\target\netxms-agent-{#VersionString}.jar"; DestDir: "{app}\lib\java"; Flags: ignoreversion
Source: "..\subagents\jmx\target\jmx.jar"; DestDir: "{app}\lib\java"; Flags: ignoreversion
Source: "..\subagents\opcua\target\opcua.jar"; DestDir: "{app}\lib\java"; Flags: ignoreversion
Source: "..\subagents\ubntlw\target\ubntlw.jar"; DestDir: "{app}\lib\java"; Flags: ignoreversion
; Command-line tools
Source: "..\..\..\Release\nxaevent.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\nxappget.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\nxapush.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\nxcsum.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\nxethernetip.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\nxevent.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\nxhwid.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\nxpush.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\nxsnmpget.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\nxsnmpset.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\nxsnmpwalk.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\nxtftp.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\Release\libnxclient.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\tools\scripts\nxagentd-generate-tunnel-config.cmd"; DestDir: "{app}\bin"; Flags: ignoreversion
; User agent
Source: "..\..\..\Release\nxuseragent.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce

;#include "custom.iss"

#include "common.iss"

Function GetCustomCmdLine(Param: String): String;
Begin
  Result := '';
End;
