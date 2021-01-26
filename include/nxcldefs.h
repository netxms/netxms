/*
** NetXMS - Network Management System
** Common defines for client library and server
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
** File: nxcldefs.h
**
**/

#ifndef _nxcldefs_h_
#define _nxcldefs_h_

#include <nxlog.h>
#include <uuid.h>

/**
 * Some constants
 */
#define MAX_COMMUNITY_LENGTH        128
#define MAX_OID_LENGTH              1024
#define MAX_EVENT_MSG_LENGTH        2000
#define MAX_EVENT_NAME              64
#define MAX_USERTAG_LENGTH          64
#define MAX_SESSION_NAME            256
#define MAX_USER_NAME               64
#define MAX_USER_FULLNAME           128
#define MAX_USER_DESCR              256
#define MAX_ITEM_NAME               1024
#define MAX_INSTANCE_LEN            1024
#define MAX_STRING_VALUE            256
#define MAX_USERVAR_NAME_LENGTH     64
#define MAX_AGENT_VERSION_LEN       64
#define MAX_PLATFORM_NAME_LEN       64
#define MAX_PACKAGE_NAME_LEN        64
#define MAX_POLICY_TYPE_LEN         32
#define MAX_NODE_SUBTYPE_LENGTH     128
#define MAX_HYPERVISOR_TYPE_LENGTH  32
#define MAX_HYPERVISOR_INFO_LENGTH  256
#define GROUP_EVERYONE              ((uint32_t)0x40000000)
#define INVALID_UID                 ((uint32_t)0xFFFFFFFF)
#define OBJECT_STATUS_COUNT         9
#define MAX_RCPT_ADDR_LEN           256
#define MAX_EMAIL_SUBJECT_LEN       256
#define MAC_ADDR_LENGTH             6
#define CURRENT_USER                ((uint32_t)0xFFFFFFFF)
#define MAX_DCI_DATA_RECORDS        200000
#define MAX_POLICY_CONFIG_NAME      64
#define MAX_INT32                   0x7FFFFFFF
#define MAX_NC_DESCRIPTION          256
#define MAX_NC_ERROR_MESSAGE        1024

/**
 * NetXMS agent authentication methods
 */
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
#define POLL_STATUS                 1
#define POLL_CONFIGURATION_FULL     2
#define POLL_INTERFACE_NAMES        3
#define POLL_TOPOLOGY               4
#define POLL_CONFIGURATION_NORMAL   5
#define POLL_INSTANCE_DISCOVERY     6

/**
 * Object types
 */
#define OBJECT_GENERIC               0
#define OBJECT_SUBNET                1
#define OBJECT_NODE                  2
#define OBJECT_INTERFACE             3
#define OBJECT_NETWORK               4
#define OBJECT_CONTAINER             5
#define OBJECT_ZONE                  6
#define OBJECT_SERVICEROOT           7
#define OBJECT_TEMPLATE              8
#define OBJECT_TEMPLATEGROUP         9
#define OBJECT_TEMPLATEROOT          10
#define OBJECT_NETWORKSERVICE        11
#define OBJECT_VPNCONNECTOR          12
#define OBJECT_CONDITION             13
#define OBJECT_CLUSTER			       14
//#define OBJECT_POLICYGROUP           15
//#define OBJECT_POLICYROOT            16
//#define OBJECT_AGENTPOLICY           17
//#define OBJECT_AGENTPOLICY_CONFIG    18
#define OBJECT_NETWORKMAPROOT        19
#define OBJECT_NETWORKMAPGROUP       20
#define OBJECT_NETWORKMAP            21
#define OBJECT_DASHBOARDROOT         22
#define OBJECT_DASHBOARD             23
#define OBJECT_BUSINESSSERVICEROOT   27
#define OBJECT_BUSINESSSERVICE       28
#define OBJECT_NODELINK              29
#define OBJECT_SLMCHECK              30
#define OBJECT_MOBILEDEVICE          31
#define OBJECT_RACK                  32
#define OBJECT_ACCESSPOINT           33
//#define OBJECT_AGENTPOLICY_LOGPARSER 34
#define OBJECT_CHASSIS               35
#define OBJECT_DASHBOARDGROUP        36
#define OBJECT_SENSOR                37

/** Base value for custom object classes */
#define OBJECT_CUSTOM                10000

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
 * Node capabilities
 */
#define NC_IS_SNMP                0x00000001
#define NC_IS_NATIVE_AGENT        0x00000002
#define NC_IS_BRIDGE              0x00000004
#define NC_IS_ROUTER              0x00000008
#define NC_IS_LOCAL_MGMT          0x00000010
#define NC_IS_PRINTER             0x00000020
#define NC_IS_OSPF                0x00000040
#define NC_IS_CDP                 0x00000100
#define NC_IS_NDP                 0x00000200  /* Supports Nortel (Synoptics/Bay Networks) topology discovery */ /* SONMP is an old name for NDP */
#define NC_IS_LLDP                0x00000400  /* Supports Link Layer Discovery Protocol */
#define NC_IS_VRRP                0x00000800  /* VRRP support */
#define NC_HAS_VLANS              0x00001000  /* VLAN information available */
#define NC_IS_8021X               0x00002000  /* 802.1x support enabled on node */
#define NC_IS_STP                 0x00004000  /* Spanning Tree (IEEE 802.1d) enabled on node */
#define NC_HAS_ENTITY_MIB         0x00008000  /* Supports ENTITY-MIB */
#define NC_HAS_IFXTABLE           0x00010000  /* Supports ifXTable */
#define NC_HAS_AGENT_IFXCOUNTERS  0x00020000  /* Agent supports 64-bit interface counters */
#define NC_HAS_WINPDH             0x00040000  /* Node supports Windows PDH parameters */
#define NC_IS_WIFI_CONTROLLER     0x00080000  /* Node is wireless network controller */
#define NC_IS_SMCLP               0x00100000  /* Node supports SMCLP protocol */
#define NC_IS_NEW_POLICY_TYPES    0x00200000  /* Defines if agent is already upgraded to new policy type */
#define NC_HAS_USER_AGENT         0x00400000  /* User agent (desktop support app) is installed */
#define NC_IS_ETHERNET_IP         0x00800000
#define NC_IS_MODBUS_TCP          0x01000000
#define NC_IS_PROFINET            0x02000000

/**
 * Flag separator
 */
