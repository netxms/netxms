/* 
** NetXMS - Network Management System
** Client Library API
** Copyright (C) 2003-2013 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: nxclapi.h
**
**/

#ifndef _nxclapi_h_
#define _nxclapi_h_

#ifdef LIBNXCL_CONSTANTS_ONLY

#ifndef LIBNXCL_NO_DECLARATIONS
#define LIBNXCL_NO_DECLARATIONS
#endif

#else

#include <nms_common.h>
#include <nms_util.h>
#include <nxevent.h>
#include <nxcpapi.h>
#include <nxtools.h>
#include <nxlog.h>
#include <uuid.h>

#ifdef _WIN32
#ifdef LIBNXCL_EXPORTS
#define LIBNXCL_EXPORTABLE __declspec(dllexport)
#else
#define LIBNXCL_EXPORTABLE __declspec(dllimport)
#endif
#else    /* _WIN32 */
#define LIBNXCL_EXPORTABLE
#endif

#endif /* not LIBNXCL_CONSTANTS_ONLY */

/**
 * Session handle type
 */
#ifndef LIBNXCL_CONSTANTS_ONLY
typedef void * NXC_SESSION;
#endif

/**
 * Some constants
 */
#define MAX_COMMUNITY_LENGTH     128
#define MAX_OID_LENGTH           1024
#define MAX_EVENT_MSG_LENGTH     2000
#define MAX_EVENT_NAME           64
#define MAX_USERTAG_LENGTH       64
#define MAX_SESSION_NAME         256
#define MAX_USER_NAME            64
#define MAX_USER_FULLNAME        128
#define MAX_USER_DESCR           256
#define MAX_ITEM_NAME            1024
#define MAX_STRING_VALUE         256
#define MAX_VARIABLE_NAME        256
#define MAX_AGENT_VERSION_LEN    64
#define MAX_PLATFORM_NAME_LEN    64
#define MAX_PACKAGE_NAME_LEN     64
#define MAX_HELPDESK_REF_LEN     64
#define GROUP_EVERYONE           ((UINT32)0x80000000)
#define INVALID_UID              ((UINT32)0xFFFFFFFF)
#define OBJECT_STATUS_COUNT      9
#define MAX_RCPT_ADDR_LEN        256
#define MAX_EMAIL_SUBJECT_LEN    256
#define MAC_ADDR_LENGTH          6
#define CURRENT_USER             ((UINT32)0xFFFFFFFF)
#define MAX_DCI_DATA_RECORDS     200000
#define MAX_POLICY_CONFIG_NAME   64


//
// Module flags
//

#define MODFLAG_DISABLED               0x0001
#define MODFLAG_ACCEPT_ALL_COMMANDS    0x0002


//
// NetXMS agent authentication methods
//

#define AUTH_NONE             0
#define AUTH_PLAINTEXT        1
#define AUTH_MD5_HASH         2
#define AUTH_SHA1_HASH        3

/**
 * Client-server authentication types
 */
#define NETXMS_AUTH_TYPE_PASSWORD       0
#define NETXMS_AUTH_TYPE_CERTIFICATE    1
#define NETXMS_AUTH_TYPE_SSO_TICKET     2

/**
 * Client type
 */
#define CLIENT_TYPE_DESKTOP      0
#define CLIENT_TYPE_WEB          1
#define CLIENT_TYPE_MOBILE       2
#define CLIENT_TYPE_TABLET       3
#define CLIENT_TYPE_APPLICATION  4

/**
 * Forced poll types
 */
#define POLL_STATUS           1
#define POLL_CONFIGURATION    2
#define POLL_INTERFACE_NAMES  3
#define POLL_TOPOLOGY         4

/**
 * Object types
 */
#define OBJECT_GENERIC              0
#define OBJECT_SUBNET               1
#define OBJECT_NODE                 2
#define OBJECT_INTERFACE            3
#define OBJECT_NETWORK              4
#define OBJECT_CONTAINER            5
#define OBJECT_ZONE                 6
#define OBJECT_SERVICEROOT          7
#define OBJECT_TEMPLATE             8
#define OBJECT_TEMPLATEGROUP        9
#define OBJECT_TEMPLATEROOT         10
#define OBJECT_NETWORKSERVICE       11
#define OBJECT_VPNCONNECTOR         12
#define OBJECT_CONDITION            13
#define OBJECT_CLUSTER			      14
#define OBJECT_POLICYGROUP          15
#define OBJECT_POLICYROOT           16
#define OBJECT_AGENTPOLICY          17
#define OBJECT_AGENTPOLICY_CONFIG   18
#define OBJECT_NETWORKMAPROOT       19
#define OBJECT_NETWORKMAPGROUP      20
#define OBJECT_NETWORKMAP           21
#define OBJECT_DASHBOARDROOT        22
#define OBJECT_DASHBOARD            23
#define OBJECT_REPORTROOT           24
#define OBJECT_REPORTGROUP          25
#define OBJECT_REPORT               26
#define OBJECT_BUSINESSSERVICEROOT  27
#define OBJECT_BUSINESSSERVICE      28
#define OBJECT_NODELINK             29
#define OBJECT_SLMCHECK             30
#define OBJECT_MOBILEDEVICE         31
#define OBJECT_RACK                 32
#define OBJECT_ACCESSPOINT          33

/** Base value for custom object classes */
#define OBJECT_CUSTOM               10000

/**
 * Object's status
 */
#define STATUS_NORMAL         0
#define STATUS_WARNING        1
#define STATUS_MINOR          2
#define STATUS_MAJOR          3
#define STATUS_CRITICAL       4
#define STATUS_UNKNOWN        5
#define STATUS_UNMANAGED      6
#define STATUS_DISABLED       7
#define STATUS_TESTING        8

/**
 * Event and alarm severity
 */
#define SEVERITY_NORMAL       0
#define SEVERITY_WARNING      1
#define SEVERITY_MINOR        2
#define SEVERITY_MAJOR        3
#define SEVERITY_CRITICAL     4
#define SEVERITY_FROM_EVENT   5
#define SEVERITY_TERMINATE    6
#define SEVERITY_RESOLVE      7

/**
 * Alarm states
 */
#define ALARM_STATE_OUTSTANDING  0x00
#define ALARM_STATE_ACKNOWLEDGED 0x01
#define ALARM_STATE_RESOLVED     0x02
#define ALARM_STATE_TERMINATED   0x03
#define ALARM_STATE_MASK         0x0F		/* mask for selecting alarm state */
#define ALARM_STATE_STICKY       0x10		/* bit flag indicating sticky state */

/**
 * Alarm state in help desk system
 */
#define ALARM_HELPDESK_IGNORED   0
#define ALARM_HELPDESK_OPEN      1
#define ALARM_HELPDESK_CLOSED    2

/**
 * Node flags
 */
#define NF_SYSTEM_FLAGS           0x003FFFFF
#define NF_USER_FLAGS             0xFFC00000

#define NF_IS_SNMP                0x00000001
#define NF_IS_NATIVE_AGENT        0x00000002
#define NF_IS_BRIDGE              0x00000004
#define NF_IS_ROUTER              0x00000008
#define NF_IS_LOCAL_MGMT          0x00000010
#define NF_IS_PRINTER             0x00000020
#define NF_IS_OSPF                0x00000040
#define NF_BEHIND_NAT             0x00000080
#define NF_IS_CPSNMP              0x00000100  /* CheckPoint SNMP agent on port 260 */
#define NF_IS_CDP                 0x00000200
#define NF_IS_NDP                 0x00000400  /* Supports Nortel (Synoptics/Bay Networks) topology discovery */
#define NF_IS_SONMP               0x00000400  /* SONMP is an old name for NDP */
#define NF_IS_LLDP                0x00000800 /* Supports Link Layer Discovery Protocol */
#define NF_IS_VRRP                0x00001000  /* VRRP support */
#define NF_HAS_VLANS              0x00002000  /* VLAN information available */
#define NF_IS_8021X               0x00004000  /* 802.1x support enabled on node */
#define NF_IS_STP                 0x00008000  /* Spanning Tree (IEEE 802.1d) enabled on node */
#define NF_HAS_ENTITY_MIB         0x00010000  /* Supports ENTITY-MIB */
#define NF_HAS_IFXTABLE           0x00020000  /* Supports ifXTable */
#define NF_HAS_AGENT_IFXCOUNTERS  0x00040000  /* Agent supports 64-bit interface counters */
#define NF_HAS_WINPDH             0x00080000  /* Node supports Windows PDH parameters */
#define NF_IS_WIFI_CONTROLLER     0x00100000  /* Node is wireless network controller */
#define NF_IS_SMCLP               0x00200000  /* Node supports SMCLP protocol */
#define NF_DISABLE_DISCOVERY_POLL 0x00400000
#define NF_DISABLE_TOPOLOGY_POLL  0x00800000
#define NF_DISABLE_SNMP           0x01000000
#define NF_DISABLE_NXCP           0x02000000
#define NF_DISABLE_ICMP           0x04000000
#define NF_FORCE_ENCRYPTION       0x08000000
#define NF_DISABLE_STATUS_POLL    0x10000000
#define NF_DISABLE_CONF_POLL      0x20000000
#define NF_DISABLE_ROUTE_POLL     0x40000000
#define NF_DISABLE_DATA_COLLECT   0x80000000

/**
 * Template flags
 */
#define TF_AUTO_APPLY            0x00000001
#define TF_AUTO_REMOVE           0x00000002

/**
 * Container flags
 */
#define CF_AUTO_BIND             0x00000001
#define CF_AUTO_UNBIND           0x00000002

/**
 * Interface flags
 */
#define IF_SYNTHETIC_MASK        0x00000001
#define IF_PHYSICAL_PORT         0x00000002
#define IF_EXCLUDE_FROM_TOPOLOGY 0x00000004
#define IF_LOOPBACK              0x00000008
#define IF_CREATED_MANUALLY      0x00000010
#define IF_EXPECTED_STATE_MASK   0x30000000	/* 2-bit field holding expected interface state */
#define IF_USER_FLAGS_MASK       (IF_EXCLUDE_FROM_TOPOLOGY)    /* flags that can be changed by user */

/**
 * Expected interface states
 */
#define IF_EXPECTED_STATE_UP     0
#define IF_EXPECTED_STATE_DOWN   1
#define IF_EXPECTED_STATE_IGNORE 2

/**
 * Interface administrative states
 */
#define IF_ADMIN_STATE_UNKNOWN   0
#define IF_ADMIN_STATE_UP        1
#define IF_ADMIN_STATE_DOWN      2
#define IF_ADMIN_STATE_TESTING   3

/**
 * Interface operational states
 */
#define IF_OPER_STATE_UNKNOWN    0
#define IF_OPER_STATE_UP         1
#define IF_OPER_STATE_DOWN       2
#define IF_OPER_STATE_TESTING    3

/**
 * Node ifXTable usage mode
 */
#define IFXTABLE_DEFAULT			0
#define IFXTABLE_ENABLED			1
#define IFXTABLE_DISABLED			2

/**
 * Status calculation and propagation algorithms
 */
#define SA_CALCULATE_DEFAULT              0
#define SA_CALCULATE_MOST_CRITICAL        1
#define SA_CALCULATE_SINGLE_THRESHOLD     2
#define SA_CALCULATE_MULTIPLE_THRESHOLDS  3

#define SA_PROPAGATE_DEFAULT              0
#define SA_PROPAGATE_UNCHANGED            1
#define SA_PROPAGATE_FIXED                2
#define SA_PROPAGATE_RELATIVE             3
#define SA_PROPAGATE_TRANSLATED           4


//
// Network map types
//

#define MAP_TYPE_CUSTOM          0
#define MAP_TYPE_LAYER2_TOPOLOGY 1
#define MAP_TYPE_IP_TOPOLOGY     2


//
// Components that can be locked by management pack installer
//

#define NXMP_LC_EVENTDB    0
#define NXMP_LC_EPP        1
#define NXMP_LC_TRAPCFG    2


//
// Service types
//

enum
{
	NETSRV_CUSTOM,
	NETSRV_SSH,
	NETSRV_POP3,
	NETSRV_SMTP,
	NETSRV_FTP,
	NETSRV_HTTP,
	NETSRV_HTTPS,
	NETSRV_TELNET
};


//
// Address list types
//

#define ADDR_LIST_DISCOVERY_TARGETS    1
#define ADDR_LIST_DISCOVERY_FILTER     2


//
// Discovery filter flags
//

#define DFF_ALLOW_AGENT                0x0001
#define DFF_ALLOW_SNMP                 0x0002
#define DFF_ONLY_RANGE                 0x0004


//
// Events
//

#define NXC_EVENT_CONNECTION_BROKEN    1
#define NXC_EVENT_NEW_ELOG_RECORD      2
#define NXC_EVENT_USER_DB_CHANGED      3
#define NXC_EVENT_OBJECT_CHANGED       4
#define NXC_EVENT_NOTIFICATION         5
#define NXC_EVENT_DEPLOYMENT_STATUS    6
#define NXC_EVENT_NEW_SYSLOG_RECORD    7
#define NXC_EVENT_NEW_SNMP_TRAP        8
#define NXC_EVENT_SITUATION_UPDATE     9
#define NXC_EVENT_JOB_CHANGE           10


