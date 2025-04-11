[Dirs]
Name: "{app}\etc"
Name: "{app}\etc\nxagentd.conf.d"
Name: "{app}\lib"
Name: "{app}\log"
Name: "{app}\var"

[Tasks]
Name: fspermissions; Description: "Set hardened file system permissions"
Name: sessionagent; Description: "Install session agent (will run in every user session)"; Flags: unchecked
Name: useragent; Description: "Install user support application (will run in every user session)"; Flags: unchecked

[Registry]
Root: HKLM; Subkey: "Software\NetXMS"; Flags: uninsdeletekeyifempty
Root: HKLM; Subkey: "Software\NetXMS\Agent"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\NetXMS\Agent"; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"
Root: HKLM; Subkey: "Software\NetXMS\Agent"; ValueType: string; ValueName: "ConfigFile"; ValueData: "{app}\etc\nxagentd.conf"
Root: HKLM; Subkey: "Software\NetXMS\Agent"; ValueType: string; ValueName: "ConfigIncludeDir"; ValueData: "{app}\etc\nxagentd.conf.d"
Root: HKLM; Subkey: "Software\NetXMS\Agent"; ValueType: dword; ValueName: "WithUserAgent"; ValueData: "1"; Tasks: useragent
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "NetXMSSessionAgent"; ValueData: """{app}\bin\nxsagent.exe"" -H"; Flags: uninsdeletevalue; Tasks: sessionagent
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "NetXMSUserAgent"; ValueData: """{app}\bin\nxuseragent.exe"""; Flags: uninsdeletevalue; Tasks: useragent

[InstallDelete]
Type: files; Name: "{app}\lib\java\*.jar"

[Run]
Filename: "icacls.exe"; Parameters: """{app}"" /inheritance:r"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}"" /grant:r *S-1-5-18:(OI)(CI)F"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}"" /grant:r *S-1-5-32-544:(OI)(CI)F"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}\*"" /reset /T"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}"" /grant:r *S-1-5-11:(OI)(CI)RX"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}\etc"" /inheritance:r"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}\etc"" /grant:r *S-1-5-18:(OI)(CI)F"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}\etc"" /grant:r *S-1-5-32-544:(OI)(CI)F"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}\log"" /inheritance:r"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}\log"" /grant:r *S-1-5-18:(OI)(CI)F"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}\log"" /grant:r *S-1-5-32-544:(OI)(CI)F"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}\var"" /inheritance:r"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}\var"" /grant:r *S-1-5-18:(OI)(CI)F"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "icacls.exe"; Parameters: """{app}\var"" /grant:r *S-1-5-32-544:(OI)(CI)F"; StatusMsg: "Setting file system permissions..."; Flags: runhidden waituntilterminated; Tasks: fspermissions
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-Z ""{app}\etc\nxagentd.conf"" {code:GetForceCreateConfigFlag} ""{code:GetMasterServer}"" ""{code:GetLogFile}"" ""{code:GetFileStore}"" ""{code:GetConfigIncludeDir}"" {code:GetSubagentList} {code:GetExtraConfigValues}"; WorkingDir: "{app}\bin"; StatusMsg: "Creating agent's config..."; Flags: runhidden
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-c ""{app}\etc\nxagentd.conf"" {code:GetCentralConfigOption} {code:GetCustomCmdLine} -I"; WorkingDir: "{app}\bin"; StatusMsg: "Installing service..."; Flags: runhidden
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-s"; WorkingDir: "{app}\bin"; StatusMsg: "Starting service..."; Flags: runhidden
Filename: "{tmp}\nxreload.exe"; Parameters: "-finish"; WorkingDir: "{tmp}"; StatusMsg: "Waiting for external processes..."; Flags: runhidden waituntilterminated

