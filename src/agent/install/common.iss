[Dirs]
Name: "{app}\etc"
Name: "{app}\etc\nxagentd.conf.d"
Name: "{app}\var"

[Registry]
Root: HKLM; Subkey: "Software\NetXMS"; Flags: uninsdeletekeyifempty
Root: HKLM; Subkey: "Software\NetXMS\Agent"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\NetXMS\Agent"; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"
Root: HKLM; Subkey: "Software\NetXMS\Agent"; ValueType: string; ValueName: "ConfigFile"; ValueData: "{app}\etc\nxagentd.conf"
Root: HKLM; Subkey: "Software\NetXMS\Agent"; ValueType: string; ValueName: "ConfigIncludeDir"; ValueData: "{app}\etc\nxagentd.conf.d"

[Run]
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-Z ""{app}\etc\nxagentd.conf"" ""{code:GetMasterServer}"" {{syslog} ""{app}\var"" ""{app}\etc\nxagentd.conf.d"" {code:GetSubagentList}"; WorkingDir: "{app}\bin"; StatusMsg: "Creating agent's config..."; Flags: runhidden
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-c ""{app}\etc\nxagentd.conf"" {code:GetCentralConfigOption} {code:GetCustomCmdLine} -I"; WorkingDir: "{app}\bin"; StatusMsg: "Installing service..."; Flags: runhidden
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-s"; WorkingDir: "{app}\bin"; StatusMsg: "Starting service..."; Flags: runhidden

[UninstallRun]
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-S"; StatusMsg: "Stopping service..."; RunOnceId: "StopService"; Flags: runhidden
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-R"; StatusMsg: "Uninstalling service..."; RunOnceId: "DelService"; Flags: runhidden

[Code]
Var
  ServerSelectionPage: TWizardPage;
  editServerName: TNewEdit;
  cbDownloadConfig: TNewCheckBox;
  SubagentSelectionPage: TInputOptionWizardPage;
  serverName, sbECS, sbLogWatch, sbPing, sbPortCheck, sbWinPerf, sbWMI, sbUPS, sbDownloadConfig: String;

Procedure StopService;
Var
  strExecName : String;
  iResult : Integer;
Begin
  strExecName := ExpandConstant('{app}\bin\nxagentd.exe');
  If FileExists(strExecName) Then
  Begin
    Exec(strExecName, '-S', ExpandConstant('{app}\bin'), 0, ewWaitUntilTerminated, iResult);
  End;
End;

Function BoolToStr(Val: Boolean): String;
Begin
  If Val Then
    Result := 'TRUE'
  Else
    Result := 'FALSE';
End;

Function StrToBool(Val: String): Boolean;
Begin
  If Val = 'TRUE' Then
    Result := TRUE
  Else
    Result := FALSE;
End;

Function InitializeSetup(): Boolean;
Var
  i, nCount : Integer;
  param : String;
Begin
  // Check if we are running on 64-bit Windows
  If (NOT Is64BitInstallMode) AND (ProcessorArchitecture = paX64) Then Begin
    If MsgBox('You are trying to install 32-bit version of NetXMS agent on 64-bit Windows. It is recommended to install 64-bit version instead. Do you really wish to continue installation?', mbConfirmation, MB_YESNO) = IDYES Then
      Result := TRUE
    Else
      Result := FALSE;
  End Else Begin
    Result := TRUE;
  End;
  
  // Empty values for installation data
  serverName := '';
  sbECS := 'FALSE';
  sbLogWatch := 'FALSE';
  sbPing := 'FALSE';
  sbPortCheck := 'FALSE';
  sbWinPerf := 'TRUE';
  sbWMI := 'FALSE';
  sbUPS := 'FALSE';
  sbDownloadConfig := 'FALSE';
  
  // Parse command line parameters
  nCount := ParamCount;
  For i := 1 To nCount Do Begin
    param := ParamStr(i);

    If Pos('/SERVER=', param) = 1 Then Begin
      serverName := param;
      Delete(serverName, 1, 8);
    End;

    If Pos('/CENTRALCONFIG', param) = 1 Then Begin
      sbDownloadConfig := 'TRUE';
    End;

    If Pos('/LOCALCONFIG', param) = 1 Then Begin
      sbDownloadConfig := 'FALSE';
    End;

    If Pos('/SUBAGENT=', param) = 1 Then Begin
      Delete(param, 1, 10);
      param := Uppercase(param);
      If param = 'ECS' Then
        sbECS := 'TRUE';
      If param = 'LOGWATCH' Then
        sbLogWatch := 'TRUE';
      If param = 'PING' Then
        sbPing := 'TRUE';
      If param = 'PORTCHECK' Then
        sbPortCheck := 'TRUE';
      If param = 'WINPERF' Then
        sbWinPerf := 'TRUE';
      If param = 'WMI' Then
        sbWMI := 'TRUE';
      If param = 'UPS' Then
        sbUPS := 'TRUE';
    End;

    If Pos('/NOSUBAGENT=', param) = 1 Then Begin
      Delete(param, 1, 12);
      param := Uppercase(param);
      If param = 'ECS' Then
        sbECS := 'FALSE';
      If param = 'LOGWATCH' Then
        sbLogWatch := 'FALSE';
      If param = 'PING' Then
        sbPing := 'FALSE';
      If param = 'PORTCHECK' Then
        sbPortCheck := 'FALSE';
      If param = 'WINPERF' Then
        sbWinPerf := 'FALSE';
      If param = 'WMI' Then
        sbWMI := 'FALSE';
      If param = 'UPS' Then
        sbUPS := 'FALSE';
    End;
  End;
End;

Procedure InitializeWizard;
Var
  static: TNewStaticText;
Begin
  ServerSelectionPage := CreateCustomPage(wpSelectTasks,
    'NetXMS Server', 'Select your management server.');

  static := TNewStaticText.Create(ServerSelectionPage);
  static.Top := ScaleY(8);
  static.Caption := 'Please enter host name or IP address of your NetXMS server.';
  static.AutoSize := True;
  static.Parent := ServerSelectionPage.Surface;

  editServerName := TNewEdit.Create(ServerSelectionPage);
  editServerName.Top := static.Top + static.Height + ScaleY(8);
  editServerName.Width := ServerSelectionPage.SurfaceWidth - ScaleX(8);
  editServerName.Text := GetPreviousData('MasterServer', serverName);
  editServerName.Parent := ServerSelectionPage.Surface;

  cbDownloadConfig := TNewCheckBox.Create(ServerSelectionPage);
  cbDownloadConfig.Top := editServerName.Top + editServerName.Height + ScaleY(12);
  cbDownloadConfig.Width := ServerSelectionPage.SurfaceWidth;
  cbDownloadConfig.Height := ScaleY(17);
  cbDownloadConfig.Caption := 'Download configuration file from management server on startup';
  cbDownloadConfig.Checked := StrToBool(GetPreviousData('DownloadConfig', sbDownloadConfig));
  cbDownloadConfig.Parent := ServerSelectionPage.Surface;


  SubagentSelectionPage := CreateInputOptionPage(ServerSelectionPage.Id,
    'Subagent Selection', 'Select desired subagents.',
    'Please select additional subagents you wish to load.', False, False);
  SubagentSelectionPage.Add('Extended Checksum Subagent - ecs.nsm');
  SubagentSelectionPage.Add('ICMP Pinger Subagent - ping.nsm');
  SubagentSelectionPage.Add('Log Monitoring Subagent - logwatch.nsm');
  SubagentSelectionPage.Add('Port Checker Subagent - portcheck.nsm');
  SubagentSelectionPage.Add('Windows Performance Subagent - winperf.nsm');
  SubagentSelectionPage.Add('WMI Subagent - wmi.nsm');
  SubagentSelectionPage.Add('UPS Monitoring Subagent - ups.nsm');
  SubagentSelectionPage.Values[0] := StrToBool(GetPreviousData('Subagent_ECS', sbECS));
  SubagentSelectionPage.Values[1] := StrToBool(GetPreviousData('Subagent_PING', sbPing));
  SubagentSelectionPage.Values[2] := StrToBool(GetPreviousData('Subagent_LOGWATCH', sbLogWatch));
  SubagentSelectionPage.Values[3] := StrToBool(GetPreviousData('Subagent_PORTCHECK', sbPortCheck));
  SubagentSelectionPage.Values[4] := StrToBool(GetPreviousData('Subagent_WINPERF', sbWinPerf));
  SubagentSelectionPage.Values[5] := StrToBool(GetPreviousData('Subagent_WMI', sbWMI));
  SubagentSelectionPage.Values[6] := StrToBool(GetPreviousData('Subagent_UPS', sbUPS));
End;

Procedure RegisterPreviousData(PreviousDataKey: Integer);
Begin
  SetPreviousData(PreviousDataKey, 'MasterServer', editServerName.Text);
  SetPreviousData(PreviousDataKey, 'DownloadConfig', BoolToStr(cbDownloadConfig.Checked));
  SetPreviousData(PreviousDataKey, 'Subagent_ECS', BoolToStr(SubagentSelectionPage.Values[0]));
  SetPreviousData(PreviousDataKey, 'Subagent_PING', BoolToStr(SubagentSelectionPage.Values[1]));
  SetPreviousData(PreviousDataKey, 'Subagent_LOGWATCH', BoolToStr(SubagentSelectionPage.Values[2]));
  SetPreviousData(PreviousDataKey, 'Subagent_PORTCHECK', BoolToStr(SubagentSelectionPage.Values[3]));
  SetPreviousData(PreviousDataKey, 'Subagent_WINPERF', BoolToStr(SubagentSelectionPage.Values[4]));
  SetPreviousData(PreviousDataKey, 'Subagent_WMI', BoolToStr(SubagentSelectionPage.Values[5]));
  SetPreviousData(PreviousDataKey, 'Subagent_UPS', BoolToStr(SubagentSelectionPage.Values[6]));
End;

Function GetMasterServer(Param: String): String;
Begin
  Result := editServerName.Text;
End;

Function GetSubagentList(Param: String): String;
Begin
  Result := '';
  If SubagentSelectionPage.Values[0] Then
    Result := Result + 'ecs.nsm ';
  If SubagentSelectionPage.Values[1] Then
    Result := Result + 'ping.nsm ';
  If SubagentSelectionPage.Values[2] Then
    Result := Result + 'logwatch.nsm ';
  If SubagentSelectionPage.Values[3] Then
    Result := Result + 'portcheck.nsm ';
  If SubagentSelectionPage.Values[4] Then
    Result := Result + 'winperf.nsm ';
  If SubagentSelectionPage.Values[5] Then
    Result := Result + 'wmi.nsm ';
  If SubagentSelectionPage.Values[6] Then
    Result := Result + 'ups.nsm ';
End;

Function GetCentralConfigOption(Param: String): String;
Var
  server: String;
  position: Integer;
Begin
  If cbDownloadConfig.Checked Then Begin
    server := editServerName.Text;
    position := Pos(',', server);
    If position > 0 Then Begin
      Delete(server, position, 100000);
    End;
    Result := '-M "' + server + '"';
  End
  Else
    Result := '';
End;

