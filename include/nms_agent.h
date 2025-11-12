/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: nms_agent.h
**
**/

#ifndef _nms_agent_h_
#define _nms_agent_h_

#ifdef LIBNXAGENT_EXPORTS
#define LIBNXAGENT_EXPORTABLE __EXPORT
#else
#define LIBNXAGENT_EXPORTABLE __IMPORT
#endif


#include <nms_common.h>
#include <nms_util.h>
#include <nxconfig.h>
#include <nxdbapi.h>
#include <nxcpapi.h>
#include <nxproc.h>

/**
 * Initialization function declaration macro
 */
#if defined(_STATIC_AGENT)
#define DECLARE_SUBAGENT_ENTRY_POINT(name) extern "C" bool NxSubAgentRegister_##name(NETXMS_SUBAGENT_INFO **ppInfo, Config *config)
#else
#define DECLARE_SUBAGENT_ENTRY_POINT(name) extern "C" __EXPORT bool NxSubAgentRegister(NETXMS_SUBAGENT_INFO **ppInfo, Config *config)
#endif

/**
 * Constants
 */
#define AGENT_LISTEN_PORT        4700
#define AGENT_TUNNEL_PORT        4703
#define AGENT_PROTOCOL_VERSION   2
#define MAX_RUNTIME_PARAM_NAME   1024  /* maximum possible parameter name in runtime (i.e. with arguments) */
#define MAX_RESULT_LENGTH        256
#define MAX_CMD_LEN              256
#define COMMAND_TIMEOUT          60
#define MAX_SUBAGENT_NAME        64
#define MAX_INSTANCE_COLUMNS     8
#define ZONE_PROXY_KEY_LENGTH    16
#define HARDWARE_ID_LENGTH       SHA1_DIGEST_SIZE
#define SYSTEM_ID_LENGTH         SHA1_DIGEST_SIZE
#define SSH_PORT                 22

#ifndef MODBUS_TCP_DEFAULT_PORT
#define MODBUS_TCP_DEFAULT_PORT  502
#endif

/**
 * Agent policy types
 */
#define AGENT_POLICY_CONFIG      1
#define AGENT_POLICY_LOG_PARSER  2

/**
 * Agent data subdirectories
 */
#define SUBDIR_LOGPARSER_POLICY   _T("logparser_ap")
#define SUBDIR_USERAGENT_POLICY   _T("useragent_ap")
#define SUBDIR_CONFIG_POLICY      _T("config_ap")
#define SUBDIR_CERTIFICATES       _T("certificates")
#define SUBDIR_CRL                _T("crl")

/**
 * Error codes
 */
#define ERR_SUCCESS                    ((uint32_t)0)
#define ERR_PROCESSING                 ((uint32_t)102)
#define ERR_UNKNOWN_COMMAND            ((uint32_t)400)
#define ERR_AUTH_REQUIRED              ((uint32_t)401)
#define ERR_ACCESS_DENIED              ((uint32_t)403)
#define ERR_UNKNOWN_METRIC             ((uint32_t)404)
#define ERR_UNSUPPORTED_METRIC         ((uint32_t)406)
#define ERR_REQUEST_TIMEOUT            ((uint32_t)408)
#define ERR_AUTH_FAILED                ((uint32_t)440)
#define ERR_ALREADY_AUTHENTICATED      ((uint32_t)441)
#define ERR_AUTH_NOT_REQUIRED          ((uint32_t)442)
#define ERR_INTERNAL_ERROR             ((uint32_t)500)
#define ERR_NOT_IMPLEMENTED            ((uint32_t)501)
#define ERR_OUT_OF_RESOURCES           ((uint32_t)503)
#define ERR_NOT_CONNECTED              ((uint32_t)900)
#define ERR_CONNECTION_BROKEN          ((uint32_t)901)
#define ERR_BAD_RESPONSE               ((uint32_t)902)
#define ERR_IO_FAILURE                 ((uint32_t)903)
#define ERR_RESOURCE_BUSY              ((uint32_t)904)
#define ERR_EXEC_FAILED                ((uint32_t)905)
#define ERR_ENCRYPTION_REQUIRED        ((uint32_t)906)
#define ERR_NO_CIPHERS                 ((uint32_t)907)
#define ERR_INVALID_PUBLIC_KEY         ((uint32_t)908)
#define ERR_INVALID_SESSION_KEY        ((uint32_t)909)
#define ERR_CONNECT_FAILED             ((uint32_t)910)
#define ERR_MALFORMED_COMMAND          ((uint32_t)911)
#define ERR_SOCKET_ERROR               ((uint32_t)912)
#define ERR_BAD_ARGUMENTS              ((uint32_t)913)
#define ERR_SUBAGENT_LOAD_FAILED       ((uint32_t)914)
#define ERR_FILE_OPEN_ERROR            ((uint32_t)915)
#define ERR_FILE_STAT_FAILED           ((uint32_t)916)
#define ERR_MEM_ALLOC_FAILED           ((uint32_t)917)
#define ERR_FILE_DELETE_FAILED         ((uint32_t)918)
#define ERR_NO_SESSION_AGENT           ((uint32_t)919)
#define ERR_SERVER_ID_UNSET            ((uint32_t)920)
#define ERR_NO_SUCH_INSTANCE           ((uint32_t)921)
#define ERR_OUT_OF_STATE_REQUEST       ((uint32_t)922)
#define ERR_ENCRYPTION_ERROR           ((uint32_t)923)
#define ERR_MALFORMED_RESPONSE         ((uint32_t)924)
#define ERR_INVALID_OBJECT             ((uint32_t)925)
#define ERR_FILE_ALREADY_EXISTS        ((uint32_t)926)
#define ERR_FOLDER_ALREADY_EXISTS      ((uint32_t)927)
#define ERR_INVALID_SSH_KEY_ID         ((uint32_t)928)
#define ERR_AGENT_DB_FAILURE           ((uint32_t)929)
#define ERR_INVALID_HTTP_REQUEST_CODE  ((uint32_t)930)
#define ERR_FILE_HASH_MISMATCH         ((uint32_t)931)
#define ERR_FUNCTION_NOT_SUPPORTED     ((uint32_t)932)
#define ERR_FILE_APPEND_POSSIBLE       ((uint32_t)933)
#define ERR_REMOTE_CONNECT_FAILED      ((uint32_t)934)
#define ERR_SYSCALL_FAILED             ((uint32_t)935)
#define ERR_TCP_PROXY_DISABLED         ((uint32_t)936)

/**
 * Bulk data reconciliation DCI processing status codes
 */
#define BULK_DATA_REC_RETRY      0
#define BULK_DATA_REC_SUCCESS    1
#define BULK_DATA_REC_FAILURE    2

/**
 * Max bulk data block size
 */
#define MAX_BULK_DATA_BLOCK_SIZE 8192

/**
 * Parameter handler return codes
 */
#define SYSINFO_RC_SUCCESS             0
#define SYSINFO_RC_UNSUPPORTED         1
#define SYSINFO_RC_ERROR               2
#define SYSINFO_RC_NO_SUCH_INSTANCE    3
#define SYSINFO_RC_UNKNOWN             4
#define SYSINFO_RC_ACCESS_DENIED       5

/**
 * WinPerf features
 */
#define WINPERF_AUTOMATIC_SAMPLE_COUNT    ((uint32_t)0x00000001)
#define WINPERF_REMOTE_COUNTER_CONFIG     ((uint32_t)0x00000002)

/**
 * User session states (used by session agents)
 */
#define USER_SESSION_ACTIVE         0
#define USER_SESSION_CONNECTED      1
#define USER_SESSION_DISCONNECTED   2
#define USER_SESSION_IDLE           3
#define USER_SESSION_OTHER          4

/**
 * Web service authentication type
 */
enum class WebServiceAuthType
{
   NONE = 0,
   BASIC = 1,
   DIGEST = 2,
   NTLM = 3,
   BEARER = 4,
   ANY = 5,
   ANYSAFE = 6
};

/**
 * Web service request type
 */
enum class WebServiceRequestType
{
   PARAMETER = 0,
   LIST = 1
};

/**
 * HTTP request method
 */
enum class HttpRequestMethod
{
   _GET = 0,
   _POST = 1,
   _PUT = 2,
   _DELETE = 3,
   _PATCH = 4,
   _MAX_TYPE = _PATCH
};

/**
 * Safely get authentication type from integer value
 */
static inline WebServiceAuthType WebServiceAuthTypeFromInt(int type)
{
   return ((type >= static_cast<int>(WebServiceAuthType::NONE)) &&
           (type <= static_cast<int>(WebServiceAuthType::ANYSAFE))) ? static_cast<WebServiceAuthType>(type) : WebServiceAuthType::NONE;
}

/**
 * Safely get HTTP request method from integer value
 */
static inline HttpRequestMethod HttpRequestMethodFromInt(int type)
{
   return ((type >= static_cast<int>(HttpRequestMethod::_GET)) &&
           (type <= static_cast<int>(HttpRequestMethod::_MAX_TYPE))) ? static_cast<HttpRequestMethod>(type) : HttpRequestMethod::_GET;
}

/**
 * Policy change notification
 */
struct PolicyChangeNotification
{
   uuid guid;
   const TCHAR *type;
   bool sameContent;
};

/**
 * Agent component token
 */
#pragma pack(1)
struct AgentComponentToken
{
   char component[16];
   uint64_t expirationTime;   // In network byte order
   uint64_t issuingTime;      // In network byte order
   char padding[32];          // Can be used in the future for additional fields without changing structure size
   BYTE hmac[SHA256_DIGEST_SIZE];
};
#pragma pack()

/**
 * Validate token
 */
static inline bool ValidateComponentToken(const AgentComponentToken *token, const char *secret)
{
   return ValidateMessageSignature(token, sizeof(AgentComponentToken) - SHA256_DIGEST_SIZE,
      reinterpret_cast<const BYTE*>(secret), strlen(secret), token->hmac);
}

/**
 * Internal notification codes
 */
#define AGENT_NOTIFY_POLICY_INSTALLED  1
#define AGENT_NOTIFY_TOKEN_RECEIVED    2

/**
 * Descriptions for common parameters
 */
