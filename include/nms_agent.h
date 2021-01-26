/*
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Victor Kirhenshtein
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
#define SSH_PORT                 22

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
#define ERR_SUCCESS                 ((UINT32)0)
#define ERR_PROCESSING              ((UINT32)102)
#define ERR_UNKNOWN_COMMAND         ((UINT32)400)
#define ERR_AUTH_REQUIRED           ((UINT32)401)
#define ERR_ACCESS_DENIED           ((UINT32)403)
#define ERR_UNKNOWN_PARAMETER       ((UINT32)404)
#define ERR_REQUEST_TIMEOUT         ((UINT32)408)
#define ERR_AUTH_FAILED             ((UINT32)440)
#define ERR_ALREADY_AUTHENTICATED   ((UINT32)441)
#define ERR_AUTH_NOT_REQUIRED       ((UINT32)442)
#define ERR_INTERNAL_ERROR          ((UINT32)500)
#define ERR_NOT_IMPLEMENTED         ((UINT32)501)
#define ERR_OUT_OF_RESOURCES        ((UINT32)503)
#define ERR_NOT_CONNECTED           ((UINT32)900)
#define ERR_CONNECTION_BROKEN       ((UINT32)901)
#define ERR_BAD_RESPONSE            ((UINT32)902)
#define ERR_IO_FAILURE              ((UINT32)903)
#define ERR_RESOURCE_BUSY           ((UINT32)904)
#define ERR_EXEC_FAILED             ((UINT32)905)
#define ERR_ENCRYPTION_REQUIRED     ((UINT32)906)
#define ERR_NO_CIPHERS              ((UINT32)907)
#define ERR_INVALID_PUBLIC_KEY      ((UINT32)908)
#define ERR_INVALID_SESSION_KEY     ((UINT32)909)
#define ERR_CONNECT_FAILED          ((UINT32)910)
#define ERR_MALFORMED_COMMAND       ((UINT32)911)
#define ERR_SOCKET_ERROR            ((UINT32)912)
#define ERR_BAD_ARGUMENTS           ((UINT32)913)
#define ERR_SUBAGENT_LOAD_FAILED    ((UINT32)914)
#define ERR_FILE_OPEN_ERROR         ((UINT32)915)
#define ERR_FILE_STAT_FAILED        ((UINT32)916)
#define ERR_MEM_ALLOC_FAILED        ((UINT32)917)
#define ERR_FILE_DELETE_FAILED      ((UINT32)918)
#define ERR_NO_SESSION_AGENT        ((UINT32)919)
#define ERR_SERVER_ID_UNSET         ((UINT32)920)
#define ERR_NO_SUCH_INSTANCE        ((UINT32)921)
#define ERR_OUT_OF_STATE_REQUEST    ((UINT32)922)
#define ERR_ENCRYPTION_ERROR        ((UINT32)923)
#define ERR_MALFORMED_RESPONSE      ((UINT32)924)
#define ERR_INVALID_OBJECT          ((UINT32)925)
#define ERR_FILE_ALREADY_EXISTS     ((UINT32)926)
#define ERR_FOLDER_ALREADY_EXISTS   ((UINT32)927)
#define ERR_INVALID_SSH_KEY_ID      ((UINT32)928)

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

/**
 * WinPerf features
 */
#define WINPERF_AUTOMATIC_SAMPLE_COUNT    ((UINT32)0x00000001)
#define WINPERF_REMOTE_COUNTER_CONFIG     ((UINT32)0x00000002)

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
 * Safely get authentication type from integer value
 */
inline WebServiceAuthType WebServiceAuthTypeFromInt(int type)
{
   return ((type >= static_cast<int>(WebServiceAuthType::NONE)) &&
           (type <= static_cast<int>(WebServiceAuthType::ANYSAFE))) ? static_cast<WebServiceAuthType>(type) : WebServiceAuthType::NONE;
}

/**
 * Policy change notification
 */
struct PolicyChangeNotification
{
   uuid guid;
   const TCHAR *type;
};

/**
 * Internal notification codes
 */
#define AGENT_NOTIFY_POLICY_INSTALLED  1

/**
 * Descriptions for common parameters
 */