#define DCF_COLLECTION_FLAGS      0x0000FFFF

/**
 * Node flags
 */
#define NF_EXTERNAL_GATEWAY            0x00010000
#define NF_DISABLE_DISCOVERY_POLL      0x00020000
#define NF_DISABLE_TOPOLOGY_POLL       0x00040000
#define NF_DISABLE_SNMP                0x00080000
#define NF_DISABLE_NXCP                0x00100000
#define NF_DISABLE_ICMP                0x00200000
#define NF_FORCE_ENCRYPTION            0x00400000
#define NF_DISABLE_ROUTE_POLL          0x00800000
#define NF_AGENT_OVER_TUNNEL_ONLY      0x01000000
#define NF_SNMP_SETTINGS_LOCKED        0x02000000
#define NF_PING_PRIMARY_IP             0x04000000
#define NF_DISABLE_ETHERNET_IP         0x08000000
#define NF_DISABLE_PERF_COUNT          0x10000000
#define NF_DISABLE_8021X_STATUS_POLL   0x20000000

/**
 * Data Collection Target flags (lower bits of flags field)
 */
#define DCF_DISABLE_STATUS_POLL        0x00000001
#define DCF_DISABLE_CONF_POLL          0x00000002
#define DCF_DISABLE_DATA_COLLECT       0x00000004
#define DCF_LOCATION_CHANGE_EVENT      0x00000008

/**
 * Common object runtime flags (can be used by all NetObj derived classes)
 */
#define ODF_FORCE_STATUS_POLL              0x0001
#define ODF_FORCE_CONFIGURATION_POLL       0x0002
#define ODF_CONFIGURATION_POLL_PASSED      0x0004
#define ODF_CONFIGURATION_POLL_PENDING     0x0008

/**
 * Node runtime (dynamic) flags
 */
#define NDF_RECHECK_CAPABILITIES       0x00010000
#define NDF_NEW_TUNNEL_BIND            0x00020000

/**
 * Data collection object state flags
 */
#define DCSF_UNREACHABLE               0x00000001
#define DCSF_NETWORK_PATH_PROBLEM      0x00000002

/**
 * Cluster state flags
 */
#define CLSF_DOWN                      0x00010000

/**
 * Node state flags
 */
#define NSF_AGENT_UNREACHABLE          0x00010000
#define NSF_SNMP_UNREACHABLE           0x00020000
#define NSF_ETHERNET_IP_UNREACHABLE    0x00040000
#define NSF_CACHE_MODE_NOT_SUPPORTED   0x00080000
#define NSF_SNMP_TRAP_FLOOD            0x00100000

/**
 * Sensor state flags
 */
#define SSF_PROVISIONED                0x00010000
#define SSF_REGISTERED                 0x00020000
#define SSF_ACTIVE                     0x00040000
#define SSF_CONF_UPDATE_PENDING        0x00080000

/**
 * Auto apply flags
 * Depricated flags for Container, Template and Policy
 */
#define AAF_AUTO_APPLY            0x00000001
#define AAF_AUTO_REMOVE           0x00000002

/**
 * Chassis flags
 */
#define CHF_BIND_UNDER_CONTROLLER   0x00000001

/**
 * Access point flags
 */
#define APF_INCLUDE_IN_ICMP_POLL 0x00000001

/**
 * Interface flags
 */
#define IF_SYNTHETIC_MASK              0x00000001
#define IF_PHYSICAL_PORT               0x00000002
#define IF_EXCLUDE_FROM_TOPOLOGY       0x00000004
#define IF_LOOPBACK                    0x00000008
#define IF_CREATED_MANUALLY            0x00000010
#define IF_PEER_REFLECTION             0x00000020  /* topology information obtained by reflection */
#define IF_INCLUDE_IN_ICMP_POLL        0x00000040  /* interface should be included into node ICMP poll */
#define IF_DISABLE_SNMP_STATUS_POLL    0x00000080  /* do not use SNMP for reading interface status */
#define IF_DISABLE_ICMP_STATUS_POLL    0x00000100  /* do not use ICMP for reading interface status */
#define IF_DISABLE_AGENT_STATUS_POLL   0x00000200  /* do not use NetXMS agent for reading interface status */
#define IF_EXPECTED_STATE_MASK         0x30000000	/* 2-bit field holding expected interface state */
#define IF_USER_FLAGS_MASK             /* flags that can be changed by user */ \
         ( \
            IF_EXCLUDE_FROM_TOPOLOGY | \
            IF_INCLUDE_IN_ICMP_POLL | \
            IF_DISABLE_SNMP_STATUS_POLL | \
            IF_DISABLE_ICMP_STATUS_POLL | \
            IF_DISABLE_AGENT_STATUS_POLL \
         )

/**
 * Expected interface states
 */
#define IF_EXPECTED_STATE_UP     0
#define IF_EXPECTED_STATE_DOWN   1
#define IF_EXPECTED_STATE_IGNORE 2
#define IF_EXPECTED_STATE_AUTO   3

/**
 * Default expected state for new interface creation
 */
#define IF_DEFAULT_EXPECTED_STATE_UP      0
#define IF_DEFAULT_EXPECTED_STATE_AUTO    1
#define IF_DEFAULT_EXPECTED_STATE_IGNORE  2

/**
 * Interface administrative states
 */
enum InterfaceAdminState
{
   IF_ADMIN_STATE_UNKNOWN = 0,
   IF_ADMIN_STATE_UP      = 1,
   IF_ADMIN_STATE_DOWN    = 2,
   IF_ADMIN_STATE_TESTING = 3
};

/**
 * Interface operational states
 */
enum InterfaceOperState
{
   IF_OPER_STATE_UNKNOWN     = 0,
   IF_OPER_STATE_UP          = 1,
   IF_OPER_STATE_DOWN        = 2,
   IF_OPER_STATE_TESTING     = 3,
   IF_OPER_STATE_DORMANT     = 4,
   IF_OPER_STATE_NOT_PRESENT = 5
};

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

/**
 * Network map types
 */
#define MAP_TYPE_CUSTOM                      0
#define MAP_TYPE_LAYER2_TOPOLOGY             1
#define MAP_TYPE_IP_TOPOLOGY                 2
#define MAP_INTERNAL_COMMUNICATION_TOPOLOGY  3

/**
 * Components that can be locked by management pack installer
 */
#define NXMP_LC_EVENTDB    0
#define NXMP_LC_EPP        1
#define NXMP_LC_TRAPCFG    2

