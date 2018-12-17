/*
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
#include <jansson.h>

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

#include <base64.h>

/*** Byte swapping ***/
#if WORDS_BIGENDIAN
#define htonq(x) (x)
#define ntohq(x) (x)
#define htond(x) (x)
#define ntohd(x) (x)
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
#define SwapUCS2String(x) bswap_array_16((UINT16 *)(x), -1)
#endif

#ifdef __cplusplus

#if !(HAVE_DECL_BSWAP_16)
inline UINT16 bswap_16(UINT16 val)
{
   return (val >> 8) | ((val << 8) & 0xFF00);
}
#endif

#if !(HAVE_DECL_BSWAP_32)
inline UINT32 bswap_32(UINT32 val)
{
   UINT32 result;
   BYTE *sptr = (BYTE *)&val;
   BYTE *dptr = (BYTE *)&result + 3;
   for(int i = 0; i < 4; i++, sptr++, dptr--)
      *dptr = *sptr;
   return result;
}
#endif

#if !(HAVE_DECL_BSWAP_64)
inline UINT64 bswap_64(UINT64 val)
{
   UINT64 result;
   BYTE *sptr = (BYTE *)&val;
   BYTE *dptr = (BYTE *)&result + 7;
   for(int i = 0; i < 8; i++, sptr++, dptr--)
      *dptr = *sptr;
   return result;
}
#endif

inline double bswap_double(double val)
{
   double result;
   BYTE *sptr = (BYTE *)&val;
   BYTE *dptr = (BYTE *)&result + 7;
   for(int i = 0; i < 8; i++, sptr++, dptr--)
      *dptr = *sptr;
   return result;
}

#else

#if !(HAVE_DECL_BSWAP_16)
UINT16 LIBNETXMS_EXPORTABLE __bswap_16(UINT16 val);
#define bswap_16 __bswap_16
#endif

#if !(HAVE_DECL_BSWAP_32)
UINT32 LIBNETXMS_EXPORTABLE __bswap_32(UINT32 val);
#define bswap_32 __bswap_32
#endif

#if !(HAVE_DECL_BSWAP_64)
UINT64 LIBNETXMS_EXPORTABLE __bswap_64(UINT64 val);
#define bswap_64 __bswap_64
#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif
void LIBNETXMS_EXPORTABLE bswap_array_16(UINT16 *v, int len);
void LIBNETXMS_EXPORTABLE bswap_array_32(UINT32 *v, int len);
#ifdef __cplusplus
}
#endif

/*** toupper/tolower ***/
#ifdef __cplusplus

#if !HAVE_TOLOWER
inline char tolower(char c)
{
   return ((c >= 'A') && (c <= 'Z')) ? c + ('a' - 'A') : c;
}
#endif

#if !HAVE_TOWLOWER
inline WCHAR towlower(WCHAR c)
{
   return ((c >= L'A') && (c <= L'Z')) ? c + (L'a' - L'A') : c;
}
#endif

#if !HAVE_TOUPPER
inline char toupper(char c)
{
   return ((c >= 'a') && (c <= 'z')) ? c - ('a' - 'A') : c;
}
#endif

#if !HAVE_TOWUPPER
inline WCHAR towupper(WCHAR c)
{
   return ((c >= L'a') && (c <= L'z')) ? c - (L'a' - L'A') : c;
}
#endif

#endif /* __cplusplus */

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
#define ICMP_UNREACHEABLE     1
#define ICMP_TIMEOUT          2
#define ICMP_RAW_SOCK_FAILED  3
#define ICMP_API_ERROR        4
#define ICMP_SEND_FAILED      5

/**
 * Token types for configuration loader
 */
#define CT_LONG         0
#define CT_STRING       1
#define CT_STRING_LIST  2
#define CT_END_OF_LIST  3
#define CT_BOOLEAN      4
#define CT_WORD         5
#define CT_IGNORE       6
#define CT_MB_STRING    7
#define CT_BOOLEAN64    8
#define CT_SIZE_BYTES   9   /* 64 bit integer, automatically converts K, M, G, T suffixes using 1024 as base) */
#define CT_SIZE_UNITS   10  /* 64 bit integer, automatically converts K, M, G, T suffixes using 1000 as base) */

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
#define NXLOG_USE_SYSLOG		  ((UINT32)0x00000001)
#define NXLOG_PRINT_TO_STDOUT	  ((UINT32)0x00000002)
#define NXLOG_BACKGROUND_WRITER ((UINT32)0x00000004)
#define NXLOG_DEBUG_MODE        ((UINT32)0x00000008)
#define NXLOG_IS_OPEN           ((UINT32)0x80000000)

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

#ifdef __cplusplus
extern "C" {
#endif
size_t LIBNETXMS_EXPORTABLE wcslen(const WCHAR *s);
#ifdef __cplusplus
}
#endif

#endif

/**
 * Custom wcsncpy()
 */
#if !HAVE_WCSNCPY

#ifdef __cplusplus
extern "C" {
#endif
WCHAR LIBNETXMS_EXPORTABLE *wcsncpy(WCHAR *dest, const WCHAR *src, size_t n);
#ifdef __cplusplus
}
#endif

#endif

// Some AIX versions have broken wcsdup() so we use internal implementation
#if !HAVE_WCSDUP || defined(_AIX)
#define wcsdup MemCopyStringW
#endif

/******* UNICODE related conversion and helper functions *******/

#ifdef __cplusplus
extern "C" {
#endif

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
int LIBNETXMS_EXPORTABLE ucs2_to_ucs4(const UCS2CHAR *src, int srcLen, UCS4CHAR *dst, int dstLen);
int LIBNETXMS_EXPORTABLE ucs2_to_utf8(const UCS2CHAR *src, int srcLen, char *dst, int dstLen);
int LIBNETXMS_EXPORTABLE ucs2_utf8len(const UCS2CHAR *src, int srcLen);
#if defined(_WIN32) || defined(UNICODE_UCS2)
#define ucs2_to_mb(wstr, wlen, mstr, mlen)   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, wstr, wlen, mstr, mlen, NULL, NULL)
#else
int LIBNETXMS_EXPORTABLE ucs2_to_mb(const UCS2CHAR *src, int srcLen, char *dst, int dstLen);
#endif

int LIBNETXMS_EXPORTABLE ucs4_to_ucs2(const UCS4CHAR *src, int srcLen, UCS2CHAR *dst, int dstLen);
int LIBNETXMS_EXPORTABLE ucs4_ucs2len(const UCS4CHAR *src, int srcLen);
int LIBNETXMS_EXPORTABLE ucs4_to_utf8(const UCS4CHAR *src, int srcLen, char *dst, int dstLen);
int LIBNETXMS_EXPORTABLE ucs4_utf8len(const UCS4CHAR *src, int srcLen);
#ifdef UNICODE_UCS4
#define ucs4_to_mb(wstr, wlen, mstr, mlen)   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, wstr, wlen, mstr, mlen, NULL, NULL)
#else
int LIBNETXMS_EXPORTABLE ucs4_to_mb(const UCS4CHAR *src, int srcLen, char *dst, int dstLen);
#endif

int LIBNETXMS_EXPORTABLE utf8_to_ucs4(const char *src, int srcLen, UCS4CHAR *dst, int dstLen);
int LIBNETXMS_EXPORTABLE utf8_ucs4len(const char *src, int srcLen);
int LIBNETXMS_EXPORTABLE utf8_to_ucs2(const char *src, int srcLen, UCS2CHAR *dst, int dstLen);
int LIBNETXMS_EXPORTABLE utf8_ucs2len(const char *src, int srcLen);
int LIBNETXMS_EXPORTABLE utf8_to_mb(const char *src, int srcLen, char *dst, int dstLen);

#ifdef UNICODE_UCS4
#define mb_to_ucs4(mstr, mlen, wstr, wlen)   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, mstr, mlen, wstr, wlen)
#else
int LIBNETXMS_EXPORTABLE mb_to_ucs4(const char *src, int srcLen, UCS4CHAR *dst, int dstLen);
#endif
#if defined(_WIN32) || defined(UNICODE_UCS2)
#define mb_to_ucs2(mstr, mlen, wstr, wlen)   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, mstr, mlen, wstr, wlen)
#else
int LIBNETXMS_EXPORTABLE mb_to_ucs2(const char *src, int srcLen, UCS2CHAR *dst, int dstLen);
#endif
int LIBNETXMS_EXPORTABLE mb_to_utf8(const char *src, int srcLen, char *dst, int dstLen);

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
#else
#define UTF8StringFromTString(s) UTF8StringFromMBString(s)
#endif

#ifdef __cplusplus
}
#endif

/******* end of UNICODE related conversion and helper functions *******/


/**
 * Class for serial communications
 */
#ifdef __cplusplus

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
	TCHAR *m_pszPort;
	int m_nTimeout;
	int m_nSpeed;
	int m_nDataBits;
	int m_nStopBits;
	int m_nParity;
	int m_nFlowControl;
   int m_writeBlockSize;
   int m_writeDelay;

#ifndef _WIN32
	int m_hPort;
	struct termios m_originalSettings;
#else
	HANDLE m_hPort;
#endif

   bool writeBlock(const char *data, int length);

public:
	Serial();
	~Serial();

	bool open(const TCHAR *pszPort);
	void close();
	void setTimeout(int nTimeout);
	int read(char *pBuff, int nSize); /* waits up to timeout and do single read */
	int readAll(char *pBuff, int nSize); /* read until timeout or out of space */
   int readToMark(char *buff, int size, const char **marks, char **occurence);
	bool write(const char *buffer, int length);
	void flush();
	bool set(int nSpeed, int nDataBits, int nParity, int nStopBits, int nFlowControl = FLOW_NONE);
   void setWriteBlockSize(int bs) { m_writeBlockSize = bs; }
   void setWriteDelay(int delay) { m_writeDelay = delay; }
	bool restart();
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
      *((void **)m_currentRegion) = NULL; // pointer to previous region
      m_firstDeleted = NULL;
      m_allocated = m_headerSize;
   }

   /**
    * Destroy memory pool. Will destroy allocated memory but object destructors will not be called.
    */
   ~ObjectMemoryPool()
   {
      void *r = m_currentRegion;
      while(r != NULL)
      {
         void *n = *((void **)r);
         MemFree(r);
         r = n;
      }
   }

   /**
    * Allocate memory for object without initializing
    */
   T *allocate()
   {
      T *p;
      if (m_firstDeleted != NULL)
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
      if (p != NULL)
      {
         *((T**)p) = m_firstDeleted;
         m_firstDeleted = p;
      }
   }

   /**
    * Destroy object
    */
   void destroy(T *p)
   {
      if (p != NULL)
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
      m_free = NULL;
      m_used = NULL;
   }

   ~ResourcePool()
   {
      ResourcePoolElement<T> *handle = m_used;
      while(handle != NULL)
      {
         Destroy(handle->resource);
         handle = handle->next;
      }

      handle = m_free;
      while(handle != NULL)
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
      if (m_free != NULL)
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
      ResourcePoolElement<T> *handle = m_used, *prev = NULL;
      while((handle != NULL) && (handle->resource != resource))
      {
         prev = handle;
         handle = handle->next;
      }
      if (handle != NULL)
      {
         Reset(handle->resource);
         if (prev != NULL)
            prev->next = handle->next;
         else
            m_used = handle->next;
         handle->next = m_free;
         m_free = handle;
      }
   }
};