#define DCIDESC_AGENT_ACCEPTEDCONNECTIONS            _T("Agent: accepted connections")
#define DCIDESC_AGENT_ACCEPTERRORS                   _T("Agent: accept() call errors")
#define DCIDESC_AGENT_ACCESSLEVEL                    _T("Agent: access level")
#define DCIDESC_AGENT_ACTIVECONNECTIONS              _T("Agent: active connections")
#define DCIDESC_AGENT_AUTHENTICATIONFAILURES         _T("Agent: authentication failures")
#define DCIDESC_AGENT_CONFIG_LOAD_STATUS             _T("Agent: configuration load status")
#define DCIDESC_AGENT_CONFIG_SERVER                  _T("Configuration server address set on agent startup")
#define DCIDESC_AGENT_DATACOLLQUEUESIZE              _T("Agent data collector queue size")
#define DCIDESC_AGENT_FAILEDREQUESTS                 _T("Agent: failed requests to agent")
#define DCIDESC_AGENT_FILEHANDLELIMIT                _T("Agent: maximum number of open file handles")
#define DCIDESC_AGENT_EVENTS_GENERATED               _T("Agent: generated events")
#define DCIDESC_AGENT_EVENTS_LAST_TIMESTAMP          _T("Agent: timestamp of last generated event")
#define DCIDESC_AGENT_HEAP_ACTIVE                    _T("Agent: active heap memory")
#define DCIDESC_AGENT_HEAP_ALLOCATED                 _T("Agent: allocated heap memory")
#define DCIDESC_AGENT_HEAP_MAPPED                    _T("Agent: mapped heap memory")
#define DCIDESC_AGENT_ID                             _T("Agent identifier")
#define DCIDESC_AGENT_IS_EXT_SUBAGENT_CONNECTED      _T("Check if external subagent {instance} is connected")
#define DCIDESC_AGENT_IS_RESTART_PENDING             _T("Check if agent restart is pending")
#define DCIDESC_AGENT_IS_SUBAGENT_LOADED             _T("Check if subagent {instance} is loaded")
#define DCIDESC_AGENT_IS_USERAGENT_INSTALLED         _T("Check if user support application is installed")
#define DCIDESC_AGENT_LOCALDB_FAILED_QUERIES         _T("Agent local database: failed queries")
#define DCIDESC_AGENT_LOCALDB_FILESIZE               _T("Agent local database: file size")
#define DCIDESC_AGENT_LOCALDB_SLOW_QUERIES           _T("Agent local database: long running queries")
#define DCIDESC_AGENT_LOCALDB_STATUS                 _T("Agent local database: status")
#define DCIDESC_AGENT_LOCALDB_TOTAL_QUERIES          _T("Agent local database: total queries executed")
#define DCIDESC_AGENT_LOG_STATUS                     _T("Agent: log status")
#define DCIDESC_AGENT_NOTIFICATIONPROC_QUEUESIZE     _T("Agent: notification processor queue size")
#define DCIDESC_AGENT_PROCESSEDREQUESTS              _T("Agent: processed requests")
#define DCIDESC_AGENT_PROXY_ACTIVESESSIONS           _T("Agent: active proxy sessions")
#define DCIDESC_AGENT_PROXY_CONNECTIONREQUESTS       _T("Agent: proxy connection requests")
#define DCIDESC_AGENT_PROXY_ISENABLED                _T("Check if agent proxy is enabled")
#define DCIDESC_AGENT_PUSH_VALUE                     _T("Cached value of push parameter {instance}")
#define DCIDESC_AGENT_REGISTRAR                      _T("Registrar server address set on agent startup")
#define DCIDESC_AGENT_REJECTEDCONNECTIONS            _T("Agent: rejected connections")
#define DCIDESC_AGENT_SESSION_AGENTS_COUNT           _T("Agent: connected session agents")
#define DCIDESC_AGENT_SNMP_ISPROXYENABLED            _T("Check if SNMP proxy is enabled")
#define DCIDESC_AGENT_SNMP_REQUESTS                  _T("Number of SNMP requests sent")
#define DCIDESC_AGENT_SNMP_RESPONSES                 _T("Number of SNMP responses received")
#define DCIDESC_AGENT_SNMP_SERVERREQUESTS            _T("Number of SNMP proxy requests received from server")
#define DCIDESC_AGENT_SNMP_TRAPS                     _T("Number of SNMP traps received")
#define DCIDESC_AGENT_SOURCEPACKAGESUPPORT           _T("Check if source packages are supported")
#define DCIDESC_AGENT_SUPPORTEDCIPHERS               _T("List of ciphers supported by agent")
#define DCIDESC_AGENT_SYSLOGPROXY_ISENABLED          _T("Check if syslog proxy is enabled")
#define DCIDESC_AGENT_SYSLOGPROXY_RECEIVEDMSGS       _T("Agent: received syslog messages")
#define DCIDESC_AGENT_TCPPROXY_CONNECTIONREQUESTS    _T("Agent: TCP proxy connection requests")
#define DCIDESC_AGENT_TCPPROXY_ISENABLED             _T("Check if TCP proxy is enabled")
#define DCIDESC_AGENT_THREADPOOL_ACTIVEREQUESTS      _T("Agent thread pool {instance}: active requests")
#define DCIDESC_AGENT_THREADPOOL_AVERAGEWAITTIME     _T("Agent thread pool {instance}: average wait time")
#define DCIDESC_AGENT_THREADPOOL_CURRSIZE            _T("Agent thread pool {instance}: current size")
#define DCIDESC_AGENT_THREADPOOL_LOAD                _T("Agent thread pool {instance}: current load")
#define DCIDESC_AGENT_THREADPOOL_LOADAVG             _T("Agent thread pool {instance}: load average (1 minute)")
#define DCIDESC_AGENT_THREADPOOL_LOADAVG_5           _T("Agent thread pool {instance}: load average (5 minutes)")
#define DCIDESC_AGENT_THREADPOOL_LOADAVG_15          _T("Agent thread pool {instance}: load average (15 minutes)")
#define DCIDESC_AGENT_THREADPOOL_MAXSIZE             _T("Agent thread pool {instance}: max size")
#define DCIDESC_AGENT_THREADPOOL_MINSIZE             _T("Agent thread pool {instance}: min size")
#define DCIDESC_AGENT_THREADPOOL_SCHEDULEDREQUESTS   _T("Agent thread pool {instance}: scheduled requests")
#define DCIDESC_AGENT_THREADPOOL_USAGE               _T("Agent thread pool {instance}: usage")
#define DCIDESC_AGENT_TIMEDOUTREQUESTS               _T("Number of timed out requests to agent")
#define DCIDESC_AGENT_UNSUPPORTEDREQUESTS            _T("Number of requests for unsupported parameters")
#define DCIDESC_AGENT_UPTIME                         _T("Agent's uptime")
#define DCIDESC_AGENT_USER_AGENTS_COUNT              _T("Number of connected user agents")
#define DCIDESC_AGENT_VERSION                        _T("Agent's version")
#define DCIDESC_AGENT_WEBSVCPROXY_ISENABLED          _T("Check if web service proxy is enabled")
#define DCIDESC_FILE_COUNT                           _T("Number of files {instance}")
#define DCIDESC_FILE_FOLDERCOUNT                     _T("Number of folders {instance}")
#define DCIDESC_FILE_HASH_CRC32                      _T("CRC32 checksum of {instance}")
#define DCIDESC_FILE_HASH_MD5                        _T("MD5 hash of {instance}")
#define DCIDESC_FILE_HASH_SHA1                       _T("SHA1 hash of {instance}")
#define DCIDESC_FILE_HASH_SHA256                     _T("SHA256 hash of {instance}")
#define DCIDESC_FILE_SIZE                            _T("Size of file {instance}")
#define DCIDESC_FILE_TIME_ACCESS                     _T("Time of last access to file {instance}")
#define DCIDESC_FILE_TIME_CHANGE                     _T("Time of last status change of file {instance}")
#define DCIDESC_FILE_TIME_MODIFY                     _T("Time of last modification of file {instance}")
#define DCIDESC_FS_AVAIL                             _T("Available space on file system {instance}")
#define DCIDESC_FS_AVAILINODES                       _T("Available inodes on file system {instance}")
#define DCIDESC_FS_AVAILINODESPERC                   _T("Percentage of available inodes on file system {instance}")
#define DCIDESC_FS_AVAILPERC                         _T("Percentage of available space on file system {instance}")
#define DCIDESC_FS_FREE                              _T("Free space on file system {instance}")
#define DCIDESC_FS_FREEINODES                        _T("Free inodes on file system {instance}")
#define DCIDESC_FS_FREEINODESPERC                    _T("Percentage of free inodes on file system {instance}")
#define DCIDESC_FS_FREEPERC                          _T("Percentage of free space on file system {instance}")
#define DCIDESC_FS_TOTAL                             _T("Total space on file system {instance}")
#define DCIDESC_FS_TOTALINODES                       _T("Total inodes on file system {instance}")
#define DCIDESC_FS_TYPE                              _T("Type of file system {instance}")
#define DCIDESC_FS_USED                              _T("Used space on file system {instance}")
#define DCIDESC_FS_USEDINODES                        _T("Used inodes on file system {instance}")
#define DCIDESC_FS_USEDPERC                          _T("Percentage of used space on file system {instance}")
#define DCIDESC_FS_USEDINODESPERC                    _T("Percentage of used inodes on file system {instance}")
#define DCIDESC_HARDWARE_BASEBOARD_MANUFACTURER      _T("Hardware: baseboard manufacturer")
#define DCIDESC_HARDWARE_BASEBOARD_PRODUCT           _T("Hardware: baseboard product name")
#define DCIDESC_HARDWARE_BASEBOARD_SERIALNUMBER      _T("Hardware: baseboard serial number")
#define DCIDESC_HARDWARE_BASEBOARD_TYPE              _T("Hardware: baseboard type")
#define DCIDESC_HARDWARE_BASEBOARD_VERSION           _T("Hardware: baseboard version")
#define DCIDESC_HARDWARE_BATTERY_CAPACITY            _T("Hardware: battery {instance} capacity")
#define DCIDESC_HARDWARE_BATTERY_CHEMISTRY           _T("Hardware: battery {instance} chemistry")
#define DCIDESC_HARDWARE_BATTERY_LOCATION            _T("Hardware: battery {instance} location")
#define DCIDESC_HARDWARE_BATTERY_MANUFACTURE_DATE    _T("Hardware: battery {instance} manufacture date")
#define DCIDESC_HARDWARE_BATTERY_MANUFACTURER        _T("Hardware: battery {instance} manufacturer")
#define DCIDESC_HARDWARE_BATTERY_NAME                _T("Hardware: battery {instance} name")
#define DCIDESC_HARDWARE_BATTERY_SERIALNUMBER        _T("Hardware: battery {instance} serial number")
#define DCIDESC_HARDWARE_BATTERY_VOLTAGE             _T("Hardware: battery {instance} voltage")
#define DCIDESC_HARDWARE_MEMORYDEVICE_BANK           _T("Hardware: memory device {instance} bank")
#define DCIDESC_HARDWARE_MEMORYDEVICE_CONFSPEED      _T("Hardware: memory device {instance} configured speed")
#define DCIDESC_HARDWARE_MEMORYDEVICE_FORMFACTOR     _T("Hardware: memory device {instance} form factor")
#define DCIDESC_HARDWARE_MEMORYDEVICE_LOCATION       _T("Hardware: memory device {instance} location")
#define DCIDESC_HARDWARE_MEMORYDEVICE_MANUFACTURER   _T("Hardware: memory device {instance} manufacturer")
#define DCIDESC_HARDWARE_MEMORYDEVICE_MAXSPEED       _T("Hardware: memory device {instance} max speed")
#define DCIDESC_HARDWARE_MEMORYDEVICE_PARTNUMBER     _T("Hardware: memory device {instance} part number")
#define DCIDESC_HARDWARE_MEMORYDEVICE_SERIALNUMBER   _T("Hardware: memory device {instance} serial number")
#define DCIDESC_HARDWARE_MEMORYDEVICE_SIZE           _T("Hardware: memory device {instance} size")
#define DCIDESC_HARDWARE_MEMORYDEVICE_TYPE           _T("Hardware: memory device {instance} type")
#define DCIDESC_HARDWARE_NETWORKADAPTER_AVAILABILITY _T("Hardware: network adapter {instance} availability")
#define DCIDESC_HARDWARE_NETWORKADAPTER_DESCRIPTION  _T("Hardware: network adapter {instance} description")
#define DCIDESC_HARDWARE_NETWORKADAPTER_IFINDEX      _T("Hardware: network adapter {instance} interface index")
#define DCIDESC_HARDWARE_NETWORKADAPTER_MACADDR      _T("Hardware: network adapter {instance} MAC address")
#define DCIDESC_HARDWARE_NETWORKADAPTER_MANUFACTURER _T("Hardware: network adapter {instance} manufacturer")
#define DCIDESC_HARDWARE_NETWORKADAPTER_PRODUCT      _T("Hardware: network adapter {instance} product name")
#define DCIDESC_HARDWARE_NETWORKADAPTER_SPEED        _T("Hardware: network adapter {instance} speed")
#define DCIDESC_HARDWARE_NETWORKADAPTER_TYPE         _T("Hardware: network adapter {instance} type")
#define DCIDESC_HARDWARE_PROCESSOR_CORES             _T("Hardware: processor {instance} number of cores")
#define DCIDESC_HARDWARE_PROCESSOR_CURRSPEED         _T("Hardware: processor {instance} current speed")
#define DCIDESC_HARDWARE_PROCESSOR_FAMILY            _T("Hardware: processor {instance} family")
#define DCIDESC_HARDWARE_PROCESSOR_MANUFACTURER      _T("Hardware: processor {instance} manufacturer")
#define DCIDESC_HARDWARE_PROCESSOR_MAXSPEED          _T("Hardware: processor {instance} max speed")
#define DCIDESC_HARDWARE_PROCESSOR_PARTNUMBER        _T("Hardware: processor {instance} part number")
#define DCIDESC_HARDWARE_PROCESSOR_SERIALNUMBER      _T("Hardware: processor {instance} serial number")
#define DCIDESC_HARDWARE_PROCESSOR_SOCKET            _T("Hardware: processor {instance} socket")
#define DCIDESC_HARDWARE_PROCESSOR_THREADS           _T("Hardware: processor {instance} number of threads")
#define DCIDESC_HARDWARE_PROCESSOR_TYPE              _T("Hardware: processor {instance} type")
#define DCIDESC_HARDWARE_PROCESSOR_VERSION           _T("Hardware: processor {instance} version")
#define DCIDESC_HARDWARE_STORAGEDEVICE_BUSTYPE       _T("Hardware: storage device {instance} bus type")
#define DCIDESC_HARDWARE_STORAGEDEVICE_ISREMOVABLE   _T("Hardware: storage device {instance} removable flag")
#define DCIDESC_HARDWARE_STORAGEDEVICE_MANUFACTURER  _T("Hardware: storage device {instance} manufacturer")
#define DCIDESC_HARDWARE_STORAGEDEVICE_PRODUCT       _T("Hardware: storage device {instance} product name")
#define DCIDESC_HARDWARE_STORAGEDEVICE_REVISION      _T("Hardware: storage device {instance} revision")
#define DCIDESC_HARDWARE_STORAGEDEVICE_SERIALNUMBER  _T("Hardware: storage device {instance} serial number")
#define DCIDESC_HARDWARE_STORAGEDEVICE_SIZE          _T("Hardware: storage device {instance} size")
#define DCIDESC_HARDWARE_STORAGEDEVICE_TYPE          _T("Hardware: storage device {instance} type")
#define DCIDESC_HARDWARE_STORAGEDEVICE_TYPEDESCR     _T("Hardware: storage device {instance} type description")
#define DCIDESC_HARDWARE_SYSTEM_MACHINEID            _T("Hardware: unique machine identifier")
#define DCIDESC_HARDWARE_SYSTEM_MANUFACTURER         _T("Hardware: system manufacturer")
#define DCIDESC_HARDWARE_SYSTEM_PRODUCT              _T("Hardware: product name")
#define DCIDESC_HARDWARE_SYSTEM_PRODUCTCODE          _T("Hardware: product code")
#define DCIDESC_HARDWARE_SYSTEM_SERIALNUMBER         _T("Hardware: system serial number")
#define DCIDESC_HARDWARE_SYSTEM_VERSION              _T("Hardware: version")
#define DCIDESC_HARDWARE_WAKEUPEVENT                 _T("Hardware: wakeup event")
#define DCIDESC_HYPERVISOR_TYPE                      _T("Hypervisor type")
#define DCIDESC_HYPERVISOR_VERSION                   _T("Hypervisor version")
#define DCIDESC_LVM_LV_SIZE                          _T("Size of logical volume {instance}")
#define DCIDESC_LVM_LV_STATUS                        _T("Status of logical volume {instance}")
#define DCIDESC_LVM_PV_FREE                          _T("Free space on physical volume {instance}")
#define DCIDESC_LVM_PV_FREEPERC                      _T("Percentage of free space on physical volume {instance}")
#define DCIDESC_LVM_PV_RESYNC_PART                   _T("Number of resynchronizing partitions on physical volume {instance}")
#define DCIDESC_LVM_PV_STALE_PART                    _T("Number of stale partitions on physical volume {instance}")
#define DCIDESC_LVM_PV_STATUS                        _T("Status of physical volume {instance}")
#define DCIDESC_LVM_PV_TOTAL                         _T("Total size of physical volume {instance}")
#define DCIDESC_LVM_PV_USED                          _T("Used space on physical volume {instance}")
#define DCIDESC_LVM_PV_USEDPERC                      _T("Percentage of used space on physical volume {instance}")
#define DCIDESC_LVM_VG_FREE                          _T("Free space in volume group {instance}")
#define DCIDESC_LVM_VG_FREEPERC                      _T("Percentage of free space in volume group {instance}")
#define DCIDESC_LVM_VG_LVOL_TOTAL                    _T("Number of logical volumes in volume group {instance}")
#define DCIDESC_LVM_VG_PVOL_ACTIVE                   _T("Number of active physical volumes in volume group {instance}")
#define DCIDESC_LVM_VG_PVOL_TOTAL                    _T("Number of physical volumes in volume group {instance}")
#define DCIDESC_LVM_VG_RESYNC_PART                   _T("Number of resynchronizing partitions in volume group {instance}")
#define DCIDESC_LVM_VG_STALE_PART                    _T("Number of stale partitions in volume group {instance}")
#define DCIDESC_LVM_VG_STATUS                        _T("Status of volume group {instance}")
#define DCIDESC_LVM_VG_TOTAL                         _T("Total size of volume group {instance}")
#define DCIDESC_LVM_VG_USED                          _T("Used space in volume group {instance}")
#define DCIDESC_LVM_VG_USEDPERC                      _T("Percentage of used space in volume group {instance}")
#define DCIDESC_NET_INTERFACE_64BITCOUNTERS          _T("Is 64bit interface counters supported")
#define DCIDESC_NET_INTERFACE_ADMINSTATUS            _T("Administrative status of interface {instance}")
#define DCIDESC_NET_INTERFACE_BYTESIN                _T("Number of input bytes on interface {instance}")
#define DCIDESC_NET_INTERFACE_BYTESOUT               _T("Number of output bytes on interface {instance}")
#define DCIDESC_NET_INTERFACE_DESCRIPTION            _T("Description of interface {instance}")
#define DCIDESC_NET_INTERFACE_INERRORS               _T("Number of input errors on interface {instance}")
#define DCIDESC_NET_INTERFACE_LINK                   _T("Link status for interface {instance}")
#define DCIDESC_NET_INTERFACE_MAXSPEED               _T("Maximum speed of interface {instance}")
#define DCIDESC_NET_INTERFACE_MTU                    _T("MTU for interface {instance}")
#define DCIDESC_NET_INTERFACE_OPERSTATUS             _T("Operational status of interface {instance}")
#define DCIDESC_NET_INTERFACE_OUTERRORS              _T("Number of output errors on interface {instance}")
#define DCIDESC_NET_INTERFACE_PACKETSIN              _T("Number of input packets on interface {instance}")
#define DCIDESC_NET_INTERFACE_PACKETSOUT             _T("Number of output packets on interface {instance}")
#define DCIDESC_NET_INTERFACE_SPEED                  _T("Speed of interface {instance}")
#define DCIDESC_NET_IP_FORWARDING                    _T("IP forwarding status")
#define DCIDESC_NET_IP6_FORWARDING                   _T("IPv6 forwarding status")
#define DCIDESC_NET_RESOLVER_ADDRBYNAME              _T("Resolver: address for name {instance}")
#define DCIDESC_NET_RESOLVER_NAMEBYADDR              _T("Resolver: name for address {instance}")
#define DCIDESC_PDH_COUNTERVALUE                     _T("Value of PDH counter {instance}")
#define DCIDESC_PDH_VERSION                          _T("Version of PDH.DLL")
#define DCIDESC_PHYSICALDISK_CAPACITY                _T("Capacity of hard disk {instance}")
#define DCIDESC_PHYSICALDISK_DEVICETYPE              _T("Device type of hard disk {instance}")
#define DCIDESC_PHYSICALDISK_FIRMWARE                _T("Firmware version of hard disk {instance}")
#define DCIDESC_PHYSICALDISK_MODEL                   _T("Model of hard disk {instance}")
#define DCIDESC_PHYSICALDISK_POWER_CYCLES            _T("Number of power cycles of hard disk {instance}")
#define DCIDESC_PHYSICALDISK_POWERON_TIME            _T("Power on time of hard disk {instance}")
#define DCIDESC_PHYSICALDISK_SERIALNUMBER            _T("Serial number of hard disk {instance}")
#define DCIDESC_PHYSICALDISK_SMARTATTR               _T("Generic SMART attribute {instance}")
#define DCIDESC_PHYSICALDISK_SMARTSTATUS             _T("Status of hard disk {instance} reported by SMART")
#define DCIDESC_PHYSICALDISK_TEMPERATURE             _T("Temperature of hard disk {instance}")
#define DCIDESC_PROCESS_COUNT                        _T("Number of {instance} processes")
#define DCIDESC_PROCESS_COUNTEX                      _T("Number of {instance} processes (extended)")
#define DCIDESC_PROCESS_CPUTIME                      _T("Total execution time for process {instance}")
#define DCIDESC_PROCESS_GDIOBJ                       _T("GDI objects used by process {instance}")
#define DCIDESC_PROCESS_HANDLES                      _T("Number of handles in process {instance}")
#define DCIDESC_PROCESS_IO_OTHERB                    _T("")
#define DCIDESC_PROCESS_IO_OTHEROP                   _T("")
#define DCIDESC_PROCESS_IO_READB                     _T("")
#define DCIDESC_PROCESS_IO_READOP                    _T("")
#define DCIDESC_PROCESS_IO_WRITEB                    _T("")
#define DCIDESC_PROCESS_IO_WRITEOP                   _T("")
#define DCIDESC_PROCESS_KERNELTIME                   _T("Total execution time in kernel mode for process {instance}")
#define DCIDESC_PROCESS_MEMORYUSAGE                  _T("Percentage of total physical memory used by process {instance}")
#define DCIDESC_PROCESS_PAGEFAULTS                   _T("Page faults for process {instance}")
#define DCIDESC_PROCESS_SYSCALLS                     _T("Number of system calls made by process {instance}")
#define DCIDESC_PROCESS_THREADS                      _T("Number of threads in process {instance}")
#define DCIDESC_PROCESS_USEROBJ                      _T("USER objects used by process {instance}")
#define DCIDESC_PROCESS_USERTIME                     _T("Total execution time in user mode for process {instance}")
#define DCIDESC_PROCESS_VMREGIONS                    _T("Number of mapped virtual memory regions within process {instance}")
#define DCIDESC_PROCESS_VMSIZE                       _T("Virtual memory used by process {instance}")
#define DCIDESC_PROCESS_WKSET                        _T("Physical memory used by process {instance}")
#define DCIDESC_PROCESS_ZOMBIE_COUNT                 _T("Number of {instance} zombie processes")
#define DCIDESC_SYSTEM_APPADDRESSSPACE               _T("Address space available to applications (MB)")
#define DCIDESC_SYSTEM_ARCHITECTURE                  _T("System architecture")
#define DCIDESC_SYSTEM_BIOS_DATE                     _T("BIOS date")
#define DCIDESC_SYSTEM_BIOS_VENDOR                   _T("BIOS vendor")
#define DCIDESC_SYSTEM_BIOS_VERSION                  _T("BIOS version")
#define DCIDESC_SYSTEM_CONNECTEDUSERS                _T("Number of logged in users")
#define DCIDESC_SYSTEM_CPU_CACHE_SIZE                _T("CPU {instance}: cache size (KB)")
#define DCIDESC_SYSTEM_CPU_CORE_ID                   _T("CPU {instance}: core ID")
#define DCIDESC_SYSTEM_CPU_COUNT                     _T("Number of CPU in the system")
#define DCIDESC_SYSTEM_CPU_COUNT_ONLINE              _T("Number of online CPU in the system")
#define DCIDESC_SYSTEM_CPU_COUNT_OFFLINE             _T("Number of offline CPU in the system")
#define DCIDESC_SYSTEM_CPU_FREQUENCY                 _T("CPU {instance}: frequency")
#define DCIDESC_SYSTEM_CPU_MODEL                     _T("CPU {instance}: model")
#define DCIDESC_SYSTEM_CPU_PHYSICAL_ID               _T("CPU {instance}: physical ID")
#define DCIDESC_SYSTEM_CURRENTDATE                   _T("Current system date")
#define DCIDESC_SYSTEM_CURRENTTIME                   _T("Current system time")
#define DCIDESC_SYSTEM_CURRENTTIME_ISO8601_LOCAL     _T("Current system time in ISO8601 format in local time zone")
#define DCIDESC_SYSTEM_CURRENTTIME_ISO8601_UTC       _T("Current system time in ISO8601 format in UTC")
#define DCIDESC_SYSTEM_FQDN                          _T("Fully qualified domain name")
#define DCIDESC_SYSTEM_HANDLECOUNT                   _T("Total number of handles")
#define DCIDESC_SYSTEM_HARDWAREID                    _T("Hardware ID")
#define DCIDESC_SYSTEM_HOSTNAME                      _T("Host name")
#define DCIDESC_SYSTEM_IS_RESTART_PENDING            _T("Check if system restart is pending")
#define DCIDESC_SYSTEM_IS_VIRTUAL                    _T("Virtual system indicator")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_AVAILABLE     _T("Available physical memory")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_AVAILABLE_PCT _T("Percentage of available physical memory")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_BUFFERS       _T("Physical memory used for buffers")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_BUFFERS_PCT   _T("Percentage of physical memory used for buffers")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_CACHED        _T("Physical memory used for cache")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_CACHED_PCT    _T("Percentage of physical memory used for cache")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE          _T("Free physical memory")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE_PCT      _T("Percentage of free physical memory")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_TOTAL         _T("Total amount of physical memory")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED          _T("Used physical memory")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED_PCT      _T("Percentage of used physical memory")
#define DCIDESC_SYSTEM_MEMORY_VIRTUAL_ACTIVE         _T("Active virtual memory")
#define DCIDESC_SYSTEM_MEMORY_VIRTUAL_ACTIVE_PCT     _T("Percentage of active virtual memory")
#define DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE           _T("Free virtual memory")
#define DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE_PCT       _T("Percentage of free virtual memory")
#define DCIDESC_SYSTEM_MEMORY_VIRTUAL_TOTAL          _T("Total amount of virtual memory")
#define DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED           _T("Used virtual memory")
#define DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED_PCT       _T("Percentage of used virtual memory")
#define DCIDESC_SYSTEM_MEMORY_VIRTUAL_AVAILABLE      _T("Available virtual memory")
#define DCIDESC_SYSTEM_MEMORY_VIRTUAL_AVAILABLE_PCT  _T("Percentage of available virtual memory")
#define DCIDESC_SYSTEM_MEMORY_SWAP_FREE              _T("Free swap space")
#define DCIDESC_SYSTEM_MEMORY_SWAP_FREE_PCT          _T("Percentage of free swap space")
#define DCIDESC_SYSTEM_MEMORY_SWAP_TOTAL             _T("Total amount of swap space")
#define DCIDESC_SYSTEM_MEMORY_SWAP_USED              _T("Used swap space")
#define DCIDESC_SYSTEM_MEMORY_SWAP_USED_PCT          _T("Percentage of used swap space")
#define DCIDESC_SYSTEM_MSGQUEUE_BYTES                _T("Message queue {instance}: bytes in queue")
#define DCIDESC_SYSTEM_MSGQUEUE_BYTES_MAX            _T("Message queue {instance}: maximum allowed bytes in queue")
#define DCIDESC_SYSTEM_MSGQUEUE_CHANGE_TIME          _T("Message queue {instance}: last change time")
#define DCIDESC_SYSTEM_MSGQUEUE_MESSAGES             _T("Message queue {instance}: number of messages")
#define DCIDESC_SYSTEM_MSGQUEUE_RECV_TIME            _T("Message queue {instance}: last receive time")
#define DCIDESC_SYSTEM_MSGQUEUE_SEND_TIME            _T("Message queue {instance}: last send time")
#define DCIDESC_SYSTEM_OS_BUILD                      _T("Operating system: build number")
#define DCIDESC_SYSTEM_OS_LICENSE_KEY                _T("Operating system: license key")
#define DCIDESC_SYSTEM_OS_PRODUCT_ID                 _T("Operating system: product ID")
#define DCIDESC_SYSTEM_OS_PRODUCT_NAME               _T("Operating system: product name")
#define DCIDESC_SYSTEM_OS_PRODUCT_TYPE               _T("Operating system: product type")
#define DCIDESC_SYSTEM_OS_SERVICE_PACK               _T("Operating system: service pack version")
#define DCIDESC_SYSTEM_OS_VERSION                    _T("Operating system: version")
#define DCIDESC_SYSTEM_PLATFORMNAME                  _T("Platform name")
#define DCIDESC_SYSTEM_PROCESSCOUNT                  _T("Total number of processes")
#define DCIDESC_SYSTEM_SERVICESTATE                  _T("State of {instance} service")
#define DCIDESC_SYSTEM_THREADCOUNT                   _T("Total number of threads")
#define DCIDESC_SYSTEM_TIMEZONE                      _T("System time zone")
#define DCIDESC_SYSTEM_TIMEZONEOFFSET                _T("System time zone offset")
#define DCIDESC_SYSTEM_UNAME                         _T("System uname")
#define DCIDESC_SYSTEM_UPTIME                        _T("System uptime")

