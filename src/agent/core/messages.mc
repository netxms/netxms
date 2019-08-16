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

MessageId=2
SymbolicName=MSG_AGENT_STOPPED
Language=English
NetXMS Agent stopped
.

MessageId=60
SymbolicName=MSG_TUNNEL_ESTABLISHED
Language=English
Tunnel with %1 established
.

MessageId=61
SymbolicName=MSG_TUNNEL_CLOSED
Language=English
Tunnel with %1 closed
.

MessageId=1000
SymbolicName=MSG_GENERIC
Language=English
%1
.

;#endif
