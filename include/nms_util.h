/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
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
** File: nms_util.h
**
**/

#ifndef _nms_util_h_
#define _nms_util_h_

#include <nms_common.h>
#include <nxatomic.h>
#include <nms_cscp.h>
#include <nms_threads.h>
#include <time.h>

#if BUNDLED_LIBJANSSON
#include <jansson/jansson.h>
#else
#include <jansson.h>
#endif

// JSON_EMBED was added in jansson 2.10 - ignore it for older version
#ifndef JSON_EMBED
#define JSON_EMBED 0
#endif

#if HAVE_POLL_H
#include <poll.h>
#endif

#if HAVE_BYTESWAP_H
#include <byteswap.h>
#endif

#include <base32.h>
#include <base64.h>

#include <functional>
#include <array>
#include <vector>
#include <string>

/*** Byte swapping ***/
#if WORDS_BIGENDIAN
#define htonq(x) (x)
#define ntohq(x) (x)
#define htond(x) (x)
#define ntohd(x) (x)
#define htonf(x) (x)
#define ntohf(x) (x)
#define SwapUCS2String(x)
#else
#if HAVE_HTONLL
#define htonq(x) htonll(x)
#else
#define htonq(x) bswap_64(x)
#endif
#if HAVE_NTOHLL
#define ntohq(x) ntohll(x)
#else
#define ntohq(x) bswap_64(x)
#endif
#define htond(x) bswap_double(x)
#define ntohd(x) bswap_double(x)
#define htonf(x) bswap_float(x)
#define ntohf(x) bswap_float(x)
#define SwapUCS2String(x) bswap_array_16((uint16_t *)(x), -1)
#endif

#if !(HAVE_DECL_BSWAP_16)
static inline uint16_t bswap_16(uint16_t val)
{
   return (val >> 8) | ((val << 8) & 0xFF00);
}
#endif

#if !(HAVE_DECL_BSWAP_32)
static inline uint32_t bswap_32(uint32_t val)
{
   uint32_t result;
   BYTE *sptr = (BYTE *)&val;
   BYTE *dptr = (BYTE *)&result + 3;
   for(int i = 0; i < 4; i++, sptr++, dptr--)
      *dptr = *sptr;
   return result;
}
#endif

#if !(HAVE_DECL_BSWAP_64)
static inline uint64_t bswap_64(uint64_t val)
{
   uint64_t result;
   BYTE *sptr = (BYTE *)&val;
   BYTE *dptr = (BYTE *)&result + 7;
   for(int i = 0; i < 8; i++, sptr++, dptr--)
      *dptr = *sptr;
   return result;
}
#endif

static inline double bswap_double(double val)
{
   double result;
   BYTE *sptr = (BYTE *)&val;
   BYTE *dptr = (BYTE *)&result + 7;
   for(int i = 0; i < 8; i++, sptr++, dptr--)
      *dptr = *sptr;
   return result;
}

static inline float bswap_float(float val)
{
   float result;
   BYTE *sptr = (BYTE *)&val;
   BYTE *dptr = (BYTE *)&result + 3;
   for(int i = 0; i < 4; i++, sptr++, dptr--)
      *dptr = *sptr;
   return result;
}

void LIBNETXMS_EXPORTABLE bswap_array_16(uint16_t *v, int len);
void LIBNETXMS_EXPORTABLE bswap_array_32(uint32_t *v, int len);

/*** toupper/tolower ***/
#if !HAVE_TOLOWER
static inline char tolower(char c)
{
   return ((c >= 'A') && (c <= 'Z')) ? c + ('a' - 'A') : c;
}
#endif

#if !HAVE_TOWLOWER
static inline WCHAR towlower(WCHAR c)
{
   return ((c >= L'A') && (c <= L'Z')) ? c + (L'a' - L'A') : c;
}
#endif

#if !HAVE_TOUPPER
static inline char toupper(char c)
{
   return ((c >= 'a') && (c <= 'z')) ? c - ('a' - 'A') : c;
}
#endif

#if !HAVE_TOWUPPER
static inline WCHAR towupper(WCHAR c)
{
   return ((c >= L'a') && (c <= L'z')) ? c - (L'a' - L'A') : c;
}
#endif

/*** Serial communications ***/
#ifdef _WIN32

#define FLOW_NONE       0
#define FLOW_SOFTWARE   1
#define FLOW_HARDWARE   2

#else    /* _WIN32 */

#ifdef HAVE_TERMIOS_H
# include <termios.h>
#else
# error termios.h not found
#endif

#endif   /* _WIN32 */

/**
 * Return codes for IcmpPing()
 */
#define ICMP_SUCCESS          0
#define ICMP_UNREACHABLE      1
#define ICMP_TIMEOUT          2
#define ICMP_RAW_SOCK_FAILED  3
#define ICMP_API_ERROR        4
#define ICMP_SEND_FAILED      5

/**
 * Token types for configuration loader
 */
#define CT_LONG            0
#define CT_STRING          1
#define CT_STRING_CONCAT   2   /* Concatenated string - each value concatenated to final string */
#define CT_END_OF_LIST     3
#define CT_BOOLEAN_FLAG_32 4   /* Boolean flag - set or clear bit in provided 32 bit uinteger */
#define CT_WORD            5
#define CT_IGNORE          6
#define CT_MB_STRING       7
#define CT_BOOLEAN_FLAG_64 8   /* Boolean flag - set or clear bit in provided 64 bit uinteger */
#define CT_SIZE_BYTES      9   /* 64 bit integer, automatically converts K, M, G, T suffixes using 1024 as base) */
#define CT_SIZE_UNITS      10  /* 64 bit integer, automatically converts K, M, G, T suffixes using 1000 as base) */
#define CT_STRING_SET      11  /* Each value added to StringSet object */
#define CT_STRING_LIST     12  /* Each value added to StringList object */
#define CT_BOOLEAN         13  /* Data is pointer to bool variable which will be set to true or false */

/**
 * Uninitialized value for override indicator
 */
#define NXCONFIG_UNINITIALIZED_VALUE  (-1)

/**
 * Return codes for NxLoadConfig()
 */
#define NXCFG_ERR_OK       0
#define NXCFG_ERR_NOFILE   1
#define NXCFG_ERR_SYNTAX   2

/**
 * nxlog_open() flags
 */
#define NXLOG_USE_SYSLOG        ((uint32_t)0x00000001)  /* use syslog (Event Log on Windows) as log device */
#define NXLOG_PRINT_TO_STDOUT   ((uint32_t)0x00000002)  /* print log copy to stdout */
#define NXLOG_BACKGROUND_WRITER ((uint32_t)0x00000004)  /* enable background log writer */
#define NXLOG_DEBUG_MODE        ((uint32_t)0x00000008)  /* log debug mode - pront additional debug infor about logger itself */
#define NXLOG_USE_SYSTEMD       ((uint32_t)0x00000010)  /* use stderr as log device and systemd output format */
#define NXLOG_JSON_FORMAT       ((uint32_t)0x00000020)  /* write log in JSON format */
#define NXLOG_USE_STDOUT        ((uint32_t)0x00000040)  /* use stdout as log device */
#define NXLOG_ROTATION_ERROR    ((uint32_t)0x40000000)  /* internal flag, should not be set by caller */
#define NXLOG_IS_OPEN           ((uint32_t)0x80000000)  /* internal flag, should not be set by caller */

/**
 * nxlog rotation policy
 */
#define NXLOG_ROTATION_DISABLED  0
#define NXLOG_ROTATION_DAILY     1
#define NXLOG_ROTATION_BY_SIZE   2

/**
 * Custom wcslen()
 */
#if !HAVE_WCSLEN
size_t LIBNETXMS_EXPORTABLE wcslen(const WCHAR *s);
#endif

/**
 * Custom wcsncpy()
 */
#if !HAVE_WCSNCPY
WCHAR LIBNETXMS_EXPORTABLE *wcsncpy(WCHAR *dest, const WCHAR *src, size_t n);
#endif

// Some AIX versions have broken wcsdup() so we use internal implementation
#if !HAVE_WCSDUP || defined(_AIX)
#define wcsdup MemCopyStringW
#endif

/******* UNICODE related conversion and helper functions *******/

// Basic string functions
#ifdef UNICODE_UCS2

#define ucs2_strlen  wcslen
#define ucs2_strncpy wcsncpy
#define ucs2_strdup  MemCopyStringW

size_t LIBNETXMS_EXPORTABLE ucs4_strlen(const UCS4CHAR *s);
UCS4CHAR LIBNETXMS_EXPORTABLE *ucs4_strncpy(UCS4CHAR *dest, const UCS2CHAR *src, size_t n);
UCS4CHAR LIBNETXMS_EXPORTABLE *ucs4_strdup(const UCS4CHAR *src);

#else

size_t LIBNETXMS_EXPORTABLE ucs2_strlen(const UCS2CHAR *s);
UCS2CHAR LIBNETXMS_EXPORTABLE *ucs2_strncpy(UCS2CHAR *dest, const UCS2CHAR *src, size_t n);
UCS2CHAR LIBNETXMS_EXPORTABLE *ucs2_strdup(const UCS2CHAR *src);

#define ucs4_strlen  wcslen
#define ucs4_strncpy wcsncpy
#define ucs4_strdup  MemCopyStringW

#endif

// Character conversion functions
#ifdef _WIN32
#define wchar_to_mb(wstr, wlen, mstr, mlen)   (size_t)WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, (wstr), (int)(wlen), (mstr), (int)(mlen), nullptr, nullptr)
#define mb_to_wchar(mstr, mlen, wstr, wlen)   (size_t)MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (mstr), (int)(mlen), (wstr), (int)(wlen))
#else
size_t LIBNETXMS_EXPORTABLE wchar_to_mb(const wchar_t *src, ssize_t srcLen, char *dst, size_t dstLen);
size_t LIBNETXMS_EXPORTABLE mb_to_wchar(const char *src, ssize_t srcLen, wchar_t *dst, size_t dstLen);
#endif

size_t LIBNETXMS_EXPORTABLE wchar_to_mbcp(const wchar_t *src, ssize_t srcLen, char *dst, size_t dstLen, const char *codepage);
size_t LIBNETXMS_EXPORTABLE mbcp_to_wchar(const char *src, ssize_t srcLen, wchar_t *dst, size_t dstLen, const char *codepage);

size_t LIBNETXMS_EXPORTABLE ucs2_to_ucs4(const UCS2CHAR *src, ssize_t srcLen, UCS4CHAR *dst, size_t dstLen);
size_t LIBNETXMS_EXPORTABLE ucs2_to_utf8(const UCS2CHAR *src, ssize_t srcLen, char *dst, size_t dstLen);
size_t LIBNETXMS_EXPORTABLE ucs2_utf8len(const UCS2CHAR *src, ssize_t srcLen);
#ifdef UNICODE_UCS2
#define ucs2_to_mb   wchar_to_mb
#else
size_t LIBNETXMS_EXPORTABLE ucs2_to_mb(const UCS2CHAR *src, ssize_t srcLen, char *dst, size_t dstLen);
#endif
size_t LIBNETXMS_EXPORTABLE ucs2_to_ASCII(const UCS2CHAR *src, ssize_t srcLen, char *dst, size_t dstLen);
size_t LIBNETXMS_EXPORTABLE ucs2_to_ISO8859_1(const UCS2CHAR *src, ssize_t srcLen, char *dst, size_t dstLen);

size_t LIBNETXMS_EXPORTABLE ucs4_to_ucs2(const UCS4CHAR *src, ssize_t srcLen, UCS2CHAR *dst, size_t dstLen);
size_t LIBNETXMS_EXPORTABLE ucs4_ucs2len(const UCS4CHAR *src, ssize_t srcLen);
size_t LIBNETXMS_EXPORTABLE ucs4_to_utf8(const UCS4CHAR *src, ssize_t srcLen, char *dst, size_t dstLen);
size_t LIBNETXMS_EXPORTABLE ucs4_utf8len(const UCS4CHAR *src, ssize_t srcLen);
#ifdef UNICODE_UCS4
#define ucs4_to_mb   wchar_to_mb
#else
size_t LIBNETXMS_EXPORTABLE ucs4_to_mb(const UCS4CHAR *src, ssize_t srcLen, char *dst, size_t dstLen);
#endif
size_t LIBNETXMS_EXPORTABLE ucs4_to_ASCII(const UCS4CHAR *src, ssize_t srcLen, char *dst, size_t dstLen);
size_t LIBNETXMS_EXPORTABLE ucs4_to_ISO8859_1(const UCS4CHAR *src, ssize_t srcLen, char *dst, size_t dstLen);

size_t LIBNETXMS_EXPORTABLE utf8_to_ucs4(const char *src, ssize_t srcLen, UCS4CHAR *dst, size_t dstLen);
size_t LIBNETXMS_EXPORTABLE utf8_ucs4len(const char *src, ssize_t srcLen);
size_t LIBNETXMS_EXPORTABLE utf8_to_ucs2(const char *src, ssize_t srcLen, UCS2CHAR *dst, size_t dstLen);
size_t LIBNETXMS_EXPORTABLE utf8_ucs2len(const char *src, ssize_t srcLen);
size_t LIBNETXMS_EXPORTABLE utf8_to_mb(const char *src, ssize_t srcLen, char *dst, size_t dstLen);
size_t LIBNETXMS_EXPORTABLE utf8_to_mbcp(const char *src, ssize_t srcLen, char *dst, size_t dstLen, const char* codepage);
size_t LIBNETXMS_EXPORTABLE utf8_to_ASCII(const char *src, ssize_t srcLen, char *dst, size_t dstLen);
size_t LIBNETXMS_EXPORTABLE utf8_to_ISO8859_1(const char *src, ssize_t srcLen, char *dst, size_t dstLen);

#ifdef UNICODE_UCS4
#define mb_to_ucs4   mb_to_wchar
#else
size_t LIBNETXMS_EXPORTABLE mb_to_ucs4(const char *src, ssize_t srcLen, UCS4CHAR *dst, size_t dstLen);
#endif
#ifdef UNICODE_UCS2
#define mb_to_ucs2   mb_to_wchar
#else
size_t LIBNETXMS_EXPORTABLE mb_to_ucs2(const char *src, ssize_t srcLen, UCS2CHAR *dst, size_t dstLen);
#endif
size_t LIBNETXMS_EXPORTABLE mb_to_utf8(const char *src, ssize_t srcLen, char *dst, size_t dstLen);
size_t LIBNETXMS_EXPORTABLE mbcp_to_utf8(const char *src, ssize_t srcLen, char *dst, size_t dstLen, const char* codepage);
size_t LIBNETXMS_EXPORTABLE ASCII_to_utf8(const char *src, ssize_t srcLen, char *dst, size_t dstLen);
size_t LIBNETXMS_EXPORTABLE ASCII_to_ucs2(const char *src, ssize_t srcLen, UCS2CHAR *dst, size_t dstLen);
size_t LIBNETXMS_EXPORTABLE ASCII_to_ucs4(const char *src, ssize_t srcLen, UCS4CHAR *dst, size_t dstLen);
size_t LIBNETXMS_EXPORTABLE ISO8859_1_to_utf8(const char *src, ssize_t srcLen, char *dst, size_t dstLen);
size_t LIBNETXMS_EXPORTABLE ISO8859_1_to_ucs2(const char *src, ssize_t srcLen, UCS2CHAR *dst, size_t dstLen);
size_t LIBNETXMS_EXPORTABLE ISO8859_1_to_ucs4(const char *src, ssize_t srcLen, UCS4CHAR *dst, size_t dstLen);

#ifdef UNICODE_UCS4
#define utf8_to_wchar   utf8_to_ucs4
#define utf8_wcharlen   utf8_ucs4len
#define ASCII_to_wchar  ASCII_to_ucs4
#define wchar_to_utf8   ucs4_to_utf8
#define wchar_utf8len   ucs4_utf8len
#define wchar_to_ASCII  ucs4_to_ASCII
#else
#define utf8_to_wchar   utf8_to_ucs2
#define utf8_wcharlen   utf8_ucs2len
#define ASCII_to_wchar  ASCII_to_ucs2
#define wchar_to_utf8   ucs2_to_utf8
#define wchar_utf8len   ucs2_utf8len
#define wchar_to_ASCII  ucs2_to_ASCII
#endif

#ifdef UNICODE
#define utf8_to_tchar   utf8_to_wchar
#define tchar_to_utf8   wchar_to_utf8
#else
#define utf8_to_tchar   utf8_to_mb
#define tchar_to_utf8   mb_to_utf8
#endif

// Conversion helpers
WCHAR LIBNETXMS_EXPORTABLE *WideStringFromMBString(const char *src);
WCHAR LIBNETXMS_EXPORTABLE *WideStringFromMBStringSysLocale(const char *src);
WCHAR LIBNETXMS_EXPORTABLE *WideStringFromUTF8String(const char *src);
char LIBNETXMS_EXPORTABLE *MBStringFromWideString(const WCHAR *src);
char LIBNETXMS_EXPORTABLE *MBStringFromWideStringSysLocale(const WCHAR *src);
char LIBNETXMS_EXPORTABLE *MBStringFromUTF8String(const char *src);
char LIBNETXMS_EXPORTABLE *UTF8StringFromWideString(const WCHAR *src);
char LIBNETXMS_EXPORTABLE *UTF8StringFromMBString(const char *src);
UCS2CHAR LIBNETXMS_EXPORTABLE *UCS2StringFromUCS4String(const UCS4CHAR *src);
UCS4CHAR LIBNETXMS_EXPORTABLE *UCS4StringFromUCS2String(const UCS2CHAR *src);

#ifdef UNICODE_UCS2
#define UCS2StringFromMBString   WideStringFromMBString
#define MBStringFromUCS2String   MBStringFromWideString
#define UCS2StringFromUTF8String WideStringFromUTF8String
#define UTF8StringFromUCS2String UTF8StringFromWideString
#else
UCS2CHAR LIBNETXMS_EXPORTABLE *UCS2StringFromMBString(const char *src);
char LIBNETXMS_EXPORTABLE *MBStringFromUCS2String(const UCS2CHAR *src);
UCS2CHAR LIBNETXMS_EXPORTABLE *UCS2StringFromUTF8String(const char *src);
char LIBNETXMS_EXPORTABLE *UTF8StringFromUCS2String(const UCS2CHAR *src);
#endif

#ifdef UNICODE_UCS4
#define UCS4StringFromMBString   WideStringFromMBString
#define MBStringFromUCS4String   MBStringFromWideString
#else
UCS4CHAR LIBNETXMS_EXPORTABLE *UCS4StringFromMBString(const char *src);
char LIBNETXMS_EXPORTABLE *MBStringFromUCS4String(const UCS4CHAR *src);
#endif

#ifdef UNICODE
#define UTF8StringFromTString(s) UTF8StringFromWideString(s)
#define TStringFromUTF8String(s) WideStringFromUTF8String(s)
#else
#define UTF8StringFromTString(s) UTF8StringFromMBString(s)
#define TStringFromUTF8String(s) MBStringFromUTF8String(s)
#endif

/**
 * Convert wide character string to multibyte character string using current system locale
 */
static inline void WideCharToMultiByteSysLocale(const WCHAR *src, char *dst, size_t dstSize)
{
#if HAVE_WCSTOMBS
   size_t bytes = wcstombs(dst, src, dstSize);
   if (bytes == (size_t)-1)
      *dst = 0;
   else if (bytes < dstSize)
      dst[bytes] = 0;
   else
      dst[dstSize - 1] = 0;
#else
   wchar_to_mb(src, -1, dst, dstSize);
#endif
}

/**
 * Convert multibyte character string to wide character string using current system locale
 */
static inline void MultiByteToWideCharSysLocale(const char *src, WCHAR *dst, size_t dstSize)
{
#if HAVE_MBSTOWCS
   size_t chars = mbstowcs(dst, src, dstSize);
   if (chars == (size_t)-1)
      *dst = 0;
   else if (chars < dstSize)
      dst[chars] = 0;
   else
      dst[dstSize - 1] = 0;
#else
   mb_to_wchar(src, -1, dst, dstSize);
#endif
}

/******* end of UNICODE related conversion and helper functions *******/


/******* Timestamp formatting functions *******/
TCHAR LIBNETXMS_EXPORTABLE *FormatTimestamp(time_t t, TCHAR *buffer);
TCHAR LIBNETXMS_EXPORTABLE *FormatTimestampMs(int64_t timestamp, TCHAR *buffer);
std::string LIBNETXMS_EXPORTABLE FormatISO8601Timestamp(time_t t);
std::string LIBNETXMS_EXPORTABLE FormatISO8601TimestampMs(int64_t t);

/**
 * Get current time in milliseconds
 */
static inline int64_t GetCurrentTimeMs()
{
#ifdef _WIN32
   FILETIME ft;
   GetSystemTimeAsFileTime(&ft);
   LARGE_INTEGER li;
   li.LowPart  = ft.dwLowDateTime;
   li.HighPart = ft.dwHighDateTime;
   return (li.QuadPart - EPOCHFILETIME) / 10000;  // Offset to the Epoch time and convert to milliseconds
#else
   struct timeval tv;
   gettimeofday(&tv, nullptr);
   return (int64_t)tv.tv_sec * 1000 + (int64_t)(tv.tv_usec / 1000);
#endif
}

/**
 * Get number of milliseconds counted by monotonic clock since some
 * unspecified point in the past (system boot time on most systems)
 */
static inline int64_t GetMonotonicClockTime()
{
#if defined(_WIN32)
   return static_cast<int64_t>(GetTickCount64());
#elif defined(__sun)
   return static_cast<int64_t>(gethrtime() / _LL(1000000));
#else
   struct timespec ts;
   clock_gettime(CLOCK_MONOTONIC, &ts);
   return static_cast<int64_t>(ts.tv_sec) * _LL(1000) + static_cast<int64_t>(ts.tv_nsec) / _LL(1000000);
#endif
}

/**
 * Timestamp class - holds timestamp in milliseconds
 */
class Timestamp
{
private:
   int64_t v;

   Timestamp(int64_t t)
   {
      v = t;
   }

public:
   Timestamp() = default;

   static Timestamp now()
   {
      return Timestamp(GetCurrentTimeMs());
   }

   static Timestamp fromMilliseconds(int64_t ms)
   {
      return Timestamp(ms);
   }

   static Timestamp fromTime(time_t t)
   {
      return Timestamp(static_cast<int64_t>(t) * 1000);
   }

   bool isNull() const
   {
      return v == 0;
   }

   int64_t asMilliseconds() const
   {
      return v;
   }

   int64_t asMicroseconds() const
   {
      return v * _LL(1000);
   }

   int64_t asNanoseconds() const
   {
      return v * _LL(1000000);
   }

   time_t asTime() const
   {
      return static_cast<time_t>(v / 1000);
   }

   json_t *asJson() const
   {
      return (v == 0) ? json_null() : json_string(FormatISO8601TimestampMs(v).c_str());
   }

   bool operator==(const Timestamp& other) const
   {
      return v == other.v;
   }

   bool operator!=(const Timestamp& other) const
   {
      return v != other.v;
   }

   bool operator<(const Timestamp& other) const
   {
      return v < other.v;
   }

   bool operator<=(const Timestamp& other) const
   {
      return v <= other.v;
   }

   bool operator>(const Timestamp& other) const
   {
      return v > other.v;
   }

   bool operator>=(const Timestamp& other) const
   {
      return v >= other.v;
   }

   int64_t operator-(const Timestamp& other) const
   {
      return v - other.v;
   }

   Timestamp operator-(int64_t milliseconds) const
   {
      return Timestamp(v - milliseconds);
   }

   Timestamp& operator-=(int64_t milliseconds)
   {
      v -= milliseconds;
      return *this;
   }

   Timestamp operator+(int64_t milliseconds) const
   {
      return Timestamp(v + milliseconds);
   }

   Timestamp& operator+=(int64_t milliseconds)
   {
      v += milliseconds;
      return *this;
   }
};

/**
 * Class for serial communications
 */
#ifndef _WIN32
enum
{
	NOPARITY,
	ODDPARITY,
	EVENPARITY,
	ONESTOPBIT,
	TWOSTOPBITS
};

enum
{
	FLOW_NONE,
	FLOW_HARDWARE,
	FLOW_SOFTWARE
};

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE (-1)
#endif
#endif   /* _WIN32 */

class LIBNETXMS_EXPORTABLE Serial
{
private:
	TCHAR *m_device;
	uint32_t m_timeout;
	int m_speed;
	int m_dataBits;
	int m_stopBits;
	int m_parity;
	int m_flowControl;
   uint32_t m_writeDelay;
	size_t m_writeBlockSize;

#ifndef _WIN32
	int m_handle;
	struct termios m_originalSettings;
#else
	HANDLE m_handle;
#endif

   bool writeBlock(const void *data, size_t size);

public:
	Serial();
	~Serial();

	bool open(const TCHAR *device);
	void close();

	void setTimeout(uint32_t timeout);
   bool set(int speed, int dataBits = 8, int parity = NOPARITY, int stopBits = ONESTOPBIT, int flowControl = FLOW_NONE);
   void setWriteBlockSize(size_t bs) { m_writeBlockSize = bs; }
   void setWriteDelay(uint32_t delay) { m_writeDelay = delay; }

	ssize_t read(void *buffer, size_t size); /* waits up to timeout and do single read */
	ssize_t readAll(void *buffer, size_t size); /* read until timeout or out of space */
	ssize_t readToMark(char *buffer, size_t size, const char **marks, char **occurence);
	bool write(const void *buffer, size_t size);
	void flush();
	bool restart();
};

/**
 * Buffer pointer with small allocation optimization
 */
template<typename T, size_t BUFFER_SIZE = 64> class Buffer
{
private:
   T *m_allocatedBuffer;
   size_t m_size;
   BYTE m_internalBuffer[BUFFER_SIZE - sizeof(T*) - sizeof(size_t)];

public:
   Buffer()
   {
      m_allocatedBuffer = nullptr;
      m_size = 0;
   }

   Buffer(size_t numElements)
   {
      m_size = numElements * sizeof(T);
      if (m_size <= sizeof(m_internalBuffer))
      {
         m_allocatedBuffer = nullptr;
         memset(m_internalBuffer, 0, m_size);
      }
      else
      {
         m_allocatedBuffer = MemAllocArray<T>(numElements);
      }
   }

   Buffer(const T* data, size_t numElements)
   {
      m_size = numElements * sizeof(T);
      if (m_size <= sizeof(m_internalBuffer))
      {
         memcpy(m_internalBuffer, data, m_size);
         m_allocatedBuffer = nullptr;
      }
      else
      {
         m_allocatedBuffer = MemCopyBlock(data, m_size);
      }
   }

   Buffer(const Buffer& src)
   {
      m_size = src.m_size;
      if (src.m_allocatedBuffer != nullptr)
      {
         m_allocatedBuffer = MemCopyBlock(src.m_allocatedBuffer, m_size);
      }
      else
      {
         m_allocatedBuffer = nullptr;
         memcpy(m_internalBuffer, src.m_internalBuffer, sizeof(m_internalBuffer));
      }
   }

   Buffer(Buffer&& src)
   {
      m_allocatedBuffer = src.m_allocatedBuffer;
      m_size = src.m_size;
      if (m_allocatedBuffer != nullptr)
      {
         src.m_allocatedBuffer = nullptr;
      }
      else
      {
         memcpy(m_internalBuffer, src.m_internalBuffer, sizeof(m_internalBuffer));
      }
      src.m_size = 0;
   }

   ~Buffer()
   {
      MemFree(m_allocatedBuffer);
   }

   T *buffer() { return (m_allocatedBuffer != nullptr) ? m_allocatedBuffer : reinterpret_cast<T*>(m_internalBuffer); }
   operator T*() { return buffer(); }
   operator const T*() const { return buffer(); }
   T& operator[](size_t index) { return buffer()[index]; }
   T& operator[](ssize_t index) { return buffer()[index]; }
#ifdef __64BIT__
   T& operator[](int index) { return buffer()[index]; }
   T& operator[](uint32_t index) { return buffer()[index]; }
#endif
   size_t size() const { return m_size; }
   size_t numElements() const { return m_size / sizeof(T); }
   bool isInternal() const { return m_allocatedBuffer == nullptr; }

   void realloc(size_t numElements)
   {
      size_t size = numElements * sizeof(T);
      if (size > sizeof(m_internalBuffer))
      {
         if (m_allocatedBuffer == nullptr)
         {
            m_allocatedBuffer = MemAllocArrayNoInit<T>(numElements);
            memcpy(m_allocatedBuffer, m_internalBuffer, m_size);
         }
         else
         {
            m_allocatedBuffer = MemRealloc(m_allocatedBuffer, size);
         }
      }
      else if (m_allocatedBuffer != nullptr)
      {
         memcpy(m_internalBuffer, m_allocatedBuffer, size);
         MemFreeAndNull(m_allocatedBuffer);
      }
      m_size = size;
   }

   void set(const T *data, size_t numElements)
   {
      realloc(numElements);
      memcpy(buffer(), data, numElements * sizeof(T));
   }

   void setPreallocated(T *data, size_t numElements)
   {
      MemFree(m_allocatedBuffer);
      m_allocatedBuffer = data;
      m_size = numElements * sizeof(T);
   }

   T *takeBuffer()
   {
      T *data;
      if (m_allocatedBuffer != nullptr)
      {
         data = m_allocatedBuffer;
         m_allocatedBuffer = nullptr;
      }
      else
      {
         data = MemCopyBlock(reinterpret_cast<T*>(m_internalBuffer), m_size);
      }
      m_size = 0;
      return data;
   }
};

