;/****************************************************************************
; Messages for NetXMS Server
;****************************************************************************/
;
;#ifndef _messages_h_
;#define _messages_h_
;

MessageIdTypedef=DWORD

MessageId=1
SymbolicName=MSG_SERVER_STARTED
Language=English
NetXMS Server started
.

MessageId=
SymbolicName=MSG_SERVER_STOPPED
Language=English
NetXMS Server stopped
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
Unable to establish connection with database (%1)
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
SymbolicName=MSG_CONTAINER_LOAD_FAILED
Language=English
Failed to load container object with id %1 from database
.

MessageId=
SymbolicName=MSG_TG_LOAD_FAILED
Language=English
Failed to load template group object with id %1 from database
.

MessageId=
SymbolicName=MSG_TEMPLATE_LOAD_FAILED
Language=English
Failed to load template object with id %1 from database
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
Unknown SNMP variable type %1 in GET response PDU
.

MessageId=
SymbolicName=MSG_SNMP_GET_ERROR
Language=English
Error %1 in processing SNMP GET request
.

MessageId=
SymbolicName=MSG_SNMP_BAD_PACKET
Language=English
Error in SNMP response packet: %1
.

MessageId=
SymbolicName=MSG_OID_PARSE_ERROR
Language=English
Error parsing SNMP OID "%1" in %2 (destination IP address %3)
.

MessageId=
SymbolicName=MSG_SNMP_TRANSPORT_FAILED
Language=English
Unable to create UDP transport for SNMP
.

MessageId=
SymbolicName=MSG_SOCKET_FAILED
Language=English
Unable to create socket in function %1
.

MessageId=
SymbolicName=MSG_BIND_ERROR
Language=English
Unable to bind socket to port %1 in function %2: %3
.

MessageId=
SymbolicName=MSG_ACCEPT_ERROR
Language=English
Error returned by accept() system call: %1
.

MessageId=
SymbolicName=MSG_TOO_MANY_ACCEPT_ERRORS
Language=English
There are too many consecutive accept() errors
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

MessageId=
SymbolicName=MSG_SESSION_CLOSED
Language=English
Client session closed due to communication error (%1)
.

MessageId=
SymbolicName=MSG_TOO_MANY_SESSIONS
Language=English
Too many client sessions open - unable to accept new client connection
.

MessageId=
SymbolicName=MSG_ERROR_LOADING_USERS
Language=English
Unable to load users and user groups from database (probably database is corrupted)
.

MessageId=
SymbolicName=MSG_SQL_ERROR
Language=English
SQL query failed (Query = "%1"): %2
.

MessageId=
SymbolicName=MSG_INVALID_SHA1_HASH
Language=English
Invalid password hash for user %1: password reset to default
.

MessageId=
SymbolicName=MSG_EPP_LOAD_FAILED
Language=English
Error loading event processing policy from database
.

MessageId=
SymbolicName=MSG_INIT_LOCKS_FAILED
Language=English
Error initializing component locks table
.

MessageId=
SymbolicName=MSG_DB_LOCKED
Language=English
Database is already locked by another NetXMS server instance (IP address: %1, machine info: %2)
.

MessageId=
SymbolicName=MSG_ACTIONS_LOAD_FAILED
Language=English
Error loading event actions configuration from database
.

MessageId=
SymbolicName=MSG_CREATE_PROCESS_FAILED
Language=English
Unable to create process "%1": %2
.

MessageId=
SymbolicName=MSG_NO_UNIQUE_ID
Language=English
Unable to assign unique ID to object in group "%1". You should perform database optimization to fix that.
.

MessageId=
SymbolicName=MSG_INVALID_DTYPE
Language=English
Internal error: invalid DTYPE %1 in %2
.

MessageId=
SymbolicName=MSG_IOCTL_FAILED
Language=English
Call to ioctl() failed (Operation: %1 Error: %2)
.

MessageId=
SymbolicName=MSG_IFNAMEINDEX_FAILED
Language=English
Call to if_nameindex() failed (%1)
.

MessageId=
SymbolicName=MSG_SUPERUSER_CREATED
Language=English
Superuser account created because it was not presented in database
.