/**
 * Class that can store any object with connected to it mutex
 */
template <class T> class ObjectLock
{
private:
   Mutex m_mutex;
   T *m_object;

public:
   ObjectLock() { m_object = NULL; }
   ObjectLock(T *object) { m_object = object; }
   ObjectLock(const ObjectLock<T> &src) { m_object = src.m_object; m_mutex = src.m_mutex; }

   void lock() { m_mutex.lock(); }
   void unlock() { m_mutex.unlock(); }

   T *get() { return m_object; }
   void set(T *object) { m_object = object; }

   operator T*() { return m_object; }
   ObjectLock<T>& operator =(const ObjectLock<T> &src)
   {
      m_object = src.m_object;
      m_mutex = src.m_mutex;
      return *this;
   }
};

/**
 * Pair class (stores pair of items)
 */
template <typename T1, typename T2> class Pair
{
private:
   T1 value1;
   T2 value2;

public:
   Pair() { }
   Pair(const T1& v1, const T2& v2)
   {
      value1 = v1;
      value2 = v2;
   }
   Pair(const Pair<T1, T2>& src)
   {
      value1 = src.value1;
      value2 = src.value2;
   }

   Pair<T1, T2>& operator=(const Pair<T1, T2> &src)
   {
      return Pair<T1, T2>(src.value1, src.value2);
   }

   const T1& first() const { return value1; }
   const T2& second() const { return value2; }
};

/**
 * uuid_t wrapper class
 */
class uuid;

/**
 * Dynamic string class
 */
class LIBNETXMS_EXPORTABLE String
{
protected:
   TCHAR *m_buffer;
   size_t m_length;
   size_t m_allocated;
   size_t m_allocationStep;

public:
	static const int npos;

   String();
   String(const TCHAR *init);
	String(const String &src);
   ~String();

	TCHAR *getBuffer() { return m_buffer; }
   void setBuffer(TCHAR *buffer);

   size_t getAllocationStep() { return m_allocationStep; }
   void setAllocationStep(size_t step) { m_allocationStep = step; }

   String& operator =(const TCHAR *str);
	String& operator =(const String &src);
   String& operator +=(const TCHAR *str);
   String& operator +=(const String &str);
   String operator +(const String &right) const;
   String operator +(const TCHAR *right) const;
   operator const TCHAR*() const { return CHECK_NULL_EX(m_buffer); }

   bool operator ==(const String &s) const { return equals(s); }
   bool operator !=(const String &s) const { return !equals(s); }

	char *getUTF8String();

   void append(const TCHAR *str) { if (str != NULL) append(str, _tcslen(str)); }
	void append(const TCHAR *str, size_t len);
   void append(const TCHAR c) { append(&c, 1); }
   void append(INT32 n);
   void append(UINT32 n);
   void append(INT64 n);
   void append(UINT64 n);
   void append(const uuid& guid);

	void appendPreallocated(TCHAR *str) { if (str != NULL) { append(str); MemFree(str); } }

	void appendMBString(const char *str, size_t len, int nCodePage);
	void appendWideString(const WCHAR *str, size_t len);

   void appendFormattedString(const TCHAR *format, ...);
   void appendFormattedStringV(const TCHAR *format, va_list args);

   void clear();

	size_t length() const { return m_length; }
	bool isEmpty() const { return m_length == 0; }

	bool equals(const String& s) const;
	bool equals(const TCHAR *s) const;
   bool startsWith(const String& s) const;
   bool startsWith(const TCHAR *s) const;
   bool endsWith(const String& s) const;
   bool endsWith(const TCHAR *s) const;

	TCHAR charAt(size_t pos) const { return (pos < m_length) ? m_buffer[pos] : 0; }

   String substring(size_t start, ssize_t len) const;
	TCHAR *substring(size_t start, ssize_t len, TCHAR *buffer) const;
	String left(size_t len) const { return substring(0, static_cast<ssize_t>(len)); }
   String right(size_t len) const { return substring((m_length > len) ? m_length - len : 0, static_cast<ssize_t>(len)); }

   StringList *split(const TCHAR *separator) const;

	int find(const TCHAR *str, size_t start = 0) const;

   void escapeCharacter(int ch, int esc);
   void replace(const TCHAR *pszSrc, const TCHAR *pszDst);
	void trim();
	void shrink(size_t chars = 1);

   void toUppercase();
   void toLowercase();
};

/**
 * Abstract iterator class (to be implemented by actual iterable class)
 */
class LIBNETXMS_EXPORTABLE AbstractIterator
{
   DISABLE_COPY_CTOR(AbstractIterator)

protected:
   AbstractIterator();

public:
   virtual ~AbstractIterator();

   virtual bool hasNext() = 0;
   virtual void *next() = 0;
   virtual void remove() = 0;
};

/**
 * Iterator class (public interface for iteration)
 */
template <class T> class Iterator
{
   DISABLE_COPY_CTOR(Iterator)

private:
   AbstractIterator *m_worker;

public:
   Iterator(AbstractIterator *worker) { m_worker = worker; }
   ~Iterator() { delete m_worker; }

   bool hasNext() { return m_worker->hasNext(); }
   T *next() { return (T *)m_worker->next(); }
   void remove() { m_worker->remove(); }
};

/**
 * Dynamic array class
 */
class LIBNETXMS_EXPORTABLE Array
{
   DISABLE_COPY_CTOR(Array)

private:
	int m_size;
	int m_allocated;
	int m_grow;
   size_t m_elementSize;
	void **m_data;
	bool m_objectOwner;

	void internalRemove(int index, bool allowDestruction);
	void destroyObject(void *object) { if (object != NULL) m_objectDestructor(object); }

protected:
   bool m_storePointers;
	void (*m_objectDestructor)(void *);

   Array(void *data, int initial, int grow, size_t elementSize);
   Array(const Array *src);

   void *__getBuffer() const { return m_data; }

public:
	Array(int initial = 0, int grow = 16, bool owner = false);
	virtual ~Array();

	int add(void *element);
   void *get(int index) const { return ((index >= 0) && (index < m_size)) ? (m_storePointers ? m_data[index] : (void *)((char *)m_data + index * m_elementSize)): NULL; }
   void *first() const { return get(0); }
   void *last() const { return get(m_size - 1); }
   int indexOf(void *element) const;
	void set(int index, void *element);
	void replace(int index, void *element);
   void insert(int index, void *element);
	void remove(int index) { internalRemove(index, true); }
   void remove(void *element) { internalRemove(indexOf(element), true); }
	void unlink(int index) { internalRemove(index, false); }
	void unlink(void *element) { internalRemove(indexOf(element), false); }
	void clear();
   void sort(int (*cb)(const void *, const void *));
   void *find(const void *key, int (*cb)(const void *, const void *)) const;

	int size() const { return m_size; }
	bool isEmpty() const { return m_size == 0; }

	void setOwner(bool owner) { m_objectOwner = owner; }
	bool isOwner() const { return m_objectOwner; }
};

/**
 * Array iterator class
 */
class LIBNETXMS_EXPORTABLE ArrayIterator : public AbstractIterator
{
   DISABLE_COPY_CTOR(ArrayIterator)

private:
   Array *m_array;
   int m_pos;

public:
   ArrayIterator(Array *array);

   virtual bool hasNext() OVERRIDE;
   virtual void *next() OVERRIDE;
   virtual void remove() OVERRIDE;
};

/**
 * Template class for dynamic array which holds objects
 */
template <class T> class ObjectArray : public Array
{
   DISABLE_COPY_CTOR(ObjectArray)

private:
	static void destructor(void *object) { delete static_cast<T*>(object); }

public:
	ObjectArray(int initial = 0, int grow = 16, bool owner = false) : Array(initial, grow, owner) { m_objectDestructor = destructor; }
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
	void remove(int index) { Array::remove(index); }
   void remove(T *object) { Array::remove((void *)object); }
	void unlink(int index) { Array::unlink(index); }
   void unlink(T *object) { Array::unlink((void *)object); }

   void sort(int (*cb)(const T **, const T **)) { Array::sort((int (*)(const void *, const void *))cb); }
   T *find(const T *key, int (*cb)(const T **, const T **)) const
   {
      T **result = (T **)Array::find(&key, (int (*)(const void *, const void *))cb);
      return (result != NULL) ? *result : NULL;
   }

   const T * const *getBuffer() const { return static_cast<const T * const *>(__getBuffer()); }

   Iterator<T> *iterator() { return new Iterator<T>(new ArrayIterator(this)); }
};

/**
 * Template class for dynamic array which holds references to objects with inaccessible destructors
 */
template <class T> class ObjectRefArray : public Array
{
   DISABLE_COPY_CTOR(ObjectRefArray)

private:
	static void destructor(void *object) { }

public:
	ObjectRefArray(int initial = 0, int grow = 16) : Array(initial, grow, false) { m_objectDestructor = destructor; }
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

   Iterator<T> *iterator() { return new Iterator<T>(new ArrayIterator(this)); }
};

/**
 * Template class for dynamic array which holds scalar values
 */
template <class T> class IntegerArray : public Array
{
   DISABLE_COPY_CTOR(IntegerArray)

private:
	static void destructor(void *element) { }

public:
	IntegerArray(int initial = 0, int grow = 16) : Array(NULL, initial, grow, sizeof(T)) { m_objectDestructor = destructor; m_storePointers = (sizeof(T) == sizeof(void *)); }
	IntegerArray(const IntegerArray<T> *src) : Array(src) { }
	virtual ~IntegerArray() { }

   int add(T value) { return Array::add(m_storePointers ? CAST_TO_POINTER(value, void *) : &value); }
   T get(int index) const { if (m_storePointers) return CAST_FROM_POINTER(Array::get(index), T); T *p = (T*)Array::get(index); return (p != NULL) ? *p : 0; }
   int indexOf(T value) const { return Array::indexOf(m_storePointers ? CAST_TO_POINTER(value, void *) : &value); }
   bool contains(T value) const { return indexOf(value) >= 0; }
   void set(int index, T value) { Array::set(index, m_storePointers ? CAST_TO_POINTER(value, void *) : &value); }
   void replace(int index, T value) { Array::replace(index, m_storePointers ? CAST_TO_POINTER(value, void *) : &value); }

   T *getBuffer() const { return (T*)__getBuffer(); }

   json_t *toJson() const { json_t *a = json_array(); for(int i = 0; i < size(); i++) json_array_append_new(a, json_integer(get(i))); return a; }
};

/**
 * Auxilliary class to hold dynamically allocated array of structures
 */