#define DCIDESC_AGENT_ACCEPTEDCONNECTIONS            _T("Number of connections accepted by agent")
#define DCIDESC_AGENT_ACCEPTERRORS                   _T("Number of accept() call errors")
#define DCIDESC_AGENT_ACTIVECONNECTIONS              _T("Number of active connections to agent")
#define DCIDESC_AGENT_AUTHENTICATIONFAILURES         _T("Number of authentication failures")
#define DCIDESC_AGENT_CONFIG_SERVER                  _T("Configuration server address set on agent startup")
#define DCIDESC_AGENT_DATACOLLQUEUESIZE              _T("Agent data collector queue size")
#define DCIDESC_AGENT_FAILEDREQUESTS                 _T("Number of failed requests to agent")
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
#define DCIDESC_AGENT_LOCALDB_SLOW_QUERIES           _T("Agent local database: long running queries")
#define DCIDESC_AGENT_LOCALDB_STATUS                 _T("Agent local database: status")
#define DCIDESC_AGENT_LOCALDB_TOTAL_QUERIES          _T("Agent local database: total queries executed")
#define DCIDESC_AGENT_LOG_STATUS                     _T("Agent log status")
#define DCIDESC_AGENT_NOTIFICATIONPROC_QUEUESIZE     _T("Agent notification processor queue size")
#define DCIDESC_AGENT_PROCESSEDREQUESTS              _T("Number of requests processed by agent")
#define DCIDESC_AGENT_PROXY_ACTIVESESSIONS           _T("Number of active proxy sessions")
#define DCIDESC_AGENT_PROXY_CONNECTIONREQUESTS       _T("Number of proxy connection requests")
#define DCIDESC_AGENT_PROXY_ISENABLED                _T("Check if agent proxy is enabled")
#define DCIDESC_AGENT_PUSH_VALUE                     _T("Cached value of push parameter {instance}")
#define DCIDESC_AGENT_REGISTRAR                      _T("Registrar server address set on agent startup")
#define DCIDESC_AGENT_REJECTEDCONNECTIONS            _T("Number of connections rejected by agent")
#define DCIDESC_AGENT_SESSION_AGENTS_COUNT           _T("Number of connected session agents")
#define DCIDESC_AGENT_SNMP_ISPROXYENABLED            _T("Check if SNMP proxy is enabled")
#define DCIDESC_AGENT_SNMP_REQUESTS                  _T("Number of SNMP requests sent")
#define DCIDESC_AGENT_SNMP_RESPONSES                 _T("Number of SNMP responses received")
#define DCIDESC_AGENT_SNMP_SERVERREQUESTS            _T("Number of SNMP proxy requests received from server")
#define DCIDESC_AGENT_SNMP_TRAPS                     _T("Number of SNMP traps received")
#define DCIDESC_AGENT_SOURCEPACKAGESUPPORT           _T("Check if source packages are supported")
#define DCIDESC_AGENT_SUPPORTEDCIPHERS               _T("List of ciphers supported by agent")
#define DCIDESC_AGENT_SYSLOGPROXY_ISENABLED          _T("Check if syslog proxy is enabled")
#define DCIDESC_AGENT_SYSLOGPROXY_RECEIVEDMSGS       _T("Number of syslog messages received by agent")
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
#define DCIDESC_FILE_COUNT                           _T("Number of files {instance}")
#define DCIDESC_FILE_FOLDERCOUNT                     _T("Number of folders {instance}")
#define DCIDESC_FILE_HASH_CRC32                      _T("CRC32 checksum of {instance}")
#define DCIDESC_FILE_HASH_MD5                        _T("MD5 hash of {instance}")
#define DCIDESC_FILE_HASH_SHA1                       _T("SHA1 hash of {instance}")
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
#define DCIDESC_PHYSICALDISK_FIRMWARE                _T("Firmware version of hard disk {instance}")
#define DCIDESC_PHYSICALDISK_MODEL                   _T("Model of hard disk {instance}")
#define DCIDESC_PHYSICALDISK_SERIALNUMBER            _T("Serial number of hard disk {instance}")
#define DCIDESC_PHYSICALDISK_SMARTATTR               _T("")
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
#define DCIDESC_SYSTEM_CPU_FREQUENCY                 _T("CPU {instance}: frequency")
#define DCIDESC_SYSTEM_CPU_MODEL                     _T("CPU {instance}: model")
#define DCIDESC_SYSTEM_CPU_PHYSICAL_ID               _T("CPU {instance}: physical ID")
#define DCIDESC_SYSTEM_CURRENTTIME                   _T("Current system time")
#define DCIDESC_SYSTEM_FQDN                          _T("Fully qualified domain name")
#define DCIDESC_SYSTEM_HANDLECOUNT                   _T("Total number of handles")
#define DCIDESC_SYSTEM_HARDWAREID                    _T("Hardware ID")
#define DCIDESC_SYSTEM_HOSTNAME                      _T("Host name")
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
#define DCTDESC_SYSTEM_ACTIVE_USER_SESSIONS          _T("Active user sessions")
#define DCTDESC_SYSTEM_INSTALLED_PRODUCTS            _T("Installed products")
#define DCTDESC_SYSTEM_OPEN_FILES                    _T("Open files")
#define DCTDESC_SYSTEM_PROCESSES                     _T("Processes")

