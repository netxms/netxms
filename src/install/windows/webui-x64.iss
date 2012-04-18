; Installation script for NetXMS WebUI / Windows x64

#include "setup-webui.iss"

OutputBaseFilename=netxms-webui-1.2.0-x64
ArchitecturesInstallIn64BitMode=x64
ArchitecturesAllowed=x64

[Files]
Source: files-x64\jre\*; DestDir: "{app}\bin\jre"; Flags: ignoreversion recursesubdirs; Components: jre
Source: jetty\*; DestDir: {app}\WebUI; Flags: ignoreversion recursesubdirs; Components: webui
Source: nxmc\nxmc.war; DestDir: {app}\WebUI\nxmc; Flags: ignoreversion; Components: webui
Source: files-x64\prunsrv.exe; DestDir: {app}\WebUI; Flags: ignoreversion; Components: webui

#include "common-webui.iss"
