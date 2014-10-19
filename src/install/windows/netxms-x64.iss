; Installation script for NetXMS Server / Windows x64

#include "setup.iss"
OutputBaseFilename=netxms-1.2.17-x64
ArchitecturesInstallIn64BitMode=x64
ArchitecturesAllowed=x64

[Components]
Name: "base"; Description: "Base Files"; Types: full compact custom; Flags: fixed
Name: "console"; Description: "Administrator's Console"; Types: full
Name: "tools"; Description: "Command Line Tools"; Types: full
Name: "server"; Description: "NetXMS Server"; Types: full compact
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
Source: "..\..\..\x64\Release\libnetxms.dll"; DestDir: "{app}\bin"; BeforeInstall: StopAllServices; Flags: ignoreversion; Components: base
Source: "..\..\..\x64\Release\libnetxms.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\x64\Release\libexpat.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base
Source: "..\..\..\x64\Release\libexpat.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\x64\Release\libpng.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base
Source: "..\..\..\x64\Release\libpng.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\x64\Release\libtre.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base
Source: "..\..\..\x64\Release\libtre.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\x64\Release\nxzlib.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base
Source: "..\..\..\x64\Release\nxzlib.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
; Executables and DLLs shared between different components (server, console, etc.)
Source: "..\..\..\x64\Release\libnxcl.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: tools
Source: "..\..\..\x64\Release\libnxcl.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: tools and pdb
Source: "..\..\..\x64\Release\libnxmap.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: tools server
Source: "..\..\..\x64\Release\libnxmap.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: (tools or server) and pdb
Source: "..\..\..\x64\Release\libnxsnmp.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server console
Source: "..\..\..\x64\Release\libnxsnmp.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: (server or console) and pdb
Source: "..\..\..\x64\Release\libnxsl.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server console
Source: "..\..\..\x64\Release\libnxsl.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: (server or console) and pdb
Source: "..\..\..\x64\Release\nxscript.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server console
Source: "..\..\..\x64\Release\nxconfig.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\nxinstall.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
; Server files
Source: "..\..\..\x64\Release\appagent.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\appagent.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\nxsqlite.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\nxsqlite.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\libnxlp.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\libnxlp.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\libnxdb.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\libnxdb.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\libnxsrv.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\libnxsrv.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\libstrophe.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\libstrophe.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\nxcore.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\nxcore.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\netxmsd.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\netxmsd.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\db2.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\db2.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\informix.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\informix.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\mysql.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\mysql.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\mssql.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\mssql.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\odbc.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\odbc.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\pgsql.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\pgsql.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\sqlite.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\sqlite.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\oracle.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\oracle.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\generic.sms"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\generic.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\nxagent.sms"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\nxagent.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\portech.sms"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\portech.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\dbemu.sms"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\dbemu.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\websms.sms"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\websms.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\nxaction.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\nxadm.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\nxdbmgr.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\nxencpasswd.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\nxget.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\nxget.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\nxsnmpget.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\nxsnmpwalk.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\nxsnmpset.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\nxupload.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\nxmibc.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\nxagentd.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\nxagentd.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\nxsagent.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\nxsagent.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\db2.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\db2.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\dbquery.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\dbquery.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\ecs.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\ecs.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\filemgr.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\filemgr.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\informix.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\informix.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\logwatch.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\logwatch.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\netsvc.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\netsvc.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\odbcquery.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\odbcquery.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\oracle.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\oracle.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\ping.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\ping.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\portcheck.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\portcheck.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\sms.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\sms.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\ups.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\ups.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\winnt.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\winnt.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\winperf.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\winperf.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\wmi.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\wmi.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\airespace.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\airespace.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\at.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\at.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\avaya-ers.dll"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\avaya-ers.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\baystack.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\baystack.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\cisco.dll"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\cisco.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\cat2900xl.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\cat2900xl.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\catalyst.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\catalyst.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\cisco-esw.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\cisco-esw.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\cisco-sb.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\cisco-sb.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\dell-pwc.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\dell-pwc.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\ers8000.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\ers8000.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\h3c.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\h3c.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\mikrotik.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\mikrotik.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\netscreen.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\netscreen.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\ntws.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\ntws.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\ping3.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\ping3.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\procurve.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\procurve.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\symbol-ws.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\symbol-ws.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\ubnt.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\ubnt.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\jira.hdlink"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\jira.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\sql\dbinit_mssql.sql"; DestDir: "{app}\lib\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbinit_mysql.sql"; DestDir: "{app}\lib\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbinit_oracle.sql"; DestDir: "{app}\lib\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbinit_pgsql.sql"; DestDir: "{app}\lib\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbinit_sqlite.sql"; DestDir: "{app}\lib\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbschema_mssql.sql"; DestDir: "{app}\lib\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbschema_mysql.sql"; DestDir: "{app}\lib\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbschema_oracle.sql"; DestDir: "{app}\lib\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbschema_pgsql.sql"; DestDir: "{app}\lib\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbschema_sqlite.sql"; DestDir: "{app}\lib\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\contrib\mibs\*.txt"; DestDir: "{app}\var\mibs"; Flags: ignoreversion; Components: server
Source: "..\..\..\contrib\netxmsd.conf-dist"; DestDir: "{app}\etc"; Flags: ignoreversion; Components: server
Source: "..\..\..\contrib\nxagentd.conf-dist"; DestDir: "{app}\etc"; Flags: ignoreversion; Components: server
Source: "..\..\..\images\*"; DestDir: "{app}\var\images"; Flags: ignoreversion; Components: server
; Console files
Source: "..\..\..\x64\Release\scilexer.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "..\..\..\x64\Release\scilexer.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console and pdb
Source: "..\..\..\x64\Release\nxuilib.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "..\..\..\x64\Release\nxuilib.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console and pdb
Source: "..\..\..\x64\Release\nxlexer.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "..\..\..\x64\Release\nxlexer.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console and pdb
Source: "..\..\..\x64\Release\nxcon.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "..\..\..\x64\Release\nxcon.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console and pdb
Source: "..\..\..\x64\Release\nxav.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "..\..\..\x64\Release\nxav.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console and pdb
Source: "..\..\..\x64\Release\nxnotify.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "..\..\..\x64\Release\nxnotify.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console and pdb
Source: "..\..\java\build\win32.win32.x86_64\nxmc\*"; DestDir: "{app}\bin"; Flags: ignoreversion recursesubdirs; Components: console
; Command-line tools files
Source: "..\..\..\x64\Release\nxalarm.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: tools
Source: "..\..\..\x64\Release\nxsms.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: tools
Source: "..\..\..\x64\Release\nxevent.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: tools
Source: "..\..\..\x64\Release\nxpush.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: tools
Source: "..\..\..\x64\Release\nxapush.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: tools
Source: "..\..\..\x64\Release\nxappget.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: tools
; Third party files
Source: "..\files\windows\x64\libeay32.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base
Source: "..\files\windows\x64\ssleay32.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base
Source: "..\files\windows\x64\libcurl.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\files\windows\x64\libmysql.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\mysql
Source: "..\files\windows\x64\libpq.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\pgsql
Source: "..\files\windows\x64\libintl.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\pgsql
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

