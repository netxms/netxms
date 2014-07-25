; Installation script for NetXMS WebUI / Windows

#include "setup-webui.iss"
OutputBaseFilename=netxms-webui-1.2.15

[Files]
Source: ..\files\windows\x86\prunsrv.exe; DestDir: "{app}\WebUI"; BeforeInstall: StopAllServices; Flags: ignoreversion; Components: webui
Source: ..\files\java\jetty\jetty-runner.jar; DestDir: "{app}\WebUI"; Flags: ignoreversion; Components: webui
Source: ..\files\java\jetty\start.jar; DestDir: "{app}\WebUI"; Flags: ignoreversion; Components: webui
Source: nxmc\nxmc.war; DestDir: "{app}\WebUI\nxmc"; Flags: ignoreversion; Components: webui
Source: nxmc\nxmc.properties; DestDir: "{app}\WebUI\nxmc\lib"; Flags: ignoreversion; Components: webui
Source: ..\files\windows\x86\jre\*; DestDir: "{app}\bin\jre"; Flags: ignoreversion recursesubdirs; Components: jre

#include "common-webui.iss"