#define DCIDESC_SYSTEM_CPU_LOADAVG                   _T("Average CPU load for last minute")
#define DCIDESC_SYSTEM_CPU_LOADAVG5                  _T("Average CPU load for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_LOADAVG15                 _T("Average CPU load for last 15 minutes")

#define DCIDESC_SYSTEM_CPU_INTERRUPTS                _T("CPU interrupt count")
#define DCIDESC_SYSTEM_CPU_INTERRUPTS_EX             _T("CPU {instance} interrupt count")
#define DCIDESC_SYSTEM_CPU_CONTEXT_SWITCHES          _T("CPU context switch count")

#define DCIDESC_SYSTEM_CPU_USAGE_EX                  _T("Average CPU {instance} utilization for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_EX                 _T("Average CPU {instance} utilization for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_EX                _T("Average CPU {instance} utilization for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE                     _T("Average CPU utilization for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5                    _T("Average CPU utilization for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15                   _T("Average CPU utilization for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGECURR                 _T("Current CPU utilization")
#define DCIDESC_SYSTEM_CPU_USAGECURR_EX              _T("Current CPU {instance} utilization")

#define DCIDESC_SYSTEM_CPU_USAGE_USER_EX             _T("Average CPU {instance} utilization (user) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_USER_EX            _T("Average CPU {instance} utilization (user) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_USER_EX           _T("Average CPU {instance} utilization (user) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE_USER                _T("Average CPU utilization (user) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_USER               _T("Average CPU utilization (user) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_USER              _T("Average CPU utilization (user) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGECURR_USER            _T("Current CPU utilization (user)")
#define DCIDESC_SYSTEM_CPU_USAGECURR_USER_EX         _T("Current CPU {instance} utilization (user)")

