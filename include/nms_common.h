/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
** File: nms_common.h
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

#include <config.h>
#if defined(WITH_OPENSSL)
#define _WITH_ENCRYPTION   1
#endif

#else    /* _WIN32 */

#define _WITH_ENCRYPTION   1
#define WITH_OPENSSL       1
#define WITH_LDAP          1
#define VA_LIST_IS_POINTER 1

#if !defined(WINDOWS_ONLY) && !defined(_CRT_SECURE_NO_DEPRECATE)
#define _CRT_SECURE_NO_DEPRECATE
#endif

// Turn off C++ exceptions use in standard header files
#ifndef _HAS_EXCEPTIONS
#define _HAS_EXCEPTIONS    0
#endif

// prevent defining min and max macros in Windows headers
#ifndef NOMINMAX
#define NOMINMAX
#endif

// prevent defining ETIMEDOUT, ECONNRESET, etc. to wrong values
#ifndef ALLOW_CRT_POSIX_ERROR_CODES
#define _CRT_NO_POSIX_ERROR_CODES
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#endif   /* _WIN32 */

#if HAVE_JEMALLOC_JEMALLOC_H
#include <jemalloc/jemalloc.h>
#endif

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

#include <unicode.h>

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include <netxms-version.h>

#if HAVE_WIDEC_H
#include <widec.h>
#endif /* HAVE_WIDEC */

#include <symbol_visibility.h>

/******* export attribute for libnetxms symbols *******/
#ifdef LIBNETXMS_EXPORTS
#define LIBNETXMS_EXPORTABLE __EXPORT
#else
#define LIBNETXMS_EXPORTABLE __IMPORT
#endif

/**
 * Define __64BIT__ if compiling for 64bit platform with Visual C++
 */
#if defined(_M_X64) || defined(_M_IA64) || defined(__LP64__) || defined(__PPC64__) || defined(__x86_64__)
#ifndef __64BIT__
#define __64BIT__
#endif
#endif

/**
 * Wrappers for 64-bit integer constants
 */
#if defined(__GNUC__) || defined(__HP_aCC) || defined(__IBMC__) || defined(__IBMCPP__) || defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#define _LL(x) (x ## LL)
#define _ULL(x) (x ## ULL)
#elif defined(_MSC_VER)
#define _LL(x) (x ## i64)
#define _ULL(x) (x ## ui64)
#else
#define _LL(x) (x)
#define _ULL(x) (x)
#endif

/**
 * Common constants
 */
#define MAX_SECRET_LENGTH        64
#define MAX_DB_STRING            256
#define MAX_PARAM_NAME           256
#define MAX_CONFIG_VALUE         2000
#define MAX_COLUMN_NAME          64
#define MAX_DNS_NAME             256
#define MAX_HELPDESK_REF_LEN     64
#define MAX_PASSWORD             256
#define MAX_SSH_LOGIN_LEN        64
#define MAX_SSH_PASSWORD_LEN     64
#define GROUP_FLAG               ((UINT32)0x80000000)

#define NETXMS_MAX_CIPHERS       6
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

#ifndef EVENTLOG_DEBUG_TYPE
#define EVENTLOG_DEBUG_TYPE		0x0080
#endif

#define INVALID_INDEX         0xFFFFFFFF
#define MD5_DIGEST_SIZE       16
#define SHA1_DIGEST_SIZE      20
#define SHA256_DIGEST_SIZE    32

#define FILE_BUFFER_SIZE      ((size_t)32768)

/**
 * Compatibility defines for C sources
 */
#if !defined(__cplusplus) && !defined(CORTEX) && !HAVE_BOOL
typedef int bool;
#endif

/**
 * Oracle Pro*C compatibility
 */
#ifdef ORA_PROC
#undef BYTE
#undef DWORD
#endif

/**
 * Java class path separator character
 */
#ifdef _WIN32
#define JAVA_CLASSPATH_SEPARATOR   _T(';')
#else
#define JAVA_CLASSPATH_SEPARATOR   _T(':')
#endif


/***** Platform dependent includes and defines *****/

#if defined(_WIN32) || defined(UNDER_CE)

/********** WINDOWS ********************/

#ifdef _MSC_FULL_VER
#define __BUILD_VERSION_STRING(s,v) s v
#define __STR_NX(x) #x
#define __STR(x) __STR_NX(x)
#define CPP_COMPILER_VERSION __STR(__BUILD_VERSION_STRING(Microsoft C/C++ Optimizing Compiler Version,_MSC_FULL_VER))
#endif

#define _Noreturn __declspec(noreturn)

#define CAN_DELETE_COPY_CTOR      1
#define HAVE_OVERRIDE_SPECIFIER   1
#define HAVE_THREAD_LOCAL_STORAGE 1

// Disable some warnings:
//   4577 -  'noexcept' used with no exception handling mode specified
#pragma warning(disable: 4577)

// Set API version to Windows 7
#ifndef _WIN32_WINNT
#define _WIN32_WINNT		0x0601
#endif

/**
 * Epoch offset for FILETIME structure
 */
#define EPOCHFILETIME           (116444736000000000i64)

#define WITH_IPV6               1
#define WITH_PYTHON             1

#define USE_BUNDLED_LIBTRE      1
#define USE_BUNDLED_GETOPT      1

#define SAFE_FGETWS_WITH_POPEN  1

#define FREE_IS_NULL_SAFE       1

#define FS_PATH_SEPARATOR           _T("\\")
#define FS_PATH_SEPARATOR_A         "\\"
#define FS_PATH_SEPARATOR_W         L"\\"
#define FS_PATH_SEPARATOR_CHAR      _T('\\')
#define FS_PATH_SEPARATOR_CHAR_A    '\\'
#define FS_PATH_SEPARATOR_CHAR_W    L'\\'