#ifdef __HP_aCC
#pragma pack 1
#else
#pragma pack(1)
#endif

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

#ifdef __HP_aCC
#pragma pack
#else
#pragma pack()
#endif

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

   virtual bool open();
   virtual bool write(const BYTE *data, size_t dataSize, bool compressedStream);
   virtual void close(bool success);

   const TCHAR *getFileName() const { return m_fileName; }
   time_t getLastUpdateTime() const { return m_lastUpdateTime; }
};

/**
 * API for CommSession
 */
class AbstractCommSession : public RefCountObject
{
public:
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

   virtual bool sendMessage(const NXCPMessage *msg) = 0;
   virtual void postMessage(const NXCPMessage *msg) = 0;
   virtual bool sendRawMessage(const NXCP_MESSAGE *msg) = 0;
   virtual void postRawMessage(const NXCP_MESSAGE *msg) = 0;
	virtual bool sendFile(UINT32 requestId, const TCHAR *file, long offset, bool allowCompression, VolatileCounter *cancelationFlag) = 0;
   virtual UINT32 doRequest(NXCPMessage *msg, UINT32 timeout) = 0;
   virtual NXCPMessage *doRequestEx(NXCPMessage *msg, UINT32 timeout) = 0;
   virtual NXCPMessage *waitForMessage(UINT16 code, UINT32 id, UINT32 timeout) = 0;
   virtual uint32_t generateRequestId() = 0;
   virtual int getProtocolVersion() = 0;
   virtual UINT32 openFile(TCHAR* nameOfFile, UINT32 requestId, time_t fileModTime = 0) = 0;
   virtual void debugPrintf(int level, const TCHAR *format, ...) = 0;
   virtual void prepareProxySessionSetupMsg(NXCPMessage *msg) = 0;
};

/**
 * Subagent's parameter information
 */
struct NETXMS_SUBAGENT_PARAM
{
   TCHAR name[MAX_PARAM_NAME];
   LONG (* handler)(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *);
   const TCHAR *arg;
   int dataType;		// Use DT_DEPRECATED to indicate deprecated parameter
   TCHAR description[MAX_DB_STRING];
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
   LONG (* handler)(const TCHAR *, const TCHAR *, StringList *, AbstractCommSession *);
   const TCHAR *arg;
   TCHAR description[MAX_DB_STRING];
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
};

/**
 * Subagent's action information
 */
struct NETXMS_SUBAGENT_ACTION
{
   TCHAR name[MAX_PARAM_NAME];
   LONG (* handler)(const TCHAR *, const StringList *, const TCHAR *, AbstractCommSession *);
   const TCHAR *arg;
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
   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, value, -1, rbuf, MAX_RESULT_LENGTH, NULL, NULL);
   rbuf[MAX_RESULT_LENGTH - 1] = 0;
#endif
}

static inline void ret_mbstring(TCHAR *rbuf, const char *value)
{
#ifdef UNICODE
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, value, -1, rbuf, MAX_RESULT_LENGTH);
   rbuf[MAX_RESULT_LENGTH - 1] = 0;
#else
   strlcpy(rbuf, value, MAX_RESULT_LENGTH);
#endif
}

static inline void ret_boolean(TCHAR *rbuf, bool value)
{
   rbuf[0] = value ? '1' : '0';
   rbuf[1] = 0;
}

static inline void ret_int(TCHAR *rbuf, LONG value)
{
#if defined(_WIN32) && (_MSC_VER >= 1300) && !defined(__clang__)
   _sntprintf_s(rbuf, MAX_RESULT_LENGTH, _TRUNCATE, _T("%ld"), (long)value);
#else
   _sntprintf(rbuf, MAX_RESULT_LENGTH, _T("%ld"), (long)value);
#endif
}

static inline void ret_uint(TCHAR *rbuf, UINT32 value)
{
#if defined(_WIN32) && (_MSC_VER >= 1300) && !defined(__clang__)
   _sntprintf_s(rbuf, MAX_RESULT_LENGTH, _TRUNCATE, _T("%lu"), (unsigned long)value);
#else
   _sntprintf(rbuf, MAX_RESULT_LENGTH, _T("%lu"), (unsigned long)value);
#endif
}

