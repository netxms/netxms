/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006 Victor Kirhenshtein
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
#include "config.h"
#ifdef WITH_OPENSSL
#define _WITH_ENCRYPTION   1
#endif
#endif
#else    /* _WIN32 */
#ifndef UNDER_CE
#define _WITH_ENCRYPTION   1
#endif
#define _CRT_SECURE_NO_DEPRECATE
#endif

#include <unicode.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <netxms-version.h>


//
// Wrappers for 64-bit constants
//

#ifdef __GNUC__
#define _LL(x) (x ## LL)
#define _ULL(x) (x ## ULL)
#else
#define _LL(x) (x)
#define _ULL(x) (x)
#endif


//
// Common constants
//

#define MAX_SECRET_LENGTH        64
#define INVALID_POINTER_VALUE    ((void *)0xFFFFFFFF)
#define MAX_DB_STRING            256
#define MAX_PARAM_NAME           256
#define GROUP_FLAG               ((DWORD)0x80000000)

#define NETXMS_MAX_CIPHERS       4
#define NETXMS_RSA_KEYLEN        2048

#ifndef LLONG_MAX
#define LLONG_MAX    _LL(9223372036854775807)
#endif

#ifndef LLONG_MIN
#define LLONG_MIN    (-LLONG_MAX - 1)
#endif

#ifndef ULLONG_MAX
#define ULLONG_MAX   _ULL(18446744073709551615)
#endif


//
// Platform dependent includes and defines
//

#if defined(_WIN32) || defined(UNDER_CE)

/********** WINDOWS ********************/

#define FS_PATH_SEPARATOR  _T("\\")

#include <winsock2.h>
#include <windows.h>
#include <stdlib.h>

#ifndef UNDER_CE
#include <sys/stat.h>
#include <process.h>
#include <io.h>
#include <fcntl.h>
#endif

#ifndef S_IRUSR
# define S_IRUSR      0400
#endif
#ifndef S_IWUSR
# define S_IWUSR      0200
#endif

#define snprintf  _snprintf

typedef unsigned __int64 QWORD;
typedef __int64 INT64;
typedef int socklen_t;

#define INT64_FMT    _T("%I64d")
#define UINT64_FMT   _T("%I64u")
#ifdef __64BIT__
#define TIME_T_FMT   _T("%I64u")
#else
#define TIME_T_FMT   _T("%u")
#endif

// Socket compatibility
#define SHUT_RD      0
#define SHUT_WR      1
#define SHUT_RDWR    2

#define SetSocketReuseFlag(sd)
#define SELECT_NFDS(x)  ((int)(x))

#ifdef UNDER_CE
#define O_RDONLY     0x0004
#define O_WRONLY     0x0001
#define O_RDWR       0x0002
#define O_CREAT      0x0100
#define O_EXCL       0x0200
#define O_TRUNC      0x0800
#endif

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
#include <netdb.h>

typedef int BOOL;
#if (SIZEOF_LONG == 4)
typedef long LONG;
#else
typedef int LONG;
#endif
#if (SIZEOF_LONG == 4)
typedef unsigned long DWORD;
#else
typedef unsigned int DWORD;
#endif
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

#define INT64_FMT    _T("%lld")
#define UINT64_FMT   _T("%llu")
#define TIME_T_FMT   _T("%u")

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

// Socket compatibility
typedef int SOCKET;

#define closesocket(x) close(x)
#define WSAGetLastError() (errno)

#define WSAEINTR        EINTR
#define INVALID_SOCKET  (-1)

//#define SetSocketReuseFlag(sd)
#define SetSocketReuseFlag(sd) { \
	int nVal = 1; \
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (const void *)&nVal,  \
			(socklen_t)sizeof(nVal)); \
}

#define SELECT_NFDS(x)  (x)

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
#if (SIZEOF_LONG == 4)
typedef long LONG;
typedef unsigned long DWORD;
#undef __64BIT__
#else
typedef int LONG;
typedef unsigned int DWORD;
#ifndef __64BIT__
#define __64BIT__
#endif
#endif
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

#define INT64_FMT    _T("%lld")
#define UINT64_FMT   _T("%llu")
#ifdef __64BIT__
#define TIME_T_FMT   _T("%llu")
#else
#define TIME_T_FMT   _T("%u")
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