MessageId=
SymbolicName=MSG_EVERYONE_GROUP_CREATED
Language=English
User group "Everyone" created because it was not presented in database
.

MessageId=
SymbolicName=MSG_INVALID_DATA_DIR
Language=English
Data directory "%1" does not exist or is inaccessible
.

MessageId=
SymbolicName=MSG_ERROR_CREATING_DATA_DIR
Language=English
Error creating data directory "%1"
.

MessageId=
SymbolicName=MSG_INVALID_EPP_OBJECT
Language=English
Invalid object identifier %1 in event processing policy
.

MessageId=
SymbolicName=MSG_INVALID_CONTAINER_MEMBER
Language=English
Inconsistent database: container object %1 has reference to non-existing child object %2
.

MessageId=
SymbolicName=MSG_ROOT_INVALID_CHILD_ID
Language=English
Inconsistent database: %2 object has reference to non-existing child object %1
.

MessageId=
SymbolicName=MSG_ERROR_READ_IMAGE_CATALOG
Language=English
Error reading image catalog from database
.

MessageId=
SymbolicName=MSG_IMAGE_FILE_IO_ERROR
Language=English
I/O error reading image file %2 (image ID %1)
.

MessageId=
SymbolicName=MSG_WRONG_DB_VERSION
Language=English
Your database has format version %1, but server is compiled for version %2
.

MessageId=
SymbolicName=MSG_ACTION_INIT_ERROR
Language=English
Unable to initialize actions
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
SymbolicName=MSG_INVALID_DCT_MAP
Language=English
Inconsistent database: template object %1 has reference to non-existing node object %2
.

MessageId=
SymbolicName=MSG_DCT_MAP_NOT_NODE
Language=English
Inconsistent database: template object %1 has reference to child object %2 which is not a node object
.

MessageId=
SymbolicName=MSG_INVALID_TRAP_OID
Language=English
Invalid trap enterprise ID %1 in trap configuration table
.

MessageId=
SymbolicName=MSG_INVALID_TRAP_ARG_OID
Language=English
Invalid trap parameter OID %1 for trap %2 in trap configuration table
.

MessageId=
SymbolicName=MSG_MODULE_BAD_MAGIC
Language=English
Module "%1" has invalid magic number - probably it was compiled for different NetXMS server version
.

MessageId=
SymbolicName=MSG_MODULE_LOADED
Language=English
Server module "%1" loaded successfully
.

MessageId=
SymbolicName=MSG_NO_MODULE_ENTRY_POINT
Language=English
Unable to find entry point in server module "%1"
.

MessageId=
SymbolicName=MSG_MODULE_INIT_FAILED
Language=English
Initialization of server module "%1" failed
.

MessageId=
SymbolicName=MSG_NETSRV_LOAD_FAILED
Language=English
Failed to load network service object with id %1 from database
.

MessageId=
SymbolicName=MSG_SMSDRV_NO_ENTRY_POINTS
Language=English
Unable to find all required exportable functions in SMS driver "%1"
.

MessageId=
SymbolicName=MSG_SMSDRV_INIT_FAILED
Language=English
SMS driver "%1" initialization failed
.

MessageId=
SymbolicName=MSG_ZONE_LOAD_FAILED
Language=English
Failed to load zone object with id %1 from database
.

MessageId=
SymbolicName=MSG_GSM_MODEM_INFO
Language=English
GSM modem on %1 initialized successfully. Hardware ID: "%2".
.

MessageId=
SymbolicName=MSG_INIT_CRYPTO_FAILED
Language=English
Failed to initialize cryptografy module
.

MessageId=
SymbolicName=MSG_VPNC_LOAD_FAILED
Language=English
Failed to load VPN connector object with id %1 from database
.

MessageId=
SymbolicName=MSG_INVALID_NODE_ID_EX
Language=English
Inconsistent database: %3 %1 linked to non-existing node %2
.

MessageId=
SymbolicName=MSG_SCRIPT_COMPILATION_ERROR
Language=English
Error compiling library script %2 (ID: %1): %3
.

MessageId=
SymbolicName=MSG_RADIUS_UNKNOWN_ENCR_METHOD
Language=English
RADIUS client error: encryption style %1 is not implemented (attribute %2)
.

