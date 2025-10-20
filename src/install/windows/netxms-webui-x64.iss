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
InfoAfterFile=web\readme.txt
Compression=lzma2
LZMANumBlockThreads=4
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

[InstallDelete]
Type: files; Name: "{app}\jetty-base\webapps\ROOT\nxmc-*.jar"
Type: files; Name: "{app}\jetty-base\lib\logging\logback-classic-1.3.5.jar"
Type: files; Name: "{app}\jetty-base\lib\logging\logback-core-1.3.5.jar"
Type: filesandordirs; Name: "{app}\jetty-home\lib\*"
Type: filesandordirs; Name: "{app}\jetty-home\modules\*"

[Files]
; Launcher components
Source: "..\..\..\x64\Release\jansson.dll"; DestDir: "{app}\bin"; BeforeInstall: StopAllServices; Flags: ignoreversion signonce; Components: webui
Source: "..\..\..\x64\Release\libnetxms.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: webui
Source: "..\..\..\x64\Release\nxweblauncher.exe"; DestDir: "{app}\bin"; Flags: ignoreversion signonce; Components: webui
Source: "..\files\windows\x64\libcurl.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\files\windows\x64\libexpat.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\files\windows\x64\pcre.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\files\windows\x64\pcre16.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\files\windows\x64\openssl-3\libcrypto-3-x64.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\files\windows\x64\openssl-3\libssl-3-x64.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
Source: "..\files\windows\x64\zlib.dll"; DestDir: "{app}\bin"; Flags: ignoreversion signonce
; Jetty binaries
Source: "..\files\java\jetty-home\*"; Excludes: "webdefault.xml"; DestDir: "{app}\jetty-home"; Flags: ignoreversion recursesubdirs; Components: webui
Source: "web\webdefault.xml"; DestDir: "{app}\jetty-home\etc"; Flags: ignoreversion; Components: webui
; Web applications and configuration
Source: "web\api.war"; DestDir: "{app}\jetty-base\webapps"; Flags: ignoreversion; Components: api
Source: "web\frontpage\*"; DestDir: "{app}\jetty-base\webapps\ROOT"; Flags: ignoreversion recursesubdirs; Components: webui
Source: "web\jetty-base\*"; DestDir: "{app}\jetty-base"; Flags: onlyifdoesntexist recursesubdirs; Components: webui
Source: "web\launcher.conf"; DestDir: "{app}\etc"; Flags: onlyifdoesntexist; Components: webui
Source: "web\lib\*"; DestDir: "{app}\jetty-base\lib"; Flags: ignoreversion recursesubdirs; Components: webui
Source: "web\nxmc.war"; DestDir: "{app}\jetty-base\webapps"; Flags: ignoreversion; Components: webui
Source: "web\nxmc-{#VersionString}-standalone.jar"; DestDir: "{app}\jetty-base\webapps\ROOT"; Flags: ignoreversion; Components: webui
Source: "web\readme.txt"; DestDir: "{app}"; Flags: ignoreversion; Components: webui
; Java Runtime
Source: "..\files\windows\x64\jre\*"; DestDir: "{app}\jre"; Flags: ignoreversion recursesubdirs; Components: jre
; Install-time files
Source: "..\files\windows\x64\vcredist_x64.exe"; DestDir: "{app}"; DestName: "vcredist.exe"; Flags: ignoreversion deleteafterinstall

[Registry]
Root: HKLM; Subkey: "Software\NetXMS\WebUI"; ValueType: string; ValueName: "InstallDir"; ValueData: "{app}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\NetXMS\WebUI"; ValueType: string; ValueName: "LauncherConfigFile"; ValueData: "{app}\etc\launcher.conf"; Flags: uninsdeletekey

[Run]
Filename: "{app}\vcredist.exe"; Parameters: "/install /passive /norestart"; WorkingDir: "{app}"; StatusMsg: "Installing Visual C++ runtime..."; Flags: waituntilterminated
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
