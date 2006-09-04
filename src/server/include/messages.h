/****************************************************************************
 Messages for NetXMS Server
****************************************************************************/

#ifndef _messages_h_
#define _messages_h_

//
// MessageId: MSG_SERVER_STARTED
//
// MessageText:
//
// NetXMS Server started
//
#define MSG_SERVER_STARTED             ((DWORD)0x00000001)

//
// MessageId: MSG_SERVER_STOPPED
//
// MessageText:
//
// NetXMS Server stopped
//
#define MSG_SERVER_STOPPED             ((DWORD)0x00000002)

//
// MessageId: MSG_DLSYM_FAILED
//
// MessageText:
//
// Unable to resolve symbol "%1": %2
//
#define MSG_DLSYM_FAILED               ((DWORD)0x00000003)

//
// MessageId: MSG_DLOPEN_FAILED
//
// MessageText:
//
// Unable to load module "%1": %2
//
#define MSG_DLOPEN_FAILED              ((DWORD)0x00000004)

//
// MessageId: MSG_DBDRV_NO_ENTRY_POINTS
//
// MessageText:
//
// Unable to find all required exportable functions in database driver "%1"
//
#define MSG_DBDRV_NO_ENTRY_POINTS      ((DWORD)0x00000005)

//
// MessageId: MSG_DBDRV_INIT_FAILED
//
// MessageText:
//
// Database driver "%1" initialization failed
//
#define MSG_DBDRV_INIT_FAILED          ((DWORD)0x00000006)

//
// MessageId: MSG_DBDRV_LOADED
//
// MessageText:
//
// Database driver "%1" loaded and initialized successfully
//
#define MSG_DBDRV_LOADED               ((DWORD)0x00000007)

//
// MessageId: MSG_DB_CONNFAIL
//
// MessageText:
//
// Unable to establish connection with database
//
#define MSG_DB_CONNFAIL                ((DWORD)0x00000008)

//
// MessageId: MSG_OBJECT_ID_EXIST
//
// MessageText:
//
// Attempt to add node with non-unique ID %1 to database
//
#define MSG_OBJECT_ID_EXIST            ((DWORD)0x00000009)

//
// MessageId: MSG_OBJECT_ADDR_EXIST
//
// MessageText:
//
// Attempt to add node with non-unique IP address %1 to database
//
#define MSG_OBJECT_ADDR_EXIST          ((DWORD)0x0000000a)

//
// MessageId: MSG_BAD_NETOBJ_TYPE
//
// MessageText:
//
// Internal error: invalid NetObj.Type() value %1
//
#define MSG_BAD_NETOBJ_TYPE            ((DWORD)0x0000000b)

//
// MessageId: MSG_RAW_SOCK_FAILED
//
// MessageText:
//
// Unable to create raw socket for ICMP protocol
//
#define MSG_RAW_SOCK_FAILED            ((DWORD)0x0000000c)

//
// MessageId: MSG_DUPLICATE_KEY
//
// MessageText:
//
// Attempt to add object with non-unique key %1 to index
//
#define MSG_DUPLICATE_KEY              ((DWORD)0x0000000d)

//
// MessageId: MSG_SUBNET_LOAD_FAILED
//
// MessageText:
//
// Failed to load subnet object with id %1 from database
//
#define MSG_SUBNET_LOAD_FAILED         ((DWORD)0x0000000e)

//
// MessageId: MSG_NODE_LOAD_FAILED
//
// MessageText:
//
// Failed to load node object with id %1 from database
//
#define MSG_NODE_LOAD_FAILED           ((DWORD)0x0000000f)

//
// MessageId: MSG_INTERFACE_LOAD_FAILED
//
// MessageText:
//
// Failed to load interface object with id %1 from database
//
#define MSG_INTERFACE_LOAD_FAILED      ((DWORD)0x00000010)

//
// MessageId: MSG_CONTAINER_LOAD_FAILED
//
// MessageText:
//
// Failed to load container object with id %1 from database
//
#define MSG_CONTAINER_LOAD_FAILED      ((DWORD)0x00000011)

//
// MessageId: MSG_TG_LOAD_FAILED
//
// MessageText:
//
// Failed to load template group object with id %1 from database
//
#define MSG_TG_LOAD_FAILED             ((DWORD)0x00000012)

//
// MessageId: MSG_TEMPLATE_LOAD_FAILED
//
// MessageText:
//
// Failed to load template object with id %1 from database
//
#define MSG_TEMPLATE_LOAD_FAILED       ((DWORD)0x00000013)

