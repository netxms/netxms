/* 
** NetXMS - Network Management System
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
** File: nms_agent.h
**
**/

#ifndef _nms_agent_h_
#define _nms_agent_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nxconfig.h>

/**
 * Initialization function declaration macro
 */
#if defined(_STATIC_AGENT) || defined(_NETWARE)
#define DECLARE_SUBAGENT_ENTRY_POINT(name) extern "C" BOOL NxSubAgentRegister_##name(NETXMS_SUBAGENT_INFO **ppInfo, Config *config)
#else
#ifdef _WIN32
#define DECLSPEC_EXPORT __declspec(dllexport) __cdecl
#else
#define DECLSPEC_EXPORT
#endif
#define DECLARE_SUBAGENT_ENTRY_POINT(name) extern "C" BOOL DECLSPEC_EXPORT NxSubAgentRegister(NETXMS_SUBAGENT_INFO **ppInfo, Config *config)
#endif

/**
 * Constants
 */
#define AGENT_LISTEN_PORT        4700
#define AGENT_PROTOCOL_VERSION   2
#define MAX_RESULT_LENGTH        256
#define MAX_CMD_LEN              256
#define COMMAND_TIMEOUT          60
#define MAX_SUBAGENT_NAME        64
#define MAX_INSTANCE_COLUMNS     8

/**
 * Agent policy types
 */
#define AGENT_POLICY_CONFIG      1
#define AGENT_POLICY_LOG_PARSER  2

/**
 * Error codes
 */
#define ERR_SUCCESS                 ((DWORD)0)
#define ERR_UNKNOWN_COMMAND         ((DWORD)400)
#define ERR_AUTH_REQUIRED           ((DWORD)401)
#define ERR_ACCESS_DENIED           ((DWORD)403)
#define ERR_UNKNOWN_PARAMETER       ((DWORD)404)
#define ERR_REQUEST_TIMEOUT         ((DWORD)408)
#define ERR_AUTH_FAILED             ((DWORD)440)
#define ERR_ALREADY_AUTHENTICATED   ((DWORD)441)
#define ERR_AUTH_NOT_REQUIRED       ((DWORD)442)
#define ERR_INTERNAL_ERROR          ((DWORD)500)
#define ERR_NOT_IMPLEMENTED         ((DWORD)501)
#define ERR_OUT_OF_RESOURCES        ((DWORD)503)
#define ERR_NOT_CONNECTED           ((DWORD)900)
#define ERR_CONNECTION_BROKEN       ((DWORD)901)
#define ERR_BAD_RESPONSE            ((DWORD)902)
#define ERR_IO_FAILURE              ((DWORD)903)
#define ERR_RESOURCE_BUSY           ((DWORD)904)
#define ERR_EXEC_FAILED             ((DWORD)905)
#define ERR_ENCRYPTION_REQUIRED     ((DWORD)906)
#define ERR_NO_CIPHERS              ((DWORD)907)
#define ERR_INVALID_PUBLIC_KEY      ((DWORD)908)
#define ERR_INVALID_SESSION_KEY     ((DWORD)909)
#define ERR_CONNECT_FAILED          ((DWORD)910)
#define ERR_MALFORMED_COMMAND       ((DWORD)911)
#define ERR_SOCKET_ERROR            ((DWORD)912)
#define ERR_BAD_ARGUMENTS           ((DWORD)913)
#define ERR_SUBAGENT_LOAD_FAILED    ((DWORD)914)
#define ERR_FILE_OPEN_ERROR         ((DWORD)915)
#define ERR_FILE_STAT_FAILED        ((DWORD)916)
#define ERR_MEM_ALLOC_FAILED        ((DWORD)917)
#define ERR_FILE_DELETE_FAILED      ((DWORD)918)

/**
 * Parameter handler return codes
 */
#define SYSINFO_RC_SUCCESS       0
#define SYSINFO_RC_UNSUPPORTED   1
#define SYSINFO_RC_ERROR         2

/**
 * WinPerf features
 */
