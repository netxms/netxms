[Components]
Name: "webui"; Description: "Web Interface"; Types: full compact custom; Flags: fixed
Name: "jre"; Description: "Java Runtime Environment"; Types: full

[Run]
Filename: "{app}\WebUI\prunsrv.exe"; Parameters: "//IS/nxWebUI {code:GenerateInstallParameters}"; WorkingDir: "{app}\WebUI"; StatusMsg: "Installing WebUI service..."; Flags: runhidden
Filename: "{app}\WebUI\prunsrv.exe"; Parameters: "//ES//nxWebUI"; WorkingDir: "{app}\WebUI"; StatusMsg: "Starting WebUI service..."; Flags: runhidden

[UninstallRun]
Filename: "{app}\WebUI\prunsrv.exe"; Parameters: "//SS//nxWebUI"; WorkingDir: "{app}\WebUI"; StatusMsg: "Stopping WebUI service..."; Flags: runhidden
Filename: "{app}\WebUI\prunsrv.exe"; Parameters: "//DS//nxWebUI"; WorkingDir: "{app}\WebUI"; StatusMsg: "Removing WebUI service..."; Flags: runhidden

[Code]
var
  DetailsPage : TInputQueryWizardPage;

Procedure InitializeWizard;
begin
  DetailsPage := CreateInputQueryPage(wpSelectComponents,
      'Server Settings',
      'Web interface server server settings',
      'Please check default settings and adjust them if required');
  DetailsPage.Add('Port:', False);
  DetailsPage.Values[0] := GetPreviousData('JettyPort', '8787');
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
  strJvmArgument := ExpandConstant('--DisplayName="NetXMS WebUI" --Description="NetXMS Web Interface (winstone)" --Install="{app}\WebUI\prunsrv.exe" --LogPath="{app}\WebUI\logs" --LogLevel=Debug --StdOutput=auto --StdError=auto --StartMode=jvm --StopMode=jvm --Jvm=auto --Classpath="{app}\WebUI\winstone-0.9.10.jar" --StartClass=winstone.Launcher ++StartParams=--controlPort=47777 ++StartParams=--warfile ++StartParams="{app}\WebUI\nxmc\nxmc.war" --StopClass=winstone.tools.WinstoneControl ++StopParams=shutdown ++StopParams=--port=47777')

  if IsComponentSelected('jre') then
  begin
    strJvmArgument := strJvmArgument + ExpandConstant(' --Jvm="{app}\bin\jre\bin\server\jvm.dll"');
  end;

  strJvmArgument := strJvmArgument + ' ++StartParams=--httpPort=' + GetJettyPort();

  Result := strJvmArgument;
end;

Procedure StopAllServices;
Var
  iResult: Integer;
Begin
  Exec('net.exe', 'stop nxWebUI', ExpandConstant('{app}\WebUI'), 0, ewWaitUntilTerminated, iResult);
End;

