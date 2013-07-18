; Installation script for NetXMS WebUI / Windows

#include "setup-webui.iss"
OutputBaseFilename=netxms-webui-1.2.8

[Files]
Source: files\prunsrv.exe; DestDir: "{app}\WebUI"; BeforeInstall: StopAllServices; Flags: ignoreversion; Components: webui
Source: jetty\*; DestDir: "{app}\WebUI"; Flags: ignoreversion recursesubdirs; Components: webui
Source: nxmc\nxmc.war; DestDir: "{app}\WebUI\nxmc"; Flags: ignoreversion; Components: webui
Source: files\jre\*; DestDir: "{app}\bin\jre"; Flags: ignoreversion recursesubdirs; Components: jre

#include "common-webui.iss"
