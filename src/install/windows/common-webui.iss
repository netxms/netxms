[Components]
Name: "webui"; Description: "Web Interface"; Types: full compact custom; Flags: fixed
Name: "jre"; Description: "Java Runtime Environment"; Types: full

[Run]
Filename: {app}\WebUI\prunsrv.exe; Parameters: "//IS/nxWebUI {code:GenerateInstallParameters}"; WorkingDir: "{app}\WebUI"; StatusMsg: "Installing WebUI service..."; Flags: runhidden
Filename: {app}\WebUI\prunsrv.exe; Parameters: "//ES//nxWebUI"; WorkingDir: "{app}\WebUI"; StatusMsg: "Starting WebUI service..."; Flags: runhidden

[UninstallRun]
Filename: {app}\WebUI\prunsrv.exe; Parameters: "//SS//nxWebUI"; WorkingDir: "{app}\WebUI"; StatusMsg: "Stopping WebUI service..."; Flags: runhidden
Filename: {app}\WebUI\prunsrv.exe; Parameters: "//DS//nxWebUI"; WorkingDir: "{app}\WebUI"; StatusMsg: "Removing WebUI service..."; Flags: runhidden

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
  DetailsPage.Values[0] := '8787';
end;

Function GetJettyPort(Param: String): String;
begin
  result := DetailsPage.Values[0];
end;

Function GenerateInstallParameters(Param: String): String;
var
  strJvmArgument: String;
begin
  strJvmArgument := ExpandConstant('--DisplayName="NetXMS WebUI" --Description="NetXMS Web Interface (jetty)" --Install="{app}\WebUI\prunsrv.exe" --LogPath="{app}\WebUI\logs" --LogLevel=Debug --StdOutput=auto --StdError=auto --StartMode=jvm --StopMode=jvm --Jvm=auto ++JvmOptions=-Djetty.home="{app}\WebUI" ++JvmOptions=-DSTOP.PORT=8087 ++JvmOptions=-DSTOP.KEY=downB0y ++JvmOptions=-Djetty.logs="{app}\WebUI\logs" ++JvmOptions=-Dorg.eclipse.jetty.util.log.SOURCE=true ++JvmOptions=-XX:MaxPermSize=128M ++JvmOptions=-XX:+CMSClassUnloadingEnabled ++JvmOptions=-XX:+CMSPermGenSweepingEnabled --Classpath="{app}\WebUI\start.jar" --StartClass=org.eclipse.jetty.start.Main ++StartParams=OPTIONS=All ++StartParams="{app}\WebUI\etc\jetty.xml" ++StartParams="{app}\WebUI\etc\jetty-deploy.xml" ++StartParams="{app}\WebUI\etc\jetty-contexts.xml" --StopClass=org.eclipse.jetty.start.Main ++StopParams=--stop')

  if IsComponentSelected('jre') then
  begin
    strJvmArgument := strJvmArgument + ExpandConstant(' --Jvm="{app}\bin\jre\bin\server\jvm.dll"');
  end;

  strJvmArgument := strJvmArgument + ' ++JvmOptions=-Djetty.port=' + GetJettyPort('');

  Result := strJvmArgument;

  (* MsgBox(strJvmArgument, mbInformation, MB_OK); *)
end;

