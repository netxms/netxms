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
SymbolicName=MSG_AGENT_SHUTDOWN
Language=English
NetXMS Agent stopped
.

MessageId=
SymbolicName=MSG_DLSYM_FAILED
Language=English
Unable to resolve symbol "%1": %2
.

MessageId=
SymbolicName=MSG_DLOPEN_FAILED
Language=English
Unable to load module "%1": %2
.

MessageId=
SymbolicName=MSG_SUBAGENT_LOADED
Language=English
Subagent "%1" loaded successfully
.

MessageId=
SymbolicName=MSG_SUBAGENT_LOAD_FAILED
Language=English
Error loading subagent module "%1"
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

;#endif
