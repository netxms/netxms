;/****************************************************************************
; Messages for NetXMS NetFlow/IPFIX Collector (NXFLOWD)
;****************************************************************************/
;
;#ifndef _messages_h_
;#define _messages_h_
;

MessageIdTypedef=DWORD

MessageId=1
SymbolicName=MSG_NXFLOWD_STARTED
Language=English
NetXMS NetFlow/IPFIX Collector started
.

MessageId=
SymbolicName=MSG_WSASTARTUP_FAILED
Language=English
WSAStartup failed: %1
.

MessageId=
SymbolicName=MSG_IPFIX_INIT_FAILED
Language=English
IPFIX library initialization failed
.

MessageId=
SymbolicName=MSG_IPFIX_REGISTER_FAILED
Language=English
IPFIX collector registration failed
.

MessageId=
SymbolicName=MSG_IPFIX_LISTEN_FAILED
Language=English
Unable to start IPFIX listener for %1 protocol
.

MessageId=
SymbolicName=MSG_COLLECTOR_STARTED
Language=English
Collector thread started
.

MessageId=
SymbolicName=MSG_COLLECTOR_STOPPED
Language=English
Collector thread stopped
.

MessageId=
SymbolicName=MSG_POLLING_ERROR
Language=English
IPFIX polling error
.

MessageId=
SymbolicName=MSG_DEBUG
Language=English
%1
.

MessageId=
SymbolicName=MSG_OTHER
Language=English
%1
.

MessageId=
SymbolicName=MSG_DB_CONNFAIL
Language=English
Unable to establish connection with database (%1)
.

MessageId=
SymbolicName=MSG_SQL_ERROR
Language=English
SQL query failed (Query = "%1"): %2
.

MessageId=
SymbolicName=MSG_DEBUG_TAG
Language=English
[%1] %2
.

;#endif
