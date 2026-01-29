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

[Files]
; Common files
Source: "..\..\..\out\x64\Release\bin\libnetxms.dll"; DestDir: "{app}\bin"; BeforeInstall: StopAllServices; Flags: ignoreversion signonce; Components: base
Source: "..\..\..\out\x64\Release\bin\libnxjava.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\..\..\out\x64\Release\bin\libethernetip.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\..\..\out\x64\Release\bin\libargon2.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\..\..\out\x64\Release\bin\nxpng.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\..\..\out\x64\Release\bin\nxjansson.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
; Server core
Source: "..\..\..\out\x64\Release\bin\libnxagent.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\libnxdb.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\libnxlp.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\libnxsl.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\libnxsde.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\libnxsnmp.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\libnxsrv.dll"; DestDir: "{app}\bin"; BeforeInstall: RenameOldFile; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\netxmsd.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\nxcore.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\nxcrashsrv.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\nxscript.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\nxsqlite.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
; Modules
Source: "..\..\..\out\x64\Release\bin\aitools.nxm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\leef.nxm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\ntcb.nxm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\otlp.nxm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\wcc.nxm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\webapi.nxm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\wlcbridge.nxm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
; DB drivers
Source: "..\..\..\out\x64\Release\bin\db2.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\informix.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\mariadb.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\mssql.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\mysql.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\odbc.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\oracle.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\pgsql.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\sqlite.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
; NC drivers
Source: "..\..\..\out\x64\Release\bin\anysms.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\dbtable.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\googlechat.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\gsm.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\kannel.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\matrix.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\mattermost.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\mqtt.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\msteams.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\mymobile.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\nexmo.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\nxagent.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\portech.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\shell.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\slack.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\smseagle.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\smtp.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\snmptrap.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\telegram.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\text2reach.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\textfile.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\twilio.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\websms.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\xmpp.ncd"; DestDir: "{app}\lib\ncdrv"; Flags: ignoreversion signonce; Components: server
; Reporting server
Source: "..\..\server\nxreportd\java\target\nxreportd-{#VersionString}.jar"; DestDir: "{app}\lib\java"; Flags: ignoreversion; Components: server\reporting
Source: "..\..\server\nxreportd\java\target\lib\*.jar"; DestDir: "{app}\lib\java"; Flags: ignoreversion; Components: server\reporting
; Tools
Source: "..\..\..\out\x64\Release\bin\libnxdbmgr.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\nxaction.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\nxadm.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\nxdbmgr.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\nxdownload.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\nxencpasswd.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\nxget.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\nxmibc.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\nxminfo.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\nxsnmpget.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\nxsnmpwalk.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\nxsnmpset.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\nxtftp.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\nxupload.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\nxwsget.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
; Agent
Source: "..\..\..\out\x64\Release\bin\nxagentd.exe"; DestDir: "{app}\bin"; BeforeInstall: RenameOldFile; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\nxsagent.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\aifileops.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\bind9.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\db2.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\dbquery.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\devemu.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\filemgr.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\informix.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\java.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\logwatch.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\mqtt.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\mysql.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\netsvc.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\oracle.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\pgsql.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\ping.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\prometheus.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\sms.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\ssh.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
;; NOT IMPLEMENTED ;; Source: "..\..\..\out\x64\Release\bin\tuxedo.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\ups.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\wineventsync.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\winnt.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\winperf.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\wmi.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\agent\subagents\java\java\target\netxms-agent-{#VersionString}.jar"; DestDir: "{app}\lib\java"; Flags: ignoreversion; Components: server
Source: "..\..\agent\subagents\jmx\target\jmx.jar"; DestDir: "{app}\lib\java"; Flags: ignoreversion; Components: server
Source: "..\..\agent\subagents\opcua\target\opcua.jar"; DestDir: "{app}\lib\java"; Flags: ignoreversion; Components: server
Source: "..\..\agent\subagents\ubntlw\target\ubntlw.jar"; DestDir: "{app}\lib\java"; Flags: ignoreversion; Components: server
Source: "..\..\agent\tools\scripts\nxagentd-generate-tunnel-config.cmd"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
; Network device drivers
Source: "..\..\..\out\x64\Release\bin\at.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\avaya.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\bdcom.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\bulat.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\cambium.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\cisco.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\dell-pwc.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\dlink.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\edgecore.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\eltex.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\etherwan.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\extreme.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\fortinet.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\hirschmann.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\hpe.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\huawei.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\ignitenet.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\juniper.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\lenovo.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\mds.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\mikrotik.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\moxa.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\net-snmp.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\netonix.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\origo.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\ping3.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\planet.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\qtech.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\rittal.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\saf.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\siemens.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\symbol-ws.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\tb.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\tplink.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\ubnt.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\westerstrand.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\zyxel.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion signonce; Components: server
; Fan-out drivers
Source: "..\..\..\out\x64\Release\bin\clickhouse.pdsd"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\influxdb.pdsd"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
; Helpdesk links
Source: "..\..\..\out\x64\Release\bin\jira.nxm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\..\..\out\x64\Release\bin\redmine.nxm"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
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
; AI skills and prompts
Source: "..\..\server\aitools\prompts\*.md"; DestDir: "{app}\share\prompts"; Flags: ignoreversion; Components: server
Source: "..\..\server\aitools\skills\*.md"; DestDir: "{app}\share\skills"; Flags: ignoreversion; Components: server
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
Source: "..\..\..\out\x64\Release\bin\libnxclient.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\out\x64\Release\bin\nxalarm.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\out\x64\Release\bin\nxaevent.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\out\x64\Release\bin\nxap.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\out\x64\Release\bin\nxapush.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\out\x64\Release\bin\nxcsum.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\out\x64\Release\bin\nxethernetip.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\out\x64\Release\bin\nxevent.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\out\x64\Release\bin\nxgenguid.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\out\x64\Release\bin\nxhwid.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\out\x64\Release\bin\nxnotify.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\out\x64\Release\bin\nxpush.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
Source: "..\..\..\out\x64\Release\bin\nxshell.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: tools
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
Source: "..\files\windows\x64\jq.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\files\windows\x64\libcrypto-1_1-x64.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server\pgsql
Source: "..\files\windows\x64\libcurl.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\files\windows\x64\libexpat.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\files\windows\x64\libiconv-2.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\pgsql
Source: "..\files\windows\x64\libintl-8.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\pgsql
Source: "..\files\windows\x64\libmariadb.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\mariadb
Source: "..\files\windows\x64\libmicrohttpd.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\files\windows\x64\libmysql.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\mysql
Source: "..\files\windows\x64\libpq.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\pgsql
Source: "..\files\windows\x64\libssl-1_1-x64.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server\pgsql
Source: "..\files\windows\x64\libstrophe.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\files\windows\x64\modbus.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\files\windows\x64\pcre.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\files\windows\x64\pcre16.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\files\windows\x64\prunsrv.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server\reporting
Source: "..\files\windows\x64\prunmgr.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server\reporting
Source: "..\files\windows\x64\reproxy.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\files\windows\x64\smartctl.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\files\windows\x64\unzip.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\files\windows\x64\zlib.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\files\windows\x64\openssl-3\capi.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\files\windows\x64\openssl-3\libcrypto-3-x64.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\files\windows\x64\openssl-3\libmosquitto.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\files\windows\x64\openssl-3\libssh.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: server
Source: "..\files\windows\x64\openssl-3\libssl-3-x64.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\files\windows\x64\openssl-3\openssl.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: base
Source: "..\files\windows\x64\LICENSE.reproxy"; DestDir: "{app}"; Flags: ignoreversion; Components: server
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
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-Z ""{app}\etc\nxagentd.conf"" 127.0.0.1,::1 ""{app}\log\nxagentd.log"" ""{app}\var"" ""{app}\etc\nxagentd.conf.d"" netsvc ssh winperf wmi"; WorkingDir: "{app}\bin"; StatusMsg: "Creating agent configuration file..."; Components: server
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
  copyPasswordButton: TNewButton;
  taskPageShown, serverWasConfigured: Boolean;
  generatedAdminPassword: String;