/**
 * Object memory pool
 */
template <class T> class ObjectMemoryPool
{
private:
   void *m_currentRegion;
   T *m_firstDeleted;
   size_t m_headerSize;
   size_t m_regionSize;
   size_t m_elementSize;
   size_t m_allocated;
   size_t m_elements;

public:
   /**
    * Create new memory pool
    */
   ObjectMemoryPool(size_t regionCapacity = 256)
   {
      m_headerSize = sizeof(void*);
      if (m_headerSize % 16 != 0)
         m_headerSize += 16 - m_headerSize % 16;
      m_elementSize = sizeof(T);
      if (m_elementSize % 8 != 0)
         m_elementSize += 8 - m_elementSize % 8;
      m_regionSize = m_headerSize + regionCapacity * m_elementSize;
      m_currentRegion = MemAlloc(m_regionSize);
      *((void **)m_currentRegion) = nullptr; // pointer to previous region
      m_firstDeleted = nullptr;
      m_allocated = m_headerSize;
      m_elements = 0;
   }

   /**
    * Destroy memory pool. Will destroy allocated memory but object destructors will not be called.
    */
   ~ObjectMemoryPool()
   {
      void *r = m_currentRegion;
      while(r != nullptr)
      {
         void *n = *((void **)r);
         MemFree(r);
         r = n;
      }
   }

   /**
    * Release allocated memory without calling object destructors
    */
   void reset()
   {
      void *r = *((void **)m_currentRegion);
      while(r != nullptr)
      {
         void *n = *((void **)r);
         MemFree(r);
         r = n;
      }
      *((void **)m_currentRegion) = nullptr;
      m_allocated = m_headerSize;
      m_firstDeleted = nullptr;
   }

   /**
    * Allocate memory for object without initializing
    */
   T *allocate()
   {
      T *p;
      if (m_firstDeleted != nullptr)
      {
         p = m_firstDeleted;
         m_firstDeleted = *((T**)p);
      }
      else if (m_allocated < m_regionSize)
      {
         p = (T*)((char*)m_currentRegion + m_allocated);
         m_allocated += m_elementSize;
      }
      else
      {
         void *region = MemAlloc(m_regionSize);
         *((void **)region) = m_currentRegion;
         m_currentRegion = region;
         p = (T*)((char*)m_currentRegion + m_headerSize);
         m_allocated = m_headerSize + m_elementSize;
      }
      m_elements++;
      return p;
   }

   /**
    * Create object using default constructor
    */
   T *create()
   {
      T *p = allocate();
      return new(p) T();
   }

   /**
    * Free memory block without calling object destructor
    */
   void free(T *p)
   {
      if (p != nullptr)
      {
         *((T**)p) = m_firstDeleted;
         m_firstDeleted = p;
         m_elements--;
      }
   }

   /**
    * Destroy object
    */
   void destroy(T *p)
   {
      if (p != nullptr)
      {
         p->~T();
         free(p);
      }
   }

   /**
    * Get region capacity
    */
   size_t getRegionCapacity() const
   {
      return (m_regionSize - m_headerSize) / m_elementSize;
   }

   /**
    * Get total number of allocated elememnts
    */
   size_t getElementCount() const
   {
      return m_elements;
   }

   /**
    * Get number of allocated regions
    */
   size_t getRegionCount() const
   {
      size_t count = 0;
      void *r = m_currentRegion;
      while(r != nullptr)
      {
         count++;
         r = *((void **)r);
      }
      return count;
   }

   /**
    * Get estimated memory usage by this pool
    */
   uint64_t getMemoryUsage() const
   {
      return sizeof(ObjectMemoryPool<T>) + getRegionCount() * m_regionSize;
   }
};

/**
 * Synchronized version of ObjectMemoryPool (uses spinlock for locking)
 */
template<typename T> class SynchronizedObjectMemoryPool : public ObjectMemoryPool<T>
{
private:
   VolatileCounter m_lock;

   void lock()
   {
      while (InterlockedCompareExchange(&m_lock, 1, 0) != 0);
   }

   void unlock()
   {
      InterlockedDecrement(&m_lock);
   }

public:
   /**
    * Create new memory pool
    */
   SynchronizedObjectMemoryPool(size_t regionCapacity = 256) : ObjectMemoryPool<T>(regionCapacity)
   {
      m_lock = 0;
   }

   /**
    * Allocate memory for object without initializing
    */
   T *allocate()
   {
      lock();
      T *p = ObjectMemoryPool<T>::allocate();
      unlock();
      return p;
   }

   /**
    * Create object using default constructor
    */
   T *create()
   {
      T *p = allocate();
      return new(p) T();
   }

   /**
    * Free memory block without calling object destructor
    */
   void free(T *p)
   {
      lock();
      ObjectMemoryPool<T>::free(p);
      unlock();
   }

   /**
    * Destroy object
    */
   void destroy(T *p)
   {
      if (p != nullptr)
      {
         p->~T();
         free(p);
      }
   }
};

/**
 * Simple allocate only heap
 */
class LIBNETXMS_EXPORTABLE MemoryPool
{
private:
   void *m_currentRegion;
   size_t m_headerSize;
   size_t m_regionSize;
   size_t m_allocated;

public:
   /**
    * Create new memory pool
    */
   MemoryPool(size_t regionSize = 8192);
   
   /**
    * Forbid copy construction
    */
   MemoryPool(const MemoryPool& src) = delete;

   /**
    * Move constructor
    */
   MemoryPool(MemoryPool&& src);

   /**
    * Move assignment
    */
   MemoryPool& operator=(MemoryPool&& src);

   /**
    * Destroy memory pool (object destructors will not be called)
    */
   ~MemoryPool();

   /**
    * Allocate memory block
    */
   void *allocate(size_t size);

   /**
    * Allocate memory block for array of objects
    */
   template<typename T> T *allocateArray(size_t count)
   {
      return static_cast<T*>(allocate(sizeof(T) * count));
   }

   /**
    * Create object in pool
    */
   template<typename T> T *create()
   {
      void *p = allocate(sizeof(T));
      return new(p) T();
   }

   /**
    * Allocate string
    */
   TCHAR *allocateString(size_t size)
   {
      return static_cast<TCHAR*>(allocate(sizeof(TCHAR) * size));
   }

   /**
    * Allocate string
    */
   WCHAR *allocateStringW(size_t size)
   {
      return static_cast<WCHAR*>(allocate(sizeof(WCHAR) * size));
   }

   /**
    * Allocate string
    */
   char *allocateStringA(size_t size)
   {
      return static_cast<char*>(allocate(size));
   }

   /**
    * Create copy of given C string within pool
    */
   TCHAR *copyString(const TCHAR *s);

   /**
    * Create copy of given memory block within pool
    */
   template<typename T> T *copyMemoryBlock(const T *b, size_t s)
   {
      void *p = allocate(s);
      memcpy(p, b, s);
      return static_cast<T*>(p);
   }

   /**
    * Drop all allocated memory except one region
    */
   void clear();

   /**
    * Get region size
    */
   size_t regionSize() const { return m_regionSize; }

   /**
    * Get number of allocated regions
    */
   size_t getRegionCount() const;
};

/**
 * Resource pool element
 */
template<typename T> struct ResourcePoolElement
{
   ResourcePoolElement<T> *next;
   T *resource;

   ResourcePoolElement(T *r)
   {
      resource = r;
      next = NULL;
   }
};

/**
 * Generic resource pool
 */
template<typename T, T* (*Create)(), void (*Destroy)(T*), void (*Reset)()> class ResourcePool
{
private:
   ResourcePoolElement<T> *m_free;
   ResourcePoolElement<T> *m_used;
   MemoryPool m_metadata;

public:
   ResourcePool()
   {
      m_free = nullptr;
      m_used = nullptr;
   }

   ~ResourcePool()
   {
      ResourcePoolElement<T> *handle = m_used;
      while(handle != nullptr)
      {
         Destroy(handle->resource);
         handle = handle->next;
      }

      handle = m_free;
      while(handle != nullptr)
      {
         Destroy(handle->resource);
         handle = handle->next;
      }
   }

   /**
    * Acquire resource
    */
   T *acquire()
   {
      ResourcePoolElement<T> *handle;
      if (m_free != nullptr)
      {
         handle = m_free;
         m_free = handle->next;
      }
      else
      {
         handle = m_metadata.create<T>(Create());
      }
      handle->next = m_used;
      m_used = handle;
      return handle->resource;
   }

   /**
    * Release resource
    */
   void release(T *resource)
   {
      ResourcePoolElement<T> *handle = m_used, *prev = nullptr;
      while((handle != nullptr) && (handle->resource != resource))
      {
         prev = handle;
         handle = handle->next;
      }
      if (handle != nullptr)
      {
         Reset(handle->resource);
         if (prev != nullptr)
            prev->next = handle->next;
         else
            m_used = handle->next;
         handle->next = m_free;
         m_free = handle;
      }
   }
};

/**
 * Class that can store any object guarded by mutex
 */
template <class T> class ObjectLock
{
   ObjectLock(const ObjectLock& src) = delete;
   ObjectLock& operator =(const ObjectLock& src) = delete;

private:
   Mutex m_mutex;
   T m_object;

public:
   ObjectLock() : m_mutex(MutexType::FAST) { }
   ObjectLock(const T& object) : m_mutex(MutexType::FAST), m_object(object) { }

   void lock() { m_mutex.lock(); }
   bool timedLock(uint32_t timeout) { return m_mutex.timedLock(timeout); }
   void unlock() { m_mutex.unlock(); }

   const T& get() { return m_object; }
   void set(const T& object) { m_object = object; }
};

/**
 * uuid_t wrapper class
 */
class uuid;

/**
 * NXCP message class
 */
class NXCPMessage;

/**
 * String list class
 */
class LIBNETXMS_EXPORTABLE StringList
{
private:
   MemoryPool m_pool;
   int m_count;
   int m_allocated;
   TCHAR **m_values;

public:
   StringList();
   StringList(const StringList *src);
   StringList(const StringList &src);
   StringList(StringList&& src);
   StringList(const TCHAR *src, const TCHAR *separator);
   StringList(const NXCPMessage& msg, uint32_t baseId, uint32_t countId);
   StringList(const NXCPMessage& msg, uint32_t fieldId);
   StringList(json_t *json);

   StringList& operator=(const StringList& src)
   {
      clear();
      addAll(src);
      return *this;
   }

   StringList& operator=(StringList&& src);

   void add(const TCHAR *value);
   void addPreallocated(TCHAR *value)
   {
      add(value);
      MemFree(value);
   }
   void add(int32_t value);
   void add(uint32_t value);
   void add(int64_t value);
   void add(uint64_t value);
   void add(double value);
   void replace(int index, const TCHAR *value);
   void addOrReplace(int index, const TCHAR *value);
   void addOrReplacePreallocated(int index, TCHAR *value);
   void addUTF8String(const char *value)
   {
#ifdef UNICODE
      addPreallocated(WideStringFromUTF8String(value));
#else
      addPreallocated(MBStringFromUTF8String(value));
#endif
   }
#ifdef UNICODE
   void addMBString(const char *value);
#else
   void addMBString(const char *value) { add(value); }
#endif
   void addAll(const StringList *src);
   void addAll(const StringList& src) { addAll(&src); }

   void insert(int pos, const TCHAR *value);
#ifdef UNICODE
   void insertMBString(int pos, const char *value);
#else
   void insertMBString(int pos, const char *value) { insert(pos, value); }
#endif
   void insertAll(int pos, const StringList *src);
   void insertAll(int pos, const StringList& src) { insertAll(pos, &src); }

   void addAllFromMessage(const NXCPMessage& msg, uint32_t baseId, uint32_t countId);
   void addAllFromMessage(const NXCPMessage& msg, uint32_t fieldId);

   void merge(const StringList& src, bool matchCase);
   void splitAndAdd(const TCHAR *src, const TCHAR *separator);

   void remove(int index);
   void clear();

   void sort(bool ascending = true, bool caseSensitive = false);

   int size() const { return m_count; }
   bool isEmpty() const { return m_count == 0; }
   const TCHAR *get(int index) const { return ((index >=0) && (index < m_count)) ? m_values[index] : nullptr; }
   int indexOf(const TCHAR *value) const;
   bool contains(const TCHAR *value) const { return indexOf(value) != -1; }
   int indexOfIgnoreCase(const TCHAR *value) const;
   bool containsIgnoreCase(const TCHAR *value) const { return indexOfIgnoreCase(value) != -1; }
   TCHAR *join(const TCHAR *separator) const;

   void fillMessage(NXCPMessage *msg, uint32_t baseId, uint32_t countId) const;
   json_t *toJson() const;
};

/**
 * Size of internal buffer for String class
 */
#define STRING_INTERNAL_BUFFER_SIZE 64

/**
 * Immutable string
 */
class LIBNETXMS_EXPORTABLE String
{
protected:
   TCHAR *m_buffer;
   size_t m_length;
   TCHAR m_internalBuffer[STRING_INTERNAL_BUFFER_SIZE];

   bool isInternalBuffer() { return m_buffer == m_internalBuffer; }

   bool fuzzyEqualsImpl(const String& s, double threshold, bool ignoreCase) const;

public:
   static const ssize_t npos;
   static const String empty;

   String();
   String(const TCHAR *init);
   String(const TCHAR *init, ssize_t len, Ownership takeOwnership = Ownership::False);
   String(const char *init, const char *codepage);
   String(const String& src);
   String(String&& src);
   virtual ~String();

   operator const TCHAR*() const { return m_buffer; }
   const TCHAR *cstr() const { return m_buffer; }

   bool operator ==(const String &s) const { return equals(s); }
   bool operator !=(const String &s) const { return !equals(s); }

   String& operator =(const String &src) = delete;

   String operator +(const String &right) const;
   String operator +(const TCHAR *right) const;

	char *getUTF8String() const;

	size_t length() const { return m_length; }
	bool isEmpty() const { return m_length == 0; }
	bool isBlank() const;

   ssize_t find(const TCHAR *str, size_t start = 0) const;
   ssize_t findIgnoreCase(const TCHAR *str, size_t start = 0) const;

	bool equals(const String& s) const;
	bool equals(const TCHAR *s) const;
   bool equalsIgnoreCase(const String& s) const;
   bool equalsIgnoreCase(const TCHAR *s) const;
   bool fuzzyEquals(const String& s, double threshold) const
   {
      return fuzzyEqualsImpl(s, threshold, false);
   }
   bool fuzzyEquals(const TCHAR *s, double threshold) const
   {
      return (s != nullptr) ? fuzzyEqualsImpl(String(s), threshold, false) : false;
   }
   bool fuzzyEqualsIgnoreCase(const String& s, double threshold) const
   {
      return fuzzyEqualsImpl(s, threshold, true);
   }
   bool fuzzyEqualsIgnoreCase(const TCHAR *s, double threshold) const
   {
      return (s != nullptr) ? fuzzyEqualsImpl(String(s), threshold, true) : false;
   }
   bool startsWith(const String& s) const;
   bool startsWith(const TCHAR *s) const;
   bool endsWith(const String& s) const;
   bool endsWith(const TCHAR *s) const;
   bool contains(const String& s) const { return find(s.m_buffer, 0) != npos; }
   bool contains(const TCHAR *s) const { return find(s, 0) != npos; }
   bool containsIgnoreCase(const String& s) const { return findIgnoreCase(s.m_buffer, 0) != npos; }
   bool containsIgnoreCase(const TCHAR *s) const { return findIgnoreCase(s, 0) != npos; }

	TCHAR charAt(size_t pos) const { return (pos < m_length) ? m_buffer[pos] : 0; }

   String substring(size_t start, ssize_t len) const;
	TCHAR *substring(size_t start, ssize_t len, TCHAR *buffer) const;
	String left(size_t len) const { return substring(0, static_cast<ssize_t>(len)); }
   String right(size_t len) const { return substring((m_length > len) ? m_length - len : 0, static_cast<ssize_t>(len)); }

   StringList split(const TCHAR *separator) const { return String::split(m_buffer, m_length, separator); }
   StringList split(const TCHAR *separator, bool trim) const { return String::split(m_buffer, m_length, separator, trim); }
   static StringList split(TCHAR *str, const TCHAR *separator, bool trim = false) { return String::split(str, _tcslen(str), separator, trim); }
   static StringList split(TCHAR *str, size_t len, const TCHAR *separator, bool trim = false);

   void split(const TCHAR *separator, bool trim, std::function<void (const String&)> callback) const
   {
      if (m_length > 0)
         String::split(m_buffer, m_length, separator, trim, callback);
   }
   static void split(const TCHAR *str, const TCHAR *separator, bool trim, std::function<void (const String&)> callback) { String::split(str, _tcslen(str), separator, trim, callback); }
   static void split(const TCHAR *str, size_t len, const TCHAR *separator, bool trim, std::function<void (const String&)> callback);

   static String toString(int32_t v, const TCHAR *format = nullptr);
   static String toString(uint32_t v, const TCHAR *format = nullptr);
   static String toString(int64_t v, const TCHAR *format = nullptr);
   static String toString(uint64_t v, const TCHAR *format = nullptr);
   static String toString(double v, const TCHAR *format = nullptr);
   static String toString(const BYTE *v, size_t len);
};

/**
 * Mutable string - value can be re-assigned
 */
class LIBNETXMS_EXPORTABLE MutableString : public String
{
public:
   MutableString() : String() { }
   MutableString(const TCHAR *init) : String(init) { }
   MutableString(const TCHAR *init, size_t len) : String(init, len) { }
   MutableString(const char *init, const char *codepage) : String(init, codepage) { }
   MutableString(const String& src) : String(src) { }
   MutableString(const MutableString& src) : String(src) { }
   MutableString(MutableString&& src) : String(src) { }

   MutableString& operator =(const String &src);
   MutableString& operator =(const MutableString &src);
   MutableString& operator =(const TCHAR *src);
};

#ifdef _WIN32
template class LIBNETXMS_TEMPLATE_EXPORTABLE shared_ptr<String>;
#endif

/**
 * Shared string
 */
class LIBNETXMS_EXPORTABLE SharedString
{
private:
   shared_ptr<String> m_string;

public:
   SharedString() { }
   SharedString(const SharedString& str) : m_string(str.m_string) { }
   SharedString(const String& str) : m_string(make_shared<String>(str)) { }
   SharedString(const TCHAR *str) { if (str != nullptr) m_string = make_shared<String>(str); }
   SharedString(TCHAR *str, Ownership takeOwnership) { if (str != nullptr) m_string = make_shared<String>(str, -1, takeOwnership); }
   SharedString(const char *str, const char *codepage) { if (str != nullptr) m_string = make_shared<String>(str, codepage); }

   SharedString& operator=(const SharedString &str)
   {
      if (&str != this)
         m_string = str.m_string;
      return *this;
   }
   SharedString& operator=(const String &str)
   {
      m_string = make_shared<String>(str);
      return *this;
   }
   SharedString& operator=(const TCHAR *str)
   {
      if (str != nullptr)
         m_string =  make_shared<String>(str);
      else
         m_string.reset();
      return *this;
   }

   const String &str() const
   {
      const String *s = m_string.get();
      return (s != nullptr) ? *s : String::empty;
   }
   const TCHAR *cstr() const
   {
      const String *s = m_string.get();
      return (s != nullptr) ? s->cstr() : _T("");
   }
   operator const TCHAR*() const { return cstr(); }
   operator const String&() const { return str(); }

   bool isNull() const
   {
      return m_string.get() == nullptr;
   }
   bool isEmpty() const
   {
      const String *s = m_string.get();
      return (s != nullptr) ? s->isEmpty() : true;
   }
   bool isBlank() const
   {
      const String *s = m_string.get();
      return (s != nullptr) ? s->isBlank() : true;
   }
   size_t length() const { return m_string->length(); }
};

/**
 * Dynamic string
 */
class LIBNETXMS_EXPORTABLE StringBuffer : public String
{
protected:
   size_t m_allocated;
   size_t m_allocationStep;

   void insertPlaceholder(size_t index, size_t len);

public:
   StringBuffer();
   StringBuffer(const TCHAR *init);
   StringBuffer(const TCHAR *init, size_t length);
   StringBuffer(const char *init, const char *codepage);
   StringBuffer(const StringBuffer& src);
   StringBuffer(StringBuffer&& src);
   StringBuffer(const String& src);
   StringBuffer(const SharedString& src);
   virtual ~StringBuffer();

   size_t getAllocationStep() const { return m_allocationStep; }
   void setAllocationStep(size_t step) { m_allocationStep = step; }

   TCHAR *getBuffer() { return m_buffer; }
   void setBuffer(TCHAR *buffer);
   TCHAR *takeBuffer()
   {
      TCHAR *b = isInternalBuffer() ? MemCopyString(m_internalBuffer) : m_buffer;
      m_buffer = m_internalBuffer;
      m_allocated = 0;
      m_length = 0;
      m_buffer[0] = 0;
      return b;
   }

   StringBuffer& operator =(const TCHAR *str);
   StringBuffer& operator =(const StringBuffer &src);
   StringBuffer& operator =(const String &src);
   StringBuffer& operator =(const SharedString &src);

   StringBuffer& operator +=(const TCHAR *str) { append(str); return *this; }
   StringBuffer& operator +=(const String &str) { append(str.cstr(), str.length()); return *this; }
   StringBuffer& operator +=(const SharedString &str) { append(str.cstr(), str.length()); return *this; }

   StringBuffer& append(const String& str) { insert(m_length, str.cstr(), str.length()); return *this; }
   StringBuffer& append(const TCHAR *str) { if (str != nullptr) append(str, _tcslen(str)); return *this; }
   StringBuffer& append(const TCHAR *str, size_t len) { insert(m_length, str, len); return *this; }
   StringBuffer& append(const TCHAR c) { return append(&c, 1); }
   StringBuffer& append(bool b) { return b ? append(_T("true"), 4) : append(_T("false"), 5); }
   StringBuffer& append(int32_t n, const TCHAR *format = nullptr) { insert(m_length, n, format); return *this; }
   StringBuffer& append(uint32_t n, const TCHAR *format = nullptr) { insert(m_length, n, format); return *this; }
   StringBuffer& append(int64_t n, const TCHAR *format = nullptr) { insert(m_length, n, format); return *this; }
   StringBuffer& append(uint64_t n, const TCHAR *format = nullptr) { insert(m_length, n, format); return *this; }
   StringBuffer& append(double d, const TCHAR *format = nullptr) { insert(m_length, d, format); return *this; }
   StringBuffer& append(const uuid& guid) { insert(m_length, guid); return *this; }
   StringBuffer& append(Timestamp t) { insert(m_length, t); return *this; }

   StringBuffer& appendPreallocated(TCHAR *str)
   {
      if (str != nullptr)
      {
         append(str);
         MemFree(str);
      }
      return *this;
   }

   StringBuffer& appendMBString(const char *str, ssize_t len = -1) { insertMBString(m_length, str, len); return *this; }
   StringBuffer& appendUtf8String(const char *str, ssize_t len = -1) { insertUtf8String(m_length, str, len); return *this; }
   StringBuffer& appendWideString(const WCHAR *str, ssize_t len = -1) { insertWideString(m_length, str, len); return *this; }

   StringBuffer& appendFormattedString(const TCHAR *format, ...);
   StringBuffer& appendFormattedStringV(const TCHAR *format, va_list args) { insertFormattedStringV(m_length, format, args); return *this; }

   StringBuffer& appendAsHexString(const void *data, size_t len, TCHAR separator = 0) { insertAsHexString(m_length, data, len, separator); return *this; }

   void insert(size_t index, const String& str)  { insert(index, str.cstr(), str.length()); }
   void insert(size_t index, const TCHAR *str)  { if (str != nullptr) insert(index, str, _tcslen(str)); }
   void insert(size_t index, const TCHAR *str, size_t len);
   void insert(size_t index, const TCHAR c) { insert(index, &c, 1); }
   void insert(size_t index, int32_t n, const TCHAR *format = nullptr);
   void insert(size_t index, uint32_t n, const TCHAR *format = nullptr);
   void insert(size_t index, int64_t n, const TCHAR *format = nullptr);
   void insert(size_t index, uint64_t n, const TCHAR *format = nullptr);
   void insert(size_t index, double d, const TCHAR *format = nullptr);
   void insert(size_t index, const uuid& guid);
   void insert(size_t index, Timestamp t) { insert(index, t.asMilliseconds()); }

   void insertPreallocated(size_t index, TCHAR *str) { if (str != nullptr) { insert(index, str); MemFree(str); } }

   void insertMBString(size_t index, const char *str, ssize_t len = -1);
   void insertUtf8String(size_t index, const char *str, ssize_t len = -1);
   void insertWideString(size_t index, const WCHAR *str, ssize_t len = -1);

   void insertFormattedString(size_t index, const TCHAR *format, ...);
   void insertFormattedStringV(size_t index, const TCHAR *format, va_list args);

   void insertAsHexString(size_t index, const void *data, size_t len, TCHAR separator = 0);

   void clear(bool releaseBuffer = true);

   StringBuffer& escapeCharacter(int ch, int esc);
   StringBuffer& replace(const TCHAR *src, const TCHAR *dst);
   StringBuffer& trim();
   StringBuffer& shrink(size_t chars = 1);
   StringBuffer& removeRange(size_t start, ssize_t len = -1);

   StringBuffer& toUppercase();
   StringBuffer& toLowercase();
};

/**
 * String container with configurable internal buffer size
 */
template<size_t BufferSize> class StringContainer
{
private:
   TCHAR *m_value;
   TCHAR m_buffer[BufferSize];

public:
   StringContainer()
   {
      m_value = m_buffer;
      m_buffer[0] = 0;
   }

   StringContainer(const TCHAR *src)
   {
      if (src != nullptr)
      {
         size_t len = _tcslen(src);
         if (len >= BufferSize)
         {
            m_value = MemCopyBlock(src, (len + 1) * sizeof(TCHAR));
         }
         else
         {
            m_value = m_buffer;
            memcpy(m_buffer, src, (len + 1) * sizeof(TCHAR));
         }
      }
      else
      {
         m_value = m_buffer;
         m_buffer[0] = 0;
      }
   }

   StringContainer(const String& src)
   {
      if (src.length() >= BufferSize)
      {
         m_value = MemCopyBlock(src.cstr(), (src.length() + 1) * sizeof(TCHAR));
      }
      else
      {
         m_value = m_buffer;
         memcpy(m_buffer, src.cstr(), (src.length() + 1) * sizeof(TCHAR));
      }
   }

   StringContainer(const StringContainer& src)
   {
      if (src.m_value != src.m_buffer)
      {
         m_value = MemCopyString(src.m_value);
      }
      else
      {
         m_value = m_buffer;
         memcpy(m_buffer, src.m_buffer, BufferSize * sizeof(TCHAR));
      }
   }

   StringContainer(StringContainer&& src)
   {
      if (src.m_value != src.m_buffer)
      {
         m_value = src.m_value;
         src.m_value = src.m_buffer;
         src.m_buffer[0] = 0;
      }
      else
      {
         m_value = m_buffer;
         memcpy(m_buffer, src.m_buffer, BufferSize * sizeof(TCHAR));
      }
   }

   ~StringContainer()
   {
      if (m_value != m_buffer)
         MemFree(m_value);
   }

   StringContainer& operator =(const TCHAR *src)
   {
      if (m_value != m_buffer)
         MemFree(m_value);

      if (src != nullptr)
      {
         size_t len = _tcslen(src);
         if (len >= BufferSize)
         {
            m_value = MemCopyBlock(src, (len + 1) * sizeof(TCHAR));
         }
         else
         {
            m_value = m_buffer;
            memcpy(m_buffer, src, (len + 1) * sizeof(TCHAR));
         }
      }
      else
      {
         m_value = m_buffer;
         m_buffer[0] = 0;
      }
      return *this;
   }

   StringContainer& operator =(const String& src)
   {
      if (m_value != m_buffer)
         MemFree(m_value);

      if (src.length() >= BufferSize)
      {
         m_value = MemCopyBlock(src.cstr(), (src.length() + 1) * sizeof(TCHAR));
      }
      else
      {
         m_value = m_buffer;
         memcpy(m_buffer, src.cstr(), (src.length() + 1) * sizeof(TCHAR));
      }
      return *this;
   }

   StringContainer& operator =(const StringContainer& src)
   {
      if (m_value != m_buffer)
         MemFree(m_value);

      if (src.m_value != src.m_buffer)
      {
         m_value = MemCopyString(src.m_value);
      }
      else
      {
         m_value = m_buffer;
         memcpy(m_buffer, src.m_buffer, BufferSize * sizeof(TCHAR));
      }
      return *this;
   }

   StringContainer& operator =(StringContainer&& src)
   {
      if (m_value != m_buffer)
         MemFree(m_value);

      if (src.m_value != src.m_buffer)
      {
         m_value = src.m_value;
         src.m_value = src.m_buffer;
         src.m_buffer[0] = 0;
      }
      else
      {
         m_value = m_buffer;
         memcpy(m_buffer, src.m_buffer, BufferSize * sizeof(TCHAR));
      }
      return *this;
   }

   void reset()
   {
      if (m_value != m_buffer)
      {
         MemFree(m_value);
         m_value = m_buffer;
      }
      m_value[0] = 0;
   }

   operator const TCHAR*() const { return m_value; }
   const TCHAR *cstr() const { return m_value; }
   bool isEmpty() const { return m_value[0] == 0; }
};

