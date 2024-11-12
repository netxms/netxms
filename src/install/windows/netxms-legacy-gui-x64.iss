; Installation script for NetXMS legacy GUI / Windows x64

#include "..\..\..\build\netxms-build-tag.iss"

[Setup]
AppId=netxms-legacy-gui
AppName=NetXMS GUI Client (Legacy)
AppVerName=NetXMS GUI Client (Legacy) {#VersionString}
AppVersion={#VersionString}
AppPublisher=Raden Solutions
AppPublisherURL=http://www.radensolutions.com
AppSupportURL=http://www.netxms.org
AppUpdatesURL=http://www.netxms.org
DefaultDirName=C:\Program Files\NetXMS Legacy Client
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
OutputBaseFilename=netxms-legacy-gui-{#VersionString}-x64
ArchitecturesInstallIn64BitMode=x64
ArchitecturesAllowed=x64

[Components]
Name: "nxmc"; Description: "NetXMS Management Console (nxmc)"; Types: full compact custom
Name: "jre"; Description: "Java Runtime Environment"; Types: full

[Files]
Source: "..\..\..\out\nxmc\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs; Components: nxmc
; Java Runtime
Source: "..\files\windows\x64\jre\*"; DestDir: "{app}\jre"; Flags: ignoreversion recursesubdirs; Components: jre

[InstallDelete]
Type: filesandordirs; Name: "{app}\plugins"
Type: filesandordirs; Name: "{app}\p2"

[Icons]
Name: "{group}\NetXMS GUI Client (Legacy)"; Filename: "{app}\nxmc.exe"; Components: nxmc

[Tasks]
Name: desktopicon; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: quicklaunchicon; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