//
// States (used both by client library and server)
//

#define STATE_DISCONNECTED    0
#define STATE_CONNECTING      1
#define STATE_CONNECTED       2
#define STATE_AUTHENTICATED   3

/**
 * Notification codes
 */
#define NX_NOTIFY_SHUTDOWN          1
#define NX_NOTIFY_EVENTDB_CHANGED   2
#define NX_NOTIFY_ALARM_DELETED     3
#define NX_NOTIFY_NEW_ALARM         4
#define NX_NOTIFY_ALARM_CHANGED     5
#define NX_NOTIFY_ACTION_CREATED    6
#define NX_NOTIFY_ACTION_MODIFIED   7
#define NX_NOTIFY_ACTION_DELETED    8
#define NX_NOTIFY_OBJTOOLS_CHANGED  9
#define NX_NOTIFY_DBCONN_STATUS     10
#define NX_NOTIFY_ALARM_TERMINATED  11
#define NX_NOTIFY_GRAPHS_CHANGED		12
#define NX_NOTIFY_ETMPL_CHANGED     13
#define NX_NOTIFY_ETMPL_DELETED     14
#define NX_NOTIFY_OBJTOOL_DELETED   15
#define NX_NOTIFY_TRAPCFG_CREATED   16
#define NX_NOTIFY_TRAPCFG_MODIFIED  17
#define NX_NOTIFY_TRAPCFG_DELETED   18
#define NX_NOTIFY_MAPTBL_CHANGED    19
#define NX_NOTIFY_MAPTBL_DELETED    20
#define NX_NOTIFY_DCISUMTBL_CHANGED 21
#define NX_NOTIFY_DCISUMTBL_DELETED 22

/**
 * Request completion codes
 */
#define RCC_SUCCESS                  ((UINT32)0)
#define RCC_COMPONENT_LOCKED         ((UINT32)1)
#define RCC_ACCESS_DENIED            ((UINT32)2)
#define RCC_INVALID_REQUEST          ((UINT32)3)
#define RCC_TIMEOUT                  ((UINT32)4)
#define RCC_OUT_OF_STATE_REQUEST     ((UINT32)5)
#define RCC_DB_FAILURE               ((UINT32)6)
#define RCC_INVALID_OBJECT_ID        ((UINT32)7)
#define RCC_ALREADY_EXIST            ((UINT32)8)
#define RCC_COMM_FAILURE             ((UINT32)9)
#define RCC_SYSTEM_FAILURE           ((UINT32)10)
#define RCC_INVALID_USER_ID          ((UINT32)11)
#define RCC_INVALID_ARGUMENT         ((UINT32)12)
#define RCC_DUPLICATE_DCI            ((UINT32)13)
#define RCC_INVALID_DCI_ID           ((UINT32)14)
#define RCC_OUT_OF_MEMORY            ((UINT32)15)
#define RCC_IO_ERROR                 ((UINT32)16)
#define RCC_INCOMPATIBLE_OPERATION   ((UINT32)17)
#define RCC_OBJECT_CREATION_FAILED   ((UINT32)18)
#define RCC_OBJECT_LOOP              ((UINT32)19)
#define RCC_INVALID_OBJECT_NAME      ((UINT32)20)
#define RCC_INVALID_ALARM_ID         ((UINT32)21)
#define RCC_INVALID_ACTION_ID        ((UINT32)22)
#define RCC_OPERATION_IN_PROGRESS    ((UINT32)23)
#define RCC_DCI_COPY_ERRORS          ((UINT32)24)
#define RCC_INVALID_EVENT_CODE       ((UINT32)25)
#define RCC_NO_WOL_INTERFACES        ((UINT32)26)
#define RCC_NO_MAC_ADDRESS           ((UINT32)27)
#define RCC_NOT_IMPLEMENTED          ((UINT32)28)
#define RCC_INVALID_TRAP_ID          ((UINT32)29)
#define RCC_DCI_NOT_SUPPORTED        ((UINT32)30)
#define RCC_VERSION_MISMATCH         ((UINT32)31)
#define RCC_NPI_PARSE_ERROR          ((UINT32)32)
#define RCC_DUPLICATE_PACKAGE        ((UINT32)33)
#define RCC_PACKAGE_FILE_EXIST       ((UINT32)34)
#define RCC_RESOURCE_BUSY            ((UINT32)35)
#define RCC_INVALID_PACKAGE_ID       ((UINT32)36)
#define RCC_INVALID_IP_ADDR          ((UINT32)37)
#define RCC_ACTION_IN_USE            ((UINT32)38)
#define RCC_VARIABLE_NOT_FOUND       ((UINT32)39)
#define RCC_BAD_PROTOCOL             ((UINT32)40)
#define RCC_ADDRESS_IN_USE           ((UINT32)41)
#define RCC_NO_CIPHERS               ((UINT32)42)
#define RCC_INVALID_PUBLIC_KEY       ((UINT32)43)
#define RCC_INVALID_SESSION_KEY      ((UINT32)44)
#define RCC_NO_ENCRYPTION_SUPPORT    ((UINT32)45)
#define RCC_INTERNAL_ERROR           ((UINT32)46)
#define RCC_EXEC_FAILED              ((UINT32)47)
#define RCC_INVALID_TOOL_ID          ((UINT32)48)
#define RCC_SNMP_ERROR               ((UINT32)49)
#define RCC_BAD_REGEXP               ((UINT32)50)
#define RCC_UNKNOWN_PARAMETER        ((UINT32)51)
#define RCC_FILE_IO_ERROR            ((UINT32)52)
#define RCC_CORRUPTED_MIB_FILE       ((UINT32)53)
#define RCC_TRANSFER_IN_PROGRESS     ((UINT32)54)
#define RCC_INVALID_JOB_ID           ((UINT32)55)
#define RCC_INVALID_SCRIPT_ID        ((UINT32)56)
#define RCC_INVALID_SCRIPT_NAME      ((UINT32)57)
#define RCC_UNKNOWN_MAP_NAME         ((UINT32)58)
#define RCC_INVALID_MAP_ID           ((UINT32)59)
#define RCC_ACCOUNT_DISABLED         ((UINT32)60)
#define RCC_NO_GRACE_LOGINS          ((UINT32)61)
#define RCC_CONNECTION_BROKEN        ((UINT32)62)
#define RCC_INVALID_CONFIG_ID        ((UINT32)63)
#define RCC_DB_CONNECTION_LOST       ((UINT32)64)
#define RCC_ALARM_OPEN_IN_HELPDESK   ((UINT32)65)
#define RCC_ALARM_NOT_OUTSTANDING    ((UINT32)66)
#define RCC_NOT_PUSH_DCI             ((UINT32)67)
#define RCC_CONFIG_PARSE_ERROR       ((UINT32)68)
#define RCC_CONFIG_VALIDATION_ERROR  ((UINT32)69)
#define RCC_INVALID_GRAPH_ID         ((UINT32)70)
#define RCC_LOCAL_CRYPTO_ERROR		 ((UINT32)71)
#define RCC_UNSUPPORTED_AUTH_TYPE	 ((UINT32)72)
#define RCC_BAD_CERTIFICATE			 ((UINT32)73)
#define RCC_INVALID_CERT_ID          ((UINT32)74)
#define RCC_SNMP_FAILURE             ((UINT32)75)
#define RCC_NO_L2_TOPOLOGY_SUPPORT	 ((UINT32)76)
#define RCC_INVALID_SITUATION_ID     ((UINT32)77)
#define RCC_INSTANCE_NOT_FOUND       ((UINT32)78)
#define RCC_INVALID_EVENT_ID         ((UINT32)79)
#define RCC_AGENT_ERROR              ((UINT32)80)
#define RCC_UNKNOWN_VARIABLE         ((UINT32)81)
#define RCC_RESOURCE_NOT_AVAILABLE   ((UINT32)82)
#define RCC_JOB_CANCEL_FAILED        ((UINT32)83)
#define RCC_INVALID_POLICY_ID        ((UINT32)84)
#define RCC_UNKNOWN_LOG_NAME         ((UINT32)85)
#define RCC_INVALID_LOG_HANDLE       ((UINT32)86)
#define RCC_WEAK_PASSWORD            ((UINT32)87)
#define RCC_REUSED_PASSWORD          ((UINT32)88)
#define RCC_INVALID_SESSION_HANDLE   ((UINT32)89)
#define RCC_CLUSTER_MEMBER_ALREADY   ((UINT32)90)
#define RCC_JOB_HOLD_FAILED          ((UINT32)91)
#define RCC_JOB_UNHOLD_FAILED        ((UINT32)92)
#define RCC_ZONE_ID_ALREADY_IN_USE   ((UINT32)93)
#define RCC_INVALID_ZONE_ID          ((UINT32)94)
#define RCC_ZONE_NOT_EMPTY           ((UINT32)95)
#define RCC_NO_COMPONENT_DATA        ((UINT32)96)
#define RCC_INVALID_ALARM_NOTE_ID    ((UINT32)97)
#define RCC_ENCRYPTION_ERROR         ((UINT32)98)
#define RCC_INVALID_MAPPING_TABLE_ID ((UINT32)99)
#define RCC_NO_SOFTWARE_PACKAGE_DATA ((UINT32)100)
#define RCC_INVALID_SUMMARY_TABLE_ID ((UINT32)101)
#define RCC_USER_LOGGED_IN           ((UINT32)102)
#define RCC_XML_PARSE_ERROR          ((UINT32)103)
#define RCC_HIGH_QUERY_COST          ((UINT32)104)
#define RCC_LICENSE_VIOLATION        ((UINT32)105)
#define RCC_CLIENT_LICENSE_EXCEEDED  ((UINT32)106)
#define RCC_OBJECT_ALREADY_EXISTS    ((UINT32)107)

/**
 * Mask bits for NXCModifyEventTemplate()
 */
#define EM_SEVERITY        ((UINT32)0x01)
#define EM_FLAGS           ((UINT32)0x02)
#define EM_NAME            ((UINT32)0x04)
#define EM_MESSAGE         ((UINT32)0x08)
#define EM_DESCRIPTION     ((UINT32)0x10)
#define EM_ALL             ((UINT32)0x1F)

/**
 * Mask bits (flags) for NXCModifyObject()
 */
#define OBJ_UPDATE_NAME             ((QWORD)_ULL(0x0000000001))
#define OBJ_UPDATE_AGENT_PORT       ((QWORD)_ULL(0x0000000002))
#define OBJ_UPDATE_AGENT_AUTH       ((QWORD)_ULL(0x0000000004))
#define OBJ_UPDATE_AGENT_SECRET     ((QWORD)_ULL(0x0000000008))
#define OBJ_UPDATE_SNMP_VERSION     ((QWORD)_ULL(0x0000000010))
#define OBJ_UPDATE_SNMP_AUTH        ((QWORD)_ULL(0x0000000020))
#define OBJ_UPDATE_ACL              ((QWORD)_ULL(0x0000000040))
#define OBJ_UPDATE_GEOLOCATION      ((QWORD)_ULL(0x0000000080))
#define OBJ_UPDATE_SYNC_NETS        ((QWORD)_ULL(0x0000000100))
#define OBJ_UPDATE_SERVICE_TYPE     ((QWORD)_ULL(0x0000000200))
#define OBJ_UPDATE_IP_PROTO         ((QWORD)_ULL(0x0000000400))
#define OBJ_UPDATE_IP_PORT          ((QWORD)_ULL(0x0000000800))
#define OBJ_UPDATE_CHECK_REQUEST    ((QWORD)_ULL(0x0000001000))
#define OBJ_UPDATE_CHECK_RESPONSE   ((QWORD)_ULL(0x0000002000))
#define OBJ_UPDATE_POLLER_NODE      ((QWORD)_ULL(0x0000004000))
#define OBJ_UPDATE_IP_ADDR          ((QWORD)_ULL(0x0000008000))
#define OBJ_UPDATE_PEER_GATEWAY     ((QWORD)_ULL(0x0000010000))
#define OBJ_UPDATE_NETWORK_LIST     ((QWORD)_ULL(0x0000020000))
#define OBJ_UPDATE_STATUS_ALG       ((QWORD)_ULL(0x0000040000))
#define OBJ_UPDATE_PROXY_NODE       ((QWORD)_ULL(0x0000080000))
#define OBJ_UPDATE_FLAGS            ((QWORD)_ULL(0x0000100000))
#define OBJ_UPDATE_ACT_EVENT        ((QWORD)_ULL(0x0000200000))
#define OBJ_UPDATE_DEACT_EVENT      ((QWORD)_ULL(0x0000400000))
#define OBJ_UPDATE_SOURCE_OBJECT    ((QWORD)_ULL(0x0000800000))
#define OBJ_UPDATE_ACTIVE_STATUS    ((QWORD)_ULL(0x0001000000))
#define OBJ_UPDATE_INACTIVE_STATUS  ((QWORD)_ULL(0x0002000000))
#define OBJ_UPDATE_DCI_LIST         ((QWORD)_ULL(0x0004000000))
#define OBJ_UPDATE_SCRIPT           ((QWORD)_ULL(0x0008000000))
#define OBJ_UPDATE_CLUSTER_TYPE		((QWORD)_ULL(0x0010000000))
#define OBJ_UPDATE_RESOURCES			((QWORD)_ULL(0x0020000000))
#define OBJ_UPDATE_SNMP_PROXY			((QWORD)_ULL(0x0040000000))
#define OBJ_UPDATE_REQUIRED_POLLS   ((QWORD)_ULL(0x0080000000))
#define OBJ_UPDATE_TRUSTED_NODES    ((QWORD)_ULL(0x0100000000))
#define OBJ_UPDATE_CUSTOM_ATTRS     ((QWORD)_ULL(0x0200000000))
#define OBJ_UPDATE_USE_IFXTABLE     ((QWORD)_ULL(0x0400000000))
#define OBJ_UPDATE_AUTOBIND         ((QWORD)_ULL(0x1000000000))
#define OBJ_UPDATE_SNMP_PORT        ((QWORD)_ULL(0x2000000000))
#define OBJ_UPDATE_PRIMARY_NAME     ((QWORD)_ULL(0x4000000000))