#define WINPERF_AUTOMATIC_SAMPLE_COUNT    ((DWORD)0x00000001)
#define WINPERF_REMOTE_COUNTER_CONFIG     ((DWORD)0x00000002)

/**
 * Descriptions for common parameters
 */
#define DCIDESC_FS_AVAIL                          _T("Available space on file system {instance}")
#define DCIDESC_FS_AVAILPERC                      _T("Percentage of available space on file system {instance}")
#define DCIDESC_FS_FREE                           _T("Free space on file system {instance}")
#define DCIDESC_FS_FREEPERC                       _T("Percentage of free space on file system {instance}")
#define DCIDESC_FS_TOTAL                          _T("Total space on file system {instance}")
#define DCIDESC_FS_USED                           _T("Used space on file system {instance}")
#define DCIDESC_FS_USEDPERC                       _T("Percentage of used space on file system {instance}")
#define DCIDESC_NET_INTERFACE_ADMINSTATUS         _T("Administrative status of interface {instance}")
#define DCIDESC_NET_INTERFACE_BYTESIN             _T("Number of input bytes on interface {instance}")
#define DCIDESC_NET_INTERFACE_BYTESOUT            _T("Number of output bytes on interface {instance}")
#define DCIDESC_NET_INTERFACE_DESCRIPTION         _T("Description of interface {instance}")
#define DCIDESC_NET_INTERFACE_INERRORS            _T("Number of input errors on interface {instance}")
#define DCIDESC_NET_INTERFACE_LINK                _T("Link status for interface {instance}")
#define DCIDESC_NET_INTERFACE_MTU                 _T("MTU for interface {instance}")
#define DCIDESC_NET_INTERFACE_OPERSTATUS          _T("Operational status of interface {instance}")
#define DCIDESC_NET_INTERFACE_OUTERRORS           _T("Number of output errors on interface {instance}")
#define DCIDESC_NET_INTERFACE_PACKETSIN           _T("Number of input packets on interface {instance}")
#define DCIDESC_NET_INTERFACE_PACKETSOUT          _T("Number of output packets on interface {instance}")
#define DCIDESC_NET_INTERFACE_SPEED               _T("Speed of interface {instance}")
#define DCIDESC_NET_IP_FORWARDING                 _T("IP forwarding status")
#define DCIDESC_NET_IP6_FORWARDING                _T("IPv6 forwarding status")
#define DCIDESC_PHYSICALDISK_FIRMWARE             _T("Firmware version of hard disk {instance}")
#define DCIDESC_PHYSICALDISK_MODEL                _T("Model of hard disk {instance}")
#define DCIDESC_PHYSICALDISK_SERIALNUMBER         _T("Serial number of hard disk {instance}")
#define DCIDESC_PHYSICALDISK_SMARTATTR            _T("")
#define DCIDESC_PHYSICALDISK_SMARTSTATUS          _T("Status of hard disk {instance} reported by SMART")
#define DCIDESC_PHYSICALDISK_TEMPERATURE          _T("Temperature of hard disk {instance}")
#define DCIDESC_SYSTEM_CPU_COUNT                  _T("Number of CPU in the system")
#define DCIDESC_SYSTEM_HOSTNAME                   _T("Host name")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE       _T("Free physical memory")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE_PCT   _T("Percentage of free physical memory")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_TOTAL      _T("Total amount of physical memory")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED       _T("Used physical memory")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_USED_PCT   _T("Percentage of used physical memory")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_AVAILABLE  _T("Available physical memory")
#define DCIDESC_SYSTEM_MEMORY_PHYSICAL_AVAILABLE_PCT _T("Percentage of available physical memory")
#define DCIDESC_SYSTEM_MEMORY_VIRTUAL_ACTIVE      _T("Active virtual memory")
#define DCIDESC_SYSTEM_MEMORY_VIRTUAL_ACTIVE_PCT  _T("Percentage of active virtual memory")
#define DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE        _T("Free virtual memory")
#define DCIDESC_SYSTEM_MEMORY_VIRTUAL_FREE_PCT    _T("Percentage of free virtual memory")
#define DCIDESC_SYSTEM_MEMORY_VIRTUAL_TOTAL       _T("Total amount of virtual memory")
#define DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED        _T("Used virtual memory")
#define DCIDESC_SYSTEM_MEMORY_VIRTUAL_USED_PCT    _T("Percentage of used virtual memory")
#define DCIDESC_SYSTEM_MEMORY_VIRTUAL_AVAILABLE   _T("Available virtual memory")
#define DCIDESC_SYSTEM_MEMORY_VIRTUAL_AVAILABLE_PCT _T("Percentage of available virtual memory")
#define DCIDESC_SYSTEM_MEMORY_SWAP_FREE           _T("Free swap space")
#define DCIDESC_SYSTEM_MEMORY_SWAP_FREE_PCT       _T("Percentage of free swap space")
#define DCIDESC_SYSTEM_MEMORY_SWAP_TOTAL          _T("Total amount of swap space")
#define DCIDESC_SYSTEM_MEMORY_SWAP_USED           _T("Used swap space")
#define DCIDESC_SYSTEM_MEMORY_SWAP_USED_PCT       _T("Percentage of used swap space")
#define DCIDESC_SYSTEM_UNAME                      _T("System uname")
#define DCIDESC_AGENT_ACCEPTEDCONNECTIONS         _T("Number of connections accepted by agent")
#define DCIDESC_AGENT_ACCEPTERRORS                _T("Number of accept() call errors")
#define DCIDESC_AGENT_ACTIVECONNECTIONS           _T("Number of active connections to agent")
#define DCIDESC_AGENT_AUTHENTICATIONFAILURES      _T("Number of authentication failures")
#define DCIDESC_AGENT_CONFIG_SERVER               _T("Configuration server address set on agent startup")
#define DCIDESC_AGENT_FAILEDREQUESTS              _T("Number of failed requests to agent")
#define DCIDESC_AGENT_GENERATED_TRAPS             _T("Number of traps generated by agent")
#define DCIDESC_AGENT_IS_SUBAGENT_LOADED          _T("Check if given subagent is loaded")
#define DCIDESC_AGENT_LAST_TRAP_TIME              _T("Timestamp of last generated trap")
#define DCIDESC_AGENT_PROCESSEDREQUESTS           _T("Number of requests processed by agent")
#define DCIDESC_AGENT_REGISTRAR                   _T("Registrar server address set on agent startup")
#define DCIDESC_AGENT_REJECTEDCONNECTIONS         _T("Number of connections rejected by agent")
#define DCIDESC_AGENT_SENT_TRAPS                  _T("Number of traps successfully sent to server")
#define DCIDESC_AGENT_SOURCEPACKAGESUPPORT        _T("Check if source packages are supported")
#define DCIDESC_AGENT_SUPPORTEDCIPHERS            _T("List of ciphers supported by agent")
#define DCIDESC_AGENT_TIMEDOUTREQUESTS            _T("Number of timed out requests to agent")
#define DCIDESC_AGENT_UNSUPPORTEDREQUESTS         _T("Number of requests for unsupported parameters")
#define DCIDESC_AGENT_UPTIME                      _T("Agent's uptime")
#define DCIDESC_AGENT_VERSION                     _T("Agent's version")
#define DCIDESC_FILE_COUNT                        _T("Number of files {instance}")
#define DCIDESC_FILE_HASH_CRC32                   _T("CRC32 checksum of {instance}")
#define DCIDESC_FILE_HASH_MD5                     _T("MD5 hash of {instance}")
#define DCIDESC_FILE_HASH_SHA1                    _T("SHA1 hash of {instance}")
#define DCIDESC_FILE_SIZE                         _T("Size of file {instance}")
#define DCIDESC_FILE_TIME_ACCESS                  _T("Time of last access to file {instance}")
#define DCIDESC_FILE_TIME_CHANGE                  _T("Time of last status change of file {instance}")
#define DCIDESC_FILE_TIME_MODIFY                  _T("Time of last modification of file {instance}")
#define DCIDESC_SYSTEM_CURRENTTIME                _T("Current system time")
#define DCIDESC_SYSTEM_PLATFORMNAME               _T("Platform name")
#define DCIDESC_PROCESS_COUNT                     _T("Number of {instance} processes")
#define DCIDESC_PROCESS_COUNTEX                   _T("Number of {instance} processes (extended)")
#define DCIDESC_PROCESS_CPUTIME                   _T("Total execution time for process {instance}")
#define DCIDESC_PROCESS_GDIOBJ                    _T("GDI objects used by process {instance}")
#define DCIDESC_PROCESS_IO_OTHERB                 _T("")
#define DCIDESC_PROCESS_IO_OTHEROP                _T("")
#define DCIDESC_PROCESS_IO_READB                  _T("")
#define DCIDESC_PROCESS_IO_READOP                 _T("")
#define DCIDESC_PROCESS_IO_WRITEB                 _T("")
#define DCIDESC_PROCESS_IO_WRITEOP                _T("")
#define DCIDESC_PROCESS_KERNELTIME                _T("Total execution time in kernel mode for process {instance}")
#define DCIDESC_PROCESS_PAGEFAULTS                _T("Page faults for process {instance}")
#define DCIDESC_PROCESS_SYSCALLS                  _T("Number of system calls made by process {instance}")
#define DCIDESC_PROCESS_THREADS                   _T("Number of threads in process {instance}")
#define DCIDESC_PROCESS_USEROBJ                   _T("USER objects used by process {instance}")
#define DCIDESC_PROCESS_USERTIME                  _T("Total execution time in user mode for process {instance}")
#define DCIDESC_PROCESS_VMSIZE                    _T("Virtual memory used by process {instance}")
#define DCIDESC_PROCESS_WKSET                     _T("Physical memory used by process {instance}")
#define DCIDESC_SYSTEM_APPADDRESSSPACE            _T("Address space available to applications (MB)")
#define DCIDESC_SYSTEM_CONNECTEDUSERS             _T("Number of logged in users")
#define DCIDESC_SYSTEM_PROCESSCOUNT               _T("Total number of processes")
#define DCIDESC_SYSTEM_SERVICESTATE               _T("State of {instance} service")
#define DCIDESC_SYSTEM_PROCESSCOUNT               _T("Total number of processes")
#define DCIDESC_SYSTEM_THREADCOUNT                _T("Total number of threads")
#define DCIDESC_PDH_COUNTERVALUE                  _T("Value of PDH counter {instance}")
#define DCIDESC_PDH_VERSION                       _T("Version of PDH.DLL")
#define DCIDESC_SYSTEM_UPTIME                     _T("System uptime")
#define DCIDESC_SYSTEM_CPU_LOADAVG                _T("Average CPU load for last minute")
#define DCIDESC_SYSTEM_CPU_LOADAVG5               _T("Average CPU load for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_LOADAVG15              _T("Average CPU load for last 15 minutes")


