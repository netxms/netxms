/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** $module: nms_common.h
**
**/

#ifndef _nms_common_h_
#define _nms_common_h_

#ifdef WIN32
#ifndef _WIN32
#define _WIN32
#endif
#endif

#if !defined(_WIN32) && !defined(UNDER_CE)
#ifdef _NETWARE
#include <config-netware.h>
#else
#include <config.h>
#endif
#endif

#include <unicode.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <netxms-version.h>


//
// Common constants
//

#define MAX_SECRET_LENGTH        64
#define INVALID_POINTER_VALUE    ((void *)0xFFFFFFFF)
#define MAX_DB_STRING            256
#define MAX_PARAM_NAME           256

#ifndef LLONG_MAX
#define LLONG_MAX    9223372036854775807
#endif

#ifndef LLONG_MIN
#define LLONG_MIN    (-LLONG_MAX - 1)
#endif

#ifndef ULLONG_MAX
#define ULLONG_MAX   18446744073709551615
#endif


//
// Platform dependent includes and defines
//

#if defined(_WIN32) || defined(UNDER_CE)

/********** WINDOWS ********************/

#define FS_PATH_SEPARATOR  _T("\\")

#include <winsock2.h>
#include <windows.h>

#ifndef UNDER_CE
#include <sys/stat.h>
#include <process.h>
#include <io.h>
#include <fcntl.h>
#endif

#define S_IRUSR      0400
#define S_IWUSR      0200

#define snprintf  _snprintf

typedef unsigned __int64 QWORD;
typedef __int64 INT64;
typedef int socklen_t;

// Socket compatibility
#define SHUT_RD      0
#define SHUT_WR      1
#define SHUT_RDWR    2

#define SetSocketReuseFlag(sd)

#elif defined(_NETWARE)

/********** NETWARE ********************/

#define FS_PATH_SEPARATOR  _T("/")

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <wchar.h>

typedef int BOOL;
typedef long int LONG;
typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef void * HANDLE;
typedef void * HMODULE;

#ifdef X_INT64_X
typedef X_INT64_X INT64;
#else
#error Target system does not have signed 64bit integer type
#endif

#ifdef X_UINT64_X
typedef X_UINT64_X QWORD;
#else
#error Target system does not have unsigned 64bit integer type
#endif

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

// Socket compatibility
typedef int SOCKET;

#define closesocket(x) close(x)
#define WSAGetLastError() (errno)

#define WSAEINTR  EINTR

//#define SetSocketReuseFlag(sd)
#define SetSocketReuseFlag(sd) { \
	int nVal = 1; \
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (const void *)&nVal,  \
			(socklen_t)sizeof(nVal)); \
}

#else    /* not _WIN32 and not _NETWARE */

/*********** UNIX *********************/

#include <errno.h>

#define FS_PATH_SEPARATOR  _T("/")

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_INT_TYPES_H
#include <sys/int_types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#if HAVE_NETDB_H
#include <netdb.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <dirent.h>

typedef int BOOL;
typedef long int LONG;
typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef void * HANDLE;
typedef void * HMODULE;

#if HAVE_INT64_T
typedef int64_t INT64;
#else
#error Target system does not have signed 64bit integer type
#endif

#if HAVE_UINT64_T
typedef uint64_t QWORD;
#elif HAVE_U_INT64_T
typedef u_int64_t QWORD;
#else
#error Target system does not have unsigned 64bit integer type
#endif


#define TRUE   1
#define FALSE  0

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

// Socket compatibility
typedef int SOCKET;

#define closesocket(x) close(x)
#define WSAGetLastError() (errno)

#define WSAEINTR  EINTR

#define SetSocketReuseFlag(sd) { \
	int nVal = 1; \
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (const void *)&nVal,  \
			(socklen_t)sizeof(nVal)); \
}

#endif   /* _WIN32 */


//
// open() flags compatibility
//

#ifndef O_BINARY
#define O_BINARY 0
#endif


//
// Event log severity codes (UNIX only)
//

#ifndef _WIN32
#define EVENTLOG_INFORMATION_TYPE   0
#define EVENTLOG_WARNING_TYPE       1
#define EVENTLOG_ERROR_TYPE         2
#endif   /* _WIN32 */


//
// Interface types
//