#define WEXITSTATUS(x)          (x)

#if _MSC_VER >= 1310
#define HAVE_SCPRINTF           1
#define HAVE_VSCPRINTF          1
#define HAVE_SCWPRINTF          1
#define HAVE_VSCWPRINTF         1
#endif

#if _MSC_VER >= 1800
#define HAVE_STRTOLL            1
#define HAVE_STRTOULL           1
#define HAVE_WCSTOLL            1
#define HAVE_WCSTOULL           1
#endif

#define HAVE_TOUPPER            1
#define HAVE_TOWUPPER           1
#define HAVE_TOLOWER            1
#define HAVE_TOWLOWER           1

#define HAVE_SNPRINTF           1
#define HAVE_VSNPRINTF          1

#define HAVE_GETADDRINFO        1

#define HAVE_STDARG_H           1

#define HAVE_ALLOCA             1

#define HAVE_WCSLEN             1
#define HAVE_WCSNCPY            1
#define HAVE_WCSDUP             1
#define HAVE_WUTIME             1

#define HAVE_LIBCURL            1

#define HAVE_DIRENT_D_TYPE      1

#define HAVE_MOSQUITTO_THREADED_SET	1

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <malloc.h>

#include <stdarg.h>
#ifndef va_copy
#define va_copy(x,y)            (x = y)
#endif
#define HAVE_DECL_VA_COPY       1

#ifndef UNDER_CE
#include <sys/stat.h>
#include <sys/utime.h>
#include <process.h>
#include <io.h>
#include <fcntl.h>
#include <direct.h>
#endif

#ifndef S_IRUSR
#define S_IRUSR      _S_IREAD
#endif
#ifndef S_IWUSR
#define S_IWUSR      _S_IWRITE
#endif
#ifndef S_IRGRP
#define S_IRGRP      _S_IREAD
#endif
#ifndef S_IWGRP
#define S_IWGRP      _S_IWRITE
#endif
#ifndef S_IROTH
#define S_IROTH      _S_IREAD
#endif
#ifndef S_IWOTH
#define S_IWOTH      _S_IWRITE
#endif

#define STDIN_FILENO    _fileno(stdin)
#define STDOUT_FILENO   _fileno(stdout)
#define STDERR_FILENO   _fileno(stderr)

#define snprintf     _snprintf
#define vsnprintf    _vsnprintf
#define snwprintf    _snwprintf
#define vsnwprintf   _vsnwprintf
#define scprintf     _scprintf
#define vscprintf    _vscprintf
#define scwprintf    _scwprintf
#define vscwprintf   _vscwprintf
#define stricmp      _stricmp
#define strnicmp     _strnicmp
#define wcsicmp      _wcsicmp
#define wcsnicmp     _wcsnicmp
#define strlwr(s)    _strlwr(s)
#define strupr(s)    _strupr(s)

typedef UINT64 QWORD;   // for compatibility
typedef int socklen_t;
typedef long pid_t;

#ifdef _WIN64
typedef __int64 ssize_t;
#else
typedef int ssize_t;
#endif

typedef signed __int8 int8_t;
typedef signed __int16 int16_t;
typedef signed __int32 int32_t;
typedef signed __int64 int64_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;

#define INT64_FMT			_T("%I64d")
#define INT64_FMTA		"%I64d"
#define INT64_FMTW		L"%I64d"
#define UINT64_FMT		_T("%I64u")
#define UINT64_FMTA		"%I64u"
#define UINT64_FMTW		L"%I64u"
#define UINT64X_FMT(m)  _T("%") m _T("I64X")
#if defined(__64BIT__) || (_MSC_VER > 1300)
#define TIME_T_FMT      _T("%I64u")
#define TIME_T_FCAST(x) ((UINT64)(x))
#else
#define TIME_T_FMT      _T("%u")
#define TIME_T_FCAST(x) ((UINT32)(x))
#endif

#ifndef __clang__

#define HAVE_DECL_BSWAP_16 1
#define HAVE_DECL_BSWAP_32 1
#define HAVE_DECL_BSWAP_64 1

#define bswap_16(n)  _byteswap_ushort(n)
#define bswap_32(n)  _byteswap_ulong(n)
#define bswap_64(n)  _byteswap_uint64(n)

#endif

#define HAVE_LIBEXPAT  1
#define XMPP_SUPPORTED 1
#define HAVE_LOCALE_H  1
#define HAVE_SETLOCALE 1

#else    /* not _WIN32 */

/*********** UNIX *********************/

#ifndef PREFIX
#ifdef UNICODE
#define PREFIX  L"/usr/local"
#else
#define PREFIX  "/usr/local"
#endif
#warning Installation prefix not defined, defaulting to /usr/local
#endif

#ifndef SYSCONFDIR
#ifdef UNICODE
#define SYSCONFDIR   PREFIX L"/etc"
#else
#define SYSCONFDIR   PREFIX "/etc"
#endif
#warning SYSCONFDIR not defined, defaulting to $prefix/etc
#endif

#if HAVE_THREAD_LOCAL_SPECIFIER
#define HAVE_THREAD_LOCAL_STORAGE 1
#elif HAVE_THREAD_SPECIFIER
#define thread_local __thread
#define HAVE_THREAD_LOCAL_STORAGE 1
#else
#define thread_local
#define HAVE_THREAD_LOCAL_STORAGE 0
#endif

#if HAVE_STDBOOL_H
#include <stdbool.h>
#endif

#if HAVE_ALLOCA_H
#include <alloca.h>
#endif

#if HAVE_WCHAR_H
#include <wchar.h>
#endif

#if HAVE_WCTYPE_H
#include <wctype.h>
#endif