//
// Global user rights
//

#ifndef LIBNXCL_CUSTOM_USER_RIGHTS

#define SYSTEM_ACCESS_MANAGE_USERS        0x00000001
#define SYSTEM_ACCESS_SERVER_CONFIG       0x00000002
#define SYSTEM_ACCESS_CONFIGURE_TRAPS     0x00000004
#define SYSTEM_ACCESS_MANAGE_SESSIONS     0x00000008
#define SYSTEM_ACCESS_VIEW_EVENT_DB       0x00000010
#define SYSTEM_ACCESS_EDIT_EVENT_DB       0x00000020
#define SYSTEM_ACCESS_EPP                 0x00000040
#define SYSTEM_ACCESS_MANAGE_ACTIONS      0x00000080
#define SYSTEM_ACCESS_DELETE_ALARMS       0x00000100
#define SYSTEM_ACCESS_MANAGE_PACKAGES     0x00000200
#define SYSTEM_ACCESS_VIEW_EVENT_LOG      0x00000400
#define SYSTEM_ACCESS_MANAGE_TOOLS        0x00000800
#define SYSTEM_ACCESS_MANAGE_SCRIPTS      0x00001000
#define SYSTEM_ACCESS_VIEW_TRAP_LOG       0x00002000
#define SYSTEM_ACCESS_VIEW_AUDIT_LOG      0x00004000
#define SYSTEM_ACCESS_MANAGE_AGENT_CFG    0x00008000
#define SYSTEM_ACCESS_MANAGE_SITUATIONS   0x00010000
#define SYSTEM_ACCESS_SEND_SMS            0x00020000
#define SYSTEM_ACCESS_MOBILE_DEVICE_LOGIN 0x00040000
#define SYSTEM_ACCESS_REGISTER_AGENTS     0x00080000
#define SYSTEM_ACCESS_READ_FILES          0x00100000
#define SYSTEM_ACCESS_SERVER_CONSOLE      0x00200000
#define SYSTEM_ACCESS_MANAGE_FILES        0x00400000
#define SYSTEM_ACCESS_MANAGE_MAPPING_TBLS 0x00800000
#define SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS 0x01000000
#define SYSTEM_ACCESS_REPORTING_SERVER    0x02000000

#define SYSTEM_ACCESS_FULL                0x03FFFFFF

#endif	/* LIBNXCL_CUSTOM_USER_RIGHTS */


//
// Object access rights
//

#define OBJECT_ACCESS_READ          0x00000001
#define OBJECT_ACCESS_MODIFY        0x00000002
#define OBJECT_ACCESS_CREATE        0x00000004
#define OBJECT_ACCESS_DELETE        0x00000008
#define OBJECT_ACCESS_READ_ALARMS   0x00000010
#define OBJECT_ACCESS_ACL           0x00000020
#define OBJECT_ACCESS_ACK_ALARMS    0x00000040
#define OBJECT_ACCESS_SEND_EVENTS   0x00000080
#define OBJECT_ACCESS_CONTROL       0x00000100
#define OBJECT_ACCESS_TERM_ALARMS   0x00000200
#define OBJECT_ACCESS_PUSH_DATA     0x00000400


//
// Object sync flags
//

#define OBJECT_SYNC_SEND_UPDATES    0x0001
#define OBJECT_SYNC_DUAL_CONFIRM    0x0002


//
// User/group flags
//

#define UF_MODIFIED                 0x0001
#define UF_DELETED                  0x0002
#define UF_DISABLED                 0x0004
#define UF_CHANGE_PASSWORD          0x0008
#define UF_CANNOT_CHANGE_PASSWORD   0x0010
#define UF_INTRUDER_LOCKOUT         0x0020
#define UF_PASSWORD_NEVER_EXPIRES   0x0040


//
// Fields for NXCModifyUserEx
//

#define USER_MODIFY_LOGIN_NAME         0x00000001
#define USER_MODIFY_DESCRIPTION        0x00000002
#define USER_MODIFY_FULL_NAME          0x00000004
#define USER_MODIFY_FLAGS              0x00000008
#define USER_MODIFY_ACCESS_RIGHTS      0x00000010
#define USER_MODIFY_MEMBERS            0x00000020
#define USER_MODIFY_CERT_MAPPING       0x00000040
#define USER_MODIFY_AUTH_METHOD        0x00000080
#define USER_MODIFY_PASSWD_LENGTH      0x00000100
#define USER_MODIFY_TEMP_DISABLE       0x00000200
#define USER_MODIFY_CUSTOM_ATTRIBUTES  0x00000400


//
// User certificate mapping methods
//

#define USER_MAP_CERT_BY_SUBJECT		0
#define USER_MAP_CERT_BY_PUBKEY		1


//
// User database change notification types
//

#define USER_DB_CREATE              0
#define USER_DB_DELETE              1
#define USER_DB_MODIFY              2

/**
 * Situation change notification types
 */
#define SITUATION_CREATE            1
#define SITUATION_DELETE            2
#define SITUATION_UPDATE            3
#define SITUATION_INSTANCE_UPDATE   4
#define SITUATION_INSTANCE_DELETE   5

/**
 * Report rendering formats
 */
#define REPORT_FORMAT_PDF           1
#define REPORT_FORMAT_HTML          2

/**
 * Data collection object types
 */
#define DCO_TYPE_GENERIC   0
#define DCO_TYPE_ITEM      1
#define DCO_TYPE_TABLE     2

/**
 * DCI flags
 */
#define DCF_ADVANCED_SCHEDULE       ((UINT16)0x0001)
#define DCF_ALL_THRESHOLDS          ((UINT16)0x0002)
#define DCF_RAW_VALUE_OCTET_STRING  ((UINT16)0x0004)
#define DCF_SHOW_ON_OBJECT_TOOLTIP  ((UINT16)0x0008)
#define DCF_AGGREGATE_FUNCTION_MASK ((UINT16)0x0070)
#define DCF_AGGREGATE_ON_CLUSTER    ((UINT16)0x0080)
#define DCF_TRANSFORM_AGGREGATED    ((UINT16)0x0100)

/**
 * Get cluster aggregation function from DCI flags
 */
#define DCF_GET_AGGREGATION_FUNCTION(flags) (((flags) & DCF_AGGREGATE_FUNCTION_MASK) >> 4)
#define DCF_SET_AGGREGATION_FUNCTION(flags,func) (((flags) & ~DCF_AGGREGATE_FUNCTION_MASK) | (((func) & 0x07) << 4))

/**
 * DCTable column flags
 */
#define TCF_DATA_TYPE_MASK          ((UINT16)0x000F)
#define TCF_AGGREGATE_FUNCTION_MASK ((UINT16)0x0070)
#define TCF_INSTANCE_COLUMN         ((UINT16)0x0100)
#define TCF_INSTANCE_LABEL_COLUMN   ((UINT16)0x0200)

/**
 * Get cluster aggregation function from column flags
 */
#define TCF_GET_AGGREGATION_FUNCTION(flags) (((flags) & TCF_AGGREGATE_FUNCTION_MASK) >> 4)
#define TCF_SET_AGGREGATION_FUNCTION(flags,func) (((flags) & ~DCF_AGGREGATE_FUNCTION_MASK) | (((func) & 0x07) << 4))

/**
 * Get data type from column flags
 */
#define TCF_GET_DATA_TYPE(flags) ((flags) & TCF_DATA_TYPE_MASK)
#define TCF_SET_DATA_TYPE(flags,dt) (((flags) & ~TCF_DATA_TYPE_MASK) | ((dt) & 0x0F))

/**
 * Aggregation functions
 */
#define DCF_FUNCTION_SUM 0
#define DCF_FUNCTION_AVG 1
#define DCF_FUNCTION_MIN 2
#define DCF_FUNCTION_MAX 3

/**
 * SNMP raw types
 */
#define SNMP_RAWTYPE_NONE           0
#define SNMP_RAWTYPE_INT32          1
#define SNMP_RAWTYPE_UINT32         2
#define SNMP_RAWTYPE_INT64          3
#define SNMP_RAWTYPE_UINT64         4
#define SNMP_RAWTYPE_DOUBLE         5
#define SNMP_RAWTYPE_IP_ADDR        6
#define SNMP_RAWTYPE_MAC_ADDR       7

/**
 * Data sources
 */
#define DS_INTERNAL           0
#define DS_NATIVE_AGENT       1
#define DS_SNMP_AGENT         2
#define DS_CHECKPOINT_AGENT   3
#define DS_PUSH_AGENT         4
#define DS_WINPERF            5
#define DS_SMCLP              6

/**
 * Item status
 */
#define ITEM_STATUS_ACTIVE          0
#define ITEM_STATUS_DISABLED        1
#define ITEM_STATUS_NOT_SUPPORTED   2

/**
 * Delta calculation methods for DCIs
 */
#define DCM_ORIGINAL_VALUE       0
#define DCM_SIMPLE               1
#define DCM_AVERAGE_PER_SECOND   2
#define DCM_AVERAGE_PER_MINUTE   3

/**
 * Threshold functions
 */
#define F_LAST       0
#define F_AVERAGE    1
#define F_DEVIATION  2
#define F_DIFF       3
#define F_ERROR      4
#define F_SUM        5

/**
 * Threshold operations
 */
#define OP_LE        0
#define OP_LE_EQ     1
#define OP_EQ        2
#define OP_GT_EQ     3
#define OP_GT        4
#define OP_NE        5
#define OP_LIKE      6
#define OP_NOTLIKE   7

/**
 * DCI base units
 */
#define DCI_BASEUNITS_OTHER             0
#define DCI_BASEUNITS_CUSTOM            1
#define DCI_BASEUNITS_BYTES             2
#define DCI_BASEUNITS_BITS              3
#define DCI_BASEUNITS_SECONDS           4
#define DCI_BASEUNITS_PERCENTS          5
#define DCI_BASEUNITS_BITS_PER_SECOND   6
#define DCI_BASEUNITS_BYTES_PER_SECOND  7

/**
 * DCI instance discovery methods
 */
#define IDM_NONE                        0
#define IDM_AGENT_LIST                  1
#define IDM_AGENT_TABLE                 2
#define IDM_SNMP_WALK_VALUES            3
#define IDM_SNMP_WALK_OIDS              4

/**
 * Event policy rule flags
 */
#define RF_STOP_PROCESSING       0x0001
#define RF_NEGATED_SOURCE        0x0002
#define RF_NEGATED_EVENTS        0x0004
#define RF_GENERATE_ALARM        0x0008
#define RF_DISABLED              0x0010
#define RF_TERMINATE_BY_REGEXP   0x0020
#define RF_SEVERITY_INFO         0x0100
#define RF_SEVERITY_WARNING      0x0200
#define RF_SEVERITY_MINOR        0x0400
#define RF_SEVERITY_MAJOR        0x0800
#define RF_SEVERITY_CRITICAL     0x1000


//
// Action types
//

#define ACTION_EXEC           0
#define ACTION_REMOTE         1
#define ACTION_SEND_EMAIL     2
#define ACTION_SEND_SMS       3
#define ACTION_FORWARD_EVENT  4
#define ACTION_NXSL_SCRIPT    5

/**
 * Network map types
 */
#define NETMAP_USER_DEFINED   0
#define NETMAP_IP_TOPOLOGY    1
#define NETMAP_L2_TOPOLOGY    2

/**
 * Network map flags
 */
#define MF_SHOW_STATUS_ICON      0x00000001
#define MF_SHOW_STATUS_FRAME     0x00000002
#define MF_SHOW_STATUS_BKGND     0x00000004
#define MF_SHOW_END_NODES        0x00000008
#define MF_CALCULATE_STATUS      0x00000010

/**
 * Network map layouts
 */
#define MAP_LAYOUT_MANUAL        0x7FFF
#define MAP_LAYOUT_SPRING        0
#define MAP_LAYOUT_RADIAL        1
#define MAP_LAYOUT_HTREE         2
#define MAP_LAYOUT_VTREE		   3
#define MAP_LAYOUT_SPARSE_VTREE  4


//
// Deployment manager status codes
//

#define DEPLOYMENT_STATUS_PENDING      0
#define DEPLOYMENT_STATUS_TRANSFER     1
#define DEPLOYMENT_STATUS_INSTALLATION 2
#define DEPLOYMENT_STATUS_COMPLETED    3
#define DEPLOYMENT_STATUS_FAILED       4
#define DEPLOYMENT_STATUS_INITIALIZE   5
#define DEPLOYMENT_STATUS_FINISHED     255

