;/****************************************************************************
; Messages for NetXMS Core Agent
;****************************************************************************/
;
;#ifndef _messages_h_
;#define _messages_h_
;

MessageIdTypedef=DWORD

MessageId=1
SymbolicName=MSG_AGENT_STARTED
Language=English
NetXMS Agent started
.

MessageId=
SymbolicName=MSG_AGENT_STOPPED
Language=English
NetXMS Agent stopped
.

MessageId=
SymbolicName=MSG_SUBAGENT_LOADED
Language=English
Subagent "%1" loaded successfully
.

MessageId=
SymbolicName=MSG_SUBAGENT_LOAD_FAILED
Language=English
Error loading subagent module "%1": %2
.

MessageId=
SymbolicName=MSG_NO_SUBAGENT_ENTRY_POINT
Language=English
Unable to find entry point in subagent module "%1"
.

MessageId=
SymbolicName=MSG_SUBAGENT_INIT_FAILED
Language=English
Initialization of subagent "%1" failed
.

MessageId=
SymbolicName=MSG_UNEXPECTED_IRC
Language=English
Internal error: unexpected iRC=%1 in ProcessCommand("%2")
.

MessageId=
SymbolicName=MSG_SOCKET_ERROR
Language=English
Unable to open socket: %1
.

MessageId=
SymbolicName=MSG_BIND_ERROR
Language=English
Unable to bind socket: %1
.

MessageId=
SymbolicName=MSG_ACCEPT_ERROR
Language=English
Unable to accept incoming connection: %1
.

MessageId=
SymbolicName=MSG_TOO_MANY_ERRORS
Language=English
Too many consecutive errors on accept() call
.

MessageId=
SymbolicName=MSG_WSASTARTUP_FAILED
Language=English
WSAStartup failed: %1
.

MessageId=
SymbolicName=MSG_TOO_MANY_SESSIONS
Language=English
Too many communication sessions open - unable to accept new connection
.

MessageId=
SymbolicName=MSG_SESSION_BROKEN
Language=English
Communication session broken: %1
.

MessageId=
SymbolicName=MSG_DEBUG
Language=English
%1
.

MessageId=
SymbolicName=MSG_AUTH_FAILED
Language=English
Authentication failed for peer %1, method: %2
.

MessageId=
SymbolicName=MSG_UNEXPECTED_ATTRIBUTE
Language=English
Internal error: unexpected process attribute code %1 in GetProcessAttribute()
.

MessageId=
SymbolicName=MSG_UNEXPECTED_TYPE
Language=English
Internal error: unexpected type code %1 in GetProcessAttribute()
.

MessageId=
SymbolicName=MSG_NO_FUNCTION
Language=English
Unable to resolve symbol "%1"
.

MessageId=
SymbolicName=MSG_NO_DLL
Language=English
Unable to get handle to "%1"
.

MessageId=
SymbolicName=MSG_ADD_ACTION_FAILED
Language=English
Unable to add action "%1"
.

MessageId=
SymbolicName=MSG_CREATE_PROCESS_FAILED
Language=English
Unable to create process "%1": %2
.

MessageId=
SymbolicName=MSG_SUBAGENT_MSG
Language=English
%1
.

MessageId=
SymbolicName=MSG_SUBAGENT_BAD_MAGIC
Language=English
Subagent "%1" has invalid magic number - probably it was compiled for different agent version
.

MessageId=
SymbolicName=MSG_ADD_EXT_PARAM_FAILED
Language=English
Unable to add external parameter "%1"
.

MessageId=
SymbolicName=MSG_CREATE_TMP_FILE_FAILED
Language=English
Unable to create temporary file to hold process output: %1
.

MessageId=
SymbolicName=MSG_SIGNAL_RECEIVED
Language=English
Signal %1 received
.

MessageId=
SymbolicName=MSG_INIT_CRYPTO_FAILED
Language=English
Failed to initialize cryptografy module
.

MessageId=
SymbolicName=MSG_DECRYPTION_FAILURE
Language=English
Failed to decrypt received message
.

MessageId=
SymbolicName=MSG_GETVERSION_FAILED
Language=English
Unable to deterime OS version (GetVersionEx() failed: %1)
.

MessageId=
SymbolicName=MSG_SELECT_ERROR
Language=English
Call to select() failed: %1
.

MessageId=
SymbolicName=MSG_SUBAGENT_ALREADY_LOADED
Language=English
Subagent "%1" already loaded from module "%2"
.

MessageId=
SymbolicName=MSG_PROCESS_KILLED
Language=English
Process "%1" killed because of execution timeout
.

MessageId=
SymbolicName=MSG_DEBUG_SESSION
Language=English
[session:%1] %2
.

MessageId=
SymbolicName=MSG_SUBAGENT_REGISTRATION_FAILED
Language=English
Registration of subagent "%1" failed
.

MessageId=
SymbolicName=MSG_LISTENING
Language=English
Listening on socket %1:%2
.

MessageId=
SymbolicName=MSG_WAITFORPROCESS_FAILED
Language=English
Call to WaitForProcess failed for process %1
.

MessageId=
SymbolicName=MSG_REGISTRATION_FAILED
Language=English
Registration on management server failed: %1
.

MessageId=
SymbolicName=MSG_REGISTRATION_SUCCESSFULL
Language=English
Successfully registered on management server %1
.

MessageId=
SymbolicName=MSG_WATCHDOG_STARTED
Language=English
Watchdog process started
.

MessageId=
SymbolicName=MSG_WATCHDOG_STOPPED
Language=English
Watchdog process stopped
.

MessageId=
SymbolicName=MSG_EXCEPTION
Language=English
EXCEPTION %1 (%2) at %3 (crash dump was generated); please send files %4 and %5 to dump@netxms.org
.

MessageId=
SymbolicName=MSG_USE_CONFIG_D
Language=English
Additional configs was loaded from %1
.

MessageId=
SymbolicName=MSG_SQL_ERROR
Language=English
SQL query failed (Query = "%1"): %2
.

MessageId=
SymbolicName=MSG_DB_LIBRARY
Language=English
DB Library: %1
.

MessageId=
SymbolicName=MSG_DEBUG_LEVEL
Language=English
Debug level set to %1
.

MessageId=
SymbolicName=MSG_REGISTRY_LOAD_FAILED
Language=English
Failed to load agent's registry from file %1
.

MessageId=
SymbolicName=MSG_REGISTRY_SAVE_FAILED
Language=English
Failed to save agent's registry to file %1: %2
.

MessageId=
SymbolicName=MSG_ADD_PARAM_PROVIDER_FAILED
Language=English
Unable to add external parameters provider "%1"
.

MessageId=
SymbolicName=MSG_ADD_EXTERNAL_SUBAGENT_FAILED
Language=English
Unable to add external subagent "%1"
.

MessageId=
SymbolicName=MSG_LOCAL_DB_OPEN_FAILED
Language=English
Unable to open local database
.

MessageId=
SymbolicName=MSG_LOCAL_DB_CORRUPTED
Language=English
Local database corrupted and cannot be used
.

;#endif