#define DCIDESC_SYSTEM_CPU_USAGE_NICE_EX             _T("Average CPU {instance} utilization (nice) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_NICE_EX            _T("Average CPU {instance} utilization (nice) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_NICE_EX           _T("Average CPU {instance} utilization (nice) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE_NICE                _T("Average CPU utilization (nice) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_NICE               _T("Average CPU utilization (nice) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_NICE              _T("Average CPU utilization (nice) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGECURR_NICE            _T("Current CPU utilization (nice)")
#define DCIDESC_SYSTEM_CPU_USAGECURR_NICE_EX         _T("Current CPU {instance} utilization (nice)")

#define DCIDESC_SYSTEM_CPU_USAGE_SYSTEM_EX           _T("Average CPU {instance} utilization (system) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_SYSTEM_EX          _T("Average CPU {instance} utilization (system) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_SYSTEM_EX         _T("Average CPU {instance} utilization (system) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE_SYSTEM              _T("Average CPU utilization (system) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_SYSTEM             _T("Average CPU utilization (system) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_SYSTEM            _T("Average CPU utilization (system) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGECURR_SYSTEM          _T("Current CPU utilization (system)")
#define DCIDESC_SYSTEM_CPU_USAGECURR_SYSTEM_EX       _T("Current CPU {instance} utilization (system)")