MessageId=
SymbolicName=MSG_RADIUS_DECR_FAILED
Language=English
RADIUS client error: decryption (style %1) failed for attribute %2
.

MessageId=
SymbolicName=MSG_RADIUS_AUTH_SUCCESS
Language=English
User %1 was successfully authenticated by RADIUS server %2
.

MessageId=
SymbolicName=MSG_RADIUS_AUTH_FAILED
Language=English
Authentication request for user %1 was rejected by RADIUS server %2
.

MessageId=
SymbolicName=MSG_UNKNOWN_AUTH_METHOD
Language=English
Unsupported authentication method %1 requested for user %2
.

MessageId=
SymbolicName=MSG_CONDITION_LOAD_FAILED
Language=English
Failed to load condition object with id %1 from database
.

MessageId=
SymbolicName=MSG_COND_SCRIPT_COMPILATION_ERROR
Language=English
Failed to compile evaluation script for condition object %1 "%2": %3
.

MessageId=
SymbolicName=MSG_COND_SCRIPT_EXECUTION_ERROR
Language=English
Failed to execute evaluation script for condition object %1 "%2": %3
.

MessageId=
SymbolicName=MSG_DB_LOCK_REMOVED
Language=English
Stalled database lock removed
.

MessageId=
SymbolicName=MSG_DBDRV_API_VERSION_MISMATCH
Language=English
Database driver "%1" cannot be loaded because of API version mismatch (driver: %3; server: %2)
.

MessageId=
SymbolicName=MSG_PLATFORM_SUBAGENT_LOADED
Language=English
Platform subagent "%1" successfully loaded
.

MessageId=
SymbolicName=MSG_PLATFORM_SUBAGENT_NOT_LOADED
Language=English
Cannot load platform subagent "%1": %2
.

MessageId=
SymbolicName=MSG_EPRULE_SCRIPT_COMPILATION_ERROR
Language=English
Failed to compile evaluation script for event processing policy rule #%1: %2
.

MessageId=
SymbolicName=MSG_EPRULE_SCRIPT_EXECUTION_ERROR
Language=English
Failed to execute evaluation script for event processing policy rule #%1: %2
.

MessageId=
SymbolicName=MSG_CANNOT_FIND_SELF
Language=English
NetXMS server cannot create node object for itself - probably because platform subagent cannot be loaded (check above error messages, if any)
.

MessageId=
SymbolicName=MSG_INVALID_CLUSTER_MEMBER
Language=English
Inconsistent database: cluster object %1 has reference to non-existing node object %2
.

MessageId=
SymbolicName=MSG_CLUSTER_MEMBER_NOT_NODE
Language=English
Inconsistent database: cluster object %1 has reference to child object %2 which is not a node object
.

MessageId=
SymbolicName=MSG_CLUSTER_LOAD_FAILED
Language=English
Failed to load cluster object with id %1 from database
.

MessageId=
SymbolicName=MSG_EXCEPTION
Language=English
EXCEPTION %1 (%2) at %3 (crash dump was generated); please send files %4 and %5 to dump@netxms.org
.

MessageId=
SymbolicName=MSG_CANNOT_INIT_CERT_STORE
Language=English
Cannot initialize certificate store: %1
.

MessageId=
SymbolicName=MSG_CANNOT_LOAD_CERT
Language=English
Cannot load certificate %1 from database: %2
.

MessageId=
SymbolicName=MSG_CANNOT_ADD_CERT
Language=English
Cannot add certificate %1 to store: %2
.

MessageId=
SymbolicName=MSG_CA_CERTIFICATES_LOADED
Language=English
Successfully loaded %1 trusted CA certificates
.

MessageId=
SymbolicName=MSG_INTERNAL_ERROR
Language=English
Internal error: %1
.

MessageId=
SymbolicName=MSG_CODEPAGE_ERROR
Language=English
Unable to set codepage to %1
.

MessageId=
SymbolicName=MSG_INVALID_BEACON
Language=English
Invalid beacon host name or address %1 - host will be excluded from beacon list
.