/**
 * Session subscription codes (data channels)
 */
#define NXC_CHANNEL_EVENTS       0x00000001
#define NXC_CHANNEL_SYSLOG       0x00000002
#define NXC_CHANNEL_ALARMS       0x00000004
#define NXC_CHANNEL_OBJECTS      0x00000008
#define NXC_CHANNEL_SNMP_TRAPS   0x00000010
#define NXC_CHANNEL_AUDIT_LOG    0x00000020
#define NXC_CHANNEL_SITUATIONS   0x00000040


//
// Node creation flags
//

#define NXC_NCF_DISABLE_ICMP     0x0001
#define NXC_NCF_DISABLE_NXCP     0x0002
#define NXC_NCF_DISABLE_SNMP     0x0004
#define NXC_NCF_CREATE_UNMANAGED 0x0008


//
// Server components
//

#define SRV_COMPONENT_DISCOVERY_MGR    1


//
// Configuration import flags
//

#define CFG_IMPORT_REPLACE_EVENT_BY_CODE   0x0001
#define CFG_IMPORT_REPLACE_EVENT_BY_NAME   0x0002


//
// Graph access flags
//

#define NXGRAPH_ACCESS_READ		0x01
#define NXGRAPH_ACCESS_WRITE		0x02


//
// Cluster types
//

#define CLUSTER_TYPE_GENERIC			0


//
// SNMP trap flags
//

#define TRAP_VARBIND_FORCE_TEXT     0x0001

/**
 * IP network
 */
typedef struct
{
   UINT32 dwAddr;
   UINT32 dwMask;
} IP_NETWORK;

/**
 * Agent's parameter information
 */
typedef struct
{
   TCHAR szName[MAX_PARAM_NAME];
   TCHAR szDescription[MAX_DB_STRING];
   int iDataType;
} NXC_AGENT_PARAM;

/**
 * Server action definition structure
 */
typedef struct
{
   UINT32 dwId;
   int iType;
   BOOL bIsDisabled;
   TCHAR szName[MAX_OBJECT_NAME];
   TCHAR szRcptAddr[MAX_RCPT_ADDR_LEN];
   TCHAR szEmailSubject[MAX_EMAIL_SUBJECT_LEN];
   TCHAR *pszData;
} NXC_ACTION;

/**
 * Alarm structure
 */
typedef struct
{
   QWORD qwSourceEventId;  // Originating event ID
   UINT32 dwAlarmId;        // Unique alarm ID
   UINT32 dwCreationTime;   // Alarm creation time in UNIX time format
   UINT32 dwLastChangeTime; // Alarm's last change time in UNIX time format
   UINT32 dwSourceObject;   // Source object ID
   UINT32 dwSourceEventCode;// Originating event code
   BYTE nCurrentSeverity;  // Alarm's current severity
   BYTE nOriginalSeverity; // Alarm's original severity
   BYTE nState;            // Current state
   BYTE nHelpDeskState;    // State of alarm in helpdesk system
   UINT32 dwAckByUser;      // ID of user who was acknowledged this alarm (0 for system)
	UINT32 dwResolvedByUser; // ID of user who was resolved this alarm (0 for system)
   UINT32 dwTermByUser;     // ID of user who was terminated this alarm (0 for system)
   UINT32 dwRepeatCount;
	UINT32 dwTimeout;
	UINT32 dwTimeoutEvent;
   TCHAR szMessage[MAX_EVENT_MSG_LENGTH];
   TCHAR szKey[MAX_DB_STRING];
   TCHAR szHelpDeskRef[MAX_HELPDESK_REF_LEN];
   void *pUserData;        // Can be freely used by client application
	UINT32 noteCount;        // Number of notes added to alarm
} NXC_ALARM;

/**
 * Trap parameter mapping entry
 */
typedef struct
{
   UINT32 *pdwObjectId;     // Trap OID
   UINT32 dwOidLen;         // Trap OID length (if highest bit is set, then it's a position)
	UINT32 dwFlags;
   TCHAR szDescription[MAX_DB_STRING];
} NXC_OID_MAP;

/**
 * Trap configuration entry
 */
typedef struct
{
   UINT32 dwId;             // Entry ID
   UINT32 *pdwObjectId;     // Trap OID
   UINT32 dwOidLen;         // Trap OID length
   UINT32 dwEventCode;      // Event code
   UINT32 dwNumMaps;        // Number of parameter mappings
   NXC_OID_MAP *pMaps;
   TCHAR szDescription[MAX_DB_STRING];
	TCHAR szUserTag[MAX_USERTAG_LENGTH];
} NXC_TRAP_CFG_ENTRY;

/**
 * Condition's input DCI definition
 */
typedef struct
{
   UINT32 id;
   UINT32 nodeId;
   int function;    // Average, last, diff
   int polls;       // Number of polls used for average
} INPUT_DCI;

/**
 * Cluster resource
 */
typedef struct
{
	UINT32 dwId;
	TCHAR szName[MAX_DB_STRING];
	UINT32 dwIpAddr;
	UINT32 dwCurrOwner;
} CLUSTER_RESOURCE;


/********************************************************************
 * Following part of this file shouldn't be included by server code *
 ********************************************************************/

#ifndef LIBNXCL_NO_DECLARATIONS


//
// NXCConnect flags
//

#define NXCF_DEFAULT						   0
#define NXCF_ENCRYPT						   0x0001
#define NXCF_EXACT_VERSION_MATCH		   0x0002
#define NXCF_USE_CERTIFICATE			   0x0004
#define NXCF_IGNORE_PROTOCOL_VERSION   0x0008


//
// Node dynamic flags useful for client
//

#ifndef __NDF_FLAGS_DEFINED
#define NDF_UNREACHABLE                0x0004
#define NDF_AGENT_UNREACHABLE          0x0008
#define NDF_SNMP_UNREACHABLE           0x0010
#define NDF_CPSNMP_UNREACHABLE         0x0200
#define NDF_POLLING_DISABLED           0x0800
#endif


//
// Callbacks data types
//

typedef void (* NXC_EVENT_HANDLER)(NXC_SESSION hSession, UINT32 dwEvent, UINT32 dwCode, void *pArg);
typedef void (* NXC_DEBUG_CALLBACK)(TCHAR *pMsg);

/**
 * Event log record structure
 */
typedef struct
{
   QWORD qwEventId;
   UINT32 dwTimeStamp;
   UINT32 dwEventCode;
   UINT32 dwSourceId;
   UINT32 dwSeverity;
   TCHAR szMessage[MAX_EVENT_MSG_LENGTH];
	TCHAR szUserTag[MAX_USERTAG_LENGTH];
} NXC_EVENT;

/**
 * Syslog record structure
 */
typedef struct
{
   QWORD qwMsgId;
   UINT32 dwTimeStamp;
   WORD wFacility;
   WORD wSeverity;
   UINT32 dwSourceObject;
   TCHAR szHost[MAX_SYSLOG_HOSTNAME_LEN];
   TCHAR szTag[MAX_SYSLOG_TAG_LEN];
   TCHAR *pszText;
} NXC_SYSLOG_RECORD;

/**
 * Server's configuration variable
 */
typedef struct
{
   TCHAR szName[MAX_OBJECT_NAME];
   TCHAR szValue[MAX_DB_STRING];
   BOOL bNeedRestart;
} NXC_SERVER_VARIABLE;

/**
 * Agent package information
 */
typedef struct
{
   UINT32 dwId;
   TCHAR szName[MAX_PACKAGE_NAME_LEN];
   TCHAR szVersion[MAX_AGENT_VERSION_LEN];
   TCHAR szPlatform[MAX_PLATFORM_NAME_LEN];
   TCHAR szFileName[MAX_DB_STRING];
   TCHAR szDescription[MAX_DB_STRING];
} NXC_PACKAGE_INFO;

/**
 * Event configuration structure
 */
typedef struct
{
   UINT32 dwCode;
   UINT32 dwSeverity;
   UINT32 dwFlags;
   TCHAR szName[MAX_EVENT_NAME];
   TCHAR *pszMessage;
   TCHAR *pszDescription;
} NXC_EVENT_TEMPLATE;


//
// Entry in object's ACL
//

typedef struct
{
   UINT32 dwUserId;
   UINT32 dwAccessRights;
} NXC_ACL_ENTRY;


//
// Network object structure
//

struct __nxc_object_iface
{
	UINT32 dwFlags;
   UINT32 dwIpNetMask;   // Ip netmask.
   UINT32 dwIfIndex;     // Interface index.
   UINT32 dwIfType;      // Interface type
	UINT32 dwSlot;			// Slot number
	UINT32 dwPort;			// Port number
   BYTE bMacAddr[MAC_ADDR_LENGTH];
	WORD wRequiredPollCount;
	BYTE adminState;
	BYTE operState;
};

struct __nxc_object_node
{
	TCHAR szPrimaryName[MAX_DNS_NAME];
   UINT32 dwFlags;
	UINT32 dwRuntimeFlags;
   UINT32 dwNodeType;
   UINT32 dwPollerNode;
   UINT32 dwProxyNode;
	UINT32 dwSNMPProxy;
   UINT32 dwZoneId;
   TCHAR szSharedSecret[MAX_SECRET_LENGTH];
   TCHAR *pszAuthName;     // SNMP authentication name
	TCHAR *pszAuthPassword; // SNMP v3 USM auth password
	TCHAR *pszPrivPassword; // SNMP v3 USM privacy password
   TCHAR *pszSnmpObjectId;
	WORD wSnmpAuthMethod;
	WORD wSnmpPrivMethod;
	WORD wSnmpPort;
   WORD wAgentPort;     // Listening TCP port for native agent
   WORD wAuthMethod;    // Native agent's authentication method
   TCHAR szAgentVersion[MAX_AGENT_VERSION_LEN];
   TCHAR szPlatformName[MAX_PLATFORM_NAME_LEN];
	BYTE nUseIfXTable;
   BYTE nSNMPVersion;
	WORD wRequiredPollCount;
	TCHAR szSysDescription[MAX_DB_STRING];
	BYTE bridgeBaseAddress[MAC_ADDR_LENGTH];
};

struct __nxc_object_subnet
{
   UINT32 dwIpNetMask;
   UINT32 dwZoneId;
};

struct __nxc_object_container
{
   UINT32 dwCategory;
	UINT32 dwFlags;
	TCHAR *pszAutoBindFilter;
};

struct __nxc_object_dct
{
   UINT32 dwVersion;
	UINT32 dwFlags;
	TCHAR *pszAutoApplyFilter;
};

struct __nxc_object_netsrv
{
   int iServiceType;
   WORD wProto;
   WORD wPort;
   UINT32 dwPollerNode;
   TCHAR *pszRequest;
   TCHAR *pszResponse;
	WORD wRequiredPollCount;
};

struct __nxc_object_zone
{
   UINT32 dwZoneId;
   UINT32 dwAgentProxy;
   UINT32 dwSnmpProxy;
   UINT32 dwIcmpProxy;
};

struct __nxc_object_vpnc
{
   UINT32 dwPeerGateway;
   UINT32 dwNumLocalNets;
   UINT32 dwNumRemoteNets;
   IP_NETWORK *pLocalNetList;
   IP_NETWORK *pRemoteNetList;
};

struct __nxc_object_cond
{
   TCHAR *pszScript;
   UINT32 dwActivationEvent;
   UINT32 dwDeactivationEvent;
   UINT32 dwSourceObject;
   WORD wActiveStatus;
   WORD wInactiveStatus;
   UINT32 dwNumDCI;
   INPUT_DCI *pDCIList;
};

struct __nxc_object_cluster
{
	UINT32 dwZoneId;
	UINT32 dwClusterType;
	UINT32 dwNumSyncNets;
	IP_NETWORK *pSyncNetList;
	UINT32 dwNumResources;
	CLUSTER_RESOURCE *pResourceList;
};

struct __nxc_geolocation
{
	int type;
	double latitude;
	double longitude;
};

typedef struct
{
   UINT32 dwId;          // Unique object's identifier
   int iClass;          // Object's class
   int iStatus;         // Object's status
   UINT32 dwIpAddr;      // IP address
   TCHAR szName[MAX_OBJECT_NAME];
   UINT32 dwNumParents;
   UINT32 *pdwParentList;
   UINT32 dwNumChilds;
   UINT32 *pdwChildList;
   BOOL bIsDeleted;     // TRUE for deleted objects
   BOOL bInheritRights;
   UINT32 dwAclSize;
   NXC_ACL_ENTRY *pAccessList;
   int iStatusCalcAlg;  // Status calculation algorithm
   int iStatusPropAlg;  // Status propagation alhorithm
   int iFixedStatus;    // Status to be propagated if alg = SA_PROPAGATE_FIXED
   int iStatusShift;    // Status shift for SA_PROPAGATE_RELATIVE
   int iStatusTrans[4];
   int iStatusSingleTh;
   int iStatusThresholds[4];
   TCHAR *pszComments;
	UINT32 dwNumTrustedNodes;
	UINT32 *pdwTrustedNodes;
	struct __nxc_geolocation geolocation;
#ifdef __cplusplus
	StringMap *pCustomAttrs;
#else
	void *pCustomAttrs;
#endif
   union
   {
      struct __nxc_object_iface iface;
      struct __nxc_object_node node;
      struct __nxc_object_subnet subnet;
      struct __nxc_object_container container;
      struct __nxc_object_dct dct;
      struct __nxc_object_netsrv netsrv;
      struct __nxc_object_zone zone;
      struct __nxc_object_vpnc vpnc;  // VPN connector
      struct __nxc_object_cond cond;  // Condition
		struct __nxc_object_cluster cluster;
   };
} NXC_OBJECT;