#define DCIDESC_SYSTEM_CPU_USAGE_IDLE_EX             _T("Average CPU {instance} utilization (idle) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_IDLE_EX            _T("Average CPU {instance} utilization (idle) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_IDLE_EX           _T("Average CPU {instance} utilization (idle) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE_IDLE                _T("Average CPU utilization (idle) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_IDLE               _T("Average CPU utilization (idle) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_IDLE              _T("Average CPU utilization (idle) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGECURR_IDLE            _T("Current CPU utilization (idle)")
#define DCIDESC_SYSTEM_CPU_USAGECURR_IDLE_EX         _T("Current CPU {instance} utilization (idle)")

#define DCIDESC_SYSTEM_CPU_USAGE_IOWAIT_EX           _T("Average CPU {instance} utilization (iowait) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_IOWAIT_EX          _T("Average CPU {instance} utilization (iowait) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_IOWAIT_EX         _T("Average CPU {instance} utilization (iowait) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE_IOWAIT              _T("Average CPU utilization (iowait) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_IOWAIT             _T("Average CPU utilization (iowait) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_IOWAIT            _T("Average CPU utilization (iowait) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGECURR_IOWAIT          _T("Current CPU utilization (iowait)")
#define DCIDESC_SYSTEM_CPU_USAGECURR_IOWAIT_EX       _T("Current CPU {instance} utilization (iowait)")

#define DCIDESC_SYSTEM_CPU_USAGE_IRQ_EX              _T("Average CPU {instance} utilization (irq) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_IRQ_EX             _T("Average CPU {instance} utilization (irq) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_IRQ_EX            _T("Average CPU {instance} utilization (irq) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE_IRQ                 _T("Average CPU utilization (irq) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_IRQ                _T("Average CPU utilization (irq) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_IRQ               _T("Average CPU utilization (irq) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGECURR_IRQ             _T("Current CPU utilization (irq)")
#define DCIDESC_SYSTEM_CPU_USAGECURR_IRQ_EX          _T("Current CPU {instance} utilization (irq)")

#define DCIDESC_SYSTEM_CPU_USAGE_SOFTIRQ_EX          _T("Average CPU {instance} utilization (softirq) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_SOFTIRQ_EX         _T("Average CPU {instance} utilization (softirq) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_SOFTIRQ_EX        _T("Average CPU {instance} utilization (softirq) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE_SOFTIRQ             _T("Average CPU utilization (softirq) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_SOFTIRQ            _T("Average CPU utilization (softirq) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_SOFTIRQ           _T("Average CPU utilization (softirq) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGECURR_SOFTIRQ         _T("Current CPU utilization (softirq)")
#define DCIDESC_SYSTEM_CPU_USAGECURR_SOFTIRQ_EX      _T("Current CPU {instance} utilization (softirq)")

#define DCIDESC_SYSTEM_CPU_USAGE_STEAL_EX            _T("Average CPU {instance} utilization (steal) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_STEAL_EX           _T("Average CPU {instance} utilization (steal) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_STEAL_EX          _T("Average CPU {instance} utilization (steal) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE_STEAL               _T("Average CPU utilization (steal) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_STEAL              _T("Average CPU utilization (steal) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_STEAL             _T("Average CPU utilization (steal) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGECURR_STEAL           _T("Current CPU utilization (steal)")
#define DCIDESC_SYSTEM_CPU_USAGECURR_STEAL_EX        _T("Current CPU {instance} utilization (steal)")

#define DCIDESC_SYSTEM_CPU_USAGE_GUEST_EX            _T("Average CPU {instance} utilization (guest) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_GUEST_EX           _T("Average CPU {instance} utilization (guest) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_GUEST_EX          _T("Average CPU {instance} utilization (guest) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE_GUEST               _T("Average CPU utilization (guest) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_GUEST              _T("Average CPU utilization (guest) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_GUEST             _T("Average CPU utilization (guest) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGECURR_GUEST           _T("Current CPU utilization (guest)")
#define DCIDESC_SYSTEM_CPU_USAGECURR_GUEST_EX        _T("Current CPU {instance} utilization (guest)")

#define DCIDESC_SYSTEM_CPU_VENDORID                  _T("CPU vendor ID")

#define DCIDESC_SYSTEM_ENERGYCONSUMPTION_POWERZONE   _T("Energy consumption: power zone {instance}")
#define DCIDESC_SYSTEM_ENERGYCONSUMPTION_TOTAL       _T("Energy consumption: total")

#define DCIDESC_SYSTEM_IO_DISKQUEUE                  _T("Average disk queue length for last minute")
#define DCIDESC_SYSTEM_IO_DISKQUEUE_MIN              _T("Minimum disk queue length for last minute")
#define DCIDESC_SYSTEM_IO_DISKQUEUE_MAX              _T("Maximum disk queue length for last minute")
#define DCIDESC_SYSTEM_IO_DISKQUEUE_EX               _T("Average disk queue length of device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_DISKQUEUE_EX_MIN           _T("Minimum disk queue length of device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_DISKQUEUE_EX_MAX           _T("Maximum disk queue length of device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_DISKREADTIME               _T("Percent of CPU time spent reading for last minute")
#define DCIDESC_SYSTEM_IO_DISKREADTIME_EX            _T("Percent of CPU time spent reading on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_DISKTIME                   _T("Percent of CPU time spent doing I/O for last minute")
#define DCIDESC_SYSTEM_IO_DISKTIME_EX                _T("Percent of CPU time spent doing I/O on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_DISKWRITETIME              _T("Percent of CPU time spent writing for last minute")
#define DCIDESC_SYSTEM_IO_DISKWRITETIME_EX           _T("Percent of CPU time spent writing on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_READS                      _T("Average number of read operations for last minute")
#define DCIDESC_SYSTEM_IO_READS_MIN                  _T("Minimum number of read operations for last minute")
#define DCIDESC_SYSTEM_IO_READS_MAX                  _T("Maximum number of read operations for last minute")
#define DCIDESC_SYSTEM_IO_READS_EX                   _T("Average number of read operations on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_READS_EX_MIN               _T("Minimum number of read operations on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_READS_EX_MAX               _T("Maximum number of read operations on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_READWAITTIME               _T("Average read request wait time for last minute")
#define DCIDESC_SYSTEM_IO_READWAITTIME_EX            _T("Average read request wait time on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_WAITTIME                   _T("Average I/O request wait time for last minute")
#define DCIDESC_SYSTEM_IO_WAITTIME_EX                _T("Average I/O request wait time for device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_WRITES                     _T("Average number of write operations for last minute")
#define DCIDESC_SYSTEM_IO_WRITES_MIN                 _T("Minimum number of write operations for last minute")
#define DCIDESC_SYSTEM_IO_WRITES_MAX                 _T("Maximum number of write operations for last minute")
#define DCIDESC_SYSTEM_IO_WRITES_EX                  _T("Average number of write operations on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_WRITES_EX_MIN              _T("Minimum number of write operations on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_WRITES_EX_MAX              _T("Maximum number of write operations on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_WRITEWAITTIME              _T("Average write request wait time for last minute")
#define DCIDESC_SYSTEM_IO_WRITEWAITTIME_EX           _T("Average write request wait time on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_XFERS                      _T("Average number of I/O transfers for last minute")
#define DCIDESC_SYSTEM_IO_XFERS_MIN                  _T("Minimum number of I/O transfers for last minute")
#define DCIDESC_SYSTEM_IO_XFERS_MAX                  _T("Maximum number of I/O transfers for last minute")
#define DCIDESC_SYSTEM_IO_XFERS_EX                   _T("Average number of I/O transfers on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_XFERS_EX_MIN               _T("Minimum number of I/O transfers on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_XFERS_EX_MAX               _T("Maximum number of I/O transfers on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_BYTEREADS                  _T("Average number of bytes read for last minute")
#define DCIDESC_SYSTEM_IO_BYTEREADS_MIN              _T("Minimum number of bytes read for last minute")
#define DCIDESC_SYSTEM_IO_BYTEREADS_MAX              _T("Maximum number of bytes read for last minute")
#define DCIDESC_SYSTEM_IO_BYTEREADS_EX               _T("Average number of bytes read on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_BYTEREADS_EX_MIN           _T("Minimum number of bytes read on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_BYTEREADS_EX_MAX           _T("Maximum number of bytes read on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_BYTEWRITES                 _T("Average number of bytes written for last minute")
#define DCIDESC_SYSTEM_IO_BYTEWRITES_MIN             _T("Minimum number of bytes written for last minute")
#define DCIDESC_SYSTEM_IO_BYTEWRITES_MAX             _T("Maximum number of bytes written for last minute")
#define DCIDESC_SYSTEM_IO_BYTEWRITES_EX              _T("Average number of bytes written on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_BYTEWRITES_EX_MIN          _T("Minimum number of bytes written on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_BYTEWRITES_EX_MAX          _T("Maximum number of bytes written on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_OPENFILES                  _T("Number of open files")

#define DCIDESC_DEPRECATED                           _T("<deprecated>")