MessageId=
SymbolicName=MSG_LISTENING_FOR_CLIENTS
Language=English
Listening for client connections on TCP socket %1:%2
.

MessageId=
SymbolicName=MSG_LISTENING_FOR_SNMP
Language=English
Listening for SNMP traps on UDP socket %1:%2
.

MessageId=
SymbolicName=MSG_LISTENING_FOR_SYSLOG
Language=English
Listening for syslog messages on UDP socket %1:%2
.

MessageId=
SymbolicName=MSG_OTHER
Language=English
%1
.

MessageId=
SymbolicName=MSG_SYSLOG_PARSER_INIT_FAILED
Language=English
Cannot initialize syslog parser: %1
.

MessageId=
SymbolicName=MSG_ISC_SESSION_ESTABLISHED
Language=English
Established ISC session from %1 to service %2
.

MessageId=
SymbolicName=MSG_ISC_SESSION_SETUP_FAILED
Language=English
Failed to establish ISC session from %1 to service %2: %3
.

MessageId=
SymbolicName=MSG_EVENT_FORWARD_FAILED
Language=English
Failed to forward event to server %1: %2
.

MessageId=
SymbolicName=MSG_TEMPLATE_SCRIPT_COMPILATION_ERROR
Language=English
Failed to compile filter script for template object %1 "%2": %3
.

MessageId=
SymbolicName=MSG_TEMPLATE_SCRIPT_EXECUTION_ERROR
Language=English
Failed to execute filter script for template object %1 "%2": %3
.

MessageId=
SymbolicName=MSG_CONTAINER_SCRIPT_COMPILATION_ERROR
Language=English
Failed to compile filter script for container object %1 "%2": %3
.

MessageId=
SymbolicName=MSG_CONTAINER_SCRIPT_EXECUTION_ERROR
Language=English
Failed to execute filter script for container object %1 "%2": %3
.

MessageId=
SymbolicName=MSG_PG_LOAD_FAILED
Language=English
Failed to load policy group object with id %1 from database
.

MessageId=
SymbolicName=MSG_AGENTPOLICY_LOAD_FAILED
Language=English
Failed to load agent policy object with id %1 from database
.

MessageId=
SymbolicName=MSG_INVALID_AP_BINDING
Language=English
Inconsistent database: agent policy object %1 has reference to non-existing node object %2
.

MessageId=
SymbolicName=MSG_AP_BINDING_NOT_NODE
Language=English
Inconsistent database: agent policy object %1 has reference to child object %2 which is not a node object
.

MessageId=
SymbolicName=MSG_MG_LOAD_FAILED
Language=English
Failed to load network map group object with id %1 from database
.

MessageId=
SymbolicName=MSG_NETMAP_LOAD_FAILED
Language=English
Failed to load network map object with id %1 from database
.

MessageId=
SymbolicName=MSG_SET_PROCESS_AFFINITY_NOT_SUPPORTED
Language=English
Setting process CPU affinity is not supported on this operating system
.

MessageId=
SymbolicName=MSG_WSASTARTUP_FAILED
Language=English
Unable to initialize Windows Sockets: %1
.

MessageId=
SymbolicName=MSG_NDD_LOADED
Language=English
Network device driver "%1" loaded successfully
.

MessageId=
SymbolicName=MSG_NO_NDD_ENTRY_POINT
Language=English
Unable to find entry point in network device driver "%1"
.

MessageId=
SymbolicName=MSG_NDD_INIT_FAILED
Language=English
Initialization of network device driver "%1" failed
.

MessageId=
SymbolicName=MSG_NDD_API_VERSION_MISMATCH
Language=English
Network device driver "%1" cannot be loaded because of API version mismatch (driver: %3; server: %2)
.

MessageId=
SymbolicName=MSG_DASHBOARD_LOAD_FAILED
Language=English
Failed to load dashboard object with id %1 from database
.

MessageId=
SymbolicName=MSG_SMSDRV_INVALID_OPTION
Language=English
Invalid option %1 passed to SMS driver
.

MessageId=
SymbolicName=MSG_RG_LOAD_FAILED
Language=English
Failed to load report group object with id %1 from database
.