/**
 * Network service types
 */
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

/**
 * Address list types
 */
#define ADDR_LIST_DISCOVERY_TARGETS    1
#define ADDR_LIST_DISCOVERY_FILTER     2

/**
 * Discovery filter flags
 */
#define DFF_ALLOW_AGENT                0x0001
#define DFF_ALLOW_SNMP                 0x0002
#define DFF_ONLY_RANGE                 0x0004

/**
 * Connection point types
 */
#define CP_TYPE_INDIRECT      0
#define CP_TYPE_DIRECT        1
#define CP_TYPE_WIRELESS      2
#define CP_TYPE_UNKNOWN       3

/**
 * Events
 */
#define NXC_EVENT_CONNECTION_BROKEN    1
#define NXC_EVENT_NEW_ELOG_RECORD      2
#define NXC_EVENT_USER_DB_CHANGED      3
#define NXC_EVENT_OBJECT_CHANGED       4
#define NXC_EVENT_NOTIFICATION         5
#define NXC_EVENT_DEPLOYMENT_STATUS    6
#define NXC_EVENT_NEW_SYSLOG_RECORD    7
#define NXC_EVENT_NEW_SNMP_TRAP        8
//#define NXC_EVENT_SITUATION_UPDATE     9
#define NXC_EVENT_JOB_CHANGE           10

/**
 * Session states (used both by client library and server)
 */
enum SessionState
{
   STATE_DISCONNECTED    = 0,
   STATE_CONNECTING      = 1,
   STATE_CONNECTED       = 2,
   STATE_AUTHENTICATED   = 3
};

/**
 * Notification codes
 */
#define NX_NOTIFY_SHUTDOWN                   1
#define NX_NOTIFY_RELOAD_EVENT_DB            2
#define NX_NOTIFY_ALARM_DELETED              3
#define NX_NOTIFY_NEW_ALARM                  4
#define NX_NOTIFY_ALARM_CHANGED              5
#define NX_NOTIFY_ACTION_CREATED             6
#define NX_NOTIFY_ACTION_MODIFIED            7
#define NX_NOTIFY_ACTION_DELETED             8
#define NX_NOTIFY_OBJTOOLS_CHANGED           9
#define NX_NOTIFY_DBCONN_STATUS              10
#define NX_NOTIFY_ALARM_TERMINATED           11
#define NX_NOTIFY_GRAPHS_CHANGED             12
#define NX_NOTIFY_ETMPL_CHANGED              13
#define NX_NOTIFY_ETMPL_DELETED              14
#define NX_NOTIFY_OBJTOOL_DELETED            15
#define NX_NOTIFY_TRAPCFG_CREATED            16
#define NX_NOTIFY_TRAPCFG_MODIFIED           17
#define NX_NOTIFY_TRAPCFG_DELETED            18
#define NX_NOTIFY_MAPTBL_CHANGED             19
#define NX_NOTIFY_MAPTBL_DELETED             20
#define NX_NOTIFY_DCISUMTBL_CHANGED          21
#define NX_NOTIFY_DCISUMTBL_DELETED          22
#define NX_NOTIFY_CERTIFICATE_CHANGED        23
#define NX_NOTIFY_ALARM_STATUS_FLOW_CHANGED  24
#define NX_NOTIFY_FILE_LIST_CHANGED          25
#define NX_NOTIFY_FILE_MONITORING_FAILED     26
#define NX_NOTIFY_SESSION_KILLED             27
#define NX_NOTIFY_GRAPHS_DELETED             28
#define NX_NOTIFY_SCHEDULE_UPDATE            29
#define NX_NOTIFY_ALARM_CATEGORY_UPDATED     30
#define NX_NOTIFY_ALARM_CATEGORY_DELETED     31
#define NX_NOTIFY_MULTIPLE_ALARMS_TERMINATED 32
#define NX_NOTIFY_MULTIPLE_ALARMS_RESOLVED   33
#define NX_NOTIFY_FORCE_DCI_POLL             34
#define NX_NOTIFY_DCI_UPDATE                 35
#define NX_NOTIFY_DCI_DELETE                 36
#define NX_NOTIFY_DCI_STATE_CHANGE           37
#define NX_NOTIFY_OBJECTS_OUT_OF_SYNC        38
#define NX_NOTIFY_DCI_STATE_CHANGE           37
#define NX_NOTIFY_POLICY_MODIFIED            39
#define NX_NOTIFY_POLICY_DELETED             40
#define NX_NOTIFY_USER_AGENT_MESSAGE_CHANGED 41
#define NX_NOTIFICATION_CHANNEL_CHANGED      42
#define NX_NOTIFY_PHYSICAL_LINK_UPDATE       43
#define NX_NOTIFY_COMMUNITIES_CONFIG_CHANGED 44
#define NX_NOTIFY_USM_CONFIG_CHANGED         45
#define NX_NOTIFY_PORTS_CONFIG_CHANGED       46
#define NX_NOTIFY_SECRET_CONFIG_CHANGED      47
#define NX_NOTIFY_OBJECT_CATEGORY_UPDATED    48
#define NX_NOTIFY_OBJECT_CATEGORY_DELETED    49
#define NX_NOTIFY_GEO_AREA_UPDATED           50
#define NX_NOTIFY_GEO_AREA_DELETED           51
#define NX_NOTIFY_OBJECTS_IN_SYNC            52
#define NX_NOTIFY_SSH_KEY_DATA_CHAGED        53

/**
 * Request completion codes
 */