// Fix for wcs* functions visibility in Solaris 11
#if defined(__sun) && (__cplusplus >= 199711L)
#if HAVE_WCSDUP
using std::wcsdup;
#endif
#if HAVE_WCSCASECMP
using std::wcscasecmp;
#endif
#if HAVE_WCSNCASECMP
using std::wcsncasecmp;
#endif
#endif

#include <errno.h>

#define FS_PATH_SEPARATOR           _T("/")
#define FS_PATH_SEPARATOR_A         "/"
#define FS_PATH_SEPARATOR_W         L"/"
#define FS_PATH_SEPARATOR_CHAR      _T('/')
#define FS_PATH_SEPARATOR_CHAR_A    '/'
#define FS_PATH_SEPARATOR_CHAR_W    L'/'

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_UTIME_H
#include <utime.h>
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

#if HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

#if HAVE_NET_NH_H
#include <net/nh.h>
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

#if _USE_GNU_PTH
#include <pth.h>
#else
#include <pthread.h>
#endif

#ifdef __IBMCPP__
#include <builtins.h>
#endif

#ifdef __minix
#undef HAVE_ITOA  /* Minix has non-compatible itoa() */
#undef HAVE_GETHOSTBYNAME2_R
#endif

#include <sys/socket.h>
#include <sys/un.h>

typedef int BOOL;
#if (SIZEOF_LONG == 4)
#if (SIZEOF_INT == 4)
typedef int INT32;
typedef unsigned int UINT32;
#else
typedef long INT32;
typedef unsigned long UINT32;
#endif
#undef __64BIT__
#else
typedef int INT32;
typedef unsigned int UINT32;
#ifndef __64BIT__
#define __64BIT__
#endif
#endif
typedef unsigned short UINT16;
typedef short INT16;
typedef unsigned char BYTE;
typedef INT32 LONG;  // for compatibility
typedef UINT32 DWORD;  // for compatibility
typedef UINT16 WORD;  // for compatibility
typedef void * HANDLE;
typedef void * HMODULE;

#if !HAVE_MODE_T
typedef int mode_t;
#endif

// We have to use long as INT64 on HP-UX - otherwise
// there will be compilation errors because of type redefinition in
// system includes
#ifdef _HPUX

#ifdef __LP64__
typedef long INT64;
typedef unsigned long UINT64;
#else
typedef long long INT64;
typedef unsigned long long UINT64;
#endif

#else    /* _HPUX */

#if HAVE_LONG_LONG && (SIZEOF_LONG_LONG == 8)
typedef long long INT64;
#elif HAVE_INT64_T
typedef int64_t INT64;
#elif SIZEOF_LONG == 8
typedef long INT64;
#else
#error Target system does not have signed 64bit integer type
#endif

#if HAVE_UNSIGNED_LONG_LONG && (SIZEOF_LONG_LONG == 8)
typedef unsigned long long UINT64;
#elif HAVE_UINT64_T
typedef uint64_t UINT64;
#elif HAVE_U_INT64_T
typedef u_int64_t UINT64;
#elif SIZEOF_LONG == 8
typedef unsigned long UINT64;
#else
#error Target system does not have unsigned 64bit integer type
#endif

#endif   /* _HPUX */

typedef UINT64 QWORD;   // for compatibility

#define INT64_FMT			_T("%lld")
#define INT64_FMTW		L"%lld"
#define INT64_FMTA		"%lld"
#define UINT64_FMT		_T("%llu")
#define UINT64_FMTW		L"%llu"
#define UINT64_FMTA		"%llu"
#define UINT64X_FMT(m)  _T("%") m _T("llX")
#ifdef __64BIT__
#define TIME_T_FMT		_T("%llu")
#define TIME_T_FCAST(x)         ((UINT64)(x))
#else
#define TIME_T_FMT		_T("%u")
#define TIME_T_FCAST(x)         ((UINT32)(x))
#endif

#ifndef TRUE
#define TRUE   1
#endif
#ifndef FALSE
#define FALSE  0
#endif

// on AIX open_memstream only declared if _XOPEN_SOURCE >= 700
// which is incompatible with other parts of code
#if defined(__IBMC__) || defined(__IBMCPP__)
#ifdef __cplusplus
extern "C"
#endif
FILE *open_memstream(char **, size_t *);
#endif

// Some systems may define true and false which may break overloaded functions
#ifdef __cplusplus
#undef true
#undef false
#endif

#ifndef MAX_PATH
#ifdef PATH_MAX
#define MAX_PATH PATH_MAX
#else
#define MAX_PATH 1024
#endif
#endif

// Windows compatibility layer for standard C I/O functions
#ifdef __cplusplus

inline int _access(const char *pathname, int mode) { return ::access(pathname, mode); }
inline int _chdir(const char *path) { return ::chdir(path);  }
inline int _close(int fd) { return ::close(fd); }
inline int _fileno(FILE *stream) { return fileno(stream); }
inline int _isatty(int fd) { return ::isatty(fd); }
inline off_t _lseek(int fd, off_t offset, int whence) { return ::lseek(fd, offset, whence);  }
inline int _mkdir(const char *pathname, mode_t mode) { return ::mkdir(pathname, mode); }
inline int _open(const char *pathname, int flags) { return ::open(pathname, flags); }
inline int _open(const char *pathname, int flags, mode_t mode) { return ::open(pathname, flags, mode); }
inline int _pclose(FILE *stream) { return ::pclose(stream); }
inline FILE *_popen(const char *command, const char *type) { return ::popen(command, type); }
inline int _putenv(char *string) { return ::putenv(string); }
inline ssize_t _read(int fd, void *buf, size_t count) { return ::read(fd, buf, count); }
inline int _remove(const char *pathname) { return ::remove(pathname); }
inline int _unlink(const char *pathname) { return ::unlink(pathname); }
inline ssize_t _write(int fd, const void *buf, size_t count) { return ::write(fd, buf, count); }