template <class T> class StructArray : public Array
{
   DISABLE_COPY_CTOR(StructArray)

private:
	static void destructor(void *element) { }

public:
	StructArray(int initial = 0, int grow = 16) : Array(NULL, initial, grow, sizeof(T)) { m_objectDestructor = destructor; }
	StructArray(T *data, int size, int grow = 16) : Array(data, size, grow, sizeof(T)) { m_objectDestructor = destructor; }
	StructArray(const StructArray<T> *src) : Array(src) { }
	virtual ~StructArray() { }

	int add(const T *element) { return Array::add((void *)element); }
	T *get(int index) const { return (T*)Array::get(index); }
   int indexOf(const T *element) const { return Array::indexOf((void *)element); }
   bool contains(const T *element) const { return indexOf(element) >= 0; }
	void set(int index, const T *element) { Array::set(index, (void *)element); }
	void replace(int index, const T *element) { Array::replace(index, (void *)element); }
	void remove(int index) { Array::remove(index); }
   void remove(const T *element) { Array::remove((void *)element); }
	void unlink(int index) { Array::unlink(index); }
   void unlink(const T *element) { Array::unlink((void *)element); }

   T *getBuffer() const { return (T*)__getBuffer(); }
};

/**
 * Entry of string map (internal)
 */
struct StringMapEntry;

/**
 * Key/value pair
 */
struct KeyValuePair
{
   const TCHAR *key;
   const void *value;
};

/**
 * String maps base class
 */
class LIBNETXMS_EXPORTABLE StringMapBase
{
protected:
   StringMapEntry *m_data;
	bool m_objectOwner;
   bool m_ignoreCase;
	void (*m_objectDestructor)(void *, StringMapBase *);

	StringMapEntry *find(const TCHAR *key, size_t keyLen) const;
	void setObject(TCHAR *key, void *value, bool keyPreAlloc);
	void *getObject(const TCHAR *key) const;
   void *getObject(const TCHAR *key, size_t len) const;
	void destroyObject(void *object) { if (object != NULL) m_objectDestructor(object, this); }
	void fillValues(Array *a) const;

public:
	StringMapBase(bool objectOwner);
	virtual ~StringMapBase();

   void setOwner(bool owner) { m_objectOwner = owner; }
   void setIgnoreCase(bool ignore);

	void remove(const TCHAR *key);
	void clear();
   void filterElements(bool (*filter)(const TCHAR *, const void *, void *), void *userData);

	int size() const;
   bool isEmpty() const { return size() == 0; }
   bool contains(const TCHAR *key) const { return (key != NULL) ? (find(key, _tcslen(key) * sizeof(TCHAR)) != NULL) : false; }
   bool contains(const TCHAR *key, size_t len) const { return (key != NULL) ? (find(key, len * sizeof(TCHAR)) != NULL) : false; }

   EnumerationCallbackResult forEach(EnumerationCallbackResult (*cb)(const TCHAR *, const void *, void *), void *userData) const;
   const void *findElement(bool (*comparator)(const TCHAR *, const void *, void *), void *userData) const;

   StructArray<KeyValuePair> *toArray() const;
   StringList *keys() const;
};

/**
 * NXCP message class
 */
class NXCPMessage;

/**
 * String map class
 */
class LIBNETXMS_EXPORTABLE StringMap : public StringMapBase
{
public:
	StringMap() : StringMapBase(true) { }
	StringMap(const StringMap &src);
	virtual ~StringMap();

	StringMap& operator =(const StringMap &src);

	void set(const TCHAR *key, const TCHAR *value) { if (key != NULL) setObject((TCHAR *)key, _tcsdup(value), false); }
	void setPreallocated(TCHAR *key, TCHAR *value) { setObject(key, value, true); }
   void set(const TCHAR *key, INT32 value);
	void set(const TCHAR *key, UINT32 value);
   void set(const TCHAR *key, INT64 value);
   void set(const TCHAR *key, UINT64 value);

   void addAll(const StringMap *src);

	const TCHAR *get(const TCHAR *key) const { return (const TCHAR *)getObject(key); }
   const TCHAR *get(const TCHAR *key, size_t len) const { return (const TCHAR *)getObject(key, len); }
   INT32 getInt32(const TCHAR *key, INT32 defaultValue) const;
	UINT32 getUInt32(const TCHAR *key, UINT32 defaultValue) const;
   INT64 getInt64(const TCHAR *key, INT64 defaultValue) const;
   UINT64 getUInt64(const TCHAR *key, UINT64 defaultValue) const;
   double getDouble(const TCHAR *key, double defaultValue) const;
	bool getBoolean(const TCHAR *key, bool defaultValue) const;

   void fillMessage(NXCPMessage *msg, UINT32 sizeFieldId, UINT32 baseFieldId) const;
   void loadMessage(const NXCPMessage *msg, UINT32 sizeFieldId, UINT32 baseFieldId);

   json_t *toJson() const;
};

/**
 * String map template for holding objects as values
 */
template <class T> class StringObjectMap : public StringMapBase
{
   DISABLE_COPY_CTOR(StringObjectMap)

private:
	static void destructor(void *object, StringMapBase *map) { delete (T*)object; }

public:
	StringObjectMap(bool objectOwner) : StringMapBase(objectOwner) { m_objectDestructor = destructor; }

	void set(const TCHAR *key, T *object) { setObject((TCHAR *)key, (void *)object, false); }
	void setPreallocated(TCHAR *key, T *object) { setObject((TCHAR *)key, (void *)object, true); }
	T *get(const TCHAR *key) const { return (T*)getObject(key); }
   T *get(const TCHAR *key, size_t len) const { return (T*)getObject(key, len); }
   ObjectArray<T> *values() const { ObjectArray<T> *v = new ObjectArray<T>(size()); fillValues(v); return v; }
};

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
	StringList(const TCHAR *src, const TCHAR *separator);
   StringList(const NXCPMessage *msg, UINT32 baseId, UINT32 countId);
	~StringList();

	void add(const TCHAR *value);
	void addPreallocated(TCHAR *value);
	void add(INT32 value);
	void add(UINT32 value);
	void add(INT64 value);
	void add(UINT64 value);
	void add(double value);
	void replace(int index, const TCHAR *value);
	void addOrReplace(int index, const TCHAR *value);
	void addOrReplacePreallocated(int index, TCHAR *value);
#ifdef UNICODE
   void addMBString(const char *value);
#else
	void addMBString(const char *value) { add(value); }
#endif
   void addAll(const StringList *src);

   void insert(int pos, const TCHAR *value);
#ifdef UNICODE
   void insertMBString(int pos, const char *value);
#else
   void insertMBString(int pos, const char *value) { insert(pos, value); }
#endif
   void insertAll(int pos, const StringList *src);

   void merge(const StringList *src, bool matchCase);
   void splitAndAdd(const TCHAR *src, const TCHAR *separator);

   void remove(int index);
	void clear();

   void sort(bool ascending = true, bool caseSensitive = false);

	int size() const { return m_count; }
	bool isEmpty() const { return m_count == 0; }
	const TCHAR *get(int index) const { return ((index >=0) && (index < m_count)) ? m_values[index] : NULL; }
	int indexOf(const TCHAR *value) const;
	bool contains(const TCHAR *value) const { return indexOf(value) != -1; }
	int indexOfIgnoreCase(const TCHAR *value) const;
   bool containsIgnoreCase(const TCHAR *value) const { return indexOfIgnoreCase(value) != -1; }
   TCHAR *join(const TCHAR *separator);

   void fillMessage(NXCPMessage *msg, UINT32 baseId, UINT32 countId) const;
   json_t *toJson() const;
};

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
 * String set iterator
 */
class LIBNETXMS_EXPORTABLE StringSetIterator : public AbstractIterator
{
   DISABLE_COPY_CTOR(StringSetIterator)

private:
   StringSet *m_stringSet;
   StringSetEntry *m_curr;
   StringSetEntry *m_next;

public:
   StringSetIterator(StringSet *stringSet);

   virtual bool hasNext() OVERRIDE;
   virtual void *next() OVERRIDE;
   virtual void remove() OVERRIDE;
};

/**
 * String set class
 */
class LIBNETXMS_EXPORTABLE StringSet
{
   friend class StringSetIterator;

private:
   StringSetEntry *m_data;

public:
   StringSet();
   ~StringSet();

   void add(const TCHAR *str);
   void addPreallocated(TCHAR *str);
   void remove(const TCHAR *str);
   void clear();

   int size() const;
   bool contains(const TCHAR *str) const;
   bool equals(const StringSet *s) const;

   void addAll(StringSet *src);
   void addAll(TCHAR **strings, int count);
   void splitAndAdd(const TCHAR *src, const TCHAR *separator);
   void addAllPreallocated(TCHAR **strings, int count);

   void forEach(bool (*cb)(const TCHAR *, void *), void *userData) const;

   void fillMessage(NXCPMessage *msg, UINT32 baseId, UINT32 countId) const;
   void addAllFromMessage(const NXCPMessage *msg, UINT32 baseId, UINT32 countId, bool clearBeforeAdd, bool toUppercase);

   String join(const TCHAR *separator);

   Iterator<const TCHAR> *iterator() { return new Iterator<const TCHAR>(new StringSetIterator(this)); }
};

/**
 * Opaque hash set entry structure
 */
struct HashSetEntry;

/**
 * Hash set base class (for fixed size non-pointer keys)
 */
class LIBNETXMS_EXPORTABLE HashSetBase
{
private:
   HashSetEntry *m_data;
   unsigned int m_keylen;

protected:
   HashSetBase(unsigned int keylen);

   void _put(const void *key);
   void _remove(const void *key);
   bool _contains(const void *key) const;

public:
   virtual ~HashSetBase();

   int size() const;

   void clear();

   EnumerationCallbackResult forEach(EnumerationCallbackResult (*cb)(const void *, void *), void *userData) const;
};

/**
 * Hash set template
 */
template <class K> class HashSet : public HashSetBase
{
public:
   HashSet() : HashSetBase(sizeof(K)) { }

   void put(const K& key) { _put(&key); }
   void remove(const K& key) { _remove(&key); }
   bool contains(const K& key) { return _contains(&key); }
};

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

	HashMapEntry *find(const void *key) const;
	void destroyObject(void *object) { if (object != NULL) m_objectDestructor(object, this); }

protected:
	void (*m_objectDestructor)(void *, HashMapBase *);

	HashMapBase(bool objectOwner, unsigned int keylen);

	void *_get(const void *key) const;
	void _set(const void *key, void *value);
	void _remove(const void *key);

   bool _contains(const void *key) const { return find(key) != NULL; }

public:
   virtual ~HashMapBase();

   void setOwner(bool owner) { m_objectOwner = owner; }
	void clear();

	int size() const;

   EnumerationCallbackResult forEach(EnumerationCallbackResult (*cb)(const void *, const void *, void *), void *userData) const;
   const void *findElement(bool (*comparator)(const void *, const void *, void *), void *userData) const;
};

/**
 * Hash map iterator
 */
