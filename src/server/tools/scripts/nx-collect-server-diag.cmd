@echo off

REM Create folder
FOR /F "delims=" %%a in ('powershell.exe Get-Date -format "yyyyMMdd-HHmmss"') DO SET timeanddate=%%a
set logdir=%TEMP%\netxms.info.%timeanddate%
mkdir "%logdir%"

set execdir=%~dp0
set errorlog="%logdir%\stderr.txt"

REM #################################
REM # OS level information:

set oslog=%logdir%\os.txt
set currentlog=%oslog%
echo.  >%oslog%

REM Host name, OS version, Memory usage, Uptime
CALL :print_header "System info"
systeminfo >> %oslog%

REM Process list
CALL :print_header "Process list"
tasklist /V >> %oslog%

REM Interfaces
CALL :print_header "Interfaces"
ipconfig /all >> %oslog%

REM IP routes
CALL :print_header "IP routes"
route PRINT >> %oslog%

REM ARP cache
CALL :print_header "ARP cache"
arp -a -v >> %oslog%

REM ####################################
REM # Output of nxadm commands:

set nxadmlog=%logdir%\nxadm.txt
set currentlog=%nxadmlog%
echo. >%nxadmlog%

REM debug
CALL :print_header "debug"
%execdir%nxadm -c "debug"  >> %nxadmlog%
REM show dbcp
CALL :print_header "show dbcp:"  
%execdir%nxadm -c "show dbcp"  >> %nxadmlog%
REM show dbstats
CALL :print_header "show dbstats:"  
%execdir%nxadm -c "show dbstats"  >> %nxadmlog%
REM show flags
CALL :print_header "show flags:" 
%execdir%nxadm -c "show flags"  >> %nxadmlog%
REM show heap details
CALL :print_header "show heap details:"  
%execdir%nxadm -c "show heap details"  >> %nxadmlog%
REM show heap summary
CALL :print_header "show heap summary:"  
%execdir%nxadm -c "show heap summary"  >> %nxadmlog%
REM show index id
CALL :print_header "show index id:"  
%execdir%nxadm -c "show index id"  >> %nxadmlog%
REM show index interface
CALL :print_header "show index interface:"  
%execdir%nxadm -c "show index interface"  >> %nxadmlog%
REM show index nodeaddr
CALL :print_header "show index nodeaddr:" 
%execdir%nxadm -c "show index nodeaddr"  >> %nxadmlog%
REM show index subnet
CALL :print_header "show index subnet:" 
%execdir%nxadm -c "show index subnet"  >> %nxadmlog%
REM show index zone
CALL :print_header "show index zone:"  
%execdir%nxadm -c "show index zone"  >> %nxadmlog%
REM show modules
CALL :print_header "show modules:" 
%execdir%nxadm -c "show modules"  >> %nxadmlog%
REM show msgwq
CALL :print_header "show msgwq:" 
%execdir%nxadm -c "show msgwq"  >> %nxadmlog%
REM show objects
CALL :print_header "show objects:"  
%execdir%nxadm -c "show objects"  >> %nxadmlog%
REM show pe
CALL :print_header "show pe:" 
%execdir%nxadm -c "show pe"  >> %nxadmlog%
REM show pollers
CALL :print_header "show pollers:" 
%execdir%nxadm -c "show pollers"  >> %nxadmlog%
REM show queues
CALL :print_header "show queues:" 
%execdir%nxadm -c "show queues"  >> %nxadmlog%
REM show sessions
CALL :print_header "show sessions:" 
%execdir%nxadm -c "show sessions"  >> %nxadmlog%
REM show stats
CALL :print_header "show stats:" 
%execdir%nxadm -c "show stats"  >> %nxadmlog%
REM show tunnels
CALL :print_header "show tunnels:" 
%execdir%nxadm -c "show tunnels"  >> %nxadmlog%
REM show users
CALL :print_header "show users:" 
%execdir%nxadm -c "show users"  >> %nxadmlog%
REM show watchdog
CALL :print_header "show watchdog:" 
%execdir%nxadm -c "show watchdog"  >> %nxadmlog%

REM netxmsd, nxagentd log files
For /f "tokens=1* delims=^" %%a in ('%execdir%netxmsd -l') do (set netxmslog=%%a)
ECHO netxmsd log: %netxmslog% >> %errorlog%
if /I NOT "{syslog}" EQU "%netxmslog%" (copy "%netxmslog%" "%logdir%\" >> %errorlog%) ELSE (echo Server log is Event Log >> %errorlog%)

For /f "tokens=1* delims=^" %%a in ('%execdir%nxagentd -l') do (set netxmslog=%%a)
ECHO nxagentd log: %netxmslog% >> %errorlog%
if /I NOT "{syslog}" EQU "%netxmslog%" (copy "%netxmslog%" "%logdir%\" >> %errorlog%) ELSE (echo Agent log is Event Log >> %errorlog%)

REM Zip DeleteFolder

powershell.exe -File "%execdir%zip.ps1" -source "%logdir%" -archive "%logdir%.zip"
echo Diagnostic file: %logdir%.zip
DEL /F /Q /S "%logdir%" >NUL:
RMDIR "%logdir%" >NUL:

PAUSE
EXIT /B 0 

:print_header
echo. >> %currentlog%
echo ********** %~1 ********** >> %currentlog%
echo. >> %currentlog%
EXIT /B 0