/**
 * Text file writer that uses StringBuffer interface
 */
class LIBNETXMS_EXPORTABLE TextFileWriter
{
private:
   FILE *m_handle;

public:
   TextFileWriter(FILE *handle)
   {
      m_handle = handle;
   }
   ~TextFileWriter()
   {
      if (m_handle != nullptr)
         fclose(m_handle);
   }

   void close()
   {
      if (m_handle != nullptr)
      {
         fclose(m_handle);
         m_handle = nullptr;
      }
   }

   TextFileWriter& appendMBString(const char *str, ssize_t len = -1);
   TextFileWriter& appendUtf8String(const char *str, ssize_t len = -1);
   TextFileWriter& appendWideString(const wchar_t *str, ssize_t len = -1);

   TextFileWriter& append(const TCHAR *str, size_t len)
   {
#ifdef UNICODE
      return appendWideString(str, len);
#else
      return appendMBString(str, len);
#endif
   }
   TextFileWriter& append(const TCHAR *str)
   {
      if (str != nullptr)
         append(str, _tcslen(str));
      return *this;
   }
   TextFileWriter& append(const String& str)
   {
      append(str.cstr(), str.length());
      return *this;
   }
   TextFileWriter& append(const TCHAR c)
   {
      return append(&c, 1);
   }
   TextFileWriter& append(bool b)
   {
      return b ? appendUtf8String("true", 4) : appendUtf8String("false", 5);
   }
   TextFileWriter& append(int32_t n, const TCHAR *format = nullptr);
   TextFileWriter& append(uint32_t n, const TCHAR *format = nullptr);
   TextFileWriter& append(int64_t n, const TCHAR *format = nullptr);
   TextFileWriter& append(uint64_t n, const TCHAR *format = nullptr);
   TextFileWriter& append(double d, const TCHAR *format = nullptr);
   TextFileWriter& append(const uuid& guid);

   TextFileWriter& appendPreallocated(TCHAR *str)
   {
      if (str != nullptr)
      {
         append(str);
         MemFree(str);
      }
      return *this;
   }

   TextFileWriter& appendFormattedString(const TCHAR *format, ...);
   TextFileWriter& appendFormattedStringV(const TCHAR *format, va_list args);

   TextFileWriter& appendAsHexString(const void *data, size_t len, char separator = 0);
   TextFileWriter& appendAsBase64String(const void *data, size_t len);

   TextFileWriter& operator +=(const TCHAR *str)
   {
      append(str);
      return *this;
   }
   TextFileWriter& operator +=(const String &str)
   {
      append(str.cstr(), str.length());
      return *this;
   }
   TextFileWriter& operator +=(const SharedString &str)
   {
      append(str.cstr(), str.length());
      return *this;
   }
};

/**
 * Abstract const iterator class (to be implemented by actual iterable class)
 */
class LIBNETXMS_EXPORTABLE AbstractIterator
{
private:
   int m_refCount;

protected:
   AbstractIterator()
   {
      m_refCount = 1;
   }

public:
   virtual ~AbstractIterator() { }

   void incRefCount() { m_refCount++; }
   void decRefCount()
   {
      m_refCount--;
      if (m_refCount == 0)
         delete this;
   }

   virtual bool hasNext() = 0;
   virtual void *next() = 0;
   virtual void *value() = 0;
   virtual bool equals(AbstractIterator* other) = 0;
   virtual void remove() = 0;
   virtual void unlink() = 0;
};

/**
 * Const iterator class (public interface for iteration over const object)
 */
template<class T> class ConstIterator
{
protected:
   AbstractIterator *m_worker;

public:
   ConstIterator(AbstractIterator *worker) { m_worker = worker; }
   ConstIterator(const ConstIterator& other)
   {
      m_worker = other.m_worker;
      m_worker->incRefCount();
   }
   ~ConstIterator()
   {
      m_worker->decRefCount();
   }

   bool hasNext() { return m_worker->hasNext(); }
   T *next() { return static_cast<T*>(m_worker->next()); }

   T *value() { return static_cast<T*>(m_worker->value()); }
   T* operator* () { return static_cast<T*>(m_worker->value()); }

   ConstIterator& operator=(const ConstIterator& other)
   {
      m_worker->decRefCount(); 
      m_worker = other.m_worker;
      m_worker->incRefCount();
      return *this; 
   }

   bool operator==(const ConstIterator& other) { return m_worker->equals(other.m_worker); }
   bool operator!=(const ConstIterator& other) { return !m_worker->equals(other.m_worker); }
   ConstIterator& operator++()   // Prefix increment operator.
   {
      m_worker->next();
      return *this;
   }
   ConstIterator operator++(int) // Postfix increment operator.
   {  
      ConstIterator temp = *this;
      m_worker->next();
      return temp;
   }
};

/**
 * Iterator class (public interface for iteration)
 */
template<class T> class Iterator : public ConstIterator<T>
{
public:
   Iterator(AbstractIterator *worker) : ConstIterator<T>(worker) { }
   Iterator(const Iterator& other) : ConstIterator<T>(other) { }
   Iterator& operator=(const Iterator& other)
   {
      ConstIterator<T>::operator = (other);
      return *this;
   }

   void remove() { this->m_worker->remove(); }
   void unlink() { this->m_worker->unlink(); }
};

/**
 * Iterator class (public interface for iteration)
 */
template<class T> class SharedPtrIterator : public Iterator<T>
{
   static shared_ptr<T> m_null;

public:
   SharedPtrIterator(AbstractIterator *worker) : Iterator<T>(worker) { }
   SharedPtrIterator(const SharedPtrIterator& other) : Iterator<T>(other) { }
   SharedPtrIterator& operator=(const SharedPtrIterator& other)
   {
      Iterator<T>::operator = (other);
      return *this;
   }

   const shared_ptr<T>& next()
   {
      auto p = static_cast<shared_ptr<T>*>(this->m_worker->next());
      return (p != nullptr) ? *p : m_null;
   }
   const shared_ptr<T>& value()
   {
      auto p = static_cast<shared_ptr<T>*>(this->m_worker->value());
      return (p != nullptr) ? *p : m_null;
   }
   const shared_ptr<T>& operator* ()
   {
      auto p = static_cast<shared_ptr<T>*>(this->m_worker->value());
      return (p != nullptr) ? *p : m_null;
   }
};

template<class T> shared_ptr<T> SharedPtrIterator<T>::m_null = shared_ptr<T>();

/**
 * Dynamic array class
 */
class LIBNETXMS_EXPORTABLE Array
{
private:
	int m_size;
	int m_allocated;
	int m_grow;
   size_t m_elementSize;
	void **m_data;
	bool m_objectOwner;
	void *m_context;

	bool internalRemove(int index, bool allowDestruction);
	void destroyObject(void *object) { if (object != nullptr) m_objectDestructor(object, this); }

protected:
   bool m_storePointers;
	void (*m_objectDestructor)(void *, Array *);

   Array(const void *data, int initial, int grow, size_t elementSize);
   Array(const Array& src);
   Array(Array&& src);

   void *__getBuffer() const { return m_data; }

public:
	Array(int initial = 0, int grow = 16, Ownership owner = Ownership::False, void (*objectDestructor)(void *, Array *) = nullptr);
	virtual ~Array();

   void *get(int index) const { return ((index >= 0) && (index < m_size)) ? (m_storePointers ? m_data[index] : (void *)((char *)m_data + index * m_elementSize)) : nullptr; }
   void *first() const { return get(0); }
   void *last() const { return get(m_size - 1); }
   int indexOf(void *element) const;
   void *bsearch(const void *key, int (*cb)(const void *, const void *)) const;
   void *find(std::function<bool (const void*)> comparator) const;

	int add(void *element);
	void addAll(const Array& src);
	void addAll(const Array *src) { if (src != nullptr) addAll(*src); }
	void set(int index, void *element);
	void replace(int index, void *element);
   void insert(int index, void *element);
	bool remove(int index) { return internalRemove(index, true); }
   bool remove(void *element) { return internalRemove(indexOf(element), true); }
	bool unlink(int index) { return internalRemove(index, false); }
	bool unlink(void *element) { return internalRemove(indexOf(element), false); }
   void clear();
	void shrinkTo(int size);
	void shrinkBy(int count) { if (count >= m_size) clear(); else shrinkTo(m_size - count); }
   void sort(int (*cb)(const void *, const void *));
   void sort(int (*cb)(void *, const void *, const void *), void *context);
   void swap(int index1, int index2);

   void swap(Array& other);

   void *addPlaceholder();
   void *replaceWithPlaceholder(int index);

	int size() const { return m_size; }
	bool isEmpty() const { return m_size == 0; }
	int sizeIncrement() const { return m_grow; }

	size_t memoryUsage() const { return m_allocated * m_elementSize; }

	void setOwner(Ownership owner) { m_objectOwner = (owner == Ownership::True); }
	bool isOwner() const { return m_objectOwner; }

	void setContext(void *context) { m_context = context; }
	void *getContext() const { return m_context; }
};

/**
 * Array iterator class
 */
class LIBNETXMS_EXPORTABLE ArrayIterator : public AbstractIterator
{
private:
   Array *m_array;
   int m_pos;

public:
   ArrayIterator(Array *array, int pos = -1);

   virtual bool hasNext() override;
   virtual void *next() override;
   virtual void remove() override;
   virtual void unlink() override;
   virtual void *value() override;
   virtual bool equals(AbstractIterator* other) override;
};

/**
 * Template class for dynamic array which holds objects
 */
template <class T> class ObjectArray : public Array
{
private:
	static void destructor(void *object, Array *array) { delete static_cast<T*>(object); }

public:
	ObjectArray(int initial = 0, int grow = 16, Ownership owner = Ownership::False) : Array(initial, grow, owner) { m_objectDestructor = destructor; }
	ObjectArray(ObjectArray&& src) : Array(std::move(src)) { }
   ObjectArray(const ObjectArray& src) = delete;
	virtual ~ObjectArray() { }

	int add(T *object) { return Array::add((void *)object); }
	T *get(int index) const { return (T*)Array::get(index); }
	T *first() const { return get(0); }
	T *last() const { return get(size() - 1); }
   int indexOf(T *object) const { return Array::indexOf((void *)object); }
   bool contains(T *object) const { return indexOf(object) >= 0; }
	void set(int index, T *object) { Array::set(index, (void *)object); }
	void replace(int index, T *object) { Array::replace(index, (void *)object); }
   void insert(int index, T *object) { Array::insert(index, (void *)object); }
	bool remove(int index) { return Array::remove(index); }
   bool remove(T *object) { return Array::remove((void *)object); }
	bool unlink(int index) { return Array::unlink(index); }
   bool unlink(T *object) { return Array::unlink((void *)object); }

   using Array::sort;
   void sort(int (*cb)(const T **, const T **)) { Array::sort((int (*)(const void *, const void *))cb); }
   template<typename C> void sort(int (*cb)(C *, const T **, const T **), C *context) { Array::sort((int (*)(void *, const void *, const void *))cb, (void *)context); }
   T *bsearch(const T *key, int (*cb)(const T **, const T **)) const
   {
      T **result = (T **)Array::bsearch(&key, (int (*)(const void *, const void *))cb);
      return (result != nullptr) ? *result : nullptr;
   }

   const T * const *getBuffer() const { return static_cast<const T * const *>(__getBuffer()); }

   Iterator<T> begin()
   {
      return Iterator<T>(new ArrayIterator(this));
   }

   ConstIterator<T> begin() const
   {
      return ConstIterator<T>(new ArrayIterator(const_cast<ObjectArray<T>*>(this)));
   }

   Iterator<T> end()
   {
      return Iterator<T>(new ArrayIterator(this, size() - 1));
   }

   ConstIterator<T> end() const
   {
      return ConstIterator<T>(new ArrayIterator(const_cast<ObjectArray<T>*>(this), size() - 1));
   }
};

/**
 * Template class for dynamic array which holds references to objects with inaccessible destructors
 */
template <class T> class ObjectRefArray : public Array
{
private:
	static void destructor(void *object, Array *array) { }

public:
	ObjectRefArray(int initial = 0, int grow = 16) : Array(initial, grow, Ownership::False) { m_objectDestructor = destructor; }
	ObjectRefArray(ObjectRefArray&& src) : Array(std::move(src)) { }
   ObjectRefArray(const ObjectRefArray& src) = delete;
	virtual ~ObjectRefArray() { }

	int add(T *object) { return Array::add((void *)object); }
	T *get(int index) const { return (T*)Array::get(index); }
   int indexOf(T *object) const { return Array::indexOf((void *)object); }
   bool contains(T *object) const { return indexOf(object) >= 0; }
	void set(int index, T *object) { Array::set(index, (void *)object); }
	void replace(int index, T *object) { Array::replace(index, (void *)object); }
	void remove(int index) { Array::remove(index); }
   void remove(T *object) { Array::remove((void *)object); }
	void unlink(int index) { Array::unlink(index); }
   void unlink(T *object) { Array::unlink((void *)object); }

   Iterator<T> begin()
   {
      return Iterator<T>(new ArrayIterator(this));
   }

   ConstIterator<T> begin() const
   {
      return ConstIterator<T>(new ArrayIterator(const_cast<ObjectArray<T>*>(this)));
   }

   Iterator<T> end()
   {
      return Iterator<T>(new ArrayIterator(this, size() - 1));
   }

   ConstIterator<T> end() const
   {
      return ConstIterator<T>(new ArrayIterator(const_cast<ObjectArray<T>*>(this), size() - 1));
   }
};

/**
 * Template class for dynamic array which holds scalar values
 */
template <class T> class IntegerArray : public Array
{
private:
	static void destructor(void *element, Array *array) { }

	static int ascendingComparator(const T *e1, const T *e2)
	{
	   return (*e1 < *e2) ? -1 : ((*e1 == *e2) ? 0 : 1);
	}

	static int descendingComparator(const T *e1, const T *e2)
   {
      return (*e1 > *e2) ? -1 : ((*e1 == *e2) ? 0 : 1);
   }

public:
	IntegerArray(int initial = 0, int grow = 1024 / sizeof(T)) : Array(nullptr, initial, grow, sizeof(T)) { m_objectDestructor = destructor; m_storePointers = (sizeof(T) == sizeof(void *)); }
   IntegerArray(const IntegerArray<T>& src) : Array(src) { }
   IntegerArray(IntegerArray<T>&& src) : Array(std::move(src)) { }
	virtual ~IntegerArray() { }

   int add(T value) { return Array::add(m_storePointers ? CAST_TO_POINTER(value, void *) : &value); }
   void addAll(const IntegerArray<T>& src) { Array::addAll(src); }
   void addAll(const IntegerArray<T>* src) { Array::addAll(src); }
   T get(int index) const { if (m_storePointers) return CAST_FROM_POINTER(Array::get(index), T); T *p = (T*)Array::get(index); return (p != NULL) ? *p : 0; }
   int indexOf(T value) const { return Array::indexOf(m_storePointers ? CAST_TO_POINTER(value, void *) : &value); }
   bool contains(T value) const { return indexOf(value) >= 0; }
   void set(int index, T value) { Array::set(index, m_storePointers ? CAST_TO_POINTER(value, void *) : &value); }
   void replace(int index, T value) { Array::replace(index, m_storePointers ? CAST_TO_POINTER(value, void *) : &value); }

   using Array::sort;
   void sort(int (*cb)(const T *, const T *)) { Array::sort((int (*)(const void *, const void *))cb); }
   template<typename C> void sort(int (*cb)(C *, const T *, const T *), C *context) { Array::sort((int (*)(void *, const void *, const void *))cb, (void *)context); }
   void sortAscending() { sort(ascendingComparator); }
   void sortDescending() { sort(descendingComparator); }
   void deduplicate()
   {
      for(int i = 0; i < size() - 1; i++)
      {
         for(int j = i+1; j < size(); j++)
         {
            if (get(i) == get(j))
            {
               remove(j);
               j--;
            }
         }
      }
   }

   T *getBuffer() const { return (T*)__getBuffer(); }

   json_t *toJson() const { json_t *a = json_array(); for(int i = 0; i < size(); i++) json_array_append_new(a, json_integer(get(i))); return a; }

   bool equals(const IntegerArray<T>& other) const
   {
      if (other.size() != size())
         return false;
      for(int i = 0; i < size(); i++)
         if (get(i) != other.get(i))
            return false;
      return true;
   }
};

#ifdef _WIN32
// Define DLL interfaces for common integer types in libnetxms
template class LIBNETXMS_TEMPLATE_EXPORTABLE IntegerArray<int32_t>;
template class LIBNETXMS_TEMPLATE_EXPORTABLE IntegerArray<uint32_t>;
template class LIBNETXMS_TEMPLATE_EXPORTABLE IntegerArray<int64_t>;
template class LIBNETXMS_TEMPLATE_EXPORTABLE IntegerArray<uint64_t>;
#endif

/**
 * Auxilliary class to hold dynamically allocated array of structures
 */
template <class T> class StructArray : public Array
{
private:
	static void destructor(void *element, Array *array) { }

public:
	StructArray(int initial = 0, int grow = 16) : Array(nullptr, initial, grow, sizeof(T)) { m_objectDestructor = destructor; }
	StructArray(const T *data, int size, int grow = 16) : Array(data, size, grow, sizeof(T)) { m_objectDestructor = destructor; }
   StructArray(const StructArray<T>& src) : Array(src) { }
	StructArray(StructArray<T>&& src) : Array(std::move(src)) { }
	virtual ~StructArray() { }

	int add(const T *element) { return Array::add((void *)element); }
   int add(const T &element) { return Array::add((void *)&element); }
   T *addPlaceholder() { return (T*)Array::addPlaceholder(); }
	T *get(int index) const { return (T*)Array::get(index); }
   int indexOf(const T *element) const { return Array::indexOf((void *)element); }
   int indexOf(const T &element) const { return Array::indexOf((void *)&element); }
   bool contains(const T *element) const { return indexOf(element) >= 0; }
   bool contains(const T &element) const { return indexOf(element) >= 0; }
	void set(int index, const T *element) { Array::set(index, (void *)element); }
   void set(int index, const T &element) { Array::set(index, (void *)&element); }
	void replace(int index, const T *element) { Array::replace(index, (void *)element); }
   void replace(int index, const T &element) { Array::replace(index, (void *)&element); }
	void remove(int index) { Array::remove(index); }
   void remove(const T *element) { Array::remove((void *)element); }
   void remove(const T &element) { Array::remove((void *)&element); }
	void unlink(int index) { Array::unlink(index); }
   void unlink(const T *element) { Array::unlink((void *)element); }
   void unlink(const T &element) { Array::unlink((void *)&element); }
   T *find(std::function<bool (const T*)> comparator) const
   {
      return const_cast<T*>(static_cast<const T*>(Array::find(
         [comparator](const void* element) { return comparator(static_cast<const T*>(element)); }
      )));
   }

   using Array::sort;
   void sort(int (*cb)(const T*, const T*)) { Array::sort((int (*)(const void*, const void*))cb); }

   T *getBuffer() const { return (T*)__getBuffer(); }

   Iterator<T> begin()
   {
      return Iterator<T>(new ArrayIterator(this));
   }

   ConstIterator<T> begin() const
   {
      return ConstIterator<T>(new ArrayIterator(const_cast<StructArray<T>*>(this)));
   }

   Iterator<T> end()
   {
      return Iterator<T>(new ArrayIterator(this, size() - 1));
   }

   ConstIterator<T> end() const
   {
      return ConstIterator<T>(new ArrayIterator(const_cast<StructArray<T>*>(this), size() - 1));
   }
};

/**
 * Shared object array
 */
template <class T> class SharedObjectArray final
{
   // Note: member order is important because m_data allocates memory from m_pool
private:
   ObjectMemoryPool<shared_ptr<T>> m_pool;
   Array m_data;

   static shared_ptr<T> m_null;

   static void destructor(void *element, Array *array)
   {
      static_cast<SharedObjectArray<T>*>(array->getContext())->m_pool.destroy(static_cast<shared_ptr<T>*>(element));
   }

   static int sortCallback(void *context, const shared_ptr<T> **e1, const shared_ptr<T> **e2)
   {
      int (*cb)(const T&, const T&) = reinterpret_cast<int (*)(const T&, const T&)>(context);
      return cb(*((*e1)->get()), *((*e2)->get()));
   }

public:
   SharedObjectArray(int initial = 0, int grow = 16) :
      m_pool(std::max(grow, 64)), m_data(initial, grow, Ownership::True, SharedObjectArray<T>::destructor)
   {
      m_data.setContext(this);
   }
   SharedObjectArray(const SharedObjectArray& src) :
      m_pool(src.m_pool.getRegionCapacity()), m_data(src.m_data.size(), src.m_data.sizeIncrement(), Ownership::True, SharedObjectArray<T>::destructor)
   {
      m_data.setContext(this);
      for(int i = 0; i < src.m_data.size(); i++)
         add(*static_cast<shared_ptr<T>*>(src.m_data.get(i)));
   }

   int add(shared_ptr<T> element) { return m_data.add(new(m_pool.allocate()) shared_ptr<T>(element)); }
   int add(T *element) { return m_data.add(new(m_pool.allocate()) shared_ptr<T>(element)); }
   void addAll(const SharedObjectArray& src)
   {
      for(int i = 0; i < src.m_data.size(); i++)
         add(*static_cast<shared_ptr<T>*>(src.m_data.get(i)));
   }

   T *get(int index) const
   {
      auto p = static_cast<shared_ptr<T>*>(m_data.get(index));
      return (p != nullptr) ? p->get() : nullptr;
   }
   const shared_ptr<T>& getShared(int index) const
   {
      auto p = static_cast<shared_ptr<T>*>(m_data.get(index));
      return (p != nullptr) ? *p : m_null;
   }
   void replace(int index, shared_ptr<T> element)
   {
      auto p = static_cast<shared_ptr<T>**>(m_data.replaceWithPlaceholder(index));
      if (p != nullptr)
         *p = new(m_pool.allocate()) shared_ptr<T>(element);
   }
   void replace(int index, T *element)
   {
      auto p = static_cast<shared_ptr<T>**>(m_data.replaceWithPlaceholder(index));
      if (p != nullptr)
         *p = new(m_pool.allocate()) shared_ptr<T>(element);
   }
   void remove(int index) { m_data.remove(index); }
   void clear() { m_data.clear(); }

   int size() const { return m_data.size(); }
   bool isEmpty() const { return m_data.isEmpty(); }

   void sort(int (*cb)(const T&, const T&)) { m_data.sort(reinterpret_cast<int (*)(void *, const void *, const void *)>(sortCallback), (void *)cb); }

   SharedPtrIterator<T> begin()
   {
      return SharedPtrIterator<T>(new ArrayIterator(&m_data));
   }

   SharedPtrIterator<T> begin() const
   {
      return SharedPtrIterator<T>(new ArrayIterator(const_cast<Array*>(&m_data)));
   }

   SharedPtrIterator<T> end()
   {
      return SharedPtrIterator<T>(new ArrayIterator(&m_data, m_data.size() - 1));
   }

   SharedPtrIterator<T> end() const
   {
      return SharedPtrIterator<T>(new ArrayIterator(const_cast<Array*>(&m_data), m_data.size() - 1));
   }
};

template <class T> shared_ptr<T> SharedObjectArray<T>::m_null = shared_ptr<T>();

/**
 * Helper class that allows to return array and it's size in one object
 */
template<typename T> class ArrayReference
{
private:
   T *m_data;
   size_t m_size;

public:
   ArrayReference(T *data, size_t size)
   {
      m_data = data;
      m_size = size;
   }
   ArrayReference()
   {
      m_data = nullptr;
      m_size = 0;
   }

   T *data() const { return m_data; }
   size_t size() const { return m_size; }
};

/**
 * Entry of string map (internal)
 */
struct StringMapEntry;

/**
 * Key/value pair
 */
template <typename T> struct KeyValuePair
{
   const TCHAR *key;
   T *value;
};

class StringMapBase;

#ifdef _WIN32
template struct LIBNETXMS_EXPORTABLE std::pair<const TCHAR*, const TCHAR*>;
#endif

/**
 * String map iterator
 */
class LIBNETXMS_EXPORTABLE StringMapIterator : public AbstractIterator
{
private:
   StringMapBase *m_map;
   StringMapEntry *m_curr;
   StringMapEntry *m_next;
   KeyValuePair<void> m_element;

public:
   StringMapIterator(StringMapBase *map = nullptr);   //Constructor

   virtual bool hasNext() override;
   virtual void *next() override;
   virtual void remove() override;
   virtual void unlink() override;
   virtual void *value() override;
   virtual bool equals(AbstractIterator* other) override;

   const TCHAR *key();
};

/**
 * String maps base class
 */
class LIBNETXMS_EXPORTABLE StringMapBase
{
   friend class StringMapIterator;

private:
   StringMapEntry *find(const TCHAR *key, size_t keyLen) const;
   void destroyObject(void *object) { if (object != nullptr) m_objectDestructor(object, this); }

protected:
   StringMapEntry *m_data;
   bool m_objectOwner;
   bool m_ignoreCase;
   void (*m_objectDestructor)(void *, StringMapBase *);
   void *m_context;

   void setObject(TCHAR *key, void *value, bool keyPreAlloc);
   void *getObject(const TCHAR *key) const;
   void *getObject(const TCHAR *key, size_t len) const;
   void fillValues(Array *a) const;
   void *unlink(const TCHAR *key);

   ~StringMapBase();

public:
   StringMapBase(Ownership objectOwner, void (*destructor)(void *, StringMapBase *) = nullptr);
   StringMapBase(const StringMapBase& src) = delete;
   StringMapBase(StringMapBase&& src);

   void setOwner(Ownership owner) { m_objectOwner = (owner == Ownership::True); }
   void setIgnoreCase(bool ignore);

   void remove(const TCHAR *key) { remove(key, _tcslen(key)); }
   void remove(const TCHAR *key, size_t keyLen);
   void clear();
   void filterElements(bool (*filter)(const TCHAR *, const void *, void *), void *userData);

   int size() const;
   bool isEmpty() const { return size() == 0; }
   bool contains(const TCHAR *key) const { return (key != nullptr) ? (find(key, _tcslen(key) * sizeof(TCHAR)) != nullptr) : false; }
   bool contains(const TCHAR *key, size_t len) const { return (key != nullptr) ? (find(key, len * sizeof(TCHAR)) != nullptr) : false; }

   EnumerationCallbackResult forEach(EnumerationCallbackResult (*cb)(const TCHAR*, void*, void*), void *userData);
   EnumerationCallbackResult forEach(std::function<EnumerationCallbackResult (const TCHAR*, void*)> cb);
   EnumerationCallbackResult forEach(EnumerationCallbackResult (*cb)(const TCHAR*, const void*, void*), void *userData) const;
   EnumerationCallbackResult forEach(std::function<EnumerationCallbackResult (const TCHAR*, const void*)> cb) const;
   const void *findElement(bool (*comparator)(const TCHAR*, const void*, void*), void *userData) const;
   const void *findElement(std::function<bool (const TCHAR*, const void*)> comparator) const;