//
// Structure with data for NXCModifyObject
//

typedef struct
{
   QWORD qwFlags;
   UINT32 dwObjectId;
   const TCHAR *pszName;
	const TCHAR *pszPrimaryName;
   int iAgentPort;
   int iAuthType;
   const TCHAR *pszSecret;
   const TCHAR *pszAuthName;
   const TCHAR *pszAuthPassword;
   const TCHAR *pszPrivPassword;
	WORD wSnmpAuthMethod;
	WORD wSnmpPrivMethod;
   BOOL bInheritRights;
   UINT32 dwAclSize;
   NXC_ACL_ENTRY *pAccessList;
   WORD wSNMPVersion;
   int iServiceType;
   WORD wProto;
   WORD wPort;
	WORD wSnmpPort;
   UINT32 dwPollerNode;
   const TCHAR *pszRequest;
   const TCHAR *pszResponse;
   UINT32 dwIpAddr;
   UINT32 dwPeerGateway;
   UINT32 dwNumLocalNets;
   UINT32 dwNumRemoteNets;
   IP_NETWORK *pLocalNetList;
   IP_NETWORK *pRemoteNetList;
   int iStatusCalcAlg;  // Status calculation algorithm
   int iStatusPropAlg;  // Status propagation alhorithm
   int iFixedStatus;    // Status to be propagated if alg = SA_PROPAGATE_FIXED
   int iStatusShift;    // Status shift for SA_PROPAGATE_RELATIVE
   int iStatusTrans[4];
   int iStatusSingleTh;
   int iStatusThresholds[4];
   UINT32 dwProxyNode;
	UINT32 dwSNMPProxy;
   UINT32 dwObjectFlags;
   UINT32 dwActivationEvent;
   UINT32 dwDeactivationEvent;
   UINT32 dwSourceObject;
   int nActiveStatus;
   int nInactiveStatus;
   UINT32 dwNumDCI;
   INPUT_DCI *pDCIList;
   const TCHAR *pszScript;
	UINT32 dwClusterType;
	UINT32 dwNumSyncNets;
	IP_NETWORK *pSyncNetList;
	UINT32 dwNumResources;
	CLUSTER_RESOURCE *pResourceList;
	WORD wRequiredPollCount;
	UINT32 dwNumTrustedNodes;
	UINT32 *pdwTrustedNodes;
	int nUseIfXTable;
	const TCHAR *pszAutoBindFilter;
	struct __nxc_geolocation geolocation;
#ifdef __cplusplus
	StringMap *pCustomAttrs;
#else
	void *pCustomAttrs;
#endif
} NXC_OBJECT_UPDATE;


//
// NetXMS user information structure
//

typedef struct
{
   TCHAR szName[MAX_USER_NAME];
   uuid_t guid;
   UINT32 dwId;
   UINT32 dwSystemRights;
   WORD wFlags;
   WORD nAuthMethod;        // Only for users
   UINT32 dwNumMembers;     // Only for groups
   UINT32 *pdwMemberList;   // Only for groups
   TCHAR szFullName[MAX_USER_FULLNAME];    // Only for users
   TCHAR szDescription[MAX_USER_DESCR];
	int nCertMappingMethod;	// Only for users
	TCHAR *pszCertMappingData;	// Only for users
} NXC_USER;


//
// Data collection item threshold structure
//

typedef struct
{
   UINT32 dwId;
   UINT32 dwEvent;
   UINT32 dwRearmEvent;
   WORD wFunction;
   WORD wOperation;
   UINT32 dwArg1;
   UINT32 dwArg2;
	LONG nRepeatInterval;
   TCHAR szValue[MAX_STRING_VALUE];
} NXC_DCI_THRESHOLD;


//
// Data collection item structure
//

typedef struct
{
   UINT32 dwId;
   UINT32 dwTemplateId;
	UINT32 dwResourceId;
   TCHAR szName[MAX_ITEM_NAME];
   TCHAR szDescription[MAX_DB_STRING];
   TCHAR szInstance[MAX_DB_STRING];
	TCHAR szSystemTag[MAX_DB_STRING];
   int iPollingInterval;
   int iRetentionTime;
   BYTE iSource;
   BYTE iDataType;
   BYTE iStatus;
   BYTE iDeltaCalculation;
	WORD wFlags;
   UINT32 dwNumThresholds;
   NXC_DCI_THRESHOLD *pThresholdList;
   TCHAR *pszFormula;
   UINT32 dwNumSchedules;
   TCHAR **ppScheduleList;
	UINT32 dwProxyNode;
	int nBaseUnits;
	int nMultiplier;
	TCHAR *pszCustomUnitName;
	TCHAR *pszPerfTabSettings;
	WORD nSnmpPort;
	WORD wSnmpRawType;
} NXC_DCI;


//
// Node's data collection list information
//

typedef struct
{
   UINT32 dwNodeId;
   UINT32 dwNumItems;
   NXC_DCI *pItems;
} NXC_DCI_LIST;


//
// Row structure
//

#pragma pack(1)

typedef struct
{
   UINT32 dwTimeStamp;
   union
   {
      UINT32 dwInt32;
      TCHAR szString[MAX_STRING_VALUE];
      struct
      {
         INT32 padding;
         union
         {
            INT64 qwInt64;
            double dFloat;
         } v64;
      } ext;
   } value;
} NXC_DCI_ROW;

#pragma pack()


//
// DCI's collected data
//

typedef struct
{
   UINT32 dwNodeId;      // Owner's node id
   UINT32 dwItemId;      // Item id
   UINT32 dwNumRows;     // Number of rows in set
   WORD wDataType;      // Data type (integer, string, etc.)
   WORD wRowSize;       // Size of single row in bytes
   NXC_DCI_ROW *pRows;  // Array of rows
} NXC_DCI_DATA;


//
// MIB file list
//

typedef struct
{
   UINT32 dwNumFiles;
   TCHAR **ppszName;
   BYTE **ppHash;
} NXC_MIB_LIST;


//
// Event processing policy rule
//

typedef struct
{
   UINT32 dwFlags;
   UINT32 dwId;
   UINT32 dwNumActions;
   UINT32 dwNumEvents;
   UINT32 dwNumSources;
   UINT32 *pdwActionList;
   UINT32 *pdwEventList;
   UINT32 *pdwSourceList;
   TCHAR *pszComment;
   TCHAR *pszScript;
   TCHAR szAlarmKey[MAX_DB_STRING];
   TCHAR szAlarmMessage[MAX_DB_STRING];
   WORD wAlarmSeverity;
	UINT32 dwAlarmTimeout;
	UINT32 dwAlarmTimeoutEvent;
	UINT32 dwSituationId;
	TCHAR szSituationInstance[MAX_DB_STRING];
#ifdef __cplusplus
	StringMap *pSituationAttrList;
#else
	void *pSituationAttrList;
#endif
} NXC_EPP_RULE;


//
// Event processing policy
//

typedef struct
{
   UINT32 dwNumRules;
   NXC_EPP_RULE *pRuleList;
} NXC_EPP;


//
// Package deployment status information
//

typedef struct
{
   UINT32 dwNodeId;
   UINT32 dwStatus;
   TCHAR *pszErrorMessage;
} NXC_DEPLOYMENT_STATUS;


//
// Object creation information structure
//

typedef struct
{
   int iClass;
   UINT32 dwParentId;
   TCHAR *pszName;
	TCHAR *pszComments;
   union
   {
      struct
      {
			const TCHAR *pszPrimaryName;
         UINT32 dwIpAddr;
         UINT32 dwNetMask;
         UINT32 dwCreationFlags;
         UINT32 dwProxyNode;
			UINT32 dwSNMPProxy;
      } node;
      struct
      {
         UINT32 dwCategory;
      } container;
      struct
      {
         int iServiceType;
         WORD wProto;
         WORD wPort;
         TCHAR *pszRequest;
         TCHAR *pszResponse;
			bool createStatusDci;
      } netsrv;
   } cs;
} NXC_OBJECT_CREATE_INFO;


//
// Container category
//

typedef struct
{
   UINT32 dwId;
   TCHAR szName[MAX_OBJECT_NAME];
   TCHAR *pszDescription;
} NXC_CONTAINER_CATEGORY;


//
// Container categories list
//

typedef struct
{
   UINT32 dwNumElements;
   NXC_CONTAINER_CATEGORY *pElements;
} NXC_CC_LIST;


//
// DCI information and last value for NXCGetLastValues()
//

typedef struct
{
   UINT32 dwId;
   UINT32 dwTimestamp;
   TCHAR szName[MAX_ITEM_NAME];
   TCHAR szDescription[MAX_DB_STRING];
   TCHAR szValue[MAX_DB_STRING];
   BYTE nDataType;
   BYTE nSource;
	BYTE nStatus;
} NXC_DCI_VALUE;


//
// Object tool
//

typedef struct
{
   UINT32 dwId;
   UINT32 dwFlags;
   WORD wType;
   TCHAR szName[MAX_DB_STRING];
   TCHAR szDescription[MAX_DB_STRING];
   TCHAR *pszMatchingOID;
   TCHAR *pszData;
   TCHAR *pszConfirmationText;
} NXC_OBJECT_TOOL;


//
// Object tool column information
//

typedef struct
{
   TCHAR szName[MAX_DB_STRING];
   TCHAR szOID[MAX_DB_STRING];
   int nFormat;
   int nSubstr;
} NXC_OBJECT_TOOL_COLUMN;


//
// Object tool detailed information
//

typedef struct
{
   UINT32 dwId;
   UINT32 dwFlags;
   TCHAR szName[MAX_DB_STRING];
   TCHAR szDescription[MAX_DB_STRING];
   TCHAR *pszMatchingOID;
   TCHAR *pszData;
   TCHAR *pszConfirmationText;
   UINT32 *pdwACL;
   UINT32 dwACLSize;
   WORD wType;
   WORD wNumColumns;
   NXC_OBJECT_TOOL_COLUMN *pColList;
} NXC_OBJECT_TOOL_DETAILS;


//
// Server stats
//

typedef struct
{
	UINT32 dwNumAlarms;
	UINT32 dwAlarmsBySeverity[5];
	UINT32 dwNumObjects;
	UINT32 dwNumNodes;
	UINT32 dwNumDCI;
	UINT32 dwNumClientSessions;
	UINT32 dwServerUptime;
	UINT32 dwServerProcessVMSize;
	UINT32 dwServerProcessWorkSet;
	TCHAR szServerVersion[MAX_DB_STRING];
	UINT32 dwQSizeConditionPoller;
	UINT32 dwQSizeConfPoller;
	UINT32 dwQSizeDCIPoller;
	UINT32 dwQSizeDBWriter;
	UINT32 dwQSizeEvents;
	UINT32 dwQSizeDiscovery;
	UINT32 dwQSizeNodePoller;
	UINT32 dwQSizeRoutePoller;
	UINT32 dwQSizeStatusPoller;
} NXC_SERVER_STATS;


//
// Script info
//

typedef struct
{
   UINT32 dwId;
   TCHAR szName[MAX_DB_STRING];
} NXC_SCRIPT_INFO;


//
// Client session information
//

typedef struct
{
   UINT32 dwSessionId;
   int nCipher;
   TCHAR szUserName[MAX_USER_NAME];
   TCHAR szClientApp[MAX_DB_STRING];
} NXC_CLIENT_SESSION_INFO;


//
// Trap log records
//

typedef struct
{
   QWORD qwId;
   UINT32 dwTimeStamp;
   UINT32 dwIpAddr;
   UINT32 dwObjectId;
   TCHAR szTrapOID[MAX_DB_STRING];
   TCHAR *pszTrapVarbinds;
} NXC_SNMP_TRAP_LOG_RECORD;


//
// Agent configuration info
//

typedef struct
{
   UINT32 dwId;
   UINT32 dwSequence;
   TCHAR szName[MAX_DB_STRING];
} NXC_AGENT_CONFIG_INFO;


//
// Agent configuration - all data
//

typedef struct
{
   UINT32 dwId;
   UINT32 dwSequence;
   TCHAR szName[MAX_DB_STRING];
   TCHAR *pszText;
   TCHAR *pszFilter;
} NXC_AGENT_CONFIG;


//
// DCI push data
//

typedef struct
{
   UINT32 dwId;          // DCI ID or 0 if name is used
   TCHAR *pszName;
   UINT32 dwNodeId;      // Node ID or 0 if name is used
   TCHAR *pszNodeName;
   TCHAR *pszValue;
} NXC_DCI_PUSH_DATA;


//
// Address entry for address list
//

typedef struct
{
   UINT32 dwAddr1;
   UINT32 dwAddr2;
   UINT32 dwType;     // 0 = address/mask, 1 = address range
} NXC_ADDR_ENTRY;


//
// Graph ACL element
//