class LIBNETXMS_EXPORTABLE HashMapIterator : public AbstractIterator
{
   DISABLE_COPY_CTOR(HashMapIterator)

private:
   HashMapBase *m_hashMap;
   HashMapEntry *m_curr;
   HashMapEntry *m_next;

public:
   HashMapIterator(HashMapBase *hashMap);

   virtual bool hasNext() OVERRIDE;
   virtual void *next() OVERRIDE;
   virtual void remove() OVERRIDE;
};

/**
 * Hash map template for holding objects as values
 */
template <class K, class V> class HashMap : public HashMapBase
{
private:
	static void destructor(void *object, HashMapBase *map) { delete (V*)object; }

public:
	HashMap(bool objectOwner = false) : HashMapBase(objectOwner, sizeof(K)) { m_objectDestructor = destructor; }

	V *get(const K& key) { return (V*)_get(&key); }
	void set(const K& key, V *value) { _set(&key, (void *)value); }
   void remove(const K& key) { _remove(&key); }
   bool contains(const K& key) { return _contains(&key); }

   Iterator<V> *iterator() { return new Iterator<V>(new HashMapIterator(this)); }
};

/**
 * Hash map template for holding reference counting objects as values
 */
template <class K, class V> class RefCountHashMap : public HashMapBase
{
private:
   static void destructor(void *object, HashMapBase *map) { if (object != NULL) ((V*)object)->decRefCount(); }

public:
   RefCountHashMap(bool objectOwner = false) : HashMapBase(objectOwner, sizeof(K)) { m_objectDestructor = destructor; }

   V *get(const K& key) { V *v = (V*)_get(&key); if (v != NULL) v->incRefCount(); return v; }
   V *peek(const K& key) { return (V*)_get(&key); }
   void set(const K& key, V *value) { if (value != NULL) value->incRefCount(); _set(&key, (void *)value); }
   void remove(const K& key) { _remove(&key); }
   bool contains(const K& key) { return _contains(&key); }

   Iterator<V> *iterator() { return new Iterator<V>(new HashMapIterator(this)); }
};

/**
 * Ring buffer
 */
class LIBNETXMS_EXPORTABLE RingBuffer
{
   DISABLE_COPY_CTOR(RingBuffer)

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
   ~RingBuffer();

   void write(const BYTE *data, size_t dataSize);
   size_t read(BYTE *buffer, size_t bufferSize);
   BYTE readByte();

   void clear();

   void savePos();
   void restorePos();

   size_t size() const { return m_size; }
   bool isEmpty() const { return m_size == 0; }
};

/**
 * Byte stream
 */
class LIBNETXMS_EXPORTABLE ByteStream
{
   DISABLE_COPY_CTOR(ByteStream)

private:
   BYTE *m_data;
   size_t m_size;
   size_t m_allocated;
   size_t m_pos;
   size_t m_allocationStep;

public:
   ByteStream(size_t initial = 8192);
   ByteStream(const void *data, size_t size);
   virtual ~ByteStream();

   static ByteStream *load(const TCHAR *file);

   void seek(size_t pos) { if (pos <= m_size) m_pos = pos; }
   size_t pos() { return m_pos; }
   size_t size() { return m_size; }
   bool eos() { return m_pos == m_size; }

   void setAllocationStep(size_t s) { m_allocationStep = s; }

   const BYTE *buffer(size_t *size) { *size = m_size; return m_data; }

   void write(const void *data, size_t size);
   void write(char c) { write(&c, 1); }
   void write(BYTE b) { write(&b, 1); }
   void write(INT16 n) { UINT16 x = htons((UINT16)n); write(&x, 2); }
   void write(UINT16 n) { UINT16 x = htons(n); write(&x, 2); }
   void write(INT32 n) { UINT32 x = htonl((UINT32)n); write(&x, 4); }
   void write(UINT32 n) { UINT32 x = htonl(n); write(&x, 4); }
   void write(INT64 n) { UINT64 x = htonq((UINT64)n); write(&x, 8); }
   void write(UINT64 n) { UINT64 x = htonq(n); write(&x, 8); }
   void write(double n) { double x = htond(n); write(&x, 8); }
   void writeString(const TCHAR *s);
   void writeStringUtf8(const char *s);

   size_t read(void *buffer, size_t count);
   char readChar() { return !eos() ? (char)m_data[m_pos++] : 0; }
   BYTE readByte() { return !eos() ? m_data[m_pos++] : 0; }
   INT16 readInt16();
   UINT16 readUInt16();
   INT32 readInt32();
   UINT32 readUInt32();
   INT64 readInt64();
   UINT64 readUInt64();
   double readDouble();
   TCHAR *readString();
   char *readStringUtf8();

   bool save(int f);
};

/**
 * Auxilliary class for objects which counts references and
 * destroys itself wheren reference count falls to 0
 */
class LIBNETXMS_EXPORTABLE RefCountObject
{
   DISABLE_COPY_CTOR(RefCountObject)

private:
	VolatileCounter m_refCount;

protected:
   virtual ~RefCountObject();

public:
	RefCountObject()
   {
      m_refCount = 1;
   }

	void incRefCount()
   {
      InterlockedIncrement(&m_refCount);
   }

	void decRefCount()
   {
      if (InterlockedDecrement(&m_refCount) == 0)
         delete this;
   }
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
   INT32 m_dataType;
   bool m_instanceColumn;

public:
   TableColumnDefinition(const TCHAR *name, const TCHAR *displayName, INT32 dataType, bool isInstance);
   TableColumnDefinition(const NXCPMessage *msg, UINT32 baseId);
   TableColumnDefinition(const TableColumnDefinition *src);

   void fillMessage(NXCPMessage *msg, UINT32 baseId) const;

   const TCHAR *getName() const { return m_name; }
   const TCHAR *getDisplayName() const { return m_displayName; }
   INT32 getDataType() const { return m_dataType; }
   bool isInstanceColumn() const { return m_instanceColumn; }

   void setDataType(INT32 type) { m_dataType = type; }
   void setInstanceColumn(bool isInstance) { m_instanceColumn = isInstance; }
   void setDisplayName(const TCHAR *name);
};

/**
 * Table cell
 */
class TableCell
{
   DISABLE_COPY_CTOR(TableCell)

private:
   TCHAR *m_value;
   int m_status;
   UINT32 m_objectId;

public:
   TableCell() { m_value = NULL; m_status = -1; m_objectId = 0; }
   TableCell(const TCHAR *value) { m_value = MemCopyString(value); m_status = -1; m_objectId = 0; }
   TableCell(const TCHAR *value, int status) { m_value = MemCopyString(value); m_status = status; m_objectId = 0; }
   TableCell(TableCell *src) { m_value = MemCopyString(src->m_value); m_status = src->m_status; m_objectId = src->m_objectId; }
   ~TableCell() { MemFree(m_value); }

   void set(const TCHAR *value, int status, UINT32 objectId) { MemFree(m_value); m_value = MemCopyString(value); m_status = status; m_objectId = objectId; }
   void setPreallocated(TCHAR *value, int status, UINT32 objectId) { MemFree(m_value); m_value = value; m_status = status; m_objectId = objectId; }

   const TCHAR *getValue() const { return m_value; }
   void setValue(const TCHAR *value) { MemFree(m_value); m_value = MemCopyString(value); }
   void setPreallocatedValue(TCHAR *value) { MemFree(m_value); m_value = value; }

   int getStatus() const { return m_status; }
   void setStatus(int status) { m_status = status; }

   int getObjectId() const { return m_objectId; }
   void setObjectId(UINT32 id) { m_objectId = id; }
};

/**
 * Table row
 */
class TableRow
{
   DISABLE_COPY_CTOR(TableRow)

private:
   ObjectArray<TableCell> *m_cells;
   UINT32 m_objectId;
   int m_baseRow;

public:
   TableRow(int columnCount);
   TableRow(TableRow *src);
   ~TableRow() { delete m_cells; }

   void addColumn() { m_cells->add(new TableCell); }
   void deleteColumn(int index) { m_cells->remove(index); }

   void set(int index, const TCHAR *value, int status, UINT32 objectId) { TableCell *c = m_cells->get(index); if (c != NULL) c->set(value, status, objectId); }
   void setPreallocated(int index, TCHAR *value, int status, UINT32 objectId) { TableCell *c = m_cells->get(index); if (c != NULL) c->setPreallocated(value, status, objectId); }

   void setValue(int index, const TCHAR *value) { TableCell *c = m_cells->get(index); if (c != NULL) c->setValue(value); }
   void setPreallocatedValue(int index, TCHAR *value) { TableCell *c = m_cells->get(index); if (c != NULL) c->setPreallocatedValue(value); else free(value); }

   void setStatus(int index, int status) { TableCell *c = m_cells->get(index); if (c != NULL) c->setStatus(status); }

   const TCHAR *getValue(int index) const { const TableCell *c = m_cells->get(index); return (c != NULL) ? c->getValue() : NULL; }
   int getStatus(int index) const { const TableCell *c = m_cells->get(index); return (c != NULL) ? c->getStatus() : -1; }

   UINT32 getObjectId() const { return m_objectId; }
   void setObjectId(UINT32 id) { m_objectId = id; }

   int getBaseRow() const { return m_baseRow; }
   void setBaseRow(int baseRow) { m_baseRow = baseRow; }

   UINT32 getCellObjectId(int index) const { const TableCell *c = m_cells->get(index); return (c != NULL) ? c->getObjectId() : 0; }
   void setCellObjectId(int index, UINT32 id) { TableCell *c = m_cells->get(index); if (c != NULL) c->setObjectId(id); }
};

/**
 * Class for table data storage
 */
class LIBNETXMS_EXPORTABLE Table : public RefCountObject
{
   DISABLE_COPY_CTOR(Table)

private:
   ObjectArray<TableRow> *m_data;
   ObjectArray<TableColumnDefinition> *m_columns;
	TCHAR *m_title;
   int m_source;
   bool m_extendedFormat;

	void createFromMessage(NXCPMessage *msg);
	void destroy();
   bool parseXML(const char *xml);

public:
   Table();
   Table(Table *src);
   Table(NXCPMessage *msg);
   virtual ~Table();

	int fillMessage(NXCPMessage &msg, int offset, int rowLimit);
	void updateFromMessage(NXCPMessage *msg);

   void addAll(Table *src);
   void copyRow(Table *src, int row);

   int getNumRows() const { return m_data->size(); }
   int getNumColumns() const { return m_columns->size(); }
	const TCHAR *getTitle() const { return CHECK_NULL_EX(m_title); }
   int getSource() const { return m_source; }

   bool isExtendedFormat() { return m_extendedFormat; }
   void setExtendedFormat(bool ext) { m_extendedFormat = ext; }

   const TCHAR *getColumnName(int col) const { return ((col >= 0) && (col < m_columns->size())) ? m_columns->get(col)->getName() : NULL; }
   INT32 getColumnDataType(int col) const { return ((col >= 0) && (col < m_columns->size())) ? m_columns->get(col)->getDataType() : 0; }
   const TableColumnDefinition *getColumnDefinition(int col) const { return m_columns->get(col); }
	int getColumnIndex(const TCHAR *name) const;
   ObjectArray<TableColumnDefinition> *getColumnDefinitions() { return m_columns; }