#define DCIDESC_SYSTEM_CPU_USAGE_EX               _T("Average CPU {instance} utilization for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_EX              _T("Average CPU {instance} utilization for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_EX             _T("Average CPU {instance} utilization for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE                  _T("Average CPU utilization for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5                 _T("Average CPU utilization for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15                _T("Average CPU utilization for last 15 minutes")

#define DCIDESC_SYSTEM_CPU_USAGE_USER_EX          _T("Average CPU {instance} utilization (user) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_USER_EX         _T("Average CPU {instance} utilization (user) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_USER_EX        _T("Average CPU {instance} utilization (user) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE_USER             _T("Average CPU utilization (user) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_USER            _T("Average CPU utilization (user) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_USER           _T("Average CPU utilization (user) for last 15 minutes")

#define DCIDESC_SYSTEM_CPU_USAGE_NICE_EX          _T("Average CPU {instance} utilization (nice) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_NICE_EX         _T("Average CPU {instance} utilization (nice) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_NICE_EX        _T("Average CPU {instance} utilization (nice) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE_NICE             _T("Average CPU utilization (nice) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_NICE            _T("Average CPU utilization (nice) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_NICE           _T("Average CPU utilization (nice) for last 15 minutes")

#define DCIDESC_SYSTEM_CPU_USAGE_SYSTEM_EX        _T("Average CPU {instance} utilization (system) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_SYSTEM_EX       _T("Average CPU {instance} utilization (system) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_SYSTEM_EX      _T("Average CPU {instance} utilization (system) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE_SYSTEM           _T("Average CPU utilization (system) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_SYSTEM          _T("Average CPU utilization (system) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_SYSTEM         _T("Average CPU utilization (system) for last 15 minutes")