#define RCC_SUCCESS                    ((uint32_t)0)
#define RCC_COMPONENT_LOCKED           ((uint32_t)1)
#define RCC_ACCESS_DENIED              ((uint32_t)2)
#define RCC_INVALID_REQUEST            ((uint32_t)3)
#define RCC_TIMEOUT                    ((uint32_t)4)
#define RCC_OUT_OF_STATE_REQUEST       ((uint32_t)5)
#define RCC_DB_FAILURE                 ((uint32_t)6)
#define RCC_INVALID_OBJECT_ID          ((uint32_t)7)
#define RCC_ALREADY_EXIST              ((uint32_t)8)
#define RCC_COMM_FAILURE               ((uint32_t)9)
#define RCC_SYSTEM_FAILURE             ((uint32_t)10)
#define RCC_INVALID_USER_ID            ((uint32_t)11)
#define RCC_INVALID_ARGUMENT           ((uint32_t)12)
#define RCC_DUPLICATE_DCI              ((uint32_t)13)
#define RCC_INVALID_DCI_ID             ((uint32_t)14)
#define RCC_OUT_OF_MEMORY              ((uint32_t)15)
#define RCC_IO_ERROR                   ((uint32_t)16)
#define RCC_INCOMPATIBLE_OPERATION     ((uint32_t)17)
#define RCC_OBJECT_CREATION_FAILED     ((uint32_t)18)
#define RCC_OBJECT_LOOP                ((uint32_t)19)
#define RCC_INVALID_OBJECT_NAME        ((uint32_t)20)
#define RCC_INVALID_ALARM_ID           ((uint32_t)21)
#define RCC_INVALID_ACTION_ID          ((uint32_t)22)
#define RCC_OPERATION_IN_PROGRESS      ((uint32_t)23)
#define RCC_DCI_COPY_ERRORS            ((uint32_t)24)
#define RCC_INVALID_EVENT_CODE         ((uint32_t)25)
#define RCC_NO_WOL_INTERFACES          ((uint32_t)26)
#define RCC_NO_MAC_ADDRESS             ((uint32_t)27)
#define RCC_NOT_IMPLEMENTED            ((uint32_t)28)
#define RCC_INVALID_TRAP_ID            ((uint32_t)29)
#define RCC_DCI_NOT_SUPPORTED          ((uint32_t)30)
#define RCC_VERSION_MISMATCH           ((uint32_t)31)
#define RCC_NPI_PARSE_ERROR            ((uint32_t)32)
#define RCC_DUPLICATE_PACKAGE          ((uint32_t)33)
#define RCC_PACKAGE_FILE_EXIST         ((uint32_t)34)
#define RCC_RESOURCE_BUSY              ((uint32_t)35)
#define RCC_INVALID_PACKAGE_ID         ((uint32_t)36)
#define RCC_INVALID_IP_ADDR            ((uint32_t)37)
#define RCC_ACTION_IN_USE              ((uint32_t)38)
#define RCC_VARIABLE_NOT_FOUND         ((uint32_t)39)
#define RCC_BAD_PROTOCOL               ((uint32_t)40)
#define RCC_ADDRESS_IN_USE             ((uint32_t)41)
#define RCC_NO_CIPHERS                 ((uint32_t)42)
#define RCC_INVALID_PUBLIC_KEY         ((uint32_t)43)
#define RCC_INVALID_SESSION_KEY        ((uint32_t)44)
#define RCC_NO_ENCRYPTION_SUPPORT      ((uint32_t)45)
#define RCC_INTERNAL_ERROR             ((uint32_t)46)
#define RCC_EXEC_FAILED                ((uint32_t)47)
#define RCC_INVALID_TOOL_ID            ((uint32_t)48)
#define RCC_SNMP_ERROR                 ((uint32_t)49)
#define RCC_BAD_REGEXP                 ((uint32_t)50)
#define RCC_UNKNOWN_PARAMETER          ((uint32_t)51)
#define RCC_FILE_IO_ERROR              ((uint32_t)52)
#define RCC_CORRUPTED_MIB_FILE         ((uint32_t)53)
#define RCC_TRANSFER_IN_PROGRESS       ((uint32_t)54)
#define RCC_INVALID_JOB_ID             ((uint32_t)55)
#define RCC_INVALID_SCRIPT_ID          ((uint32_t)56)
#define RCC_INVALID_SCRIPT_NAME        ((uint32_t)57)
#define RCC_UNKNOWN_MAP_NAME           ((uint32_t)58)
#define RCC_INVALID_MAP_ID             ((uint32_t)59)
#define RCC_ACCOUNT_DISABLED           ((uint32_t)60)
#define RCC_NO_GRACE_LOGINS            ((uint32_t)61)
#define RCC_CONNECTION_BROKEN          ((uint32_t)62)
#define RCC_INVALID_CONFIG_ID          ((uint32_t)63)
#define RCC_DB_CONNECTION_LOST         ((uint32_t)64)
#define RCC_ALARM_OPEN_IN_HELPDESK     ((uint32_t)65)
#define RCC_ALARM_NOT_OUTSTANDING      ((uint32_t)66)
#define RCC_NOT_PUSH_DCI               ((uint32_t)67)
#define RCC_CONFIG_PARSE_ERROR         ((uint32_t)68)
#define RCC_CONFIG_VALIDATION_ERROR    ((uint32_t)69)
#define RCC_INVALID_GRAPH_ID           ((uint32_t)70)
#define RCC_LOCAL_CRYPTO_ERROR		   ((uint32_t)71)
#define RCC_UNSUPPORTED_AUTH_TYPE	   ((uint32_t)72)
#define RCC_BAD_CERTIFICATE            ((uint32_t)73)
#define RCC_INVALID_CERT_ID            ((uint32_t)74)
#define RCC_SNMP_FAILURE               ((uint32_t)75)
#define RCC_NO_L2_TOPOLOGY_SUPPORT	   ((uint32_t)76)
#define RCC_INVALID_PSTORAGE_KEY       ((uint32_t)77)
#define RCC_NO_SUCH_INSTANCE           ((uint32_t)78)
#define RCC_INVALID_EVENT_ID           ((uint32_t)79)
#define RCC_AGENT_ERROR                ((uint32_t)80)
#define RCC_UNKNOWN_VARIABLE           ((uint32_t)81)
#define RCC_RESOURCE_NOT_AVAILABLE     ((uint32_t)82)
#define RCC_JOB_CANCEL_FAILED          ((uint32_t)83)
#define RCC_INVALID_POLICY_ID          ((uint32_t)84)
#define RCC_UNKNOWN_LOG_NAME           ((uint32_t)85)
#define RCC_INVALID_LOG_HANDLE         ((uint32_t)86)
#define RCC_WEAK_PASSWORD              ((uint32_t)87)
#define RCC_REUSED_PASSWORD            ((uint32_t)88)
#define RCC_INVALID_SESSION_HANDLE     ((uint32_t)89)
#define RCC_CLUSTER_MEMBER_ALREADY     ((uint32_t)90)
#define RCC_JOB_HOLD_FAILED            ((uint32_t)91)
#define RCC_JOB_UNHOLD_FAILED          ((uint32_t)92)
#define RCC_ZONE_ID_ALREADY_IN_USE     ((uint32_t)93)
#define RCC_INVALID_ZONE_ID            ((uint32_t)94)
#define RCC_ZONE_NOT_EMPTY             ((uint32_t)95)
#define RCC_NO_COMPONENT_DATA          ((uint32_t)96)
#define RCC_INVALID_ALARM_NOTE_ID      ((uint32_t)97)
#define RCC_ENCRYPTION_ERROR           ((uint32_t)98)
#define RCC_INVALID_MAPPING_TABLE_ID   ((uint32_t)99)
#define RCC_NO_SOFTWARE_PACKAGE_DATA   ((uint32_t)100)
#define RCC_INVALID_SUMMARY_TABLE_ID   ((uint32_t)101)
#define RCC_USER_LOGGED_IN             ((uint32_t)102)
#define RCC_XML_PARSE_ERROR            ((uint32_t)103)
#define RCC_HIGH_QUERY_COST            ((uint32_t)104)
#define RCC_LICENSE_VIOLATION          ((uint32_t)105)
#define RCC_CLIENT_LICENSE_EXCEEDED    ((uint32_t)106)
#define RCC_OBJECT_ALREADY_EXISTS      ((uint32_t)107)
#define RCC_NO_HDLINK                  ((uint32_t)108)
#define RCC_HDLINK_COMM_FAILURE        ((uint32_t)109)
#define RCC_HDLINK_ACCESS_DENIED       ((uint32_t)110)
#define RCC_HDLINK_INTERNAL_ERROR      ((uint32_t)111)
#define RCC_NO_LDAP_CONNECTION         ((uint32_t)112)
#define RCC_NO_ROUTING_TABLE           ((uint32_t)113)
#define RCC_NO_FDB                     ((uint32_t)114)
#define RCC_NO_LOCATION_HISTORY        ((uint32_t)115)
#define RCC_OBJECT_IN_USE              ((uint32_t)116)
#define RCC_NXSL_COMPILATION_ERROR     ((uint32_t)117)
#define RCC_NXSL_EXECUTION_ERROR       ((uint32_t)118)
#define RCC_UNKNOWN_CONFIG_VARIABLE    ((uint32_t)119)
#define RCC_UNSUPPORTED_AUTH_METHOD    ((uint32_t)120)
#define RCC_NAME_ALEARDY_EXISTS        ((uint32_t)121)
#define RCC_CATEGORY_IN_USE            ((uint32_t)122)
#define RCC_CATEGORY_NAME_EMPTY        ((uint32_t)123)
#define RCC_AGENT_FILE_DOWNLOAD_ERROR  ((uint32_t)124)
#define RCC_INVALID_TUNNEL_ID          ((uint32_t)125)
#define RCC_FILE_ALREADY_EXISTS        ((uint32_t)126)
#define RCC_FOLDER_ALREADY_EXISTS      ((uint32_t)127)
#define RCC_NO_SUCH_POLICY             ((uint32_t)128)
#define RCC_NO_HARDWARE_DATA           ((uint32_t)129)
#define RCC_CHANNEL_ALREADY_EXIST      ((uint32_t)130)
#define RCC_NO_CHANNEL_NAME            ((uint32_t)131)
#define RCC_CHANNEL_IN_USE             ((uint32_t)132)
#define RCC_INVALID_CHANNEL_NAME       ((uint32_t)133)
#define RCC_ENDPOINT_ALREADY_IN_USE    ((uint32_t)134)
#define RCC_INVALID_DRIVER_NAME        ((uint32_t)135)
#define RCC_INVALID_WEB_SERVICE_ID     ((uint32_t)136)
#define RCC_NO_SUCH_RECORD             ((uint32_t)137)
#define RCC_RECORD_DETAILS_UNAVAILABLE ((uint32_t)138)
#define RCC_INVALID_GEO_AREA_ID        ((uint32_t)139)
#define RCC_GEO_AREA_NAME_EMPTY        ((uint32_t)140)
#define RCC_GEO_AREA_IN_USE            ((uint32_t)141)
#define RCC_INVALID_SSH_KEY_ID         ((uint32_t)142)
#define RCC_SSH_KEY_IN_USE             ((uint32_t)143)

