
[Icons]
Name: "{group}\Alarm Notifier"; Filename: "{app}\bin\nxnotify.exe"; Components: console
Name: "{group}\Alarm Viewer"; Filename: "{app}\bin\nxav.exe"; Components: console
Name: "{group}\NetXMS Console"; Filename: "{app}\bin\nxcon.exe"; Components: console
Name: "{group}\Recompile MIB Files"; Filename: "{app}\bin\nxmibc.exe"; Parameters: "-P -z -d ""{app}\var\mibs"" -o ""{app}\var\mibs\netxms.mib"""; Components: console
Name: "{group}\Server Configuration Wizard"; Filename: "{app}\bin\nxconfig.exe"; Components: server
Name: "{group}\Server Console"; Filename: "{app}\bin\nxadm.exe"; Parameters: "-i"; Components: server
Name: "{group}\{cm:UninstallProgram,NetXMS}"; Filename: "{uninstallexe}"
Name: "{userdesktop}\NetXMS Console"; Filename: "{app}\bin\nxcon.exe"; Tasks: desktopicon; Components: console
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\NetXMS Console"; Filename: "{app}\bin\nxcon.exe"; Tasks: quicklaunchicon; Components: console

[Dirs]
Name: "{app}\etc"
Name: "{app}\database"
Name: "{app}\var\backgrounds"
Name: "{app}\var\images"
Name: "{app}\var\packages"
Name: "{app}\var\shared"

[Registry]
Root: HKLM; Subkey: "Software\NetXMS"; Flags: uninsdeletekeyifempty; Components: server websrv
Root: HKLM; Subkey: "Software\NetXMS\Server"; Flags: uninsdeletekey; Components: server websrv
Root: HKLM; Subkey: "Software\NetXMS\Server"; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"; Components: server websrv
Root: HKLM; Subkey: "Software\NetXMS\Server"; ValueType: string; ValueName: "ConfigFile"; ValueData: "{app}\etc\netxmsd.conf"; Components: server

[Run]
Filename: "cmd.exe"; Parameters: "/c del {app}\bin\*.manifest"; WorkingDir: "{app}\bin"; StatusMsg: "Removing old manifest files..."; Flags: runhidden
Filename: "msiexec.exe"; Parameters: "/i {app}\var\sqlncli.msi"; WorkingDir: "{app}\bin"; StatusMsg: "Installing Microsoft SQL Native Client..."; Flags: waituntilterminated; Components: server\mssql
Filename: "{app}\bin\nxmibc.exe"; Parameters: "-z -d ""{app}\var\mibs"" -o ""{app}\var\mibs\netxms.mib"""; WorkingDir: "{app}\bin"; StatusMsg: "Compiling MIB files..."; Flags: runhidden; Components: server
Filename: "{app}\bin\nxconfig.exe"; Parameters: "--create-agent-config"; WorkingDir: "{app}\bin"; StatusMsg: "Creating agent's configuration file..."; Components: server
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-c ""{app}\etc\nxagentd.conf"" -I"; WorkingDir: "{app}\bin"; StatusMsg: "Installing agent service..."; Flags: runhidden; Components: server
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-s"; WorkingDir: "{app}\bin"; StatusMsg: "Starting agent service..."; Flags: runhidden; Components: server
Filename: "{app}\bin\nxconfig.exe"; Parameters: "--configure-if-needed"; WorkingDir: "{app}\bin"; StatusMsg: "Running server configuration wizard..."; Components: server
Filename: "{app}\bin\nxdbmgr.exe"; Parameters: "-c ""{app}\etc\netxmsd.conf"" upgrade"; WorkingDir: "{app}\bin"; StatusMsg: "Upgrading database..."; Flags: runhidden; Components: server
Filename: "{app}\bin\netxmsd.exe"; Parameters: "--check-service"; WorkingDir: "{app}\bin"; StatusMsg: "Checking core service configuration..."; Flags: runhidden; Components: server
Filename: "{app}\bin\netxmsd.exe"; Parameters: "-s"; WorkingDir: "{app}\bin"; StatusMsg: "Starting core service..."; Flags: runhidden; Components: server
Filename: "{app}\bin\nxconfig.exe"; Parameters: "--create-nxhttpd-config {code:GetMasterServer}"; WorkingDir: "{app}\bin"; StatusMsg: "Creating web server's configuration file..."; Components: websrv
Filename: "{app}\bin\nxhttpd.exe"; Parameters: "-c ""{app}\etc\nxhttpd.conf"" -I"; WorkingDir: "{app}\bin"; StatusMsg: "Installing web server service..."; Flags: runhidden; Components: websrv
Filename: "{app}\bin\nxhttpd.exe"; Parameters: "-s"; WorkingDir: "{app}\bin"; StatusMsg: "Starting web server service..."; Flags: runhidden; Components: websrv