#define DCIDESC_SYSTEM_CPU_USAGE_IDLE_EX		_T("Average CPU {instance} utilization (idle) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_IDLE_EX		_T("Average CPU {instance} utilization (idle) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_IDLE_EX		_T("Average CPU {instance} utilization (idle) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE_IDLE		_T("Average CPU utilization (idle) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_IDLE		_T("Average CPU utilization (idle) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_IDLE		_T("Average CPU utilization (idle) for last 15 minutes")

#define DCIDESC_SYSTEM_CPU_USAGE_IOWAIT_EX		_T("Average CPU {instance} utilization (iowait) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_IOWAIT_EX		_T("Average CPU {instance} utilization (iowait) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_IOWAIT_EX		_T("Average CPU {instance} utilization (iowait) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE_IOWAIT		_T("Average CPU utilization (iowait) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_IOWAIT		_T("Average CPU utilization (iowait) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_IOWAIT		_T("Average CPU utilization (iowait) for last 15 minutes")

#define DCIDESC_SYSTEM_CPU_USAGE_IRQ_EX		_T("Average CPU {instance} utilization (irq) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_IRQ_EX		_T("Average CPU {instance} utilization (irq) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_IRQ_EX		_T("Average CPU {instance} utilization (irq) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE_IRQ		_T("Average CPU utilization (irq) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_IRQ		_T("Average CPU utilization (irq) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_IRQ		_T("Average CPU utilization (irq) for last 15 minutes")

