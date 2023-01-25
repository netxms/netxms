; Installation script for NetXMS WebUI / Windows x64

#include "..\..\..\build\netxms-build-tag.iss"

[Setup]
AppId=NetXMS-WebUI-Jetty
AppName=NetXMS WebUI
AppVerName=NetXMS WebUI {#VersionString}
AppVersion={#VersionString}
AppPublisher=Raden Solutions
AppPublisherURL=http://www.radensolutions.com
AppSupportURL=http://www.netxms.org
AppUpdatesURL=http://www.netxms.org
DefaultDirName=C:\NetXMS-WebUI
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
OutputBaseFilename=netxms-webui-{#VersionString}-x64
ArchitecturesInstallIn64BitMode=x64
ArchitecturesAllowed=x64

[Components]
Name: "webui"; Description: "Web Interface"; Types: full compact custom; Flags: fixed
Name: "api"; Description: "Web API"; Types: full
Name: "jre"; Description: "Java Runtime Environment"; Types: full

[Dirs]
Name: "{app}\jetty-base"; Flags: uninsalwaysuninstall
Name: "{app}\jetty-base\etc"; Flags: uninsalwaysuninstall
Name: "{app}\jetty-base\resources"; Flags: uninsalwaysuninstall
Name: "{app}\jetty-base\work"; Flags: uninsalwaysuninstall
Name: "{app}\logs"; Flags: uninsalwaysuninstall

[Files]
Source: "..\..\..\x64\Release\jansson.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: webui
Source: "..\..\..\x64\Release\libnetxms.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: webui
Source: "..\..\..\x64\Release\libexpat.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\..\..\x64\Release\nxweblauncher.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: webui
Source: "..\..\..\x64\Release\nxzlib.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\files\windows\x64\libcurl.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\files\windows\x64\pcre.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\files\windows\x64\pcre16.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\files\windows\x64\openssl-3\libcrypto-3-x64.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\files\windows\x64\openssl-3\libssl-3-x64.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\files\java\jetty-home\*"; DestDir: "{app}\jetty-home"; Flags: ignoreversion recursesubdirs; Components: webui
Source: "..\files\java\jetty-base\etc\keystore.p12"; DestDir: "{app}\jetty-base\etc"; Flags: onlyifdoesntexist; Components: webui
Source: "..\files\java\jetty-base\lib\*"; DestDir: "{app}\jetty-base\lib"; Flags: ignoreversion recursesubdirs; Components: webui
Source: "..\files\java\jetty-base\start.d\*"; DestDir: "{app}\jetty-base\start.d"; Flags: onlyifdoesntexist; Components: webui
Source: "web\api.war"; DestDir: "{app}\jetty-base\webapps"; Flags: ignoreversion; Components: api
Source: "web\frontpage\*"; DestDir: "{app}\jetty-base\webapps\ROOT"; Flags: ignoreversion recursesubdirs; Components: webui
Source: "web\logback.xml"; DestDir: "{app}\jetty-base\resources"; Flags: onlyifdoesntexist; Components: webui
Source: "web\nxapisrv.properties"; DestDir: "{app}\jetty-base\resources"; Flags: onlyifdoesntexist; Components: api
Source: "web\nxmc.properties"; DestDir: "{app}\jetty-base\resources"; Flags: onlyifdoesntexist; Components: webui
Source: "web\nxmc.war"; DestDir: "{app}\jetty-base\webapps"; Flags: ignoreversion; Components: webui
Source: "web\nxmc-legacy.war"; DestDir: "{app}\jetty-base\webapps"; Flags: ignoreversion; Components: webui
Source: "..\files\windows\x64\jre\*"; DestDir: "{app}\jre"; Flags: ignoreversion recursesubdirs; Components: jre
Source: "web\readme.txt"; DestDir: "{app}"; Flags: isreadme; Components: webui

[Registry]
Root: HKLM; Subkey: "Software\NetXMS\WebUI"; ValueType: string; ValueName: "InstallDir"; ValueData: "{app}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\NetXMS\WebUI"; ValueType: string; ValueName: "LauncherConfigFile"; ValueData: "{app}\etc\launcher.conf"; Flags: uninsdeletekey

[Run]
Filename: "{app}\bin\nxweblauncher.exe"; Parameters: "remove"; WorkingDir: "{app}\bin"; StatusMsg: "Removing WebUI service..."; Flags: runhidden
Filename: "{app}\bin\nxweblauncher.exe"; Parameters: "install"; WorkingDir: "{app}\bin"; StatusMsg: "Installing WebUI service..."; Flags: runhidden
Filename: "net.exe"; Parameters: "start nxWebUI"; WorkingDir: "{app}\bin"; StatusMsg: "Starting WebUI service..."; Flags: runhidden

[UninstallRun]
Filename: "net.exe"; Parameters: "stop nxWebUI"; WorkingDir: "{app}\bin"; RunOnceId: "nxWebUIStopSvc"; Flags: runhidden
Filename: "{app}\bin\nxweblauncher.exe"; Parameters: "remove"; WorkingDir: "{app}\bin"; RunOnceId: "nxWebUIDeleteSvc"; Flags: runhidden

[Code]
#include "firewall.iss"

Function InitializeSetup: Boolean;
Begin
  Result := Not RegKeyExists(HKEY_LOCAL_MACHINE, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\NetXMS WebUI_is1') And Not RegKeyExists(HKEY_LOCAL_MACHINE, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\NetXMS-WebUI_is1');
  If Not Result Then
  Begin
    MsgBox('Older version of NetXMS web UI must be uninstalled before installation of this version.', mbError, MB_OK);
  End
End;

Procedure StopAllServices;
Var
  iResult: Integer;
Begin
  Exec('net.exe', 'stop nxWebUI', ExpandConstant('{app}\bin'), 0, ewWaitUntilTerminated, iResult);
End;

Procedure CurStepChanged(CurStep: TSetupStep);
Begin
  If CurStep=ssPostInstall Then Begin
     SetFirewallException('NetXMS WebUI', ExpandConstant('{app}')+'\bin\nxweblauncher.exe');
     If WizardIsComponentSelected('jre') Then Begin
        SetFirewallException('NetXMS WebUI', ExpandConstant('{app}')+'\jre\bin\java.exe');
     End;
  End;
End;

Procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
Begin
  If CurUninstallStep=usPostUninstall Then Begin
     RemoveFirewallException(ExpandConstant('{app}')+'\bin\nxweblauncher.exe');
     RemoveFirewallException(ExpandConstant('{app}')+'\jre\bin\java.exe');
  End;
End;