//
// MessageId: MSG_INVALID_SUBNET_ID
//
// MessageText:
//
// Inconsistent database: node %1 linked to non-existing subnet %2
//
#define MSG_INVALID_SUBNET_ID          ((DWORD)0x00000014)

//
// MessageId: MSG_SUBNET_NOT_SUBNET
//
// MessageText:
//
// Inconsistent database: node %1 linked to object %2 which is not a subnet
//
#define MSG_SUBNET_NOT_SUBNET          ((DWORD)0x00000015)

//
// MessageId: MSG_INVALID_NODE_ID
//
// MessageText:
//
// Inconsistent database: interface %1 linked to non-existing node %2
//
#define MSG_INVALID_NODE_ID            ((DWORD)0x00000016)

//
// MessageId: MSG_NODE_NOT_NODE
//
// MessageText:
//
// Inconsistent database: interface %1 linked to object %2 which is not a node
//
#define MSG_NODE_NOT_NODE              ((DWORD)0x00000017)

//
// MessageId: MSG_SNMP_UNKNOWN_TYPE
//
// MessageText:
//
// Unknown SNMP variable type %1 in GET response PDU
//
#define MSG_SNMP_UNKNOWN_TYPE          ((DWORD)0x00000018)

//
// MessageId: MSG_SNMP_GET_ERROR
//
// MessageText:
//
// Error %1 in processing SNMP GET request
//
#define MSG_SNMP_GET_ERROR             ((DWORD)0x00000019)

//
// MessageId: MSG_SNMP_BAD_PACKET
//
// MessageText:
//
// Error in SNMP response packet: %1
//
#define MSG_SNMP_BAD_PACKET            ((DWORD)0x0000001a)

//
// MessageId: MSG_OID_PARSE_ERROR
//
// MessageText:
//
// Error parsing SNMP OID '%1'
//
#define MSG_OID_PARSE_ERROR            ((DWORD)0x0000001b)

//
// MessageId: MSG_SNMP_TRANSPORT_FAILED
//
// MessageText:
//
// Unable to create UDP transport for SNMP
//
#define MSG_SNMP_TRANSPORT_FAILED      ((DWORD)0x0000001c)

//
// MessageId: MSG_SOCKET_FAILED
//
// MessageText:
//
// Unable to create socket in function %1
//
#define MSG_SOCKET_FAILED              ((DWORD)0x0000001d)

//
// MessageId: MSG_BIND_ERROR
//
// MessageText:
//
// Unable to bind socket to port %1 in function %2: %3
//
#define MSG_BIND_ERROR                 ((DWORD)0x0000001e)

//
// MessageId: MSG_ACCEPT_ERROR
//
// MessageText:
//
// Error returned by accept() system call: %1
//
#define MSG_ACCEPT_ERROR               ((DWORD)0x0000001f)

//
// MessageId: MSG_TOO_MANY_ACCEPT_ERRORS
//
// MessageText:
//
// There are too many consecutive accept() errors
//
#define MSG_TOO_MANY_ACCEPT_ERRORS     ((DWORD)0x00000020)

//
// MessageId: MSG_AGENT_CONNECT_FAILED
//
// MessageText:
//
// Unable to establish connection with agent on host %1
//
#define MSG_AGENT_CONNECT_FAILED       ((DWORD)0x00000021)

//
// MessageId: MSG_AGENT_COMM_FAILED
//
// MessageText:
//
// Unable to communicate with agent on host %1 (possibly due to authentication failure)
//
#define MSG_AGENT_COMM_FAILED          ((DWORD)0x00000022)

//
// MessageId: MSG_AGENT_BAD_HELLO
//
// MessageText:
//
// Invalid HELLO message received from agent on host %1 (possibly due to incompatible protocol version)
//
#define MSG_AGENT_BAD_HELLO            ((DWORD)0x00000023)

//
// MessageId: MSG_GETIPADDRTABLE_FAILED
//
// MessageText:
//
// Call to GetIpAddrTable() failed: %1
//
#define MSG_GETIPADDRTABLE_FAILED      ((DWORD)0x00000024)

//
// MessageId: MSG_GETIPNETTABLE_FAILED
//
// MessageText:
//
// Call to GetIpNetTable() failed: %1
//
#define MSG_GETIPNETTABLE_FAILED       ((DWORD)0x00000025)

//
// MessageId: MSG_EVENT_LOAD_ERROR
//
// MessageText:
//
// Unable to load events from database
//
#define MSG_EVENT_LOAD_ERROR           ((DWORD)0x00000026)