#define DCTDESC_AGENT_SESSION_AGENTS                 _T("Registered session agents")
#define DCTDESC_AGENT_SUBAGENTS                      _T("Loaded subagents")
#define DCTDESC_AGENT_ZONE_CONFIGURATIONS            _T("Loaded zone configurations")
#define DCTDESC_AGENT_ZONE_PROXIES                   _T("Zone proxies")
#define DCTDESC_FILESYSTEM_VOLUMES                   _T("File system volumes")
#define DCTDESC_HARDWARE_BATTERIES                   _T("Hardware: batteries")
#define DCTDESC_HARDWARE_MEMORY_DEVICES              _T("Hardware: memory devices")
#define DCTDESC_HARDWARE_NETWORK_ADAPTERS            _T("Hardware: network adapters")
#define DCTDESC_HARDWARE_PROCESSORS                  _T("Hardware: processors")
#define DCTDESC_HARDWARE_STORAGE_DEVICES             _T("Hardware: storage devices")
#define DCTDESC_LVM_VOLUME_GROUPS                    _T("LVM volume groups")
#define DCTDESC_LVM_LOGICAL_VOLUMES                  _T("Logical volumes in volume group {instance}")
#define DCTDESC_LVM_PHYSICAL_VOLUMES                 _T("Physical volumes in volume group {instance}")
#define DCTDESC_NET_IP_NEIGHBORS                     _T("IP neighbors")
#define DCTDESC_NETWORK_INTERFACES                   _T("Network interfaces")
#define DCTDESC_WIREGUARD_INTERFACES                 _T("Wireguard interfaces")
#define DCTDESC_WIREGUARD_PEERS                      _T("Wireguard peers")
#define DCTDESC_PHYSICALDISK_DEVICES                 _T("Physical disks")
#define DCTDESC_SYSTEM_ACTIVE_USER_SESSIONS          _T("Active user sessions")
#define DCTDESC_SYSTEM_INSTALLED_PRODUCTS            _T("Installed products")
#define DCTDESC_SYSTEM_OPEN_FILES                    _T("Open files")
#define DCTDESC_SYSTEM_PROCESSES                     _T("Processes")

#pragma pack(1)

/**
 * Hash map key for server objects (64 bit server ID + 32 bit object ID)
 */
struct LIBNXAGENT_EXPORTABLE ServerObjectKey
{
   uint64_t serverId;
   uint32_t objectId;

   ServerObjectKey(uint64_t _serverId, uint32_t _objectId)
   {
      serverId = _serverId;
      objectId = _objectId;
   }

   bool equals(const ServerObjectKey& k) const
   {
      return (serverId == k.serverId) && (objectId == k.objectId);
   }
};

#pragma pack()

/**
 * Class that stores information about file that will be received
 */
class LIBNXAGENT_EXPORTABLE DownloadFileInfo
{
protected:
   TCHAR *m_fileName;
   time_t m_fileModificationTime;
   int m_fileHandle;
   StreamCompressor *m_compressor;  // stream compressor for file transfer
   time_t m_lastUpdateTime;

public:
   DownloadFileInfo(const TCHAR *name, time_t fileModificationTime = 0);
   virtual ~DownloadFileInfo();

   virtual bool open(bool append = false);
   virtual bool write(const BYTE *data, size_t dataSize, bool compressedStream);
   virtual void close(bool success, bool deleteOnFailure = true);

   static uint32_t getFileInfo(NXCPMessage *response, const TCHAR *fileName);
   const TCHAR *getFileName() const { return m_fileName; }
   time_t getLastUpdateTime() const { return m_lastUpdateTime; }
};

/**
 * API for CommSession
 */
class LIBNXAGENT_EXPORTABLE AbstractCommSession
{
   template <class C, typename... Args> friend shared_ptr<C> MakeSharedCommSession(Args&&... args);

private:
#ifdef _WIN32
   weak_ptr<AbstractCommSession> *m_self; // should be pointer because of DLL linkage issues on Windows
#else
   weak_ptr<AbstractCommSession> m_self;
#endif

protected:
#ifdef _WIN32
   void setSelfPtr(const shared_ptr<AbstractCommSession>& sptr) { *m_self = sptr; }
#else
   void setSelfPtr(const shared_ptr<AbstractCommSession>& sptr) { m_self = sptr; }
#endif

public:
   AbstractCommSession();
   AbstractCommSession(const AbstractCommSession& src) = delete;
   virtual ~AbstractCommSession();

#ifdef _WIN32
   shared_ptr<AbstractCommSession> self() const { return m_self->lock(); }
#else
   shared_ptr<AbstractCommSession> self() const { return m_self.lock(); }
#endif

   virtual uint32_t getId() = 0;

   virtual uint64_t getServerId() = 0;
   virtual const InetAddress& getServerAddress() = 0;

   virtual bool isMasterServer() = 0;
   virtual bool isControlServer() = 0;
   virtual bool canAcceptData() = 0;
   virtual bool canAcceptTraps() = 0;
   virtual bool canAcceptFileUpdates() = 0;
   virtual bool isBulkReconciliationSupported() = 0;
   virtual bool isIPv6Aware() = 0;

   virtual const TCHAR *getDebugTag() const = 0;

   virtual bool sendMessage(const NXCPMessage *msg) = 0;
   virtual void postMessage(const NXCPMessage *msg) = 0;
   virtual bool sendRawMessage(const NXCP_MESSAGE *msg) = 0;
   virtual void postRawMessage(const NXCP_MESSAGE *msg) = 0;
	virtual bool sendFile(uint32_t requestId, const TCHAR *file, off64_t offset, NXCPStreamCompressionMethod compressionMethod, VolatileCounter *cancelationFlag) = 0;
   virtual uint32_t doRequest(NXCPMessage *msg, uint32_t timeout) = 0;
   virtual NXCPMessage *doRequestEx(NXCPMessage *msg, uint32_t timeout) = 0;
   virtual NXCPMessage *waitForMessage(UINT16 code, UINT32 id, UINT32 timeout) = 0;
   virtual uint32_t generateRequestId() = 0;
   virtual int getProtocolVersion() = 0;
   virtual void openFile(NXCPMessage *response, TCHAR *nameOfFile, uint32_t requestId, time_t fileModTime = 0, FileTransferResumeMode resumeMode = FileTransferResumeMode::OVERWRITE) = 0;
   virtual void debugPrintf(int level, const TCHAR *format, ...) = 0;
   virtual void writeLog(int16_t severity, const TCHAR *format, ...) = 0;
   virtual void prepareProxySessionSetupMsg(NXCPMessage *msg) = 0;
   virtual void registerForResponseSentCondition(uint32_t requestId) = 0;
   virtual void waitForResponseSentCondition(uint32_t requestId) = 0;
};

/**
 * make_shared() replacement for AbstractCommSession derived classes
 */
template <class C, typename... Args> shared_ptr<C> MakeSharedCommSession(Args&&... args)
{
   auto object = make_shared<C>(args...);
   object->setSelfPtr(object);
   return object;
}

/**
 *  Action execution context
 */
class ActionExecutionContext
{
private:
   uint32_t m_requestId;
   shared_ptr<AbstractCommSession> m_session;
   TCHAR* m_name;
   const void* m_data;
   StringList m_args;
   bool m_sendOutput;
   bool m_completed;
   uint32_t m_rcc;
   Condition m_completionCondition;

public:
   ActionExecutionContext(const TCHAR *name, const StringList& args, const void *data, const shared_ptr<AbstractCommSession>& session, uint32_t requestId, bool sendOutput)
                        : m_session(session), m_args(args), m_completionCondition(true)
   {
      m_name = MemCopyString(name);
      m_requestId = requestId;
      m_data = data;
      m_sendOutput = sendOutput;
      m_completed = false;
      m_rcc = ERR_INTERNAL_ERROR;
   }

   ~ActionExecutionContext()
   {
      MemFree(m_name);
   }

   /**
    * Send output to server
    */
   void sendOutput(const TCHAR* text)
   {
      if (m_sendOutput)
      {
         NXCPMessage msg(CMD_COMMAND_OUTPUT, m_requestId, m_session->getProtocolVersion());
         msg.setField(VID_MESSAGE, text);
         m_session->sendMessage(&msg);
      }
   }

   /**
    * Send output to server with text encoded in UTF-8
    */
   void sendOutputUtf8(const char* text)
   {
      if (m_sendOutput)
      {
         NXCPMessage msg(CMD_COMMAND_OUTPUT, m_requestId, m_session->getProtocolVersion());
         msg.setFieldFromUtf8String(VID_MESSAGE, text);
         m_session->sendMessage(&msg);
      }
   }

   /**
    * Send end of output marker to server
    */
   void sendEndOfOutputMarker()
   {
      if (m_sendOutput)
      {
         NXCPMessage msg(CMD_COMMAND_OUTPUT, m_requestId, m_session->getProtocolVersion());
         msg.setEndOfSequence();
         m_session->sendMessage(&msg);
      }
   }

   /**
    * Mark action execution as completed
    */
   void markAsCompleted(uint32_t rcc)
   {
      if (!m_completed)
      {
         m_completed = true;
         m_rcc = rcc;
         if (m_sendOutput)
            m_session->registerForResponseSentCondition(m_requestId);
         m_completionCondition.set();
         if (m_sendOutput)
            m_session->waitForResponseSentCondition(m_requestId);
      }
   }

   /**
    * Wait for handler execution completion
    */
   uint32_t waitForCompletion()
   {
      if (!m_completed)
         m_completionCondition.wait();
      return m_rcc;
   }

   const TCHAR *getName() const { return m_name; }
   const void *getData() const { return m_data; }
   template<typename T> const T *getData() const { return static_cast<const T*>(m_data); }

   const shared_ptr<AbstractCommSession>& getSession() const { return m_session; }
   uint32_t getRequestId() const { return m_requestId; }
   const StringList& getArgs() const { return m_args; }
   int getArgCount() const { return m_args.size(); }
   bool hasArgs() const { return !m_args.isEmpty(); }
   const TCHAR *getArg(int index) const { return m_args.get(index); }
   bool isOutputRequested() const { return m_sendOutput; }
};

/**
 * Subagent's parameter information
 */
struct NETXMS_SUBAGENT_PARAM
{
   TCHAR name[MAX_PARAM_NAME];
   LONG (*handler)(const TCHAR*, const TCHAR*, TCHAR*, AbstractCommSession*);
   const TCHAR *arg;
   int dataType;		// Use DT_DEPRECATED to indicate deprecated parameter
   TCHAR description[MAX_DB_STRING];
   bool (*filter)(const TCHAR *metric, const TCHAR *arg, AbstractCommSession *session);
};

/**
 * Subagent's push parameter information
 */
struct NETXMS_SUBAGENT_PUSHPARAM
{
   TCHAR name[MAX_PARAM_NAME];
   int dataType;
   TCHAR description[MAX_DB_STRING];
};

/**
 * Subagent's list information
 */
struct NETXMS_SUBAGENT_LIST
{
   TCHAR name[MAX_PARAM_NAME];
   LONG (*handler)(const TCHAR*, const TCHAR*, StringList*, AbstractCommSession*);
   const TCHAR *arg;
   TCHAR description[MAX_DB_STRING];
   bool (*filter)(const TCHAR *metric, const TCHAR *arg, AbstractCommSession *session);
};

/**
 * Subagent's table column information
 */
struct NETXMS_SUBAGENT_TABLE_COLUMN
{
   TCHAR name[MAX_COLUMN_NAME];
   TCHAR displayName[MAX_COLUMN_NAME];
   int dataType;
   bool isInstance;
};

