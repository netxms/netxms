[Dirs]
Name: "{app}\WebUI\base"
Name: "{app}\WebUI\logs"
Name: "{app}\WebUI\work"

[Components]
Name: "webui"; Description: "Web Interface"; Types: full compact custom; Flags: fixed
Name: "jre"; Description: "Java Runtime Environment"; Types: full

[Run]
Filename: "{app}\WebUI\prunsrv.exe"; Parameters: "delete nxWebUI"; WorkingDir: "{app}\WebUI"; StatusMsg: "Removing WebUI service..."; Flags: runhidden
Filename: "{app}\WebUI\prunsrv.exe"; Parameters: "install nxWebUI {code:GenerateInstallParameters}"; WorkingDir: "{app}\WebUI"; StatusMsg: "Installing WebUI service..."; Flags: runhidden
Filename: "{app}\WebUI\prunsrv.exe"; Parameters: "start nxWebUI"; WorkingDir: "{app}\WebUI"; StatusMsg: "Starting WebUI service..."; Flags: runhidden

[UninstallRun]
Filename: "{app}\WebUI\prunsrv.exe"; Parameters: "stop nxWebUI"; WorkingDir: "{app}\WebUI"; StatusMsg: "Stopping WebUI service..."; Flags: runhidden
Filename: "{app}\WebUI\prunsrv.exe"; Parameters: "delete nxWebUI"; WorkingDir: "{app}\WebUI"; StatusMsg: "Removing WebUI service..."; Flags: runhidden

[Code]
var
  DetailsPage : TInputQueryWizardPage;

#include "firewall.iss"

Procedure InitializeWizard;
begin
  DetailsPage := CreateInputQueryPage(wpSelectComponents,
      'Server Settings',
      'Web interface server settings',
      'Please check default settings and adjust them if required');
  DetailsPage.Add('Port:', False);
  DetailsPage.Values[0] := GetPreviousData('JettyPort', '8080');
end;

Function GetJettyPort: String;
begin
  result := DetailsPage.Values[0];
end;

Procedure RegisterPreviousData(PreviousDataKey: Integer);
Begin
  SetPreviousData(PreviousDataKey, 'JettyPort', GetJettyPort());
End;

Function GenerateInstallParameters(Param: String): String;
var
  strJvmArgument: String;
begin
  strJvmArgument := ExpandConstant('--DisplayName="NetXMS WebUI" --Description="NetXMS Web Interface (jetty)" --Install="{app}\WebUI\prunsrv.exe" --Startup=auto --LogPath="{app}\WebUI\logs" --LogLevel=Debug --StdOutput=auto --StdError=auto --StartMode=jvm --StopMode=jvm --Jvm=auto --Classpath="{app}\WebUI\jetty-runner.jar;{app}\WebUI\start.jar" --StartClass=org.eclipse.jetty.runner.Runner ++StartParams=--stop-port ++StartParams=17003 ++StartParams=--stop-key ++StartParams=nxmc$jetty$key ++StartParams=--classes ++StartParams="{app}\WebUI\nxmc\lib"');
  strJvmArgument := strJvmArgument + ' ++StartParams=--port ++StartParams=' + GetJettyPort();
  strJvmArgument := strJvmArgument + ExpandConstant(' ++StartParams=--config ++StartParams="{app}\WebUI\nxmc\jetty.xml"');
  strJvmArgument := strJvmArgument + ExpandConstant(' ++StartParams="{app}\WebUI\nxmc\jetty-web.xml" --StopClass=org.eclipse.jetty.start.Main ++StopParams=-DSTOP.PORT=17003 ++StopParams=-DSTOP.KEY=nxmc$jetty$key ++StopParams=--stop');

  if IsComponentSelected('jre') then
  begin
    strJvmArgument := strJvmArgument + ExpandConstant(' --Jvm="{app}\bin\jre\bin\server\jvm.dll"');
  end;

  strJvmArgument := strJvmArgument + ExpandConstant(' --JvmOptions=-Duser.dir="{app}\WebUI\base";-Djava.io.tmpdir="{app}\WebUI\work";-Djetty.home="{app}\WebUI";-Djetty.base="{app}\WebUI\base"');

  Result := strJvmArgument;
end;

Procedure StopAllServices;
Var
  iResult: Integer;
Begin
  Exec('net.exe', 'stop nxWebUI', ExpandConstant('{app}\WebUI'), 0, ewWaitUntilTerminated, iResult);
End;

Procedure CurStepChanged(CurStep: TSetupStep);
Begin
  If CurStep=ssPostInstall Then Begin
     SetFirewallException('NetXMS WebUI', ExpandConstant('{app}')+'\WebUI\prunsrv.exe');
  End;
End;

Procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
Begin
  If CurUninstallStep=usPostUninstall Then Begin
     RemoveFirewallException(ExpandConstant('{app}')+'\WebUI\prunsrv.exe');
  End;
End;