#else

#define _access(p, m)      access((p), (m))
#define _chdir(p)          chdir(p)
#define _close(f)          close(f)
#define _fileno(f)         fileno(f)
#define _isatty(f)         isatty(f)
#define _lseek(f, o, w)    lseek((f), (o) ,(w))
#define _mkdir(p, m)       mkdir((p), (m))
#define _open              open
#define _pclose(f)         pclose(f)
#define _popen(c, m)       popen((c), (m))
#define _putenv(s)         putenv(s)
#define _read(f, b, l)     read((f), (b), (l))
#define _remove(f)         remove(f)
#define _unlink(f)         unlink(f)
#define _write(f, b, l)    write((f), (b), (l))

#endif

// Shared library suffix
#ifndef SHLIB_SUFFIX
#if defined(_HPUX) && !defined(__64BIT__)
#define SHLIB_SUFFIX	_T(".sl")
#else
#define SHLIB_SUFFIX	_T(".so")
#endif
#endif

typedef struct hostent HOSTENT;

#if HAVE_DECL___VA_COPY && !HAVE_DECL_VA_COPY
#define HAVE_DECL_VA_COPY 1
#define va_copy(d,s) __va_copy(d,s)
#endif

#endif   /* _WIN32 */

/**
 * Value used to indicate invalid pointer where NULL is not appropriate
 */
#ifdef __64BIT__
#define INVALID_POINTER_VALUE    ((void *)_ULL(0xFFFFFFFFFFFFFFFF))
#else
#define INVALID_POINTER_VALUE    ((void *)0xFFFFFFFF)
#endif

/**
 * Casting between pointer and 32-bit integer
 */
#ifdef __64BIT__
#define CAST_FROM_POINTER(p, t) ((t)((UINT64)(p)))
#define CAST_TO_POINTER(v, t) ((t)((UINT64)(v)))
#else
#define CAST_FROM_POINTER(p, t) ((t)((UINT32)(p)))
#define CAST_TO_POINTER(v, t) ((t)((UINT32)(v)))
#endif

/**
 * open() flags compatibility
 */
#ifndef O_BINARY
#define O_BINARY 0
#endif

/**
 * Windows-specific structures for non-Windows platforms
 */
#ifndef _WIN32

typedef struct tagPOINT
{
   int x;
   int y;
} POINT;

#endif

/**
 * Event log severity codes
 */
#ifndef _WIN32
#define EVENTLOG_SUCCESS                0x0000
#define EVENTLOG_ERROR_TYPE             0x0001
#define EVENTLOG_WARNING_TYPE           0x0002
#define EVENTLOG_INFORMATION_TYPE       0x0004
#define EVENTLOG_AUDIT_SUCCESS          0x0008
#define EVENTLOG_AUDIT_FAILURE          0x0010
#endif   /* _WIN32 */

#define NXLOG_DEBUG     EVENTLOG_DEBUG_TYPE
#define NXLOG_INFO      EVENTLOG_INFORMATION_TYPE
#define NXLOG_WARNING   EVENTLOG_WARNING_TYPE
#define NXLOG_ERROR     EVENTLOG_ERROR_TYPE

/**
 * Interface types
 */