//
// MessageId: MSG_THREAD_HANGS
//
// MessageText:
//
// Thread "%1" does not respond to watchdog thread
//
#define MSG_THREAD_HANGS               ((DWORD)0x00000027)

//
// MessageId: MSG_SESSION_CLOSED
//
// MessageText:
//
// Client session closed due to communication error (%1)
//
#define MSG_SESSION_CLOSED             ((DWORD)0x00000028)

//
// MessageId: MSG_TOO_MANY_SESSIONS
//
// MessageText:
//
// Too many client sessions open - unable to accept new client connection
//
#define MSG_TOO_MANY_SESSIONS          ((DWORD)0x00000029)

//
// MessageId: MSG_ERROR_LOADING_USERS
//
// MessageText:
//
// Unable to load users and user groups from database (probably database is corrupted)
//
#define MSG_ERROR_LOADING_USERS        ((DWORD)0x0000002a)

//
// MessageId: MSG_SQL_ERROR
//
// MessageText:
//
// SQL query failed (Query = "%1")
//
#define MSG_SQL_ERROR                  ((DWORD)0x0000002b)

//
// MessageId: MSG_INVALID_SHA1_HASH
//
// MessageText:
//
// Invalid password hash for user %1: password reset to default
//
#define MSG_INVALID_SHA1_HASH          ((DWORD)0x0000002c)

//
// MessageId: MSG_EPP_LOAD_FAILED
//
// MessageText:
//
// Error loading event processing policy from database
//
#define MSG_EPP_LOAD_FAILED            ((DWORD)0x0000002d)

//
// MessageId: MSG_INIT_LOCKS_FAILED
//
// MessageText:
//
// Error initializing component locks table
//
#define MSG_INIT_LOCKS_FAILED          ((DWORD)0x0000002e)

//
// MessageId: MSG_DB_LOCKED
//
// MessageText:
//
// Database is already locked by another NetXMS server instance (IP address: %1, machine info: %2)
//
#define MSG_DB_LOCKED                  ((DWORD)0x0000002f)

//
// MessageId: MSG_ACTIONS_LOAD_FAILED
//
// MessageText:
//
// Error loading event actions configuration from database
//
#define MSG_ACTIONS_LOAD_FAILED        ((DWORD)0x00000030)

//
// MessageId: MSG_CREATE_PROCESS_FAILED
//
// MessageText:
//
// Unable to create process "%1": %2
//
#define MSG_CREATE_PROCESS_FAILED      ((DWORD)0x00000031)

//
// MessageId: MSG_NO_UNIQUE_ID
//
// MessageText:
//
// Unable to assign unique ID to object in group "%1". You should perform database optimization to fix that.
//
#define MSG_NO_UNIQUE_ID               ((DWORD)0x00000032)

//
// MessageId: MSG_INVALID_DTYPE
//
// MessageText:
//
// Internal error: invalid DTYPE %1 in %2
//
#define MSG_INVALID_DTYPE              ((DWORD)0x00000033)

//
// MessageId: MSG_IOCTL_FAILED
//
// MessageText:
//
// Call to ioctl() failed (Operation: %1 Error: %2)
//
#define MSG_IOCTL_FAILED               ((DWORD)0x00000034)

//
// MessageId: MSG_IFNAMEINDEX_FAILED
//
// MessageText:
//
// Call to if_nameindex() failed (%1)
//
#define MSG_IFNAMEINDEX_FAILED         ((DWORD)0x00000035)

//
// MessageId: MSG_SUPERUSER_CREATED
//
// MessageText:
//
// Superuser account created because it was not presented in database
//
#define MSG_SUPERUSER_CREATED          ((DWORD)0x00000036)

//
// MessageId: MSG_EVERYONE_GROUP_CREATED
//
// MessageText:
//
// User group "Everyone" created because it was not presented in database
//
#define MSG_EVERYONE_GROUP_CREATED     ((DWORD)0x00000037)

//
// MessageId: MSG_INVALID_DATA_DIR
//
// MessageText:
//
// Data directory "%1" does not exist or is inaccessible
//
#define MSG_INVALID_DATA_DIR           ((DWORD)0x00000038)

//
// MessageId: MSG_ERROR_CREATING_DATA_DIR
//
// MessageText:
//
// Error creating data directory "%1"
//
#define MSG_ERROR_CREATING_DATA_DIR    ((DWORD)0x00000039)

