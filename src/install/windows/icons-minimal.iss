[Icons]
Name: "{group}\Recompile MIB Files"; Filename: "{app}\bin\nxmibc.exe"; Parameters: "-P -z -d ""{app}\var\mibs"" -o ""{app}\var\mibs\netxms.mib"""; Components: server
Name: "{group}\Server Configuration Wizard"; Filename: "{app}\bin\nxconfig.exe"; Components: server
Name: "{group}\Server Console"; Filename: "{app}\bin\nxadm.exe"; Parameters: "-i"; Components: server
Name: "{group}\{cm:UninstallProgram,NetXMS}"; Filename: "{uninstallexe}"