	void setTitle(const TCHAR *title) { MemFree(m_title); m_title = MemCopyString(title); }
   void setSource(int source) { m_source = source; }
   int addColumn(const TCHAR *name, INT32 dataType = 0, const TCHAR *displayName = NULL, bool isInstance = false);
   int addColumn(const TableColumnDefinition *d);
   void setColumnDataType(int col, INT32 dataType) { if ((col >= 0) && (col < m_columns->size())) m_columns->get(col)->setDataType(dataType); }
   int addRow();

   void deleteRow(int row);
   void deleteColumn(int col);

   void setAt(int nRow, int nCol, INT32 nData);
   void setAt(int nRow, int nCol, UINT32 dwData);
   void setAt(int nRow, int nCol, double dData);
   void setAt(int nRow, int nCol, INT64 nData);
   void setAt(int nRow, int nCol, UINT64 qwData);
   void setAt(int nRow, int nCol, const TCHAR *pszData);
   void setPreallocatedAt(int nRow, int nCol, TCHAR *pszData);

   void set(int nCol, INT32 nData) { setAt(getNumRows() - 1, nCol, nData); }
   void set(int nCol, UINT32 dwData) { setAt(getNumRows() - 1, nCol, dwData); }
   void set(int nCol, double dData) { setAt(getNumRows() - 1, nCol, dData); }
   void set(int nCol, INT64 nData) { setAt(getNumRows() - 1, nCol, nData); }
   void set(int nCol, UINT64 qwData) { setAt(getNumRows() - 1, nCol, qwData); }
   void set(int nCol, const TCHAR *data) { setAt(getNumRows() - 1, nCol, data); }
#ifdef UNICODE
   void set(int nCol, const char *data) { setPreallocatedAt(getNumRows() - 1, nCol, WideStringFromMBString(data)); }
#else
   void set(int nCol, const WCHAR *data) { setPreallocatedAt(getNumRows() - 1, nCol, MBStringFromWideString(data)); }
#endif
   void setPreallocated(int nCol, TCHAR *data) { setPreallocatedAt(getNumRows() - 1, nCol, data); }

   void setStatusAt(int row, int col, int status);
   void setStatus(int col, int status) { setStatusAt(getNumRows() - 1, col, status); }

   const TCHAR *getAsString(int nRow, int nCol, const TCHAR *defaultValue = NULL) const;
   INT32 getAsInt(int nRow, int nCol) const;
   UINT32 getAsUInt(int nRow, int nCol) const;
   INT64 getAsInt64(int nRow, int nCol) const;
   UINT64 getAsUInt64(int nRow, int nCol) const;
   double getAsDouble(int nRow, int nCol) const;

   int getStatus(int nRow, int nCol) const;

   void buildInstanceString(int row, TCHAR *buffer, size_t bufLen);
   int findRowByInstance(const TCHAR *instance);

   UINT32 getObjectId(int row) const { const TableRow *r = m_data->get(row); return (r != NULL) ? r->getObjectId() : 0; }
   void setObjectIdAt(int row, UINT32 id) { TableRow *r = m_data->get(row); if (r != NULL) r->setObjectId(id); }
   void setObjectId(UINT32 id) { setObjectIdAt(getNumRows() - 1, id); }

   void setCellObjectIdAt(int row, int col, UINT32 objectId);
   void setCellObjectId(int col, UINT32 objectId) { setCellObjectIdAt(getNumRows() - 1, col, objectId); }
   UINT32 getCellObjectId(int row, int col) const { const TableRow *r = m_data->get(row); return (r != NULL) ? r->getCellObjectId(col) : 0; }

   void setBaseRowAt(int row, int baseRow);
   void setBaseRow(int baseRow) { setBaseRowAt(getNumRows() - 1, baseRow); }
   int getBaseRow(int row) const { const TableRow *r = m_data->get(row); return (r != NULL) ? r->getBaseRow() : 0; }

   void writeToTerminal();

   static Table *createFromXML(const char *xml);
   TCHAR *createXML() const;

   static Table *createFromPackedXML(const char *packedXml);
   char *createPackedXML() const;
};

/**
 * Create JSON string with null check
 */
inline json_t *json_string_a(const char *s)
{
   return (s != NULL) ? json_string(s) : json_null();
}

/**
 * Create JSON string from wide character string
 */
inline json_t *json_string_w(const WCHAR *s)
{
   if (s == NULL)
      return json_null();
   char *us = UTF8StringFromWideString(s);
   json_t *js = json_string(us);
   free(us);
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
template<typename T> json_t *json_integer_array(const T *values, size_t size)
{
   json_t *a = json_array();
   for(size_t i = 0; i < size; i++)
      json_array_append_new(a, json_integer(values[i]));
   return a;
}

/**
 * Serialize ObjectArray as JSON
 */
template<typename T> json_t *json_object_array(const ObjectArray<T> *a)
{
   json_t *root = json_array();
   if (a != NULL)
   {
      for(int i = 0; i < a->size(); i++)
      {
         T *e = a->get(i);
         if (e != NULL)
            json_array_append_new(root, e->toJson());
      }
   }
   return root;
}

/**
 * sockaddr buffer
 */
union SockAddrBuffer
{
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
inline bool SocketAddressEquals(struct sockaddr *a1, struct sockaddr *a2)
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
enum MacAddressNotation
{
   MAC_ADDR_FLAT_STRING = 0,
   MAC_ADDR_COLON_SEPARATED = 1,
   MAC_ADDR_BYTEPAIR_COLON_SEPARATED = 2,
   MAC_ADDR_HYPHEN_SEPARATED = 3,
   MAC_ADDR_DOT_SEPARATED = 4,
   MAC_ADDR_BYTEPAIR_DOT_SEPARATED = 5
};

/**
 * MAC address
 */
class LIBNETXMS_EXPORTABLE MacAddress
{
private:
   BYTE m_value[16];
   size_t m_length;

   TCHAR *toStringInternal(TCHAR *buffer, const TCHAR separator, bool bytePair = false) const;
   TCHAR *toStringInternal3(TCHAR *buffer, const TCHAR separator) const;

public:
   MacAddress(size_t length = 0) { m_length = MIN(length, 16); memset(m_value, 0, 16); }
   MacAddress(const BYTE *value, size_t length) { m_length = MIN(length, 16); memcpy(m_value, value, m_length); }
   MacAddress(const MacAddress& src) { memcpy(m_value, src.m_value, src.m_length); m_length = src.m_length; }

   static MacAddress parse(const char *str);
   static MacAddress parse(const WCHAR *str);

   const BYTE *value() const { return m_value; }
   size_t length() const { return m_length; }

   bool isValid() const;
   bool isMulticast() const;
   bool isBroadcast() const;
   bool equals(const MacAddress &a) const;

   TCHAR *toString(TCHAR *buffer, MacAddressNotation notation = MAC_ADDR_COLON_SEPARATED) const;
   String toString(MacAddressNotation notation = MAC_ADDR_COLON_SEPARATED) const;
};

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
      UINT32 v4;
      BYTE v6[16];
   } m_addr;

public:
   InetAddress();
   InetAddress(UINT32 addr);
   InetAddress(UINT32 addr, UINT32 mask);
   InetAddress(const BYTE *addr, int maskBits = 128);

   bool isAnyLocal() const;
   bool isLoopback() const;
   bool isMulticast() const;
   bool isBroadcast() const;
   bool isLinkLocal() const;
   bool isValid() const { return m_family != AF_UNSPEC; }
   bool isValidUnicast() const { return isValid() && !isAnyLocal() && !isLoopback() && !isMulticast() && !isBroadcast() && !isLinkLocal(); }

   int getFamily() const { return m_family; }
   UINT32 getAddressV4() const { return (m_family == AF_INET) ? m_addr.v4 : 0; }
   const BYTE *getAddressV6() const { return (m_family == AF_INET6) ? m_addr.v6 : (const BYTE *)"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"; }

   bool contain(const InetAddress &a) const;
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

   String toString() const;
   TCHAR *toString(TCHAR *buffer) const;
#ifdef UNICODE
   char *toStringA(char *buffer) const;
#else
   char *toStringA(char *buffer) const { return toString(buffer); }
#endif

   json_t *toJson() const;

   BYTE *buildHashKey(BYTE *key) const;

   TCHAR *getHostByAddr(TCHAR *buffer, size_t buflen) const;

   struct sockaddr *fillSockAddr(SockAddrBuffer *buffer, UINT16 port = 0) const;

   static InetAddress resolveHostName(const WCHAR *hostname, int af = AF_INET);
   static InetAddress resolveHostName(const char *hostname, int af = AF_INET);
   static InetAddress parse(const WCHAR *str);
   static InetAddress parse(const char *str);
   static InetAddress createFromSockaddr(struct sockaddr *s);

   static const InetAddress INVALID;
   static const InetAddress LOOPBACK;
   static const InetAddress NONE;
};

/**
 * IP address list
 */
class LIBNETXMS_EXPORTABLE InetAddressList
{
   DISABLE_COPY_CTOR(InetAddressList)

private:
   ObjectArray<InetAddress> *m_list;

   int indexOf(const InetAddress& addr) const;

public:
   InetAddressList();
   ~InetAddressList();

   void add(const InetAddress& addr);
   void add(const InetAddressList& addrList);
   void replace(const InetAddress& addr);
   void remove(const InetAddress& addr);
   void clear() { m_list->clear(); }
   const InetAddress& get(int index) const { const InetAddress *a = m_list->get(index); return (a != NULL) ? *a : InetAddress::INVALID; }

   int size() const { return m_list->size(); }
   bool hasAddress(const InetAddress& addr) const { return indexOf(addr) != -1; }
   const InetAddress& findAddress(const InetAddress& addr) const { int idx = indexOf(addr); return (idx != -1) ? *m_list->get(idx) : InetAddress::INVALID; }
   const InetAddress& findSameSubnetAddress(const InetAddress& addr) const;
   const InetAddress& getFirstUnicastAddress() const;
   const InetAddress& getFirstUnicastAddressV4() const;
   bool hasValidUnicastAddress() const { return getFirstUnicastAddress().isValid(); }
   bool isLoopbackOnly() const;

   const ObjectArray<InetAddress> *getList() const { return m_list; }

   void fillMessage(NXCPMessage *msg, UINT32 sizeFieldId, UINT32 baseFieldId) const;

   json_t *toJson() const { return json_object_array(m_list); }
};

/**
 * Network connection
 */
class LIBNETXMS_EXPORTABLE SocketConnection
{
   DISABLE_COPY_CTOR(SocketConnection)

protected:
	SOCKET m_socket;
	char m_data[4096];
	int m_dataPos;

   SocketConnection();

public:
	virtual ~SocketConnection();

	bool connectTCP(const TCHAR *hostName, WORD port, UINT32 timeout);
	bool connectTCP(const InetAddress& ip, WORD port, UINT32 timeout);
	void disconnect();