//
// MessageId: MSG_INVALID_EPP_OBJECT
//
// MessageText:
//
// Invalid object identifier %1 in event processing policy
//
#define MSG_INVALID_EPP_OBJECT         ((DWORD)0x0000003a)

//
// MessageId: MSG_INVALID_CONTAINER_MEMBER
//
// MessageText:
//
// Inconsistent database: container object %1 has reference to non-existing child object %2
//
#define MSG_INVALID_CONTAINER_MEMBER   ((DWORD)0x0000003b)

//
// MessageId: MSG_ROOT_INVALID_CHILD_ID
//
// MessageText:
//
// Inconsistent database: %2 object has reference to non-existing child object %1
//
#define MSG_ROOT_INVALID_CHILD_ID      ((DWORD)0x0000003c)

//
// MessageId: MSG_ERROR_READ_IMAGE_CATALOG
//
// MessageText:
//
// Error reading image catalog from database
//
#define MSG_ERROR_READ_IMAGE_CATALOG   ((DWORD)0x0000003d)

//
// MessageId: MSG_IMAGE_FILE_IO_ERROR
//
// MessageText:
//
// I/O error reading image file %2 (image ID %1)
//
#define MSG_IMAGE_FILE_IO_ERROR        ((DWORD)0x0000003e)

//
// MessageId: MSG_WRONG_DB_VERSION
//
// MessageText:
//
// Your database has format version %1, but server is compiled for version %2
//
#define MSG_WRONG_DB_VERSION           ((DWORD)0x0000003f)

//
// MessageId: MSG_ACTION_INIT_ERROR
//
// MessageText:
//
// Unable to initialize actions
//
#define MSG_ACTION_INIT_ERROR          ((DWORD)0x00000040)

//
// MessageId: MSG_DEBUG
//
// MessageText:
//
// DEBUG: %1
//
#define MSG_DEBUG                      ((DWORD)0x00000041)

//
// MessageId: MSG_SIGNAL_RECEIVED
//
// MessageText:
//
// Signal %1 received
//
#define MSG_SIGNAL_RECEIVED            ((DWORD)0x00000042)

//
// MessageId: MSG_INVALID_DCT_MAP
//
// MessageText:
//
// Inconsistent database: template object %1 has reference to non-existing node object %2
//
#define MSG_INVALID_DCT_MAP            ((DWORD)0x00000043)

//
// MessageId: MSG_DCT_MAP_NOT_NODE
//
// MessageText:
//
// Inconsistent database: template object %1 has reference to child object %2 which is not a node object
//
#define MSG_DCT_MAP_NOT_NODE           ((DWORD)0x00000044)

//
// MessageId: MSG_INVALID_TRAP_OID
//
// MessageText:
//
// Invalid trap enterprise ID %1 in trap configuration table
//
#define MSG_INVALID_TRAP_OID           ((DWORD)0x00000045)

//
// MessageId: MSG_INVALID_TRAP_ARG_OID
//
// MessageText:
//
// Invalid trap parameter OID %1 for trap %2 in trap configuration table
//
#define MSG_INVALID_TRAP_ARG_OID       ((DWORD)0x00000046)

//
// MessageId: MSG_MODULE_BAD_MAGIC
//
// MessageText:
//
// Module "%1" has invalid magic number - probably it was compiled for different NetXMS server version
//
#define MSG_MODULE_BAD_MAGIC           ((DWORD)0x00000047)

//
// MessageId: MSG_MODULE_LOADED
//
// MessageText:
//
// Server module "%1" loaded successfully
//
#define MSG_MODULE_LOADED              ((DWORD)0x00000048)

//
// MessageId: MSG_NO_MODULE_ENTRY_POINT
//
// MessageText:
//
// Unable to find entry point in server module "%1"
//
#define MSG_NO_MODULE_ENTRY_POINT      ((DWORD)0x00000049)

//
// MessageId: MSG_MODULE_INIT_FAILED
//
// MessageText:
//
// Initialization of server module "%1" failed
//
#define MSG_MODULE_INIT_FAILED         ((DWORD)0x0000004a)

//
// MessageId: MSG_NETSRV_LOAD_FAILED
//
// MessageText:
//
// Failed to load network service object with id %1 from database
//
#define MSG_NETSRV_LOAD_FAILED         ((DWORD)0x0000004b)

//
// MessageId: MSG_SMSDRV_NO_ENTRY_POINTS
//
// MessageText:
//
// Unable to find all required exportable functions in SMS driver "%1"
//
#define MSG_SMSDRV_NO_ENTRY_POINTS     ((DWORD)0x0000004c)

