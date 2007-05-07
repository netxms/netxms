;/****************************************************************************
; Messages for NetXMS Web Interface Server (NXHTTPD)
;****************************************************************************/
;
;#ifndef _messages_h_
;#define _messages_h_
;

MessageIdTypedef=DWORD

MessageId=1
SymbolicName=MSG_NXHTTPD_STARTED
Language=English
NetXMS Web Interface Server started
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
SymbolicName=MSG_SELECT_ERROR
Language=English
Call to select() failed: %1
.

MessageId=
SymbolicName=MSG_DEBUG
Language=English
Debug: %1
.

MessageId=
SymbolicName=MSG_DEBUG_SESSION
Language=English
Debug: {%1} %2
.

MessageId=
SymbolicName=MSG_NXCL_INIT_FAILED
Language=English
NetXMS Client Library initialization failed
.

;#endif
