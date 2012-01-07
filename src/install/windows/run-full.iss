[Run]
Filename: "msiexec.exe"; Parameters: "/i {app}\var\sqlncli.msi"; WorkingDir: "{app}\bin"; StatusMsg: "Installing Microsoft SQL Native Client..."; Flags: waituntilterminated; Components: server\mssql
Filename: "{app}\bin\nxmc.exe"; Parameters: "-clean -initialize"; WorkingDir: "{app}\bin"; StatusMsg: "Initializing management console..."; Flags: waituntilterminated; Components: console