   StructArray<KeyValuePair<void>> *toArray(bool (*filter)(const TCHAR *, const void *, void *) = nullptr, void *userData = nullptr) const;
   StringList keys() const;

   void setContext(void *context) { m_context = context; }
   void *getContext() const { return m_context; }
};

/**
 * String map class. Preserves insertion order.
 */
class LIBNETXMS_EXPORTABLE StringMap : public StringMapBase
{
public:
	StringMap() : StringMapBase(Ownership::True) { }
   StringMap(const NXCPMessage& msg, uint32_t baseFieldId, uint32_t sizeFieldId);
   StringMap(json_t *json);
	StringMap(const StringMap& src);
   StringMap(StringMap&& src) : StringMapBase(std::move(src)) { }

	StringMap& operator =(const StringMap &src);

	StringMap& set(const TCHAR *key, const TCHAR *value)
	{
	   if (key != nullptr)
	      setObject(const_cast<TCHAR*>(key), MemCopyString(value), false);
	   return *this;
	}
	StringMap& setPreallocated(TCHAR *key, TCHAR *value)
	{
	   setObject(key, value, true);
	   return *this;
	}
	StringMap& set(const TCHAR *key, int32_t value);
	StringMap& set(const TCHAR *key, uint32_t value);
	StringMap& set(const TCHAR *key, int64_t value);
	StringMap& set(const TCHAR *key, uint64_t value);
	StringMap& setUTF8String(const TCHAR *key, const char *value)
	{
	   setObject(const_cast<TCHAR *>(key), TStringFromUTF8String(value), false);
	   return *this;
	}
	StringMap& setMBString(const TCHAR *key, const char *value)
	{
#ifdef UNICODE
	   setObject(const_cast<TCHAR*>(key), WideStringFromMBString(value), false);
#else
	   set(key, value);
#endif
	   return *this;
	}

   void addAll(const StringMap *src, bool (*filter)(const TCHAR *, const TCHAR *, void *) = nullptr, void *context = nullptr);
   template<typename C>
   void addAll(const StringMap *src, bool (*filter)(const TCHAR *, const TCHAR *, C *), C *context)
   {
      addAll(src, reinterpret_cast<bool (*)(const TCHAR *, const TCHAR *, void *)>(filter), context);
   }
   void addAll(const StringMap& src, bool (*filter)(const TCHAR *, const TCHAR *, void *) = nullptr, void *context = nullptr)
   {
      addAll(&src, filter, context);
   }
   template<typename C>
   void addAll(const StringMap& src, bool (*filter)(const TCHAR *, const TCHAR *, C *), C *context)
   {
      addAll(&src, reinterpret_cast<bool (*)(const TCHAR *, const TCHAR *, void *)>(filter), context);
   }

   void addAllFromMessage(const NXCPMessage& msg, uint32_t baseFieldId, uint32_t sizeFieldId);
   void addAllFromJson(json_t *json);

   TCHAR *unlink(const TCHAR *key)
   {
      return static_cast<TCHAR*>(StringMapBase::unlink(key));
   }

	const TCHAR *get(const TCHAR *key) const { return (const TCHAR *)getObject(key); }
   const TCHAR *get(const TCHAR *key, size_t len) const { return (const TCHAR *)getObject(key, len); }
   int32_t getInt32(const TCHAR *key, int32_t defaultValue) const;
	uint32_t getUInt32(const TCHAR *key, uint32_t defaultValue) const;
   int64_t getInt64(const TCHAR *key, int64_t defaultValue) const;
   uint64_t getUInt64(const TCHAR *key, uint64_t defaultValue) const;
   double getDouble(const TCHAR *key, double defaultValue) const;
	bool getBoolean(const TCHAR *key, bool defaultValue) const;

   template <typename C>
   StructArray<KeyValuePair<const TCHAR>> *toArray(bool (*filter)(const TCHAR *, const TCHAR *, C *), C *context = nullptr) const
   {
      return reinterpret_cast<StructArray<KeyValuePair<const TCHAR>>*>(StringMapBase::toArray(reinterpret_cast<bool (*)(const TCHAR*, const void*, void*)>(filter), (void *)context));
   }

   StructArray<KeyValuePair<const TCHAR>> *toArray() const
   {
      return reinterpret_cast<StructArray<KeyValuePair<const TCHAR>>*>(StringMapBase::toArray(nullptr, nullptr));
   }

   using StringMapBase::forEach;
   template <typename C>
   EnumerationCallbackResult forEach(EnumerationCallbackResult (*cb)(const TCHAR *, const TCHAR *, C *), C *context) const
   {
      return StringMapBase::forEach(reinterpret_cast<EnumerationCallbackResult (*)(const TCHAR*, const void*, void*)>(cb), (void *)context);
   }

   void fillMessage(NXCPMessage *msg, uint32_t baseFieldId, uint32_t sizeFieldId) const;

   json_t *toJson() const;

   Iterator<KeyValuePair<const TCHAR>> begin()
   {
      return Iterator<KeyValuePair<const TCHAR>>(new StringMapIterator(this));
   }

   ConstIterator<KeyValuePair<const TCHAR>> begin() const
   {
      return ConstIterator<KeyValuePair<const TCHAR>>(new StringMapIterator(const_cast<StringMap*>(this)));
   }

   Iterator<KeyValuePair<const TCHAR>> end()
   {
      return Iterator<KeyValuePair<const TCHAR>>(new StringMapIterator());
   }

   ConstIterator<KeyValuePair<const TCHAR>> end() const
   {
      return ConstIterator<KeyValuePair<const TCHAR>>(new StringMapIterator());
   }
};

/**
 * String map template for holding objects as values
 */
template <class T> class StringObjectMap : public StringMapBase
{
private:
	static void destructor(void *object, StringMapBase *map) { delete (T*)object; }

public:
	StringObjectMap(Ownership objectOwner, void (*_destructor)(void *, StringMapBase *) = destructor) : StringMapBase(objectOwner, _destructor) { }
	StringObjectMap(const StringObjectMap& src) = delete;
   StringObjectMap(StringObjectMap&& src) : StringMapBase(std::move(src)) { }

	void set(const TCHAR *key, T *object) { setObject((TCHAR *)key, (void *)object, false); }
	void setPreallocated(TCHAR *key, T *object) { setObject((TCHAR *)key, (void *)object, true); }
	T *get(const TCHAR *key) const { return (T*)getObject(key); }
   T *get(const TCHAR *key, size_t len) const { return (T*)getObject(key, len); }
   ObjectArray<T> *values() const { ObjectArray<T> *v = new ObjectArray<T>(size()); fillValues(v); return v; }
   T *unlink(const TCHAR *key) { return (T*)StringMapBase::unlink(key); }

   template <typename C>
   StructArray<KeyValuePair<T>> *toArray(bool (*filter)(const TCHAR *, const T *, C *), C *context = nullptr) const
   {
      return reinterpret_cast<StructArray<KeyValuePair<T>>*>(StringMapBase::toArray(reinterpret_cast<bool (*)(const TCHAR*, const void*, void*)>(filter), (void *)context));
   }

   StructArray<KeyValuePair<T>> *toArray() const
   {
      return reinterpret_cast<StructArray<KeyValuePair<T>>*>(StringMapBase::toArray(nullptr, nullptr));
   }

   template <typename C>
   EnumerationCallbackResult forEach(EnumerationCallbackResult (*cb)(const TCHAR*, T*, C*), C *context)
   {
      return StringMapBase::forEach(reinterpret_cast<EnumerationCallbackResult (*)(const TCHAR*, void*, void*)>(cb), (void *)context);
   }

   EnumerationCallbackResult forEach(std::function<EnumerationCallbackResult (const TCHAR*, T*)> cb)
   {
      return StringMapBase::forEach([&cb] (const TCHAR *key, void *value) { return cb(key, static_cast<T*>(value)); });
   }

   template <typename C>
   EnumerationCallbackResult forEach(EnumerationCallbackResult (*cb)(const TCHAR*, const T*, C*), C *context) const
   {
      return StringMapBase::forEach(reinterpret_cast<EnumerationCallbackResult (*)(const TCHAR*, const void*, void*)>(cb), (void *)context);
   }

   EnumerationCallbackResult forEach(std::function<EnumerationCallbackResult (const TCHAR*, const T*)> cb) const
   {
      return StringMapBase::forEach([&cb] (const TCHAR *key, const void *value) { return cb(key, static_cast<const T*>(value)); });
   }

   Iterator<KeyValuePair<T>> begin()
   {
      return Iterator<KeyValuePair<T>>(new StringMapIterator(this));
   }

   ConstIterator<KeyValuePair<T>> begin() const
   {
      return ConstIterator<KeyValuePair<T>>(new StringMapIterator(const_cast<StringObjectMap<T>*>(this)));
   }

   Iterator<KeyValuePair<T>> end()
   {
      return Iterator<KeyValuePair<T>>(new StringMapIterator());
   }

   ConstIterator<KeyValuePair<T>> end() const
   {
      return ConstIterator<KeyValuePair<T>>(new StringMapIterator());
   }
};

/**
 * String to object map that holds shared pointers
 */
template <class T> class SharedStringObjectMap final
{
private:
   ObjectMemoryPool<shared_ptr<T>> m_pool;
   StringObjectMap<shared_ptr<T>> m_data;

   static shared_ptr<T> m_null;

   static void destructor(void *element, StringMapBase *stringMapBase)
   {
      static_cast<SharedStringObjectMap<T>*>(stringMapBase->getContext())->m_pool.destroy(static_cast<shared_ptr<T>*>(element));
   }

public:
   SharedStringObjectMap() : m_pool(64), m_data(Ownership::True, SharedStringObjectMap<T>::destructor) { m_data.setContext(this); }
   SharedStringObjectMap(const SharedStringObjectMap& src) = delete;

   void set(const TCHAR *key, shared_ptr<T> object) { m_data.set(key, new(m_pool.allocate()) shared_ptr<T>(object)); }
   void set(const TCHAR *key, T *object) { m_data.set(key, new(m_pool.allocate()) shared_ptr<T>(object)); }
   void setPreallocated(TCHAR *key, T *object) { m_data.setPreallocated(key, new(m_pool.allocate()) shared_ptr<T>(object)); }
   void setPreallocated(TCHAR *key, shared_ptr<T> object) { m_data.setPreallocated(key, new(m_pool.allocate()) shared_ptr<T>(object)); }

   int size() const { return m_data.size(); }
   bool isEmpty() const { return m_data.isEmpty(); }
   bool contains(const TCHAR *key) const { return m_data.contains(key); }
   bool contains(const TCHAR *key, size_t len) const { return m_data.contains(key, len); }

   T *get(const TCHAR *key) const
   {
      auto p = m_data.get(key);
      return (p != nullptr) ? p->get() : nullptr;
   }

   T *get(const TCHAR *key, size_t len) const
   {
      auto p = m_data.get(key, len);
      return (p != nullptr) ? p->get() : nullptr;
   }

   shared_ptr<T> getShared(const TCHAR *key) const
   {
      auto p = m_data.get(key);
      return (p != nullptr) ? *p : m_null;
   }

   shared_ptr<T> getShared(const TCHAR *key, size_t len) const
   {
      auto p = m_data.get(key, len);
      return (p != nullptr) ? *p : m_null;
   }

   void clear() { m_data.clear(); }
   void remove(const TCHAR *key) { m_data.remove(key); }
   void filterElements(bool (*filter)(const TCHAR *, const void *, void *), void *context) { m_data.filterElements(filter, context); }

   shared_ptr<T> unlink(const TCHAR *key)
   {
      auto p = m_data.unlink(key);
      return (p != nullptr) ? *p : m_null;
   }

   template <typename C>
   EnumerationCallbackResult forEach(EnumerationCallbackResult (*cb)(const TCHAR *, const shared_ptr<T> *, C *), C *context) const
   {
      return m_data.forEach(cb, context);
   }

   Iterator<KeyValuePair<shared_ptr<T>>> begin()
   {
      return m_data.begin();
   }

   ConstIterator<KeyValuePair<shared_ptr<T>>> begin() const
   {
      return m_data.begin();
   }

   Iterator<KeyValuePair<shared_ptr<T>>> end()
   {
      return m_data.end();
   }

   ConstIterator<KeyValuePair<shared_ptr<T>>> end() const
   {
      return m_data.end();
   }
};

template <class T> shared_ptr<T> SharedStringObjectMap<T>::m_null = shared_ptr<T>();

/**
 * Entry of string set
 */
struct StringSetEntry;

/**
 * NXCP message
 */
class NXCPMessage;

/**
 * String set
 */
class StringSet;

/**
 * String set const iterator
 */
class LIBNETXMS_EXPORTABLE StringSetIterator : public AbstractIterator
{
protected:
   StringSet *m_stringSet;
   StringSetEntry *m_curr;
   StringSetEntry *m_next;

public:
   StringSetIterator(const StringSet *stringSet = nullptr);
   StringSetIterator(const StringSetIterator& other); //Copy constructor

   virtual bool hasNext() override;
   virtual void *next() override;
   virtual void *value() override;
   virtual bool equals(AbstractIterator* other) override;
   virtual void remove() override;
   virtual void unlink() override;
};

/**
 * String set class
 */
class LIBNETXMS_EXPORTABLE StringSet
{
   friend class StringSetIterator;

private:
   StringSetEntry *m_data;
   bool m_counting;

public:
   StringSet(bool counting = false);
   ~StringSet();

   int add(const TCHAR *str);
   int addPreallocated(TCHAR *str);
   int remove(const TCHAR *str);
   void clear();

   bool isEmpty() const { return m_data == nullptr; }
   size_t size() const;
   bool contains(const TCHAR *str) const;
   int count(const TCHAR *str) const;
   bool equals(const StringSet *s) const;

   void addAll(const StringSet *src);
   void addAll(const StringSet &src) { addAll(&src); }
   void addAll(const StringList *src);
   void addAll(const StringList &src) { addAll(&src); }
   void addAll(TCHAR **strings, int count);
   void splitAndAdd(const TCHAR *src, const TCHAR *separator);
   void addAllPreallocated(TCHAR **strings, int count);

   void forEach(bool (*cb)(const TCHAR*, void*), void *context) const;
   template<typename C> void forEach(bool (*cb)(const TCHAR*, C*), C *context) const
   {
      forEach(reinterpret_cast<bool (*)(const TCHAR*, void*)>(cb), context);
   }
   void forEach(std::function<bool(const TCHAR*)> cb) const;

   void fillMessage(NXCPMessage *msg, uint32_t baseId, uint32_t countId) const;
   void addAllFromMessage(const NXCPMessage& msg, uint32_t baseId, uint32_t countId, bool clearBeforeAdd = false, bool toUppercase = false);
   void addAllFromMessage(const NXCPMessage &msg, uint32_t fieldId, bool clearBeforeAdd = false, bool toUppercase = false);

   String join(const TCHAR *separator);

   Iterator<const TCHAR> begin()
   {
      return Iterator<const TCHAR>(new StringSetIterator(this));
   }

   ConstIterator<const TCHAR> begin() const
   {
      return ConstIterator<const TCHAR>(new StringSetIterator(this));
   }

   Iterator<const TCHAR> end()
   {
      return Iterator<const TCHAR>(new StringSetIterator());
   }

   ConstIterator<const TCHAR> end() const
   {
      return ConstIterator<const TCHAR>(new StringSetIterator());
   }
};

/**
 * Opaque hash set entry structure
 */
struct HashSetEntry;

/**
 * Hash set base class
 */
class HashSetBase;

/**
 * Hash set const iterator
 */
class LIBNETXMS_EXPORTABLE HashSetIterator : public AbstractIterator
{
protected:
   HashSetBase *m_hashSet;
   HashSetEntry *m_curr;
   HashSetEntry *m_next;

public:
   HashSetIterator(const HashSetBase *hashSet = nullptr);

   virtual bool hasNext() override;
   virtual void *next() override;
   virtual void *value() override;
   virtual bool equals(AbstractIterator* other) override;
   virtual void remove() override;
   virtual void unlink() override;
};

/**
 * Hash set base class (for fixed size non-pointer keys)
 */
class LIBNETXMS_EXPORTABLE HashSetBase
{
   friend class HashSetIterator;

private:
   HashSetEntry *m_data;
   unsigned int m_keylen;
   bool m_useCounter;

   void copyData(const HashSetBase& src);

protected:
   HashSetBase(unsigned int keylen, bool useCounter);
   HashSetBase(const HashSetBase& src);
   HashSetBase(HashSetBase&& src);

   HashSetBase& operator =(const HashSetBase& src);
   HashSetBase& operator =(HashSetBase&& src);

   void _put(const void *key);
   void _remove(const void *key);
   bool _contains(const void *key) const;

public:
   virtual ~HashSetBase();

   int size() const;
   bool isEmpty() const { return size() == 0; }

   void clear();

   EnumerationCallbackResult forEach(EnumerationCallbackResult (*cb)(const void *, void *), void *context) const;
};

/**
 * Hash set template
 */
template<class K> class HashSet : public HashSetBase
{
public:
   HashSet() : HashSetBase(sizeof(K), false) { }
   HashSet(const HashSet<K>& src) : HashSetBase(src) { };
   HashSet(HashSet<K>&& src) : HashSetBase(std::move(src)) { };

   HashSet<K>& operator =(const HashSet<K>& src)
   {
      HashSetBase::operator=(src);
      return *this;
   }
   HashSet<K>& operator =(HashSet<K>&& src)
   {
      HashSetBase::operator=(std::move(src));
      return *this;
   }

   void put(const K& key) { _put(&key); }
   void putAll(const std::vector<K>& keys)
   {
      for (const K& key : keys)
         _put(&key);
   }
   void remove(const K& key) { _remove(&key); }
   void removeAll(const std::vector<K>& keys)
   {
      for (const K& key : keys)
         _remove(&key);
   }
   bool contains(const K& key) const { return _contains(&key); }

   EnumerationCallbackResult forEach(EnumerationCallbackResult (*cb)(const K*, void*), void *context) const
   {
      return HashSetBase::forEach(reinterpret_cast<EnumerationCallbackResult (*)(const void *, void *)>(cb), context);
   }

   template<typename C> EnumerationCallbackResult forEach(EnumerationCallbackResult (*cb)(const K*, C*), C *context) const
   {
      return HashSetBase::forEach(reinterpret_cast<EnumerationCallbackResult (*)(const void*, void*)>(cb), context);
   }

   Iterator<const K> begin()
   {
      return Iterator<const K>(new HashSetIterator(this));
   }

   ConstIterator<const K> begin() const
   {
      return ConstIterator<const K>(new HashSetIterator(this));
   }

   Iterator<const K> end()
   {
      return Iterator<const K>(new HashSetIterator());
   }

   ConstIterator<const K> end() const
   {
      return ConstIterator<const K>(new HashSetIterator());
   }
};

#ifdef _WIN32
// Define DLL interfaces for common integer types in libnetxms
template class LIBNETXMS_TEMPLATE_EXPORTABLE HashSet<uint32_t>;
template class LIBNETXMS_TEMPLATE_EXPORTABLE HashSet<uint64_t>;
#endif

/**
 * Hash set template
 */
template<class K> class CountingHashSet : public HashSetBase
{
public:
   CountingHashSet() : HashSetBase(sizeof(K), true) { }
   CountingHashSet(const CountingHashSet<K>& src) : HashSetBase(src) { };
   CountingHashSet(CountingHashSet<K>&& src) : HashSetBase(std::move(src)) { };

   CountingHashSet<K>& operator =(const CountingHashSet<K>& src)
   {
      HashSetBase::operator=(src);
      return *this;
   }
   CountingHashSet<K>& operator =(CountingHashSet<K>&& src)
   {
      HashSetBase::operator=(std::move(src));
      return *this;
   }

   void put(const K& key) { _put(&key); }
   void putAll(std::vector<K> keys) { for (K& key : keys) _put(&key); }
   void remove(const K& key) { _remove(&key); }
   void removeAll(std::vector<K> keys) { for (K& key : keys) _remove(&key); }
   bool contains(const K& key) const { return _contains(&key); }

   EnumerationCallbackResult forEach(EnumerationCallbackResult (*cb)(const K *, void *), void *context) const
   {
      return HashSetBase::forEach(reinterpret_cast<EnumerationCallbackResult (*)(const void *, void *)>(cb), context);
   }

   Iterator<const K> begin()
   {
      return Iterator<const K>(new HashSetIterator(this));
   }

   ConstIterator<const K> begin() const
   {
      return ConstIterator<const K>(new HashSetIterator(this));
   }

   Iterator<const K> end()
   {
      return Iterator<const K>(new HashSetIterator());
   }

   ConstIterator<const K> end() const
   {
      return ConstIterator<const K>(new HashSetIterator());
   }
};

#ifdef _WIN32
// Define DLL interfaces for common integer types in libnetxms
template class LIBNETXMS_TEMPLATE_EXPORTABLE CountingHashSet<uint32_t>;
template class LIBNETXMS_TEMPLATE_EXPORTABLE CountingHashSet<uint64_t>;
#endif

/**
 * Synchronized hash set
 */
template<class K> class SynchronizedHashSet
{
private:
   HashSet<K> m_set;
   Mutex m_mutex;

public:
   SynchronizedHashSet() : m_mutex(MutexType::FAST) { }

   void put(const K& key)
   {
      m_mutex.lock();
      m_set.put(key);
      m_mutex.unlock();
   }
   void remove(const K& key)
   {
      m_mutex.lock();
      m_set.remove(key);
      m_mutex.unlock();
   }
   void clear()
   {
      m_mutex.lock();
      m_set.clear();
      m_mutex.unlock();
   }
   bool contains(const K& key) const
   {
      m_mutex.lock();
      bool result = m_set.contains(key);
      m_mutex.unlock();
      return result;
   }

   EnumerationCallbackResult forEach(EnumerationCallbackResult (*cb)(const K *, void *), void *context) const
   {
      m_mutex.lock();
      auto result = m_set.forEach(cb, context);
      m_mutex.unlock();
      return result;
   }
};

/**
 * Synchronized counting hash set
 */
template<class K> class SynchronizedCountingHashSet
{
private:
   CountingHashSet<K> m_set;
   Mutex m_mutex;

public:
   SynchronizedCountingHashSet() : m_mutex(MutexType::FAST) { }

   void put(const K& key)
   {
      m_mutex.lock();
      m_set.put(key);
      m_mutex.unlock();
   }
   void remove(const K& key)
   {
      m_mutex.lock();
      m_set.remove(key);
      m_mutex.unlock();
   }
   bool contains(const K& key) const
   {
      m_mutex.lock();
      bool result = m_set.contains(key);
      m_mutex.unlock();
      return result;
   }
   void clear()
   {
      m_mutex.lock();
      m_set.clear();
      m_mutex.unlock();
   }

   EnumerationCallbackResult forEach(EnumerationCallbackResult (*cb)(const K *, void *), void *context) const
   {
      m_mutex.lock();
      auto result = m_set.forEach(cb, context);
      m_mutex.unlock();
      return result;
   }
};

#ifdef _WIN32
// Define DLL interfaces for common integer types in libnetxms
template class LIBNETXMS_TEMPLATE_EXPORTABLE SynchronizedHashSet<uint32_t>;
template class LIBNETXMS_TEMPLATE_EXPORTABLE SynchronizedHashSet<uint64_t>;
template class LIBNETXMS_TEMPLATE_EXPORTABLE SynchronizedCountingHashSet<uint32_t>;
template class LIBNETXMS_TEMPLATE_EXPORTABLE SynchronizedCountingHashSet<uint64_t>;
#endif

/**
 * Opaque hash map entry structure
 */
struct HashMapEntry;

/**
 * Hash map base class (for fixed size non-pointer keys)
 */
class LIBNETXMS_EXPORTABLE HashMapBase
{
   friend class HashMapIterator;

private:
   HashMapEntry *m_data;
	bool m_objectOwner;
   unsigned int m_keylen;
   void *m_context;

	HashMapEntry *find(const void *key) const;
	void destroyObject(void *object)
	{
	   if (object != nullptr)
	      m_objectDestructor(object, this);
	}

protected:
	void (*m_objectDestructor)(void *, HashMapBase *);

   HashMapBase(Ownership objectOwner, unsigned int keylen, void (*destructor)(void *, HashMapBase *) = nullptr);

	void *_get(const void *key) const;
	void _set(const void *key, void *value);
	void _remove(const void *key, bool destroyValue);

   bool _contains(const void *key) const { return find(key) != nullptr; }

public:
   virtual ~HashMapBase();

   void setOwner(Ownership owner) { m_objectOwner = (owner == Ownership::True); }
	void clear();

	int size() const;

   EnumerationCallbackResult forEach(EnumerationCallbackResult (*cb)(const void*, void*, void*), void *context) const;
   EnumerationCallbackResult forEach(std::function<EnumerationCallbackResult (const void*, void*)> cb) const;
   const void *findElement(bool (*comparator)(const void *, const void *, void *), void *context) const;
   const void *findElement(std::function<bool (const void*, const void*)> comparator) const;

   void setContext(void *context) { m_context = context; }
   void *getContext() const { return m_context; }
};

/**
 * Hash map iterator
 */
class LIBNETXMS_EXPORTABLE HashMapIterator : public AbstractIterator
{
private:
   HashMapBase *m_hashMap;
   HashMapEntry *m_curr;
   HashMapEntry *m_next;

protected:
   void *key();

public:
   HashMapIterator(HashMapBase *hashMap = nullptr);
   HashMapIterator(const HashMapIterator& other);  

   virtual bool hasNext() override;
   virtual void *next() override;
   virtual void remove() override;
   virtual void unlink() override;
   virtual void *value() override;
   virtual bool equals(AbstractIterator* other) override;
};

/**
 * Hash map template for holding objects as values
 */
template<class K, class V> class HashMap : public HashMapBase
{
private:
	static void destructor(void *object, HashMapBase *map) { delete (V*)object; }

public:
	HashMap(Ownership objectOwner = Ownership::False, void (*_destructor)(void *, HashMapBase *) = destructor) : HashMapBase(objectOwner, sizeof(K), _destructor) { }

	V *get(const K& key) const { return (V*)_get(&key); }
	void set(const K& key, V *value) { _set(&key, (void *)value); }
   void remove(const K& key) { _remove(&key, true); }
   void unlink(const K& key) { _remove(&key, false); }
   bool contains(const K& key) { return _contains(&key); }

   template<typename C> EnumerationCallbackResult forEach(EnumerationCallbackResult (*cb)(const K&, V*, C*), C *context) const
   {
      return HashMapBase::forEach([cb, context] (const void *k, void *v) { return cb(*static_cast<const K*>(k), static_cast<V*>(v), context); });
   }

   EnumerationCallbackResult forEach(std::function<EnumerationCallbackResult (const K&, V*)> cb) const
   {
      return HashMapBase::forEach([cb] (const void *k, void *v) { return cb(*static_cast<const K*>(k), static_cast<V*>(v)); });
   }

   template<typename C> const V *findElement(bool (*comparator)(const K&, const V&, C*), C *context) const
   {
      return static_cast<const V*>(HashMapBase::findElement([comparator, context] (const void *k, const void *v) { return comparator(*static_cast<const K*>(k), *static_cast<const V*>(v), context); }));
   }

   const V *findElement(std::function<bool (const K&, const V&)> comparator) const
   {
      return static_cast<const V*>(HashMapBase::findElement([comparator] (const void *k, const void *v) { return comparator(*static_cast<const K*>(k), *static_cast<const V*>(v)); }));
   }

   Iterator<V> begin()
   {
      return Iterator<V>(new HashMapIterator(this));
   }

   Iterator<V> end()
   {
      return Iterator<V>(new HashMapIterator());
   }
};

/**
 * Synchronized hash map
 */