static inline void ret_double(TCHAR *rbuf, double value, int digits = 6)
{
#if defined(_WIN32) && (_MSC_VER >= 1300) && !defined(__clang__)
   _sntprintf_s(rbuf, MAX_RESULT_LENGTH, _TRUNCATE, _T("%1.*f"), digits, value);
#else
   _sntprintf(rbuf, MAX_RESULT_LENGTH, _T("%1.*f"), digits, value);
#endif
}

static inline void ret_int64(TCHAR *rbuf, INT64 value)
{
#if defined(_WIN32) && (_MSC_VER >= 1300) && !defined(__clang__)
   _sntprintf_s(rbuf, MAX_RESULT_LENGTH, _TRUNCATE, _T("%I64d"), value);
#else    /* _WIN32 */
   _sntprintf(rbuf, MAX_RESULT_LENGTH, INT64_FMT, value);
#endif   /* _WIN32 */
}

static inline void ret_uint64(TCHAR *rbuf, QWORD value)
{
#if defined(_WIN32) && (_MSC_VER >= 1300) && !defined(__clang__)
   _sntprintf_s(rbuf, MAX_RESULT_LENGTH, _TRUNCATE, _T("%I64u"), value);
#else    /* _WIN32 */
   _sntprintf(rbuf, MAX_RESULT_LENGTH, UINT64_FMT, value);
#endif   /* _WIN32 */
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
   virtual void onOutput(const char *text) override;
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
   virtual void onOutput(const char *text) override;
   virtual void endOfOutput() override;

public:
   LineOutputProcessExecutor(const TCHAR *command, bool shellExec = true);

   virtual bool execute() override { m_data.clear(); return ProcessExecutor::execute(); }

   const StringList& getData() const { return m_data; }
};

/**
 * LoraWAN device payload
 */
typedef BYTE lorawan_payload_t[36];

/**
 * Lora device data
 */
class LIBNXAGENT_EXPORTABLE LoraDeviceData
{
private:
   uuid m_guid;
   MacAddress m_devAddr;
   MacAddress m_devEui;
   lorawan_payload_t m_payload;
   INT32 m_decoder;
   char m_dataRate[24];
   INT32 m_rssi;
   double m_snr;
   double m_freq;
   UINT32 m_fcnt;
   UINT32 m_port;
   time_t m_lastContact;

public:
   LoraDeviceData(NXCPMessage *request);
   LoraDeviceData(DB_RESULT result, int row);

   UINT32 saveToDB(bool isNew = false) const;
   UINT32 deleteFromDB() const;

   bool isOtaa() const { return (m_devEui.length() > 0) ? true : false; }

   const uuid& getGuid() const { return m_guid; }

   const MacAddress& getDevAddr() const { return m_devAddr; }
   void setDevAddr(MacAddress devAddr) { m_devAddr = devAddr; saveToDB(); }
   const MacAddress& getDevEui() const { return m_devEui; }

   const BYTE *getPayload() const { return m_payload; }
   void setPayload(const char *payload) { StrToBinA(payload, m_payload, 36); }

   UINT32 getDecoder() { return m_decoder; }

   const char *getDataRate() const { return m_dataRate; }
   void setDataRate(const char *dataRate) { strlcpy(m_dataRate, dataRate, 24); }

   INT32 getRssi() const { return m_rssi; }
   void setRssi(INT32 rssi) { m_rssi = rssi; }

   double getSnr() const { return m_snr; }
   void setSnr(double snr) { m_snr = snr; }

   double getFreq() const { return m_freq; }
   void setFreq(double freq) { m_freq = freq; }

   UINT32 getFcnt() const { return m_fcnt; }
   void setFcnt(UINT32 fcnt) { m_fcnt = fcnt; }

   UINT32 getPort() const { return m_port; }
   void setPort(UINT32 port) { m_port = port; }