#define WSAEINTR        EINTR
#define INVALID_SOCKET  (-1)

#define SetSocketReuseFlag(sd) { \
	int nVal = 1; \
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (const void *)&nVal,  \
			(socklen_t)sizeof(nVal)); \
}

#define SELECT_NFDS(x)  (x)

#if !(HAVE_SOCKLEN_T)
typedef unsigned int socklen_t;
#endif

#endif   /* _WIN32 */


//
// Casting between pointer and 32-bit integer
//

#ifdef __64BIT__
#define CAST_FROM_POINTER(p, t) ((t)((QWORD)p))
#define CAST_TO_POINTER(v, t) ((t)((QWORD)v))
#else
#define CAST_FROM_POINTER(p, t) ((t)p)
#define CAST_TO_POINTER(v, t) ((t)v)
#endif


//
// OpenSSL
//

#ifdef _WITH_ENCRYPTION

#include <openssl/crypto.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/opensslv.h>

#ifdef NETXMS_NO_AES
#ifndef OPENSSL_NO_AES
#define OPENSSL_NO_AES
#endif
#endif

#ifdef NETXMS_NO_BF
#ifndef OPENSSL_NO_BF
#define OPENSSL_NO_BF
#endif
#endif

#ifdef NETXMS_NO_IDEA
#ifndef OPENSSL_NO_IDEA
#define OPENSSL_NO_IDEA
#endif
#endif

#ifdef NETXMS_NO_DES
#ifndef OPENSSL_NO_DES
#define OPENSSL_NO_DES
#endif
#endif

#if OPENSSL_VERSION_NUMBER >= 0x00907000
#define OPENSSL_CONST const
#else
#define OPENSSL_CONST
#endif

#else

// Prevent compilation errors on function prototypes
#define RSA void

#endif


//
// open() flags compatibility
//

#ifndef O_BINARY
#define O_BINARY 0
#endif


//
// Windows-specific structures for non-Windows platforms
// 

#ifndef _WIN32

typedef struct tagPOINT
{
   int x;
   int y;
} POINT;

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
	char m_cData[4];		// Data
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
#define safe_free_and_null(x) { if ((x) != NULL) { free(x); x = NULL; } }


//
// delete object and nullify pointer
//

#define delete_and_null(x) { delete x; x = NULL; }


//
// Convert half-byte's value to hex digit and vice versa
//

#define bin2hex(x) ((x) < 10 ? ((x) + _T('0')) : ((x) + (_T('A') - 10)))
#ifdef UNICODE
#define hex2bin(x) ((((x) >= _T('0')) && ((x) <= _T('9'))) ? ((x) - _T('0')) : \
                    (((towupper(x) >= _T('A')) && (towupper(x) <= _T('F'))) ? (towupper(x) - _T('A') + 10) : 0))
#else
#define hex2bin(x) ((((x) >= '0') && ((x) <= '9')) ? ((x) - '0') : \
                    (((toupper(x) >= 'A') && (toupper(x) <= 'F')) ? (toupper(x) - 'A' + 10) : 0))
#endif


//
// Define min() and max() if needed
//

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
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
// Validate numerical value
//

#define VALIDATE_VALUE(var,x,y,z) { if ((var < x) || (var > y)) var = z; }


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


//
// Insert parameter as string
//

#ifdef UNICODE
#define STRING(x)   L#x
#else
#define STRING(x)   #x
#endif


//
// Memory debug
//

#ifdef NETXMS_MEMORY_DEBUG												

#ifdef __cplusplus												
extern "C" {												  
#endif

void *nx_malloc(size_t, char *, int);
void *nx_realloc(void *, size_t, char *, int);
void nx_free(void *, char *, int);

void InitMemoryDebugger(void);
void PrintMemoryBlocks(void);

#ifdef __cplusplus												
}
#endif

#define malloc(x) nx_malloc(x, __FILE__, __LINE__)
#define realloc(p, x) nx_realloc(p, x, __FILE__,  __LINE__)
#define free(p) nx_free(p, __FILE__, __LINE__)											  

#endif	/* NETXMS_MEMORY_DEBUG */

#endif   /* _nms_common_h_ */