/**
 * Subagent's table information
 */
struct NETXMS_SUBAGENT_TABLE
{
   TCHAR name[MAX_PARAM_NAME];
   LONG (* handler)(const TCHAR *, const TCHAR *, Table *, AbstractCommSession *);
   const TCHAR *arg;
   TCHAR instanceColumns[MAX_COLUMN_NAME * MAX_INSTANCE_COLUMNS];
   TCHAR description[MAX_DB_STRING];
   int numColumns;
   NETXMS_SUBAGENT_TABLE_COLUMN *columns;
   bool (*filter)(const TCHAR *metric, const TCHAR *arg, AbstractCommSession *session);
};

/**
 * Subagent's action information
 */
struct NETXMS_SUBAGENT_ACTION
{
   TCHAR name[MAX_PARAM_NAME];
   uint32_t (* handler)(const shared_ptr<ActionExecutionContext>&);
   const void *arg;
   TCHAR description[MAX_DB_STRING];
};

#define NETXMS_SUBAGENT_INFO_MAGIC     ((uint32_t)0x20201227)

class NXCPMessage;

/**
 * Subagent initialization structure
 */
struct NETXMS_SUBAGENT_INFO
{
   uint32_t magic;    // Magic number to check if subagent uses correct version of this structure
   TCHAR name[MAX_SUBAGENT_NAME];
   TCHAR version[32];
   bool (*init)(Config *);   // Called to initialize subagent. Can be NULL.
   void (*shutdown)();       // Called at subagent unload. Can be NULL.
   bool (*commandHandler)(uint32_t command, NXCPMessage *request, NXCPMessage *response, AbstractCommSession *session);
   void (*notify)(uint32_t code, void *data);  // Generic notification interface
   bool (*mericFilter)(const TCHAR *metric, const TCHAR *arg, AbstractCommSession *session);
   size_t numParameters;
   NETXMS_SUBAGENT_PARAM *parameters;
   size_t numLists;
   NETXMS_SUBAGENT_LIST *lists;
   size_t numTables;
   NETXMS_SUBAGENT_TABLE *tables;
   size_t numActions;
   NETXMS_SUBAGENT_ACTION *actions;
   size_t numPushParameters;
   NETXMS_SUBAGENT_PUSHPARAM *pushParameters;
};

/**
 * Inline functions for returning parameters
 */
static inline void ret_string(TCHAR *rbuf, const TCHAR *value)
{
   _tcslcpy(rbuf, value, MAX_RESULT_LENGTH);
}

static inline void ret_wstring(TCHAR *rbuf, const WCHAR *value)
{
#ifdef UNICODE
   wcslcpy(rbuf, value, MAX_RESULT_LENGTH);
#else
   wchar_to_mb(value, -1, rbuf, MAX_RESULT_LENGTH);
   rbuf[MAX_RESULT_LENGTH - 1] = 0;
#endif
}

static inline void ret_mbstring(TCHAR *rbuf, const char *value)
{
#ifdef UNICODE
   mb_to_wchar(value, -1, rbuf, MAX_RESULT_LENGTH);
   rbuf[MAX_RESULT_LENGTH - 1] = 0;
#else
   strlcpy(rbuf, value, MAX_RESULT_LENGTH);
#endif
}

static inline void ret_utf8string(TCHAR *rbuf, const char *value)
{
#ifdef UNICODE
   utf8_to_wchar(value, -1, rbuf, MAX_RESULT_LENGTH);
   rbuf[MAX_RESULT_LENGTH - 1] = 0;
#else
   utf8_to_mb(value, -1, rbuf, MAX_RESULT_LENGTH);
   rbuf[MAX_RESULT_LENGTH - 1] = 0;
#endif
}

static inline void ret_boolean(TCHAR *rbuf, bool value)
{
   rbuf[0] = value ? '1' : '0';
   rbuf[1] = 0;
}

static inline void ret_int(TCHAR *rbuf, int32_t value)
{
   IntegerToString(value, rbuf);
}

static inline void ret_uint(TCHAR *rbuf, uint32_t value)
{
   IntegerToString(value, rbuf);
}

static inline void ret_double(TCHAR *rbuf, double value, int digits = 6)
{
#if defined(_WIN32) && (_MSC_VER >= 1300) && !defined(__clang__)
   _sntprintf_s(rbuf, MAX_RESULT_LENGTH, _TRUNCATE, _T("%1.*f"), digits, value);
#else
   _sntprintf(rbuf, MAX_RESULT_LENGTH, _T("%1.*f"), digits, value);
#endif
}

static inline void ret_int64(TCHAR *rbuf, int64_t value)
{
   IntegerToString(value, rbuf);
}

static inline void ret_uint64(TCHAR *rbuf, uint64_t value)
{
   IntegerToString(value, rbuf);
}

/**
 * Process executor that collect key-value pairs from output
 */
class LIBNXAGENT_EXPORTABLE KeyValueOutputProcessExecutor : public ProcessExecutor
{
private:
   StringMap m_data;
   StringBuffer m_buffer;
   TCHAR m_separator;

protected:
   virtual void onOutput(const char *text, size_t length) override;
   virtual void endOfOutput() override;

public:
   KeyValueOutputProcessExecutor(const TCHAR *command, bool shellExec = true);

   virtual bool execute() override { m_data.clear(); return ProcessExecutor::execute(); }

   const StringMap& getData() const { return m_data; }

   TCHAR getSeparator() const { return m_separator; }
   void setSeparator(TCHAR s) { m_separator = s; }
};

/**
 * Process executor that collect process output line by line
 */
class LIBNXAGENT_EXPORTABLE LineOutputProcessExecutor : public ProcessExecutor
{
private:
   StringList m_data;
   StringBuffer m_buffer;

protected:
   virtual void onOutput(const char *text, size_t length) override;
   virtual void endOfOutput() override;

public:
   LineOutputProcessExecutor(const TCHAR *command, bool shellExec = true);

   virtual bool execute() override
   {
      m_data.clear();
      return ProcessExecutor::execute();
   }

   const StringList& getData() const { return m_data; }
};

/**
 * Notification displayed by user agent
 */
class LIBNXAGENT_EXPORTABLE UserAgentNotification
{
private:
   ServerObjectKey m_id;
   TCHAR *m_message;
   time_t m_startTime;
   time_t m_endTime;
   bool m_startup;
   bool m_read;

public:
   UserAgentNotification(uint64_t serverId, const NXCPMessage *msg, uint32_t baseId);
   UserAgentNotification(const NXCPMessage *msg, uint32_t baseId);
   UserAgentNotification(uint64_t serverId, uint32_t notificationId, TCHAR *message, time_t start, time_t end, bool startup);
   UserAgentNotification(const UserAgentNotification *src);
   ~UserAgentNotification();

   const ServerObjectKey& getId() const { return m_id; }
   const TCHAR *getMessage() const { return m_message; }
   time_t getStartTime() const { return m_startTime; }
   time_t getEndTime() const { return m_endTime; }
   bool isInstant() const { return m_startTime == 0; }
   bool isStartup() const { return m_startup; }
   bool isRead() const { return m_read; }

   void setRead() { m_read = true; }

   void fillMessage(NXCPMessage *msg, uint32_t baseId);
   void saveToDatabase(DB_HANDLE hdb);
};

#ifdef _WIN32

/**
 * Process information for GetProcessListForUserSession
 */
struct LIBNXAGENT_EXPORTABLE ProcessInformation
{
   DWORD pid;
   TCHAR *name;

   ProcessInformation(DWORD _pid, const TCHAR *_name)
   {
      pid = _pid;
      name = MemCopyString(_name);
   }
   ~ProcessInformation()
   {
      MemFree(name);
   }
};

#endif

/**
 * Utility class for parsing and handling named options in metric arguments
 */
class LIBNXAGENT_EXPORTABLE OptionList
{
private:
   StringMap m_options;
   bool m_valid;

public:
   OptionList(const TCHAR *parameters, int offset = 1);

   bool isValid() const
   {
      return m_valid;
   }

   bool exists(const TCHAR *key) const
   {
      return m_options.contains(key);
   }

   const TCHAR *get(const TCHAR *key, const TCHAR *defaultValue = nullptr) const
   {
      const TCHAR *value = m_options.get(key);
      return (value != nullptr) ? value : defaultValue;
   }

   int16_t getAsInt16(const TCHAR *key, int16_t defaultValue = 0) const
   {
      const TCHAR *value = m_options.get(key);
      return (value != nullptr) ? static_cast<int16_t>(_tcstol(value, nullptr, 0)) : defaultValue;
   }

   uint16_t getAsUInt16(const TCHAR *key, uint16_t defaultValue = 0) const
   {
      const TCHAR *value = m_options.get(key);
      return (value != nullptr) ? static_cast<uint16_t>(_tcstoul(value, nullptr, 0)) : defaultValue;
   }

   int32_t getAsInt32(const TCHAR *key, int32_t defaultValue = 0) const
   {
      const TCHAR *value = m_options.get(key);
      return (value != nullptr) ? _tcstol(value, nullptr, 0) : defaultValue;
   }

   uint32_t getAsUInt32(const TCHAR *key, uint32_t defaultValue = 0) const
   {
      const TCHAR *value = m_options.get(key);
      return (value != nullptr) ? _tcstoul(value, nullptr, 0) : defaultValue;
   }

   bool getAsBoolean(const TCHAR *key, bool defaultValue = false) const;
};

/**
 * API for subagents
 */
bool LIBNXAGENT_EXPORTABLE AgentGetMetricArgA(const TCHAR *param, int index, char *arg, size_t maxSize, bool inBrackets = true);
bool LIBNXAGENT_EXPORTABLE AgentGetMetricArgW(const TCHAR *param, int index, WCHAR *arg, size_t maxSize, bool inBrackets = true);
#ifdef UNICODE
#define AgentGetMetricArg AgentGetMetricArgW
#define AgentGetParameterArg AgentGetMetricArgW
#else
#define AgentGetMetricArg AgentGetMetricArgA
#define AgentGetParameterArg AgentGetMetricArgA
#endif
#define AgentGetParameterArgA AgentGetMetricArgA
#define AgentGetParameterArgW AgentGetMetricArgW
InetAddress LIBNXAGENT_EXPORTABLE AgentGetMetricArgAsInetAddress(const TCHAR *metric, int index, bool inBrackets = true);
bool LIBNXAGENT_EXPORTABLE AgentGetMetricArgAsInt16(const TCHAR *metric, int index, int16_t *value, int16_t defaultValue = 0, bool inBrackets = true);
bool LIBNXAGENT_EXPORTABLE AgentGetMetricArgAsUInt16(const TCHAR *metric, int index, uint16_t *value, uint16_t defaultValue = 0, bool inBrackets = true);
bool LIBNXAGENT_EXPORTABLE AgentGetMetricArgAsInt32(const TCHAR *metric, int index, int32_t *value, int32_t defaultValue = 0, bool inBrackets = true);
bool LIBNXAGENT_EXPORTABLE AgentGetMetricArgAsUInt32(const TCHAR *metric, int index, uint32_t *value, uint32_t defaultValue = 0, bool inBrackets = true);
bool LIBNXAGENT_EXPORTABLE AgentGetMetricArgAsInt64(const TCHAR *metric, int index, int64_t *value, int64_t defaultValue = 0, bool inBrackets = true);
bool LIBNXAGENT_EXPORTABLE AgentGetMetricArgAsUInt64(const TCHAR *metric, int index, uint64_t *value, uint64_t defaultValue = 0, bool inBrackets = true);
bool LIBNXAGENT_EXPORTABLE AgentGetMetricArgAsBoolean(const TCHAR *metric, int index, bool *value, bool defaultValue = false, bool inBrackets = true);