/**
 * Mask bits for NXCModifyEventTemplate()
 */
#define EM_SEVERITY        ((uint32_t)0x01)
#define EM_FLAGS           ((uint32_t)0x02)
#define EM_NAME            ((uint32_t)0x04)
#define EM_MESSAGE         ((uint32_t)0x08)
#define EM_DESCRIPTION     ((uint32_t)0x10)
#define EM_ALL             ((uint32_t)0x1F)

/**
 * Mask bits (flags) for NXCModifyObject()
 */
#define OBJ_UPDATE_NAME             _ULL(0x0000000001)
#define OBJ_UPDATE_AGENT_PORT       _ULL(0x0000000002)
#define OBJ_UPDATE_AGENT_AUTH       _ULL(0x0000000004)
#define OBJ_UPDATE_AGENT_SECRET     _ULL(0x0000000008)
#define OBJ_UPDATE_SNMP_VERSION     _ULL(0x0000000010)
#define OBJ_UPDATE_SNMP_AUTH        _ULL(0x0000000020)
#define OBJ_UPDATE_ACL              _ULL(0x0000000040)
#define OBJ_UPDATE_GEOLOCATION      _ULL(0x0000000080)
#define OBJ_UPDATE_SYNC_NETS        _ULL(0x0000000100)
#define OBJ_UPDATE_SERVICE_TYPE     _ULL(0x0000000200)
#define OBJ_UPDATE_IP_PROTO         _ULL(0x0000000400)
#define OBJ_UPDATE_IP_PORT          _ULL(0x0000000800)
#define OBJ_UPDATE_CHECK_REQUEST    _ULL(0x0000001000)
#define OBJ_UPDATE_CHECK_RESPONSE   _ULL(0x0000002000)
#define OBJ_UPDATE_POLLER_NODE      _ULL(0x0000004000)
#define OBJ_UPDATE_IP_ADDR          _ULL(0x0000008000)
#define OBJ_UPDATE_PEER_GATEWAY     _ULL(0x0000010000)
#define OBJ_UPDATE_NETWORK_LIST     _ULL(0x0000020000)
#define OBJ_UPDATE_STATUS_ALG       _ULL(0x0000040000)
#define OBJ_UPDATE_PROXY_NODE       _ULL(0x0000080000)
#define OBJ_UPDATE_FLAGS            _ULL(0x0000100000)
#define OBJ_UPDATE_ACT_EVENT        _ULL(0x0000200000)
#define OBJ_UPDATE_DEACT_EVENT      _ULL(0x0000400000)
#define OBJ_UPDATE_SOURCE_OBJECT    _ULL(0x0000800000)
#define OBJ_UPDATE_ACTIVE_STATUS    _ULL(0x0001000000)
#define OBJ_UPDATE_INACTIVE_STATUS  _ULL(0x0002000000)
#define OBJ_UPDATE_DCI_LIST         _ULL(0x0004000000)
#define OBJ_UPDATE_SCRIPT           _ULL(0x0008000000)
#define OBJ_UPDATE_CLUSTER_TYPE		_ULL(0x0010000000)
#define OBJ_UPDATE_RESOURCES			_ULL(0x0020000000)
#define OBJ_UPDATE_SNMP_PROXY			_ULL(0x0040000000)
#define OBJ_UPDATE_REQUIRED_POLLS   _ULL(0x0080000000)
#define OBJ_UPDATE_TRUSTED_NODES    _ULL(0x0100000000)
#define OBJ_UPDATE_CUSTOM_ATTRS     _ULL(0x0200000000)
#define OBJ_UPDATE_USE_IFXTABLE     _ULL(0x0400000000)
#define OBJ_UPDATE_AUTOBIND         _ULL(0x1000000000)
#define OBJ_UPDATE_SNMP_PORT        _ULL(0x2000000000)
#define OBJ_UPDATE_PRIMARY_NAME     _ULL(0x4000000000)

