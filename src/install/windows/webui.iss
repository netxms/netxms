; Installation script for NetXMS WebUI / Windows

#include "setup-webui.iss"
OutputBaseFilename=netxms-webui-1.2.3-rc4

[Files]
Source: files\jre\*; DestDir: "{app}\bin\jre"; Flags: ignoreversion recursesubdirs; Components: jre
Source: jetty\*; DestDir: "{app}\WebUI"; Flags: ignoreversion recursesubdirs; Components: webui
Source: nxmc\nxmc.war; DestDir: "{app}\WebUI\nxmc"; Flags: ignoreversion; Components: webui
Source: files\prunsrv.exe; DestDir: "{app}\WebUI"; Flags: ignoreversion; Components: webui

#include "common-webui.iss"