void LIBNXAGENT_EXPORTABLE AgentPostEvent(uint32_t eventCode, const TCHAR *eventName, time_t timestamp);
void LIBNXAGENT_EXPORTABLE AgentPostEvent(uint32_t eventCode, const TCHAR *eventName, time_t timestamp, const StringMap &args);
void LIBNXAGENT_EXPORTABLE AgentQueueNotificationMessage(NXCPMessage *msg);

bool LIBNXAGENT_EXPORTABLE AgentEnumerateSessions(EnumerationCallbackResult (* callback)(AbstractCommSession *, void *), void *data);
shared_ptr<AbstractCommSession> LIBNXAGENT_EXPORTABLE AgentFindServerSession(uint64_t serverId);

bool LIBNXAGENT_EXPORTABLE AgentPushParameterData(const TCHAR *parameter, const TCHAR *value);
bool LIBNXAGENT_EXPORTABLE AgentPushParameterDataInt32(const TCHAR *parameter, LONG value);
bool LIBNXAGENT_EXPORTABLE AgentPushParameterDataUInt32(const TCHAR *parameter, uint32_t value);
bool LIBNXAGENT_EXPORTABLE AgentPushParameterDataInt64(const TCHAR *parameter, int64_t value);
bool LIBNXAGENT_EXPORTABLE AgentPushParameterDataUInt64(const TCHAR *parameter, uint64_t value);
bool LIBNXAGENT_EXPORTABLE AgentPushParameterDataDouble(const TCHAR *parameter, double value);

const TCHAR LIBNXAGENT_EXPORTABLE *AgentGetDataDirectory();

DB_HANDLE LIBNXAGENT_EXPORTABLE AgentGetLocalDatabaseHandle();

void LIBNXAGENT_EXPORTABLE AgentExecuteAction(const TCHAR *action, const StringList& args);

bool LIBNXAGENT_EXPORTABLE AgentGetScreenInfoForUserSession(uint32_t sessionId, uint32_t *width, uint32_t *height, uint32_t *bpp);

void LIBNXAGENT_EXPORTABLE AgentRegisterProblem(int severity, const TCHAR *key, const TCHAR *message);
void LIBNXAGENT_EXPORTABLE AgentUnregisterProblem(const TCHAR *key);

void LIBNXAGENT_EXPORTABLE AgentSetTimer(uint32_t delay, std::function<void()> callback);

TCHAR LIBNXAGENT_EXPORTABLE *ReadRegistryAsString(const TCHAR *attr, TCHAR *buffer = nullptr, size_t bufferSize = 0, const TCHAR *defaultValue = nullptr);
int32_t LIBNXAGENT_EXPORTABLE ReadRegistryAsInt32(const TCHAR *attr, int32_t defaultValue);
int64_t LIBNXAGENT_EXPORTABLE ReadRegistryAsInt64(const TCHAR *attr, int64_t defaultValue);
bool LIBNXAGENT_EXPORTABLE WriteRegistry(const TCHAR *attr, const TCHAR *value);
bool LIBNXAGENT_EXPORTABLE WriteRegistry(const TCHAR *attr, int32_t value);
bool LIBNXAGENT_EXPORTABLE WriteRegistry(const TCHAR *attr, int64_t value);
bool LIBNXAGENT_EXPORTABLE DeleteRegistryEntry(const TCHAR *attr);

const char LIBNXAGENT_EXPORTABLE *SMBIOS_GetHardwareManufacturer();
const char LIBNXAGENT_EXPORTABLE *SMBIOS_GetHardwareProduct();
const char LIBNXAGENT_EXPORTABLE *SMBIOS_GetHardwareSerialNumber();
uuid LIBNXAGENT_EXPORTABLE SMBIOS_GetHardwareUUID();
const char LIBNXAGENT_EXPORTABLE *SMBIOS_GetBaseboardSerialNumber();
LIBNXAGENT_EXPORTABLE const char * const *SMBIOS_GetOEMStrings();
bool LIBNXAGENT_EXPORTABLE SMBIOS_Parse(BYTE *(*reader)(size_t*));
LONG LIBNXAGENT_EXPORTABLE SMBIOS_ParameterHandler(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG LIBNXAGENT_EXPORTABLE SMBIOS_BatteryParameterHandler(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG LIBNXAGENT_EXPORTABLE SMBIOS_MemDevParameterHandler(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG LIBNXAGENT_EXPORTABLE SMBIOS_ProcessorParameterHandler(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG LIBNXAGENT_EXPORTABLE SMBIOS_ListHandler(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG LIBNXAGENT_EXPORTABLE SMBIOS_TableHandler(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session);

bool LIBNXAGENT_EXPORTABLE GetSystemHardwareId(BYTE *hwid);
bool LIBNXAGENT_EXPORTABLE GetHardwareSerialNumber(char *buffer, size_t size);
bool LIBNXAGENT_EXPORTABLE GetUniqueSystemId(BYTE *sysid);

#ifdef _WIN32
ObjectArray<ProcessInformation> LIBNXAGENT_EXPORTABLE *GetProcessListForUserSession(DWORD sessionId);
bool LIBNXAGENT_EXPORTABLE CheckProcessPresenseInSession(DWORD sessionId, const TCHAR *processName);
#endif

void LIBNXAGENT_EXPORTABLE AddLocalCRL(const TCHAR *fileName);
void LIBNXAGENT_EXPORTABLE AddRemoteCRL(const char *url, bool allowDownload);
bool LIBNXAGENT_EXPORTABLE CheckCertificateRevocation(X509 *cert, const X509 *issuer);
void LIBNXAGENT_EXPORTABLE ReloadAllCRLs();

void LIBNXAGENT_EXPORTABLE TCPScanAddressRange(const InetAddress& from, const InetAddress& to, uint16_t port, void (*callback)(const InetAddress&, uint32_t, void*), void *context);

int LIBNXAGENT_EXPORTABLE TextToDataType(const TCHAR *name);

#if WITH_MODBUS
typedef struct _modbus modbus_t;
bool LIBNXAGENT_EXPORTABLE ModbusCheckConnection(const InetAddress& addr, uint16_t port, int32_t unitId);
int LIBNXAGENT_EXPORTABLE ModbusExecute(const InetAddress& addr, uint16_t port, int32_t unitId, std::function<int32_t (modbus_t*, const char*, uint16_t, int32_t)> callback, int commErrorStatus);
int LIBNXAGENT_EXPORTABLE ModbusReadHoldingRegisters(modbus_t *mb, int address, const TCHAR *conversion, TCHAR *value, size_t size);
int LIBNXAGENT_EXPORTABLE ModbusReadInputRegisters(modbus_t *mb, int address, const TCHAR *conversion, TCHAR *value, size_t size);
#endif

/* legacy logging functions - to be removed */
static inline void AgentWriteLog(int logLevel, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   nxlog_write2(logLevel, format, args);
   va_end(args);
}

static inline void AgentWriteDebugLog(int level, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   nxlog_debug2(level, format, args);
   va_end(args);
}

#define AgentWriteLog2 nxlog_write2
#define AgentWriteDebugLog2 nxlog_debug2

/**
 * TFTP client error codes
 */
enum TFTPError
{
   TFTP_SUCCESS = 0,
   TFTP_FILE_READ_ERROR = 1,
   TFTP_FILE_WRITE_ERROR = 2,
   TFTP_SOCKET_ERROR = 3,
   TFTP_TIMEOUT = 4,
   TFTP_PROTOCOL_ERROR = 5,
   TFTP_FILE_NOT_FOUND = 6,
   TFTP_ACCESS_VIOLATION = 7,
   TFTP_DISK_FULL = 8,
   TFTP_ILLEGAL_OPERATION = 9,
   TFTP_UNKNOWN_TRANSFER_ID = 10,
   TFTP_FILE_ALREADY_EXISTS = 11,
   TFTP_NO_SUCH_USER = 12
};

/**
 * TFTP client implementation
 */
TFTPError LIBNXAGENT_EXPORTABLE TFTPWrite(const TCHAR *fileName, const TCHAR *remoteFileName, const InetAddress& addr, uint16_t port = 69, std::function<void (size_t)> progressCallback = std::function<void (size_t)>());
TFTPError LIBNXAGENT_EXPORTABLE TFTPWrite(const BYTE *data, size_t size, const TCHAR *remoteFileName, const InetAddress& addr, uint16_t port = 69, std::function<void (size_t)> progressCallback = std::function<void (size_t)>());
TFTPError LIBNXAGENT_EXPORTABLE TFTPWrite(std::istream *stream, const TCHAR *remoteFileName, const InetAddress& addr, uint16_t port = 69, std::function<void (size_t)> progressCallback = std::function<void (size_t)>());
TFTPError LIBNXAGENT_EXPORTABLE TFTPRead(const TCHAR *fileName, const TCHAR *remoteFileName, const InetAddress& addr, uint16_t port = 69, std::function<void (size_t)> progressCallback = std::function<void (size_t)>());
TFTPError LIBNXAGENT_EXPORTABLE TFTPRead(ByteStream *output, const TCHAR *remoteFileName, const InetAddress& addr, uint16_t port = 69, std::function<void (size_t)> progressCallback = std::function<void (size_t)>());
TFTPError LIBNXAGENT_EXPORTABLE TFTPRead(std::ostream *stream, const TCHAR *remoteFileName, const InetAddress& addr, uint16_t port = 69, std::function<void (size_t)> progressCallback = std::function<void (size_t)>());
const TCHAR LIBNXAGENT_EXPORTABLE *TFTPErrorMessage(TFTPError code);

/**
 * Wrapper for SleepAndCheckForShutdownEx (for backward compatibility)
 * @return True if shutdown was initiated.
 */
static inline bool AgentSleepAndCheckForShutdown(uint32_t milliseconds)
{
   return SleepAndCheckForShutdownEx(milliseconds);
}

#endif   /* _nms_agent_h_ */