/**
 * Global user rights
 */
#ifndef NETXMS_CUSTOM_USER_RIGHTS

#define SYSTEM_ACCESS_MANAGE_USERS            _ULL(0x000000000001)
#define SYSTEM_ACCESS_SERVER_CONFIG           _ULL(0x000000000002)
#define SYSTEM_ACCESS_CONFIGURE_TRAPS         _ULL(0x000000000004)
#define SYSTEM_ACCESS_MANAGE_SESSIONS         _ULL(0x000000000008)
#define SYSTEM_ACCESS_VIEW_EVENT_DB           _ULL(0x000000000010)
#define SYSTEM_ACCESS_EDIT_EVENT_DB           _ULL(0x000000000020)
#define SYSTEM_ACCESS_EPP                     _ULL(0x000000000040)
#define SYSTEM_ACCESS_MANAGE_ACTIONS          _ULL(0x000000000080)
#define SYSTEM_ACCESS_DELETE_ALARMS           _ULL(0x000000000100)
#define SYSTEM_ACCESS_MANAGE_PACKAGES         _ULL(0x000000000200)
#define SYSTEM_ACCESS_VIEW_EVENT_LOG          _ULL(0x000000000400)
#define SYSTEM_ACCESS_MANAGE_TOOLS            _ULL(0x000000000800)
#define SYSTEM_ACCESS_MANAGE_SCRIPTS          _ULL(0x000000001000)
#define SYSTEM_ACCESS_VIEW_TRAP_LOG           _ULL(0x000000002000)
#define SYSTEM_ACCESS_VIEW_AUDIT_LOG          _ULL(0x000000004000)
#define SYSTEM_ACCESS_MANAGE_AGENT_CFG        _ULL(0x000000008000)
#define SYSTEM_ACCESS_PERSISTENT_STORAGE      _ULL(0x000000010000)
#define SYSTEM_ACCESS_SEND_NOTIFICATION       _ULL(0x000000020000)
#define SYSTEM_ACCESS_MOBILE_DEVICE_LOGIN     _ULL(0x000000040000)
#define SYSTEM_ACCESS_REGISTER_AGENTS         _ULL(0x000000080000)
#define SYSTEM_ACCESS_READ_SERVER_FILES       _ULL(0x000000100000)
#define SYSTEM_ACCESS_SERVER_CONSOLE          _ULL(0x000000200000)
#define SYSTEM_ACCESS_MANAGE_SERVER_FILES     _ULL(0x000000400000)
#define SYSTEM_ACCESS_MANAGE_MAPPING_TBLS     _ULL(0x000000800000)
#define SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS     _ULL(0x000001000000)
#define SYSTEM_ACCESS_REPORTING_SERVER        _ULL(0x000002000000)
#define SYSTEM_ACCESS_XMPP_COMMANDS           _ULL(0x000004000000)
#define SYSTEM_ACCESS_MANAGE_IMAGE_LIB        _ULL(0x000008000000)
#define SYSTEM_ACCESS_UNLINK_ISSUES           _ULL(0x000010000000)
#define SYSTEM_ACCESS_VIEW_SYSLOG             _ULL(0x000020000000)
#define SYSTEM_ACCESS_USER_SCHEDULED_TASKS    _ULL(0x000040000000)
#define SYSTEM_ACCESS_OWN_SCHEDULED_TASKS     _ULL(0x000080000000)
#define SYSTEM_ACCESS_ALL_SCHEDULED_TASKS     _ULL(0x000100000000)
#define SYSTEM_ACCESS_SCHEDULE_SCRIPT         _ULL(0x000200000000)
#define SYSTEM_ACCESS_SCHEDULE_FILE_UPLOAD    _ULL(0x000400000000)
#define SYSTEM_ACCESS_SCHEDULE_MAINTENANCE    _ULL(0x000800000000)
#define SYSTEM_ACCESS_MANAGE_REPOSITORIES     _ULL(0x001000000000)
#define SYSTEM_ACCESS_VIEW_REPOSITORIES       _ULL(0x002000000000)
#define SYSTEM_ACCESS_VIEW_ALL_ALARMS         _ULL(0x004000000000)
#define SYSTEM_ACCESS_EXTERNAL_INTEGRATION    _ULL(0x008000000000)
#define SYSTEM_ACCESS_SETUP_TCP_PROXY         _ULL(0x010000000000)
#define SYSTEM_ACCESS_IMPORT_CONFIGURATION    _ULL(0x020000000000)
#define SYSTEM_ACCESS_UA_NOTIFICATIONS        _ULL(0x040000000000)
#define SYSTEM_ACCESS_WEB_SERVICE_DEFINITIONS _ULL(0x080000000000)
#define SYSTEM_ACCESS_OBJECT_CATEGORIES       _ULL(0x100000000000)
#define SYSTEM_ACCESS_MANAGE_GEO_AREAS        _ULL(0x200000000000)
#define SYSTEM_ACCESS_SSH_KEY_CONFIGURATION   _ULL(0x400000000000)

