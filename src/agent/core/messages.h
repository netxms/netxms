/****************************************************************************
 Messages for NetXMS Core Agent
****************************************************************************/

#ifndef _messages_h_
#define _messages_h_

//
// MessageId: MSG_AGENT_STARTED
//
// MessageText:
//
// NetXMS Agent started
//
#define MSG_AGENT_STARTED              ((DWORD)0x00000001)

//
// MessageId: MSG_AGENT_SHUTDOWN
//
// MessageText:
//
// NetXMS Agent stopped
//
#define MSG_AGENT_SHUTDOWN             ((DWORD)0x00000002)

//
// MessageId: MSG_SUBAGENT_LOADED
//
// MessageText:
//
// Subagent "%1" loaded successfully
//
#define MSG_SUBAGENT_LOADED            ((DWORD)0x00000003)

//
// MessageId: MSG_SUBAGENT_LOAD_FAILED
//
// MessageText:
//
// Error loading subagent module "%1": %2
//
#define MSG_SUBAGENT_LOAD_FAILED       ((DWORD)0x00000004)

//
// MessageId: MSG_NO_SUBAGENT_ENTRY_POINT
//
// MessageText:
//
// Unable to find entry point in subagent module "%1"
//
#define MSG_NO_SUBAGENT_ENTRY_POINT    ((DWORD)0x00000005)

//
// MessageId: MSG_SUBAGENT_INIT_FAILED
//
// MessageText:
//
// Initialization of subagent "%1" failed
//
#define MSG_SUBAGENT_INIT_FAILED       ((DWORD)0x00000006)

//
// MessageId: MSG_UNEXPECTED_IRC
//
// MessageText:
//
// Internal error: unexpected iRC=%1 in ProcessCommand("%2")
//
#define MSG_UNEXPECTED_IRC             ((DWORD)0x00000007)

//
// MessageId: MSG_SOCKET_ERROR
//
// MessageText:
//
// Unable to open socket: %1
//
#define MSG_SOCKET_ERROR               ((DWORD)0x00000008)

//
// MessageId: MSG_BIND_ERROR
//
// MessageText:
//
// Unable to bind socket: %1
//
#define MSG_BIND_ERROR                 ((DWORD)0x00000009)

//
// MessageId: MSG_ACCEPT_ERROR
//
// MessageText:
//
// Unable to accept incoming connection: %1
//
#define MSG_ACCEPT_ERROR               ((DWORD)0x0000000a)

//
// MessageId: MSG_TOO_MANY_ERRORS
//
// MessageText:
//
// Too many consecutive errors on accept() call
//
#define MSG_TOO_MANY_ERRORS            ((DWORD)0x0000000b)

//
// MessageId: MSG_WSASTARTUP_FAILED
//
// MessageText:
//
// WSAStartup failed: %1
//
#define MSG_WSASTARTUP_FAILED          ((DWORD)0x0000000c)

//
// MessageId: MSG_TOO_MANY_SESSIONS
//
// MessageText:
//
// Too many communication sessions open - unable to accept new connection
//
#define MSG_TOO_MANY_SESSIONS          ((DWORD)0x0000000d)

//
// MessageId: MSG_SESSION_BROKEN
//
// MessageText:
//
// Communication session broken: %1
//
#define MSG_SESSION_BROKEN             ((DWORD)0x0000000e)

//
// MessageId: MSG_DEBUG
//
// MessageText:
//
// Debug: %1
//
#define MSG_DEBUG                      ((DWORD)0x0000000f)

//
// MessageId: MSG_AUTH_FAILED
//
// MessageText:
//
// Authentication failed for peer %1, method: %2
//
#define MSG_AUTH_FAILED                ((DWORD)0x00000010)

//
// MessageId: MSG_UNEXPECTED_ATTRIBUTE
//
// MessageText:
//
// Internal error: unexpected process attribute code %1 in GetProcessAttribute()
//
#define MSG_UNEXPECTED_ATTRIBUTE       ((DWORD)0x00000011)