#define IFTYPE_OTHER                      1
#define IFTYPE_REGULAR1822                2
#define IFTYPE_HDH1822                    3
#define IFTYPE_DDN_X25                    4
#define IFTYPE_RFC877_X25                 5
#define IFTYPE_ETHERNET_CSMACD            6
#define IFTYPE_ISO88023_CSMACD            7
#define IFTYPE_ISO88024_TOKENBUS          8
#define IFTYPE_ISO88025_TOKENRING         9
#define IFTYPE_ISO88026_MAN               10
#define IFTYPE_STARLAN                    11
#define IFTYPE_PROTEON_10MBIT             12
#define IFTYPE_PROTEON_80MBIT             13
#define IFTYPE_HYPERCHANNEL               14
#define IFTYPE_FDDI                       15
#define IFTYPE_LAPB                       16
#define IFTYPE_SDLC                       17
#define IFTYPE_DS1                        18
#define IFTYPE_E1                         19
#define IFTYPE_BASIC_ISDN                 20
#define IFTYPE_PRIMARY_ISDN               21
#define IFTYPE_PROP_PTP_SERIAL            22
#define IFTYPE_PPP                        23
#define IFTYPE_SOFTWARE_LOOPBACK          24
#define IFTYPE_EON                        25
#define IFTYPE_ETHERNET_3MBIT             26
#define IFTYPE_NSIP                       27
#define IFTYPE_SLIP                       28
#define IFTYPE_ULTRA                      29
#define IFTYPE_DS3                        30
#define IFTYPE_SMDS                       31
#define IFTYPE_FRAME_RELAY                32
#define IFTYPE_RS232                      33
#define IFTYPE_PARA                       34
#define IFTYPE_ARCNET                     35
#define IFTYPE_ARCNET_PLUS                36
#define IFTYPE_ATM                        37
#define IFTYPE_MIOX25                     38
#define IFTYPE_SONET                      39
#define IFTYPE_X25PLE                     40
#define IFTYPE_ISO88022LLC                41
#define IFTYPE_LOCALTALK                  42
#define IFTYPE_SMDS_DXI                   43
#define IFTYPE_FRAME_RELAY_SERVICE        44
#define IFTYPE_V35                        45
#define IFTYPE_HSSI                       46
#define IFTYPE_HIPPI                      47
#define IFTYPE_MODEM                      48
#define IFTYPE_AAL5                       49
#define IFTYPE_SONET_PATH                 50
#define IFTYPE_SONET_VT                   51
#define IFTYPE_SMDS_ICIP                  52
#define IFTYPE_PROP_VIRTUAL               53
#define IFTYPE_PROP_MULTIPLEXOR           54
#define IFTYPE_IEEE80212                  55
#define IFTYPE_FIBRECHANNEL               56
#define IFTYPE_HIPPIINTERFACE             57
#define IFTYPE_FRAME_RELAY_INTERCONNECT   58
#define IFTYPE_AFLANE8023                 59
#define IFTYPE_AFLANE8025                 60
#define IFTYPE_CCTEMUL                    61
#define IFTYPE_FAST_ETHERNET              62
#define IFTYPE_ISDN                       63
#define IFTYPE_V11                        64
#define IFTYPE_V36                        65
#define IFTYPE_G703_AT64K                 66
#define IFTYPE_G703_AT2MB                 67
#define IFTYPE_QLLC                       68
#define IFTYPE_FASTETHERFX                69
#define IFTYPE_CHANNEL                    70
#define IFTYPE_IEEE80211                  71
#define IFTYPE_IBM370_PARCHAN             72
#define IFTYPE_ESCON                      73
#define IFTYPE_DLSW                       74
#define IFTYPE_ISDNS                      75
#define IFTYPE_ISDNU                      76
#define IFTYPE_LAPD                       77
#define IFTYPE_IPSWITCH                   78
#define IFTYPE_RSRB                       79
#define IFTYPE_ATMLOGICAL                 80
#define IFTYPE_DS0                        81
#define IFTYPE_DS0_BUNDLE                 82
#define IFTYPE_BSC                        83
#define IFTYPE_ASYNC                      84
#define IFTYPE_CNR                        85
#define IFTYPE_ISO88025DTR                86
#define IFTYPE_EPLRS                      87
#define IFTYPE_ARAP                       88
#define IFTYPE_PROPCNLS                   89
#define IFTYPE_HOSTPAD                    90
#define IFTYPE_TERMPAD                    91
#define IFTYPE_FRAME_RELAY_MPI            92
#define IFTYPE_X213                       93
#define IFTYPE_ADSL                       94
#define IFTYPE_RADSL                      95
#define IFTYPE_SDSL                       96
#define IFTYPE_VDSL                       97
#define IFTYPE_ISO88025CRFPINT            98
#define IFTYPE_MYRINET                    99
#define IFTYPE_VOICEEM                    100
#define IFTYPE_VOICEFXO                   101
#define IFTYPE_VOICEFXS                   102
#define IFTYPE_VOICEENCAP                 103
#define IFTYPE_VOICEOVERIP                104
#define IFTYPE_ATMDXI                     105
#define IFTYPE_ATMFUNI                    106
#define IFTYPE_ATMIMA                     107
#define IFTYPE_PPPMULTILINKBUNDLE         108
#define IFTYPE_IPOVERCDLC                 109
#define IFTYPE_IPOVERCLAW                 110
#define IFTYPE_STACKTOSTACK               111
#define IFTYPE_VIRTUAL_IP_ADDRESS         112
#define IFTYPE_MPC                        113
#define IFTYPE_IPOVERATM                  114
#define IFTYPE_ISO88025FIBER              115
#define IFTYPE_TDLC                       116
#define IFTYPE_GIGABIT_ETHERNET           117
#define IFTYPE_HDLC                       118
#define IFTYPE_LAPF                       119
#define IFTYPE_V37                        120
#define IFTYPE_X25MLP                     121
#define IFTYPE_X25_HUNT_GROUP             122
#define IFTYPE_TRANSPHDLC                 123
#define IFTYPE_INTERLEAVE                 124
#define IFTYPE_FAST                       125
#define IFTYPE_IP                         126
#define IFTYPE_DOCSCABLE_MACLAYER         127
#define IFTYPE_DOCSCABLE_DOWNSTREAM       128
#define IFTYPE_DOCSCABLE_UPSTREAM         129
#define IFTYPE_A12MPPSWITCH               130
#define IFTYPE_TUNNEL                     131
#define IFTYPE_COFFEE                     132
#define IFTYPE_CES                        133
#define IFTYPE_ATM_SUBINTERFACE           134
#define IFTYPE_L2VLAN                     135
#define IFTYPE_L3IPVLAN                   136
#define IFTYPE_L3IPXVLAN                  137
#define IFTYPE_DIGITAL_POWERLINE          138
#define IFTYPE_MEDIAMAIL_OVER_IP          139
#define IFTYPE_DTM                        140
#define IFTYPE_DCN                        141
#define IFTYPE_IPFORWARD                  142
#define IFTYPE_MSDSL                      143
#define IFTYPE_IEEE1394                   144
#define IFTYPE_GSN                        145
#define IFTYPE_DVBRCC_MACLAYER            146
#define IFTYPE_DVBRCC_DOWNSTREAM          147
#define IFTYPE_DVBRCC_UPSTREAM            148
#define IFTYPE_ATM_VIRTUAL                149
#define IFTYPE_MPLS_TUNNEL                150
#define IFTYPE_SRP                        151
#define IFTYPE_VOICE_OVER_ATM             152
#define IFTYPE_VOICE_OVER_FRAME_RELAY     153
#define IFTYPE_IDSL                       154
#define IFTYPE_COMPOSITE_LINK             155
#define IFTYPE_SS7_SIGLINK                156
#define IFTYPE_PROPWIRELESSP2P            157
#define IFTYPE_FRFORWARD                  158
#define IFTYPE_RFC1483                    159
#define IFTYPE_USB                        160
#define IFTYPE_IEEE8023ADLAG              161
#define IFTYPE_BGP_POLICY_ACCOUNTING      162
#define IFTYPE_FRF16MFR_BUNDLE            163
#define IFTYPE_H323_GATEKEEPER            164
#define IFTYPE_H323_PROXY                 165
#define IFTYPE_MPLS                       166
#define IFTYPE_MFSIGLINK                  167
#define IFTYPE_HDSL2                      168
#define IFTYPE_SHDSL                      169
#define IFTYPE_DS1FDL                     170
#define IFTYPE_POS                        171
#define IFTYPE_DVBASI_IN                  172
#define IFTYPE_DVBASI_OUT                 173
#define IFTYPE_PLC                        174
#define IFTYPE_NFAS                       175
#define IFTYPE_TR008                      176
#define IFTYPE_GR303RDT                   177
#define IFTYPE_GR303IDT                   178
#define IFTYPE_ISUP                       179
#define IFTYPE_PROPDOCSWIRELESSMACLAYER   180
#define IFTYPE_PROPDOCSWIRELESSDOWNSTREAM 181
#define IFTYPE_PROPDOCSWIRELESSUPSTREAM   182
#define IFTYPE_HIPERLAN2                  183
#define IFTYPE_PROPBWAP2MP                184
#define IFTYPE_SONET_OVERHEAD_CHANNEL     185
#define IFTYPE_DW_OVERHEAD_CHANNEL        186
#define IFTYPE_AAL2                       187
#define IFTYPE_RADIOMAC                   188
#define IFTYPE_ATMRADIO                   189
#define IFTYPE_IMT                        190
#define IFTYPE_MVL                        191
#define IFTYPE_REACHDSL                   192
#define IFTYPE_FRDLCIENDPT                193
#define IFTYPE_ATMVCIENDPT                194
#define IFTYPE_OPTICAL_CHANNEL            195
#define IFTYPE_OPTICAL_TRANSPORT          196
#define IFTYPE_PROPATM                    197
#define IFTYPE_VOICE_OVER_CABLE           198
#define IFTYPE_INFINIBAND                 199
#define IFTYPE_TELINK                     200
#define IFTYPE_Q2931                      201
#define IFTYPE_VIRTUALTG                  202
#define IFTYPE_SIPTG                      203
#define IFTYPE_SIPSIG                     204
#define IFTYPE_DOCSCABLEUPSTREAMCHANNEL   205
#define IFTYPE_ECONET                     206
#define IFTYPE_PON155                     207
#define IFTYPE_PON622                     208
#define IFTYPE_BRIDGE                     209
#define IFTYPE_LINEGROUP                  210
#define IFTYPE_VOICEEMFGD                 211
#define IFTYPE_VOICEFGDEANA               212
#define IFTYPE_VOICEDID                   213
#define IFTYPE_MPEG_TRANSPORT             214
#define IFTYPE_SIXTOFOUR                  215
#define IFTYPE_GTP                        216
#define IFTYPE_PDNETHERLOOP1              217
#define IFTYPE_PDNETHERLOOP2              218
#define IFTYPE_OPTICAL_CHANNEL_GROUP      219
#define IFTYPE_HOMEPNA                    220
#define IFTYPE_GFP                        221
#define IFTYPE_CISCO_ISL_VLAN             222
#define IFTYPE_ACTELIS_METALOOP           223
#define IFTYPE_FCIPLINK                   224
#define IFTYPE_RPR                        225
#define IFTYPE_QAM                        226
#define IFTYPE_LMP                        227
#define IFTYPE_CBLVECTASTAR               228
#define IFTYPE_DOCSCABLEMCMTSDOWNSTREAM   229
#define IFTYPE_ADSL2                      230
#define IFTYPE_MACSECCONTROLLEDIF         231
#define IFTYPE_MACSECUNCONTROLLEDIF       232
#define IFTYPE_AVICIOPTICALETHER          233
#define IFTYPE_ATM_BOND                   234
#define IFTYPE_VOICEFGDOS                 235
#define IFTYPE_MOCA_VERSION1              236
#define IFTYPE_IEEE80216WMAN              237
#define IFTYPE_ADSL2PLUS                  238
#define IFTYPE_DVBRCSMACLAYER             239
#define IFTYPE_DVBTDM                     240
#define IFTYPE_DVBRCSTDMA                 241
#define IFTYPE_X86LAPS                    242
#define IFTYPE_WWANPP                     243
#define IFTYPE_WWANPP2                    244
#define IFTYPE_VOICEEBS                   245
#define IFTYPE_IFPWTYPE                   246
#define IFTYPE_ILAN                       247
#define IFTYPE_PIP                        248
#define IFTYPE_ALUELP                     249
#define IFTYPE_GPON                       250
#define IFTYPE_VDSL2                      251
#define IFTYPE_CAPWAP_DOT11_PROFILE       252
#define IFTYPE_CAPWAP_DOT11_BSS           253
#define IFTYPE_CAPWAP_WTP_VIRTUAL_RADIO   254
#define IFTYPE_BITS                       255
#define IFTYPE_DOCSCABLEUPSTREAMRFPORT    256
#define IFTYPE_CABLEDOWNSTREAMRFPORT      257
#define IFTYPE_VMWARE_VIRTUAL_NIC         258
#define IFTYPE_IEEE802154                 259
#define IFTYPE_OTNODU                     260
#define IFTYPE_OTNOTU                     261
#define IFTYPE_IFVFITYPE                  262
#define IFTYPE_G9981                      263
#define IFTYPE_G9982                      264
#define IFTYPE_G9983                      265
#define IFTYPE_ALUEPON                    266
#define IFTYPE_ALUEPONONU                 267
#define IFTYPE_ALUEPONPHYSICALUNI         268
#define IFTYPE_ALUEPONLOGICALLINK         269
#define IFTYPE_ALUGPONONU                 270
#define IFTYPE_ALUGPONPHYSICALUNI         271
#define IFTYPE_VMWARE_NIC_TEAM            272

