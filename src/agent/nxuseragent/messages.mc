;/****************************************************************************
; Messages for NetXMS User Agent
;****************************************************************************/
;
;#ifndef _messages_h_
;#define _messages_h_
;

MessageIdTypedef=DWORD

MessageId=1
SymbolicName=MSG_USERAGENT_STARTED
Language=English
NetXMS user agent started
.

MessageId=
SymbolicName=MSG_USERAGENT_STOPPED
Language=English
NetXMS user agent stopped
.

MessageId=
SymbolicName=MSG_CONFIG_LOAD_FAILED
Language=English
Error loading user agent configuration
.

MessageId=
SymbolicName=MSG_INIT_FAILED
Language=English
NetXMS User Agent initialization failed
.

MessageId=
SymbolicName=MSG_GENERIC
Language=English
%1
.

MessageId=
SymbolicName=MSG_DEBUG
Language=English
%1
.

MessageId=
SymbolicName=MSG_DEBUG_TAG
Language=English
[%1] %2
.

MessageId=
SymbolicName=MSG_EXCEPTION
Language=English
EXCEPTION %1 (%2) at %3 (crash dump was generated); please send files %4 and %5 to dump@netxms.org
.

;#endif