#define SYSTEM_ACCESS_FULL                    _ULL(0x7FFFFFFFFFFF)

#endif	/* NETXMS_CUSTOM_USER_RIGHTS */

/**
 * Object access rights
 */
#define OBJECT_ACCESS_READ          0x00000001
#define OBJECT_ACCESS_MODIFY        0x00000002
#define OBJECT_ACCESS_CREATE        0x00000004
#define OBJECT_ACCESS_DELETE        0x00000008
#define OBJECT_ACCESS_READ_ALARMS   0x00000010
#define OBJECT_ACCESS_ACL           0x00000020
#define OBJECT_ACCESS_UPDATE_ALARMS 0x00000040
#define OBJECT_ACCESS_SEND_EVENTS   0x00000080
#define OBJECT_ACCESS_CONTROL       0x00000100
#define OBJECT_ACCESS_TERM_ALARMS   0x00000200
#define OBJECT_ACCESS_PUSH_DATA     0x00000400
#define OBJECT_ACCESS_CREATE_ISSUE  0x00000800
#define OBJECT_ACCESS_DOWNLOAD      0x00001000
#define OBJECT_ACCESS_UPLOAD        0x00002000
#define OBJECT_ACCESS_MANAGE_FILES  0x00004000
#define OBJECT_ACCESS_MAINTENANCE   0x00008000
#define OBJECT_ACCESS_READ_AGENT    0x00010000
#define OBJECT_ACCESS_READ_SNMP     0x00020000
#define OBJECT_ACCESS_SCREENSHOT    0x00040000

/**
 * Object sync flags
 */
#define OBJECT_SYNC_SEND_UPDATES    0x0001
#define OBJECT_SYNC_DUAL_CONFIRM    0x0002

/**
 * User/group flags
 */
#define UF_MODIFIED                 0x0001
#define UF_DELETED                  0x0002
#define UF_DISABLED                 0x0004
#define UF_CHANGE_PASSWORD          0x0008
#define UF_CANNOT_CHANGE_PASSWORD   0x0010
#define UF_INTRUDER_LOCKOUT         0x0020
#define UF_PASSWORD_NEVER_EXPIRES   0x0040
#define UF_LDAP_USER                0x0080
#define UF_SYNC_EXCEPTION           0x0100
#define UF_CLOSE_OTHER_SESSIONS     0x0200

/**
 * Fields for NXCModifyUserEx
 */
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
#define USER_MODIFY_XMPP_ID            0x00000800
#define USER_MODIFY_GROUP_MEMBERSHIP   0x00001000
#define USER_MODIFY_EMAIL              0x00002000
#define USER_MODIFY_PHONE_NUMBER       0x00004000

/**
 * Certificate mapping methods
 */
enum CertificateMappingMethod
{
   MAP_CERTIFICATE_BY_SUBJECT     = 0,
   MAP_CERTIFICATE_BY_PUBKEY      = 1,
   MAP_CERTIFICATE_BY_CN          = 2,
   MAP_CERTIFICATE_BY_TEMPLATE_ID = 3
};

/**
 * User database change notification types
 */
#define USER_DB_CREATE              0
#define USER_DB_DELETE              1
#define USER_DB_MODIFY              2

/**
 * Data collection object types
 */
#define DCO_TYPE_GENERIC   0
#define DCO_TYPE_ITEM      1
#define DCO_TYPE_TABLE     2
#define DCO_TYPE_LIST      3

enum HistoricalDataType
{
   DCO_TYPE_PROCESSED = 0,
   DCO_TYPE_RAW = 1,
   DCO_TYPE_BOTH = 2
};

/**
 * DCI flags
 */
#define DCF_ALL_THRESHOLDS           ((uint32_t)0x00002)
#define DCF_RAW_VALUE_OCTET_STRING   ((uint32_t)0x00004)
#define DCF_SHOW_ON_OBJECT_TOOLTIP   ((uint32_t)0x00008)
#define DCF_AGGREGATE_FUNCTION_MASK  ((uint32_t)0x00070)
#define DCF_AGGREGATE_ON_CLUSTER     ((uint32_t)0x00080)
#define DCF_TRANSFORM_AGGREGATED     ((uint32_t)0x00100)
#define DCF_CALCULATE_NODE_STATUS    ((uint32_t)0x00400)
#define DCF_SHOW_IN_OBJECT_OVERVIEW  ((uint32_t)0x00800)
#define DCF_CACHE_MODE_MASK          ((uint32_t)0x03000)
#define DCF_AGGREGATE_WITH_ERRORS    ((uint32_t)0x04000)
#define DCF_HIDE_ON_LAST_VALUES_PAGE ((uint32_t)0x08000)
#define DCF_MULTIPLIERS_MASK         ((uint32_t)0x30000)

/**
 * Get cluster aggregation function from DCI flags
 */
#define DCF_GET_AGGREGATION_FUNCTION(flags) (((flags) & DCF_AGGREGATE_FUNCTION_MASK) >> 4)

/**
 * Set cluster aggregation function in DCI flags
 */
#define DCF_SET_AGGREGATION_FUNCTION(flags,func) (((flags) & ~DCF_AGGREGATE_FUNCTION_MASK) | (((func) & 0x07) << 4))

/**
 * Get cache mode from DCI flags
 */
#define DCF_GET_CACHE_MODE(flags) (((flags) & DCF_CACHE_MODE_MASK) >> 12)

/**
 * Set cache mode in DCI flags
 */
#define DCF_SET_CACHE_MODE(flags,mode) (((flags) & ~DCF_CACHE_MODE_MASK) | (((mode) & 0x03) << 12))

/**
 * DCTable column flags
 */
#define TCF_DATA_TYPE_MASK          ((uint16_t)0x000F)
#define TCF_AGGREGATE_FUNCTION_MASK ((uint16_t)0x0070)
#define TCF_INSTANCE_COLUMN         ((uint16_t)0x0100)
#define TCF_INSTANCE_LABEL_COLUMN   ((uint16_t)0x0200)
#define TCF_SNMP_HEX_STRING         ((uint16_t)0x0400)

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
#define DS_WEB_SERVICE        3
#define DS_PUSH_AGENT         4
#define DS_WINPERF            5
#define DS_SMCLP              6
#define DS_SCRIPT             7
#define DS_SSH                8
#define DS_MQTT               9
#define DS_DEVICE_DRIVER      10

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
#define F_SCRIPT     6