/**
 * IP Header -- RFC 791
 */
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

/**
 * ICMP Header - RFC 792
 */
typedef struct tagICMPHDR
{
	BYTE m_cType;			// Type
	BYTE m_cCode;			// Code
	WORD m_wChecksum;		// Checksum
	WORD m_wId;				// Identification
	WORD m_wSeq;			// Sequence
} ICMPHDR;

/**
 * INADDR_NONE can be undefined on some systems
 */
#ifndef INADDR_NONE
#define INADDR_NONE	(0xFFFFFFFF)
#endif

/**
 * Check if given string is NULL and always return valid pointer
 */
#ifdef __cplusplus
static inline const TCHAR *CHECK_NULL(const TCHAR *x) { return (x == NULL) ? _T("(null)") : x; }
static inline const char *CHECK_NULL_A(const char *x) { return (x == NULL) ? "(null)" : x; }
static inline const WCHAR *CHECK_NULL_W(const WCHAR *x) { return (x == NULL) ? L"(null)" : x; }
static inline const TCHAR *CHECK_NULL_EX(const TCHAR *x) { return (x == NULL) ? _T("") : x; }
static inline const char *CHECK_NULL_EX_A(const char *x) { return (x == NULL) ? "" : x; }
static inline const WCHAR *CHECK_NULL_EX_W(const WCHAR *x) { return (x == NULL) ? L"" : x; }
#else
#define CHECK_NULL(x)      ((x) == NULL ? _T("(null)") : (x))
#define CHECK_NULL_A(x)    ((x) == NULL ? "(null)" : (x))
#define CHECK_NULL_W(x)    ((x) == NULL ? L"(null)" : (x))
#define CHECK_NULL_EX(x)   ((x) == NULL ? _T("") : (x))
#define CHECK_NULL_EX_A(x) ((x) == NULL ? "" : (x))
#define CHECK_NULL_EX_W(x) ((x) == NULL ? L"" : (x))
#endif

