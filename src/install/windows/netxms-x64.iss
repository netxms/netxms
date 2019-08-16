; Installation script for NetXMS Server / Windows x64

#include "setup.iss"
OutputBaseFilename=netxms-{#VersionString}-x64
ArchitecturesInstallIn64BitMode=x64
ArchitecturesAllowed=x64

[Components]
Name: "base"; Description: "Base Files"; Types: full compact custom; Flags: fixed
Name: "console"; Description: "Administrator's Console"; Types: full
Name: "tools"; Description: "Command Line Tools"; Types: full
Name: "server"; Description: "NetXMS Server"; Types: full compact
Name: "server\mariadb"; Description: "MariaDB Client Library"; Types: full
Name: "server\mssql"; Description: "Microsoft SQL Server 2008 Native Client"; Types: full
Name: "server\mysql"; Description: "MySQL Client Library"; Types: full
Name: "server\pgsql"; Description: "PostgreSQL Client Library"; Types: full
Name: "server\oracle"; Description: "Oracle Instant Client"; Types: full
Name: "jre"; Description: "Java Runtime Environment"; Types: full
Name: "pdb"; Description: "Install PDB files for selected components"; Types: custom

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; Common files
Source: "..\..\..\ChangeLog"; DestDir: "{app}\doc"; Flags: ignoreversion; Components: base
Source: "..\..\..\x64\Release\libnetxms.dll"; DestDir: "{app}\bin"; BeforeInstall: StopAllServices; Flags: ignoreversion signonce; Components: base
Source: "..\..\..\x64\Release\libnetxms.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\x64\Release\libnxjava.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\..\..\x64\Release\libnxjava.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\x64\Release\libnxmb.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\..\..\x64\Release\libnxmb.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\x64\Release\libexpat.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\..\..\x64\Release\libexpat.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\x64\Release\libpng.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\..\..\x64\Release\libpng.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\x64\Release\libtre.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\..\..\x64\Release\libtre.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\x64\Release\nxzlib.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\..\..\x64\Release\nxzlib.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\x64\Release\jansson.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\..\..\x64\Release\jansson.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
; Server core
Source: "..\..\..\x64\Release\appagent.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\appagent.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\libnxagent.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\libnxagent.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\libnxdb.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\libnxdb.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\libnxlp.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\libnxlp.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\libnxsl.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\libnxsl.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\libnxsnmp.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\libnxsnmp.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\libnxsrv.dll"; DestDir: "{app}\bin"; BeforeInstall: RenameOldFile; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\libnxsrv.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\libstrophe.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\libstrophe.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\netxmsd.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\netxmsd.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\nxconfig.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxcore.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxcore.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\nxinstall.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxscript.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxsqlite.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxsqlite.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
; DB drivers
Source: "..\..\..\x64\Release\db2.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\db2.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\informix.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\informix.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\mariadb.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\mariadb.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\mssql.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\mssql.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\mysql.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\mysql.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\odbc.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\odbc.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\oracle.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\oracle.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\pgsql.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\pgsql.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\sqlite.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\sqlite.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
; NC drivers
Source: "..\..\..\x64\Release\dbemu.ncd"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\dbemu.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\generic.ncd"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\generic.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\kannel.ncd"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\kannel.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\nexmo.ncd"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nexmo.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\nxagent.ncd"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxagent.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\portech.ncd"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\portech.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\slack.ncd"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\slack.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\smseagle.ncd"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\smseagle.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\text2reach.ncd"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\text2reach.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\websms.ncd"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\websms.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
; Tools
Source: "..\..\..\x64\Release\libnxdbmgr.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxaction.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxadm.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxdbmgr.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxencpasswd.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxget.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxminfo.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxsnmpget.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxsnmpwalk.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxsnmpset.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxupload.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxmibc.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
; Agent
Source: "..\..\..\x64\Release\libnxpython.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\libnxpython.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\nxagentd.exe"; DestDir: "{app}\bin"; BeforeInstall: RenameOldFile; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxagentd.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\nxsagent.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxsagent.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\db2.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\db2.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\dbquery.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\dbquery.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\devemu.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\devemu.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\ecs.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\ecs.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\filemgr.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\filemgr.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\informix.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\informix.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\java.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\java.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\logwatch.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\logwatch.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\mqtt.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\mqtt.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\netsvc.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\netsvc.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\odbcquery.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\odbcquery.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\oracle.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\oracle.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\ping.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\ping.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\portcheck.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\portcheck.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\python.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\python.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\sms.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\sms.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\ssh.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\ssh.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\tuxedo.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\tuxedo.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\ups.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\ups.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\winnt.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\winnt.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\winperf.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\winperf.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\wmi.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\wmi.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\agent\subagents\bind9\target\bind9.jar"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: server
Source: "..\..\agent\subagents\java\java\target\netxms-agent.jar"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: server
Source: "..\..\agent\subagents\jmx\target\jmx.jar"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: server
Source: "..\..\agent\subagents\ubntlw\target\ubntlw.jar"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: server
; Network device drivers
Source: "..\..\..\x64\Release\airespace.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\airespace.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\at.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\at.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\avaya.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\avaya.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\cisco.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\cisco.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\dell-pwc.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\dell-pwc.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\dlink.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\dlink.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\extreme.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\extreme.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\hpe.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\hpe.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\huawei.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\huawei.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\ignitenet.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\ignitenet.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\juniper.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\juniper.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\mikrotik.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\mikrotik.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\net-snmp.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\net-snmp.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\netonix.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\netonix.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\ping3.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\ping3.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\qtech-olt.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\qtech-olt.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\rittal.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\rittal.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\symbol-ws.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\symbol-ws.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\tb.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\tb.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\ubnt.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\ubnt.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
; Helpdesk links
Source: "..\..\..\x64\Release\jira.hdlink"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\jira.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
; DB schema
Source: "..\..\..\sql\dbinit_db2.sql"; DestDir: "{app}\share\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbinit_mssql.sql"; DestDir: "{app}\share\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbinit_mysql.sql"; DestDir: "{app}\share\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbinit_oracle.sql"; DestDir: "{app}\share\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbinit_pgsql.sql"; DestDir: "{app}\share\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbinit_sqlite.sql"; DestDir: "{app}\share\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbinit_tsdb.sql"; DestDir: "{app}\share\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbschema_db2.sql"; DestDir: "{app}\share\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbschema_mssql.sql"; DestDir: "{app}\share\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbschema_mysql.sql"; DestDir: "{app}\share\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbschema_oracle.sql"; DestDir: "{app}\share\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbschema_pgsql.sql"; DestDir: "{app}\share\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbschema_sqlite.sql"; DestDir: "{app}\share\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbschema_tsdb.sql"; DestDir: "{app}\share\sql"; Flags: ignoreversion; Components: server
; Misc files
Source: "{app}\var\mibs\*.txt"; DestDir: "{app}\share\mibs"; Flags: ignoreversion external skipifsourcedoesntexist; Components: server
Source: "..\..\..\contrib\mibs\*.txt"; DestDir: "{app}\share\mibs"; Flags: ignoreversion; Components: server
Source: "..\..\..\contrib\templates\*.xml"; DestDir: "{app}\share\templates"; Flags: ignoreversion; Components: server
Source: "..\..\..\contrib\netxmsd.conf-dist"; DestDir: "{app}\etc"; Flags: ignoreversion; Components: server
Source: "..\..\..\contrib\nxagentd.conf-dist"; DestDir: "{app}\etc"; Flags: ignoreversion; Components: server
Source: "..\..\..\images\*"; DestDir: "{app}\var\images"; Flags: ignoreversion; Components: server
Source: "..\..\..\contrib\music\*"; DestDir: "{app}\var\files"; Flags: ignoreversion; Components: server
Source: "..\..\libnxjava\java\base\netxms-base\target\netxms-base.jar"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: server
Source: "..\..\libnxjava\java\bridge\target\netxms-java-bridge.jar"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: server
; Command-line tools files
Source: "..\..\..\x64\Release\libnxclient.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\x64\Release\libnxclient.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: tools and pdb
Source: "..\..\..\x64\Release\nxalarm.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\x64\Release\nxap.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\x64\Release\nxappget.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\x64\Release\nxapush.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\x64\Release\nxcsum.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\x64\Release\nxevent.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\x64\Release\nxgenguid.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\x64\Release\nxnotify.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\x64\Release\nxpush.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\x64\Release\nxshell.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\client\java\netxms-client\target\netxms-client.jar"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: tools
Source: "..\..\client\nxshell\java\target\nxshell.jar"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: tools
; Diagnostic tools
Source: "..\..\server\tools\scripts\nx-collect-server-diag.cmd"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base
Source: "..\..\server\tools\scripts\zip.ps1"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base
; Management console
Source: "..\..\java\build\win32.win32.x86_64\nxmc\*"; DestDir: "{app}\bin"; Flags: ignoreversion recursesubdirs; Components: console
; Third party files
Source: "..\files\windows\x64\libcrypto.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\files\windows\x64\libcurl.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\files\windows\x64\libmosquitto.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\files\windows\x64\libmosquitto.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\files\windows\x64\libssh.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\files\windows\x64\libssl.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\files\windows\x64\libmariadb.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\mariadb
Source: "..\files\windows\x64\libmariadb.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\mariadb and pdb
Source: "..\files\windows\x64\libmysql.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\mysql
Source: "..\files\windows\x64\libpq.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\pgsql
Source: "..\files\windows\x64\libintl-8.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\pgsql
Source: "..\files\windows\x64\openssl.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\files\windows\x64\oci.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\oracle
Source: "..\files\windows\x64\oraociei11.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\oracle
Source: "..\files\windows\x64\jre\*"; DestDir: "{app}\bin\jre"; Flags: ignoreversion recursesubdirs; Components: jre
; Install-time files
Source: "..\files\windows\x64\vcredist_x64.exe"; DestDir: "{app}\var"; DestName: "vcredist.exe"; Flags: ignoreversion deleteafterinstall; Components: base
Source: "..\files\windows\x86\rm.exe"; DestDir: "{app}\var"; Flags: ignoreversion deleteafterinstall; Components: base
Source: "..\files\windows\x64\sqlncli.msi"; DestDir: "{app}\var"; Flags: ignoreversion deleteafterinstall; Components: server\mssql

#include "icons.iss"
#include "run-full.iss"
#include "common.iss"