MessageId=
SymbolicName=MSG_REPORT_LOAD_FAILED
Language=English
Failed to load report object with id %1 from database
.

MessageId=
SymbolicName=MSG_SLMCHECK_SCRIPT_COMPILATION_ERROR
Language=English
Failed to compile script for service check object %1 "%2": %3
.

MessageId=
SymbolicName=MSG_SLMCHECK_SCRIPT_EXECUTION_ERROR
Language=English
Failed to execute script for service check object %1 "%2": %3
.

MessageId=
SymbolicName=MSG_BUSINESS_SERVICE_LOAD_FAILED
Language=English
Failed to load business service object with id %1 from database
.

MessageId=
SymbolicName=MSG_NODE_LINK_LOAD_FAILED
Language=English
Failed to load node link object with id %1 from database
.

MessageId=
SymbolicName=MSG_SERVICE_CHECK_LOAD_FAILED
Language=English
Failed to load service check object with id %1 from database
.

MessageId=
SymbolicName=MSG_MOBILEDEVICE_LOAD_FAILED
Language=English
Failed to load mobile device object with id %1 from database
.

MessageId=
SymbolicName=MSG_MD_SESSION_CLOSED
Language=English
Mobile device session closed due to communication error (%1)
.

MessageId=
SymbolicName=MSG_TOO_MANY_MD_SESSIONS
Language=English
Too many mobile device sessions open - unable to accept new client connection
.

MessageId=
SymbolicName=MSG_LISTENING_FOR_MOBILE_DEVICES
Language=English
Listening for mobile device connections on TCP socket %1:%2
.

MessageId=
SymbolicName=MSG_AP_LOAD_FAILED
Language=English
Failed to load access point object with id %1 from database
.

MessageId=
SymbolicName=MSG_RACK_LOAD_FAILED
Language=English
Failed to load rack object with id %1 from database
.

MessageId=
SymbolicName=MSG_NETMAP_SCRIPT_COMPILATION_ERROR
Language=English
Failed to compile filter script for network map object %1 "%2": %3
.

MessageId=
SymbolicName=MSG_NETMAP_SCRIPT_EXECUTION_ERROR
Language=English
Failed to execute filter script for network map object %1 "%2": %3
.

MessageId=
SymbolicName=MSG_XMPP_ERROR
Language=English
XMPP: %1 %2
.

MessageId=
SymbolicName=MSG_XMPP_WARNING
Language=English
XMPP: %1 %2
.

MessageId=
SymbolicName=MSG_XMPP_INFO
Language=English
XMPP: %1 %2
.

MessageId=
SymbolicName=MSG_SERIAL_PORT_OPEN_FAILED
Language=English
Unable to open serial port %1
.

MessageId=
SymbolicName=MSG_SERIAL_PORT_SET_FAILED
Language=English
Unable to configure serial port %1
.

MessageId=
SymbolicName=MSG_HDLINK_LOADED
Language=English
Helpdesk link module %1 (version %2) loaded successfully
.

MessageId=
SymbolicName=MSG_NO_HDLINK_ENTRY_POINT
Language=English
Unable to find entry point in helpdesk link module "%1"
.

MessageId=
SymbolicName=MSG_HDLINK_INIT_FAILED
Language=English
Initialization of helpdesk link module "%1" failed
.

MessageId=
SymbolicName=MSG_HDLINK_API_VERSION_MISMATCH
Language=English
Helpdesk link module "%1" cannot be loaded because of API version mismatch (module: %3; server: %2)
.

MessageId=
SymbolicName=MSG_PDSDRV_LOADED
Language=English
Performance data storage driver "%1" loaded successfully
.

MessageId=
SymbolicName=MSG_NO_PDSDRV_ENTRY_POINT
Language=English
Unable to find entry point in performance data storage driver "%1"
.

MessageId=
SymbolicName=MSG_PDSDRV_INIT_FAILED
Language=English
Initialization of performance data storage driver "%1" failed
.

MessageId=
SymbolicName=MSG_PDSDRV_API_VERSION_MISMATCH
Language=English
Performance data storage driver "%1" cannot be loaded because of API version mismatch (driver: %3; server: %2)
.

;#endif