template <class K, class V> class SynchronizedHashMap final
{
private:
   HashMap<K, V> m_data;
   Mutex m_mutex;

public:
   SynchronizedHashMap(Ownership ownership) : m_data(ownership), m_mutex(MutexType::FAST)
   {
      m_data.setContext(this);
   }
   SynchronizedHashMap(const SynchronizedHashMap& src) = delete;

   void set(const K& key, V *element)
   {
      m_mutex.lock();
      m_data.set(key, element);
      m_mutex.unlock();
   }

   V *get(const K& key) const
   {
      m_mutex.lock();
      auto p = m_data.get(key);
      m_mutex.unlock();
      return p;
   }

   V *take(const K& key)
   {
      m_mutex.lock();
      auto p = m_data.get(key);
      if (p != nullptr)
         m_data.unlink(key);
      m_mutex.unlock();
      return p;
   }

   void remove(const K& key)
   {
      m_mutex.lock();
      m_data.remove(key);
      m_mutex.unlock();
   }

   void clear()
   {
      m_mutex.lock();
      m_data.clear();
      m_mutex.unlock();
   }

   bool contains(const K& key)
   {
      m_mutex.lock();
      bool contains = m_data.contains(key);
      m_mutex.unlock();
      return contains;
   }

   int size() const
   {
      m_mutex.lock();
      int size = m_data.size();
      m_mutex.unlock();
      return size;
   }

   template <typename C>
   EnumerationCallbackResult forEach(EnumerationCallbackResult (*cb)(const K&, V*, C*), C *context) const
   {
      m_mutex.lock();
      EnumerationCallbackResult result = m_data.forEach(cb, context);
      m_mutex.unlock();
      return result;
   }

   EnumerationCallbackResult forEach(std::function<EnumerationCallbackResult (const K&, V*)> cb) const
   {
      m_mutex.lock();
      EnumerationCallbackResult result = m_data.forEach(cb);
      m_mutex.unlock();
      return result;
   }

   template <typename C>
   const V *findElement(bool(*comparator)(const K&, const V&, C*), C *context) const
   {
      m_mutex.lock();
      auto result = m_data.findElement(comparator, context);
      m_mutex.unlock();
      return result;
   }

   const V *findElement(std::function<bool (const K&, const V&)> comparator) const
   {
      m_mutex.lock();
      auto result = m_data.findElement(comparator);
      m_mutex.unlock();
      return result;
   }
};

/**
 * Shared hash map
 */
template <class K, class V> class SharedHashMap final
{
   // Note: member order is important because m_data allocates memory from m_pool
private:
   ObjectMemoryPool<shared_ptr<V>> m_pool;
   HashMap<K, shared_ptr<V>> m_data;

   static shared_ptr<V> m_null;

   static void destructor(void *element, HashMapBase *hashMapBase)
   {
      static_cast<SharedHashMap<K, V>*>(hashMapBase->getContext())->m_pool.destroy(static_cast<shared_ptr<V>*>(element));
   }

public:
   SharedHashMap() : m_pool(64), m_data(Ownership::True, SharedHashMap<K, V>::destructor) { m_data.setContext(this); }
   SharedHashMap(const SharedHashMap& src) = delete;

   void set(const K& key, shared_ptr<V> element) { m_data.set(key, new(m_pool.allocate()) shared_ptr<V>(element)); }
   void set(const K& key, V *element) { m_data.set(key, new(m_pool.allocate()) shared_ptr<V>(element)); }
   V *get(const K& key) const
   {
      auto p = m_data.get(key);
      return (p != nullptr) ? p->get() : nullptr;
   }
   const shared_ptr<V>& getShared(const K& key) const
   {
      auto p = m_data.get(key);
      return (p != nullptr) ? *p : m_null;
   }
   void remove(const K& key) { m_data.remove(key); }
   void unlink(const K& key) { m_data.unlink(key); }
   void clear() { m_data.clear(); }
   bool contains(const K& key) { return m_data.contains(key); }

   int size() const { return m_data.size(); }

   template <typename C>
   EnumerationCallbackResult forEach(EnumerationCallbackResult (*cb)(const K&, const shared_ptr<V>&, C*), C *context) const
   {
      return m_data.forEach(
         [cb, context] (const K& key, shared_ptr<V> *value) -> EnumerationCallbackResult
         {
            return cb(key, *value, context);
         });
   }

   EnumerationCallbackResult forEach(std::function<EnumerationCallbackResult (const K&, const shared_ptr<V>&)> cb) const
   {
      return m_data.forEach(
         [cb] (const K& key, shared_ptr<V> *value) -> EnumerationCallbackResult
         {
            return cb(key, *value);
         });
   }

   SharedPtrIterator<V> begin()
   {
      return SharedPtrIterator<V>(new HashMapIterator(&m_data));
   }

   SharedPtrIterator<V> end()
   {
      return SharedPtrIterator<V>(new HashMapIterator());
   }
};

template <class K, class V> shared_ptr<V> SharedHashMap<K, V>::m_null = shared_ptr<V>();

/**
 * Synchronized shared hash map
 */
template <class K, class V> class SynchronizedSharedHashMap final
{
   // Note: member order is important because m_data allocates memory from m_pool
private:
   ObjectMemoryPool<shared_ptr<V>> m_pool;
   HashMap<K, shared_ptr<V>> m_data;
   Mutex m_mutex;

   static void destructor(void *element, HashMapBase *hashMapBase)
   {
      static_cast<SynchronizedSharedHashMap<K, V>*>(hashMapBase->getContext())->m_pool.destroy(static_cast<shared_ptr<V>*>(element));
   }

public:
   SynchronizedSharedHashMap() : m_pool(64), m_data(Ownership::True, SynchronizedSharedHashMap<K, V>::destructor), m_mutex(MutexType::FAST)
   {
      m_data.setContext(this);
   }
   SynchronizedSharedHashMap(const SynchronizedSharedHashMap& src) = delete;

   void set(const K& key, shared_ptr<V> element)
   {
      m_mutex.lock();
      m_data.set(key, new(m_pool.allocate()) shared_ptr<V>(element));
      m_mutex.unlock();
   }

   void set(const K& key, V *element)
   {
      m_mutex.lock();
      m_data.set(key, new(m_pool.allocate()) shared_ptr<V>(element));
      m_mutex.unlock();
   }

   shared_ptr<V> getShared(const K& key) const
   {
      shared_ptr<V> result;
      m_mutex.lock();
      shared_ptr<V> *element = m_data.get(key);
      if (element != nullptr)
         result = *element;
      m_mutex.unlock();
      return result;
   }

   void remove(const K& key)
   {
      m_mutex.lock();
      m_data.remove(key);
      m_mutex.unlock();
   }

   void clear()
   {
      m_mutex.lock();
      m_data.clear();
      m_mutex.unlock();
   }

   bool contains(const K& key)
   {
      m_mutex.lock();
      bool contains = m_data.contains(key);
      m_mutex.unlock();
      return contains;
   }

   int size() const
   {
      m_mutex.lock();
      int size = m_data.size();
      m_mutex.unlock();
      return size;
   }

   template <typename C>
   EnumerationCallbackResult forEach(EnumerationCallbackResult (*cb)(const K&, const shared_ptr<V>&, C*), C *context) const
   {
      m_mutex.lock();
      EnumerationCallbackResult result = m_data.forEach(
         [cb, context] (const K& key, shared_ptr<V> *value) -> EnumerationCallbackResult
         {
            return cb(key, *value, context);
         });
      m_mutex.unlock();
      return result;
   }

   EnumerationCallbackResult forEach(std::function<EnumerationCallbackResult (const K&, const shared_ptr<V>&)> cb) const
   {
      m_mutex.lock();
      EnumerationCallbackResult result = m_data.forEach(
         [cb] (const K& key, shared_ptr<V> *value) -> EnumerationCallbackResult
         {
            return cb(key, *value);
         });
      m_mutex.unlock();
      return result;
   }

   template <typename C>
   shared_ptr<V> findElement(bool (*comparator)(const K&, const V&, C*), C *context) const
   {
      shared_ptr<V> result;
      m_mutex.lock();
      const shared_ptr<V> *element = m_data.findElement(
         [comparator, context] (const K& key, const shared_ptr<V>& value) -> bool
         {
            return comparator(key, *value, context);
         });
      if (element != nullptr)
         result = *element;
      m_mutex.unlock();
      return result;
   }

   shared_ptr<V> findElement(std::function<bool (const K&, const V&)> comparator) const
   {
      shared_ptr<V> result;
      m_mutex.lock();
      const shared_ptr<V> *element = m_data.findElement(
         [comparator] (const K& key, const shared_ptr<V>& value) -> bool
         {
            return comparator(key, *value);
         });
      if (element != nullptr)
         result = *element;
      m_mutex.unlock();
      return result;
   }
};

/**
 * Ring buffer
 */
class LIBNETXMS_EXPORTABLE RingBuffer
{
private:
   BYTE *m_data;
   size_t m_size;
   size_t m_allocated;
   size_t m_allocationStep;
   size_t m_readPos;
   size_t m_writePos;
   size_t m_savedPos;
   size_t m_savedSize;

public:
   RingBuffer(size_t initial = 8192, size_t allocationStep = 8192);
   RingBuffer(const RingBuffer& src) = delete;
   ~RingBuffer()
   {
      MemFree(m_data);
   }

   void write(const BYTE *data, size_t dataSize);
   size_t read(BYTE *buffer, size_t bufferSize);
   BYTE readByte();

   void clear();

   void savePos();
   void restorePos();

   size_t size() const { return m_size; }
   bool isEmpty() const { return m_size == 0; }
};

#ifdef WORDS_BIGENDIAN
#define HostToBigEndian16(n) (n)
#define HostToBigEndian32(n) (n)
#define HostToBigEndian64(n) (n)
#define HostToBigEndianD(n) (n)
#define HostToBigEndianF(n) (n)
#define HostToLittleEndian16(n) bswap_16(n)
#define HostToLittleEndian32(n) bswap_32(n)
#define HostToLittleEndian64(n) bswap_64(n)
#define HostToLittleEndianD(n) bswap_double(n)
#define HostToLittleEndianF(n) bswap_float(n)
#define BigEndianToHost16(n) (n)
#define BigEndianToHost32(n) (n)
#define BigEndianToHost64(n) (n)
#define BigEndianToHostD(n) (n)
#define BigEndianToHostF(n) (n)
#define LittleEndianToHost16(n) bswap_16(n)
#define LittleEndianToHost32(n) bswap_32(n)
#define LittleEndianToHost64(n) bswap_64(n)
#define LittleEndianToHostD(n) bswap_double(n)
#define LittleEndianToHostF(n) bswap_float(n)
#else
#define HostToBigEndian16(n) bswap_16(n)
#define HostToBigEndian32(n) bswap_32(n)
#define HostToBigEndian64(n) bswap_64(n)
#define HostToBigEndianD(n) bswap_double(n)
#define HostToBigEndianF(n) bswap_float(n)
#define HostToLittleEndian16(n) (n)
#define HostToLittleEndian32(n) (n)
#define HostToLittleEndian64(n) (n)
#define HostToLittleEndianD(n) (n)
#define HostToLittleEndianF(n) (n)
#define BigEndianToHost16(n) bswap_16(n)
#define BigEndianToHost32(n) bswap_32(n)
#define BigEndianToHost64(n) bswap_64(n)
#define BigEndianToHostD(n) bswap_double(n)
#define BigEndianToHostF(n) bswap_float(n)
#define LittleEndianToHost16(n) (n)
#define LittleEndianToHost32(n) (n)
#define LittleEndianToHost64(n) (n)
#define LittleEndianToHostD(n) (n)
#define LittleEndianToHostF(n) (n)
#endif

/**
 * Byte stream
 */
class LIBNETXMS_EXPORTABLE ConstByteStream
{
protected:
   BYTE *m_data;
   size_t m_size;
   size_t m_allocated;
   size_t m_allocationStep;
   size_t m_pos;

   ssize_t getEncodedStringLength(ssize_t byteCount, bool isLenPrepended, bool isNullTerminated, size_t charSize);
   char *readStringCore(ssize_t byteCount, bool isLenPrepended, bool isNullTerminated);
   WCHAR *readStringWCore(const char* codepage, ssize_t byteCount, bool isLenPrepended, bool isNullTerminated);
   char *readStringAsUTF8Core(const char* codepage, ssize_t byteCount, bool isLenPrepended, bool isNullTerminated);
   ssize_t readStringU(WCHAR* buffer, const char* codepage, ssize_t byteCount);

   ConstByteStream() {}

public:
   ConstByteStream(const BYTE *data, size_t size);
   ConstByteStream(const ConstByteStream& src) = delete;
   virtual ~ConstByteStream() {}

   off_t seek(off_t offset, int origin = SEEK_SET);

   /**
    * Find first occurrence of a given value in buffer starting from current position
    * @return Position in the stream. Returns -1 on failure.
    */
   template<typename T> off_t find(T value)
   {
      auto p = static_cast<BYTE*>(memmem(&m_data[m_pos], m_size - m_pos, &value, sizeof(T)));
      return p != nullptr ? p - m_data : -1;
   }

   size_t pos() const { return m_pos; }
   size_t size() const { return m_size; }
   bool eos() const { return m_pos == m_size; }

   const BYTE *buffer(size_t *size) const { *size = m_size; return m_data; }
   const BYTE *buffer() const { return m_data; }

   size_t read(void *buffer, size_t count);

   char readChar() { return !eos() ? (char)m_data[m_pos++] : 0; }
   BYTE readByte() { return !eos() ? m_data[m_pos++] : 0; }

   uint16_t readUInt16B()
   {
      uint16_t n = 0;
      read(&n, 2);
      return BigEndianToHost16(n);
   }

   uint32_t readUInt32B()
   {
      uint32_t n = 0;
      read(&n, 4);
      return BigEndianToHost32(n);
   }

   uint64_t readUInt64B()
   {
      uint64_t n = 0;
      read(&n, 8);
      return BigEndianToHost64(n);
   }

   double readDoubleB()
   {
      double n = 0;
      read(&n, 8);
      return BigEndianToHostD(n);
   }

   float readFloatB()
   {
      float n = 0;
      read(&n, 4);
      return BigEndianToHostF(n);
   }

   int16_t readInt16B() { return (int16_t)readUInt16B(); }
   int32_t readInt32B() { return (int32_t)readUInt32B(); }
   int64_t readInt64B() { return (int64_t)readUInt64B(); }

   uint16_t readUInt16L()
   {
      uint16_t n = 0;
      read(&n, 2);
      return LittleEndianToHost16(n);
   }

   uint32_t readUInt32L()
   {
      uint32_t n = 0;
      read(&n, 4);
      return LittleEndianToHost32(n);
   }

   uint64_t readUInt64L()
   {
      uint64_t n = 0;
      read(&n, 8);
      return LittleEndianToHost64(n);
   }

   double readDoubleL()
   {
      double n = 0;
      read(&n, 8);
      return LittleEndianToHostD(n);
   }

   float readFloatL()
   {
      float n = 0;
      read(&n, 4);
      return LittleEndianToHostF(n);
   }

   int16_t readInt16L() { return (int16_t)readUInt16L(); }
   int32_t readInt32L() { return (int32_t)readUInt32L(); }
   int64_t readInt64L() { return (int64_t)readUInt64L(); }

   /**
    * Read string of the known length.
    * @param codepage encoding of the stored string.
    * @param length number of bytes to read.
    * @return Dynamically allocated wide character string. Must be freed by caller.
    */
   WCHAR* readStringW(const char* codepage, size_t length) { return readStringWCore(codepage, length, false, false); }

   /**
    * Read string of the known length.
    * @param codepage encoding of the stored string.
    * @param length number of bytes to read.
    * @return Dynamically allocated multibyte character string in UTF-8 encoding. Must be freed by caller.
    */
   char* readStringAsUtf8(const char* codepage, size_t length) { return readStringAsUTF8Core(codepage, length, false, false); }

   /**
    * Read string of the known length.
    * @param length number of bytes to read.
    * @return Dynamically allocated multibyte character string in the same encoding as the stored string has. Must be freed by caller.
    */
   char* readStringA(size_t length) { return readStringCore(length, false, false); }

   /**
    * Read string prepended with length (Pascal type).
    * @param codepage encoding of the stored string.
    * @return Dynamically allocated wide character string. Must be freed by caller.
    */
   WCHAR* readPStringW(const char* codepage) { return readStringWCore(codepage, -1, true, false); }

   /**
    * Read string prepended with length (Pascal type).
    * @param codepage encoding of the stored string.
    * @return Dynamically allocated multibyte character string in UTF-8 encoding. Must be freed by caller.
    */
   char* readPStringAsUtf8(const char* codepage) { return readStringAsUTF8Core(codepage, -1, true, false); }

   /**
    * Read string prepended with length (Pascal type).
    * @return Dynamically allocated multibyte character string in the same encoding as the stored string has. Must be freed by caller.
    */
   char* readPStringA() { return readStringCore(-1, true, false); }

   /**
    * Read null-terminated string (C type).
    * @param codepage encoding of the stored string.
    * @return Dynamically allocated wide character string. Must be freed by caller.
    */
   WCHAR* readCStringW(const char* codepage) { return readStringWCore(codepage, -1, false, true); }

    /**
    * Read null-terminated string (C type).
    * @param codepage encoding of the stored string.
    * @return Dynamically allocated multibyte character string in UTF-8 encoding. Must be freed by caller.
    */
   char* readCStringAsUtf8(const char* codepage) { return readStringAsUTF8Core(codepage, -1, false, true); }

    /**
    * Read null-terminated string (C type).
    * @return Dynamically allocated multibyte character string in the same encoding as the stored string has. Must be freed by caller.
    */
   char* readCStringA() { return readStringCore(-1, false, true); }

   /**
    * Read string from NXCP message
    */
   TCHAR *readNXCPString(MemoryPool *pool);

   bool save(int f);
};

/**
 * Byte stream
 */
class LIBNETXMS_EXPORTABLE ByteStream : public ConstByteStream
{
private:
   size_t m_allocated;
   size_t m_allocationStep;
   ssize_t writeStringU(const WCHAR* str, size_t length, const char* codepage);

public:
   ByteStream(size_t initial = 8192);
   ByteStream(const void *data, size_t size);
   ByteStream(const ByteStream& src) = delete;
   virtual ~ByteStream();

   static ByteStream *load(const TCHAR *file);

   /**
    * Callback for writing data received from cURL to provided byte stream
    */
   static size_t curlWriteFunction(char *ptr, size_t size, size_t nmemb, ByteStream *data);

   void setAllocationStep(size_t s) { m_allocationStep = s; }

   BYTE *takeBuffer();

   void clear() { m_size = 0; m_pos = 0; }

   void write(const void *data, size_t size);
   void write(char c) { write(&c, 1); }
   void write(BYTE b) { write(&b, 1); }
#if CAN_OVERLOAD_INT8_T
   void write(int8_t c) { write(&c, 1); }
#endif

   void writeB(uint16_t n) { n = HostToBigEndian16(n); write(&n, 2); }
   void writeB(int16_t n) { writeB(static_cast<uint16_t>(n)); }
   void writeB(uint32_t n) { n = HostToBigEndian32(n); write(&n, 4); }
   void writeB(int32_t n) { writeB(static_cast<uint32_t>(n)); }
   void writeB(uint64_t n) { n = HostToBigEndian64(n); write(&n, 8); }
   void writeB(int64_t n) { writeB(static_cast<uint64_t>(n)); }
   void writeB(float n) { n = HostToBigEndianF(n); write(&n, 4); }
   void writeB(double n) { n = HostToBigEndianD(n); write(&n, 8); }

   void writeL(uint16_t n) { n = HostToLittleEndian16(n); write(&n, 2); }
   void writeL(int16_t n) { writeL(static_cast<uint16_t>(n)); }
   void writeL(uint32_t n) { n = HostToLittleEndian32(n); write(&n, 4); }
   void writeL(int32_t n) { writeL(static_cast<uint32_t>(n)); }
   void writeL(uint64_t n) { n = HostToLittleEndian64(n); write(&n, 8); }
   void writeL(int64_t n) { writeL(static_cast<uint64_t>(n)); }
   void writeL(float n) { n = HostToLittleEndianF(n); write(&n, 4); }
   void writeL(double n) { n = HostToLittleEndianD(n); write(&n, 8); }

   void writeSignedLEB128(int64_t n);
   void writeUnsignedLEB128(uint64_t n);

   size_t writeString(const WCHAR *str, const char *codepage, ssize_t length, bool prependLength, bool nullTerminate);
   size_t writeString(const char *str, ssize_t length, bool prependLength, bool nullTerminate);

   /**
    * Write string. No length indicators are written.
    * @param str the input string
    * @param codepage encoding of the in-stream string
    * @return Count of bytes written.
    */
   size_t writeString(const WCHAR *str, const char *codepage = "UTF-8")
   {
      return writeString(str, codepage, -1, false, false);
   }

   size_t writeNXCPString(const TCHAR *string);
};

class NXCPMessage;

/**
 * Table column definition
 */
class LIBNETXMS_EXPORTABLE TableColumnDefinition
{
private:
   TCHAR m_name[MAX_COLUMN_NAME];
   TCHAR m_displayName[MAX_DB_STRING];
   int32_t m_dataType;
   bool m_instanceColumn;
   TCHAR m_unitName[63];
   int m_multiplier;
   int m_useMultiplier;

public:
   TableColumnDefinition(const TCHAR *name, const TCHAR *displayName, int32_t dataType, bool isInstance);
   TableColumnDefinition(const NXCPMessage& msg, uint32_t baseId);
   TableColumnDefinition(const TableColumnDefinition& src) = default;

   void fillMessage(NXCPMessage *msg, uint32_t baseId) const;

   json_t *toJson() const;

   const TCHAR *getName() const { return m_name; }
   const TCHAR *getDisplayName() const { return m_displayName; }
   int32_t getDataType() const { return m_dataType; }
   bool isInstanceColumn() const { return m_instanceColumn; }
   const TCHAR *getUnitName() const { return m_unitName; }
   int getMultiplier() const { return m_multiplier; }

   void setDataType(int32_t type) { m_dataType = type; }
   void setInstanceColumn(bool isInstance) { m_instanceColumn = isInstance; }
   void setDisplayName(const TCHAR *name);
   void setUnitName(const TCHAR *name);
   void setMultiplier(int multiplier) { m_multiplier = multiplier; }
   void setUseMultiplier(int useMultiplier) { m_useMultiplier = useMultiplier; }
};

/**
 * Table cell
 */
class TableCell
{
private:
   TCHAR *m_value;
   int m_status;
   uint32_t m_objectId;

public:
   TableCell()
   {
      m_value = nullptr;
      m_status = -1;
      m_objectId = 0;
   }
   TableCell(const TCHAR *value)
   {
      m_value = MemCopyString(value);
      m_status = -1;
      m_objectId = 0;
   }
   TableCell(const TCHAR *value, int status)
   {
      m_value = MemCopyString(value);
      m_status = status;
      m_objectId = 0;
   }
   TableCell(const TableCell& src)
   {
      m_value = MemCopyString(src.m_value);
      m_status = src.m_status;
      m_objectId = src.m_objectId;
   }
   ~TableCell()
   {
      MemFree(m_value);
   }

   void set(const TCHAR *value, int status, uint32_t objectId)
   {
      MemFree(m_value);
      m_value = MemCopyString(value);
      m_status = status;
      m_objectId = objectId;
   }
   void setPreallocated(TCHAR *value, int status, uint32_t objectId)
   {
      MemFree(m_value);
      m_value = value;
      m_status = status;
      m_objectId = objectId;
   }

   const TCHAR *getValue() const { return m_value; }
   void setValue(const TCHAR *value)
   {
      MemFree(m_value);
      m_value = MemCopyString(value);
   }
   void setPreallocatedValue(TCHAR *value)
   {
      MemFree(m_value);
      m_value = value;
   }

   int getStatus() const { return m_status; }
   void setStatus(int status) { m_status = status; }

   int getObjectId() const { return m_objectId; }
   void setObjectId(uint32_t id) { m_objectId = id; }
};

#ifdef _WIN32
template class LIBNETXMS_TEMPLATE_EXPORTABLE ObjectArray<TableCell>;
#endif

/**
 * Table row
 */
class TableRow
{
private:
   ObjectArray<TableCell> m_cells;
   uint32_t m_objectId;
   int m_baseRow;

public:
   TableRow(int columnCount);
   TableRow(const TableRow& src);
   ~TableRow() = default;

   void addColumn() { m_cells.add(new TableCell); }
   void insertColumn(int index) { m_cells.insert(index, new TableCell); }
   void deleteColumn(int index) { m_cells.remove(index); }
   void swapColumns(int column1, int column2) { m_cells.swap(column1, column2); }

   void set(int index, const TCHAR *value, int status, uint32_t objectId)
   {
      TableCell *c = m_cells.get(index);
      if (c != nullptr)
         c->set(value, status, objectId);
   }
   void setPreallocated(int index, TCHAR *value, int status, uint32_t objectId)
   {
      TableCell *c = m_cells.get(index);
      if (c != nullptr)
         c->setPreallocated(value, status, objectId);
   }

   void setValue(int index, const TCHAR *value)
   {
      TableCell *c = m_cells.get(index);
      if (c != nullptr)
         c->setValue(value);
   }
   void setPreallocatedValue(int index, TCHAR *value)
   {
      TableCell *c = m_cells.get(index);
      if (c != nullptr)
         c->setPreallocatedValue(value);
      else
         MemFree(value);
   }

   void setStatus(int index, int status)
   {
      TableCell *c = m_cells.get(index);
      if (c != nullptr)
         c->setStatus(status);
   }

   const TCHAR *getValue(int index) const
   {
      const TableCell *c = m_cells.get(index);
      return (c != nullptr) ? c->getValue() : nullptr;
   }
   int getStatus(int index) const
   {
      const TableCell *c = m_cells.get(index);
      return (c != nullptr) ? c->getStatus() : -1;
   }

   uint32_t getObjectId() const { return m_objectId; }
   void setObjectId(uint32_t id) { m_objectId = id; }

   int getBaseRow() const { return m_baseRow; }
   void setBaseRow(int baseRow) { m_baseRow = baseRow; }

   uint32_t getCellObjectId(int index) const
   {
      const TableCell *c = m_cells.get(index);
      return (c != nullptr) ? c->getObjectId() : 0;
   }
   void setCellObjectId(int index, uint32_t id)
   {
      TableCell *c = m_cells.get(index);
      if (c != nullptr)
         c->setObjectId(id);
   }
};

#ifdef _WIN32
template class LIBNETXMS_TEMPLATE_EXPORTABLE ObjectArray<TableRow>;
template class LIBNETXMS_TEMPLATE_EXPORTABLE ObjectArray<TableColumnDefinition>;
#endif

/**
 * Class for table data storage
 */
class LIBNETXMS_EXPORTABLE Table
{
private:
   ObjectArray<TableRow> m_data;
   ObjectArray<TableColumnDefinition> m_columns;
   TCHAR *m_title;
   int m_source;
   bool m_extendedFormat;

   void createFromMessage(const NXCPMessage& msg);
   bool parseXML(const char *xml);

public:
   Table();
   Table(const NXCPMessage& msg);
   Table(const Table& src);
   ~Table();

   int fillMessage(NXCPMessage* msg, int offset, int rowLimit) const;
   void updateFromMessage(const NXCPMessage& msg);

   void addAll(const Table *src);
   int copyRow(const Table *src, int row);
   void merge(const Table *src);
   int mergeRow(const Table *src, int row, int insertBefore = -1);

   int getNumRows() const { return m_data.size(); }
   int getNumColumns() const { return m_columns.size(); }
   const TCHAR *getTitle() const { return CHECK_NULL_EX(m_title); }
   int getSource() const { return m_source; }

   bool isExtendedFormat() { return m_extendedFormat; }
   void setExtendedFormat(bool ext) { m_extendedFormat = ext; }

   const TCHAR *getColumnName(int col) const { return ((col >= 0) && (col < m_columns.size())) ? m_columns.get(col)->getName() : nullptr; }
   int32_t getColumnDataType(int col) const { return ((col >= 0) && (col < m_columns.size())) ? m_columns.get(col)->getDataType() : 0; }
   const TableColumnDefinition *getColumnDefinition(int col) const { return m_columns.get(col); }
	int getColumnIndex(const TCHAR *name) const;
   const ObjectArray<TableColumnDefinition>& getColumnDefinitions() { return m_columns; }

   void setTitle(const TCHAR *title) { MemFree(m_title); m_title = MemCopyString(title); }
   void setSource(int source) { m_source = source; }
   int addColumn(const TCHAR *name, int32_t dataType = 0, const TCHAR *displayName = nullptr, bool isInstance = false);
   int addColumn(const TableColumnDefinition& d);
   void insertColumn(int index, const TCHAR *name, int32_t dataType, const TCHAR *displayName, bool isInstance);
   void swapColumns(int column1, int column2);
   void setColumnDataType(int col, int32_t dataType)
   {
      if ((col >= 0) && (col < m_columns.size()))
         m_columns.get(col)->setDataType(dataType);
   }
   int addRow();
   int insertRow(int insertBefore);

