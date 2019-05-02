[Icons]
Name: "{group}\Collect NetXMS diagnostic information"; Filename: "{app}\bin\nx-collect-server-diag.cmd"; Components: server
Name: "{group}\NetXMS Console"; Filename: "{app}\bin\nxmc.exe"; Components: console
Name: "{group}\Recompile MIB Files"; Filename: "{app}\bin\nxmibc.exe"; Parameters: "-P -z -d ""{app}\var\mibs"" -o ""{app}\var\mibs\netxms.mib"""; Components: server
Name: "{group}\Server Configuration Wizard"; Filename: "{app}\bin\nxconfig.exe"; Components: server
Name: "{group}\Server Console"; Filename: "{app}\bin\nxadm.exe"; Parameters: "-i"; Components: server
Name: "{group}\{cm:UninstallProgram,NetXMS}"; Filename: "{uninstallexe}"
Name: "{userdesktop}\NetXMS Console"; Filename: "{app}\bin\nxmc.exe"; Tasks: desktopicon; Components: console
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\NetXMS Console"; Filename: "{app}\bin\nxmc.exe"; Tasks: quicklaunchicon; Components: console
