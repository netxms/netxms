; Installation script for NetXMS Server / Windows x64

#include "setup.iss"
OutputBaseFilename=netxms-1.1.10-x64-minimal
ArchitecturesInstallIn64BitMode=x64
ArchitecturesAllowed=x64

[Components]
Name: "base"; Description: "Base Files"; Types: full compact custom; Flags: fixed
Name: "tools"; Description: "Command Line Tools"; Types: full
Name: "server"; Description: "NetXMS Server"; Types: full compact
Name: "websrv"; Description: "Web Server"; Types: full
Name: "pdb"; Description: "Install PDB files for selected components"; Types: custom

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; Common files
Source: "..\..\..\ChangeLog"; DestDir: "{app}\doc"; Flags: ignoreversion; Components: base
Source: "..\..\..\x64\Release\libnetxms.dll"; DestDir: "{app}\bin"; BeforeInstall: StopAllServices; Flags: ignoreversion; Components: base
Source: "..\..\..\x64\Release\libnetxms.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\x64\Release\libnetxmsw.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base
Source: "..\..\..\x64\Release\libnetxmsw.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\x64\Release\libexpat.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base
Source: "..\..\..\x64\Release\libexpat.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\x64\Release\libtre.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base
Source: "..\..\..\x64\Release\libtre.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\x64\Release\nxzlib.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base
Source: "..\..\..\x64\Release\nxzlib.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
; Executables and DLLs shared between different components (server, console, etc.)
Source: "..\..\..\x64\Release\libnxcl.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: tools websrv
Source: "..\..\..\x64\Release\libnxcl.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: (tools or websrv) and pdb
Source: "..\..\..\x64\Release\libnxclw.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: tools
Source: "..\..\..\x64\Release\libnxclw.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: tools and pdb
Source: "..\..\..\x64\Release\libnxmap.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: tools server websrv
Source: "..\..\..\x64\Release\libnxmap.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: (tools or server or websrv) and pdb
Source: "..\..\..\x64\Release\libnxmapw.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\libnxmapw.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\libnxsnmpw.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\libnxsnmpw.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\libnxsl.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\libnxsl.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\nxscript.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\nxconfig.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server websrv
; Server files
Source: "..\..\..\x64\Release\nxsqlite.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\nxsqlite.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\libnxsnmp.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\libnxsnmp.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\libnxlp.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\libnxlp.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\libnxdb.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\libnxdb.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\libnxdbw.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\libnxdbw.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\libnxsrv.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\libnxsrv.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\nxcore.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\nxcore.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\netxmsd.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\netxmsd.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
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
Source: "..\..\..\x64\Release\informix.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\informix.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\generic.sms"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\generic.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\nxagent.sms"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\nxagent.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\portech.sms"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\portech.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\dbemu.sms"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\dbemu.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
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
Source: "..\..\..\x64\Release\winnt.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\winnt.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\winperf.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\winperf.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\wmi.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\wmi.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\ping.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\ping.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\portcheck.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\portcheck.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\ecs.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\ecs.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\logwatch.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\logwatch.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\ups.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\ups.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\odbcquery.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\odbcquery.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\avaya-ers.dll"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\avaya-ers.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\baystack.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\baystack.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\ers8000.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\ers8000.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\cisco.dll"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\cisco.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\cat2900xl.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\cat2900xl.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\catalyst.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\catalyst.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\netscreen.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\x64\Release\netscreen.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
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
; Command-line tools files
Source: "..\..\..\x64\Release\nxalarm.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: tools
Source: "..\..\..\x64\Release\nxsms.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: tools
Source: "..\..\..\x64\Release\nxevent.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: tools
Source: "..\..\..\x64\Release\nxpush.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: tools
; Web server files
Source: "..\..\..\x64\Release\libgd.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: websrv
Source: "..\..\..\x64\Release\libgd.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: websrv and pdb
Source: "..\..\..\x64\Release\nxhttpd.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: websrv
Source: "..\..\..\x64\Release\nxhttpd.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: websrv and pdb
Source: "..\..\webui\nxhttpd\static\*.js"; DestDir: "{app}\var\www"; Flags: ignoreversion; Components: websrv
Source: "..\..\webui\nxhttpd\static\netxms.css"; DestDir: "{app}\var\www"; Flags: ignoreversion; Components: websrv
Source: "..\..\webui\nxhttpd\static\images\*.png"; DestDir: "{app}\var\www\images"; Flags: ignoreversion; Components: websrv
Source: "..\..\webui\nxhttpd\static\images\buttons\normal\*.png"; DestDir: "{app}\var\www\images\buttons\normal"; Flags: ignoreversion; Components: websrv
Source: "..\..\webui\nxhttpd\static\images\buttons\pressed\*.png"; DestDir: "{app}\var\www\images\buttons\pressed"; Flags: ignoreversion; Components: websrv
Source: "..\..\webui\nxhttpd\static\images\ctrlpanel\*.png"; DestDir: "{app}\var\www\images\ctrlpanel"; Flags: ignoreversion; Components: websrv
Source: "..\..\webui\nxhttpd\static\images\objects\*.png"; DestDir: "{app}\var\www\images\objects"; Flags: ignoreversion; Components: websrv
Source: "..\..\webui\nxhttpd\static\images\status\*.png"; DestDir: "{app}\var\www\images\status"; Flags: ignoreversion; Components: websrv
; Third party files
Source: "Files-x64\libeay32.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base
; Install-time files
Source: "Files-x64\vcredist_x64.exe"; DestDir: "{app}\var"; DestName: "vcredist.exe"; Flags: ignoreversion deleteafterinstall; Components: base
Source: "Files\rm.exe"; DestDir: "{app}\var"; Flags: ignoreversion deleteafterinstall; Components: base

#include "common.iss"