   void deleteRow(int row);
   void deleteColumn(int col);

   void setAt(int row, int col, int32_t value);
   void setAt(int row, int col, uint32_t value);
   void setAt(int row, int col, double value, int digits = 6);
   void setAt(int row, int col, int64_t value);
   void setAt(int row, int col, uint64_t value);
   void setAt(int row, int col, const TCHAR *value);
   void setPreallocatedAt(int row, int col, TCHAR *value);

   void set(int col, int32_t value) { setAt(getNumRows() - 1, col, value); }
   void set(int col, uint32_t value) { setAt(getNumRows() - 1, col, value); }
   void set(int col, double value, int digits = 6) { setAt(getNumRows() - 1, col, value, digits); }
   void set(int col, int64_t value) { setAt(getNumRows() - 1, col, value); }
   void set(int col, uint64_t value) { setAt(getNumRows() - 1, col, value); }
   void set(int col, const TCHAR *value) { setAt(getNumRows() - 1, col, value); }
#ifdef UNICODE
   void set(int col, const char *value) { setPreallocatedAt(getNumRows() - 1, col, WideStringFromMBString(value)); }
#else
   void set(int col, const WCHAR *value) { setPreallocatedAt(getNumRows() - 1, col, MBStringFromWideString(value)); }
#endif
   void setPreallocated(int col, TCHAR *value) { setPreallocatedAt(getNumRows() - 1, col, value); }

   void setStatusAt(int row, int col, int status);
   void setStatus(int col, int status) { setStatusAt(getNumRows() - 1, col, status); }

   const TCHAR *getAsString(int nRow, int nCol, const TCHAR *defaultValue = NULL) const;
   int32_t getAsInt(int nRow, int nCol) const;
   uint32_t getAsUInt(int nRow, int nCol) const;
   int64_t getAsInt64(int nRow, int nCol) const;
   uint64_t getAsUInt64(int nRow, int nCol) const;
   double getAsDouble(int nRow, int nCol) const;

   int getStatus(int nRow, int nCol) const;

   void buildInstanceString(int row, TCHAR *buffer, size_t bufLen);
   int findRowByInstance(const TCHAR *instance);

   int findRow(void *key, bool (*comparator)(const TableRow *, void *));

   uint32_t getObjectId(int row) const
   {
      const TableRow *r = m_data.get(row);
      return (r != nullptr) ? r->getObjectId() : 0;
   }
   void setObjectIdAt(int row, uint32_t id)
   {
      TableRow *r = m_data.get(row);
      if (r != nullptr)
         r->setObjectId(id);
   }
   void setObjectId(uint32_t id) { setObjectIdAt(getNumRows() - 1, id); }

   void setCellObjectIdAt(int row, int col, uint32_t objectId);
   void setCellObjectId(int col, uint32_t objectId) { setCellObjectIdAt(getNumRows() - 1, col, objectId); }
   uint32_t getCellObjectId(int row, int col) const
   {
      const TableRow *r = m_data.get(row);
      return (r != nullptr) ? r->getCellObjectId(col) : 0;
   }

   void setBaseRowAt(int row, int baseRow);
   void setBaseRow(int baseRow) { setBaseRowAt(getNumRows() - 1, baseRow); }
   int getBaseRow(int row) const
   {
      const TableRow *r = m_data.get(row);
      return (r != nullptr) ? r->getBaseRow() : 0;
   }

   void writeToTerminal() const;
   void dump(FILE *out, bool withHeader = true, TCHAR delimiter = _T(',')) const;
   void dump(const TCHAR *tag, int level, const TCHAR *prefix = _T(""), bool withHeader = true, TCHAR delimiter = _T(',')) const;

   json_t *toJson() const;
   json_t *toGrafanaJson() const;

   static Table *createFromXML(const char *xml);
   TCHAR *toXML() const;

   static Table *createFromPackedXML(const char *packedXml);
   char *toPackedXML() const;

   static Table *createFromCSV(const TCHAR *content, const TCHAR separator);
};

#ifdef _WIN32
template class LIBNETXMS_TEMPLATE_EXPORTABLE shared_ptr<Table>;
#endif

/**
 * Convert UNIX timestamp to JSON string in ISO 8601 format
 */
static inline json_t *json_time_string(time_t t)
{
   return (t == 0) ? json_null() : json_string(FormatISO8601Timestamp(t).c_str());
}

/**
 * Convert UNIX timestamp expressed in milliseconds to JSON string in ISO 8601 format
 */
static inline json_t *json_time_string_ms(int64_t t)
{
   return (t == 0) ? json_null() : json_string(FormatISO8601TimestampMs(t).c_str());
}

/**
 * Create JSON string with null check
 */
static inline json_t *json_string_a(const char *s)
{
   return (s != nullptr) ? json_string(s) : json_null();
}

/**
 * Create JSON string from wide character string
 */
static inline json_t *json_string_w(const WCHAR *s)
{
   if (s == nullptr)
      return json_null();
   char *us = UTF8StringFromWideString(s);
   json_t *js = json_string(us);
   MemFree(us);
   return js;
}

#ifdef UNICODE
#define json_string_t json_string_w
#else
#define json_string_t json_string_a
#endif

/**
 * Create JSON array from integer array
 */
template<typename T> static inline json_t *json_integer_array(const T *values, size_t size)
{
   json_t *a = json_array();
   for(size_t i = 0; i < size; i++)
      json_array_append_new(a, json_integer(values[i]));
   return a;
}

/**
 * Create JSON array from integer array
 */
template<typename T> static inline json_t *json_integer_array(const IntegerArray<T>& values)
{
   json_t *a = json_array();
   for(int i = 0; i < values.size(); i++)
      json_array_append_new(a, json_integer(values.get(i)));
   return a;
}

/**
 * Create JSON array from integer array
 */
template<typename T> static inline json_t *json_integer_array(const IntegerArray<T> *values)
{
   json_t *a = json_array();
   if (values != nullptr)
   {
      for(int i = 0; i < values->size(); i++)
         json_array_append_new(a, json_integer(values->get(i)));
   }
   return a;
}

/**
 * Serialize ObjectArray as JSON
 */
template<typename T> static inline json_t *json_object_array(const ObjectArray<T> *a)
{
   json_t *root = json_array();
   if (a != nullptr)
   {
      for(int i = 0; i < a->size(); i++)
      {
         T *e = a->get(i);
         if (e != nullptr)
            json_array_append_new(root, e->toJson());
      }
   }
   return root;
}

/**
 * Serialize ObjectArray as JSON
 */
template<typename T> static inline json_t *json_object_array(const ObjectArray<T>& a)
{
   return json_object_array<T>(&a);
}

/**
 * Serialize SharedObjectArray as JSON
 */
template<typename T> static inline json_t *json_object_array(const SharedObjectArray<T> *a)
{
   json_t *root = json_array();
   if (a != nullptr)
   {
      for(int i = 0; i < a->size(); i++)
      {
         T *e = a->get(i);
         if (e != nullptr)
            json_array_append_new(root, e->toJson());
      }
   }
   return root;
}

/**
 * Serialize SharedObjectArray as JSON
 */
template<typename T> static inline json_t *json_object_array(const SharedObjectArray<T>& a)
{
   return json_object_array<T>(&a);
}

/**
 * Serialize StructArray as JSON
 */
template<typename T> static inline json_t *json_struct_array(const StructArray<T> *a)
{
   json_t *root = json_array();
   if (a != nullptr)
   {
      for(int i = 0; i < a->size(); i++)
      {
         T *e = a->get(i);
         if (e != nullptr)
            json_array_append_new(root, e->toJson());
      }
   }
   return root;
}

/**
 * Serialize StructArray as JSON
 */
template<typename T> static inline json_t *json_struct_array(const StructArray<T>& a)
{
   return json_struct_array<T>(&a);
}

/**
 * Get string value from object
 */
static inline WCHAR *json_object_get_string_w(json_t *object, const char *tag, const WCHAR *defval)
{
   json_t *value = json_object_get(object, tag);
   return json_is_string(value) ? WideStringFromUTF8String(json_string_value(value)) : MemCopyStringW(defval);
}

/**
 * Get string value from object
 */
static inline char *json_object_get_string_a(json_t *object, const char *tag, const char *defval)
{
   json_t *value = json_object_get(object, tag);
   return json_is_string(value) ? MBStringFromUTF8String(json_string_value(value)) : MemCopyStringA(defval);
}

#ifdef UNICODE
#define json_object_get_string_t json_object_get_string_w
#else
#define json_object_get_string_t json_object_get_string_a
#endif

/**
 * Get string value from object
 */
static inline const char *json_object_get_string_utf8(json_t *object, const char *tag, const char *defval)
{
   json_t *value = json_object_get(object, tag);
   return json_is_string(value) ? json_string_value(value) : defval;
}

/**
 * Get integer value from object
 */
static inline int64_t json_object_get_int64(json_t *object, const char *tag, int64_t defval = 0)
{
   json_t *value = json_object_get(object, tag);
   return json_is_integer(value) ? json_integer_value(value) : defval;
}

/**
 * Get integer value from object
 */
static inline uint64_t json_object_get_uint64(json_t *object, const char *tag, uint64_t defval = 0)
{
   json_t *value = json_object_get(object, tag);
   return json_is_integer(value) ? static_cast<uint64_t>(json_integer_value(value)) : defval;
}

/**
 * Get integer value from object
 */
static inline int32_t json_object_get_int32(json_t *object, const char *tag, int32_t defval = 0)
{
   json_t *value = json_object_get(object, tag);
   return json_is_integer(value) ? static_cast<int32_t>(json_integer_value(value)) : defval;
}

/**
 * Get unsigned integer value from object
 */
static inline uint32_t json_object_get_uint32(json_t *object, const char *tag, uint32_t defval = 0)
{
   json_t *value = json_object_get(object, tag);
   return json_is_integer(value) ? static_cast<uint32_t>(json_integer_value(value)) : defval;
}

/**
 * Get boolean value from object
 */
static inline bool json_object_get_boolean(json_t *object, const char *tag, bool defval = false)
{
   json_t *value = json_object_get(object, tag);
   if (json_is_string(value))
   {
      const char *val = json_string_value(value);
      if (!stricmp(val, "true"))
         return true;
      if (!stricmp(val, "false"))
         return false;
   }
   return json_is_boolean(value) ? json_boolean_value(value) : (json_is_integer(value) ? (json_integer_value(value) != 0) : defval);
}

/**
 * Get UUID value from object
 */
uuid LIBNETXMS_EXPORTABLE json_object_get_uuid(json_t *object, const char *tag);

/**
 * Get time value from object
 */
time_t LIBNETXMS_EXPORTABLE json_object_get_time(json_t *object, const char *tag, time_t defval = 0);

/**
 * Get element from object by path (separated by /)
 */
json_t LIBNETXMS_EXPORTABLE *json_object_get_by_path_a(json_t *root, const char *path);

/**
 * Get element from object by path (separated by /)
 */
static inline json_t *json_object_get_by_path_w(json_t *root, const WCHAR *path)
{
   char utf8path[1024];
   wchar_to_utf8(path, -1, utf8path, 1024);
   utf8path[1023] = 0;
   return json_object_get_by_path_a(root, utf8path);
}

#ifdef UNICODE
#define json_object_get_by_path json_object_get_by_path_w
#else
#define json_object_get_by_path json_object_get_by_path_a
#endif

#if !HAVE_DECL_JSON_OBJECT_UPDATE_NEW

/**
 * Update json object with new object
 */
static inline int json_object_update_new(json_t *object, json_t *other)
{
    int ret = json_object_update(object, other);
    json_decref(other);
    return ret;
}

#endif /* HAVE_DECL_JSON_OBJECT_UPDATE_NEW */

/**
 * sockaddr buffer
 */
union SockAddrBuffer
{
   struct sockaddr sa;
   struct sockaddr_in sa4;
#ifdef WITH_IPV6
   struct sockaddr_in6 sa6;
#endif
};

/**
 * sockaddr length calculation
 */
#undef SA_LEN
#ifdef WITH_IPV6
#define SA_LEN(sa) (((sa)->sa_family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6))
#else
#define SA_LEN(sa) sizeof(struct sockaddr_in)
#endif

/**
 * Get port number from SockAddrBuffer
 */
#ifdef WITH_IPV6
#define SA_PORT(sa) ((((struct sockaddr *)sa)->sa_family == AF_INET) ? ((struct sockaddr_in *)sa)->sin_port : ((struct sockaddr_in6 *)sa)->sin6_port)
#else
#define SA_PORT(sa) (((struct sockaddr_in *)sa)->sin_port)
#endif

/**
 * Compare addresses in sockaddr
 */
static inline bool SocketAddressEquals(struct sockaddr *a1, struct sockaddr *a2)
{
   if (a1->sa_family != a2->sa_family)
      return false;
   if (a1->sa_family == AF_INET)
      return ((struct sockaddr_in *)a1)->sin_addr.s_addr == ((struct sockaddr_in *)a2)->sin_addr.s_addr;
#ifdef WITH_IPV6
   if (a1->sa_family == AF_INET6)
      return !memcmp(((struct sockaddr_in6 *)a1)->sin6_addr.s6_addr, ((struct sockaddr_in6 *)a2)->sin6_addr.s6_addr, 16);
#endif
   return false;
}

/**
 * MAC address notations
 */
enum class MacAddressNotation
{
   FLAT_STRING = 0,
   COLON_SEPARATED = 1,
   BYTEPAIR_COLON_SEPARATED = 2,
   HYPHEN_SEPARATED = 3,
   DOT_SEPARATED = 4,
   BYTEPAIR_DOT_SEPARATED = 5,
   DECIMAL_DOT_SEPARATED = 6
};

/**
 * Generic unique ID class for identifiers based on array of bytes (EUI, MAC address, etc.)
 */
template<uint16_t MaxLen> class alignas(8) GenericId
{
protected:
   uint16_t m_length;
   BYTE m_value[MaxLen];

public:
   GenericId(size_t length = 0)
   {
      memset(this, 0, sizeof(GenericId<MaxLen>));
      m_length = std::min(static_cast<uint16_t>(length), MaxLen);
   }
   GenericId(const BYTE *value, size_t length)
   {
      memset(this, 0, sizeof(GenericId<MaxLen>));
      m_length = std::min(static_cast<uint16_t>(length), MaxLen);
      memcpy(m_value, value, m_length);
   }
   GenericId(const GenericId& src)
   {
      memcpy(this, &src, sizeof(GenericId<MaxLen>));
   }

   GenericId& operator=(const GenericId& src)
   {
      memcpy(this, &src, sizeof(GenericId<MaxLen>));
      return *this;
   }

   const BYTE *value() const { return m_value; }
   size_t length() const { return m_length; }

   bool equals(const GenericId &a) const
   {
      return (a.m_length == m_length) ? memcmp(m_value, a.value(), m_length) == 0 : false;
   }
   bool equals(const BYTE *value, size_t length) const
   {
      return (static_cast<uint16_t>(length) == m_length) ? memcmp(m_value, value, m_length) == 0 : false;
   }

   bool isNull() const
   {
      for(int i = 0; i < static_cast<int>(m_length); i++)
         if (m_value[i] != 0)
            return false;
      return true;
   }
};

/**
 * MAC address
 */
class LIBNETXMS_EXPORTABLE MacAddress : public GenericId<8>
{
private:
   wchar_t *toStringInternalW(wchar_t *buffer, wchar_t separator, bool bytePair = false) const;
   wchar_t *toStringInternal3W(wchar_t *buffer, wchar_t separator) const;
   wchar_t *toStringInternalDecimalW(wchar_t *buffer, wchar_t separator) const;
   char *toStringInternalA(char *buffer, char separator, bool bytePair = false) const;
   char *toStringInternal3A(char *buffer, char separator) const;
   char *toStringInternalDecimalA(char *buffer, char separator) const;

public:
   MacAddress(size_t length = 0) : GenericId<8>(length) { }
   MacAddress(const BYTE *value, size_t length) : GenericId<8>(value, length) { }
   MacAddress(const MacAddress& src) : GenericId<8>(src) { }

   MacAddress& operator=(const MacAddress& src)
   {
      GenericId<8>::operator=(src);
      return *this;
   }

   static MacAddress parse(const char *str, bool partialMac = false);
   static MacAddress parse(const wchar_t *str, bool partialMac = false);

   bool isValid() const { return !isNull(); }
   bool isBroadcast() const;
   bool isMulticast() const { return (m_length == 6) ? ((m_value[0] & 0x01) != 0) && !isBroadcast() : false; }
   bool equals(const MacAddress &a) const { return GenericId<8>::equals(a); }
   bool equals(const BYTE *value, size_t length = 6) const { return GenericId<8>::equals(value, length); }

   String toString(MacAddressNotation notation = MacAddressNotation::COLON_SEPARATED) const;
   char *toStringA(char *buffer, MacAddressNotation notation = MacAddressNotation::COLON_SEPARATED) const;
   wchar_t *toStringW(wchar_t *buffer, MacAddressNotation notation = MacAddressNotation::COLON_SEPARATED) const;
   TCHAR *toString(TCHAR *buffer, MacAddressNotation notation = MacAddressNotation::COLON_SEPARATED) const
   {
#ifdef UNICODE
      return toStringW(buffer, notation);
#else
      return toStringA(buffer, notation);
#endif
   }

   json_t *toJson(MacAddressNotation notation = MacAddressNotation::COLON_SEPARATED) const
   {
      char buffer[32];
      return json_string(toStringA(buffer, notation));
   }

   static const MacAddress NONE;
   static const MacAddress ZERO;
};

/**
 * Calculate number of bits in IPv4 netmask (in host byte order)
 */
static inline int BitsInMask(uint32_t mask)
{
   int bits;
   for(bits = 0; mask != 0; bits++, mask <<= 1);
   return bits;
}

/**
 * Calculate number of bits in IP netmask (in network byte order)
 */
static inline int BitsInMask(const BYTE *mask, size_t size)
{
   int bits = 0;
   for(size_t i = 0; i < size; i++, bits += 8)
   {
      BYTE byte = mask[i];
      if (byte != 0xFF)
      {
         for(; byte != 0; bits++, byte <<= 1);
         break;
      }
   }
   return bits;
}

#define MAX_IP_ADDR_TEXT_LEN 40

/**
 * IP address
 */
class LIBNETXMS_EXPORTABLE InetAddress
{
private:
   short m_maskBits;
   short m_family;
   union
   {
      uint32_t v4;
      BYTE v6[16];
   } m_addr;

   static const InetAddress IPV4_LINK_LOCAL;
   static const InetAddress IPV6_LINK_LOCAL;

public:
   InetAddress()
   {
      m_family = AF_UNSPEC;
      m_maskBits = 0;
      memset(&m_addr, 0, sizeof(m_addr));
   }
   InetAddress(uint32_t addr)
   {
      m_family = AF_INET;
      memset(&m_addr, 0, sizeof(m_addr));
      m_addr.v4 = addr;
      m_maskBits = 32;
   }
   InetAddress(uint32_t addr, uint32_t mask)
   {
      m_family = AF_INET;
      memset(&m_addr, 0, sizeof(m_addr));
      m_addr.v4 = addr;
      m_maskBits = BitsInMask(mask);
   }
   InetAddress(const BYTE *addr, int maskBits = 128)
   {
      m_family = AF_INET6;
      memcpy(m_addr.v6, addr, 16);
      m_maskBits = maskBits;
   }

   bool isAnyLocal() const
   {
      return (m_family == AF_INET) ? (m_addr.v4 == 0) : !memcmp(m_addr.v6, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16);
   }
   bool isLoopback() const
   {
      return (m_family == AF_INET) ? ((m_addr.v4 & 0xFF000000) == 0x7F000000) : !memcmp(m_addr.v6, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01", 16);
   }
   bool isMulticast() const
   {
      return (m_family == AF_INET) ? ((m_addr.v4 >= 0xE0000000) && (m_addr.v4 != 0xFFFFFFFF)) : (m_addr.v6[0] == 0xFF);
   }
   bool isBroadcast() const
   {
      return (m_family == AF_INET) ? (m_addr.v4 == 0xFFFFFFFF) : false;
   }
   bool isLinkLocal() const
   {
      return (m_family == AF_INET) ? IPV4_LINK_LOCAL.contains(*this) : IPV6_LINK_LOCAL.contains(*this);
   }
   bool isValid() const
   {
      return m_family != AF_UNSPEC;
   }
   bool isValidUnicast() const
   {
      return isValid() && !isAnyLocal() && !isLoopback() && !isMulticast() && !isBroadcast() && !isLinkLocal();
   }

   int getFamily() const { return m_family; }
   uint32_t getAddressV4() const { return (m_family == AF_INET) ? m_addr.v4 : 0; }
   const BYTE *getAddressV6() const { return (m_family == AF_INET6) ? m_addr.v6 : (const BYTE *)"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"; }

   bool contains(const InetAddress &a) const;
   bool sameSubnet(const InetAddress &a) const;
   bool equals(const InetAddress &a) const;
   int compareTo(const InetAddress &a) const;
   bool inRange(const InetAddress& start, const InetAddress& end) const;

   void setMaskBits(int m) { m_maskBits = m; }
   int getMaskBits() const { return m_maskBits; }
   int getHostBits() const { return (m_family == AF_INET) ? (32 - m_maskBits) : (128 - m_maskBits); }

   InetAddress getSubnetAddress() const;
   InetAddress getSubnetBroadcast() const;
   bool isSubnetBroadcast(int maskBits) const;
   bool isSubnetBroadcast() const { return isSubnetBroadcast(m_maskBits); }

   String toString() const;
   TCHAR *toString(TCHAR *buffer) const;
#ifdef UNICODE
   char *toStringA(char *buffer) const;
#else
   char *toStringA(char *buffer) const { return toString(buffer); }
#endif

   void toOID(uint32_t *oid) const;

   json_t *toJson() const;

   BYTE *buildHashKey(BYTE *key) const;

   TCHAR *getHostByAddr(TCHAR *buffer, size_t buflen) const;

   struct sockaddr *fillSockAddr(SockAddrBuffer *buffer, uint16_t port = 0) const;

   static InetAddress resolveHostName(const WCHAR *hostname, int af = AF_UNSPEC);
   static InetAddress resolveHostName(const char *hostname, int af = AF_UNSPEC);
   static InetAddress parse(const WCHAR *str);
   static InetAddress parse(const char *str);
   static InetAddress parse(const WCHAR *addrStr, const WCHAR *maskStr);
   static InetAddress parse(const char *addrStr, const char *maskStr);
   static InetAddress createFromSockaddr(const struct sockaddr *s);

   static const InetAddress INVALID;
   static const InetAddress LOOPBACK;
   static const InetAddress NONE;
};

#ifdef _WIN32
template class LIBNETXMS_TEMPLATE_EXPORTABLE ObjectArray<InetAddress>;
#endif

/**
 * IP address list
 */
class LIBNETXMS_EXPORTABLE InetAddressList
{
private:
   ObjectArray<InetAddress> m_list;

   int indexOf(const InetAddress& addr) const;

public:
   InetAddressList() : m_list(0, 8, Ownership::True) { }
   InetAddressList(const InetAddressList& src) = delete;

   void add(const InetAddress& addr);
   void add(const InetAddressList& addrList);
   void replace(const InetAddress& addr);
   void remove(const InetAddress& addr);
   void clear() { m_list.clear(); }
   const InetAddress& get(int index) const { const InetAddress *a = m_list.get(index); return (a != nullptr) ? *a : InetAddress::INVALID; }

   int size() const { return m_list.size(); }
   bool isEmpty() const { return m_list.isEmpty(); }
   bool hasAddress(const InetAddress& addr) const { return indexOf(addr) != -1; }
   const InetAddress& findAddress(const InetAddress& addr) const { int idx = indexOf(addr); return (idx != -1) ? *m_list.get(idx) : InetAddress::INVALID; }
   const InetAddress& findSameSubnetAddress(const InetAddress& addr) const;
   const InetAddress& getFirstUnicastAddress() const;
   const InetAddress& getFirstUnicastAddressV4() const;
   bool hasValidUnicastAddress() const { return getFirstUnicastAddress().isValid(); }
   bool isLoopbackOnly() const;

   const ObjectArray<InetAddress>& getList() const { return m_list; }

   void fillMessage(NXCPMessage *msg, uint32_t baseFieldId, uint32_t sizeFieldId) const;

   json_t *toJson() const { return json_object_array(m_list); }
   String toString(const TCHAR *separator = _T(", ")) const;

   static InetAddressList *resolveHostName(const WCHAR *hostname);
   static InetAddressList *resolveHostName(const char *hostname);
};

/**
 * Network connection
 */
class LIBNETXMS_EXPORTABLE SocketConnection
{
protected:
	SOCKET m_socket;
	char m_data[4096];   // cached data
	size_t m_dataSize;   // size of cached data
	size_t m_dataReadPos;

   SocketConnection();

   virtual ssize_t readFromSocket(void *buffer, size_t size, uint32_t timeout);

public:
   SocketConnection(SOCKET s);
   SocketConnection(const SocketConnection& src) = delete;
	virtual ~SocketConnection();

	bool connectTCP(const TCHAR *hostName, uint16_t port, uint32_t timeout);
	bool connectTCP(const InetAddress& ip, uint16_t port, uint32_t timeout);
	void disconnect();

   ssize_t read(void *buffer, size_t size, uint32_t timeout = INFINITE);
   bool readFully(void *buffer, size_t size, uint32_t timeout = INFINITE);
   bool skip(size_t size, uint32_t timeout = INFINITE);
   bool skipBytes(BYTE value, uint32_t timeout = INFINITE);
	bool canRead(uint32_t timeout);
	bool waitForText(const char *text, uint32_t timeout) { return waitForData(text, strlen(text), timeout); }
   bool waitForData(const void *data, size_t size, uint32_t timeout);

   ssize_t write(const void *buffer, size_t size);
	bool writeLine(const char *line);

	static SocketConnection *createTCPConnection(const TCHAR *hostName, uint16_t port, uint32_t timeout);
};

/**
 * Telnet connection - handles all telnet negotiation
 */
class LIBNETXMS_EXPORTABLE TelnetConnection : public SocketConnection
{
protected:
   TelnetConnection() : SocketConnection() { }
   TelnetConnection(const TelnetConnection& src) = delete;

   virtual ssize_t readFromSocket(void *buffer, size_t size, uint32_t timeout) override;

   bool connect(const TCHAR *hostName, uint16_t port, uint32_t timeout);
   bool connect(const InetAddress& ip, uint16_t port, uint32_t timeout);

public:
	static TelnetConnection *createConnection(const TCHAR *hostName, uint16_t port, uint32_t timeout);
   static TelnetConnection *createConnection(const InetAddress& ip, uint16_t port, uint32_t timeout);

   ssize_t readLine(char *buffer, size_t size, uint32_t timeout = INFINITE);
};

#if defined(FD_SETSIZE) && (FD_SETSIZE <= 1024)
#define SOCKET_POLLER_MAX_SOCKETS FD_SETSIZE
#else
#define SOCKET_POLLER_MAX_SOCKETS 1024
#endif

/**
 * Socket poller
 */
class LIBNETXMS_EXPORTABLE SocketPoller
{
private:
   bool m_write;
   int m_count;
#if HAVE_POLL
   struct pollfd m_sockets[SOCKET_POLLER_MAX_SOCKETS];
#else
   bool m_invalidDescriptor;
   fd_set m_rwDescriptors;
   fd_set m_exDescriptors;
#ifndef _WIN32
   SOCKET m_maxfd;
#endif
#endif

public:
   SocketPoller(bool write = false)
   {
      m_write = write;
      m_count = 0;
#if !HAVE_POLL
      m_invalidDescriptor = false;
      FD_ZERO(&m_rwDescriptors);
      FD_ZERO(&m_exDescriptors);
#ifndef _WIN32
      m_maxfd = 0;
#endif
#endif
   }
   SocketPoller(const SocketPoller& src) = delete;

   bool add(SOCKET s);
   int poll(uint32_t timeout);
   bool isSet(SOCKET s);
   bool isReady(SOCKET s);
   bool isError(SOCKET s);
   void reset()
   {
      m_count = 0;
#if !HAVE_POLL
      FD_ZERO(&m_rwDescriptors);
      FD_ZERO(&m_exDescriptors);
#ifndef _WIN32
      m_maxfd = 0;
#endif
#endif
   }