Const
  CF_UNICODETEXT = 13;
  GMEM_MOVEABLE = $0002;

Function OpenClipboard(hWndNewOwner: Cardinal): Boolean; external 'OpenClipboard@user32.dll stdcall';
Function EmptyClipboard(): Boolean; external 'EmptyClipboard@user32.dll stdcall';
Function CloseClipboard(): Boolean; external 'CloseClipboard@user32.dll stdcall';
Function SetClipboardData(uFormat: Cardinal; hMem: Cardinal): Cardinal; external 'SetClipboardData@user32.dll stdcall';
Function GlobalAlloc(uFlags: Cardinal; dwBytes: Cardinal): Cardinal; external 'GlobalAlloc@kernel32.dll stdcall';
Function GlobalLock(hMem: Cardinal): Cardinal; external 'GlobalLock@kernel32.dll stdcall';
Function GlobalUnlock(hMem: Cardinal): Boolean; external 'GlobalUnlock@kernel32.dll stdcall';
Function lstrcpyW(lpDest: Cardinal; lpSrc: String): Cardinal; external 'lstrcpyW@kernel32.dll stdcall';

Function CopyTextToClipboard(text: String): Boolean;
Var
  hGlobal: Cardinal;
  pGlobal: Cardinal;
Begin
  Result := False;
  If OpenClipboard(0) Then Begin
    EmptyClipboard();
    hGlobal := GlobalAlloc(GMEM_MOVEABLE, (Length(text) + 1) * 2);
    If hGlobal <> 0 Then Begin
      pGlobal := GlobalLock(hGlobal);
      If pGlobal <> 0 Then Begin
        lstrcpyW(pGlobal, text);
        GlobalUnlock(hGlobal);
        If SetClipboardData(CF_UNICODETEXT, hGlobal) <> 0 Then
          Result := True;
      End;
    End;
    CloseClipboard();
  End;