	bool canRead(UINT32 timeout);
	virtual int read(char *pBuff, int nSize, UINT32 timeout = INFINITE);
	bool waitForText(const char *text, int timeout);

	int write(const char *pBuff, int nSize);
	bool writeLine(const char *line);

	static SocketConnection *createTCPConnection(const TCHAR *hostName, WORD port, UINT32 timeout);
};

/**
 * Telnet connection - handles all telnet negotiation
 */
class LIBNETXMS_EXPORTABLE TelnetConnection : public SocketConnection
{
   DISABLE_COPY_CTOR(TelnetConnection)

protected:
   TelnetConnection() : SocketConnection() { }

	bool connectTCP(const TCHAR *hostName, WORD port, UINT32 timeout);
	bool connectTCP(const InetAddress& ip, WORD port, UINT32 timeout);

   bool connect(const TCHAR *hostName, WORD port, UINT32 timeout);
   bool connect(const InetAddress& ip, WORD port, UINT32 timeout);

public:
	static TelnetConnection *createConnection(const TCHAR *hostName, WORD port, UINT32 timeout);
   static TelnetConnection *createConnection(const InetAddress& ip, WORD port, UINT32 timeout);

	virtual int read(char *pBuff, int nSize, UINT32 timeout = INFINITE) OVERRIDE;
	int readLine(char *buffer, int size, UINT32 timeout = INFINITE);
};

/**
 * Postal address representation
 */
class LIBNETXMS_EXPORTABLE PostalAddress
{
   DISABLE_COPY_CTOR(PostalAddress)

private:
   TCHAR *m_country;
   TCHAR *m_city;
   TCHAR *m_streetAddress;
   TCHAR *m_postcode;

public:
   PostalAddress();
   PostalAddress(const TCHAR *country, const TCHAR *city, const TCHAR *streetAddress, const TCHAR *postcode);
   ~PostalAddress();

   const TCHAR *getCountry() const { return CHECK_NULL_EX(m_country); }
   const TCHAR *getCity() const { return CHECK_NULL_EX(m_city); }
   const TCHAR *getStreetAddress() const { return CHECK_NULL_EX(m_streetAddress); }
   const TCHAR *getPostCode() const { return CHECK_NULL_EX(m_postcode); }

   json_t *toJson() const;

   void setCountry(const TCHAR *country) { MemFree(m_country); m_country = MemCopyString(country); }
   void setCity(const TCHAR *city) { MemFree(m_city); m_city = MemCopyString(city); }
   void setStreetAddress(const TCHAR *streetAddress) { MemFree(m_streetAddress); m_streetAddress = MemCopyString(streetAddress); }
   void setPostCode(const TCHAR *postcode) { MemFree(m_postcode); m_postcode = MemCopyString(postcode); }
};

/**
 * Max number of polled sockets
 */
#define SOCKET_POLLER_MAX_SOCKETS    16

/**
 * Socket poller
 */
class LIBNETXMS_EXPORTABLE SocketPoller
{
   DISABLE_COPY_CTOR(SocketPoller)

private:
   bool m_write;
   int m_count;
#if HAVE_POLL
   struct pollfd m_sockets[SOCKET_POLLER_MAX_SOCKETS];
#else
   fd_set m_sockets;
#ifndef _WIN32
   SOCKET m_maxfd;
#endif
#endif

public:
   SocketPoller(bool write = false);
   ~SocketPoller();

   bool add(SOCKET s);
   int poll(UINT32 timeout);
   bool isSet(SOCKET s);
   void reset();
};

/**
 * Abstract communication channel
 */
class LIBNETXMS_EXPORTABLE AbstractCommChannel : public RefCountObject
{
   DISABLE_COPY_CTOR(AbstractCommChannel)

protected:
   virtual ~AbstractCommChannel();

public:
   AbstractCommChannel();

   virtual int send(const void *data, size_t size, MUTEX mutex = INVALID_MUTEX_HANDLE) = 0;
   virtual int recv(void *buffer, size_t size, UINT32 timeout = INFINITE) = 0;
   virtual int poll(UINT32 timeout, bool write = false) = 0;
   virtual int shutdown() = 0;
   virtual void close() = 0;
};

/**
 * Socket communication channel
 */
class LIBNETXMS_EXPORTABLE SocketCommChannel : public AbstractCommChannel
{
   DISABLE_COPY_CTOR(SocketCommChannel)

private:
   SOCKET m_socket;
   bool m_owner;

protected:
   virtual ~SocketCommChannel();

public:
   SocketCommChannel(SOCKET socket, bool owner = true);

   virtual int send(const void *data, size_t size, MUTEX mutex = INVALID_MUTEX_HANDLE) OVERRIDE;
   virtual int recv(void *buffer, size_t size, UINT32 timeout = INFINITE) OVERRIDE;
   virtual int poll(UINT32 timeout, bool write = false) OVERRIDE;
   virtual int shutdown() OVERRIDE;
   virtual void close() OVERRIDE;
};

#endif   /* __cplusplus */

/**
 * Configuration item template for configuration loader
 */
typedef struct
{
   TCHAR token[64];
   BYTE type;
   BYTE separator;     // Separator character for lists
   WORD listElements;  // Number of list elements, should be set to 0 before calling NxLoadConfig()
   UINT64 bufferSize;  // Buffer size for strings or flag to be set for CT_BOOLEAN
   UINT32 bufferPos;   // Should be set to 0
   void *buffer;
   void *overrideIndicator;
} NX_CFG_TEMPLATE;

/**
 * Code translation structure
 */
typedef struct __CODE_TO_TEXT
{
   int code;
   const TCHAR *text;
} CODE_TO_TEXT;

/**
 * getopt() prototype if needed
 */
#if USE_BUNDLED_GETOPT
#include <netxms_getopt.h>
#endif

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


/***** For compatibility *****/
#ifdef UNICODE
#define nx_strncpy      wcslcpy
#else
#define nx_strncpy      strlcpy
#endif

//
// Functions
//

#ifdef __cplusplus

int LIBNETXMS_EXPORTABLE ConnectEx(SOCKET s, struct sockaddr *addr, int len, UINT32 timeout);
int LIBNETXMS_EXPORTABLE SendEx(SOCKET hSocket, const void *data, size_t len, int flags, MUTEX mutex);
int LIBNETXMS_EXPORTABLE RecvEx(SOCKET hSocket, void *data, size_t len, int flags, UINT32 timeout);
bool LIBNETXMS_EXPORTABLE RecvAll(SOCKET s, void *buffer, size_t size, UINT32 timeout);

SOCKET LIBNETXMS_EXPORTABLE ConnectToHost(const InetAddress& addr, UINT16 port, UINT32 timeout);

int LIBNETXMS_EXPORTABLE BitsInMask(UINT32 mask);
int LIBNETXMS_EXPORTABLE BitsInMask(const BYTE *mask, size_t size);

TCHAR LIBNETXMS_EXPORTABLE *IpToStr(UINT32 dwAddr, TCHAR *szBuffer);
#ifdef UNICODE
char LIBNETXMS_EXPORTABLE *IpToStrA(UINT32 dwAddr, char *szBuffer);
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

#endif   /* __cplusplus */