[UninstallRun]
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-K"; StatusMsg: "Stopping external subagents..."; RunOnceId: "StopExternalAgents"; Flags: runhidden
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-S"; StatusMsg: "Stopping service..."; RunOnceId: "StopService"; Flags: runhidden
Filename: "taskkill.exe"; Parameters: "/F /IM nxagentd.exe /T"; StatusMsg: "Killing agent processes..."; RunOnceId: "KillProcess"; Flags: runhidden
Filename: "taskkill.exe"; Parameters: "/F /IM nxcrashsrv.exe /T"; StatusMsg: "Killing agent processes..."; RunOnceId: "KillCrashServerProcess"; Flags: runhidden
Filename: "taskkill.exe"; Parameters: "/F /IM nxsagent.exe /T"; StatusMsg: "Killing session agent processes..."; RunOnceId: "KillSessionAgentProcess"; Flags: runhidden
Filename: "taskkill.exe"; Parameters: "/F /IM nxuseragent.exe /T"; StatusMsg: "Killing user agent processes..."; RunOnceId: "KillUserAgentProcess"; Flags: runhidden
Filename: "{app}\bin\nxagentd.exe"; Parameters: "-R"; StatusMsg: "Uninstalling service..."; RunOnceId: "DelService"; Flags: runhidden

[UninstallDelete]
Type: files; Name: "{app}\bin\nxreload.exe"
Type: filesandordirs; Name: "{app}\etc\*"
Type: filesandordirs; Name: "{app}\log\*"
Type: filesandordirs; Name: "{app}\var\*"

[Code]
Var
  ServerSelectionPage: TWizardPage;
  editServerName: TNewEdit;
  cbDownloadConfig, cbSetupTunnel: TNewCheckBox;
  SubagentSelectionPage: TInputOptionWizardPage;
  serverName, sbBind9, sbFileMgr, sbLogWatch, sbMQTT, sbNetSvc, sbPing, sbSSH,
    sbWinEventSync, sbWinPerf, sbWMI, sbUPS, sbDownloadConfig, sbSetupTunnel, extraConfigValues: String;
  backupFileSuffix: String;
  logFile: String;
  fileSTore: String;
  configIncludeDir: String;
  forceCreateConfig, reinstallService: Boolean;

#include "..\..\install\windows\firewall.iss"

Function ExecAndLog(execName, options, workingDir: String): Integer;
Var
  iResult : Integer;
Begin
  Log('Executing ' + execName + ' ' + options);
  Log('   Working directory: ' + workingDir);
  Exec(execName, options, workingDir, 0, ewWaitUntilTerminated, iResult);
  Log('   Result: ' + IntToStr(iResult));
  Result := iResult;
End;

Procedure StopService;
Var
  strExecName : String;
  iResult, iRetryCount : Integer;
Begin
  Log('Stopping agent service');
  FileCopy(ExpandConstant('{tmp}\nxreload.exe'), ExpandConstant('{app}\bin\nxreload.exe'), False);

  strExecName := ExpandConstant('{app}\bin\nxagentd.exe');
  If FileExists(strExecName) Then
  Begin
    ExecAndLog(strExecName, '-k', ExpandConstant('{app}\bin'));
    ExecAndLog(strExecName, '-S', ExpandConstant('{app}\bin'));
  End Else Begin
    ExecAndLog(ExpandConstant('{sys}\sc.exe'), 'stop NetXMSAgentdW32', ExpandConstant('{tmp}'));
  End;

  ExecAndLog('taskkill.exe', '/IM nxsagent.exe /F', ExpandConstant('{tmp}'));
  ExecAndLog('taskkill.exe', '/IM nxuseragent.exe /F', ExpandConstant('{tmp}'));

  iRetryCount := 12;
  iResult := ExecAndLog('taskkill.exe', '/IM nxagentd.exe /F', ExpandConstant('{tmp}'));
  While (iResult = 1) And (iRetryCount > 0) Do
  Begin
    Sleep(5000);
    iResult := ExecAndLog('taskkill.exe', '/IM nxagentd.exe /F', ExpandConstant('{tmp}'));
    iRetryCount := iRetryCount - 1;
  End;

  If reinstallService Then
  Begin
    If FileExists(strExecName) Then
    Begin
      ExecAndLog(strExecName, '-R', ExpandConstant('{app}\bin'));
    End Else Begin
      ExecAndLog(ExpandConstant('{sys}\sc.exe'), 'delete NetXMSAgentdW32', ExpandConstant('{tmp}'));
    End;
  End;