   bool hasInvalidDescriptor() const
   {
#if HAVE_POLL
      return false;
#else
      return m_invalidDescriptor;
#endif
   }
};

/**
 * Result of background socket poll
 */
enum class BackgroundSocketPollResult
{
   SUCCESS = 0,
   TIMEOUT = 1,
   FAILURE = 2,
   SHUTDOWN = 3,
   CANCELLED = 4
};

/**
 * Backgroud socket poll request
 */
struct BackgroundSocketPollRequest
{
   BackgroundSocketPollRequest *next;
   SOCKET socket;
   void (*callback)(BackgroundSocketPollResult, SOCKET, void*);
   void *context;
   int64_t queueTime;
   uint32_t timeout;
   bool cancelled;
};

#ifdef _WIN32
template class LIBNETXMS_TEMPLATE_EXPORTABLE SynchronizedObjectMemoryPool<BackgroundSocketPollRequest>;
#endif

/**
 * Background socket poller
 */
class LIBNETXMS_EXPORTABLE BackgroundSocketPoller
{
private:
   SynchronizedObjectMemoryPool<BackgroundSocketPollRequest> m_memoryPool;
   THREAD m_workerThread;
   uint32_t m_workerThreadId;
   SOCKET m_controlSockets[2];
   Mutex m_mutex;
   BackgroundSocketPollRequest *m_head;
   bool m_shutdown;

   void workerThread();
   void notifyWorkerThread(char command = 'W');

public:
   BackgroundSocketPoller();
   BackgroundSocketPoller(const BackgroundSocketPoller& src) = delete;
   ~BackgroundSocketPoller();

   void poll(SOCKET socket, uint32_t timeout, void (*callback)(BackgroundSocketPollResult, SOCKET, void*), void *context);
   template<typename C> void poll(SOCKET socket, uint32_t timeout, void (*callback)(BackgroundSocketPollResult, SOCKET, C*), C *context)
   {
      poll(socket, timeout, reinterpret_cast<void (*)(BackgroundSocketPollResult, SOCKET, void*)>(callback), context);
   }
   void cancel(SOCKET socket);
   void shutdown();

   bool isValid() const { return (m_controlSockets[0] != INVALID_SOCKET) && (m_workerThread != INVALID_THREAD_HANDLE); }
};

/**
 * Handle for background socket poller to track poller usage
 */
struct BackgroundSocketPollerHandle
{
   BackgroundSocketPoller poller;
   VolatileCounter usageCount;

   BackgroundSocketPollerHandle()
   {
      usageCount = 0;
   }
};

/**
 * Abstract communication channel (forward declaration for poll wrapper)
 */
class AbstractCommChannel;

/**
 * Wrapper data for background poller with shared pointer to context
 */
template<typename C> struct AbstractCommChannel_PollWrapperData
{
   shared_ptr<C> context;
   void (*callback)(BackgroundSocketPollResult, AbstractCommChannel*, const shared_ptr<C>&);

   AbstractCommChannel_PollWrapperData(void (*_callback)(BackgroundSocketPollResult, AbstractCommChannel*, const shared_ptr<C>&), const shared_ptr<C>& _context) : context(_context)
   {
      callback = _callback;
   }
};

/**
 * Wrapper for background poller with shared pointer to context
 */
template<typename C> void AbstractCommChannel_PollWrapperCallback(BackgroundSocketPollResult result, AbstractCommChannel *channel, AbstractCommChannel_PollWrapperData<C> *context)
{
   context->callback(result, channel, context->context);
   delete context;
}

/**
 * Abstract communication channel
 */
class LIBNETXMS_EXPORTABLE AbstractCommChannel
{
public:
   AbstractCommChannel();
   AbstractCommChannel(const AbstractCommChannel& src) = delete;
   virtual ~AbstractCommChannel();

   virtual ssize_t send(const void *data, size_t size, Mutex* mutex = nullptr) = 0;
   virtual ssize_t recv(void *buffer, size_t size, uint32_t timeout = INFINITE) = 0;
   virtual int poll(uint32_t timeout, bool write = false) = 0;
   virtual int shutdown() = 0;
   virtual void close() = 0;

   virtual void backgroundPoll(uint32_t timeout, void (*callback)(BackgroundSocketPollResult, AbstractCommChannel*, void*), void *context) = 0;
   template<typename C> void backgroundPoll(uint32_t timeout, void (*callback)(BackgroundSocketPollResult, AbstractCommChannel*, C*), C *context)
   {
      backgroundPoll(timeout, reinterpret_cast<void (*)(BackgroundSocketPollResult, AbstractCommChannel*, void*)>(callback), reinterpret_cast<void*>(context));
   }
   template<typename C> void backgroundPoll(uint32_t timeout, void (*callback)(BackgroundSocketPollResult, AbstractCommChannel*, const shared_ptr<C>&), const shared_ptr<C>& context)
   {
      backgroundPoll(timeout, AbstractCommChannel_PollWrapperCallback<C>, new AbstractCommChannel_PollWrapperData<C>(callback, context));
   }
};

#ifdef _WIN32
template class LIBNETXMS_TEMPLATE_EXPORTABLE shared_ptr<AbstractCommChannel>;
#endif

/**
 * Socket communication channel
 */
class LIBNETXMS_EXPORTABLE SocketCommChannel : public AbstractCommChannel
{
private:
   SOCKET m_socket;
   bool m_owner;
#ifndef _WIN32
   int m_controlPipe[2];
#endif
   BackgroundSocketPollerHandle *m_socketPoller;

public:
   SocketCommChannel(SOCKET socket, BackgroundSocketPollerHandle *socketPoller = nullptr, Ownership owner = Ownership::True);
   SocketCommChannel(const SocketCommChannel& src) = delete;
   virtual ~SocketCommChannel();

   virtual ssize_t send(const void *data, size_t size, Mutex* mutex = nullptr) override;
   virtual ssize_t recv(void *buffer, size_t size, uint32_t timeout = INFINITE) override;
   virtual int poll(uint32_t timeout, bool write = false) override;
   virtual void backgroundPoll(uint32_t timeout, void (*callback)(BackgroundSocketPollResult, AbstractCommChannel*, void*), void *context) override;
   virtual int shutdown() override;
   virtual void close() override;
};

/**
 * Code translation structure
 */
struct CodeLookupElement
{
   int32_t code;
   const TCHAR *text;
};

/**
 * Store for shared_ptr with synchronized access
 */
template<typename T> class shared_ptr_store
{
private:
   shared_ptr<T> pointer;
   Mutex mutex;

public:
   shared_ptr_store() : mutex(MutexType::FAST)
   {
   }

   shared_ptr_store(const shared_ptr<T>& p) : pointer(p), mutex(MutexType::FAST)
   {
   }

   ~shared_ptr_store()
   {
   }

   operator shared_ptr<T>()
   {
      return get();
   }

   T* operator-> ()
   {
      return get().operator->();
   }

   shared_ptr<T> get()
   {
      mutex.lock();
      auto p = pointer;
      mutex.unlock();
      return p;
   }

   void set(const shared_ptr<T>& p)
   {
      mutex.lock();
      pointer = p;
      mutex.unlock();
   }
};

/**
 * Configuration item template for configuration loader
 */
struct NX_CFG_TEMPLATE
{
   TCHAR token[64];
   BYTE type;
   BYTE separator;         // Separator character for lists
   uint16_t listElements;  // Number of list elements, should be set to 0 before calling NxLoadConfig()
   uint64_t bufferSize;    // Buffer size for strings or flag to be set for CT_BOOLEAN
   uint32_t bufferPos;     // Should be set to 0
   void *buffer;
   void *overrideIndicator;
};

//
// Structures for opendir() / readdir() / closedir()
//

#ifdef _WIN32

#define DT_UNKNOWN   0
#define DT_REG       1
#define DT_DIR       2

typedef struct dirent
{
   long            d_ino;  /* inode number (not used by MS-DOS) */
   unsigned char   d_type; /* file type */
   int             d_namlen;       /* Name length */
   char            d_name[MAX_PATH];    /* file name */
} _DIRECT;

typedef struct _dir_struc
{
   HANDLE handle;
   struct dirent dirstr; /* Directory structure to return */
} DIR;

#ifdef UNICODE

typedef struct dirent_w
{
   long            d_ino;  /* inode number (not used by MS-DOS) */
   unsigned char   d_type; /* file type */
   int             d_namlen;       /* Name length */
   WCHAR           d_name[MAX_PATH];    /* file name */
} _DIRECTW;

typedef struct _dir_struc_w
{
   HANDLE handle;
   struct dirent_w dirstr; /* Directory structure to return */
} DIRW;

#define _TDIR DIRW
#define _TDIRECT _DIRECTW
#define _tdirent dirent_w

#else

#define _TDIR DIR
#define _TDIRECT _DIRECT
#define _tdirent dirent

#endif

#else	/* not _WIN32 */

typedef struct dirent_w
{
   long            d_ino;  /* inode number */
#if HAVE_DIRENT_D_TYPE
   unsigned char   d_type; /* file type */
#endif
   WCHAR           d_name[257];    /* file name */
} _DIRECTW;

typedef struct _dir_struc_w
{
   DIR *dir;               /* Original non-unicode structure */
   struct dirent_w dirstr; /* Directory structure to return */
} DIRW;

#endif   /* _WIN32 */

//
// Functions
//

static inline const TCHAR *BooleanToString(bool v)
{
   return v ? _T("true") : _T("false");
}

int LIBNETXMS_EXPORTABLE ConnectEx(SOCKET s, struct sockaddr *addr, int len, uint32_t timeout, bool *isTimeout = nullptr);
ssize_t LIBNETXMS_EXPORTABLE SendEx(SOCKET hSocket, const void *data, size_t len, int flags, Mutex* mutex);
ssize_t LIBNETXMS_EXPORTABLE RecvEx(SOCKET hSocket, void *data, size_t len, int flags, uint32_t timeout, SOCKET controlSocket = INVALID_SOCKET);
bool LIBNETXMS_EXPORTABLE RecvAll(SOCKET s, void *buffer, size_t size, uint32_t timeout);

static inline bool SocketCanRead(SOCKET hSocket, uint32_t timeout)
{
   SocketPoller sp;
   sp.add(hSocket);
   return sp.poll(timeout) > 0;
}

static inline bool SocketCanWrite(SOCKET hSocket, uint32_t timeout)
{
   SocketPoller sp(true);
   sp.add(hSocket);
   return sp.poll(timeout) > 0;
}

SOCKET LIBNETXMS_EXPORTABLE ConnectToHost(const InetAddress& addr, uint16_t port, uint32_t timeout);
SOCKET LIBNETXMS_EXPORTABLE ConnectToHostUDP(const InetAddress& addr, uint16_t port);

TCHAR LIBNETXMS_EXPORTABLE *IpToStr(uint32_t ipAddr, TCHAR *buffer);
#ifdef UNICODE
char LIBNETXMS_EXPORTABLE *IpToStrA(uint32_t ipAddr, char *buffer);
#else
#define IpToStrA IpToStr
#endif
TCHAR LIBNETXMS_EXPORTABLE *Ip6ToStr(const BYTE *addr, TCHAR *buffer);
#ifdef UNICODE
char LIBNETXMS_EXPORTABLE *Ip6ToStrA(const BYTE *addr, char *buffer);
#else
#define Ip6ToStrA Ip6ToStr
#endif
TCHAR LIBNETXMS_EXPORTABLE *SockaddrToStr(struct sockaddr *addr, TCHAR *buffer);

void LIBNETXMS_EXPORTABLE InitNetXMSProcess(bool commandLineTool, bool isClientApp = false);
void LIBNETXMS_EXPORTABLE InitiateProcessShutdown();
bool LIBNETXMS_EXPORTABLE SleepAndCheckForShutdown(uint32_t seconds);
bool LIBNETXMS_EXPORTABLE SleepAndCheckForShutdownEx(uint32_t milliseconds);
bool LIBNETXMS_EXPORTABLE IsShutdownInProgress();
Condition LIBNETXMS_EXPORTABLE *GetShutdownConditionObject();

#ifdef _WIN32
void LIBNETXMS_EXPORTABLE EnableFatalExitOnCRTError(bool enable);
#endif

const TCHAR LIBNETXMS_EXPORTABLE *CodeToText(int32_t code, CodeLookupElement *lookupTable, const TCHAR *defaultText = _T("Unknown"));
int LIBNETXMS_EXPORTABLE CodeFromText(const TCHAR *text, CodeLookupElement *lookupTable, int32_t defaultCode = -1);

#ifndef _WIN32
#if defined(UNICODE_UCS2) || defined(UNICODE_UCS4)
void LIBNETXMS_EXPORTABLE __wcsupr(WCHAR *in);
#define wcsupr __wcsupr
#endif
void LIBNETXMS_EXPORTABLE __strupr(char *in);
#define strupr __strupr
#endif   /* _WIN32 */

// Use internally the following quick sort prototype (based on Windows version of qsort_s):
// void qsort_s(void *base, size_t nmemb, size_t size, int (*compare)(void *, const void *, const void *), void *context);
#if defined(_WIN32)
#define QSort qsort_s
#else
void LIBNETXMS_EXPORTABLE QSort(void *base, size_t nmemb, size_t size, int (*compare)(void *, const void *, const void *), void *context);
#endif

uint64_t LIBNETXMS_EXPORTABLE FileSizeW(const wchar_t *pszFileName);
uint64_t LIBNETXMS_EXPORTABLE FileSizeA(const char *pszFileName);
#ifdef UNICODE
#define FileSize FileSizeW
#else
#define FileSize FileSizeA
#endif

WCHAR LIBNETXMS_EXPORTABLE *BinToStrExW(const void *data, size_t size, WCHAR *str, WCHAR separator, size_t padding);
char LIBNETXMS_EXPORTABLE *BinToStrExA(const void *data, size_t size, char *str, char separator, size_t padding);
#ifdef UNICODE
#define BinToStrEx BinToStrExW
#else
#define BinToStrEx BinToStrExA
#endif

static inline WCHAR *BinToStrW(const void *data, size_t size, WCHAR *str)
{
   return BinToStrExW(data, size, str, 0, 0);
}
static inline char *BinToStrA(const void *data, size_t size, char *str)
{
   return BinToStrExA(data, size, str, 0, 0);
}
#ifdef UNICODE
#define BinToStr BinToStrW
#else
#define BinToStr BinToStrA
#endif

WCHAR LIBNETXMS_EXPORTABLE *BinToStrExWL(const void *data, size_t size, WCHAR *str, WCHAR separator, size_t padding);
char LIBNETXMS_EXPORTABLE *BinToStrExAL(const void *data, size_t size, char *str, char separator, size_t padding);
#ifdef UNICODE
#define BinToStrExL BinToStrExWL
#else
#define BinToStrExL BinToStrExAL
#endif

static inline WCHAR *BinToStrWL(const void *data, size_t size, WCHAR *str)
{
   return BinToStrExWL(data, size, str, 0, 0);
}
static inline char *BinToStrAL(const void *data, size_t size, char *str)
{
   return BinToStrExAL(data, size, str, 0, 0);
}
#ifdef UNICODE
#define BinToStrL BinToStrWL
#else
#define BinToStrL BinToStrAL
#endif

size_t LIBNETXMS_EXPORTABLE StrToBinW(const WCHAR *pStr, uint8_t *data, size_t size);
size_t LIBNETXMS_EXPORTABLE StrToBinA(const char *pStr, uint8_t *data, size_t size);
#ifdef UNICODE
#define StrToBin StrToBinW
#else
#define StrToBin StrToBinA
#endif

TCHAR LIBNETXMS_EXPORTABLE *MACToStr(const uint8_t *data, TCHAR *str);

const char LIBNETXMS_EXPORTABLE *ExtractWordA(const char *line, char *buffer, int index = 0);
const WCHAR LIBNETXMS_EXPORTABLE *ExtractWordW(const WCHAR *line, WCHAR *buffer, int index = 0);
#ifdef UNICODE
#define ExtractWord ExtractWordW
#else
#define ExtractWord ExtractWordA
#endif

int LIBNETXMS_EXPORTABLE NumCharsA(const char *str, char ch);
int LIBNETXMS_EXPORTABLE NumCharsW(const WCHAR *str, WCHAR ch);
#ifdef UNICODE
#define NumChars NumCharsW
#else
#define NumChars NumCharsA
#endif

void LIBNETXMS_EXPORTABLE RemoveTrailingCRLFA(char *str);
void LIBNETXMS_EXPORTABLE RemoveTrailingCRLFW(WCHAR *str);
#ifdef UNICODE
#define RemoveTrailingCRLF RemoveTrailingCRLFW
#else
#define RemoveTrailingCRLF RemoveTrailingCRLFA
#endif

bool LIBNETXMS_EXPORTABLE RegexpMatchA(const char *str, const char *expr, bool matchCase);
bool LIBNETXMS_EXPORTABLE RegexpMatchW(const WCHAR *str, const WCHAR *expr, bool matchCase);
#ifdef UNICODE
#define RegexpMatch RegexpMatchW
#else
#define RegexpMatch RegexpMatchA
#endif

size_t LIBNETXMS_EXPORTABLE CalculateLevenshteinDistance(const TCHAR *s1, size_t len1, const TCHAR *s2, size_t len2, bool ignoreCase);
static inline size_t CalculateLevenshteinDistance(const TCHAR *s1, const TCHAR *s2, bool ignoreCase = false)
{
   size_t len1 = _tcslen(s1);
   size_t len2 = _tcslen(s2);
   return CalculateLevenshteinDistance(s1, len1, s2, len2, ignoreCase);
}
double LIBNETXMS_EXPORTABLE CalculateStringSimilarity(const TCHAR *s1, const TCHAR *s2, bool ignoreCase);
bool LIBNETXMS_EXPORTABLE FuzzyMatchStrings(const TCHAR *s1, const TCHAR *s2, double threshold = 0.8);
bool LIBNETXMS_EXPORTABLE FuzzyMatchStringsIgnoreCase(const TCHAR *s1, const TCHAR *s2, double threshold = 0.8);

const TCHAR LIBNETXMS_EXPORTABLE *ExpandFileName(const TCHAR *name, TCHAR *buffer, size_t bufSize, bool allowShellCommands);
String LIBNETXMS_EXPORTABLE ShortenFilePathForDisplay(const TCHAR *path, size_t maxLen);

bool LIBNETXMS_EXPORTABLE CreateDirectoryTree(const TCHAR *path);
bool LIBNETXMS_EXPORTABLE DeleteDirectoryTree(const TCHAR *path);
bool LIBNETXMS_EXPORTABLE SetLastModificationTime(TCHAR *fileName, time_t lastModDate);
bool LIBNETXMS_EXPORTABLE CopyFileOrDirectory(const TCHAR *oldName, const TCHAR *newName);
bool LIBNETXMS_EXPORTABLE MoveFileOrDirectory(const TCHAR *oldName, const TCHAR *newName);
bool LIBNETXMS_EXPORTABLE MergeFiles(const TCHAR *source, const TCHAR *destination);

bool LIBNETXMS_EXPORTABLE VerifyFileSignature(const TCHAR *file);

bool LIBNETXMS_EXPORTABLE MatchStringA(const char *pattern, const char *str, bool matchCase);
bool LIBNETXMS_EXPORTABLE MatchStringW(const WCHAR *pattern, const WCHAR *str, bool matchCase);
#ifdef UNICODE
#define MatchString MatchStringW
#else
#define MatchString MatchStringA
#endif

char LIBNETXMS_EXPORTABLE *TrimA(char *str);
WCHAR LIBNETXMS_EXPORTABLE *TrimW(WCHAR *str);
#ifdef UNICODE
#define Trim TrimW
#else
#define Trim TrimA
#endif

TCHAR LIBNETXMS_EXPORTABLE **SplitString(const TCHAR *source, TCHAR sep, int *numStrings, bool mergeSeparators = false);
int LIBNETXMS_EXPORTABLE GetLastMonthDay(struct tm *currTime);
bool LIBNETXMS_EXPORTABLE MatchScheduleElement(TCHAR *pszPattern, int nValue, int maxValue, struct tm *localTime, time_t currTime, bool checkSeconds);
bool LIBNETXMS_EXPORTABLE MatchSchedule(const TCHAR *schedule, bool *withSeconds, struct tm *currTime, time_t now);

BOOL LIBNETXMS_EXPORTABLE IsValidObjectName(const TCHAR *pszName, BOOL bExtendedChars = FALSE);
BOOL LIBNETXMS_EXPORTABLE IsValidScriptName(const TCHAR *pszName);
/* deprecated */ void LIBNETXMS_EXPORTABLE TranslateStr(TCHAR *pszString, const TCHAR *pszSubStr, const TCHAR *pszReplace);
const TCHAR LIBNETXMS_EXPORTABLE *GetCleanFileName(const TCHAR *fileName);
void LIBNETXMS_EXPORTABLE GetOSVersionString(TCHAR *pszBuffer, int nBufSize);

uint32_t LIBNETXMS_EXPORTABLE CalculateCRC32(const BYTE *data, size_t size, uint32_t crc);
bool LIBNETXMS_EXPORTABLE CalculateFileCRC32(const TCHAR *fileName, uint32_t *result);

void LIBNETXMS_EXPORTABLE CalculateMD5Hash(const void *data, size_t size, BYTE *hash);
void LIBNETXMS_EXPORTABLE MD5HashForPattern(const void *data, size_t patternSize, size_t fullSize, BYTE *hash);
bool LIBNETXMS_EXPORTABLE CalculateFileMD5Hash(const TCHAR *fileName, BYTE *hash, int64_t size = 0);

void LIBNETXMS_EXPORTABLE CalculateSHA1Hash(const void *data, size_t size, BYTE *hash);
void LIBNETXMS_EXPORTABLE SHA1HashForPattern(const void *data, size_t patternSize, size_t fullSize, BYTE *hash);
bool LIBNETXMS_EXPORTABLE CalculateFileSHA1Hash(const TCHAR *fileName, BYTE *hash, int64_t size = 0);

void LIBNETXMS_EXPORTABLE CalculateSHA224Hash(const void *data, size_t size, BYTE *hash);
void LIBNETXMS_EXPORTABLE SHA224HashForPattern(const void *data, size_t patternSize, size_t fullSize, BYTE *hash);
bool LIBNETXMS_EXPORTABLE CalculateFileSHA224Hash(const TCHAR *fileName, BYTE *hash, int64_t size = 0);

void LIBNETXMS_EXPORTABLE CalculateSHA256Hash(const void *data, size_t size, BYTE *hash);
void LIBNETXMS_EXPORTABLE SHA256HashForPattern(const void *data, size_t patternSize, size_t fullSize, BYTE *hash);
bool LIBNETXMS_EXPORTABLE CalculateFileSHA256Hash(const TCHAR *fileName, BYTE *hash, int64_t size = 0);

void LIBNETXMS_EXPORTABLE CalculateSHA384Hash(const void *data, size_t size, BYTE *hash);
void LIBNETXMS_EXPORTABLE SHA384HashForPattern(const void *data, size_t patternSize, size_t fullSize, BYTE *hash);
bool LIBNETXMS_EXPORTABLE CalculateFileSHA384Hash(const TCHAR *fileName, BYTE *hash, int64_t size = 0);

void LIBNETXMS_EXPORTABLE CalculateSHA512Hash(const void *data, size_t size, BYTE *hash);
void LIBNETXMS_EXPORTABLE SHA512HashForPattern(const void *data, size_t patternSize, size_t fullSize, BYTE *hash);
bool LIBNETXMS_EXPORTABLE CalculateFileSHA512Hash(const TCHAR *fileName, BYTE *hash, int64_t size = 0);

void LIBNETXMS_EXPORTABLE SignMessage(const void *message, size_t mlen, const BYTE *key, size_t klen, BYTE *signature);
bool LIBNETXMS_EXPORTABLE ValidateMessageSignature(const void *message, size_t mlen, const BYTE *key, size_t klen, const BYTE *signature);

void LIBNETXMS_EXPORTABLE GenerateRandomBytes(BYTE *buffer, size_t size);
int32_t LIBNETXMS_EXPORTABLE GenerateRandomNumber(int32_t minValue, int32_t maxValue);

void LIBNETXMS_EXPORTABLE LogOpenSSLErrorStack(int level);

void LIBNETXMS_EXPORTABLE ICEEncryptData(const BYTE *in, size_t inLen, BYTE *out, const BYTE *key);
void LIBNETXMS_EXPORTABLE ICEDecryptData(const BYTE *in, size_t inLen, BYTE *out, const BYTE *key);

bool LIBNETXMS_EXPORTABLE DecryptPasswordW(const WCHAR *login, const WCHAR *encryptedPasswd, WCHAR *decryptedPasswd, size_t bufferLenght);
bool LIBNETXMS_EXPORTABLE DecryptPasswordA(const char *login, const char *encryptedPasswd, char *decryptedPasswd, size_t bufferLenght);
#ifdef UNICODE
#define DecryptPassword DecryptPasswordW
#else
#define DecryptPassword DecryptPasswordA
#endif

HMODULE LIBNETXMS_EXPORTABLE DLOpen(const TCHAR *libName, TCHAR *errorText);
HMODULE LIBNETXMS_EXPORTABLE DLOpenEx(const TCHAR *libName, bool global, TCHAR *errorText);
void LIBNETXMS_EXPORTABLE DLClose(HMODULE hModule);
void LIBNETXMS_EXPORTABLE *DLGetSymbolAddr(HMODULE hModule, const char *symbol, TCHAR *errorText);

#ifdef _WIN32

TCHAR LIBNETXMS_EXPORTABLE *GetSystemErrorText(uint32_t error, TCHAR *buffer, size_t size);
String LIBNETXMS_EXPORTABLE GetSystemErrorText(uint32_t error);
bool LIBNETXMS_EXPORTABLE GetWindowsVersionString(TCHAR *versionString, size_t size);
void LIBNETXMS_EXPORTABLE WindowsProductNameFromVersion(OSVERSIONINFOEX *ver, TCHAR *buffer);
int64_t LIBNETXMS_EXPORTABLE GetProcessRSS();

#endif

TCHAR LIBNETXMS_EXPORTABLE *GetSocketErrorText(uint32_t errorCode, TCHAR *buffer, size_t size);
TCHAR LIBNETXMS_EXPORTABLE *GetLastSocketErrorText(TCHAR *buffer, size_t size);

TCHAR LIBNETXMS_EXPORTABLE *GetLocalHostName(TCHAR *buffer, size_t size, bool fqdn);

#if !HAVE_DAEMON || !HAVE_DECL_DAEMON
int LIBNETXMS_EXPORTABLE __daemon(int nochdir, int noclose);
#define daemon __daemon
#endif

#ifdef _WIN32

#define nx_wprintf wprintf
#define nx_fwprintf fwprintf
#define nx_swprintf swprintf
#define nx_vwprintf vwprintf
#define nx_vfwprintf vfwprintf
#define nx_vswprintf vswprintf

#else

bool LIBNETXMS_EXPORTABLE SetDefaultCodepage(const char *cp);

DWORD LIBNETXMS_EXPORTABLE GetEnvironmentVariable(const TCHAR *var, TCHAR *buffer, DWORD size);
BOOL LIBNETXMS_EXPORTABLE SetEnvironmentVariable(const TCHAR *var, const TCHAR *value);

#ifdef UNICODE
int LIBNETXMS_EXPORTABLE nx_wprintf(const wchar_t *format, ...);
int LIBNETXMS_EXPORTABLE nx_fwprintf(FILE *fp, const wchar_t *format, ...);
int LIBNETXMS_EXPORTABLE nx_swprintf(wchar_t *buffer, size_t size, const wchar_t *format, ...);
int LIBNETXMS_EXPORTABLE nx_vwprintf(const wchar_t *format, va_list args);
int LIBNETXMS_EXPORTABLE nx_vfwprintf(FILE *fp, const wchar_t *format, va_list args);
int LIBNETXMS_EXPORTABLE nx_vswprintf(wchar_t *buffer, size_t size, const wchar_t *format, va_list args);

int LIBNETXMS_EXPORTABLE nx_wscanf(const wchar_t *format, ...);
int LIBNETXMS_EXPORTABLE nx_fwscanf(FILE *fp, const wchar_t *format, ...);
int LIBNETXMS_EXPORTABLE nx_swscanf(const wchar_t *str, const wchar_t *format, ...);
int LIBNETXMS_EXPORTABLE nx_vwscanf(const wchar_t *format, va_list args);
int LIBNETXMS_EXPORTABLE nx_vfwscanf(FILE *fp, const wchar_t *format, va_list args);
int LIBNETXMS_EXPORTABLE nx_vswscanf(const wchar_t *str, const wchar_t *format, va_list args);
#endif

#endif	/* _WIN32 */

#ifdef _WITH_ENCRYPTION
wchar_t LIBNETXMS_EXPORTABLE *ERR_error_string_W(int errorCode, wchar_t *buffer);
#endif

#if defined(UNICODE) && !defined(_WIN32)

int LIBNETXMS_EXPORTABLE _wopen(const wchar_t *_name, int flags, ...);
int LIBNETXMS_EXPORTABLE _wchmod(const wchar_t *_name, int mode);
int LIBNETXMS_EXPORTABLE _wmkdir(const wchar_t *_path, int mode);
int LIBNETXMS_EXPORTABLE _wrmdir(const wchar_t *_path);
int LIBNETXMS_EXPORTABLE _wrename(const wchar_t *_oldpath, const wchar_t *_newpath);
int LIBNETXMS_EXPORTABLE _wremove(const wchar_t *_path);
int LIBNETXMS_EXPORTABLE _waccess(const wchar_t *_path, int mode);
wchar_t LIBNETXMS_EXPORTABLE *_wcserror(int errnum);
#if HAVE_STRERROR_R
#if HAVE_POSIX_STRERROR_R
int LIBNETXMS_EXPORTABLE wcserror_r(int errnum, wchar_t *strerrbuf, size_t buflen);
#else
wchar_t LIBNETXMS_EXPORTABLE *wcserror_r(int errnum, wchar_t *strerrbuf, size_t buflen);
#endif
#endif

#endif	/* UNICODE && !_WIN32*/

#if !HAVE_STRTOLL
INT64 LIBNETXMS_EXPORTABLE strtoll(const char *nptr, char **endptr, int base);
#endif
#if !HAVE_STRTOULL
UINT64 LIBNETXMS_EXPORTABLE strtoull(const char *nptr, char **endptr, int base);
#endif

#if !HAVE_WCSTOLL
INT64 LIBNETXMS_EXPORTABLE wcstoll(const WCHAR *nptr, WCHAR **endptr, int base);
#endif
#if !HAVE_WCSTOULL
UINT64 LIBNETXMS_EXPORTABLE wcstoull(const WCHAR *nptr, WCHAR **endptr, int base);
#endif

#if !HAVE_STRLWR && !defined(_WIN32)
char LIBNETXMS_EXPORTABLE *strlwr(char *str);
#endif

#if !HAVE_WCSLWR && !defined(_WIN32)
WCHAR LIBNETXMS_EXPORTABLE *wcslwr(WCHAR *str);
#endif

#if !HAVE_STRLCPY
size_t LIBNETXMS_EXPORTABLE strlcpy(char *dst, const char *src, size_t size);
#endif

#if !HAVE_WCSLCPY
size_t LIBNETXMS_EXPORTABLE wcslcpy(WCHAR *dst, const WCHAR *src, size_t size);
#endif

#if !HAVE_STRLCAT
size_t LIBNETXMS_EXPORTABLE strlcat(char *dst, const char *src, size_t size);
#endif

#if !HAVE_WCSLCAT
size_t LIBNETXMS_EXPORTABLE wcslcat(WCHAR *dst, const WCHAR *src, size_t size);
#endif

#if !HAVE_WCSCASECMP && !defined(_WIN32)
int LIBNETXMS_EXPORTABLE wcscasecmp(const wchar_t *s1, const wchar_t *s2);
#endif

#if !HAVE_WCSNCASECMP && !defined(_WIN32)
int LIBNETXMS_EXPORTABLE wcsncasecmp(const wchar_t *s1, const wchar_t *s2, size_t n);
#endif

#if !HAVE_STRCASESTR
char LIBNETXMS_EXPORTABLE *strcasestr(const char *s, const char *ss);
#endif

#if !HAVE_WCSCASESTR
WCHAR LIBNETXMS_EXPORTABLE *wcscasestr(const WCHAR *s, const WCHAR *ss);
#endif

#if !HAVE_MEMMEM
void LIBNETXMS_EXPORTABLE *memmem(const void *h0, size_t k, const void *n0, size_t l);
#endif

#ifdef _WIN32
#define stristr strcasestr
#define wcsistr wcscasestr
#ifdef UNICODE
#define _tcsistr wcscasestr
#else
#define _tcsistr strcasestr
#endif
#endif

#if !defined(_WIN32) && (!HAVE_WCSFTIME || !WORKING_WCSFTIME)
size_t LIBNETXMS_EXPORTABLE nx_wcsftime(WCHAR *buffer, size_t bufsize, const WCHAR *format, const struct tm *t);
#undef wcsftime
#define wcsftime nx_wcsftime
#endif

#ifdef _WIN32
#ifdef UNICODE
DIRW LIBNETXMS_EXPORTABLE *wopendir(const WCHAR *path);
struct dirent_w LIBNETXMS_EXPORTABLE *wreaddir(DIRW *p);
int LIBNETXMS_EXPORTABLE wclosedir(DIRW *p);

#define _topendir wopendir
#define _treaddir wreaddir
#define _tclosedir wclosedir
#else   /* not UNICODE */
#define _topendir opendir
#define _treaddir readdir
#define _tclosedir closedir
#endif

DIR LIBNETXMS_EXPORTABLE *opendir(const char *path);
struct dirent LIBNETXMS_EXPORTABLE *readdir(DIR *p);
int LIBNETXMS_EXPORTABLE closedir(DIR *p);

#else	/* not _WIN32 */

DIRW LIBNETXMS_EXPORTABLE *wopendir(const WCHAR *filename);
struct dirent_w LIBNETXMS_EXPORTABLE *wreaddir(DIRW *dirp);
int LIBNETXMS_EXPORTABLE wclosedir(DIRW *dirp);

#endif

#if defined(_WIN32) || !(HAVE_SCANDIR)
int LIBNETXMS_EXPORTABLE scandir(const char *dir, struct dirent ***namelist,
              int (*select)(const struct dirent *),
              int (*compar)(const struct dirent **, const struct dirent **));
int LIBNETXMS_EXPORTABLE alphasort(const struct dirent **a, const struct dirent **b);
#endif

char LIBNETXMS_EXPORTABLE *IntegerToString(int32_t value, char *str, int base = 10);
WCHAR LIBNETXMS_EXPORTABLE *IntegerToString(int32_t value, WCHAR *str, int base = 10);
char LIBNETXMS_EXPORTABLE *IntegerToString(uint32_t value, char *str, int base = 10);
WCHAR LIBNETXMS_EXPORTABLE *IntegerToString(uint32_t value, WCHAR *str, int base = 10);
char LIBNETXMS_EXPORTABLE *IntegerToString(int64_t value, char *str, int base = 10);
WCHAR LIBNETXMS_EXPORTABLE *IntegerToString(int64_t value, WCHAR *str, int base = 10);
char LIBNETXMS_EXPORTABLE *IntegerToString(uint64_t value, char *str, int base = 10);
WCHAR LIBNETXMS_EXPORTABLE *IntegerToString(uint64_t value, WCHAR *str, int base = 10);

bool LIBNETXMS_EXPORTABLE nxlog_open(const TCHAR *logName, UINT32 flags);
void LIBNETXMS_EXPORTABLE nxlog_close();
void LIBNETXMS_EXPORTABLE nxlog_write(int16_t severity, const TCHAR *format, ...);
void LIBNETXMS_EXPORTABLE nxlog_write2(int16_t severity, const TCHAR *format, va_list args);
void LIBNETXMS_EXPORTABLE nxlog_write_tag(int16_t severity, const TCHAR *tag, const TCHAR *format, ...);
void LIBNETXMS_EXPORTABLE nxlog_write_tag2(int16_t severity, const TCHAR *tag, const TCHAR *format, va_list args);
void LIBNETXMS_EXPORTABLE nxlog_report_event(DWORD msg, int level, int stringCount, const TCHAR *altMessage, ...);
void LIBNETXMS_EXPORTABLE nxlog_debug(int level, const TCHAR *format, ...);
void LIBNETXMS_EXPORTABLE nxlog_debug2(int level, const TCHAR *format, va_list args);
void LIBNETXMS_EXPORTABLE nxlog_debug_tag(const TCHAR *tag, int level, const TCHAR *format, ...);
void LIBNETXMS_EXPORTABLE nxlog_debug_tag2(const TCHAR *tag, int level, const TCHAR *format, va_list args);
void LIBNETXMS_EXPORTABLE nxlog_debug_tag_object(const TCHAR *tag, UINT32 objectId, int level, const TCHAR *format, ...);
void LIBNETXMS_EXPORTABLE nxlog_debug_tag_object2(const TCHAR *tag, UINT32 objectId, int level, const TCHAR *format, va_list args);
bool LIBNETXMS_EXPORTABLE nxlog_set_rotation_policy(int rotationMode, UINT64 maxLogSize, int historySize, const TCHAR *dailySuffix);
bool LIBNETXMS_EXPORTABLE nxlog_rotate();
void LIBNETXMS_EXPORTABLE nxlog_set_debug_level(int level);
void LIBNETXMS_EXPORTABLE nxlog_set_debug_level_tag(const TCHAR *tags, int level);
void LIBNETXMS_EXPORTABLE nxlog_set_debug_level_tag(const char *definition);
int LIBNETXMS_EXPORTABLE nxlog_get_debug_level();
int LIBNETXMS_EXPORTABLE nxlog_get_debug_level_tag(const TCHAR *tag);
int LIBNETXMS_EXPORTABLE nxlog_get_debug_level_tag_object(const TCHAR *tag, uint32_t objectId);
void LIBNETXMS_EXPORTABLE nxlog_reset_debug_level_tags();

/**
 * Debug tag information
 */
struct DebugTagInfo
{
   TCHAR tag[64];
   int level;