End;

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
        dbInitCheckCreateDB.Enabled := True;
        dbInitDBALogin.Enabled := dbInitCheckCreateDB.Checked;
        dbInitDBAPassword.Enabled := dbInitCheckCreateDB.Checked;
      End;
    5: { SQLite }
      Begin
        dbInitServerLabel.Caption := 'Path to database file';
        dbInitName.Enabled := False;
        dbInitLogin.Enabled := False;
        dbInitPassword.Enabled := False;
        dbInitCheckCreateDB.Checked := False;
        dbInitCheckCreateDB.Enabled := False;
        dbInitDBALogin.Enabled := False;
        dbInitDBAPassword.Enabled := False;
      End;
    Else
      Begin
        dbInitServerLabel.Caption := 'Database server';
        dbInitName.Enabled := True;
        dbInitLogin.Enabled := True;
        dbInitPassword.Enabled := True;
        dbInitCheckCreateDB.Enabled := True;
        dbInitDBALogin.Enabled := dbInitCheckCreateDB.Checked;
        dbInitDBAPassword.Enabled := dbInitCheckCreateDB.Checked;
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

Procedure CopyPasswordToClipboard(Sender: TObject);
Begin
  If generatedAdminPassword <> '' Then Begin
    If CopyTextToClipboard(generatedAdminPassword) Then
      copyPasswordButton.Caption := 'Copied!'
    Else
      copyPasswordButton.Caption := 'Copy failed';
  End;
End;

Procedure InitializeWizard;
Begin
  CreateDatabaseInitPage;

  copyPasswordButton := TNewButton.Create(WizardForm);
  copyPasswordButton.Parent := WizardForm.FinishedPage;
  copyPasswordButton.Caption := 'Copy Password';
  copyPasswordButton.Left := WizardForm.FinishedLabel.Left;
  copyPasswordButton.Top := WizardForm.FinishedLabel.Top + WizardForm.FinishedLabel.Height + ScaleY(8);
  copyPasswordButton.Width := ScaleX(120);
  copyPasswordButton.Height := ScaleY(25);
  copyPasswordButton.Visible := False;
  copyPasswordButton.OnClick := @CopyPasswordToClipboard;
End;

Procedure CurPageChanged(CurPageID: Integer);
Begin
  If (CurPageID = wpSelectTasks) And Not taskPageShown Then Begin
    taskPageShown := True;
    If serverWasConfigured Then Begin
      WizardSelectTasks('!initializeDatabase');
    End;
  End;
  If (CurPageID = wpFinished) And WizardIsTaskSelected('initializeDatabase') And (generatedAdminPassword <> '') Then Begin
    WizardForm.FinishedLabel.Caption :=
      'Setup has finished installing NetXMS on your computer.' + #13#10 + #13#10 +
      'IMPORTANT: Generated admin password for NetXMS server' + #13#10 +
      '   Login:      admin' + #13#10 +
      '   Password:  ' + generatedAdminPassword + #13#10 + #13#10 +
      'Please save this password - it will not be shown again.' + #13#10 +
      'You will be required to change it on first login.';
    copyPasswordButton.Caption := 'Copy Password';
    copyPasswordButton.Top := WizardForm.FinishedLabel.Top + WizardForm.FinishedLabel.Height + ScaleY(8);
    copyPasswordButton.Visible := True;
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

Function GenerateRandomPassword: String;
Var
  i, r: Integer;
  charset, password: String;
Begin
  charset := 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
  password := '';
  For i := 1 To 16 Do Begin
    r := Random(Length(charset)) + 1;
    password := password + charset[r];
  End;
  Result := password;
End;

Function GetDatabaseInitOptions(arg : String): String;
Var options : String;
Begin
  options := '';
  If dbInitCheckCreateDB.Checked Then Begin
    options := '-C "' + dbInitDBALogin.Text + '/' + dbInitDBAPassword.Text + '"';
  End;
  generatedAdminPassword := GenerateRandomPassword;
  options := options + ' -p ' + generatedAdminPassword;
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