//
// MessageId: MSG_SMSDRV_INIT_FAILED
//
// MessageText:
//
// SMS driver "%1" initialization failed
//
#define MSG_SMSDRV_INIT_FAILED         ((DWORD)0x0000004d)

//
// MessageId: MSG_ZONE_LOAD_FAILED
//
// MessageText:
//
// Failed to load zone object with id %1 from database
//
#define MSG_ZONE_LOAD_FAILED           ((DWORD)0x0000004e)

//
// MessageId: MSG_GSM_MODEM_INFO
//
// MessageText:
//
// GSM modem on %1 initialized successfully. Hardware ID: "%2".
//
#define MSG_GSM_MODEM_INFO             ((DWORD)0x0000004f)

//
// MessageId: MSG_INIT_CRYPTO_FAILED
//
// MessageText:
//
// Failed to initialize cryptografy module
//
#define MSG_INIT_CRYPTO_FAILED         ((DWORD)0x00000050)

//
// MessageId: MSG_VPNC_LOAD_FAILED
//
// MessageText:
//
// Failed to load VPN connector object with id %1 from database
//
#define MSG_VPNC_LOAD_FAILED           ((DWORD)0x00000051)

//
// MessageId: MSG_INVALID_NODE_ID_EX
//
// MessageText:
//
// Inconsistent database: %3 %1 linked to non-existing node %2
//
#define MSG_INVALID_NODE_ID_EX         ((DWORD)0x00000052)

//
// MessageId: MSG_SCRIPT_COMPILATION_ERROR
//
// MessageText:
//
// Error compiling library script %2 (ID: %1): %3
//
#define MSG_SCRIPT_COMPILATION_ERROR   ((DWORD)0x00000053)

//
// MessageId: MSG_RADIUS_UNKNOWN_ENCR_METHOD
//
// MessageText:
//
// RADIUS client error: encryption style %1 is not implemented (attribute %2)
//
#define MSG_RADIUS_UNKNOWN_ENCR_METHOD ((DWORD)0x00000054)

//
// MessageId: MSG_RADIUS_DECR_FAILED
//
// MessageText:
//
// RADIUS client error: decryption (style %1) failed for attribute %2
//
#define MSG_RADIUS_DECR_FAILED         ((DWORD)0x00000055)

//
// MessageId: MSG_RADIUS_AUTH_SUCCESS
//
// MessageText:
//
// User %1 was successfully authenticated by RADIUS server %2
//
#define MSG_RADIUS_AUTH_SUCCESS        ((DWORD)0x00000056)

//
// MessageId: MSG_RADIUS_AUTH_FAILED
//
// MessageText:
//
// Authentication request for user %1 was rejected by RADIUS server %2
//
#define MSG_RADIUS_AUTH_FAILED         ((DWORD)0x00000057)

//
// MessageId: MSG_UNKNOWN_AUTH_METHOD
//
// MessageText:
//
// Unsupported authentication method %1 requested for user %2
//
#define MSG_UNKNOWN_AUTH_METHOD        ((DWORD)0x00000058)

//
// MessageId: MSG_CONDITION_LOAD_FAILED
//
// MessageText:
//
// Failed to load condition object with id %1 from database
//
#define MSG_CONDITION_LOAD_FAILED      ((DWORD)0x00000059)

//
// MessageId: MSG_COND_SCRIPT_COMPILATION_ERROR
//
// MessageText:
//
// Failed to compile evaluation script for condition object %1 "%2": %3
//
#define MSG_COND_SCRIPT_COMPILATION_ERROR ((DWORD)0x0000005a)

//
// MessageId: MSG_COND_SCRIPT_EXECUTION_ERROR
//
// MessageText:
//
// Failed to execute evaluation script for condition object %1 "%2": %3
//
#define MSG_COND_SCRIPT_EXECUTION_ERROR ((DWORD)0x0000005b)

//
// MessageId: MSG_DB_LOCK_REMOVED
//
// MessageText:
//
// Stalled database lock removed
//
#define MSG_DB_LOCK_REMOVED            ((DWORD)0x0000005c)

//
// MessageId: MSG_DBDRV_API_VERSION_MISMATCH
//
// MessageText:
//
// Database driver "%1" cannot be loaded because of API version mismatch (driver: %3; server: %2)
//
#define MSG_DBDRV_API_VERSION_MISMATCH ((DWORD)0x0000005d)

#endif