#define DCIDESC_SYSTEM_CPU_USAGE_SOFTIRQ_EX		_T("Average CPU {instance} utilization (softirq) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_SOFTIRQ_EX		_T("Average CPU {instance} utilization (softirq) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_SOFTIRQ_EX		_T("Average CPU {instance} utilization (softirq) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE_SOFTIRQ		_T("Average CPU utilization (softirq) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_SOFTIRQ		_T("Average CPU utilization (softirq) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_SOFTIRQ		_T("Average CPU utilization (softirq) for last 15 minutes")

#define DCIDESC_SYSTEM_CPU_USAGE_STEAL_EX         _T("Average CPU {instance} utilization (steal) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_STEAL_EX        _T("Average CPU {instance} utilization (steal) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_STEAL_EX       _T("Average CPU {instance} utilization (steal) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE_STEAL            _T("Average CPU utilization (steal) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_STEAL           _T("Average CPU utilization (steal) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_STEAL          _T("Average CPU utilization (steal) for last 15 minutes")

#define DCIDESC_SYSTEM_CPU_USAGE_GUEST_EX         _T("Average CPU {instance} utilization (guest) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_GUEST_EX        _T("Average CPU {instance} utilization (guest) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_GUEST_EX       _T("Average CPU {instance} utilization (guest) for last 15 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE_GUEST            _T("Average CPU utilization (guest) for last minute")
#define DCIDESC_SYSTEM_CPU_USAGE5_GUEST           _T("Average CPU utilization (guest) for last 5 minutes")
#define DCIDESC_SYSTEM_CPU_USAGE15_GUEST          _T("Average CPU utilization (guest) for last 15 minutes")