typedef struct
{
	UINT32 dwUserId;
	UINT32 dwAccess;
} NXC_GRAPH_ACL_ENTRY;


//
// Graph information
//

typedef struct
{
	UINT32 dwId;
	TCHAR *pszName;
	TCHAR *pszConfig;
	UINT32 dwOwner;
	UINT32 dwAclSize;
	NXC_GRAPH_ACL_ENTRY *pACL;
} NXC_GRAPH;


//
// System DCI information
//

typedef struct
{
	UINT32 dwId;
	TCHAR szName[MAX_DB_STRING];
	int nStatus;
	TCHAR *pszSettings;
} NXC_PERFTAB_DCI;


//
// Certificate information
//

typedef struct
{
	UINT32 dwId;
	int nType;
	TCHAR *pszSubject;
	TCHAR *pszComments;
} NXC_CERT_INFO;


//
// Certificate list
//

typedef struct
{
	UINT32 dwNumElements;
	NXC_CERT_INFO *pElements;
} NXC_CERT_LIST;


//
// SNMP USM credential
//

typedef struct
{
	TCHAR name[MAX_DB_STRING];
	int authMethod;
	int privMethod;
	TCHAR authPassword[MAX_DB_STRING];
	TCHAR privPassword[MAX_DB_STRING];
} NXC_SNMP_USM_CRED;


//
// Connection point information
//

typedef struct
{
	UINT32 localNodeId;
	UINT32 localInterfaceId;
	BYTE localMacAddr[MAC_ADDR_LENGTH];
	UINT32 remoteNodeId;
	UINT32 remoteInterfaceId;
	UINT32 remoteIfIndex;
} NXC_CONNECTION_POINT;


// All situation stuff will be available only in C++
#ifdef __cplusplus

//
// Situation instance
//

typedef struct
{
	TCHAR *m_name;
	StringMap *m_attrList;
} NXC_SITUATION_INSTANCE;


//
// Situation
//

typedef struct
{
	UINT32 m_id;
	TCHAR *m_name;
	TCHAR *m_comments;
	int m_instanceCount;
	NXC_SITUATION_INSTANCE *m_instanceList;
} NXC_SITUATION;


//
// Situation list
//

typedef struct
{
	int m_count;
	NXC_SITUATION *m_situations;
} NXC_SITUATION_LIST;

#endif	/* __cplusplus */


//
// Functions
//

