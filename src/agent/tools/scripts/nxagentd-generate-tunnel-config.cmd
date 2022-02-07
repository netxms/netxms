@echo off

IF "%1"=="" GOTO HELP
IF "%1"=="-h" GOTO HELP
IF "%1"=="--help" GOTO HELP

SET _HOST=%1
SET _PORT=4703
IF NOT "%2"=="" ( SET _PORT=%2 )

echo Trying to connect to %_HOST%:%_PORT%... 1>&2

SET _FINGERPRINT=
FOR /F "tokens=2 delims==" %%s IN ('echo ^| openssl s_client -showcerts -connect %_HOST%:%_PORT% 2^>NUL ^| openssl x509 -noout -fingerprint -sha256 2^>NUL ^| find "Fingerprint="') DO (
   SET _FINGERPRINT=%%s
)

IF "%_FINGERPRINT%"=="" (
   echo.
   echo Cannot get certificate fingerprint from %_HOST%:%_PORT%.
   echo Make sure that NetXMS is running, configured correctly, and accept connections.
   echo.
   echo Certificate pinning is disabled.
   SET _FINGERPRINT_PREFIX=#
   SET _FINGERPRINT= # disabled!
) ELSE (
   echo.
   echo Got server's certificate fingerprint.
)

echo.
echo Add following section (or update existing) to agent's config (usually C:\NetXMS\etc\nxagentd.conf) and restart to apply:
echo.
echo [ServerConnection/%_HOST%]
echo Hostname=%_HOST%
echo Port=%_PORT%
echo %_FINGERPRINT_PREFIX%ServerCertificateFingerprint=%_FINGERPRINT%
echo.
echo ## If agent's certificate is externally provisioned, set certifiate location
echo ## Both certificate and private key should be in the same file, in PEM format (certificate first)
echo # Certificate=/path/to/agent.pem # path to certificate + key
echo # Password= # If private key is encrypted, optional

GOTO END

::
:: Display help message
::
:HELP
echo This helper script generates tunnel configuration section for the agent.
echo It attempts to connect to the netxms instance and get fingerprint of the server certificate.
echo If successfull - this certificate will be pinned to prevent MITM attacks.
echo.
echo Usage examples: 
echo    1. Connect to nx1.example.org port 4703:
echo       nxagentd-generate-tunnel-config.cmd nx1.example.org
echo.
echo    2. Connect to nx1.example.org port 5814:
echo       nxagentd-generate-tunnel-config.cmd nx1.example.org 5814

::
:: Cleanup
::
:END
SET _HOST=
SET _PORT=
SET _FINGERPRINT=