#define DCIDESC_SYSTEM_IO_DISKQUEUE               _T("Average disk queue length for last minute")
#define DCIDESC_SYSTEM_IO_DISKQUEUE_MIN           _T("Minimum disk queue length for last minute")
#define DCIDESC_SYSTEM_IO_DISKQUEUE_MAX           _T("Maximum disk queue length for last minute")
#define DCIDESC_SYSTEM_IO_DISKQUEUE_EX            _T("Average disk queue length of device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_DISKQUEUE_EX_MIN        _T("Minimum disk queue length of device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_DISKQUEUE_EX_MAX        _T("Maximum disk queue length of device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_DISKTIME                _T("Percent of CPU time spent on I/O for last minute")
#define DCIDESC_SYSTEM_IO_DISKTIME_EX             _T("Percent of CPU time spent on I/O on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_WAITTIME                _T("Average I/O request wait time")
#define DCIDESC_SYSTEM_IO_WAITTIME_EX             _T("Average I/O request wait time for device {instance}")
#define DCIDESC_SYSTEM_IO_READS                   _T("Average number of read operations for last minute")
#define DCIDESC_SYSTEM_IO_READS_MIN               _T("Minimum number of read operations for last minute")
#define DCIDESC_SYSTEM_IO_READS_MAX               _T("Maximum number of read operations for last minute")
#define DCIDESC_SYSTEM_IO_READS_EX                _T("Average number of read operations on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_READS_EX_MIN            _T("Minimum number of read operations on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_READS_EX_MAX            _T("Maximum number of read operations on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_WRITES                  _T("Average number of write operations for last minute")
#define DCIDESC_SYSTEM_IO_WRITES_MIN              _T("Minimum number of write operations for last minute")
#define DCIDESC_SYSTEM_IO_WRITES_MAX              _T("Maximum number of write operations for last minute")
#define DCIDESC_SYSTEM_IO_WRITES_EX               _T("Average number of write operations on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_WRITES_EX_MIN           _T("Minimum number of write operations on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_WRITES_EX_MAX           _T("Maximum number of write operations on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_XFERS                   _T("Average number of I/O transfers for last minute")
#define DCIDESC_SYSTEM_IO_XFERS_MIN               _T("Minimum number of I/O transfers for last minute")
#define DCIDESC_SYSTEM_IO_XFERS_MAX               _T("Maximum number of I/O transfers for last minute")
#define DCIDESC_SYSTEM_IO_XFERS_EX                _T("Average number of I/O transfers on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_XFERS_EX_MIN            _T("Minimum number of I/O transfers on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_XFERS_EX_MAX            _T("Maximum number of I/O transfers on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_BYTEREADS               _T("Average number of bytes read for last minute")
#define DCIDESC_SYSTEM_IO_BYTEREADS_MIN           _T("Minimum number of bytes read for last minute")
#define DCIDESC_SYSTEM_IO_BYTEREADS_MAX           _T("Maximum number of bytes read for last minute")
#define DCIDESC_SYSTEM_IO_BYTEREADS_EX            _T("Average number of bytes read on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_BYTEREADS_EX_MIN        _T("Minimum number of bytes read on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_BYTEREADS_EX_MAX        _T("Maximum number of bytes read on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_BYTEWRITES              _T("Average number of bytes written for last minute")
#define DCIDESC_SYSTEM_IO_BYTEWRITES_MIN          _T("Minimum number of bytes written for last minute")
#define DCIDESC_SYSTEM_IO_BYTEWRITES_MAX          _T("Maximum number of bytes written for last minute")
#define DCIDESC_SYSTEM_IO_BYTEWRITES_EX           _T("Average number of bytes written on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_BYTEWRITES_EX_MIN       _T("Minimum number of bytes written on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_BYTEWRITES_EX_MAX       _T("Maximum number of bytes written on device {instance} for last minute")
#define DCIDESC_SYSTEM_IO_OPENFILES               _T("Number of open files")


#define DCIDESC_DEPRECATED                        _T("<deprecated>")


#define DCTDESC_AGENT_SUBAGENTS                   _T("Load subagents")
#define DCTDESC_FILESYSTEM_VOLUMES                _T("File system volumes")
#define DCTDESC_SYSTEM_INSTALLED_PRODUCTS         _T("Installed products")
#define DCTDESC_SYSTEM_PROCESSES                  _T("Process table")

/**
 * Subagent's parameter information
 */
