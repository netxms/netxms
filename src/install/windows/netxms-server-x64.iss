; Installation script for NetXMS Server / Windows x64

#include "..\..\..\build\netxms-build-tag.iss"

[Setup]
AppName=NetXMS
AppVerName=NetXMS {#VersionString}
AppVersion={#VersionString}
AppPublisher=Raden Solutions
AppPublisherURL=http://www.radensolutions.com
AppSupportURL=http://www.netxms.org
AppUpdatesURL=http://www.netxms.org
DefaultDirName=C:\NetXMS
DefaultGroupName=NetXMS
DisableDirPage=auto
DisableProgramGroupPage=auto
AllowNoIcons=yes
WizardStyle=modern
LicenseFile=..\..\..\GPL.txt
Compression=lzma2
LZMANumBlockThreads=4
SolidCompression=yes
LanguageDetectionMethod=none
CloseApplications=no
SignTool=signtool
WizardSmallImageFile=logo_small.bmp
OutputBaseFilename=netxms-server-{#VersionString}-x64
ArchitecturesInstallIn64BitMode=x64
ArchitecturesAllowed=x64

[Components]
Name: "base"; Description: "Base Files"; Types: full compact custom; Flags: fixed
Name: "tools"; Description: "Command Line Tools"; Types: full
Name: "server"; Description: "NetXMS Server"; Types: full compact
Name: "server\mariadb"; Description: "MariaDB Client Library"; Types: full
Name: "server\mysql"; Description: "MySQL Client Library"; Types: full
Name: "server\pgsql"; Description: "PostgreSQL Client Library"; Types: full
Name: "server\reporting"; Description: "Reporting Server"; Types: full
Name: "pdb"; Description: "Install PDB files for selected components"; Types: custom

[Files]
; Common files
Source: "..\..\..\x64\Release\libnetxms.dll"; DestDir: "{app}\bin"; BeforeInstall: StopAllServices; Flags: ignoreversion signonce; Components: base
Source: "..\..\..\x64\Release\libnetxms.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\x64\Release\libnxjava.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\..\..\x64\Release\libnxjava.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\x64\Release\libnxmb.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\..\..\x64\Release\libnxmb.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\x64\Release\libethernetip.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\..\..\x64\Release\libethernetip.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\x64\Release\libpng.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\..\..\x64\Release\libpng.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
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
Source: "..\..\..\x64\Release\libnxsde.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\libnxsde.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\libnxsnmp.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\libnxsnmp.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\libnxsrv.dll"; DestDir: "{app}\bin"; BeforeInstall: RenameOldFile; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\libnxsrv.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\netxmsd.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\netxmsd.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\nxcore.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxcore.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\nxcrashsrv.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxscript.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxsqlite.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxsqlite.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
; Modules
Source: "..\..\..\x64\Release\leef.nxm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\leef.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\ntcb.nxm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\ntcb.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\wcc.nxm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\wcc.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\webapi.nxm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\webapi.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\wlcbridge.nxm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\wlcbridge.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
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
Source: "..\..\..\x64\Release\anysms.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\anysms.pdb"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\dbtable.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\dbtable.pdb"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\googlechat.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\googlechat.pdb"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\gsm.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\gsm.pdb"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\kannel.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\kannel.pdb"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\mattermost.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\mattermost.pdb"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\mqtt.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\msteams.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\msteams.pdb"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\mymobile.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\mymobile.pdb"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\nexmo.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nexmo.pdb"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\nxagent.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxagent.pdb"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\portech.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\portech.pdb"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\shell.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\shell.pdb"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\slack.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\slack.pdb"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\smseagle.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\smseagle.pdb"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\smtp.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\smtp.pdb"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\snmptrap.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\snmptrap.pdb"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\telegram.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\telegram.pdb"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\text2reach.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\text2reach.pdb"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\textfile.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\textfile.pdb"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\twilio.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\twilio.pdb"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\websms.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\websms.pdb"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\xmpp.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\xmpp.pdb"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion; Components: server and pdb
; Reporting server
Source: "..\..\server\nxreportd\java\target\nxreportd-{#VersionString}.jar"; DestDir: "{app}\lib\java"; Flags: ignoreversion; Components: server\reporting
Source: "..\..\server\nxreportd\java\target\lib\*.jar"; DestDir: "{app}\lib\java"; Flags: ignoreversion; Components: server\reporting
; Tools
Source: "..\..\..\x64\Release\libnxdbmgr.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxaction.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxadm.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxdbmgr.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxdownload.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxencpasswd.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxget.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxmibc.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxminfo.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxsnmpget.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxsnmpwalk.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxsnmpset.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxtftp.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxupload.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxwsget.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
; Agent
Source: "..\..\..\x64\Release\libnxpython.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\libnxpython.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\nxagentd.exe"; DestDir: "{app}\bin"; BeforeInstall: RenameOldFile; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxagentd.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\nxsagent.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\nxsagent.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\bind9.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\bind9.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\db2.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\db2.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\dbquery.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\dbquery.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\devemu.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\devemu.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
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
Source: "..\..\..\x64\Release\mysql.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\mysql.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\netsvc.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\netsvc.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\oracle.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\oracle.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\pgsql.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\pgsql.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\ping.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\ping.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
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
Source: "..\..\..\x64\Release\wineventsync.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\wineventsync.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\winnt.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\winnt.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\winperf.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\winperf.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\wmi.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\wmi.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\agent\subagents\java\java\target\netxms-agent-{#VersionString}.jar"; DestDir: "{app}\lib\java"; Flags: ignoreversion; Components: server
Source: "..\..\agent\subagents\jmx\target\jmx.jar"; DestDir: "{app}\lib\java"; Flags: ignoreversion; Components: server
Source: "..\..\agent\subagents\opcua\target\opcua.jar"; DestDir: "{app}\lib\java"; Flags: ignoreversion; Components: server
Source: "..\..\agent\subagents\ubntlw\target\ubntlw.jar"; DestDir: "{app}\lib\java"; Flags: ignoreversion; Components: server
Source: "..\..\agent\tools\scripts\nxagentd-generate-tunnel-config.cmd"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
; Network device drivers
Source: "..\..\..\x64\Release\at.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\at.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\avaya.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\avaya.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\cambium.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\cambium.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\cisco.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\cisco.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\dell-pwc.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\dell-pwc.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\dlink.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\dlink.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\edgecore.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\edgecore.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\eltex.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\eltex.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\etherwan.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\etherwan.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\extreme.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\extreme.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\fortinet.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\fortinet.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\hirschmann.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\hirschmann.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\hpe.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\hpe.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\huawei.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\huawei.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\ignitenet.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\ignitenet.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\juniper.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\juniper.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\mds.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\mds.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\mikrotik.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\mikrotik.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\moxa.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\moxa.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\net-snmp.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\net-snmp.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\netonix.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\netonix.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\ping3.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\ping3.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\planet.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\planet.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\qtech.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\qtech.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\rittal.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\rittal.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\saf.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\saf.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\siemens.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\siemens.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\symbol-ws.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\symbol-ws.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\tb.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\tb.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\tplink.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\tplink.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\ubnt.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\ubnt.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\westerstrand.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\westerstrand.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
; Fan-out drivers
Source: "..\..\..\x64\Release\clickhouse.pdsd"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\clickhouse.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\influxdb.pdsd"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\influxdb.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
; Helpdesk links
Source: "..\..\..\x64\Release\jira.hdlink"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\jira.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\x64\Release\redmine.hdlink"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\x64\Release\redmine.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
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
Source: "..\..\..\contrib\mibs\*.mib"; DestDir: "{app}\share\mibs"; Flags: ignoreversion; Components: server
Source: "..\..\..\contrib\oui\*.csv"; DestDir: "{app}\share\oui"; Flags: ignoreversion; Components: server
Source: "..\..\..\contrib\templates\*.xml"; DestDir: "{app}\share\templates"; Flags: ignoreversion; Components: server
Source: "..\..\..\contrib\netxmsd.conf-dist"; DestDir: "{app}\etc"; Flags: ignoreversion; Components: server
Source: "..\..\..\contrib\nxagentd.conf-dist"; DestDir: "{app}\etc"; Flags: ignoreversion; Components: server
Source: "..\..\..\images\*"; DestDir: "{app}\var\images"; Flags: ignoreversion; Components: server
Source: "..\..\..\contrib\music\*"; DestDir: "{app}\var\files"; Flags: ignoreversion; Components: server
; Common Java libraries
Source: "..\..\java-common\netxms-base\target\netxms-base-{#VersionString}.jar"; DestDir: "{app}\lib\java"; Flags: ignoreversion; Components: base
Source: "..\..\libnxjava\java\target\netxms-java-bridge-{#VersionString}.jar"; DestDir: "{app}\lib\java"; Flags: ignoreversion; Components: base
; Command line tools
Source: "..\..\..\x64\Release\libnxclient.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\x64\Release\libnxclient.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: tools and pdb
Source: "..\..\..\x64\Release\nxalarm.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\x64\Release\nxaevent.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\x64\Release\nxap.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\x64\Release\nxappget.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\x64\Release\nxapush.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\x64\Release\nxcsum.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\x64\Release\nxethernetip.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\x64\Release\nxevent.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\x64\Release\nxgenguid.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\x64\Release\nxhwid.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\x64\Release\nxnotify.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\x64\Release\nxpush.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\x64\Release\nxshell.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\client\java\netxms-client\target\netxms-client-{#VersionString}.jar"; DestDir: "{app}\lib\java"; Flags: ignoreversion; Components: tools or server\reporting
Source: "..\..\client\nxshell\java\target\nxshell-{#VersionString}.jar"; DestDir: "{app}\lib\java"; Flags: ignoreversion; Components: tools
Source: "..\..\client\nxshell\java\target\lib\*.jar"; DestDir: "{app}\lib\java"; Flags: ignoreversion; Components: tools
; License manager
#ifexist "..\..\..\private\common\x64\Release\nxlicmgr.exe"
Source: "..\..\..\private\common\x64\Release\nxlicmgr.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
#endif
; Diagnostic tools
Source: "..\..\server\tools\scripts\nx-collect-server-diag.cmd"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base
Source: "..\..\server\tools\scripts\zip.ps1"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base
; Third party files
Source: "..\files\windows\x64\isotree.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\files\windows\x64\isotree.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\files\windows\x64\jq.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\files\windows\x64\jq.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\files\windows\x64\libcrypto-1_1-x64.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server\pgsql
Source: "..\files\windows\x64\libcurl.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\files\windows\x64\libexpat.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\files\windows\x64\libexpat.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\files\windows\x64\libiconv-2.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\pgsql
Source: "..\files\windows\x64\libintl-8.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\pgsql
Source: "..\files\windows\x64\libmariadb.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\mariadb
Source: "..\files\windows\x64\libmariadb.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\mariadb and pdb
Source: "..\files\windows\x64\libmicrohttpd.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\files\windows\x64\libmicrohttpd.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\files\windows\x64\libmysql.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\mysql
Source: "..\files\windows\x64\libpq.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\pgsql
Source: "..\files\windows\x64\libssl-1_1-x64.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server\pgsql
Source: "..\files\windows\x64\libstrophe.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\files\windows\x64\libstrophe.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\files\windows\x64\modbus.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\files\windows\x64\modbus.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\files\windows\x64\pcre.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\files\windows\x64\pcre16.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\files\windows\x64\prunsrv.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server\reporting
Source: "..\files\windows\x64\prunmgr.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server\reporting
Source: "..\files\windows\x64\smartctl.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\files\windows\x64\unzip.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\files\windows\x64\zlib.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\files\windows\x64\zlib.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\files\windows\x64\openssl-3\capi.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\files\windows\x64\openssl-3\capi.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\files\windows\x64\openssl-3\libcrypto-3-x64.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\files\windows\x64\openssl-3\libcrypto-3-x64.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\files\windows\x64\openssl-3\libmosquitto.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\files\windows\x64\openssl-3\libmosquitto.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\files\windows\x64\openssl-3\libssh.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\files\windows\x64\openssl-3\libssh.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\files\windows\x64\openssl-3\libssl-3-x64.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\files\windows\x64\openssl-3\libssl-3-x64.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\files\windows\x64\openssl-3\openssl.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
; Install-time files
Source: "..\files\windows\x64\vcredist_x64.exe"; DestDir: "{app}\var"; DestName: "vcredist.exe"; Flags: ignoreversion deleteafterinstall; Components: base
Source: "..\files\windows\x64\vcredist-2013-x64.exe"; DestDir: "{app}\var"; Flags: ignoreversion deleteafterinstall; Components: server\pgsql
Source: "..\files\windows\x86\rm.exe"; DestDir: "{app}\var"; Flags: ignoreversion deleteafterinstall; Components: base

[Icons]
Name: "{group}\Collect NetXMS diagnostic information"; Filename: "{app}\bin\nx-collect-server-diag.cmd"; Components: server
Name: "{group}\Server Console"; Filename: "{app}\bin\nxadm.exe"; Parameters: "-i"; Components: server
Name: "{group}\{cm:UninstallProgram,NetXMS}"; Filename: "{uninstallexe}"

[Tasks]
Name: desktopicon; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: quicklaunchicon; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: initializeDatabase; Description: "Initialize database"; Components: server; Flags: checkedonce
Name: upgradeDatabase; Description: "Upgrade database schema if needed"; Components: server
Name: startCore; Description: "Start NetXMS Core service after installation"; Components: server
Name: startReporting; Description: "Start NetXMS Reporting Server service after installation"; Components: server\reporting
Name: fspermissions; Description: "Set hardened file system permissions"

[Dirs]
Name: "{app}\etc"
Name: "{app}\etc\nxreportd"; Components: server\reporting
Name: "{app}\dump"
Name: "{app}\log"
Name: "{app}\var\backgrounds"
Name: "{app}\var\files"
Name: "{app}\var\housekeeper"
Name: "{app}\var\images"
Name: "{app}\var\nxreportd\definitions"; Components: server\reporting
Name: "{app}\var\packages"
Name: "{app}\share"

[Registry]
Root: HKLM; Subkey: "Software\NetXMS"; Flags: uninsdeletekeyifempty; Components: server
Root: HKLM; Subkey: "Software\NetXMS\Server"; Flags: uninsdeletekey; Components: server
Root: HKLM; Subkey: "Software\NetXMS\Server"; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"; Components: server
Root: HKLM; Subkey: "Software\NetXMS\Server"; ValueType: string; ValueName: "ConfigFile"; ValueData: "{app}\etc\netxmsd.conf"; Components: server

[InstallDelete]
Type: files; Name: "{app}\lib\java\*.jar"

[Run]
Filename: "{app}\var\rm.exe"; Parameters: "-f ""{app}\bin\*.manifest"""; WorkingDir: "{app}\bin"; StatusMsg: "Removing old manifest files..."; Flags: runhidden
Filename: "{app}\var\rm.exe"; Parameters: "-rf ""{app}\bin\Microsoft.VC80.*"""; WorkingDir: "{app}\var"; StatusMsg: "Removing old CRT files..."; Flags: runhidden
Filename: "{app}\var\rm.exe"; Parameters: "-rf ""{app}\lib\ndd\Microsoft.VC80.CRT"""; WorkingDir: "{app}\var"; StatusMsg: "Removing old CRT files..."; Flags: runhidden
Filename: "{app}\var\rm.exe"; Parameters: "-f ""{app}\bin\libnetxmsw.*"""; WorkingDir: "{app}\bin"; StatusMsg: "Removing old DLL files..."; Flags: runhidden
Filename: "{app}\var\rm.exe"; Parameters: "-f ""{app}\bin\libnxclw.*"""; WorkingDir: "{app}\bin"; StatusMsg: "Removing old DLL files..."; Flags: runhidden
Filename: "{app}\var\rm.exe"; Parameters: "-f ""{app}\bin\libnxdbw.*"""; WorkingDir: "{app}\bin"; StatusMsg: "Removing old DLL files..."; Flags: runhidden
Filename: "{app}\var\rm.exe"; Parameters: "-f ""{app}\bin\libnxmap.*"""; WorkingDir: "{app}\bin"; StatusMsg: "Removing old DLL files..."; Flags: runhidden
Filename: "{app}\var\rm.exe"; Parameters: "-f ""{app}\bin\libnxmapw.*"""; WorkingDir: "{app}\bin"; StatusMsg: "Removing old DLL files..."; Flags: runhidden
Filename: "{app}\var\rm.exe"; Parameters: "-f ""{app}\bin\libnxsnmpw.*"""; WorkingDir: "{app}\bin"; StatusMsg: "Removing old DLL files..."; Flags: runhidden
Filename: "{app}\var\rm.exe"; Parameters: "-rf ""{app}\lib\sql"""; WorkingDir: "{app}\bin"; StatusMsg: "Removing old SQL files..."; Flags: runhidden
Filename: "{app}\var\rm.exe"; Parameters: "-rf ""{app}\lib\ndd\airespace.*"""; WorkingDir: "{app}\bin"; StatusMsg: "Removing old device drivers..."; Flags: runhidden
Filename: "{app}\var\rm.exe"; Parameters: "-rf ""{app}\lib\ndd\avaya-ers.*"""; WorkingDir: "{app}\bin"; StatusMsg: "Removing old device drivers..."; Flags: runhidden
Filename: "{app}\var\rm.exe"; Parameters: "-rf ""{app}\lib\ndd\baystack.*"""; WorkingDir: "{app}\bin"; StatusMsg: "Removing old device drivers..."; Flags: runhidden
Filename: "{app}\var\rm.exe"; Parameters: "-rf ""{app}\lib\ndd\cat2900xl.*"""; WorkingDir: "{app}\bin"; StatusMsg: "Removing old device drivers..."; Flags: runhidden
Filename: "{app}\var\rm.exe"; Parameters: "-rf ""{app}\lib\ndd\catalyst.*"""; WorkingDir: "{app}\bin"; StatusMsg: "Removing old device drivers..."; Flags: runhidden
Filename: "{app}\var\rm.exe"; Parameters: "-rf ""{app}\lib\ndd\cisco.dll"""; WorkingDir: "{app}\bin"; StatusMsg: "Removing old device drivers..."; Flags: runhidden
Filename: "{app}\var\rm.exe"; Parameters: "-rf ""{app}\lib\ndd\cisco-esw.*"""; WorkingDir: "{app}\bin"; StatusMsg: "Removing old device drivers..."; Flags: runhidden
Filename: "{app}\var\rm.exe"; Parameters: "-rf ""{app}\lib\ndd\cisco-sb.*"""; WorkingDir: "{app}\bin"; StatusMsg: "Removing old device drivers..."; Flags: runhidden
Filename: "{app}\var\rm.exe"; Parameters: "-rf ""{app}\lib\ndd\ers8000.*"""; WorkingDir: "{app}\bin"; StatusMsg: "Removing old device drivers..."; Flags: runhidden
Filename: "{app}\var\rm.exe"; Parameters: "-rf ""{app}\lib\ndd\h3c.*"""; WorkingDir: "{app}\bin"; StatusMsg: "Removing old device drivers..."; Flags: runhidden
Filename: "{app}\var\rm.exe"; Parameters: "-rf ""{app}\lib\ndd\hpsw.*"""; WorkingDir: "{app}\bin"; StatusMsg: "Removing old device drivers..."; Flags: runhidden
Filename: "{app}\var\rm.exe"; Parameters: "-rf ""{app}\lib\ndd\netscreen.*"""; WorkingDir: "{app}\bin"; StatusMsg: "Removing old device drivers..."; Flags: runhidden
Filename: "{app}\var\rm.exe"; Parameters: "-rf ""{app}\lib\ndd\ntws.*"""; WorkingDir: "{app}\bin"; StatusMsg: "Removing old device drivers..."; Flags: runhidden
Filename: "{app}\var\rm.exe"; Parameters: "-rf ""{app}\lib\ndd\procurve.*"""; WorkingDir: "{app}\bin"; StatusMsg: "Removing old device drivers..."; Flags: runhidden
Filename: "{app}\var\rm.exe"; Parameters: "-rf ""{app}\lib\ndd\qtech-olt.*"""; WorkingDir: "{app}\bin"; StatusMsg: "Removing old device drivers..."; Flags: runhidden
Filename: "{app}\var\vcredist.exe"; Parameters: "/install /passive /norestart"; WorkingDir: "{app}\var"; StatusMsg: "Installing Visual C++ runtime..."; Flags: waituntilterminated
Filename: "{app}\var\vcredist-2013-x64.exe"; Parameters: "/install /passive /norestart"; WorkingDir: "{app}\var"; StatusMsg: "Installing Visual C++ 2013 runtime..."; Flags: waituntilterminated; Components: server\pgsql
Filename: "icacls.exe"; Parameters: """{app}"" /inheritance:r"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}"" /grant:r *S-1-5-18:(OI)(CI)F"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}"" /grant:r *S-1-5-19:(OI)(CI)RX"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}"" /grant:r *S-1-5-32-544:(OI)(CI)F"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}\*"" /reset /T"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}"" /grant:r *S-1-5-11:(OI)(CI)RX"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}\etc"" /inheritance:r"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}\etc"" /grant:r *S-1-5-18:(OI)(CI)F"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}\etc"" /grant:r *S-1-5-19:(OI)(CI)RX"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}\etc"" /grant:r *S-1-5-32-544:(OI)(CI)F"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}\log"" /inheritance:r"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}\log"" /grant:r *S-1-5-18:(OI)(CI)F"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}\log"" /grant:r *S-1-5-19:(OI)(CI)F"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}\log"" /grant:r *S-1-5-32-544:(OI)(CI)F"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}\var"" /inheritance:r"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}\var"" /grant:r *S-1-5-18:(OI)(CI)F"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}\var"" /grant:r *S-1-5-19:(OI)(CI)F"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}\var"" /grant:r *S-1-5-32-544:(OI)(CI)F"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "{app}\bin\nxmibc.exe"; Parameters: "-z -d ""{app}\share\mibs"" -d ""{app}\var\mibs"" -o ""{app}\var\netxms.cmib"""; WorkingDir: "{app}\bin"; StatusMsg: "Compiling MIB files..."; Flags: runhidden; Components: server
; Setup agent service
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-Z ""{app}\etc\nxagentd.conf"" 127.0.0.1,::1 ""{app}\log\nxagentd.log"" ""{app}\var"" ""{app}\etc\nxagentd.conf.d"" portcheck.nsm ssh.nsm winperf.nsm wmi.nsm"; WorkingDir: "{app}\bin"; StatusMsg: "Creating agent configuration file..."; Components: server
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-c ""{app}\etc\nxagentd.conf"" -I"; WorkingDir: "{app}\bin"; StatusMsg: "Installing agent service..."; Flags: runhidden; Components: server
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-s"; WorkingDir: "{app}\bin"; StatusMsg: "Starting agent service..."; Flags: runhidden; Components: server
; Setup server core service
Filename: "{app}\bin\netxmsd.exe"; Parameters: "-c ""{app}\etc\netxmsd.conf"" -G {code:GetServerConfigEntries}"; WorkingDir: "{app}\bin"; StatusMsg: "Creating server configuration file..."; Components: server; Tasks: initializeDatabase
Filename: "{app}\bin\nxdbmgr.exe"; Parameters: "-c ""{app}\etc\netxmsd.conf"" init -P -F {code:GetDatabaseSyntax} {code:GetDatabaseInitOptions}"; WorkingDir: "{app}\bin"; StatusMsg: "Initializing database..."; Components: server; Tasks: initializeDatabase
Filename: "{app}\bin\nxdbmgr.exe"; Parameters: "-c ""{app}\etc\netxmsd.conf"" upgrade"; WorkingDir: "{app}\bin"; StatusMsg: "Upgrading database..."; Flags: runhidden; Components: server; Tasks: upgradeDatabase
Filename: "{app}\bin\netxmsd.exe"; Parameters: "-c ""{app}\etc\netxmsd.conf"" -I"; WorkingDir: "{app}\bin"; StatusMsg: "Installing core service..."; Flags: runhidden; Components: server
Filename: "{app}\bin\netxmsd.exe"; Parameters: "-c ""{app}\etc\netxmsd.conf"" --check-service"; WorkingDir: "{app}\bin"; StatusMsg: "Checking core service configuration..."; Flags: runhidden; Components: server
Filename: "{app}\bin\netxmsd.exe"; Parameters: "-s -m"; WorkingDir: "{app}\bin"; StatusMsg: "Starting core service..."; Flags: runhidden; Components: server; Tasks: startCore
; Setup reporting service
Filename: "{app}\bin\prunsrv.exe"; Parameters: "//IS//nxreportd --DisplayName=""NetXMS Reporting Server"" --Description=""This service provides report generation capabilities to NetXMS server"" --Startup=auto --StartMode=jvm --StartClass=org.netxms.reporting.Startup --StopMode=jvm --StopClass=org.netxms.reporting.Startup --StopMethod=stop --StopTimeout=10 ++JvmOptions=""-Dnxreportd.workspace={app}\var\nxreportd"" ++JvmOptions=""-Dnxreportd.logfile={app}\log\nxreportd.log"" ++JvmOptions=""-Dnxreportd.bindir={app}\bin"" --Classpath=""{app}\etc\nxreportd;{app}\lib\java\nxreportd-{#VersionString}.jar"" --LogPath=""{app}\log"""; WorkingDir: "{app}\bin"; StatusMsg: "Installing reporting server service..."; Flags: runhidden; Components: server\reporting
Filename: "{app}\bin\prunsrv.exe"; Parameters: "//US//nxreportd --DisplayName=""NetXMS Reporting Server"" --Description=""This service provides report generation capabilities to NetXMS server"" --Startup=auto --StartMode=jvm --StartClass=org.netxms.reporting.Startup --StopMode=jvm --StopClass=org.netxms.reporting.Startup --StopMethod=stop --StopTimeout=10 ++JvmOptions=""-Dnxreportd.workspace={app}\var\nxreportd"" ++JvmOptions=""-Dnxreportd.logfile={app}\log\nxreportd.log"" ++JvmOptions=""-Dnxreportd.bindir={app}\bin"" --Classpath=""{app}\etc\nxreportd;{app}\lib\java\nxreportd-{#VersionString}.jar"" --LogPath=""{app}\log"""; WorkingDir: "{app}\bin"; StatusMsg: "Installing reporting server service..."; Flags: runhidden; Components: server\reporting
Filename: "{app}\bin\prunsrv.exe"; Parameters: "//ES//nxreportd"; WorkingDir: "{app}\bin"; StatusMsg: "Starting reporting server service..."; Flags: runhidden; Components: server\reporting; Tasks: startReporting

[UninstallRun]
Filename: "{app}\bin\prunsrv.exe"; Parameters: "//SS//nxreportd"; StatusMsg: "Stopping reporting server service..."; RunOnceId: "StopReportingService"; Flags: runhidden; Components: server\reporting
Filename: "{app}\bin\prunsrv.exe"; Parameters: "//DS//nxreportd"; StatusMsg: "Uninstalling reporting server service..."; RunOnceId: "DelReportingService"; Flags: runhidden; Components: server\reporting
Filename: "{app}\bin\netxmsd.exe"; Parameters: "-S"; StatusMsg: "Stopping core service..."; RunOnceId: "StopCoreService"; Flags: runhidden; Components: server
Filename: "{app}\bin\netxmsd.exe"; Parameters: "-R"; StatusMsg: "Uninstalling core service..."; RunOnceId: "DelCoreService"; Flags: runhidden; Components: server
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-S"; StatusMsg: "Stopping agent service..."; RunOnceId: "StopAgentService"; Flags: runhidden; Components: server
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-R"; StatusMsg: "Uninstalling agent service..."; RunOnceId: "DelAgentService"; Flags: runhidden; Components: server

[Code]
Var
  DatabaseInitPage: TWizardPage;
  backupFileSuffix: String;
  dbInitServer, dbInitName, dbInitLogin, dbInitDBALogin: TEdit;
  dbInitPassword, dbInitDBAPassword: TPasswordEdit;
  dbInitCheckCreateDB: TNewCheckBox;
  dbInitType: TNewComboBox;
  dbInitServerLabel: TNewStaticText;
  taskPageShown, serverWasConfigured: Boolean;

#include "firewall.iss"

Procedure StopAllServices;
Var
  iResult: Integer;
Begin
  Exec('net.exe', 'stop nxreportd', ExpandConstant('{app}\bin'), 0, ewWaitUntilTerminated, iResult);
  Exec('net.exe', 'stop NetXMSCore', ExpandConstant('{app}\bin'), 0, ewWaitUntilTerminated, iResult);
  Exec('net.exe', 'stop NetXMSAgentdW32', ExpandConstant('{app}\bin'), 0, ewWaitUntilTerminated, iResult);
End;

Function InitializeSetup(): Boolean;
Var
  flag: Cardinal;
Begin
  serverWasConfigured := False;
  If RegQueryDWordValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\NetXMS\Server', 'ServerIsConfigured', flag) Then Begin
    Log('Server configuration flag from pre-3.9 version is present: ' + IntToStr(flag));
    If flag <> 0 Then Begin
      serverWasConfigured := True;
    End;
  End;

  // Common suffix for backup files
  backupFileSuffix := '.bak.' + GetDateTimeString('yyyymmddhhnnss', #0, #0);
  taskPageShown := False;
  Result := True;
End;

Procedure OnDatabaseSelectionChange(Sender: TObject);
Begin
  Case dbInitType.ItemIndex Of
    3: { Oracle }
      Begin
        dbInitServerLabel.Caption := 'TNS name or connection string';
        dbInitName.Enabled := False;
        dbInitLogin.Enabled := True;
        dbInitPassword.Enabled := True;
      End;
    5: { SQLite }
      Begin
        dbInitServerLabel.Caption := 'Path to database file';
        dbInitName.Enabled := False;
        dbInitLogin.Enabled := False;
        dbInitPassword.Enabled := False;
      End;
    Else
      Begin
        dbInitServerLabel.Caption := 'Database server';
        dbInitName.Enabled := True;
        dbInitLogin.Enabled := True;
        dbInitPassword.Enabled := True;
      End;
  End;
End;

Procedure OnDatabaseCreationOptionChange(Sender: TObject);
Begin
  dbInitDBALogin.Enabled := dbInitCheckCreateDB.Checked;
  dbInitDBAPassword.Enabled := dbInitCheckCreateDB.Checked;
End;

Procedure CreateDatabaseInitPage;
Var
  lblLeft, lblRight: TNewStaticText;
  offset, controlWidth, innerSpacing, outerSpacing: Integer;
Begin
  DatabaseInitPage := CreateCustomPage(wpSelectTasks,
    'Initialize Database', 'Initialize database for use by NetXMS server');

  innerSpacing := ScaleY(3);
  outerSpacing := ScaleY(10);

  { database type }
  lblLeft := TNewStaticText.Create(DatabaseInitPage);
  lblLeft.Anchors := [akLeft, akTop];
  lblLeft.Parent := DatabaseInitPage.Surface;
  lblLeft.Caption := 'Database type';

  offset := lblLeft.Left;
  controlWidth := DatabaseInitPage.Surface.Width / 2 - offset - offset / 2;

  dbInitType := TNewComboBox.Create(DatabaseInitPage);
  dbInitType.Top := lblLeft.Top + lblLeft.Height + innerSpacing;
  dbInitType.Width := controlWidth;
  dbInitType.Anchors := [akLeft, akTop];
  dbInitType.Parent := DatabaseInitPage.Surface;
  dbInitType.Style := csDropDownList;
  dbInitType.Items.Add('MariaDB');
  dbInitType.Items.Add('Microsoft SQL');
  dbInitType.Items.Add('MySQL');
  dbInitType.Items.Add('Oracle');
  dbInitType.Items.Add('PostgreSQL');
  dbInitType.Items.Add('SQLite');
  dbInitType.Items.Add('TimescaleDB');
  dbInitType.ItemIndex := 4;
  dbInitType.OnChange := @OnDatabaseSelectionChange;

  { server name }
  lblLeft := TNewStaticText.Create(DatabaseInitPage);
  lblLeft.Anchors := [akLeft, akTop];
  lblLeft.Parent := DatabaseInitPage.Surface;
  lblLeft.Caption := 'Database server';
  lblLeft.Top := dbInitType.Top + dbInitType.Height + outerSpacing;
  dbInitServerLabel := lblLeft;

  dbInitServer := TEdit.Create(DatabaseInitPage);
  dbInitServer.Anchors := [akLeft, akTop];
  dbInitServer.Parent := DatabaseInitPage.Surface;
  dbInitServer.Top := lblLeft.Top + lblLeft.Height + innerSpacing;
  dbInitServer.Width := controlWidth;

  { database name }
  lblLeft := TNewStaticText.Create(DatabaseInitPage);
  lblLeft.Anchors := [akLeft, akTop];
  lblLeft.Parent := DatabaseInitPage.Surface;
  lblLeft.Caption := 'Database name';
  lblLeft.Top := dbInitServer.Top + dbInitServer.Height + outerSpacing;

  dbInitName := TEdit.Create(DatabaseInitPage);
  dbInitName.Anchors := [akLeft, akTop];
  dbInitName.Parent := DatabaseInitPage.Surface;
  dbInitName.Top := lblLeft.Top + lblLeft.Height + innerSpacing;
  dbInitName.Width := controlWidth;

  { login }
  lblRight := TNewStaticText.Create(DatabaseInitPage);
  lblRight.Anchors := [akTop, akRight];
  lblRight.Parent := DatabaseInitPage.Surface;
  lblRight.Caption := 'Login name';
  lblRight.Top := dbInitType.Top + dbInitType.Height + outerSpacing;
  lblRight.Left := DatabaseInitPage.Surface.Width / 2 + offset / 2;

  dbInitLogin := TEdit.Create(DatabaseInitPage);
  dbInitLogin.Anchors := [akTop, akRight];
  dbInitLogin.Parent := DatabaseInitPage.Surface;
  dbInitLogin.Top := lblRight.Top + lblRight.Height + innerSpacing;
  dbInitLogin.Left := lblRight.Left;
  dbInitLogin.Width := controlWidth;

  { password }
  lblRight := TNewStaticText.Create(DatabaseInitPage);
  lblRight.Anchors := [akTop, akRight];
  lblRight.Parent := DatabaseInitPage.Surface;
  lblRight.Caption := 'Password';
  lblRight.Top := dbInitLogin.Top + dbInitLogin.Height + outerSpacing;
  lblRight.Left := DatabaseInitPage.Surface.Width / 2 + offset / 2;

  dbInitPassword := TPasswordEdit.Create(DatabaseInitPage);
  dbInitPassword.Anchors := [akTop, akRight];
  dbInitPassword.Parent := DatabaseInitPage.Surface;
  dbInitPassword.Top := lblRight.Top + lblRight.Height + innerSpacing;
  dbInitPassword.Left := lblRight.Left;
  dbInitPassword.Width := controlWidth;

  { database creation option }
  dbInitCheckCreateDB := TNewCheckBox.Create(DatabaseInitPage);
  dbInitCheckCreateDB.Anchors := [akLeft, akTop];
  dbInitCheckCreateDB.Parent := DatabaseInitPage.Surface;
  dbInitCheckCreateDB.Caption := 'Create database and database user before initialization';
  dbInitCheckCreateDB.Top := dbInitName.Top + dbInitName.Height + outerSpacing * 2;
  dbInitCheckCreateDB.Width := controlWidth * 2;
  dbInitCheckCreateDB.OnClick := @OnDatabaseCreationOptionChange;

  { DBA login }
  lblLeft := TNewStaticText.Create(DatabaseInitPage);
  lblLeft.Anchors := [akLeft, akTop];
  lblLeft.Parent := DatabaseInitPage.Surface;
  lblLeft.Caption := 'DBA login name';
  lblLeft.Top := dbInitCheckCreateDB.Top + dbInitCheckCreateDB.Height + outerSpacing;

  dbInitDBALogin := TEdit.Create(DatabaseInitPage);
  dbInitDBALogin.Anchors := [akLeft, akTop];
  dbInitDBALogin.Parent := DatabaseInitPage.Surface;
  dbInitDBALogin.Top := lblLeft.Top + lblLeft.Height + innerSpacing;
  dbInitDBALogin.Width := controlWidth;
  dbInitDBALogin.Enabled := False;

  { DBA password }
  lblRight := TNewStaticText.Create(DatabaseInitPage);
  lblRight.Anchors := [akTop, akRight];
  lblRight.Parent := DatabaseInitPage.Surface;
  lblRight.Caption := 'DBA password';
  lblRight.Top := dbInitCheckCreateDB.Top + dbInitCheckCreateDB.Height + outerSpacing;
  lblRight.Left := DatabaseInitPage.Surface.Width / 2 + offset / 2;

  dbInitDBAPassword := TPasswordEdit.Create(DatabaseInitPage);
  dbInitDBAPassword.Anchors := [akTop, akRight];
  dbInitDBAPassword.Parent := DatabaseInitPage.Surface;
  dbInitDBAPassword.Top := lblRight.Top + lblRight.Height + innerSpacing;
  dbInitDBAPassword.Left := lblRight.Left;
  dbInitDBAPassword.Width := controlWidth;
  dbInitDBAPassword.Enabled := False;
End;

Procedure InitializeWizard;
Begin
  CreateDatabaseInitPage;
End;

Procedure CurPageChanged(CurPageID: Integer);
Begin
  If (CurPageID = wpSelectTasks) And Not taskPageShown Then Begin
    taskPageShown := True;
    If serverWasConfigured Then Begin
      WizardSelectTasks('!initializeDatabase');
    End;
  End;
End;

Function ShouldSkipPage(PageID: Integer): Boolean;
Begin
  If PageID = DatabaseInitPage.ID Then
    Result := not WizardIsTaskSelected('initializeDatabase')
  Else
    Result := False;
End;

Function GetDatabaseType: String;
Begin
  Case dbInitType.ItemIndex Of
    0: Result := 'MariaDB';
    1: Result := 'Microsoft SQL';
    2: Result := 'MySQL';
    3: Result := 'Oracle';
    4: Result := 'PostgreSQL';
    5: Result := 'SQLite';
    6: Result := 'TimescaleDB';
  End;
End;

Function GetDatabaseSyntax(arg : String): String;
Begin
  Case dbInitType.ItemIndex Of
    0: Result := 'mysql';
    1: Result := 'mssql';
    2: Result := 'mysql';
    3: Result := 'oracle';
    4: Result := 'pgsql';
    5: Result := 'sqlite';
    6: Result := 'tsdb';
  End;
End;

Function GetDatabaseDriverName: String;
Begin
  Case dbInitType.ItemIndex Of
    0: Result := 'mariadb.ddr';
    1: Result := 'mssql.ddr';
    2: Result := 'mysql.ddr';
    3: Result := 'oracle.ddr';
    4: Result := 'pgsql.ddr';
    5: Result := 'sqlite.ddr';
    6: Result := 'pgsql.ddr';
  End;
End;

Function UpdateReadyMemo(Space, NewLine, MemoUserInfoInfo, MemoDirInfo, MemoTypeInfo,
                         MemoComponentsInfo, MemoGroupInfo, MemoTasksInfo: String): String;
Var
  Server, Text: String;
Begin
  Text := '';
  If MemoUserInfoInfo <> '' Then
    Text := Text + MemoUserInfoInfo + NewLine + NewLine;
  If MemoDirInfo <> '' Then
    Text := Text + MemoDirInfo + NewLine + NewLine;
  If MemoTypeInfo <> '' Then
    Text := Text + MemoTypeInfo + NewLine + NewLine;
  If MemoComponentsInfo <> '' Then
    Text := Text + MemoComponentsInfo + NewLine + NewLine;
  If MemoGroupInfo <> '' Then
    Text := Text + MemoGroupInfo + NewLine + NewLine;
  If MemoTasksInfo <> '' Then
    Text := Text + MemoTasksInfo + NewLine + NewLine;
  If WizardIsTaskSelected('initializeDatabase') Then Begin
    Text := Text + 'Initialize database:' + NewLine;
    Text := Text + Space + 'Database type: ' + GetDatabaseType + NewLine;
    If dbInitType.ItemIndex = 3 Then Begin
      Text := Text + Space + 'Connection string: ' + dbInitServer.Text + NewLine;
      Text := Text + Space + 'Login: ' + dbInitLogin.Text + NewLine;
    End
    Else Begin
      If dbInitType.ItemIndex = 5 Then Begin
        Server := dbInitServer.Text;
        If Server = '' Then
          Server := ExpandConstant('{app}\var\netxmsd.db');
        Text := Text + Space + 'Path to database file: ' + Server + NewLine;
      End
      Else Begin
        Server := dbInitServer.Text;
        If Server = '' Then
          Server := 'localhost';
        Text := Text + Space + 'Server: ' + Server + NewLine;
        Text := Text + Space + 'Database name: ' + dbInitName.Text + NewLine;
        Text := Text + Space + 'Login: ' + dbInitLogin.Text + NewLine;
      End;
    End;
  End;

  Result := Text;
End;

Function GetServerConfigEntries(arg : String): String;
Var options, dbFile : String;
Begin
  options := '-A DBDriver=' + GetDatabaseDriverName;
  If dbInitType.ItemIndex = 5 Then { SQLite }
  Begin
    dbFile := dbInitServer.Text;
    If dbFile = '' Then
      dbFile := ExpandConstant('{app}\var\netxmsd.db');
    options := options + ' -A "DBName=' + dbFile + '"';
  End Else Begin
    options := options + ' -A DBServer=';
    If dbInitServer.Text = '' Then
      options := options + 'localhost'
    Else
      options := options + dbInitServer.Text;

    If (dbInitType.ItemIndex <> 3) And (dbInitName.Text <> '') Then { not Oracle }
      options := options + ' -A DBName=' + dbInitName.Text;

    If dbInitLogin.Text <> '' Then
      options := options + ' -A DBLogin=' + dbInitLogin.Text;

    If dbInitPassword.Text <> '' Then
      options := options + ' -A DBPassword=' + dbInitPassword.Text;
  End;

  options := options + ' -A "LogFile=' + ExpandConstant('{app}\log\netxmsd.log') + '"';
  Result := options;
End;

Function GetDatabaseInitOptions(arg : String): String;
Var options : String;
Begin
  options := '';
  If dbInitCheckCreateDB.Checked Then Begin
    options := '-C "' + dbInitDBALogin.Text + '/' + dbInitDBAPassword.Text + '"';
  End;
  Result := options;
End;

Procedure RenameOldFile;
Begin
  RenameFile(ExpandConstant(CurrentFileName), ExpandConstant(CurrentFileName) + backupFileSuffix);
End;

Procedure DeleteBackupFiles;
Var
  fd: TFindRec;
  basePath: String;
Begin
  basePath := ExpandConstant('{app}\bin\');
  If FindFirst(basePath + '*.bak.*', fd) Then
  Begin
    Try
      Repeat
        DeleteFile(basePath + fd.Name);
      Until Not FindNext(fd);
    Finally
      FindClose(fd);
    End;
  End;
End;

Procedure CurStepChanged(CurStep: TSetupStep);
Begin
  If CurStep=ssPostInstall Then Begin
    SetFirewallException('NetXMS Server', ExpandConstant('{app}')+'\bin\netxmsd.exe');
    SetFirewallException('NetXMS Agent', ExpandConstant('{app}')+'\bin\nxagentd.exe');
    SetFirewallException('NetXMS TFTP Client', ExpandConstant('{app}')+'\bin\nxtftp.exe');
    DeleteBackupFiles;
  End;
End;

Procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
Begin
  If CurUninstallStep=usPostUninstall Then Begin
    RemoveFirewallException(ExpandConstant('{app}')+'\bin\netxmsd.exe');
    RemoveFirewallException(ExpandConstant('{app}')+'\bin\nxagentd.exe');
    RemoveFirewallException(ExpandConstant('{app}')+'\bin\nxtftp.exe');
  End;
End;
