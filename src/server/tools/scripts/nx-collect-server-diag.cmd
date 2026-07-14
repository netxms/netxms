@echo off

REM Parse command line arguments for nxadm credentials
set nxadm_opts=

:parse_args
if "%~1"=="" goto done_args
if /I "%~1"=="-u" (
   set nxadm_opts=%nxadm_opts% -u %~2
   shift
   shift
   goto parse_args
)
if /I "%~1"=="-p" (
   set nxadm_opts=%nxadm_opts% -p %~2
   shift
   shift
   goto parse_args
)
if /I "%~1"=="-P" (
   set nxadm_prompt_password=1
   shift
   goto parse_args
)
shift
goto parse_args
:done_args

if defined nxadm_prompt_password (
   set /p nxadm_pw="Password: "
   set nxadm_opts=%nxadm_opts% -p %nxadm_pw%
)

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
%execdir%nxadm %nxadm_opts% -c "debug"  >> %nxadmlog%
if %ERRORLEVEL% EQU 5 (
   echo ERROR: nxadm authentication failed. Use -u and -P options to provide credentials.
   DEL /F /Q /S "%logdir%" >NUL:
   RMDIR "%logdir%" >NUL:
   PAUSE
   EXIT /B 5
)
if %ERRORLEVEL% NEQ 0 (
   echo WARNING: cannot connect to server ^(nxadm exit code %ERRORLEVEL%^), server statistics will not be collected. Server may be down or shutting down.
   echo cannot connect to server ^(nxadm exit code %ERRORLEVEL%^) >> %nxadmlog%
   GOTO skip_nxadm
)
REM show dbcp
CALL :print_header "show dbcp:"  
%execdir%nxadm %nxadm_opts% -c "show dbcp"  >> %nxadmlog%
REM show dbstats
CALL :print_header "show dbstats:"  
%execdir%nxadm %nxadm_opts% -c "show dbstats"  >> %nxadmlog%
REM show flags
CALL :print_header "show flags:" 
%execdir%nxadm %nxadm_opts% -c "show flags"  >> %nxadmlog%
REM show heap details
CALL :print_header "show heap details:"  
%execdir%nxadm %nxadm_opts% -c "show heap details"  >> %nxadmlog%
REM show heap summary
CALL :print_header "show heap summary:"  
%execdir%nxadm %nxadm_opts% -c "show heap summary"  >> %nxadmlog%
REM show index id
CALL :print_header "show index id:"  
%execdir%nxadm %nxadm_opts% -c "show index id"  >> %nxadmlog%
REM show index interface
CALL :print_header "show index interface:"  
%execdir%nxadm %nxadm_opts% -c "show index interface"  >> %nxadmlog%
REM show index nodeaddr
CALL :print_header "show index nodeaddr:" 
%execdir%nxadm %nxadm_opts% -c "show index nodeaddr"  >> %nxadmlog%
REM show index subnet
CALL :print_header "show index subnet:" 
%execdir%nxadm %nxadm_opts% -c "show index subnet"  >> %nxadmlog%
REM show index zone
CALL :print_header "show index zone:"  
%execdir%nxadm %nxadm_opts% -c "show index zone"  >> %nxadmlog%
REM show modules
CALL :print_header "show modules:" 
%execdir%nxadm %nxadm_opts% -c "show modules"  >> %nxadmlog%
REM show msgwq
CALL :print_header "show msgwq:" 
%execdir%nxadm %nxadm_opts% -c "show msgwq"  >> %nxadmlog%
REM show objects
CALL :print_header "show objects:"  
%execdir%nxadm %nxadm_opts% -c "show objects"  >> %nxadmlog%
REM show pe
CALL :print_header "show pe:" 
%execdir%nxadm %nxadm_opts% -c "show pe"  >> %nxadmlog%
REM show pollers
CALL :print_header "show pollers:" 
%execdir%nxadm %nxadm_opts% -c "show pollers"  >> %nxadmlog%
REM show queues
CALL :print_header "show queues:" 
%execdir%nxadm %nxadm_opts% -c "show queues"  >> %nxadmlog%
REM show sessions
CALL :print_header "show sessions:" 
%execdir%nxadm %nxadm_opts% -c "show sessions"  >> %nxadmlog%
REM show stats
CALL :print_header "show stats:" 
%execdir%nxadm %nxadm_opts% -c "show stats"  >> %nxadmlog%
REM show tunnels
CALL :print_header "show tunnels:" 
%execdir%nxadm %nxadm_opts% -c "show tunnels"  >> %nxadmlog%
REM show users
CALL :print_header "show users:" 
%execdir%nxadm %nxadm_opts% -c "show users"  >> %nxadmlog%
REM show watchdog
CALL :print_header "show watchdog:" 
%execdir%nxadm %nxadm_opts% -c "show watchdog"  >> %nxadmlog%

:skip_nxadm

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