typedef struct
{
   TCHAR name[MAX_PARAM_NAME];
   LONG (* handler)(const TCHAR *, const TCHAR *, TCHAR *);
   const TCHAR *arg;
   int dataType;		// Use DT_DEPRECATED to indicate deprecated parameter
   TCHAR description[MAX_DB_STRING];
} NETXMS_SUBAGENT_PARAM;

/**
 * Subagent's push parameter information
 */
typedef struct
{
   TCHAR name[MAX_PARAM_NAME];
   int dataType;
   TCHAR description[MAX_DB_STRING];
} NETXMS_SUBAGENT_PUSHPARAM;

/**
 * Subagent's list information
 */
typedef struct
{
   TCHAR name[MAX_PARAM_NAME];
   LONG (* handler)(const TCHAR *, const TCHAR *, StringList *);
   const TCHAR *arg;
} NETXMS_SUBAGENT_LIST;

/**
 * Subagent's table information
 */
typedef struct
{
   TCHAR name[MAX_PARAM_NAME];
   LONG (* handler)(const TCHAR *, const TCHAR *, Table *);
   const TCHAR *arg;
	TCHAR instanceColumns[MAX_COLUMN_NAME * MAX_INSTANCE_COLUMNS];
   TCHAR description[MAX_DB_STRING];
} NETXMS_SUBAGENT_TABLE;

/**
 * Subagent's action information
 */
typedef struct
{
   TCHAR name[MAX_PARAM_NAME];
   LONG (* handler)(const TCHAR *, StringList *, const TCHAR *);
   const TCHAR *arg;
   TCHAR description[MAX_DB_STRING];
} NETXMS_SUBAGENT_ACTION;

/**
 * Subagent initialization structure
 */
#define NETXMS_SUBAGENT_INFO_MAGIC     ((DWORD)0x20110301)

class CSCPMessage;

typedef struct
{
   DWORD magic;    // Magic number to check if subagent uses correct version of this structure
   TCHAR name[MAX_SUBAGENT_NAME];
   TCHAR version[32];
	BOOL (* init)(Config *);   // Called to initialize subagent. Can be NULL.
   void (* shutdown)();       // Called at subagent unload. Can be NULL.
   BOOL (* commandHandler)(DWORD dwCommand, CSCPMessage *pRequest,
                           CSCPMessage *pResponse, void *session);
   DWORD numParameters;
   NETXMS_SUBAGENT_PARAM *parameters;
   DWORD numLists;
   NETXMS_SUBAGENT_LIST *lists;
   DWORD numTables;
   NETXMS_SUBAGENT_TABLE *tables;
   DWORD numActions;
   NETXMS_SUBAGENT_ACTION *actions;
	DWORD numPushParameters;
	NETXMS_SUBAGENT_PUSHPARAM *pushParameters;
} NETXMS_SUBAGENT_INFO;

/**
 * Inline functions for returning parameters
 */
inline void ret_string(TCHAR *rbuf, const TCHAR *value)
{
	nx_strncpy(rbuf, value, MAX_RESULT_LENGTH);
}

inline void ret_wstring(TCHAR *rbuf, const WCHAR *value)
{
#ifdef UNICODE
	nx_strncpy(rbuf, value, MAX_RESULT_LENGTH);
#else
	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, value, -1, rbuf, MAX_RESULT_LENGTH, NULL, NULL);
	rbuf[MAX_RESULT_LENGTH - 1] = 0;
#endif
}

inline void ret_mbstring(TCHAR *rbuf, const char *value)
{
#ifdef UNICODE
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, value, -1, rbuf, MAX_RESULT_LENGTH);
	rbuf[MAX_RESULT_LENGTH - 1] = 0;
#else
	nx_strncpy(rbuf, value, MAX_RESULT_LENGTH);
#endif
}

inline void ret_int(TCHAR *rbuf, LONG value)
{
#if defined(_WIN32) && (_MSC_VER >= 1300)
   _sntprintf_s(rbuf, MAX_RESULT_LENGTH, _TRUNCATE, _T("%ld"), (long)value);
#else
   _sntprintf(rbuf, MAX_RESULT_LENGTH, _T("%ld"), (long)value);
#endif
}