[UninstallRun]
Filename: "{app}\bin\nxhttpd.exe"; Parameters: "-S"; StatusMsg: "Stopping web server service..."; RunOnceId: "StopWebService"; Flags: runhidden; Components: websrv
Filename: "{app}\bin\nxhttpd.exe"; Parameters: "-R"; StatusMsg: "Uninstalling web server service..."; RunOnceId: "DelWebService"; Flags: runhidden; Components: websrv
Filename: "{app}\bin\netxmsd.exe"; Parameters: "-S"; StatusMsg: "Stopping core service..."; RunOnceId: "StopCoreService"; Flags: runhidden; Components: server
Filename: "{app}\bin\netxmsd.exe"; Parameters: "-R"; StatusMsg: "Uninstalling core service..."; RunOnceId: "DelCoreService"; Flags: runhidden; Components: server
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-S"; StatusMsg: "Stopping agent service..."; RunOnceId: "StopAgentService"; Flags: runhidden; Components: server
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-R"; StatusMsg: "Uninstalling agent service..."; RunOnceId: "DelAgentService"; Flags: runhidden; Components: server

[Code]
Var
  HttpdSettingsPage: TInputQueryWizardPage;
  flagStartConsole: Boolean;

Function InitializeSetup(): Boolean;
Var
  i, nCount : Integer;
  param : String;
Begin
  // Set default values for flags
  flagStartConsole := FALSE;

  // Parse command line parameters
  nCount := ParamCount;
  For i := 1 To nCount Do Begin
    param := ParamStr(i);

    If Pos('/RUNCONSOLE', param) = 1 Then Begin
      flagStartConsole := TRUE;
    End;
  End;
  
  Result := TRUE;
End;

Procedure DeinitializeSetup;
Var
  strExecName: String;
  iResult: Integer;
Begin
  If flagStartConsole Then Begin
    strExecName := ExpandConstant('{app}\bin\nxcon.exe');
    If FileExists(strExecName) Then
    Begin
      Exec(strExecName, '', ExpandConstant('{app}\bin'), SW_SHOW, ewNoWait, iResult);
    End;
  End;
End;

Procedure StopAllServices;
Var
  iResult: Integer;
Begin
  Exec('net.exe', 'stop NetXMSCore', ExpandConstant('{app}\bin'), 0, ewWaitUntilTerminated, iResult);
  Exec('net.exe', 'stop nxhttpd', ExpandConstant('{app}\bin'), 0, ewWaitUntilTerminated, iResult);
  Exec('net.exe', 'stop NetXMSAgentdW32', ExpandConstant('{app}\bin'), 0, ewWaitUntilTerminated, iResult);
End;

Procedure InitializeWizard;
Begin
  HttpdSettingsPage := CreateInputQueryPage(wpSelectTasks,
    'Select Master Server', 'Where is master server for web interface?',
    'Please enter host name or IP address of NetXMS master server. NetXMS web interface you are installing will provide connectivity to that server.');
  HttpdSettingsPage.Add('NetXMS server:', False);
  HttpdSettingsPage.Values[0] := GetPreviousData('MasterServer', 'localhost');
End;

Procedure RegisterPreviousData(PreviousDataKey: Integer);
Begin
  SetPreviousData(PreviousDataKey, 'MasterServer', HttpdSettingsPage.Values[0]);
End;

Function ShouldSkipPage(PageID: Integer): Boolean;
Begin
  If PageID = HttpdSettingsPage.ID Then
    Result := not IsComponentSelected('websrv')
  Else
    Result := False;
End;

Function GetMasterServer(Param: String): String;
Begin
  Result := HttpdSettingsPage.Values[0];
End;