   INT32 getLastContact() const { return (INT32)m_lastContact; }
   void updateLastContact() { m_lastContact = time(NULL); }
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
 * API for subagents
 */
bool LIBNXAGENT_EXPORTABLE AgentGetParameterArgA(const TCHAR *param, int index, char *arg, int maxSize, bool inBrackets = true);
bool LIBNXAGENT_EXPORTABLE AgentGetParameterArgW(const TCHAR *param, int index, WCHAR *arg, int maxSize, bool inBrackets = true);
#ifdef UNICODE
#define AgentGetParameterArg AgentGetParameterArgW
#else
#define AgentGetParameterArg AgentGetParameterArgA
#endif

void LIBNXAGENT_EXPORTABLE AgentWriteLog(int logLevel, const TCHAR *format, ...)
#if !defined(UNICODE) && (defined(__GNUC__) || defined(__clang__))
   __attribute__ ((format(printf, 2, 3)))
#endif
;
void LIBNXAGENT_EXPORTABLE AgentWriteLog2(int logLevel, const TCHAR *format, va_list args);
void LIBNXAGENT_EXPORTABLE AgentWriteDebugLog(int level, const TCHAR *format, ...)
#if !defined(UNICODE) && (defined(__GNUC__) || defined(__clang__))
   __attribute__ ((format(printf, 2, 3)))
#endif
;
void LIBNXAGENT_EXPORTABLE AgentWriteDebugLog2(int level, const TCHAR *format, va_list args);

void LIBNXAGENT_EXPORTABLE AgentPostEvent(uint32_t eventCode, const TCHAR *eventName, time_t timestamp, const char *format, ...);
void LIBNXAGENT_EXPORTABLE AgentPostEvent2(uint32_t eventCode, const TCHAR *eventName, time_t timestamp, int count, const TCHAR **args);
void LIBNXAGENT_EXPORTABLE AgentQueueNotifictionMessage(NXCPMessage *msg);

bool LIBNXAGENT_EXPORTABLE AgentEnumerateSessions(EnumerationCallbackResult (* callback)(AbstractCommSession *, void *), void *data);
AbstractCommSession LIBNXAGENT_EXPORTABLE *AgentFindServerSession(UINT64 serverId);

bool LIBNXAGENT_EXPORTABLE AgentSendFileToServer(void *session, UINT32 requestId, const TCHAR *file, long offset, 
                                                 bool allowCompression, VolatileCounter *cancellationFlag);

bool LIBNXAGENT_EXPORTABLE AgentPushParameterData(const TCHAR *parameter, const TCHAR *value);
bool LIBNXAGENT_EXPORTABLE AgentPushParameterDataInt32(const TCHAR *parameter, LONG value);
bool LIBNXAGENT_EXPORTABLE AgentPushParameterDataUInt32(const TCHAR *parameter, UINT32 value);
bool LIBNXAGENT_EXPORTABLE AgentPushParameterDataInt64(const TCHAR *parameter, INT64 value);
bool LIBNXAGENT_EXPORTABLE AgentPushParameterDataUInt64(const TCHAR *parameter, QWORD value);
bool LIBNXAGENT_EXPORTABLE AgentPushParameterDataDouble(const TCHAR *parameter, double value);

const TCHAR LIBNXAGENT_EXPORTABLE *AgentGetDataDirectory();

DB_HANDLE LIBNXAGENT_EXPORTABLE AgentGetLocalDatabaseHandle();

void LIBNXAGENT_EXPORTABLE AgentExecuteAction(const TCHAR *action, const StringList& args);

bool LIBNXAGENT_EXPORTABLE AgentGetScreenInfoForUserSession(uint32_t sessionId, uint32_t *width, uint32_t *height, uint32_t *bpp);

TCHAR LIBNXAGENT_EXPORTABLE *ReadRegistryAsString(const TCHAR *attr, TCHAR *buffer = NULL, int bufSize = 0, const TCHAR *defaultValue = NULL);
INT32 LIBNXAGENT_EXPORTABLE ReadRegistryAsInt32(const TCHAR *attr, INT32 defaultValue);
INT64 LIBNXAGENT_EXPORTABLE ReadRegistryAsInt64(const TCHAR *attr, INT64 defaultValue);
bool LIBNXAGENT_EXPORTABLE WriteRegistry(const TCHAR *attr, const TCHAR *value);
bool LIBNXAGENT_EXPORTABLE WriteRegistry(const TCHAR *attr, INT32 value);
bool LIBNXAGENT_EXPORTABLE WriteRegistry(const TCHAR *attr, INT64 value);
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

#ifdef _WIN32
ObjectArray<ProcessInformation> LIBNXAGENT_EXPORTABLE *GetProcessListForUserSession(DWORD sessionId);
bool LIBNXAGENT_EXPORTABLE CheckProcessPresenseInSession(DWORD sessionId, const TCHAR *processName);
#endif

void LIBNXAGENT_EXPORTABLE AddLocalCRL(const TCHAR *fileName);
void LIBNXAGENT_EXPORTABLE AddRemoteCRL(const char *url, bool allowDownload);
bool LIBNXAGENT_EXPORTABLE CheckCertificateRevocation(X509 *cert, const X509 *issuer);
void LIBNXAGENT_EXPORTABLE ReloadAllCRLs();

/**
 * Wrapper for SleepAndCheckForShutdownEx (for backward compatibility)
 */
inline bool AgentSleepAndCheckForShutdown(UINT32 milliseconds)
{
   return SleepAndCheckForShutdownEx(milliseconds);
}

#endif   /* _nms_agent_h_ */
