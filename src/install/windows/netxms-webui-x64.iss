; Installation script for NetXMS WebUI / Windows x64

#include "..\..\..\build\netxms-build-tag.iss"

[Setup]
AppId=NetXMS-WebUI
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

[Files]
Source: ..\files\windows\x64\prunsrv.exe; DestDir: "{app}\bin"; BeforeInstall: StopAllServices; Flags: ignoreversion; Components: webui
Source: ..\files\java\jetty\jetty-runner.jar; DestDir: "{app}\bin"; Flags: ignoreversion; Components: webui
Source: ..\files\java\jetty\start.jar; DestDir: "{app}\bin"; Flags: ignoreversion; Components: webui
Source: web\server.xml; DestDir: "{app}\config"; Flags: ignoreversion; Components: webui
Source: web\nxmc.xml; DestDir: "{app}\config"; Flags: ignoreversion; Components: webui
Source: web\api.xml; DestDir: "{app}\config"; Flags: ignoreversion; Components: api
Source: web\nxmc.war; DestDir: "{app}\webapps"; Flags: ignoreversion; Components: webui
Source: web\nxmc.properties.sample; DestDir: "{app}\lib"; Flags: ignoreversion; Components: webui
Source: web\api.war; DestDir: "{app}\webapps"; Flags: ignoreversion; Components: api
Source: ..\files\windows\x64\jre\*; DestDir: "{app}\jre"; Flags: ignoreversion recursesubdirs; Components: jre

[Dirs]
Name: "{app}\base"
Name: "{app}\logs"
Name: "{app}\work"

[Components]
Name: "webui"; Description: "Web Interface"; Types: full compact custom; Flags: fixed
Name: "api"; Description: "Web API"; Types: full
Name: "jre"; Description: "Java Runtime Environment"; Types: full

[Run]
Filename: "{app}\bin\prunsrv.exe"; Parameters: "delete nxWebUI"; WorkingDir: "{app}\bin"; StatusMsg: "Removing WebUI service..."; Flags: runhidden
Filename: "{app}\bin\prunsrv.exe"; Parameters: "install nxWebUI {code:GenerateInstallParameters}"; WorkingDir: "{app}\bin"; StatusMsg: "Installing WebUI service..."; Flags: runhidden
Filename: "{app}\bin\prunsrv.exe"; Parameters: "start nxWebUI"; WorkingDir: "{app}\bin"; StatusMsg: "Starting WebUI service..."; Flags: runhidden

[UninstallRun]
Filename: "{app}\bin\prunsrv.exe"; Parameters: "stop nxWebUI"; WorkingDir: "{app}\bin"; StatusMsg: "Stopping WebUI service..."; Flags: runhidden
Filename: "{app}\bin\prunsrv.exe"; Parameters: "delete nxWebUI"; WorkingDir: "{app}\bin"; StatusMsg: "Removing WebUI service..."; Flags: runhidden

[Code]
var
  DetailsPage : TInputQueryWizardPage;

#include "firewall.iss"

Function InitializeSetup: Boolean;
Begin
  Result := Not RegKeyExists(HKEY_LOCAL_MACHINE, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\NetXMS WebUI_is1');
  If Not Result Then
  Begin
    MsgBox('Older version of NetXMS web UI must be uninstalled before installation of this version.', mbError, MB_OK);
  End
End;

Procedure InitializeWizard;
Begin
  DetailsPage := CreateInputQueryPage(wpSelectComponents,
      'Server Settings',
      'Web interface server settings',
      'Please check default settings and adjust them if required');
  DetailsPage.Add('Port:', False);
  DetailsPage.Values[0] := GetPreviousData('JettyPort', '8080');
End;

Function GetJettyPort: String;
Begin
  result := DetailsPage.Values[0];
End;

Procedure RegisterPreviousData(PreviousDataKey: Integer);
Begin
  SetPreviousData(PreviousDataKey, 'JettyPort', GetJettyPort());
End;

Function GenerateInstallParameters(Param: String): String;
Var
  strJvmArgument: String;
Begin
  strJvmArgument := ExpandConstant('--DisplayName="NetXMS WebUI" --Description="NetXMS Web Interface (jetty)" --Install="{app}\bin\prunsrv.exe" --Startup=auto --ServiceUser=LocalSystem --LogPath="{app}\logs" --LogLevel=Debug --StdOutput=auto --StdError=auto --StartMode=jvm --StopMode=jvm --Jvm=auto --Classpath="{app}\bin\jetty-runner.jar;{app}\bin\start.jar" --StartClass=org.eclipse.jetty.runner.Runner ++StartParams=--stop-port ++StartParams=17003 ++StartParams=--stop-key ++StartParams=nxmc$jetty$key ++StartParams=--classes ++StartParams="{app}\lib"');
  strJvmArgument := strJvmArgument + ' ++StartParams=--port ++StartParams=' + GetJettyPort();
  strJvmArgument := strJvmArgument + ExpandConstant(' ++StartParams=--config ++StartParams="{app}\config\server.xml" ++StartParams="{app}\config\nxmc.xml"');

  If IsComponentSelected('api') Then
  Begin
    strJvmArgument := strJvmArgument + ExpandConstant(' ++StartParams="{app}\config\api.xml"');
  End;

  strJvmArgument := strJvmArgument + ExpandConstant(' --StopClass=org.eclipse.jetty.start.Main ++StopParams=-DSTOP.PORT=17003 ++StopParams=-DSTOP.KEY=nxmc$jetty$key ++StopParams=--stop');

  If IsComponentSelected('jre') Then
  Begin
    strJvmArgument := strJvmArgument + ExpandConstant(' --Jvm="{app}\jre\bin\server\jvm.dll"');
  End;

  strJvmArgument := strJvmArgument + ExpandConstant(' --JvmOptions=-Duser.dir="{app}\base";-Djava.io.tmpdir="{app}\work";-Djetty.home="{app}";-Djetty.base="{app}\base"');

  Result := strJvmArgument;
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
     SetFirewallException('NetXMS WebUI', ExpandConstant('{app}')+'\bin\prunsrv.exe');
  End;
End;

Procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
Begin
  If CurUninstallStep=usPostUninstall Then Begin
     RemoveFirewallException(ExpandConstant('{app}')+'\bin\prunsrv.exe');
  End;
End;
