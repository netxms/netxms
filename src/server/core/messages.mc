;/****************************************************************************
; Messages for NMS Core
;****************************************************************************/
;
;#ifndef _messages_h_
;#define _messages_h_
;

MessageIdTypedef=DWORD

MessageId=1
SymbolicName=MSG_SERVER_STARTED
Language=English
NMS Server started
.

MessageId=
SymbolicName=MSG_SERVER_STOPPED
Language=English
NMS Server stopped
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
SymbolicName=MSG_DBDRV_NO_ENTRY_POINTS
Language=English
Unable to find all required exportable functions in database driver "%1"
.

MessageId=
SymbolicName=MSG_DBDRV_INIT_FAILED
Language=English
Database driver "%1" initialization failed
.

MessageId=
SymbolicName=MSG_DBDRV_LOADED
Language=English
Database driver "%1" loaded and initialized successfully
.

MessageId=
SymbolicName=MSG_DB_CONNFAIL
Language=English
Unable to establish connection with database
.

MessageId=
SymbolicName=MSG_NODE_EXIST
Language=English
Attempt to add existing node %1 to database
.

MessageId=
SymbolicName=MSG_OBJECT_ID_EXIST
Language=English
Attempt to add node with non-unique ID %1 to database
.

MessageId=
SymbolicName=MSG_OBJECT_ADDR_EXIST
Language=English
Attempt to add node with non-unique IP address %1 to database
.

MessageId=
SymbolicName=MSG_BAD_NETOBJ_TYPE
Language=English
Internal error: invalid NetObj.Type() value %1
.

MessageId=
SymbolicName=MSG_RAW_SOCK_FAILED
Language=English
Unable to create raw socket for ICMP protocol
.

MessageId=
SymbolicName=MSG_DUPLICATE_KEY
Language=English
Attempt to add object with non-unique key %1 to index
.

;#endif
