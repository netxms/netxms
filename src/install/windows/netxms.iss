; Installation script for NetXMS Server / Windows x86

#include "setup.iss"
OutputBaseFilename=netxms-2.2

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
Source: "..\..\..\Release\libnetxms.dll"; DestDir: "{app}\bin"; BeforeInstall: StopAllServices; Flags: ignoreversion signonce; Components: base
Source: "..\..\..\Release\libnetxms.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\Release\libnxjava.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\..\..\Release\libnxjava.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\Release\libnxmb.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\..\..\Release\libnxmb.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\Release\libexpat.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\..\..\Release\libexpat.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\Release\libpng.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\..\..\Release\libpng.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\Release\libtre.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\..\..\Release\libtre.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\Release\nxzlib.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\..\..\Release\nxzlib.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\Release\jansson.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\..\..\Release\jansson.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
; Executables and DLLs shared between different components (server, console, etc.)
Source: "..\..\..\Release\libnxsnmp.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server console
Source: "..\..\..\Release\libnxsnmp.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: (server or console) and pdb
Source: "..\..\..\Release\libnxsl.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server console
Source: "..\..\..\Release\libnxsl.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: (server or console) and pdb
Source: "..\..\..\Release\nxscript.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server console
Source: "..\..\..\Release\nxconfig.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\nxinstall.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
; Server core
Source: "..\..\..\Release\appagent.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\appagent.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\libnxagent.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\libnxagent.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\nxsqlite.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\nxsqlite.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\libnxlp.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\libnxlp.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\libnxdb.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\libnxdb.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\libnxsrv.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\libnxsrv.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\libstrophe.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\libstrophe.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\nxcore.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\nxcore.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\netxmsd.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\netxmsd.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
; DB drivers
Source: "..\..\..\Release\db2.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\db2.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\informix.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\informix.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\mariadb.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\mariadb.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\mysql.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\mysql.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\mssql.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\mssql.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\odbc.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\odbc.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\pgsql.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\pgsql.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\sqlite.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\sqlite.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\oracle.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\oracle.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
; SMS drivers
Source: "..\..\..\Release\dbemu.sms"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\dbemu.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\generic.sms"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\generic.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\kannel.sms"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\kannel.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\nexmo.sms"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\nexmo.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\nxagent.sms"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\nxagent.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\portech.sms"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\portech.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\slack.sms"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\slack.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\smseagle.sms"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\smseagle.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\text2reach.sms"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\text2reach.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\websms.sms"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\websms.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
; Tools
Source: "..\..\..\Release\nxaction.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\nxadm.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\nxdbmgr.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\nxencpasswd.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\nxget.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\nxget.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\nxminfo.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\nxsnmpget.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\nxsnmpwalk.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\nxsnmpset.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\nxupload.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\nxmibc.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
; Agent
Source: "..\..\..\Release\nxagentd.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\nxagentd.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\nxsagent.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\nxsagent.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\db2.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\db2.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\dbquery.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\dbquery.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\devemu.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\devemu.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\ecs.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\ecs.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\filemgr.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\filemgr.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\informix.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\informix.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\java.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\java.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\logwatch.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\logwatch.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\netsvc.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\netsvc.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\odbcquery.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\odbcquery.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\oracle.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\oracle.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\ping.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\ping.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\portcheck.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\portcheck.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\sms.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\sms.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\ssh.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\ssh.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\ups.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\ups.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\winnt.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\winnt.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\winperf.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\winperf.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\wmi.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\wmi.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
; Network device drivers
Source: "..\..\..\Release\airespace.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\airespace.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\at.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\at.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\avaya-ers.dll"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\avaya-ers.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\baystack.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\baystack.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\cisco.dll"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\cisco.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\cat2900xl.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\cat2900xl.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\catalyst.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\catalyst.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\cisco-esw.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\cisco-esw.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\cisco-sb.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\cisco-sb.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\dell-pwc.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\dell-pwc.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\dlink.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\dlink.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\ers8000.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\ers8000.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\extreme.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\extreme.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\h3c.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\h3c.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\hpsw.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\hpsw.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\ignitenet.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\ignitenet.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\juniper.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\juniper.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\mikrotik.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\mikrotik.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\netonix.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\netonix.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\netscreen.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\netscreen.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\ntws.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\ntws.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\ping3.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\ping3.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\procurve.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\procurve.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\qtech-olt.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\qtech-olt.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\symbol-ws.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\symbol-ws.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\tb.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\tb.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\ubnt.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\ubnt.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
; Helpdesk links
Source: "..\..\..\Release\jira.hdlink"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\Release\jira.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
; DB schema
Source: "..\..\..\sql\dbinit_mssql.sql"; DestDir: "{app}\share\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbinit_mysql.sql"; DestDir: "{app}\share\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbinit_oracle.sql"; DestDir: "{app}\share\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbinit_pgsql.sql"; DestDir: "{app}\share\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbinit_sqlite.sql"; DestDir: "{app}\share\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbschema_mssql.sql"; DestDir: "{app}\share\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbschema_mysql.sql"; DestDir: "{app}\share\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbschema_oracle.sql"; DestDir: "{app}\share\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbschema_pgsql.sql"; DestDir: "{app}\share\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbschema_sqlite.sql"; DestDir: "{app}\share\sql"; Flags: ignoreversion; Components: server
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
Source: "..\..\agent\subagents\bind9\target\bind9.jar"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: server
Source: "..\..\agent\subagents\java\java\target\netxms-agent.jar"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: server
Source: "..\..\agent\subagents\jmx\target\jmx.jar"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: server
Source: "..\..\agent\subagents\ubntlw\target\ubntlw.jar"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: server
; Command-line tools files
Source: "..\..\..\Release\libnxclient.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\Release\libnxclient.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: tools and pdb
Source: "..\..\..\Release\nxalarm.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\Release\nxappget.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\Release\nxapush.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\Release\nxevent.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\Release\nxpush.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\Release\nxshell.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\client\nxshell\java\target\nxshell.jar"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: tools
Source: "..\..\..\Release\nxsms.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
; Management console
Source: "..\..\java\build\win32.win32.x86\nxmc\*"; DestDir: "{app}\bin"; Flags: ignoreversion recursesubdirs; Components: console
; Third party files
Source: "..\files\windows\x86\libcrypto.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\files\windows\x86\libcurl.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\files\windows\x86\libmosquitto.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\files\windows\x86\libssh.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\files\windows\x86\libssl.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\files\windows\x86\libmariadb.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\mariadb
Source: "..\files\windows\x86\libmysql.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\mysql
Source: "..\files\windows\x86\libpq.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\pgsql
Source: "..\files\windows\x86\intl.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\pgsql
Source: "..\files\windows\x86\oci.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\oracle
Source: "..\files\windows\x86\oraociei11.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\oracle
Source: "..\files\windows\x86\jre\*"; DestDir: "{app}\bin\jre"; Flags: ignoreversion recursesubdirs; Components: jre
; Install-time files
Source: "..\files\windows\x86\vcredist_x86.exe"; DestDir: "{app}\var"; DestName: "vcredist.exe"; Flags: ignoreversion deleteafterinstall; Components: base
Source: "..\files\windows\x86\rm.exe"; DestDir: "{app}\var"; Flags: ignoreversion deleteafterinstall; Components: base
Source: "..\files\windows\x86\sqlncli.msi"; DestDir: "{app}\var"; Flags: ignoreversion deleteafterinstall; Components: server\mssql

#include "icons.iss"
#include "run-full.iss"
#include "common.iss"