#ifdef __cplusplus
extern "C" {
#endif

/** Library info/management **/
UINT32 LIBNXCL_EXPORTABLE NXCGetVersion(void);
const TCHAR LIBNXCL_EXPORTABLE *NXCGetErrorText(UINT32 dwError);

BOOL LIBNXCL_EXPORTABLE NXCInitialize(void);
void LIBNXCL_EXPORTABLE NXCShutdown(void);
void LIBNXCL_EXPORTABLE NXCSetDebugCallback(NXC_DEBUG_CALLBACK pFunc);

/** Low-level messaging **/
UINT32 LIBNXCL_EXPORTABLE NXCGenerateMessageId(NXC_SESSION hSession);
BOOL LIBNXCL_EXPORTABLE NXCSendMessage(NXC_SESSION hSession, CSCPMessage *msg);
CSCPMessage LIBNXCL_EXPORTABLE *NXCWaitForMessage(NXC_SESSION hSession, WORD wCode, UINT32 dwRqId);
UINT32 LIBNXCL_EXPORTABLE NXCWaitForRCC(NXC_SESSION hSession, UINT32 dwRqId);

/** Session management **/
UINT32 LIBNXCL_EXPORTABLE NXCConnect(UINT32 dwFlags, const TCHAR *pszServer, const TCHAR *pszLogin, 
                                    const TCHAR *pszPassword, UINT32 dwCertLen,
                                    BOOL (* pfSign)(BYTE *, UINT32, BYTE *, UINT32 *, void *),
                                    void *pSignArg, NXC_SESSION *phSession, const TCHAR *pszClientInfo,
                                    TCHAR **ppszUpgradeURL);
void LIBNXCL_EXPORTABLE NXCDisconnect(NXC_SESSION hSession);
void LIBNXCL_EXPORTABLE NXCStartWatchdog(NXC_SESSION hSession);
void LIBNXCL_EXPORTABLE NXCSetEventHandler(NXC_SESSION hSession, NXC_EVENT_HANDLER pHandler);
void LIBNXCL_EXPORTABLE NXCSetCommandTimeout(NXC_SESSION hSession, UINT32 dwTimeout);
void LIBNXCL_EXPORTABLE NXCGetServerID(NXC_SESSION hSession, BYTE *pbsId);
BOOL LIBNXCL_EXPORTABLE NXCIsDBConnLost(NXC_SESSION hSession);
UINT32 LIBNXCL_EXPORTABLE NXCSubscribe(NXC_SESSION hSession, UINT32 dwChannels);
UINT32 LIBNXCL_EXPORTABLE NXCUnsubscribe(NXC_SESSION hSession, UINT32 dwChannels);
void LIBNXCL_EXPORTABLE NXCGetLastLockOwner(NXC_SESSION hSession, TCHAR *pszBuffer,
                                            int nBufSize);
const TCHAR LIBNXCL_EXPORTABLE *NXCGetServerTimeZone(NXC_SESSION hSession);
UINT32 LIBNXCL_EXPORTABLE NXCCheckConnection(NXC_SESSION hSession);

/** SMS **/
UINT32 LIBNXCL_EXPORTABLE NXCSendSMS(NXC_SESSION hSession, TCHAR *phone, TCHAR *message);

/** Get/set client data associated with session **/
void LIBNXCL_EXPORTABLE *NXCGetClientData(NXC_SESSION hSession);
void LIBNXCL_EXPORTABLE NXCSetClientData(NXC_SESSION hSession, void *pData);

/** Objects **/
UINT32 LIBNXCL_EXPORTABLE NXCSyncObjects(NXC_SESSION hSession);
UINT32 LIBNXCL_EXPORTABLE NXCSyncObjectsEx(NXC_SESSION hSession, const TCHAR *pszCacheFile, BOOL bSyncComments);
UINT32 LIBNXCL_EXPORTABLE NXCSyncObjectSet(NXC_SESSION hSession, UINT32 *idList, UINT32 length, bool syncComments, WORD flags);
UINT32 LIBNXCL_EXPORTABLE NXCSyncSingleObject(NXC_SESSION hSession, UINT32 objectId);
NXC_OBJECT LIBNXCL_EXPORTABLE *NXCFindObjectById(NXC_SESSION hSession, UINT32 dwId);
NXC_OBJECT LIBNXCL_EXPORTABLE *NXCFindObjectByIdNoLock(NXC_SESSION hSession, UINT32 dwId);
NXC_OBJECT LIBNXCL_EXPORTABLE *NXCFindObjectByName(NXC_SESSION hSession, const TCHAR *pszName, UINT32 dwCurrObject);
NXC_OBJECT LIBNXCL_EXPORTABLE *NXCFindObjectByComments(NXC_SESSION hSession, const TCHAR *comments, UINT32 dwCurrObject);
NXC_OBJECT LIBNXCL_EXPORTABLE *NXCFindObjectByIPAddress(NXC_SESSION hSession, UINT32 dwIpAddr, UINT32 dwCurrObject);
void LIBNXCL_EXPORTABLE NXCEnumerateObjects(NXC_SESSION hSession, BOOL (* pHandler)(NXC_OBJECT *));
NXC_OBJECT LIBNXCL_EXPORTABLE *NXCGetTopologyRootObject(NXC_SESSION hSession);
NXC_OBJECT LIBNXCL_EXPORTABLE *NXCGetServiceRootObject(NXC_SESSION hSession);
NXC_OBJECT LIBNXCL_EXPORTABLE *NXCGetTemplateRootObject(NXC_SESSION hSession);
void LIBNXCL_EXPORTABLE *NXCGetObjectIndex(NXC_SESSION hSession, UINT32 *pdwNumObjects);
void LIBNXCL_EXPORTABLE NXCLockObjectIndex(NXC_SESSION hSession);
void LIBNXCL_EXPORTABLE NXCUnlockObjectIndex(NXC_SESSION hSession);
UINT32 LIBNXCL_EXPORTABLE NXCModifyObject(NXC_SESSION hSession, NXC_OBJECT_UPDATE *pData);
UINT32 LIBNXCL_EXPORTABLE NXCSetObjectMgmtStatus(NXC_SESSION hSession, UINT32 dwObjectId, BOOL bIsManaged);
UINT32 LIBNXCL_EXPORTABLE NXCCreateObject(NXC_SESSION hSession, NXC_OBJECT_CREATE_INFO *pCreateInfo, UINT32 *pdwObjectId);
UINT32 LIBNXCL_EXPORTABLE NXCBindObject(NXC_SESSION hSession, UINT32 dwParentObject, UINT32 dwChildObject);
UINT32 LIBNXCL_EXPORTABLE NXCUnbindObject(NXC_SESSION hSession, UINT32 dwParentObject, UINT32 dwChildObject);
UINT32 LIBNXCL_EXPORTABLE NXCAddClusterNode(NXC_SESSION hSession, UINT32 clusterId, UINT32 nodeId);
UINT32 LIBNXCL_EXPORTABLE NXCRemoveTemplate(NXC_SESSION hSession, UINT32 dwTemplateId,
                                           UINT32 dwNodeId, BOOL bRemoveDCI);
UINT32 LIBNXCL_EXPORTABLE NXCDeleteObject(NXC_SESSION hSession, UINT32 dwObject);
UINT32 LIBNXCL_EXPORTABLE NXCPollNode(NXC_SESSION hSession, UINT32 dwObjectId, int iPollType, 
                                     void (* pCallback)(TCHAR *, void *), void *pArg);
UINT32 LIBNXCL_EXPORTABLE NXCWakeUpNode(NXC_SESSION hSession, UINT32 dwObjectId);
UINT32 LIBNXCL_EXPORTABLE NXCGetSupportedParameters(NXC_SESSION hSession, UINT32 dwNodeId,
                                                   UINT32 *pdwNumParams, 
                                                   NXC_AGENT_PARAM **ppParamList);
void LIBNXCL_EXPORTABLE NXCGetComparableObjectName(NXC_SESSION hSession, UINT32 dwObjectId,
                                                   TCHAR *pszName);
void LIBNXCL_EXPORTABLE NXCGetComparableObjectNameEx(NXC_OBJECT *pObject, TCHAR *pszName);
UINT32 LIBNXCL_EXPORTABLE NXCSaveObjectCache(NXC_SESSION hSession, const TCHAR *pszFile);
UINT32 LIBNXCL_EXPORTABLE NXCGetAgentConfig(NXC_SESSION hSession, UINT32 dwNodeId,
                                           TCHAR **ppszConfig);
UINT32 LIBNXCL_EXPORTABLE NXCUpdateAgentConfig(NXC_SESSION hSession, UINT32 dwNodeId,
                                              TCHAR *pszConfig, BOOL bApply);
UINT32 LIBNXCL_EXPORTABLE NXCExecuteAction(NXC_SESSION hSession, UINT32 dwObjectId,
                                          TCHAR *pszAction);
UINT32 LIBNXCL_EXPORTABLE NXCGetObjectComments(NXC_SESSION hSession,
                                              UINT32 dwObjectId, TCHAR **ppszText);
UINT32 LIBNXCL_EXPORTABLE NXCUpdateObjectComments(NXC_SESSION hSession,
                                                 UINT32 dwObjectId, TCHAR *pszText);
BOOL LIBNXCL_EXPORTABLE NXCIsParent(NXC_SESSION hSession, UINT32 dwParent, UINT32 dwChild);
UINT32 LIBNXCL_EXPORTABLE NXCGetDCIEventsList(NXC_SESSION hSession, UINT32 dwObjectId,
                                             UINT32 **ppdwList, UINT32 *pdwListSize);
UINT32 LIBNXCL_EXPORTABLE NXCGetDCIInfo(NXC_SESSION hSession, UINT32 dwNodeId,
													UINT32 dwItemId, NXC_DCI *pInfo);
UINT32 LIBNXCL_EXPORTABLE NXCGetDCIThresholds(NXC_SESSION hSession, UINT32 dwNodeId, UINT32 dwItemId,
															NXC_DCI_THRESHOLD **ppList, UINT32 *pdwSize);

/** Container categories **/
UINT32 LIBNXCL_EXPORTABLE NXCLoadCCList(NXC_SESSION hSession, NXC_CC_LIST **ppList);
void LIBNXCL_EXPORTABLE NXCDestroyCCList(NXC_CC_LIST *pList);

/** Events **/
UINT32 LIBNXCL_EXPORTABLE NXCSyncEvents(NXC_SESSION hSession, UINT32 dwMaxRecords);
UINT32 LIBNXCL_EXPORTABLE NXCLoadEventDB(NXC_SESSION hSession);
UINT32 LIBNXCL_EXPORTABLE NXCModifyEventTemplate(NXC_SESSION hSession, NXC_EVENT_TEMPLATE *pArg);
UINT32 LIBNXCL_EXPORTABLE NXCGenerateEventCode(NXC_SESSION hSession, UINT32 *pdwEventCode);
void LIBNXCL_EXPORTABLE NXCAddEventTemplate(NXC_SESSION hSession, NXC_EVENT_TEMPLATE *pEventTemplate);
void LIBNXCL_EXPORTABLE NXCDeleteEDBRecord(NXC_SESSION hSession, UINT32 dwEventCode);
UINT32 LIBNXCL_EXPORTABLE NXCDeleteEventTemplate(NXC_SESSION hSession, UINT32 dwEventCode);
BOOL LIBNXCL_EXPORTABLE NXCGetEventDB(NXC_SESSION hSession, NXC_EVENT_TEMPLATE ***pppTemplateList, UINT32 *pdwNumRecords);
const TCHAR LIBNXCL_EXPORTABLE *NXCGetEventName(NXC_SESSION hSession, UINT32 dwId);
BOOL LIBNXCL_EXPORTABLE NXCGetEventNameEx(NXC_SESSION hSession, UINT32 dwId, TCHAR *pszBuffer, UINT32 dwBufSize);
int LIBNXCL_EXPORTABLE NXCGetEventSeverity(NXC_SESSION hSession, UINT32 dwId);
BOOL LIBNXCL_EXPORTABLE NXCGetEventText(NXC_SESSION hSession, UINT32 dwId, TCHAR *pszBuffer, UINT32 dwBufSize);
UINT32 LIBNXCL_EXPORTABLE NXCSendEvent(NXC_SESSION hSession, UINT32 dwEventCode, UINT32 dwObjectId,
												  int iNumArgs, TCHAR **pArgList, TCHAR *pszUserTag);

/** Syslog **/
UINT32 LIBNXCL_EXPORTABLE NXCSyncSyslog(NXC_SESSION hSession, UINT32 dwMaxRecords);

/** User management **/
UINT32 LIBNXCL_EXPORTABLE NXCLoadUserDB(NXC_SESSION hSession);
NXC_USER LIBNXCL_EXPORTABLE *NXCFindUserById(NXC_SESSION hSession, UINT32 dwId);
BOOL LIBNXCL_EXPORTABLE NXCGetUserDB(NXC_SESSION hSession, NXC_USER **ppUserList, UINT32 *pdwNumUsers);
UINT32 LIBNXCL_EXPORTABLE NXCLockUserDB(NXC_SESSION hSession);
UINT32 LIBNXCL_EXPORTABLE NXCUnlockUserDB(NXC_SESSION hSession);
UINT32 LIBNXCL_EXPORTABLE NXCCreateUser(NXC_SESSION hSession, TCHAR *pszName, BOOL bIsGroup, UINT32 *pdwNewId);
UINT32 LIBNXCL_EXPORTABLE NXCDeleteUser(NXC_SESSION hSession, UINT32 dwId);
UINT32 LIBNXCL_EXPORTABLE NXCModifyUser(NXC_SESSION hSession, NXC_USER *pUserInfo);
UINT32 LIBNXCL_EXPORTABLE NXCModifyUserEx(NXC_SESSION hSession, NXC_USER *pUserInfo, UINT32 dwFields);
UINT32 LIBNXCL_EXPORTABLE NXCSetPassword(NXC_SESSION hSession, UINT32 userId, const TCHAR *newPassword, const TCHAR *oldPassword);
BOOL LIBNXCL_EXPORTABLE NXCNeedPasswordChange(NXC_SESSION hSession);
UINT32 LIBNXCL_EXPORTABLE NXCGetUserVariable(NXC_SESSION hSession, UINT32 dwUserId,
                                            TCHAR *pszVarName, TCHAR *pszValue, UINT32 dwSize);
UINT32 LIBNXCL_EXPORTABLE NXCSetUserVariable(NXC_SESSION hSession, UINT32 dwUserId,
                                            TCHAR *pszVarName, TCHAR *pszValue);
UINT32 LIBNXCL_EXPORTABLE NXCCopyUserVariable(NXC_SESSION hSession, UINT32 dwSrcUserId,
                                             UINT32 dwDstUserId, TCHAR *pszVarName,
                                             BOOL bMove);
UINT32 LIBNXCL_EXPORTABLE NXCEnumUserVariables(NXC_SESSION hSession, UINT32 dwUserId,
                                              TCHAR *pszPattern, UINT32 *pdwNumVars,
                                              TCHAR ***pppszVarList);
UINT32 LIBNXCL_EXPORTABLE NXCDeleteUserVariable(NXC_SESSION hSession, UINT32 dwUserId,
                                               TCHAR *pszVarName);
UINT32 LIBNXCL_EXPORTABLE NXCGetSessionList(NXC_SESSION hSession, UINT32 *pdwNumSessions,
                                           NXC_CLIENT_SESSION_INFO **ppList);
UINT32 LIBNXCL_EXPORTABLE NXCKillSession(NXC_SESSION hSession, UINT32 dwSessionId);
UINT32 LIBNXCL_EXPORTABLE NXCGetCurrentUserId(NXC_SESSION hSession);
UINT32 LIBNXCL_EXPORTABLE NXCGetCurrentSystemAccess(NXC_SESSION hSession);

/** Data collection **/
UINT32 LIBNXCL_EXPORTABLE NXCOpenNodeDCIList(NXC_SESSION hSession, UINT32 dwNodeId, NXC_DCI_LIST **ppItemList);
UINT32 LIBNXCL_EXPORTABLE NXCCloseNodeDCIList(NXC_SESSION hSession, NXC_DCI_LIST *pItemList);
UINT32 LIBNXCL_EXPORTABLE NXCCreateNewDCI(NXC_SESSION hSession, NXC_DCI_LIST *pItemList, UINT32 *pdwItemId);
UINT32 LIBNXCL_EXPORTABLE NXCUpdateDCI(NXC_SESSION hSession, UINT32 dwNodeId, NXC_DCI *pItem);
UINT32 LIBNXCL_EXPORTABLE NXCDeleteDCI(NXC_SESSION hSession, NXC_DCI_LIST *pItemList, UINT32 dwItemId);
UINT32 LIBNXCL_EXPORTABLE NXCSetDCIStatus(NXC_SESSION hSession, UINT32 dwNodeId, UINT32 dwNumItems, 
                                         UINT32 *pdwItemList, int iStatus);
UINT32 LIBNXCL_EXPORTABLE NXCCopyDCI(NXC_SESSION hSession, UINT32 dwSrcNodeId, UINT32 dwDstNodeId, 
                                    UINT32 dwNumItems, UINT32 *pdwItemList, BOOL bMove);
UINT32 LIBNXCL_EXPORTABLE NXCApplyTemplate(NXC_SESSION hSession, UINT32 dwTemplateId, UINT32 dwNodeId);
UINT32 LIBNXCL_EXPORTABLE NXCItemIndex(NXC_DCI_LIST *pItemList, UINT32 dwItemId);
UINT32 LIBNXCL_EXPORTABLE NXCGetDCIData(NXC_SESSION hSession, UINT32 dwNodeId, UINT32 dwItemId, UINT32 dwMaxRows,
                                       UINT32 dwTimeFrom, UINT32 dwTimeTo, NXC_DCI_DATA **ppData);
UINT32 LIBNXCL_EXPORTABLE NXCGetDCIDataEx(NXC_SESSION hSession, UINT32 dwNodeId, UINT32 dwItemId, 
                                         UINT32 dwMaxRows, UINT32 dwTimeFrom, UINT32 dwTimeTo, 
                                         NXC_DCI_DATA **ppData, NXC_DCI_THRESHOLD **thresholds, UINT32 *numThresholds);
void LIBNXCL_EXPORTABLE NXCDestroyDCIData(NXC_DCI_DATA *pData);
NXC_DCI_ROW LIBNXCL_EXPORTABLE *NXCGetRowPtr(NXC_DCI_DATA *pData, UINT32 dwRow);
UINT32 LIBNXCL_EXPORTABLE NXCGetLastValues(NXC_SESSION hSession, UINT32 dwNodeId,
                                          UINT32 *pdwNumItems, NXC_DCI_VALUE **ppValueList);
UINT32 LIBNXCL_EXPORTABLE NXCClearDCIData(NXC_SESSION hSession, UINT32 dwNodeId, UINT32 dwItemId);
UINT32 LIBNXCL_EXPORTABLE NXCAddThresholdToItem(NXC_DCI *pItem, NXC_DCI_THRESHOLD *pThreshold);
BOOL LIBNXCL_EXPORTABLE NXCDeleteThresholdFromItem(NXC_DCI *pItem, UINT32 dwIndex);
BOOL LIBNXCL_EXPORTABLE NXCSwapThresholds(NXC_DCI *pItem, UINT32 dwIndex1, UINT32 dwIndex2);
UINT32 LIBNXCL_EXPORTABLE NXCQueryParameter(NXC_SESSION hSession, UINT32 dwNodeId, int iOrigin, TCHAR *pszParameter,
                                           TCHAR *pszBuffer, UINT32 dwBufferSize);
UINT32 LIBNXCL_EXPORTABLE NXCResolveDCINames(NXC_SESSION hSession, UINT32 dwNumDCI,
                                            INPUT_DCI *pDCIList, TCHAR ***pppszNames);
UINT32 LIBNXCL_EXPORTABLE NXCGetPerfTabDCIList(NXC_SESSION hSession, UINT32 dwNodeId,
                                              UINT32 *pdwNumItems, NXC_PERFTAB_DCI **ppList);
UINT32 LIBNXCL_EXPORTABLE NXCPushDCIData(NXC_SESSION hSession, UINT32 dwNumItems,
                                        NXC_DCI_PUSH_DATA *pItems, UINT32 *pdwIndex);
UINT32 LIBNXCL_EXPORTABLE NXCTestDCITransformation(NXC_SESSION hSession, UINT32 dwNodeId, UINT32 dwItemId,
																  const TCHAR *script, const TCHAR *value, BOOL *execStatus,
																  TCHAR *execResult, size_t resultBufSize);

/** MIB files **/
UINT32 LIBNXCL_EXPORTABLE NXCGetMIBFileTimeStamp(NXC_SESSION hSession, UINT32 *pdwTimeStamp);
UINT32 LIBNXCL_EXPORTABLE NXCDownloadMIBFile(NXC_SESSION hSession, const TCHAR *pszName);

/** Event processing policy **/
NXC_EPP LIBNXCL_EXPORTABLE *NXCCreateEmptyEventPolicy();
UINT32 LIBNXCL_EXPORTABLE NXCOpenEventPolicy(NXC_SESSION hSession, NXC_EPP **ppEventPolicy);
UINT32 LIBNXCL_EXPORTABLE NXCCloseEventPolicy(NXC_SESSION hSession);
UINT32 LIBNXCL_EXPORTABLE NXCSaveEventPolicy(NXC_SESSION hSession, NXC_EPP *pEventPolicy);
void LIBNXCL_EXPORTABLE NXCDestroyEventPolicy(NXC_EPP *pEventPolicy);
void LIBNXCL_EXPORTABLE NXCAddPolicyRule(NXC_EPP *policy, NXC_EPP_RULE *rule, BOOL dynAllocated);
void LIBNXCL_EXPORTABLE NXCDeletePolicyRule(NXC_EPP *pEventPolicy, UINT32 dwRule);
NXC_EPP_RULE LIBNXCL_EXPORTABLE *NXCCopyEventPolicyRule(NXC_EPP_RULE *src);
void LIBNXCL_EXPORTABLE NXCCopyEventPolicyRuleToBuffer(NXC_EPP_RULE *dst, NXC_EPP_RULE *src);

/** Alarms **/
UINT32 LIBNXCL_EXPORTABLE NXCLoadAllAlarms(NXC_SESSION hSession, UINT32 *pdwNumAlarms, NXC_ALARM **ppAlarmList);
UINT32 LIBNXCL_EXPORTABLE NXCAcknowledgeAlarm(NXC_SESSION hSession, UINT32 dwAlarmId);
UINT32 LIBNXCL_EXPORTABLE NXCTerminateAlarm(NXC_SESSION hSession, UINT32 dwAlarmId);
UINT32 LIBNXCL_EXPORTABLE NXCDeleteAlarm(NXC_SESSION hSession, UINT32 dwAlarmId);
UINT32 LIBNXCL_EXPORTABLE NXCOpenAlarm(NXC_SESSION hSession, UINT32 dwAlarmId, TCHAR *pszHelpdeskRef);
UINT32 LIBNXCL_EXPORTABLE NXCCloseAlarm(NXC_SESSION hSession, UINT32 dwAlarmId);
TCHAR LIBNXCL_EXPORTABLE *NXCFormatAlarmText(NXC_SESSION session, NXC_ALARM *alarm, TCHAR *format);

/** Actions **/
UINT32 LIBNXCL_EXPORTABLE NXCLoadActions(NXC_SESSION hSession, UINT32 *pdwNumActions, NXC_ACTION **ppActionList);
UINT32 LIBNXCL_EXPORTABLE NXCCreateAction(NXC_SESSION hSession, TCHAR *pszName, UINT32 *pdwNewId);
UINT32 LIBNXCL_EXPORTABLE NXCModifyAction(NXC_SESSION hSession, NXC_ACTION *pAction);
UINT32 LIBNXCL_EXPORTABLE NXCDeleteAction(NXC_SESSION hSession, UINT32 dwActionId);

/** SNMP trap configuration **/
UINT32 LIBNXCL_EXPORTABLE NXCLoadTrapCfg(NXC_SESSION hSession, UINT32 *pdwNumTraps, NXC_TRAP_CFG_ENTRY **ppTrapList);
void LIBNXCL_EXPORTABLE NXCDestroyTrapList(UINT32 dwNumTraps, NXC_TRAP_CFG_ENTRY *pTrapList);
UINT32 LIBNXCL_EXPORTABLE NXCCreateTrap(NXC_SESSION hSession, UINT32 *pdwTrapId);
UINT32 LIBNXCL_EXPORTABLE NXCModifyTrap(NXC_SESSION hSession, NXC_TRAP_CFG_ENTRY *pTrap);
UINT32 LIBNXCL_EXPORTABLE NXCDeleteTrap(NXC_SESSION hSession, UINT32 dwTrapId);
UINT32 LIBNXCL_EXPORTABLE NXCSyncSNMPTrapLog(NXC_SESSION hSession, UINT32 dwMaxRecords);
UINT32 LIBNXCL_EXPORTABLE NXCGetTrapCfgRO(NXC_SESSION hSession, UINT32 *pdwNumTraps,
                                         NXC_TRAP_CFG_ENTRY **ppTrapList);
NXC_TRAP_CFG_ENTRY LIBNXCL_EXPORTABLE *NXCDuplicateTrapCfgEntry(NXC_TRAP_CFG_ENTRY *src);
void LIBNXCL_EXPORTABLE NXCCopyTrapCfgEntry(NXC_TRAP_CFG_ENTRY *dst, NXC_TRAP_CFG_ENTRY *src);
void LIBNXCL_EXPORTABLE NXCDestroyTrapCfgEntry(NXC_TRAP_CFG_ENTRY *e);

/** Packages **/
UINT32 LIBNXCL_EXPORTABLE NXCLockPackageDB(NXC_SESSION hSession);
UINT32 LIBNXCL_EXPORTABLE NXCUnlockPackageDB(NXC_SESSION hSession);
UINT32 LIBNXCL_EXPORTABLE NXCGetPackageList(NXC_SESSION hSession, UINT32 *pdwNumPackages, 
                                           NXC_PACKAGE_INFO **ppList);
UINT32 LIBNXCL_EXPORTABLE NXCInstallPackage(NXC_SESSION hSession, NXC_PACKAGE_INFO *pInfo,
                                           TCHAR *pszFullPkgPath);
UINT32 LIBNXCL_EXPORTABLE NXCRemovePackage(NXC_SESSION hSession, UINT32 dwPkgId);
UINT32 LIBNXCL_EXPORTABLE NXCParseNPIFile(TCHAR *pszInfoFile, NXC_PACKAGE_INFO *pInfo);
UINT32 LIBNXCL_EXPORTABLE NXCDeployPackage(NXC_SESSION hSession, UINT32 dwPkgId,
                                          UINT32 dwNumObjects, UINT32 *pdwObjectList,
                                          UINT32 *pdwRqId);

/** Server management **/
UINT32 LIBNXCL_EXPORTABLE NXCResetServerComponent(NXC_SESSION hSession, UINT32 dwComponent);
UINT32 LIBNXCL_EXPORTABLE NXCGetServerStats(NXC_SESSION hSession, NXC_SERVER_STATS *pStats);
UINT32 LIBNXCL_EXPORTABLE NXCGetServerVariables(NXC_SESSION hSession, 
                                               NXC_SERVER_VARIABLE **ppVarList, 
                                               UINT32 *pdwNumVars);
UINT32 LIBNXCL_EXPORTABLE NXCSetServerVariable(NXC_SESSION hSession, const TCHAR *pszVarName,
                                              const TCHAR *pszValue);
UINT32 LIBNXCL_EXPORTABLE NXCDeleteServerVariable(NXC_SESSION hSession, const TCHAR *pszVarName);
UINT32 LIBNXCL_EXPORTABLE NXCGetServerConfigCLOB(NXC_SESSION hSession, const TCHAR *name, TCHAR **value);
UINT32 LIBNXCL_EXPORTABLE NXCSetServerConfigCLOB(NXC_SESSION hSession, const TCHAR *name, const TCHAR *value);

/** Object tools **/
UINT32 LIBNXCL_EXPORTABLE NXCGetObjectTools(NXC_SESSION hSession, UINT32 *pdwNumTools,
                                           NXC_OBJECT_TOOL **ppToolList);
void LIBNXCL_EXPORTABLE NXCDestroyObjectToolList(UINT32 dwNumTools, NXC_OBJECT_TOOL *pList);
#ifdef __cplusplus
UINT32 LIBNXCL_EXPORTABLE NXCExecuteTableTool(NXC_SESSION hSession, UINT32 dwNodeId,
                                             UINT32 dwToolId, Table **ppData);
#endif
UINT32 LIBNXCL_EXPORTABLE NXCGetObjectToolDetails(NXC_SESSION hSession, UINT32 dwToolId,
                                                 NXC_OBJECT_TOOL_DETAILS **ppData);
void LIBNXCL_EXPORTABLE NXCDestroyObjectToolDetails(NXC_OBJECT_TOOL_DETAILS *pData);
UINT32 LIBNXCL_EXPORTABLE NXCDeleteObjectTool(NXC_SESSION hSession, UINT32 dwToolId);
UINT32 LIBNXCL_EXPORTABLE NXCGenerateObjectToolId(NXC_SESSION hSession, UINT32 *pdwToolId);
UINT32 LIBNXCL_EXPORTABLE NXCUpdateObjectTool(NXC_SESSION hSession,
                                             NXC_OBJECT_TOOL_DETAILS *pData);
BOOL LIBNXCL_EXPORTABLE NXCIsAppropriateTool(NXC_OBJECT_TOOL *pTool, NXC_OBJECT *pObject);
UINT32 LIBNXCL_EXPORTABLE NXCExecuteServerCommand(NXC_SESSION hSession, UINT32 nodeId, const TCHAR *command);

/** Script library **/
UINT32 LIBNXCL_EXPORTABLE NXCGetScriptList(NXC_SESSION hSession, UINT32 *pdwNumScrpts,
                                          NXC_SCRIPT_INFO **ppList);
UINT32 LIBNXCL_EXPORTABLE NXCGetScript(NXC_SESSION hSession, UINT32 dwId, TCHAR **ppszCode);
UINT32 LIBNXCL_EXPORTABLE NXCUpdateScript(NXC_SESSION hSession, UINT32 *pdwId,
                                         TCHAR *pszName, TCHAR *pszCode);
UINT32 LIBNXCL_EXPORTABLE NXCRenameScript(NXC_SESSION hSession, UINT32 dwId,
                                         TCHAR *pszName);
UINT32 LIBNXCL_EXPORTABLE NXCDeleteScript(NXC_SESSION hSession, UINT32 dwId);

/** SNMP **/
UINT32 LIBNXCL_EXPORTABLE NXCSnmpWalk(NXC_SESSION hSession, UINT32 dwNode,
                                     TCHAR *pszRootOID, void *pUserData,
                                     void (* pfCallback)(TCHAR *, UINT32, TCHAR *, void *));
UINT32 LIBNXCL_EXPORTABLE NXCGetSnmpCommunityList(NXC_SESSION hSession, UINT32 *pdwNumStrings,
																 TCHAR ***pppszStringList);
UINT32 LIBNXCL_EXPORTABLE NXCUpdateSnmpCommunityList(NXC_SESSION hSession, UINT32 dwNumStrings,
											    					 TCHAR **ppszStringList);
UINT32 LIBNXCL_EXPORTABLE NXCGetSnmpUsmCredentials(NXC_SESSION hSession, UINT32 *listSize, NXC_SNMP_USM_CRED **list);
UINT32 LIBNXCL_EXPORTABLE NXCUpdateSnmpUsmCredentials(NXC_SESSION hSession, UINT32 count, NXC_SNMP_USM_CRED *list);

/** Address lists **/
UINT32 LIBNXCL_EXPORTABLE NXCGetAddrList(NXC_SESSION hSession, UINT32 dwListType,
                                        UINT32 *pdwAddrCount, NXC_ADDR_ENTRY **ppAddrList);
UINT32 LIBNXCL_EXPORTABLE NXCSetAddrList(NXC_SESSION hSession, UINT32 dwListType,
                                        UINT32 dwAddrCount, NXC_ADDR_ENTRY *pAddrList);

/** Agent configurations **/
UINT32 LIBNXCL_EXPORTABLE NXCGetAgentConfigList(NXC_SESSION hSession, UINT32 *pdwNumRecs,
                                               NXC_AGENT_CONFIG_INFO **ppList);
UINT32 LIBNXCL_EXPORTABLE NXCOpenAgentConfig(NXC_SESSION hSession, UINT32 dwCfgId,
                                            NXC_AGENT_CONFIG *pConfig);
UINT32 LIBNXCL_EXPORTABLE NXCSaveAgentConfig(NXC_SESSION hSession, NXC_AGENT_CONFIG *pConfig);
UINT32 LIBNXCL_EXPORTABLE NXCDeleteAgentConfig(NXC_SESSION hSession, UINT32 dwCfgId);
UINT32 LIBNXCL_EXPORTABLE NXCSwapAgentConfigs(NXC_SESSION hSession, UINT32 dwCfgId1, UINT32 dwCfgId2);

/** Server configuration export/import **/
UINT32 LIBNXCL_EXPORTABLE NXCExportConfiguration(NXC_SESSION hSession, TCHAR *pszDescr,
                                                UINT32 dwNumEvents, UINT32 *pdwEventList,
                                                UINT32 dwNumTemplates, UINT32 *pdwTemplateList,
                                                UINT32 dwNumTraps, UINT32 *pdwTrapList,
                                                TCHAR **ppszContent);
UINT32 LIBNXCL_EXPORTABLE NXCImportConfiguration(NXC_SESSION hSession, TCHAR *pszContent,
                                                UINT32 dwFlags, TCHAR *pszErrorText, int nErrorLen);

/** Predefined graphs **/
UINT32 LIBNXCL_EXPORTABLE NXCGetGraphList(NXC_SESSION hSession, UINT32 *pdwNumGraphs, NXC_GRAPH **ppGraphList);
void LIBNXCL_EXPORTABLE NXCDestroyGraphList(UINT32 dwNumGraphs, NXC_GRAPH *pList);
UINT32 LIBNXCL_EXPORTABLE NXCDefineGraph(NXC_SESSION hSession, NXC_GRAPH *pGraph);
UINT32 LIBNXCL_EXPORTABLE NXCDeleteGraph(NXC_SESSION hSession, UINT32 dwGraphId);

/** Certificates **/
UINT32 LIBNXCL_EXPORTABLE NXCAddCACertificate(NXC_SESSION hSession, UINT32 dwCertLen,
                                             BYTE *pCert, TCHAR *pszComments);
UINT32 LIBNXCL_EXPORTABLE NXCUpdateCertificateComments(NXC_SESSION hSession, UINT32 dwCertId,
                                                      TCHAR *pszComments);
UINT32 LIBNXCL_EXPORTABLE NXCDeleteCertificate(NXC_SESSION hSession, UINT32 dwCertId);
UINT32 LIBNXCL_EXPORTABLE NXCGetCertificateList(NXC_SESSION hSession, NXC_CERT_LIST **ppList);
void LIBNXCL_EXPORTABLE NXCDestroyCertificateList(NXC_CERT_LIST *pList);

/** Layer 2 topology **/
UINT32 LIBNXCL_EXPORTABLE NXCQueryL2Topology(NXC_SESSION hSession, UINT32 dwNodeId, void **ppTopology);
UINT32 LIBNXCL_EXPORTABLE NXCFindConnectionPoint(NXC_SESSION hSession, UINT32 objectId, NXC_CONNECTION_POINT *cpInfo);
UINT32 LIBNXCL_EXPORTABLE NXCFindMACAddress(NXC_SESSION hSession, BYTE *macAddr, NXC_CONNECTION_POINT *cpInfo);

/** Situations **/
UINT32 LIBNXCL_EXPORTABLE NXCCreateSituation(NXC_SESSION hSession, const TCHAR *name, const TCHAR *comments, UINT32 *pdwId);
UINT32 LIBNXCL_EXPORTABLE NXCModifySituation(NXC_SESSION hSession, UINT32 id,
                                            const TCHAR *name, const TCHAR *comments);
UINT32 LIBNXCL_EXPORTABLE NXCDeleteSituation(NXC_SESSION hSession, UINT32 id);
UINT32 LIBNXCL_EXPORTABLE NXCDeleteSituationInstance(NXC_SESSION hSession, UINT32 id, const TCHAR *instance);

#ifdef __cplusplus
UINT32 LIBNXCL_EXPORTABLE NXCGetSituationList(NXC_SESSION hSession, NXC_SITUATION_LIST **list);
void LIBNXCL_EXPORTABLE NXCDestroySituationList(NXC_SITUATION_LIST *list);
void LIBNXCL_EXPORTABLE NXCUpdateSituationList(NXC_SITUATION_LIST *list, int code, NXC_SITUATION *update);
#endif

#ifdef __cplusplus
}
#endif

#endif   /* LIBNXCL_NO_DECLARATIONS */

#endif   /* _nxclapi_h_ */