/**
 * DCI aggregation functions
 */
enum AggregationFunction
{
   DCI_AGG_LAST = 0,
   DCI_AGG_MIN = 1,
   DCI_AGG_MAX = 2,
   DCI_AGG_AVG = 3,
   DCI_AGG_SUM = 4
};

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
#define IDM_SCRIPT                      5
#define IDM_WINPERF                     6
#define IDM_WEB_SERVICE                 7
#define IDM_INTERNAL_TABLE              8

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
#define RF_CREATE_TICKET         0x2000

/**
 * Action types
 */
#define ACTION_EXEC           0
#define ACTION_REMOTE         1
//#define ACTION_SEND_EMAIL     2 removed
#define ACTION_NOTIFICATION   3
#define ACTION_FORWARD_EVENT  4
#define ACTION_NXSL_SCRIPT    5
#define ACTION_XMPP_MESSAGE   6

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
#define MF_FILTER_OBJECTS        0x00000020

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
 * Core subscription channels
 */
#define NXC_CHANNEL_ALARMS          _T("Core.Alarms")
#define NXC_CHANNEL_AUDIT_LOG       _T("Core.Audit")
#define NXC_CHANNEL_DC_THRESHOLDS   _T("Core.DC.Thresholds")
#define NXC_CHANNEL_EVENTS          _T("Core.Events")
#define NXC_CHANNEL_GEO_AREAS       _T("Core.GeoAreas")
#define NXC_CHANNEL_OBJECTS         _T("Core.Objects")
#define NXC_CHANNEL_SNMP_TRAPS      _T("Core.SNMP.Traps")
#define NXC_CHANNEL_SYSLOG          _T("Core.Syslog")
#define NXC_CHANNEL_USERDB          _T("Core.UserDB")

/**
 * Node creation flags
 */
#define NXC_NCF_DISABLE_ICMP         0x0001
#define NXC_NCF_DISABLE_NXCP         0x0002
#define NXC_NCF_DISABLE_SNMP         0x0004
#define NXC_NCF_CREATE_UNMANAGED     0x0008
#define NXC_NCF_ENTER_MAINTENANCE    0x0010
#define NXC_NCF_AS_ZONE_PROXY        0x0020
#define NXC_NCF_DISABLE_ETHERNET_IP  0x0040
#define NXC_NCF_SNMP_SETTINGS_LOCKED 0x0080
#define NXC_NCF_EXTERNAL_GATEWAY     0x0100

/**
 * Agent data cache modes
 */
#define AGENT_CACHE_DEFAULT      0
#define AGENT_CACHE_ON           1
#define AGENT_CACHE_OFF          2

/**
 * Server components
 */
#define SRV_COMPONENT_DISCOVERY_MGR    1

/**
 * Configuration import flags
 */
#define CFG_IMPORT_REPLACE_EVENTS                     0x0001
#define CFG_IMPORT_REPLACE_ACTIONS                    0x0002
#define CFG_IMPORT_REPLACE_TEMPLATES                  0x0004
#define CFG_IMPORT_REPLACE_TRAPS                      0x0008
#define CFG_IMPORT_REPLACE_SCRIPTS                    0x0010
#define CFG_IMPORT_REPLACE_SUMMARY_TABLES             0x0020
#define CFG_IMPORT_REPLACE_OBJECT_TOOLS               0x0040
#define CFG_IMPORT_REPLACE_EPP_RULES                  0x0080
#define CFG_IMPORT_REPLACE_TEMPLATE_NAMES_LOCATIONS   0x0100
#define CFG_IMPORT_DELETE_EMPTY_TEMPLATE_GROUPS       0x0200
#define CFG_IMPORT_REPLACE_WEB_SVCERVICE_DEFINITIONS  0x0400
#define CFG_IMPORT_REPLACE_EVERYTHING                 0x07FF

/**
* Alarm category flags
*/
#define ALARM_MODIFY_CATEGORY      0x0001
#define ALARM_MODIFY_ACCESS_LIST   0x0002

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
 * DCI resolve flags
 */
#define DCI_RES_SEARCH_NAME   0x01

#ifdef __cplusplus

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
 * Alarm structure
 */
typedef struct
{
   UINT64 sourceEventId;   // Originating event ID
   UINT32 alarmId;         // Unique alarm ID
   UINT32 creationTime;    // Alarm creation time in UNIX time format
   UINT32 lastChangeTime;  // Alarm's last change time in UNIX time format
   UINT32 sourceObject;    // Source object ID
   UINT32 sourceEventCode; // Originating event code
   UINT32 dciId;           // related DCI ID
   BYTE currentSeverity;   // Alarm's current severity
   BYTE originalSeverity;  // Alarm's original severity
   BYTE state;             // Current state
   BYTE helpDeskState;     // State of alarm in helpdesk system
   UINT32 ackByUser;       // ID of user who was acknowledged this alarm (0 for system)
	UINT32 resolvedByUser;  // ID of user who was resolved this alarm (0 for system)
   UINT32 termByUser;      // ID of user who was terminated this alarm (0 for system)
   UINT32 repeatCount;
	UINT32 timeout;
	UINT32 timeoutEvent;
   TCHAR message[MAX_EVENT_MSG_LENGTH];
   TCHAR key[MAX_DB_STRING];
   TCHAR helpDeskRef[MAX_HELPDESK_REF_LEN];
   void *userData;         // Can be freely used by client application
	UINT32 noteCount;       // Number of notes added to alarm
   UINT32 ackTimeout;      // Sticky acknowledgment end time. If acknowladgmant without timeout put 0
} NXC_ALARM;

/**
 * Condition's input DCI definition
 */
typedef struct
{
   uint32_t id;
   uint32_t nodeId;
   int function;    // Average, last, diff
   int polls;       // Number of polls used for average
} INPUT_DCI;

/**
 * Cluster resource
 */
typedef struct
{
   uint32_t dwId;
	TCHAR szName[MAX_DB_STRING];
	InetAddress ipAddr;
	uint32_t dwCurrOwner;
} CLUSTER_RESOURCE;

#endif	/* __cplusplus */

#endif   /* _nxcldefs_h_ */