#define IFTYPE_OTHER                1
#define IFTYPE_REGULAR1822          2
#define IFTYPE_HDH1822              3
#define IFTYPE_DDN_X25              4
#define IFTYPE_RFC877_X25           5
#define IFTYPE_ETHERNET_CSMACD      6
#define IFTYPE_ISO88023_CSMACD      7
#define IFTYPE_ISO88024_TOKENBUS    8
#define IFTYPE_ISO88025_TOKENRING   9
#define IFTYPE_ISO88026_MAN         10
#define IFTYPE_STARLAN              11
#define IFTYPE_PROTEON_10MBIT       12
#define IFTYPE_PROTEON_80MBIT       13
#define IFTYPE_HYPERCHANNEL         14
#define IFTYPE_FDDI                 15
#define IFTYPE_LAPB                 16
#define IFTYPE_SDLC                 17
#define IFTYPE_DS1                  18
#define IFTYPE_E1                   19
#define IFTYPE_BASIC_ISDN           20
#define IFTYPE_PRIMARY_ISDN         21
#define IFTYPE_PROP_PTP_SERIAL      22
#define IFTYPE_PPP                  23
#define IFTYPE_SOFTWARE_LOOPBACK    24
#define IFTYPE_EON                  25
#define IFTYPE_ETHERNET_3MBIT       26
#define IFTYPE_NSIP                 27
#define IFTYPE_SLIP                 28
#define IFTYPE_ULTRA                29
#define IFTYPE_DS3                  30
#define IFTYPE_SMDS                 31
#define IFTYPE_FRAME_RELAY          32

#define IFTYPE_NETXMS_NAT_ADAPTER   65535


//
// IP Header -- RFC 791
//

typedef struct tagIPHDR
{
	BYTE m_cVIHL;	         // Version and IHL
	BYTE m_cTOS;			   // Type Of Service
	WORD m_wLen;			   // Total Length
	WORD m_wId;			      // Identification
	WORD m_wFlagOff;	      // Flags and Fragment Offset
	BYTE m_cTTL;			   // Time To Live
	BYTE m_cProtocol;	      // Protocol
	WORD m_wChecksum;	      // Checksum
	struct in_addr m_iaSrc;	// Internet Address - Source
	struct in_addr m_iaDst;	// Internet Address - Destination
} IPHDR;


//
// ICMP Header - RFC 792
//

typedef struct tagICMPHDR
{
	BYTE m_cType;			// Type
	BYTE m_cCode;			// Code
	WORD m_wChecksum;		// Checksum
	WORD m_wId;				// Identification
	WORD m_wSeq;			// Sequence
	char m_cData[1];		// Data
} ICMPHDR;


//
// INADDR_NONE can be undefined on some systems
//

#ifndef INADDR_NONE
#define INADDR_NONE	(0xFFFFFFFF)
#endif


//
// Check if IP address is a broadcast
//

#define IsBroadcastAddress(addr, mask) (((addr) & (~(mask))) == (~(mask)))


//
// Check if given string is NULL and always return valid pointer
//

#define CHECK_NULL(x)      ((x) == NULL ? ((TCHAR *)_T("(null)")) : (x))
#define CHECK_NULL_EX(x)   ((x) == NULL ? ((TCHAR *)_T("")) : (x))


//
// Free memory block if it isn't NULL
//

#define safe_free(x) { if ((x) != NULL) free(x); }


//
// Convert half-byte's value to hex digit and vice versa
//

#define bin2hex(x) ((x) < 10 ? ((x) + '0') : ((x) + ('A' - 10)))
#define hex2bin(x) ((((x) >= '0') && ((x) <= '9')) ? ((x) - '0') : \
                    (((toupper(x) >= 'A') && (toupper(x) <= 'F')) ? (toupper(x) - 'A' + 10) : 0))


//
// Define min() and max() if needed
//

#ifndef min
# define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
# define max(a, b) ((a) > (b) ? (a) : (b))
#endif


//
// Define stricmp() for non-windows
//

#ifndef _WIN32
#define stricmp strcasecmp
#endif


//
// Compare two numbers and return -1, 0, or 1
//

#define COMPARE_NUMBERS(n1,n2) (((n1) < (n2)) ? -1 : (((n1) > (n2)) ? 1 : 0))


//
// Increment pointer for given number of bytes
//

#define inc_ptr(ptr, step, ptype) ptr = (ptype *)(((char *)ptr) + step)


//
// DCI (data collection item) data types
//

#define DCI_DT_INT         0
#define DCI_DT_UINT        1
#define DCI_DT_INT64       2
#define DCI_DT_UINT64      3
#define DCI_DT_STRING      4
#define DCI_DT_FLOAT       5
#define DCI_DT_NULL        6


#endif   /* _nms_common_h_ */