End;

Function PrepareToInstall(var NeedsRestart: Boolean): String;
Begin
  RegDeleteValue(HKEY_LOCAL_MACHINE, 'Software\NetXMS\Agent', 'ReloadFlag');
  ExecAndLog('taskkill.exe', '/IM nxreload.exe /F', ExpandConstant('{tmp}'));
  ExtractTemporaryFile('nxreload.exe');
  StopService;
  Result := '';
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

Procedure LoadPreviousData;
Begin
  serverName := GetPreviousData('MasterServer', serverName);
  sbDownloadConfig := GetPreviousData('DownloadConfig', sbDownloadConfig);
  sbSetupTunnel := GetPreviousData('SetupTunnel', sbSetupTunnel);
  sbBind9 := GetPreviousData('Subagent_Bind9', sbBind9);
  sbFileMgr := GetPreviousData('Subagent_FILEMGR', sbFileMgr);
  sbLogWatch := GetPreviousData('Subagent_LOGWATCH', sbLogWatch);
  sbMQTT := GetPreviousData('Subagent_MQTT', sbMQTT);
  sbNetSvc := GetPreviousData('Subagent_NETSVC', sbNetSvc);
  sbPing := GetPreviousData('Subagent_PING', sbPing);
  sbSSH := GetPreviousData('Subagent_SSH', sbSSH);
  sbUPS := GetPreviousData('Subagent_UPS', sbUPS);
  sbWinEventSync := GetPreviousData('Subagent_WINEVENTSYNC', sbWinEventSync);
  sbWinPerf := GetPreviousData('Subagent_WINPERF', sbWinPerf);
  sbWMI := GetPreviousData('Subagent_WMI', sbWMI);
End;

Function InitializeSetup(): Boolean;
Var
  i, nCount : Integer;
  param : String;
  ignorePrevData : Boolean;
