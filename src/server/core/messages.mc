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

MessageId=
SymbolicName=MSG_SUBNET_LOAD_FAILED
Language=English
Failed to load subnet object with id %1 from database
.

MessageId=
SymbolicName=MSG_NODE_LOAD_FAILED
Language=English
Failed to load node object with id %1 from database
.

MessageId=
SymbolicName=MSG_INTERFACE_LOAD_FAILED
Language=English
Failed to load interface object with id %1 from database
.

MessageId=
SymbolicName=MSG_INVALID_SUBNET_ID
Language=English
Inconsistent database: node %1 linked to non-existing subnet %2
.

MessageId=
SymbolicName=MSG_SUBNET_NOT_SUBNET
Language=English
Inconsistent database: node %1 linked to object %2 which is not a subnet
.

MessageId=
SymbolicName=MSG_INVALID_NODE_ID
Language=English
Inconsistent database: interface %1 linked to non-existing node %2
.

MessageId=
SymbolicName=MSG_NODE_NOT_NODE
Language=English
Inconsistent database: interface %1 linked to object %2 which is not a node
.

MessageId=
SymbolicName=MSG_SNMP_UNKNOWN_TYPE
Language=English
Unknown SNMP variable type %1 in GET responce PDU
.

MessageId=
SymbolicName=MSG_SNMP_GET_ERROR
Language=English
Error %1 in processing SNMP GET request
.

MessageId=
SymbolicName=MSG_SNMP_BAD_PACKET
Language=English
Error in SNMP responce packet: %1
.

MessageId=
SymbolicName=MSG_OID_PARSE_ERROR
Language=English
Error parsing SNMP OID '%1'
.

MessageId=
SymbolicName=MSG_SNMP_OPEN_FAILED
Language=English
Unable to open SNMP session
.

MessageId=
SymbolicName=MSG_SOCKET_FAILED
Language=English
Unable to create socket in function %1
.

MessageId=
SymbolicName=MSG_AGENT_CONNECT_FAILED
Language=English
Unable to establish connection with agent on host %1
.

MessageId=
SymbolicName=MSG_AGENT_COMM_FAILED
Language=English
Unable to communicate with agent on host %1 (possibly due to authentication failure)
.

MessageId=
SymbolicName=MSG_AGENT_BAD_HELLO
Language=English
Invalid HELLO message received from agent on host %1 (possibly due to incompatible protocol version)
.

MessageId=
SymbolicName=MSG_GETIPADDRTABLE_FAILED
Language=English
Call to GetIpAddrTable() failed: %1
.

MessageId=
SymbolicName=MSG_GETIPNETTABLE_FAILED
Language=English
Call to GetIpNetTable() failed: %1
.

MessageId=
SymbolicName=MSG_EVENT_LOAD_ERROR
Language=English
Unable to load events from database
.

MessageId=
SymbolicName=MSG_THREAD_HANGS
Language=English
Thread "%1" does not respond to watchdog thread
.

;#endif