//
// MessageId: MSG_UNEXPECTED_TYPE
//
// MessageText:
//
// Internal error: unexpected type code %1 in GetProcessAttribute()
//
#define MSG_UNEXPECTED_TYPE            ((DWORD)0x00000012)

//
// MessageId: MSG_NO_FUNCTION
//
// MessageText:
//
// Unable to resolve symbol "%1"
//
#define MSG_NO_FUNCTION                ((DWORD)0x00000013)

//
// MessageId: MSG_NO_DLL
//
// MessageText:
//
// Unable to get handle to "%1"
//
#define MSG_NO_DLL                     ((DWORD)0x00000014)

//
// MessageId: MSG_ADD_ACTION_FAILED
//
// MessageText:
//
// Unable to add action "%1"
//
#define MSG_ADD_ACTION_FAILED          ((DWORD)0x00000015)

//
// MessageId: MSG_CREATE_PROCESS_FAILED
//
// MessageText:
//
// Unable to create process "%1": %2
//
#define MSG_CREATE_PROCESS_FAILED      ((DWORD)0x00000016)

//
// MessageId: MSG_SUBAGENT_MSG
//
// MessageText:
//
// %1
//
#define MSG_SUBAGENT_MSG               ((DWORD)0x00000017)

//
// MessageId: MSG_SUBAGENT_BAD_MAGIC
//
// MessageText:
//
// Subagent "%1" has invalid magic number - probably it was compiled for different agent version
//
#define MSG_SUBAGENT_BAD_MAGIC         ((DWORD)0x00000018)

//
// MessageId: MSG_ADD_EXT_PARAM_FAILED
//
// MessageText:
//
// Unable to add external parameter "%1"
//
#define MSG_ADD_EXT_PARAM_FAILED       ((DWORD)0x00000019)

//
// MessageId: MSG_CREATE_TMP_FILE_FAILED
//
// MessageText:
//
// Unable to create temporary file to hold process output: %1
//
#define MSG_CREATE_TMP_FILE_FAILED     ((DWORD)0x0000001a)

//
// MessageId: MSG_SIGNAL_RECEIVED
//
// MessageText:
//
// Signal %1 received
//
#define MSG_SIGNAL_RECEIVED            ((DWORD)0x0000001b)

//
// MessageId: MSG_INIT_CRYPTO_FAILED
//
// MessageText:
//
// Failed to initialize cryptografy module
//
#define MSG_INIT_CRYPTO_FAILED         ((DWORD)0x0000001c)

//
// MessageId: MSG_DECRYPTION_FAILURE
//
// MessageText:
//
// Failed to decrypt received message
//
#define MSG_DECRYPTION_FAILURE         ((DWORD)0x0000001d)

//
// MessageId: MSG_GETVERSION_FAILED
//
// MessageText:
//
// Unable to deterime OS version (GetVersionEx() failed: %1)
//
#define MSG_GETVERSION_FAILED          ((DWORD)0x0000001e)

//
// MessageId: MSG_SELECT_ERROR
//
// MessageText:
//
// Call to select() failed: %1
//
#define MSG_SELECT_ERROR               ((DWORD)0x0000001f)

//
// MessageId: MSG_SUBAGENT_ALREADY_LOADED
//
// MessageText:
//
// Subagent "%1" already loaded from module "%2"
//
#define MSG_SUBAGENT_ALREADY_LOADED    ((DWORD)0x00000020)

//
// MessageId: MSG_PROCESS_KILLED
//
// MessageText:
//
// Process "%1" killed because of execution timeout
//
#define MSG_PROCESS_KILLED             ((DWORD)0x00000021)

//
// MessageId: MSG_DEBUG_SESSION
//
// MessageText:
//
// Debug: {%1} %2
//
#define MSG_DEBUG_SESSION              ((DWORD)0x00000022)

#endif