#ifdef __cplusplus
extern "C"
{
#endif

void LIBNETXMS_EXPORTABLE InitNetXMSProcess(bool commandLineTool);

#ifndef _WIN32
#if defined(UNICODE_UCS2) || defined(UNICODE_UCS4)
void LIBNETXMS_EXPORTABLE __wcsupr(WCHAR *in);
#define wcsupr __wcsupr
#endif
void LIBNETXMS_EXPORTABLE __strupr(char *in);
#define strupr __strupr
#endif   /* _WIN32 */

void LIBNETXMS_EXPORTABLE QSortEx(void *base, size_t nmemb, size_t size, void *arg,
												 int (*compare)(const void *, const void *, void *));

INT64 LIBNETXMS_EXPORTABLE GetCurrentTimeMs();

UINT64 LIBNETXMS_EXPORTABLE FileSizeW(const WCHAR *pszFileName);
UINT64 LIBNETXMS_EXPORTABLE FileSizeA(const char *pszFileName);
#ifdef UNICODE
#define FileSize FileSizeW
#else
#define FileSize FileSizeA
#endif

void LIBNETXMS_EXPORTABLE nx_memswap(void *block1, void *block2, size_t size);

WCHAR LIBNETXMS_EXPORTABLE *BinToStrW(const void *data, size_t size, WCHAR *str);
char LIBNETXMS_EXPORTABLE *BinToStrA(const void *data, size_t size, char *str);
#ifdef UNICODE
#define BinToStr BinToStrW
#else
#define BinToStr BinToStrA
#endif

size_t LIBNETXMS_EXPORTABLE StrToBinW(const WCHAR *pStr, BYTE *data, size_t size);
size_t LIBNETXMS_EXPORTABLE StrToBinA(const char *pStr, BYTE *data, size_t size);
#ifdef UNICODE
#define StrToBin StrToBinW
#else
#define StrToBin StrToBinA
#endif

TCHAR LIBNETXMS_EXPORTABLE *MACToStr(const BYTE *data, TCHAR *pStr);

void LIBNETXMS_EXPORTABLE StrStripA(char *pszStr);
void LIBNETXMS_EXPORTABLE StrStripW(WCHAR *pszStr);
#ifdef UNICODE
#define StrStrip StrStripW
#else
#define StrStrip StrStripA
#endif

const char LIBNETXMS_EXPORTABLE *ExtractWordA(const char *line, char *buffer);
const WCHAR LIBNETXMS_EXPORTABLE *ExtractWordW(const WCHAR *line, WCHAR *buffer);
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

const TCHAR LIBNETXMS_EXPORTABLE *ExpandFileName(const TCHAR *name, TCHAR *buffer, size_t bufSize, bool allowShellCommand);
BOOL LIBNETXMS_EXPORTABLE CreateFolder(const TCHAR *directory);
bool LIBNETXMS_EXPORTABLE SetLastModificationTime(TCHAR *fileName, time_t lastModDate);
BOOL LIBNETXMS_EXPORTABLE CopyFileOrDirectory(const TCHAR *oldName, const TCHAR *newName);
BOOL LIBNETXMS_EXPORTABLE MoveFileOrDirectory(const TCHAR *oldName, const TCHAR *newName);

bool LIBNETXMS_EXPORTABLE MatchStringA(const char *pattern, const char *str, bool matchCase);
bool LIBNETXMS_EXPORTABLE MatchStringW(const WCHAR *pattern, const WCHAR *str, bool matchCase);
#ifdef UNICODE
#define MatchString MatchStringW
#else
#define MatchString MatchStringA
#endif

TCHAR LIBNETXMS_EXPORTABLE *Trim(TCHAR *str);
TCHAR LIBNETXMS_EXPORTABLE **SplitString(const TCHAR *source, TCHAR sep, int *numStrings);
int LIBNETXMS_EXPORTABLE GetLastMonthDay(struct tm *currTime);
bool LIBNETXMS_EXPORTABLE MatchScheduleElement(TCHAR *pszPattern, int nValue, int maxValue, struct tm *localTime, time_t currTime);
bool LIBNETXMS_EXPORTABLE MatchSchedule(const TCHAR *schedule, struct tm *currTime, time_t now);

#ifdef __cplusplus
BOOL LIBNETXMS_EXPORTABLE IsValidObjectName(const TCHAR *pszName, BOOL bExtendedChars = FALSE);
#endif
BOOL LIBNETXMS_EXPORTABLE IsValidScriptName(const TCHAR *pszName);
/* deprecated */ void LIBNETXMS_EXPORTABLE TranslateStr(TCHAR *pszString, const TCHAR *pszSubStr, const TCHAR *pszReplace);
const TCHAR LIBNETXMS_EXPORTABLE *GetCleanFileName(const TCHAR *pszFileName);
void LIBNETXMS_EXPORTABLE GetOSVersionString(TCHAR *pszBuffer, int nBufSize);
BYTE LIBNETXMS_EXPORTABLE *LoadFile(const TCHAR *pszFileName, UINT32 *pdwFileSize);
#ifdef UNICODE
BYTE LIBNETXMS_EXPORTABLE *LoadFileA(const char *pszFileName, UINT32 *pdwFileSize);
#else
#define LoadFileA LoadFile
#endif
char LIBNETXMS_EXPORTABLE *LoadFileAsUTF8String(const TCHAR *fileName);

UINT32 LIBNETXMS_EXPORTABLE CalculateCRC32(const unsigned char *data, UINT32 size, UINT32 dwCRC);
BOOL LIBNETXMS_EXPORTABLE CalculateFileCRC32(const TCHAR *pszFileName, UINT32 *pResult);

typedef unsigned char MD5_STATE[128];

void LIBNETXMS_EXPORTABLE CalculateMD5Hash(const BYTE *data, size_t nbytes, BYTE *hash);
void LIBNETXMS_EXPORTABLE MD5HashForPattern(const BYTE *data, size_t patternSize, size_t fullSize, BYTE *hash);
BOOL LIBNETXMS_EXPORTABLE CalculateFileMD5Hash(const TCHAR *pszFileName, BYTE *pHash);
void LIBNETXMS_EXPORTABLE MD5Init(MD5_STATE *state);
void LIBNETXMS_EXPORTABLE MD5Update(MD5_STATE *state, const BYTE *data, size_t size);
void LIBNETXMS_EXPORTABLE MD5Finish(MD5_STATE *state, BYTE *hash);

typedef unsigned char SHA1_STATE[128];

void LIBNETXMS_EXPORTABLE CalculateSHA1Hash(const BYTE *data, size_t nbytes, BYTE *hash);
void LIBNETXMS_EXPORTABLE SHA1HashForPattern(const BYTE *data, size_t patternSize, size_t fullSize, BYTE *hash);
BOOL LIBNETXMS_EXPORTABLE CalculateFileSHA1Hash(const TCHAR *pszFileName, BYTE *pHash);
void LIBNETXMS_EXPORTABLE SHA1Init(SHA1_STATE *state);
void LIBNETXMS_EXPORTABLE SHA1Update(SHA1_STATE *state, const BYTE *data, size_t size);
void LIBNETXMS_EXPORTABLE SHA1Finish(SHA1_STATE *state, BYTE *hash);

typedef unsigned char SHA256_STATE[256];

void LIBNETXMS_EXPORTABLE CalculateSHA256Hash(const BYTE *data, size_t len, BYTE *hash);
void LIBNETXMS_EXPORTABLE SHA256Init(SHA256_STATE *state);
void LIBNETXMS_EXPORTABLE SHA256Update(SHA256_STATE *state, const BYTE *data, size_t size);
void LIBNETXMS_EXPORTABLE SHA256Finish(SHA256_STATE *state, BYTE *hash);

void LIBNETXMS_EXPORTABLE GenerateRandomBytes(BYTE *buffer, size_t size);

void LIBNETXMS_EXPORTABLE LogOpenSSLErrorStack(int level);

void LIBNETXMS_EXPORTABLE ICEEncryptData(const BYTE *in, int inLen, BYTE *out, const BYTE *key);
void LIBNETXMS_EXPORTABLE ICEDecryptData(const BYTE *in, int inLen, BYTE *out, const BYTE *key);

bool LIBNETXMS_EXPORTABLE DecryptPasswordW(const WCHAR *login, const WCHAR *encryptedPasswd, WCHAR *decryptedPasswd, size_t bufferLenght);
bool LIBNETXMS_EXPORTABLE DecryptPasswordA(const char *login, const char *encryptedPasswd, char *decryptedPasswd, size_t bufferLenght);
#ifdef UNICODE
#define DecryptPassword DecryptPasswordW
#else
#define DecryptPassword DecryptPasswordA
#endif

int LIBNETXMS_EXPORTABLE NxDCIDataTypeFromText(const TCHAR *pszText);

HMODULE LIBNETXMS_EXPORTABLE DLOpen(const TCHAR *pszLibName, TCHAR *pszErrorText);
void LIBNETXMS_EXPORTABLE DLClose(HMODULE hModule);
void LIBNETXMS_EXPORTABLE *DLGetSymbolAddr(HMODULE hModule, const char *pszSymbol, TCHAR *pszErrorText);

bool LIBNETXMS_EXPORTABLE ExtractNamedOptionValueW(const WCHAR *optString, const WCHAR *option, WCHAR *buffer, int bufSize);
bool LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsBoolW(const WCHAR *optString, const WCHAR *option, bool defVal);
INT32 LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsIntW(const WCHAR *optString, const WCHAR *option, INT32 defVal);
UINT32 LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsUIntW(const WCHAR *optString, const WCHAR *option, UINT32 defVal);
UINT64 LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsUInt64W(const WCHAR *optString, const WCHAR *option, UINT64 defVal);

bool LIBNETXMS_EXPORTABLE ExtractNamedOptionValueA(const char *optString, const char *option, char *buffer, int bufSize);
bool LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsBoolA(const char *optString, const char *option, bool defVal);
INT32 LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsIntA(const char *optString, const char *option, INT32 defVal);
UINT32 LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsUIntA(const char *optString, const char *option, UINT32 defVal);
UINT64 LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsUInt64A(const char *optString, const char *option, UINT64 defVal);

#ifdef UNICODE
#define ExtractNamedOptionValue ExtractNamedOptionValueW
#define ExtractNamedOptionValueAsBool ExtractNamedOptionValueAsBoolW
#define ExtractNamedOptionValueAsInt ExtractNamedOptionValueAsIntW
#define ExtractNamedOptionValueAsUInt ExtractNamedOptionValueAsUIntW
#define ExtractNamedOptionValueAsUInt64 ExtractNamedOptionValueAsUInt64W
#else
#define ExtractNamedOptionValue ExtractNamedOptionValueA
#define ExtractNamedOptionValueAsBool ExtractNamedOptionValueAsBoolA
#define ExtractNamedOptionValueAsInt ExtractNamedOptionValueAsIntA
#define ExtractNamedOptionValueAsUInt ExtractNamedOptionValueAsUIntA
#define ExtractNamedOptionValueAsUInt64 ExtractNamedOptionValueAsUInt64A
#endif

#ifdef __cplusplus
const TCHAR LIBNETXMS_EXPORTABLE *CodeToText(int code, CODE_TO_TEXT *translator, const TCHAR *defaultText = _T("Unknown"));
#else
const TCHAR LIBNETXMS_EXPORTABLE *CodeToText(int code, CODE_TO_TEXT *translator, const TCHAR *defaultText);
#endif
#ifdef __cplusplus
int LIBNETXMS_EXPORTABLE CodeFromText(const TCHAR *text, CODE_TO_TEXT *translator, int defaultCode = -1);
#else
int LIBNETXMS_EXPORTABLE CodeFromText(const TCHAR *text, CODE_TO_TEXT *translator, int defaultCode);
#endif

#ifdef _WIN32
TCHAR LIBNETXMS_EXPORTABLE *GetSystemErrorText(UINT32 error, TCHAR *buffer, size_t size);
BOOL LIBNETXMS_EXPORTABLE GetWindowsVersionString(TCHAR *versionString, int strSize);
INT64 LIBNETXMS_EXPORTABLE GetProcessRSS();
#endif

TCHAR LIBNETXMS_EXPORTABLE *GetLastSocketErrorText(TCHAR *buffer, size_t size);

TCHAR LIBNETXMS_EXPORTABLE *GetLocalHostName(TCHAR *buffer, size_t size, bool fqdn);

#if !HAVE_DAEMON || !HAVE_DECL_DAEMON
int LIBNETXMS_EXPORTABLE __daemon(int nochdir, int noclose);
#define daemon __daemon
#endif

UINT32 LIBNETXMS_EXPORTABLE inet_addr_w(const WCHAR *pszAddr);

#ifndef _WIN32

bool LIBNETXMS_EXPORTABLE SetDefaultCodepage(const char *cp);
int LIBNETXMS_EXPORTABLE WideCharToMultiByte(int iCodePage, UINT32 dwFlags, const WCHAR *pWideCharStr,
                                             int cchWideChar, char *pByteStr, int cchByteChar,
                                             char *pDefaultChar, BOOL *pbUsedDefChar);
int LIBNETXMS_EXPORTABLE MultiByteToWideChar(int iCodePage, UINT32 dwFlags, const char *pByteStr,
                                             int cchByteChar, WCHAR *pWideCharStr,
                                             int cchWideChar);

#ifdef UNICODE
int LIBNETXMS_EXPORTABLE nx_wprintf(const WCHAR *format, ...);
int LIBNETXMS_EXPORTABLE nx_fwprintf(FILE *fp, const WCHAR *format, ...);
int LIBNETXMS_EXPORTABLE nx_swprintf(WCHAR *buffer, size_t size, const WCHAR *format, ...);
int LIBNETXMS_EXPORTABLE nx_vwprintf(const WCHAR *format, va_list args);
int LIBNETXMS_EXPORTABLE nx_vfwprintf(FILE *fp, const WCHAR *format, va_list args);
int LIBNETXMS_EXPORTABLE nx_vswprintf(WCHAR *buffer, size_t size, const WCHAR *format, va_list args);

int LIBNETXMS_EXPORTABLE nx_wscanf(const WCHAR *format, ...);
int LIBNETXMS_EXPORTABLE nx_fwscanf(FILE *fp, const WCHAR *format, ...);
int LIBNETXMS_EXPORTABLE nx_swscanf(const WCHAR *str, const WCHAR *format, ...);
int LIBNETXMS_EXPORTABLE nx_vwscanf(const WCHAR *format, va_list args);
int LIBNETXMS_EXPORTABLE nx_vfwscanf(FILE *fp, const WCHAR *format, va_list args);
int LIBNETXMS_EXPORTABLE nx_vswscanf(const WCHAR *str, const WCHAR *format, va_list args);
#endif

#endif	/* _WIN32 */

#ifdef _WITH_ENCRYPTION
WCHAR LIBNETXMS_EXPORTABLE *ERR_error_string_W(int nError, WCHAR *pwszBuffer);
#endif

#if !defined(_WIN32) && !HAVE_WSTAT
int LIBNETXMS_EXPORTABLE wstat(const WCHAR *_path, struct stat *_sbuf);
#endif

#if defined(UNICODE) && !defined(_WIN32)

#if !HAVE_WPOPEN
FILE LIBNETXMS_EXPORTABLE *wpopen(const WCHAR *_command, const WCHAR *_type);
#endif
#if !HAVE_WFOPEN
FILE LIBNETXMS_EXPORTABLE *wfopen(const WCHAR *_name, const WCHAR *_type);
#endif
#if HAVE_FOPEN64 && !HAVE_WFOPEN64
FILE LIBNETXMS_EXPORTABLE *wfopen64(const WCHAR *_name, const WCHAR *_type);
#endif
#if !HAVE_WOPEN
int LIBNETXMS_EXPORTABLE wopen(const WCHAR *, int, ...);
#endif
#if !HAVE_WCHMOD
int LIBNETXMS_EXPORTABLE wchmod(const WCHAR *_name, int mode);
#endif
#if !HAVE_WCHDIR
int LIBNETXMS_EXPORTABLE wchdir(const WCHAR *_path);
#endif
#if !HAVE_WMKDIR
int LIBNETXMS_EXPORTABLE wmkdir(const WCHAR *_path, int mode);
#endif
#if !HAVE_WUTIME
int LIBNETXMS_EXPORTABLE wutime(const WCHAR *_path, struct utimbuf *buf);
#endif
#if !HAVE_WRMDIR
int LIBNETXMS_EXPORTABLE wrmdir(const WCHAR *_path);
#endif
#if !HAVE_WRENAME
int LIBNETXMS_EXPORTABLE wrename(const WCHAR *_oldpath, const WCHAR *_newpath);
#endif
#if !HAVE_WUNLINK
int LIBNETXMS_EXPORTABLE wunlink(const WCHAR *_path);
#endif
#if !HAVE_WREMOVE
int LIBNETXMS_EXPORTABLE wremove(const WCHAR *_path);
#endif
#if !HAVE_WSYSTEM
int LIBNETXMS_EXPORTABLE wsystem(const WCHAR *_cmd);
#endif
#if !HAVE_WMKSTEMP
int LIBNETXMS_EXPORTABLE wmkstemp(WCHAR *_template);
#endif
#if !HAVE_WACCESS
int LIBNETXMS_EXPORTABLE waccess(const WCHAR *_path, int mode);
#endif
#if !HAVE_WGETENV
WCHAR LIBNETXMS_EXPORTABLE *wgetenv(const WCHAR *_string);
#endif
#if !HAVE_WCTIME
WCHAR LIBNETXMS_EXPORTABLE *wctime(const time_t *timep);
#endif
#if !HAVE_PUTWS
int LIBNETXMS_EXPORTABLE putws(const WCHAR *s);
#endif
#if !HAVE_WCSERROR && (HAVE_STRERROR || HAVE_DECL_STRERROR)
WCHAR LIBNETXMS_EXPORTABLE *wcserror(int errnum);
#endif
#if !HAVE_WCSERROR_R && HAVE_STRERROR_R
#if HAVE_POSIX_STRERROR_R
int LIBNETXMS_EXPORTABLE wcserror_r(int errnum, WCHAR *strerrbuf, size_t buflen);
#else
WCHAR LIBNETXMS_EXPORTABLE *wcserror_r(int errnum, WCHAR *strerrbuf, size_t buflen);
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

#if !defined(_WIN32) && (!HAVE_WCSFTIME || !WORKING_WCSFTIME)
size_t LIBNETXMS_EXPORTABLE nx_wcsftime(WCHAR *buffer, size_t bufsize, const WCHAR *format, const struct tm *t);
#undef wcsftime
#define wcsftime nx_wcsftime
#endif

#ifndef _WIN32

#if HAVE_ITOA && !HAVE__ITOA
#define _itoa itoa
#undef HAVE__ITOA
#define HAVE__ITOA 1
#endif
#if !HAVE__ITOA && !defined(_WIN32)
char LIBNETXMS_EXPORTABLE *_itoa(int value, char *str, int base);
#endif

#if HAVE_ITOW && !HAVE__ITOW
#define _itow itow
#undef HAVE__ITOW
#define HAVE__ITOW 1
#endif
#if !HAVE__ITOW && !defined(_WIN32)
WCHAR LIBNETXMS_EXPORTABLE *_itow(int value, WCHAR *str, int base);
#endif

#endif /* _WIN32 */

#ifdef _WIN32
#ifdef UNICODE
DIRW LIBNETXMS_EXPORTABLE *wopendir(const WCHAR *path);
struct dirent_w LIBNETXMS_EXPORTABLE *wreaddir(DIRW *p);
int LIBNETXMS_EXPORTABLE wclosedir(DIRW *p);

#define _topendir wopendir
#define _treaddir wreaddir
#define _tclosedir wclosedir
#else
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

TCHAR LIBNETXMS_EXPORTABLE *safe_fgetts(TCHAR *buffer, int len, FILE *f);

bool LIBNETXMS_EXPORTABLE nxlog_open(const TCHAR *logName, UINT32 flags, const TCHAR *msgModule,
                                     unsigned int msgCount, const TCHAR **messages,
                                     DWORD debugMsg, DWORD debugMsgTag, DWORD genericMsg);
void LIBNETXMS_EXPORTABLE nxlog_close();
void LIBNETXMS_EXPORTABLE nxlog_write(DWORD msg, WORD wType, const char *format, ...);
void LIBNETXMS_EXPORTABLE nxlog_write_generic(WORD type, const TCHAR *format, ...);
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
int LIBNETXMS_EXPORTABLE nxlog_get_debug_level();
int LIBNETXMS_EXPORTABLE nxlog_get_debug_level_tag(const TCHAR *tag);
int LIBNETXMS_EXPORTABLE nxlog_get_debug_level_tag_object(const TCHAR *tag, UINT32 objectId);
void LIBNETXMS_EXPORTABLE nxlog_reset_debug_level_tags();

#ifdef __cplusplus

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

#endif   /* __cplusplus */

typedef void (*NxLogDebugWriter)(const TCHAR *, const TCHAR *);
void LIBNETXMS_EXPORTABLE nxlog_set_debug_writer(NxLogDebugWriter writer);

typedef void (*NxLogConsoleWriter)(const TCHAR *, ...);
void LIBNETXMS_EXPORTABLE nxlog_set_console_writer(NxLogConsoleWriter writer);

void LIBNETXMS_EXPORTABLE WriteToTerminal(const TCHAR *text);
void LIBNETXMS_EXPORTABLE WriteToTerminalEx(const TCHAR *format, ...)
#if !defined(UNICODE) && (defined(__GNUC__) || defined(__clang__))
   __attribute__ ((format(printf, 1, 2)))
#endif
;

bool LIBNETXMS_EXPORTABLE ReadPassword(const TCHAR *prompt, TCHAR *buffer, size_t bufferSize);

#ifdef _WIN32
int LIBNETXMS_EXPORTABLE mkstemp(char *tmpl);
int LIBNETXMS_EXPORTABLE wmkstemp(WCHAR *tmpl);
#ifdef UNICODE
#define _tmkstemp wmkstemp
#else
#define _tmkstemp mkstemp
#endif
#endif

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
time_t LIBNETXMS_EXPORTABLE ParseDateTimeA(const char *text, time_t defaultValue);
time_t LIBNETXMS_EXPORTABLE ParseDateTimeW(const WCHAR *text, time_t defaultValue);

#ifdef UNICODE
#define ParseDateTime ParseDateTimeW
#else
#define ParseDateTime ParseDateTimeA
#endif

#ifdef __cplusplus
}
#endif

