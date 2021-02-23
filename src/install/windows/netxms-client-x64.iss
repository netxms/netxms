; Installation script for NetXMS WebUI / Windows x64

#include "..\..\..\build\netxms-build-tag.iss"

[Setup]
AppId=netxmsclient
AppName=NetXMS Client
AppVerName=NetXMS Client {#VersionString}
AppVersion={#VersionString}
AppPublisher=Raden Solutions
AppPublisherURL=http://www.radensolutions.com
AppSupportURL=http://www.netxms.org
AppUpdatesURL=http://www.netxms.org
DefaultDirName=C:\Program Files\NetXMS Client
DefaultGroupName=NetXMS
DisableDirPage=auto
DisableProgramGroupPage=auto
AllowNoIcons=yes
WizardStyle=modern
LicenseFile=..\..\..\GPL.txt
Compression=lzma
SolidCompression=yes
LanguageDetectionMethod=none
CloseApplications=no
SignTool=signtool
OutputBaseFilename=netxms-client-{#VersionString}-x64
ArchitecturesInstallIn64BitMode=x64
ArchitecturesAllowed=x64

[Components]
Name: "nxmc"; Description: "NetXMS Management Console (nxmc)"; Types: full compact custom
Name: "nxshell"; Description: "NetXMS Python Scripting (nxshell)"; Types: full custom
Name: "cmdline"; Description: "Command Line Tools"; Types: full
Name: "jre"; Description: "Java Runtime Environment"; Types: full

[Files]
Source: ..\..\java\netxms-eclipse\Product\target\products\release\nxmc\*; DestDir: "{app}"; Flags: ignoreversion recursesubdirs; Components: nxmc
; Common files for nxshell and command line tools
Source: "..\..\..\x64\Release\jansson.dll"; DestDir: "{app}"; Flags: ignoreversion signonce; Components: nxshell or cmdline
Source: "..\..\..\x64\Release\libnetxms.dll"; DestDir: "{app}"; Flags: ignoreversion signonce; Components: nxshell or cmdline
Source: "..\..\..\x64\Release\libexpat.dll"; DestDir: "{app}"; Flags: ignoreversion signonce; Components: nxshell or cmdline
Source: "..\..\..\x64\Release\nxzlib.dll"; DestDir: "{app}"; Flags: ignoreversion signonce; Components: nxshell or cmdline
Source: "..\files\windows\x64\libcrypto.dll"; DestDir: "{app}"; Flags: ignoreversion signonce; Components: nxshell or cmdline
Source: "..\files\windows\x64\libcurl.dll"; DestDir: "{app}"; Flags: ignoreversion signonce; Components: nxshell or cmdline
Source: "..\files\windows\x64\libssl.dll"; DestDir: "{app}"; Flags: ignoreversion signonce; Components: nxshell or cmdline
Source: "..\files\windows\x64\pcre.dll"; DestDir: "{app}"; Flags: ignoreversion signonce; Components: nxshell or cmdline
Source: "..\files\windows\x64\pcre16.dll"; DestDir: "{app}"; Flags: ignoreversion signonce; Components: nxshell or cmdline
; nxshell
Source: "..\..\..\x64\Release\nxshell.exe"; DestDir: "{app}"; Flags: ignoreversion signonce; Components: nxshell
Source: "..\..\..\x64\Release\libnxjava.dll"; DestDir: "{app}"; Flags: ignoreversion signonce; Components: nxshell
Source: "..\..\client\nxshell\java\target\nxshell-{#VersionString}.jar"; DestDir: "{app}\lib\java"; Flags: ignoreversion; Components: nxshell
Source: "..\..\client\nxshell\java\target\lib\*.jar"; DestDir: "{app}\lib\java"; Flags: ignoreversion; Components: nxshell
; Command line tools
Source: "..\..\..\x64\Release\libnxclient.dll"; DestDir: "{app}"; Flags: ignoreversion signonce; Components: cmdline
Source: "..\..\..\x64\Release\nxalarm.exe"; DestDir: "{app}"; Flags: ignoreversion signonce; Components: cmdline
Source: "..\..\..\x64\Release\nxevent.exe"; DestDir: "{app}"; Flags: ignoreversion signonce; Components: cmdline
Source: "..\..\..\x64\Release\nxnotify.exe"; DestDir: "{app}"; Flags: ignoreversion signonce; Components: cmdline
Source: "..\..\..\x64\Release\nxpush.exe"; DestDir: "{app}"; Flags: ignoreversion signonce; Components: cmdline
; Java Runtime
Source: "..\files\windows\x64\jre\*"; DestDir: "{app}\jre"; Flags: ignoreversion recursesubdirs; Components: jre
; Install-time files
Source: "..\files\windows\x64\vcredist_x64.exe"; DestDir: "{app}"; DestName: "vcredist.exe"; Flags: ignoreversion deleteafterinstall; Components: nxshell or cmdline

[InstallDelete]
Type: files; Name: "{app}\lib\java\*.jar"
Type: filesandordirs; Name: "{app}\plugins"
Type: filesandordirs; Name: "{app}\p2"

[Icons]
Name: "{group}\NetXMS Management Console"; Filename: "{app}\nxmc.exe"; Components: nxmc

[Tasks]
Name: desktopicon; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: quicklaunchicon; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Run]
Filename: "{app}\vcredist.exe"; Parameters: "/install /passive /norestart"; WorkingDir: "{app}"; StatusMsg: "Installing Visual C++ runtime..."; Flags: waituntilterminated; Components: nxshell