/***** Common C++ includes *****/

#ifdef __cplusplus

#if defined(__IBMCPP_TR1__)

#include <memory>
using std::tr1::shared_ptr;

// xlC++ implementation C++11 TR1 does not have make_shared
template<typename T, typename... Args>
inline shared_ptr<T> make_shared(Args&&... args)
{
   return shared_ptr<T>(new T(args...));
}

#elif defined(__HP_aCC)

// HP aC++ does not have shared_ptr implementation, use bundled one
#include "nx_shared_ptr.h"

#elif !defined(__clang__) && defined(__GNUC__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 8))

// GCC implementation before 4.8 does not work with RTTI disabled, use bundled one
#include "nx_shared_ptr.h"

#else

#include <memory>
using std::shared_ptr;
using std::make_shared;

#endif

#endif	/* __cplusplus */


/**
 * Heap functions
 */
#ifdef __cplusplus

inline void *MemAlloc(size_t size) { return malloc(size); }
inline void *MemAllocZeroed(size_t size) { return calloc(size, 1); }
inline char *MemAllocStringA(size_t size) { return static_cast<char*>(MemAlloc(size)); }
inline WCHAR *MemAllocStringW(size_t size) { return static_cast<WCHAR*>(MemAlloc(size * sizeof(WCHAR))); }
template <typename T> T *MemAllocStruct() { return (T*)calloc(1, sizeof(T)); }
template <typename T> T *MemAllocArray(size_t count) { return (T*)calloc(count, sizeof(T)); }
template <typename T> T *MemAllocArrayNoInit(size_t count) { return (T*)MemAlloc(count * sizeof(T)); }
template <typename T> T *MemRealloc(T *p, size_t size) { return (T*)realloc(p, size); }
template <typename T> T *MemReallocArray(T *p, size_t count) { return (T*)realloc(p, count * sizeof(T)); }
#if FREE_IS_NULL_SAFE
inline void MemFree(void *p) { free(p); }
template <typename T> void MemFreeAndNull(T* &p) { free(p); p = NULL; }
#else
inline void MemFree(void *p) { if (p != NULL) free(p); }
template <typename T> void MemFreeAndNull(T* &p) { if (p != NULL) { free(p); p = NULL; } }
#endif

#else /* __cplusplus */

#define MemAlloc(size) malloc(size)
#define MemAllocZeroed(size) calloc(size, 1)
#define MemAllocStringA(size) MemAlloc(size)
#define MemAllocStringW(size) MemAlloc(size * sizeof(WCHAR))
#define MemAllocArray(count, size) calloc(count, size)
#define MemAllocArrayNoInit(count, size) MemAlloc((count) * (size))
#define MemRealloc(p, size) realloc(p, size)
#if FREE_IS_NULL_SAFE
#define MemFree(p) free(p);
#define MemFreeAndNull(p) do { free(p); p = NULL; } while(0)
#else
#define MemFree(p) do { if ((p) != NULL) free(p); } while(0)
#define MemFreeAndNull(p) do { if ((p) != NULL) { free(p); p = NULL; } } while(0)
#endif

#endif /* __cplusplus */

#ifdef UNICODE
#define MemAllocString MemAllocStringW
#else
#define MemAllocString MemAllocStringA
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBNETXMS_EXPORTABLE void *MemCopyBlock(const void *data, size_t size);

#ifdef __cplusplus
}

template<typename T> T *MemCopyBlock(const T *data, size_t size)
{
   return static_cast<T*>(MemCopyBlock(static_cast<const void*>(data), size));
}

template<typename T> T *MemCopyArray(const T *data, size_t count)
{
   return static_cast<T*>(MemCopyBlock(static_cast<const void*>(data), count * sizeof(T)));
}

#endif

#define nx_memdup MemCopyBlock

/******* C string copy functions *******/

#ifdef __cplusplus

inline char *MemCopyStringA(const char *src)
{
   return (src != NULL) ? MemCopyBlock(src, strlen(src) + 1) : NULL;
}