/***** C++ only functions *****/

#ifdef __cplusplus

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

#define EMA_FP_SHIFT  11                  /* nr of bits of precision */
#define EMA_FP_1      (1 << EMA_FP_SHIFT) /* 1.0 as fixed-point */
#define EMA_EXP_1     1884                /* 1/exp(5sec/1min) as fixed-point */
#define EMA_EXP_5     2014                /* 1/exp(5sec/5min) */
#define EMA_EXP_15    2037                /* 1/exp(5sec/15min) */

/**
 * Update xponential moving average value
 */
template<typename T> inline void UpdateExpMovingAverage(T& load, int exp, T n)
{
   load *= exp;
   load += n * (EMA_FP_1 - exp);
   load >>= EMA_FP_SHIFT;
}

TCHAR LIBNETXMS_EXPORTABLE *GetHeapInfo();
INT64 LIBNETXMS_EXPORTABLE GetAllocatedHeapMemory();
INT64 LIBNETXMS_EXPORTABLE GetActiveHeapMemory();
INT64 LIBNETXMS_EXPORTABLE GetMappedHeapMemory();

void LIBNETXMS_EXPORTABLE GetNetXMSDirectory(nxDirectoryType type, TCHAR *dir);

UINT32 LIBNETXMS_EXPORTABLE IcmpPing(const InetAddress& addr, int numRetries, UINT32 timeout, UINT32 *rtt, UINT32 packetSize, bool dontFragment);
UINT16 LIBNETXMS_EXPORTABLE CalculateIPChecksum(const void *data, size_t len);

TCHAR LIBNETXMS_EXPORTABLE *EscapeStringForXML(const TCHAR *str, int length);
String LIBNETXMS_EXPORTABLE EscapeStringForXML2(const TCHAR *str, int length = -1);
const char LIBNETXMS_EXPORTABLE *XMLGetAttr(const char **attrs, const char *name);
int LIBNETXMS_EXPORTABLE XMLGetAttrInt(const char **attrs, const char *name, int defVal);
UINT32 LIBNETXMS_EXPORTABLE XMLGetAttrUINT32(const char **attrs, const char *name, UINT32 defVal);
bool LIBNETXMS_EXPORTABLE XMLGetAttrBoolean(const char **attrs, const char *name, bool defVal);

String LIBNETXMS_EXPORTABLE EscapeStringForJSON(const TCHAR *s);
String LIBNETXMS_EXPORTABLE EscapeStringForAgent(const TCHAR *s);

StringList LIBNETXMS_EXPORTABLE *ParseCommandLine(const TCHAR *cmdline);

#if !defined(_WIN32) && !defined(_NETWARE) && defined(NMS_THREADS_H_INCLUDED)
void LIBNETXMS_EXPORTABLE BlockAllSignals(bool processWide, bool allowInterrupt);
void LIBNETXMS_EXPORTABLE StartMainLoop(ThreadFunction pfSignalHandler, ThreadFunction pfMain);
#endif

String LIBNETXMS_EXPORTABLE GenerateLineDiff(const String& left, const String& right);

bool LIBNETXMS_EXPORTABLE DeflateFile(const TCHAR *inputFile, const TCHAR *outputFile = NULL);
int LIBNETXMS_EXPORTABLE DeflateFileStream(FILE *source, FILE *dest, bool gzipFormat);

#endif

#endif   /* _nms_util_h_ */