inline void ret_uint(TCHAR *rbuf, DWORD value)
{
#if defined(_WIN32) && (_MSC_VER >= 1300)
   _sntprintf_s(rbuf, MAX_RESULT_LENGTH, _TRUNCATE, _T("%lu"), (unsigned long)value);
#else
   _sntprintf(rbuf, MAX_RESULT_LENGTH, _T("%lu"), (unsigned long)value);
#endif
}

inline void ret_double(TCHAR *rbuf, double value)
{
#if defined(_WIN32) && (_MSC_VER >= 1300)
   _sntprintf_s(rbuf, MAX_RESULT_LENGTH, _TRUNCATE, _T("%f"), value);
#else
   _sntprintf(rbuf, MAX_RESULT_LENGTH, _T("%f"), value);
#endif
}

inline void ret_int64(TCHAR *rbuf, INT64 value)
{
#if defined(_WIN32) && (_MSC_VER >= 1300)
   _sntprintf_s(rbuf, MAX_RESULT_LENGTH, _TRUNCATE, _T("%I64d"), value);
#else    /* _WIN32 */
   _sntprintf(rbuf, MAX_RESULT_LENGTH, INT64_FMT, value);
#endif   /* _WIN32 */
}

inline void ret_uint64(TCHAR *rbuf, QWORD value)
{
#if defined(_WIN32) && (_MSC_VER >= 1300)
   _sntprintf_s(rbuf, MAX_RESULT_LENGTH, _TRUNCATE, _T("%I64u"), value);
#else    /* _WIN32 */
   _sntprintf(rbuf, MAX_RESULT_LENGTH, UINT64_FMT, value);
#endif   /* _WIN32 */
}


//
// API for subagents
//

BOOL LIBNETXMS_EXPORTABLE AgentGetParameterArgA(const TCHAR *param, int index, char *arg, int maxSize);
BOOL LIBNETXMS_EXPORTABLE AgentGetParameterArgW(const TCHAR *param, int index, WCHAR *arg, int maxSize);
#ifdef UNICODE
#define AgentGetParameterArg AgentGetParameterArgW
#else
#define AgentGetParameterArg AgentGetParameterArgA
#endif

void LIBNETXMS_EXPORTABLE AgentWriteLog(int logLevel, const TCHAR *format, ...);
void LIBNETXMS_EXPORTABLE AgentWriteLog2(int logLevel, const TCHAR *format, va_list args);
void LIBNETXMS_EXPORTABLE AgentWriteDebugLog(int level, const TCHAR *format, ...);
void LIBNETXMS_EXPORTABLE AgentWriteDebugLog2(int level, const TCHAR *format, va_list args);
void LIBNETXMS_EXPORTABLE AgentSendTrap(DWORD dwEvent, const TCHAR *eventName, const char *pszFormat, ...);
void LIBNETXMS_EXPORTABLE AgentSendTrap2(DWORD dwEvent, const TCHAR *eventName, int nCount, TCHAR **ppszArgList);
BOOL LIBNETXMS_EXPORTABLE AgentSendFileToServer(void *session, DWORD requestId, const TCHAR *file, long offset);
BOOL LIBNETXMS_EXPORTABLE AgentPushParameterData(const TCHAR *parameter, const TCHAR *value);
BOOL LIBNETXMS_EXPORTABLE AgentPushParameterDataInt32(const TCHAR *parameter, LONG value);
BOOL LIBNETXMS_EXPORTABLE AgentPushParameterDataUInt32(const TCHAR *parameter, DWORD value);
BOOL LIBNETXMS_EXPORTABLE AgentPushParameterDataInt64(const TCHAR *parameter, INT64 value);
BOOL LIBNETXMS_EXPORTABLE AgentPushParameterDataUInt64(const TCHAR *parameter, QWORD value);
BOOL LIBNETXMS_EXPORTABLE AgentPushParameterDataDouble(const TCHAR *parameter, double value);

#endif   /* _nms_agent_h_ */