inline WCHAR *MemCopyStringW(const WCHAR *src)
{
   return (src != NULL) ? MemCopyBlock(src, (wcslen(src) + 1) * sizeof(WCHAR)) : NULL;
}

#else

#define MemCopyStringA(s) (((s) != NULL) ? static_cast<char*>(MemCopyBlock((s), strlen(s) + 1)) : NULL)
#define MemCopyStringW(s) (((s) != NULL) ? static_cast<WCHAR*>(MemCopyBlock((s), (wcslen(s) + 1) * sizeof(WCHAR))) : NULL)

#endif

#ifdef UNICODE
#define MemCopyString MemCopyStringW
#else
#define MemCopyString MemCopyStringA
#endif

/******* malloc/free for uthash *******/
#define uthash_malloc(sz) MemAlloc(sz)
#define uthash_free(ptr,sz) MemFree(ptr)

/**
 * delete object and nullify pointer
 */
#define delete_and_null(x) do { delete x; x = NULL; } while(0)

/**
 * Convert half-byte's value to hex digit and vice versa
 */
#define bin2hex(x) ((x) < 10 ? ((x) + _T('0')) : ((x) + (_T('A') - 10)))
#ifdef UNICODE
#define hex2bin(x) ((((x) >= _T('0')) && ((x) <= _T('9'))) ? ((x) - _T('0')) : \
                    (((towupper(x) >= _T('A')) && (towupper(x) <= _T('F'))) ? (towupper(x) - _T('A') + 10) : 0))
#else
#define hex2bin(x) ((((x) >= '0') && ((x) <= '9')) ? ((x) - '0') : \
                    (((toupper(x) >= 'A') && (toupper(x) <= 'F')) ? (toupper(x) - 'A' + 10) : 0))
#endif

/**
 * Define MIN() and MAX()
 */
#undef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#undef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/**
 * Define _tcsicmp() for non-windows
 */
#ifndef _WIN32
#define stricmp   strcasecmp
#define strnicmp  strncasecmp
#define wcsicmp   wcscasecmp
#define wcsnicmp  wcsncasecmp
#endif

/**
 * Compare two numbers and return -1, 0, or 1
 */
#define COMPARE_NUMBERS(n1,n2) (((n1) < (n2)) ? -1 : (((n1) > (n2)) ? 1 : 0))

/**
 * Increment pointer for given number of bytes
 */
#define inc_ptr(ptr, step, ptype) ptr = (ptype *)(((char *)ptr) + step)

/**
 * Validate numerical value
 */
#define VALIDATE_VALUE(var,x,y,z) { if ((var < x) || (var > y)) var = z; }

/**
 * DCI (data collection item) data types
 */
#define DCI_DT_INT         0
#define DCI_DT_UINT        1
#define DCI_DT_INT64       2
#define DCI_DT_UINT64      3
#define DCI_DT_STRING      4
#define DCI_DT_FLOAT       5
#define DCI_DT_NULL        6
#define DCI_DT_COUNTER32   7
#define DCI_DT_COUNTER64   8
#define DCI_DT_DEPRECATED  255	/* used internally by agent */

/**
 * Insert parameter as string
 */
#ifdef UNICODE
#define STRING(x)   L#x
#else
#define STRING(x)   #x
#endif

/**
 * Pipe handle
 */
#ifdef _WIN32
#define HPIPE HANDLE
#define INVALID_PIPE_HANDLE INVALID_HANDLE_VALUE
#else
#define HPIPE int
#define INVALID_PIPE_HANDLE (-1)
#endif

#include <nxsocket.h>

#ifdef __cplusplus

#include <algorithm>

/**
 * Session enumeration callback return codes
 */
enum EnumerationCallbackResult
{
   _STOP = 0,
   _CONTINUE = 1
};

/**
 * Virtualization types
 */
enum VirtualizationType
{
   VTYPE_NONE = 0,
   VTYPE_FULL = 1,
   VTYPE_CONTAINER = 2
};

/**
 * Disable copy constructor if compiler supports it
 */
#if CAN_DELETE_COPY_CTOR
#define DISABLE_COPY_CTOR(c) c (const c &s) = delete;
#else
#define DISABLE_COPY_CTOR(c)
#endif

/**
 * Override specifier if compiler supports it
 */
#if !HAVE_OVERRIDE_SPECIFIER
#define override
#endif

#endif

/**
 * "unused" attribute
 */
#if UNUSED_ATTRIBUTE_SUPPORTED
#define __UNUSED__ __attribute__((unused))
#else
#define __UNUSED__
#endif

/**
 * Header tags
 */
#define __NX_BINARY_VERSION_TAG static const char __UNUSED__ __netxms_tag_version[] = "$nxtag.version${" NETXMS_VERSION_STRING_A "}$";
#define __NX_BINARY_BUILD_TAG static const char __UNUSED__ __netxms_tag_build[] = "$nxtag.build${" NETXMS_BUILD_TAG_A "}$";
#define __NX_BINARY_APP_NAME(name) static const char __UNUSED__ __netxms_tag_name[] = "$nxtag.name${" #name "}$";

#define __NX_BINARY_ALL_TAGS(name) \
         __NX_BINARY_APP_NAME(name) \
         __NX_BINARY_VERSION_TAG \
         __NX_BINARY_BUILD_TAG

/**
 * Executable header
 */
#if WITH_JEMALLOC && defined(JEMALLOC_LOAD_WRAPPER)
#define NETXMS_EXECUTABLE_HEADER(name) __NX_BINARY_ALL_TAGS(name) JEMALLOC_LOAD_WRAPPER
#else
#define NETXMS_EXECUTABLE_HEADER(name) __NX_BINARY_ALL_TAGS(name)
#endif

#endif   /* _nms_common_h_ */
