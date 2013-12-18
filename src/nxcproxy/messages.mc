;/****************************************************************************
; Messages for NetXMS Client Proxy
;****************************************************************************/
;
;#ifndef _messages_h_
;#define _messages_h_
;

MessageIdTypedef=DWORD

MessageId=1
SymbolicName=MSG_PROXY_STARTED
Language=English
NetXMS Client Proxy started
.

MessageId=
SymbolicName=MSG_PROXY_STOPPED
Language=English
NetXMS Client Proxy stopped
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
SymbolicName=MSG_DEBUG
Language=English
%1
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
SymbolicName=MSG_LISTENING
Language=English
Listening on socket %1:%2
.

MessageId=
SymbolicName=MSG_EXCEPTION
Language=English
EXCEPTION %1 (%2) at %3 (crash dump was generated); please send files %4 and %5 to dump@netxms.org
.

MessageId=
SymbolicName=MSG_DEBUG_LEVEL
Language=English
Debug level set to %1
.

MessageId=
SymbolicName=MSG_SELECT_ERROR
Language=English
Call to select() failed: %1
.

;#endif