   DebugTagInfo(const TCHAR *_tag, int _level)
   {
      _tcslcpy(tag, _tag, 64);
      level = _level;
   }
};

ObjectArray<DebugTagInfo> LIBNETXMS_EXPORTABLE *nxlog_get_all_debug_tags();

typedef void (*NxLogDebugWriter)(const TCHAR *, const TCHAR *, va_list);
void LIBNETXMS_EXPORTABLE nxlog_set_debug_writer(NxLogDebugWriter writer);

typedef void (*NxLogConsoleWriter)(const TCHAR *, ...);
void LIBNETXMS_EXPORTABLE nxlog_set_console_writer(NxLogConsoleWriter writer);

typedef void (*NxLogRotationHook)();
void LIBNETXMS_EXPORTABLE nxlog_set_rotation_hook(NxLogRotationHook hook);

void LIBNETXMS_EXPORTABLE WriteToTerminal(const TCHAR *text);
void LIBNETXMS_EXPORTABLE WriteToTerminalEx(const TCHAR *format, ...)
#if !defined(UNICODE) && (defined(__GNUC__) || defined(__clang__))
   __attribute__ ((format(printf, 1, 2)))
#endif
;

/**
 * Check if standard output is a terminal
 */
static inline bool IsOutputToTerminal()
{
#ifdef _WIN32
   DWORD mode;
   return GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &mode) ? true : false;
#else
   return isatty(fileno(stdout)) != 0;
#endif
}

bool LIBNETXMS_EXPORTABLE ReadPassword(const TCHAR *prompt, TCHAR *buffer, size_t bufferSize);

#if !HAVE_STRPTIME
char LIBNETXMS_EXPORTABLE *strptime(const char *buf, const char *fmt, struct tm *_tm);
#endif

#if !HAVE_TIMEGM
time_t LIBNETXMS_EXPORTABLE timegm(struct tm *_tm);
#endif

#if !HAVE_INET_PTON
int LIBNETXMS_EXPORTABLE nx_inet_pton(int af, const char *src, void *dst);
#define inet_pton nx_inet_pton
#endif

int LIBNETXMS_EXPORTABLE GetSleepTime(int hour, int minute, int second);
time_t LIBNETXMS_EXPORTABLE ParseDateTimeA(const char *text, time_t defaultValue, bool utc = false);
time_t LIBNETXMS_EXPORTABLE ParseDateTimeW(const WCHAR *text, time_t defaultValue, bool utc = false);

#ifdef UNICODE
#define ParseDateTime ParseDateTimeW
#else
#define ParseDateTime ParseDateTimeA
#endif

/**
 * NetXMS directory type
 */
enum nxDirectoryType
{
   nxDirBin = 0,
   nxDirData = 1,
   nxDirEtc = 2,
   nxDirLib = 3,
   nxDirShare = 4
};

/**
 * TCP ping result
 */
enum TcpPingResult
{
   TCP_PING_SUCCESS = 0,
   TCP_PING_SOCKET_ERROR = 1,
   TCP_PING_TIMEOUT = 2,
   TCP_PING_REJECT = 3
};

// Defined by Windows header files and not used in our code anyway
#undef IGNORE

/**
 * Generic state change instruction
 */
enum class StateChange
{
   IGNORE = 0,
   SET = 1,
   CLEAR = 2
};

/**
 * Wrapper for DLGetSymbolAddr to get pointer to function
 */
template<typename T> static inline T DLGetFunctionAddr(HMODULE hModule, const char *symbol, TCHAR *errorText = nullptr)
{
   return reinterpret_cast<T>(DLGetSymbolAddr(hModule, symbol, errorText));
}

#define EMA_FP_SHIFT  11                  /* nr of bits of precision */
#define EMA_FP_1      (1 << EMA_FP_SHIFT) /* 1.0 as fixed-point */
#define EMA_EXP_12    1884                /* 1/exp(5sec/1min = 1/12 = 12 points) as fixed-point */
#define EMA_EXP_15    1916                /* 1/exp(1/15 = 15 points) */
#define EMA_EXP_60    2014                /* 1/exp(5sec/5min = 1/60 = 60 points) */
#define EMA_EXP_180   2037                /* 1/exp(5sec/15min = 1/180 = 180 points) */
#define EMA_EXP(interval, period)   static_cast<int>((1.0 / exp(static_cast<double>(interval) / static_cast<double>(period))) * 2048)

/**
 * Update exponential moving average value
 */
template<typename T> static inline void UpdateExpMovingAverage(T& load, int exp, T n)
{
   n <<= EMA_FP_SHIFT;
   load *= exp;
   load += n * (EMA_FP_1 - exp);
   load >>= EMA_FP_SHIFT;
}

/**
 * Get actual exponential load average value from internal representation
 */
template<typename T> static inline double GetExpMovingAverageValue(const T load)
{
   return static_cast<double>(load) / EMA_FP_1;
}

/**
 * Get value of given attribute protected by given mutex
 */
template<typename T> static inline T GetAttributeWithLock(const T& attr, const Mutex& mutex)
{
   mutex.lock();
   T value = attr;
   mutex.unlock();
   return value;
}

/**
 * Set value of given attribute protected by given mutex
 */
template<typename T> static inline void SetAttributeWithLock(T& attr, const T& value, const Mutex& mutex)
{
   mutex.lock();
   attr = value;
   mutex.unlock();
}

/**
 * Get TCHAR[] attribute protected by given mutex as String object
 */
static inline String GetStringAttributeWithLock(const TCHAR *attr, const Mutex& mutex)
{
   mutex.lock();
   String value(attr);
   mutex.unlock();
   return value;
}

/**
 * DJB2 hash algorithm
 */
static inline uint32_t CalculateDJB2Hash(const void *data, size_t size)
{
   uint32_t hash = 5381;
   const uint8_t *p = static_cast<const uint8_t*>(data);
   while(size-- > 0)
      hash = ((hash << 5) + hash) + *p++;
   return hash;
}

/**
 * Check if string is blank (empty or contains only spaces and tabs)
 */
static inline bool IsBlankString(const char* s)
{
   while(*s++ != 0)
      if (!isblank(*s))
         return false;
   return true;
}

/**
 * Check if string is blank (empty or contains only spaces and tabs) - UNICODE version
 */
static inline bool IsBlankString(const WCHAR* s)
{
   while(*s++ != 0)
      if (!iswblank(*s))
         return false;
   return true;
}

TCHAR LIBNETXMS_EXPORTABLE *GetHeapInfo();
int64_t LIBNETXMS_EXPORTABLE GetAllocatedHeapMemory();
int64_t LIBNETXMS_EXPORTABLE GetActiveHeapMemory();
int64_t LIBNETXMS_EXPORTABLE GetMappedHeapMemory();

void LIBNETXMS_EXPORTABLE GetNetXMSDirectory(nxDirectoryType type, TCHAR *dir);
void LIBNETXMS_EXPORTABLE SetNetXMSDataDirectory(const TCHAR *dir);

TcpPingResult LIBNETXMS_EXPORTABLE TcpPing(const InetAddress& addr, UINT16 port, UINT32 timeout);
uint32_t LIBNETXMS_EXPORTABLE IcmpPing(const InetAddress& addr, int numRetries, uint32_t timeout, uint32_t *rtt, uint32_t packetSize, bool dontFragment);
uint16_t LIBNETXMS_EXPORTABLE CalculateIPChecksum(const void *data, size_t len);

TCHAR LIBNETXMS_EXPORTABLE *EscapeStringForXML(const TCHAR *str, int length);
String LIBNETXMS_EXPORTABLE EscapeStringForXML2(const TCHAR *str, int length = -1);
const char LIBNETXMS_EXPORTABLE *XMLGetAttr(const char **attrs, const char *name);
int LIBNETXMS_EXPORTABLE XMLGetAttrInt(const char **attrs, const char *name, int defVal);
uint32_t LIBNETXMS_EXPORTABLE XMLGetAttrUInt32(const char **attrs, const char *name, uint32_t defVal);
bool LIBNETXMS_EXPORTABLE XMLGetAttrBoolean(const char **attrs, const char *name, bool defVal);

String LIBNETXMS_EXPORTABLE EscapeStringForJSON(const TCHAR *s);
String LIBNETXMS_EXPORTABLE EscapeStringForAgent(const TCHAR *s);

char LIBNETXMS_EXPORTABLE *URLEncode(const char *src, char *dst, size_t size);

StringList LIBNETXMS_EXPORTABLE *ParseCommandLine(const TCHAR *cmdline);

#if !defined(_WIN32) && defined(NMS_THREADS_H_INCLUDED)
void LIBNETXMS_EXPORTABLE BlockAllSignals(bool processWide, bool allowInterrupt);
void LIBNETXMS_EXPORTABLE StartMainLoop(ThreadFunction pfSignalHandler, ThreadFunction pfMain);
#endif

String LIBNETXMS_EXPORTABLE GenerateLineDiff(const String& left, const String& right);

bool LIBNETXMS_EXPORTABLE DeflateFile(const TCHAR *inputFile, const TCHAR *outputFile = nullptr);
int LIBNETXMS_EXPORTABLE DeflateFileStream(FILE *source, FILE *dest, bool gzipFormat);

bool LIBNETXMS_EXPORTABLE InflateFile(const TCHAR *inputFile, ByteStream *output);
int LIBNETXMS_EXPORTABLE InflateFileStream(FILE *source, ByteStream *output, bool gzipFormat);

TCHAR LIBNETXMS_EXPORTABLE *GetSystemTimeZone(TCHAR *buffer, size_t size, bool withName = true, bool forceFullOffset = false);

/**
 * Format timestamp as dd.mm.yyyy HH:MM:SS
 */
static inline String FormatTimestamp(time_t t)
{
   TCHAR buffer[32];
   return String(FormatTimestamp(t, buffer));
}

/**
 * Format timestamp in milliseconds as yyyy-mm-dd HH:MM:SS.nnn
 */
static inline String FormatTimestampMs(int64_t t)
{
   TCHAR buffer[32];
   return String(FormatTimestampMs(t, buffer));
}

String LIBNETXMS_EXPORTABLE GetEnvironmentVariableEx(const TCHAR *var);

TCHAR LIBNETXMS_EXPORTABLE *GetFileOwner(const TCHAR *file, TCHAR *buffer, size_t size);

bool LIBNETXMS_EXPORTABLE ExtractNamedOptionValueW(const WCHAR *optString, const WCHAR *option, WCHAR *buffer, size_t bufSize);
bool LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsBoolW(const WCHAR *optString, const WCHAR *option, bool defVal);
int32_t LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsIntW(const WCHAR *optString, const WCHAR *option, int32_t defVal);
uint32_t LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsUIntW(const WCHAR *optString, const WCHAR *option, uint32_t defVal);
uint64_t LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsUInt64W(const WCHAR *optString, const WCHAR *option, uint64_t defVal);
uuid LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsGUIDW(const WCHAR *optString, const WCHAR *option, const uuid& defVal);

bool LIBNETXMS_EXPORTABLE ExtractNamedOptionValueA(const char *optString, const char *option, char *buffer, size_t bufSize);
bool LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsBoolA(const char *optString, const char *option, bool defVal);
int32_t LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsIntA(const char *optString, const char *option, int32_t defVal);
uint32_t LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsUIntA(const char *optString, const char *option, uint32_t defVal);
uint64_t LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsUInt64A(const char *optString, const char *option, uint64_t defVal);
uuid LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsGUIDA(const char *optString, const char *option, const uuid& defVal);

String LIBNETXMS_EXPORTABLE SecondsToUptime(uint64_t arg, bool withSeconds);
String LIBNETXMS_EXPORTABLE FormatNumber(double n, bool useBinaryMultipliers, int multiplierPower, int precision, const TCHAR *unit = nullptr);
String LIBNETXMS_EXPORTABLE FormatDCIValue(const TCHAR *unitName, const TCHAR *value);

#ifdef UNICODE
#define ExtractNamedOptionValue ExtractNamedOptionValueW
#define ExtractNamedOptionValueAsBool ExtractNamedOptionValueAsBoolW
#define ExtractNamedOptionValueAsInt ExtractNamedOptionValueAsIntW
#define ExtractNamedOptionValueAsUInt ExtractNamedOptionValueAsUIntW
#define ExtractNamedOptionValueAsUInt64 ExtractNamedOptionValueAsUInt64W
#define ExtractNamedOptionValueAsGUID ExtractNamedOptionValueAsGUIDW
#else
#define ExtractNamedOptionValue ExtractNamedOptionValueA
#define ExtractNamedOptionValueAsBool ExtractNamedOptionValueAsBoolA
#define ExtractNamedOptionValueAsInt ExtractNamedOptionValueAsIntA
#define ExtractNamedOptionValueAsUInt ExtractNamedOptionValueAsUIntA
#define ExtractNamedOptionValueAsUInt64 ExtractNamedOptionValueAsUInt64A
#define ExtractNamedOptionValueAsGUID ExtractNamedOptionValueAsGUIDA
#endif

int LIBNETXMS_EXPORTABLE CountFilesInDirectoryA(const char *dir, bool (*filter)(const struct dirent *) = nullptr);
int LIBNETXMS_EXPORTABLE CountFilesInDirectoryW(const WCHAR *path, bool (*filter)(const struct dirent_w *) = nullptr);
#ifdef UNICODE
#define CountFilesInDirectory CountFilesInDirectoryW
#else
#define CountFilesInDirectory CountFilesInDirectoryA
#endif

/**
 * Status for SaveFile function
 */
enum class SaveFileStatus
{
   SUCCESS = 0,
   OPEN_ERROR = 1,
   WRITE_ERROR = 2,
   RENAME_ERROR = 3
};

SaveFileStatus LIBNETXMS_EXPORTABLE SaveFile(const TCHAR *fileName, const void *data, size_t size, bool binary = true, bool removeCR = false);
BYTE LIBNETXMS_EXPORTABLE *LoadFile(const TCHAR *fileName, size_t *fileSize);
#ifdef UNICODE
BYTE LIBNETXMS_EXPORTABLE *LoadFileA(const char *fileName, size_t *fileSize);
#else
#define LoadFileA LoadFile
#endif
char LIBNETXMS_EXPORTABLE *LoadFileAsUTF8String(const TCHAR *fileName);
bool LIBNETXMS_EXPORTABLE ScanFile(const TCHAR *fileName, const void *data, size_t size);

bool LIBNETXMS_EXPORTABLE ReadLineFromFileA(const char *path, char *buffer, size_t size);
bool LIBNETXMS_EXPORTABLE ReadLineFromFileW(const WCHAR *path, WCHAR *buffer, size_t size);
bool LIBNETXMS_EXPORTABLE ReadInt32FromFileA(const char *path, int32_t *value);
bool LIBNETXMS_EXPORTABLE ReadInt32FromFileW(const WCHAR *path, int32_t *value);
bool LIBNETXMS_EXPORTABLE ReadUInt64FromFileA(const char *path, uint64_t *value);
bool LIBNETXMS_EXPORTABLE ReadUInt64FromFileW(const WCHAR *path, uint64_t *value);
bool LIBNETXMS_EXPORTABLE ReadDoubleFromFileA(const char *path, double *value);
bool LIBNETXMS_EXPORTABLE ReadDoubleFromFileW(const WCHAR *path, double *value);

#ifdef UNICODE
#define ReadLineFromFile ReadLineFromFileW
#define ReadInt32FromFile ReadInt32FromFileW
#define ReadUInt64FromFile ReadUInt64FromFileW
#define ReadDoubleFromFile ReadDoubleFromFileW
#else
#define ReadLineFromFile ReadLineFromFileA
#define ReadInt32FromFile ReadInt32FromFileA
#define ReadUInt64FromFile ReadUInt64FromFileA
#define ReadDoubleFromFile ReadDoubleFromFileA
#endif

/**
 * Postal address representation
 */
class LIBNETXMS_EXPORTABLE PostalAddress
{
private:
   TCHAR *m_country;
   TCHAR *m_region;
   TCHAR *m_city;
   TCHAR *m_district;
   TCHAR *m_streetAddress;
   TCHAR *m_postcode;

   void setField(TCHAR **field, const TCHAR *value)
   {
      MemFree(*field);
      *field = ((value != nullptr) && (*value != 0)) ? MemCopyString(value) : nullptr;
   }

public:
   PostalAddress();
   PostalAddress(const TCHAR *country, const TCHAR *region, const TCHAR *city, const TCHAR *district, const TCHAR *streetAddress, const TCHAR *postcode);
   PostalAddress(const PostalAddress& src);
   ~PostalAddress();

   PostalAddress& operator =(const PostalAddress& src);

   const TCHAR *getCountry() const { return CHECK_NULL_EX(m_country); }
   const TCHAR *getRegion() const { return CHECK_NULL_EX(m_region); }
   const TCHAR *getCity() const { return CHECK_NULL_EX(m_city); }
   const TCHAR *getDistrict() const { return CHECK_NULL_EX(m_district); }
   const TCHAR *getStreetAddress() const { return CHECK_NULL_EX(m_streetAddress); }
   const TCHAR *getPostCode() const { return CHECK_NULL_EX(m_postcode); }

   json_t *toJson() const;

   void setCountry(const TCHAR *country) { setField(&m_country, country); }
   void setRegion(const TCHAR *region) { setField(&m_region, region); }
   void setCity(const TCHAR *city) { setField(&m_city, city); }
   void setDistrict(const TCHAR *district) { setField(&m_district, district); }
   void setStreetAddress(const TCHAR *streetAddress) { setField(&m_streetAddress, streetAddress); }
   void setPostCode(const TCHAR *postcode) { setField(&m_postcode, postcode); }
};

/**
 * Java UI compatibe color manipulations
 */
struct LIBNETXMS_EXPORTABLE Color
{
   BYTE red;
   BYTE green;
   BYTE blue;

   /**
    * Default constructor - create black color
    */
   Color()
   {
      red = 0;
      green = 0;
      blue = 0;
   }

   /**
    * Create from separate red/green/blue values
    */
   Color(BYTE red, BYTE green, BYTE blue)
   {
      this->red = red;
      this->green = green;
      this->blue = blue;
   }

   /**
    * Create from Java UI compatible integer value
    */
   Color(uint32_t ivalue)
   {
      red = static_cast<BYTE>(ivalue & 0xFF);
      green = static_cast<BYTE>((ivalue >> 8) & 0xFF);
      blue = static_cast<BYTE>((ivalue >> 16) & 0xFF);
   }

   /**
    * Check that two color objects represent same color
    */
   bool equals(const Color& other) const
   {
      return (red == other.red) && (green == other.green) && (blue == other.blue);
   }

   /**
    * Convert to Java UI compatible integer
    */
   uint32_t toInteger() const
   {
      return static_cast<uint32_t>(red) | (static_cast<uint32_t>(green) << 8) | (static_cast<uint32_t>(blue) << 16);
   }

   /**
    * Convert to CSS definition
    */
   String toCSS(bool alwaysUseHex = false) const;

   /**
    * Swap red and blue
    */
   void swap()
   {
      BYTE t = red;
      red = blue;
      blue = t;
   }

   /**
    * Parse color definition in CSS compatible format
    */
   static Color parseCSS(const TCHAR *css);
};

#endif   /* _nms_util_h_ */