Begin
  // Common suffix for backup files
  backupFileSuffix := '.bak.' + GetDateTimeString('yyyymmddhhnnss', #0, #0);

  // Check if we are running on 64-bit Windows
  If (NOT Is64BitInstallMode) AND (ProcessorArchitecture = paX64) Then Begin
    If MsgBox('You are trying to install 32-bit version of NetXMS agent on 64-bit Windows. It is recommended to install 64-bit version instead. Do you really wish to continue installation?', mbConfirmation, MB_YESNO) = IDYES Then
      Result := TRUE
    Else
      Result := FALSE;
  End Else Begin
    Result := TRUE;
  End;
  
  // Initial values for installation data
  serverName := '';
  sbBind9 := 'FALSE';
  sbFileMgr := 'FALSE';
  sbLogWatch := 'FALSE';
  sbMQTT := 'FALSE';
  sbNetSvc := 'FALSE';
  sbPing := 'FALSE';
  sbSSH := 'FALSE';
  sbWinEventSync := 'FALSE';
  sbWinPerf := 'TRUE';
  sbWMI := 'FALSE';
  sbUPS := 'FALSE';
  sbDownloadConfig := 'FALSE';
  sbSetupTunnel := 'FALSE';
  extraConfigValues := '';
  logFile := '';
  fileStore := '';
  configIncludeDir := '';
  forceCreateConfig := FALSE;
  reinstallService := FALSE;
  ignorePrevData := FALSE;

  // Check for /IGNOREPREVIOUSDATA option
  nCount := ParamCount;
  For i := 1 To nCount Do Begin
    param := ParamStr(i);
    If param = '/IGNOREPREVIOUSDATA' Then Begin
      ignorePrevData := TRUE;
    End;
  End;

  If Not ignorePrevData Then
    LoadPreviousData;
  
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

    If Pos('/TUNNEL', param) = 1 Then Begin
      sbSetupTunnel := 'TRUE';
    End;

    If Pos('/NOTUNNEL', param) = 1 Then Begin
      sbSetupTunnel := 'FALSE';
    End;

    If Pos('/SUBAGENT=', param) = 1 Then Begin
      Delete(param, 1, 10);
      param := Uppercase(param);
      If param = 'BIND9' Then
        sbBind9 := 'TRUE';
      If param = 'FILEMGR' Then
        sbFileMgr := 'TRUE';
      If param = 'LOGWATCH' Then
        sbLogWatch := 'TRUE';
      If param = 'MQTT' Then
        sbMQTT := 'TRUE';
      If param = 'NETSVC' Then
        sbNetSvc := 'TRUE';
      If param = 'PING' Then
        sbPing := 'TRUE';
      If param = 'SSH' Then
        sbSSH := 'TRUE';
      If param = 'WINEVENTSYNC' Then
        sbWinEventSync := 'TRUE';
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
      If param = 'BIND9' Then
        sbBind9 := 'FALSE';
      If param = 'FILEMGR' Then
        sbFileMgr := 'FALSE';
      If param = 'LOGWATCH' Then
        sbLogWatch := 'FALSE';
      If param = 'MQTT' Then
        sbMQTT := 'FALSE';
      If param = 'NETSVC' Then
        sbNetSvc := 'FALSE';
      If param = 'PING' Then
        sbPing := 'FALSE';
      If param = 'SSH' Then
        sbSSH := 'FALSE';
      If param = 'WINEVENTSYNC' Then
        sbWinEventSync := 'FALSE';
      If param = 'WINPERF' Then
        sbWinPerf := 'FALSE';
      If param = 'WMI' Then
        sbWMI := 'FALSE';
      If param = 'UPS' Then
        sbUPS := 'FALSE';
    End;

    If param = '/FORCECREATECONFIG' Then Begin
      forceCreateConfig := TRUE;
    End;

    If param = '/REINSTALLSERVICE' Then Begin
      reinstallService := TRUE;
    End;

    If Pos('/LOGFILE=', param) = 1 Then Begin
      Delete(param, 1, 9);
      logFile := param;
    End;

    If Pos('/FILESTORE=', param) = 1 Then Begin
      Delete(param, 1, 11);
      fileStore := param;
    End;

    If Pos('/CONFIGINCLUDEDIR=', param) = 1 Then Begin
      Delete(param, 1, 18);
      configIncludeDir := param;
    End;

    If Pos('/CONFIGENTRY=', param) = 1 Then Begin
      Delete(param, 1, 13);
      extraConfigValues := extraConfigValues + '~~' + param;
    End;
  End;
End;

Procedure InitializeWizard;
Var
  static: TNewStaticText;
Begin
  ServerSelectionPage := CreateCustomPage(wpSelectTasks, 'NetXMS Server', 'Select your management server.');

  static := TNewStaticText.Create(ServerSelectionPage);
  static.Top := ScaleY(8);
  static.Caption := 'Please enter host name or IP address of your NetXMS server.';
  static.AutoSize := True;
  static.Parent := ServerSelectionPage.Surface;

  editServerName := TNewEdit.Create(ServerSelectionPage);
  editServerName.Top := static.Top + static.Height + ScaleY(8);
  editServerName.Width := ServerSelectionPage.SurfaceWidth - ScaleX(8);
  editServerName.Text := serverName;
  editServerName.Parent := ServerSelectionPage.Surface;

  cbDownloadConfig := TNewCheckBox.Create(ServerSelectionPage);
  cbDownloadConfig.Top := editServerName.Top + editServerName.Height + ScaleY(12);
  cbDownloadConfig.Width := ServerSelectionPage.SurfaceWidth;
  cbDownloadConfig.Height := ScaleY(17);
  cbDownloadConfig.Caption := 'Download configuration file from management server on startup';
  cbDownloadConfig.Checked := StrToBool(sbDownloadConfig);
  cbDownloadConfig.Parent := ServerSelectionPage.Surface;

  cbSetupTunnel := TNewCheckBox.Create(ServerSelectionPage);
  cbSetupTunnel.Top := editServerName.Top + editServerName.Height + cbDownloadConfig.Height + ScaleY(24);
  cbSetupTunnel.Width := ServerSelectionPage.SurfaceWidth;
  cbSetupTunnel.Height := ScaleY(17);
  cbSetupTunnel.Caption := 'Setup tunnel to master server';
  cbSetupTunnel.Checked := StrToBool(sbSetupTunnel);
  cbSetupTunnel.Parent := ServerSelectionPage.Surface;

  SubagentSelectionPage := CreateInputOptionPage(ServerSelectionPage.Id,
    'Subagent Selection', 'Select desired subagents.',
    'Please select additional subagents you wish to load.', False, False);
  SubagentSelectionPage.Add('BIND Monitoring Subagent (bind9)');
  SubagentSelectionPage.Add('File Manager Subagent (filemgr)');
  SubagentSelectionPage.Add('ICMP Pinger Subagent (ping)');
  SubagentSelectionPage.Add('Log Monitoring Subagent (logwatch)');
  SubagentSelectionPage.Add('MQTT Subagent (mqtt)');
  SubagentSelectionPage.Add('Network Service Checker Subagent (netsvc)');
  SubagentSelectionPage.Add('SSH Subagent (ssh)');
  SubagentSelectionPage.Add('Windows Event Log Synchronization Subagent (wineventsync)');
  SubagentSelectionPage.Add('Windows Performance Subagent (winperf)');
  SubagentSelectionPage.Add('WMI Subagent (wmi)');
  SubagentSelectionPage.Add('UPS Monitoring Subagent (ups)');
  SubagentSelectionPage.Values[0] := StrToBool(sbBind9);
  SubagentSelectionPage.Values[1] := StrToBool(sbFileMgr);
  SubagentSelectionPage.Values[2] := StrToBool(sbPing);
  SubagentSelectionPage.Values[3] := StrToBool(sbLogWatch);
  SubagentSelectionPage.Values[4] := StrToBool(sbMQTT);
  SubagentSelectionPage.Values[5] := StrToBool(sbNetSvc);
  SubagentSelectionPage.Values[6] := StrToBool(sbSSH);
  SubagentSelectionPage.Values[7] := StrToBool(sbWinEventSync);
  SubagentSelectionPage.Values[8] := StrToBool(sbWinPerf);
  SubagentSelectionPage.Values[9] := StrToBool(sbWMI);
  SubagentSelectionPage.Values[10] := StrToBool(sbUPS);
End;

Function ShouldSkipPage(PageID: Integer): Boolean;
Begin
  If (PageID = ServerSelectionPage.ID) Or (PageID = SubagentSelectionPage.ID) Then
    Result := RegKeyExists(HKEY_LOCAL_MACHINE, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\NetXMS Agent_is1')
  Else
    Result := False;
End;

Procedure RegisterPreviousData(PreviousDataKey: Integer);
Begin
  SetPreviousData(PreviousDataKey, 'MasterServer', editServerName.Text);
  SetPreviousData(PreviousDataKey, 'DownloadConfig', BoolToStr(cbDownloadConfig.Checked));
  SetPreviousData(PreviousDataKey, 'SetupTunnel', BoolToStr(cbSetupTunnel.Checked));
  SetPreviousData(PreviousDataKey, 'Subagent_BIND9', BoolToStr(SubagentSelectionPage.Values[0]));
  SetPreviousData(PreviousDataKey, 'Subagent_FILEMGR', BoolToStr(SubagentSelectionPage.Values[1]));
  SetPreviousData(PreviousDataKey, 'Subagent_PING', BoolToStr(SubagentSelectionPage.Values[2]));
  SetPreviousData(PreviousDataKey, 'Subagent_LOGWATCH', BoolToStr(SubagentSelectionPage.Values[3]));
  SetPreviousData(PreviousDataKey, 'Subagent_MQTT', BoolToStr(SubagentSelectionPage.Values[4]));
  SetPreviousData(PreviousDataKey, 'Subagent_NETSVC', BoolToStr(SubagentSelectionPage.Values[5]));
  SetPreviousData(PreviousDataKey, 'Subagent_SSH', BoolToStr(SubagentSelectionPage.Values[6]));
  SetPreviousData(PreviousDataKey, 'Subagent_WINEVENTSYNC', BoolToStr(SubagentSelectionPage.Values[7]));
  SetPreviousData(PreviousDataKey, 'Subagent_WINPERF', BoolToStr(SubagentSelectionPage.Values[8]));
  SetPreviousData(PreviousDataKey, 'Subagent_WMI', BoolToStr(SubagentSelectionPage.Values[9]));
  SetPreviousData(PreviousDataKey, 'Subagent_UPS', BoolToStr(SubagentSelectionPage.Values[10]));
End;

Function GetMasterServer(Param: String): String;
Begin
  If cbSetupTunnel.Checked Then
    Result := '~' + editServerName.Text
  Else
    Result := editServerName.Text;
End;

Function GetSubagentList(Param: String): String;
Begin
  Result := '';
  If SubagentSelectionPage.Values[0] Then
    Result := Result + 'bind9 ';
  If SubagentSelectionPage.Values[1] Then
    Result := Result + 'filemgr ';
  If SubagentSelectionPage.Values[2] Then
    Result := Result + 'ping ';
  If SubagentSelectionPage.Values[3] Then
    Result := Result + 'logwatch ';
  If SubagentSelectionPage.Values[4] Then
    Result := Result + 'mqtt ';
  If SubagentSelectionPage.Values[5] Then
    Result := Result + 'netsvc ';
  If SubagentSelectionPage.Values[6] Then
    Result := Result + 'ssh ';
  If SubagentSelectionPage.Values[7] Then
    Result := Result + 'wineventsync ';
  If SubagentSelectionPage.Values[8] Then
    Result := Result + 'winperf ';
  If SubagentSelectionPage.Values[9] Then
    Result := Result + 'wmi ';
  If SubagentSelectionPage.Values[10] Then
    Result := Result + 'ups ';
End;

Function GetExtraConfigValues(Param: String): String;
Begin
  If extraConfigValues <> '' Then
    Result := '-z "' + extraConfigValues + '"'
  Else
    Result := ''
End;

Function GetLogFile(Param: String): String;
Begin
  If logFile <> '' Then
    Result := logFile
  Else
    Result := '{syslog}'
End;

Function GetConfigIncludeDir(Param: String): String;
Begin
  If configIncludeDir <> '' Then
    Result := configIncludeDir
  Else
    Result := ExpandConstant('{app}\etc\nxagentd.conf.d')
End;

Function GetFileStore(Param: String): String;
Begin
  If fileStore <> '' Then
    Result := fileStore
  Else
    Result := ExpandConstant('{app}\var')
End;

Function GetForceCreateConfigFlag(Param: String): String;
Begin
  If forceCreateConfig Then
    Result := '-F'
  Else
    Result := ''
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

Procedure RenameOldFile;
Begin
  RenameFile(ExpandConstant(CurrentFileName), ExpandConstant(CurrentFileName) + backupFileSuffix);
End;

Procedure DeleteBackupFiles;
Var
  fd: TFindRec;
  basePath: String;
Begin
  basePath := ExpandConstant('{app}\bin\');
  If FindFirst(basePath + '*.bak.*', fd) Then
  Begin
    Try
      Repeat
        DeleteFile(basePath + fd.Name);
      Until Not FindNext(fd);
    Finally
      FindClose(fd);
    End;
  End;
End;

Procedure CurStepChanged(CurStep: TSetupStep);
Begin
  If CurStep = ssPostInstall Then Begin
    SetFirewallException('NetXMS Agent', ExpandConstant('{app}')+'\bin\nxagentd.exe');
    SetFirewallException('NetXMS TFTP Client', ExpandConstant('{app}')+'\bin\nxtftp.exe');
    DeleteBackupFiles;
  End;
End;

Procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
Begin
  If CurUninstallStep = usPostUninstall Then Begin
     RemoveFirewallException(ExpandConstant('{app}')+'\bin\nxagentd.exe');
     RemoveFirewallException(ExpandConstant('{app}')+'\bin\nxtftp.exe');
  End;
End;
