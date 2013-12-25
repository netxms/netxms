/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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

#ifdef _WIN32
#ifdef LIBNETXMS_EXPORTS
#define LIBNETXMS_EXPORTABLE __declspec(dllexport)
#else
#define LIBNETXMS_EXPORTABLE __declspec(dllimport)
#endif
#else    /* _WIN32 */
#define LIBNETXMS_EXPORTABLE
#endif


#include <nms_common.h>
#include <nms_cscp.h>
#include <nms_threads.h>
#include <time.h>

#if HAVE_BYTESWAP_H
#include <byteswap.h>
#endif

#include <base64.h>


//
// Serial communications
//

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
#define ICMP_API_ERROR        3

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
#define NXLOG_USE_SYSLOG		((UINT32)0x00000001)
#define NXLOG_PRINT_TO_STDOUT	((UINT32)0x00000002)
#define NXLOG_IS_OPEN         ((UINT32)0x80000000)

/**
 * nxlog rotation policy
 */
#define NXLOG_ROTATION_DISABLED  0
#define NXLOG_ROTATION_DAILY     1
#define NXLOG_ROTATION_BY_SIZE   2

/**
 * _tcsdup() replacement
 */
#if defined(_WIN32) && defined(USE_WIN32_HEAP)
#ifdef __cplusplus
extern "C" {
#endif
char LIBNETXMS_EXPORTABLE *nx__tcsdup(const char *src);
WCHAR LIBNETXMS_EXPORTABLE *nx_wcsdup(const WCHAR *src);
#ifdef __cplusplus
}
#endif
#endif

/**
 * Custom wcsdup
 */
#if !HAVE_WCSDUP && !defined(_WIN32)
#ifdef __cplusplus
extern "C" {
#endif
	WCHAR LIBNETXMS_EXPORTABLE *wcsdup(const WCHAR *src);
#ifdef __cplusplus
}
#endif
#endif

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

#ifndef _WIN32
	int m_hPort;
	struct termios m_originalSettings;
#else
	HANDLE m_hPort;
#endif

public:
	Serial();
	~Serial();

	bool open(const TCHAR *pszPort);
	void close();
	void setTimeout(int nTimeout);
	int read(char *pBuff, int nSize); /* waits up to timeout and do single read */
	int readAll(char *pBuff, int nSize); /* read until timeout or out of space */
	bool write(const char *pBuff, int nSize);
	void flush();
	bool set(int nSpeed, int nDataBits, int nParity, int nStopBits, int nFlowControl = FLOW_NONE);
	bool restart();
};

/**
 * Dynamic string class
 */
class LIBNETXMS_EXPORTABLE String
{
protected:
   TCHAR *m_pszBuffer;
   UINT32 m_dwBufSize;

public:
	static const int npos;

   String();
   String(const TCHAR *init);
	String(const String &src);
   ~String();

	TCHAR *getBuffer() { return m_pszBuffer; }
   void setBuffer(TCHAR *pszBuffer);

   const String& operator =(const TCHAR *pszStr);
	const String& operator =(const String &src);
   const String&  operator +=(const TCHAR *str);
   const String&  operator +=(const String &str);
   operator const TCHAR*() { return CHECK_NULL_EX(m_pszBuffer); }

	char *getUTF8String();

	void addString(const TCHAR *pStr, UINT32 dwLen);
	void addDynamicString(TCHAR *pszStr) { if (pszStr != NULL) { *this += pszStr; free(pszStr); } }

	void addMultiByteString(const char *pStr, UINT32 dwSize, int nCodePage);
	void addWideCharString(const WCHAR *pStr, UINT32 dwSize);

   void addFormattedString(const TCHAR *format, ...);
   void addFormattedStringV(const TCHAR *format, va_list args);

	UINT32 getSize() { return m_dwBufSize > 0 ? m_dwBufSize - 1 : 0; }
	BOOL isEmpty() { return m_dwBufSize <= 1; }

	TCHAR *subStr(int nStart, int nLen, TCHAR *pszBuffer);
	TCHAR *subStr(int nStart, int nLen) { return subStr(nStart, nLen, NULL); }
	int find(const TCHAR *pszStr, int nStart = 0);

   void escapeCharacter(int ch, int esc);
   void replace(const TCHAR *pszSrc, const TCHAR *pszDst);
	void trim();
	void shrink(int chars = 1);
};

/**
 * String maps base class
 */
class LIBNETXMS_EXPORTABLE StringMapBase
{
protected:
	UINT32 m_size;
	TCHAR **m_keys;
	void **m_values;
	bool m_objectOwner;
   bool m_ignoreCase;
	void (*m_objectDestructor)(void *);

	UINT32 find(const TCHAR *key);
	void setObject(TCHAR *key, void *value, bool keyPreAlloc);
	void *getObject(const TCHAR *key);
	void destroyObject(void *object) { if (object != NULL) m_objectDestructor(object); }

public:
	StringMapBase(bool objectOwner);
	virtual ~StringMapBase();

   void setOwner(bool owner) { m_objectOwner = owner; }
   void setIgnoreCase(bool ignore) { m_ignoreCase = ignore; }

	void remove(const TCHAR *key);
	void clear();

	UINT32 getSize() { return m_size; }
	const TCHAR *getKeyByIndex(UINT32 idx) { return (idx < m_size) ? CHECK_NULL_EX(m_keys[idx]) : NULL; }
};

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

	void set(const TCHAR *key, const TCHAR *value) { setObject((TCHAR *)key, _tcsdup(value), false); }
	void setPreallocated(TCHAR *key, TCHAR *value) { setObject(key, value, true); }
	void set(const TCHAR *key, UINT32 value);

	const TCHAR *get(const TCHAR *key) { return (const TCHAR *)getObject(key); }
	UINT32 getULong(const TCHAR *key, UINT32 defaultValue);
	bool getBoolean(const TCHAR *key, bool defaultValue);

	const TCHAR *getValueByIndex(UINT32 idx) { return (idx < m_size) ? CHECK_NULL_EX((TCHAR *)m_values[idx]) : NULL; }
};

/**
 * String map template for holding objects as values
 */
template <class T> class StringObjectMap : public StringMapBase
{
private:
	static void destructor(void *object) { delete (T*)object; }

public:
	StringObjectMap(bool objectOwner) : StringMapBase(objectOwner) { m_objectDestructor = destructor; }

	void set(const TCHAR *key, T *object) { setObject((TCHAR *)key, (void *)object, false); }
	T *get(const TCHAR *key) { return (T*)getObject(key); }
	T *getValueByIndex(UINT32 idx) { return (idx < m_size) ? (T *)m_values[idx] : NULL; }
};

/**
 * String list class
 */
class LIBNETXMS_EXPORTABLE StringList
{
private:
	int m_count;
	int m_allocated;
	TCHAR **m_values;

public:
	StringList();
	StringList(StringList *src);
	StringList(const TCHAR *src, const TCHAR *separator);
	~StringList();

	void add(const TCHAR *value);
	void addPreallocated(TCHAR *value);
	void add(INT32 value);
	void add(UINT32 value);
	void add(INT64 value);
	void add(UINT64 value);
	void add(double value);
	void replace(int index, const TCHAR *value);
	void clear();
	int getSize() { return m_count; }
	const TCHAR *getValue(int index) { return ((index >=0) && (index < m_count)) ? m_values[index] : NULL; }
	int getIndex(const TCHAR *value);
	int getIndexIgnoreCase(const TCHAR *value);
	void remove(int index);
   void addAll(const StringList *src);
   void merge(const StringList *src, bool matchCase);
   TCHAR *join(const TCHAR *separator);
   void splitAndAdd(const TCHAR *src, const TCHAR *separator);
};

/**
 * Entry of string set
 */
struct StringSetEntry;

/**
 * String set class
 */
class LIBNETXMS_EXPORTABLE StringSet
{
private:
   StringSetEntry *m_data;

public:
   StringSet();
   ~StringSet();

   void add(const TCHAR *str);
   void remove(const TCHAR *str);
   void clear();

   int size();
   bool exist(const TCHAR *str);

   void addAll(StringSet *src);
   void forEach(bool (*cb)(const TCHAR *, void *), void *userData);
};

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

	void internalRemove(int index, bool allowDestruction);
	void destroyObject(void *object) { if (object != NULL) m_objectDestructor(object); }

protected:
	void (*m_objectDestructor)(void *);

   Array(void *data, int initial, int grow, size_t elementSize);

public:
	Array(int initial = 0, int grow = 16, bool owner = false);
	virtual ~Array();

	int add(void *element);
   void *get(int index) { return ((index >= 0) && (index < m_size)) ? ((m_elementSize <= sizeof(void *)) ? m_data[index] : (void *)((char *)m_data + index * m_elementSize)): NULL; }
   int indexOf(void *element);
	void set(int index, void *element);
	void replace(int index, void *element);
	void remove(int index) { internalRemove(index, true); }
   void remove(void *element) { internalRemove(indexOf(element), true); }
	void unlink(int index) { internalRemove(index, false); }
	void unlink(void *element) { internalRemove(indexOf(element), false); }
	void clear();

	int size() { return m_size; }

	void setOwner(bool owner) { m_objectOwner = owner; }
	bool isOwner() { return m_objectOwner; }
};

/**
 * Template class for dynamic array which holds objects
 */
template <class T> class ObjectArray : public Array
{
private:
	static void destructor(void *object) { delete (T*)object; }

public:
	ObjectArray(int initial = 0, int grow = 16, bool owner = false) : Array(initial, grow, owner) { m_objectDestructor = destructor; }
	virtual ~ObjectArray() { }

	int add(T *object) { return Array::add((void *)object); }
	T *get(int index) { return (T*)Array::get(index); }
   int indexOf(T *object) { return Array::indexOf((void *)object); }
	void set(int index, T *object) { Array::set(index, (void *)object); }
	void replace(int index, T *object) { Array::replace(index, (void *)object); }
	void remove(int index) { Array::remove(index); }
   void remove(T *object) { Array::remove((void *)object); }
	void unlink(int index) { Array::unlink(index); }
   void unlink(T *object) { Array::unlink((void *)object); }
};

/**
 * Template class for dynamic array which holds scalar values
 */
template <class T> class IntegerArray : public Array
{
private:
	static void destructor(void *element) { }

public:
	IntegerArray(int initial = 0, int grow = 16) : Array(initial, grow, false) { m_objectDestructor = destructor; }
	virtual ~IntegerArray() { }

	int add(T value) { return Array::add(CAST_TO_POINTER(value, void *)); }
	T get(int index) { return CAST_FROM_POINTER(Array::get(index), T); }
	void set(int index, T value) { Array::set(index, CAST_TO_POINTER(value, void *)); }
	void replace(int index, T value) { Array::replace(index, CAST_TO_POINTER(value, void *)); }
};

/**
 * Auxilliary class to hold dynamically allocated array of structures
 */
template <class T> class StructArray : public Array
{
private:
	static void destructor(void *element) { }

public:
	StructArray(int initial = 0, int grow = 16) : Array(NULL, initial, grow, sizeof(T)) { m_objectDestructor = destructor; }
	StructArray(T *data, int size) : Array(data, size, 16, sizeof(T)) { m_objectDestructor = destructor; }
	virtual ~StructArray() { }

	int add(T *element) { return Array::add((void *)element); }
	T *get(int index) { return (T*)Array::get(index); }
   int indexOf(T *element) { return Array::indexOf((void *)element); }
	void set(int index, T *element) { Array::set(index, (void *)element); }
	void replace(int index, T *element) { Array::replace(index, (void *)element); }
	void remove(int index) { Array::remove(index); }
   void remove(T *element) { Array::remove((void *)element); }
	void unlink(int index) { Array::unlink(index); }
   void unlink(T *element) { Array::unlink((void *)element); }
};

/**
 * Auxilliary class for objects which counts references and
 * destroys itself wheren reference count falls to 0
 */
class LIBNETXMS_EXPORTABLE RefCountObject
{
private:
	VolatileCounter m_refCount;

public:
	RefCountObject()
   {
      m_refCount = 1;
   }
   virtual ~RefCountObject();

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

class CSCPMessage;

/**
 * Table column definition
 */
class LIBNETXMS_EXPORTABLE TableColumnDefinition
{
private:
   TCHAR *m_name;
   TCHAR *m_displayName;
   INT32 m_dataType;
   bool m_instanceColumn;

public:
   TableColumnDefinition(const TCHAR *name, const TCHAR *displayName, INT32 dataType, bool isInstance);
   TableColumnDefinition(CSCPMessage *msg, UINT32 baseId);
   TableColumnDefinition(TableColumnDefinition *src);
   ~TableColumnDefinition();

   void fillMessage(CSCPMessage *msg, UINT32 baseId);

   const TCHAR *getName() { return m_name; }
   const TCHAR *getDisplayName() { return m_displayName; }
   INT32 getDataType() { return m_dataType; }
   bool isInstanceColumn() { return m_instanceColumn; }

   void setDataType(INT32 type) { m_dataType = type; }
   void setInstanceColumn(bool isInstance) { m_instanceColumn = isInstance; }
   void setDisplayName(const TCHAR *name) { safe_free(m_displayName); m_displayName = _tcsdup(CHECK_NULL_EX(name)); }
};

/**
 * Class for table data storage
 */
class LIBNETXMS_EXPORTABLE Table : public RefCountObject
{
private:
   int m_nNumRows;
   int m_nNumCols;
   TCHAR **m_data;
   ObjectArray<TableColumnDefinition> *m_columns;
	TCHAR *m_title;
   int m_source;

	void createFromMessage(CSCPMessage *msg);
	void destroy();

public:
   Table();
   Table(Table *src);
   Table(CSCPMessage *msg);
   virtual ~Table();

	int fillMessage(CSCPMessage &msg, int offset, int rowLimit);
	void updateFromMessage(CSCPMessage *msg);

   void addAll(Table *src);
   void copyRow(Table *src, int row);

   int getNumRows() { return m_nNumRows; }
   int getNumColumns() { return m_nNumCols; }
	const TCHAR *getTitle() { return CHECK_NULL_EX(m_title); }
   int getSource() { return m_source; }

   const TCHAR *getColumnName(int col) { return ((col >= 0) && (col < m_columns->size())) ? m_columns->get(col)->getName() : NULL; }
   INT32 getColumnDataType(int col) { return ((col >= 0) && (col < m_columns->size())) ? m_columns->get(col)->getDataType() : 0; }
	int getColumnIndex(const TCHAR *name);
   ObjectArray<TableColumnDefinition> *getColumnDefinitions() { return m_columns; }

	void setTitle(const TCHAR *title) { safe_free(m_title); m_title = (title != NULL) ? _tcsdup(title) : NULL; }
   void setSource(int source) { m_source = source; }
   int addColumn(const TCHAR *name, INT32 dataType = 0, const TCHAR *displayName = NULL, bool isInstance = false);
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

   void set(int nCol, INT32 nData) { setAt(m_nNumRows - 1, nCol, nData); }
   void set(int nCol, UINT32 dwData) { setAt(m_nNumRows - 1, nCol, dwData); }
   void set(int nCol, double dData) { setAt(m_nNumRows - 1, nCol, dData); }
   void set(int nCol, INT64 nData) { setAt(m_nNumRows - 1, nCol, nData); }
   void set(int nCol, UINT64 qwData) { setAt(m_nNumRows - 1, nCol, qwData); }
   void set(int nCol, const TCHAR *pszData) { setAt(m_nNumRows - 1, nCol, pszData); }
   void setPreallocated(int nCol, TCHAR *pszData) { setPreallocatedAt(m_nNumRows - 1, nCol, pszData); }

   const TCHAR *getAsString(int nRow, int nCol);
   INT32 getAsInt(int nRow, int nCol);
   UINT32 getAsUInt(int nRow, int nCol);
   INT64 getAsInt64(int nRow, int nCol);
   UINT64 getAsUInt64(int nRow, int nCol);
   double getAsDouble(int nRow, int nCol);

   void buildInstanceString(int row, TCHAR *buffer, size_t bufLen);
   int findRowByInstance(const TCHAR *instance);
};

/**
 * Network connection
 */
class LIBNETXMS_EXPORTABLE SocketConnection
{
protected:
	SOCKET m_socket;
	char m_data[4096];
	int m_dataPos;

public:
	SocketConnection();
	virtual ~SocketConnection();

	bool connectTCP(const TCHAR *hostName, WORD port, UINT32 timeout);
	bool connectTCP(UINT32 ip, WORD port, UINT32 timeout);
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
protected:
	bool connectTCP(const TCHAR *hostName, WORD port, UINT32 timeout);
	bool connectTCP(UINT32 ip, WORD port, UINT32 timeout);

public:
	static TelnetConnection *createConnection(const TCHAR *hostName, WORD port, UINT32 timeout);

   bool connect(const TCHAR *hostName, WORD port, UINT32 timeout);
	bool connect(UINT32 ip, WORD port, UINT32 timeout);
	virtual int read(char *pBuff, int nSize, UINT32 timeout = INFINITE);
	int readLine(char *buffer, int size, UINT32 timeout = INFINITE);
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
   UINT32 bufferSize;  // Buffer size for strings or flag to be set for CT_BOOLEAN
   UINT32 bufferPos;   // Should be set to 0
   void *buffer;
   void *overrideIndicator;
} NX_CFG_TEMPLATE;

/**
 * Code translation structure
 */
typedef struct  __CODE_TO_TEXT
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
// Win32 API functions missing under WinCE
//

#if defined(UNDER_CE) && defined(__cplusplus)

inline void GetSystemTimeAsFileTime(LPFILETIME pFt)
{
	SYSTEMTIME sysTime;
	
	GetSystemTime(&sysTime);
	SystemTimeToFileTime(&sysTime, pFt);
}

#endif // UNDER_CE


//
// Structures for opendir() / readdir() / closedir()
//

#ifdef _WIN32

typedef struct dirent
{
   long            d_ino;  /* inode number (not used by MS-DOS) */
   int             d_namlen;       /* Name length */
   char            d_name[257];    /* file name */
} _DIRECT;

typedef struct _dir_struc
{
   char           *start;  /* Starting position */
   char           *curr;   /* Current position */
   long            size;   /* Size of string table */
   long            nfiles; /* number if filenames in table */
   struct dirent   dirstr; /* Directory structure to return */
} DIR;

#ifdef UNICODE

typedef struct dirent_w
{
   long            d_ino;  /* inode number (not used by MS-DOS) */
   int             d_namlen;       /* Name length */
   WCHAR           d_name[257];    /* file name */
} _DIRECTW;

typedef struct _dir_struc_w
{
   WCHAR          *start;  /* Starting position */
   WCHAR          *curr;   /* Current position */
   long            size;   /* Size of string table */
   long            nfiles; /* number if filenames in table */
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

#if WORDS_BIGENDIAN
#define htonq(x) (x)
#define ntohq(x) (x)
#define htond(x) (x)
#define ntohd(x) (x)
#define SwapWideString(x)
#else
#ifdef HAVE_HTONLL
#define htonq(x) htonll(x)
#else
#define htonq(x) __bswap_64(x)
#endif
#ifdef HAVE_NTOHLL
#define ntohq(x) ntohll(x)
#else
#define ntohq(x) __bswap_64(x)
#endif
#define htond(x) __bswap_double(x)
#define ntohd(x) __bswap_double(x)
#define SwapWideString(x)  __bswap_wstr(x)
#endif

#ifdef UNDER_CE
#define close(x)        CloseHandle((HANDLE)(x))
#endif

#ifdef __cplusplus
   inline TCHAR *nx_strncpy(TCHAR *pszDest, const TCHAR *pszSrc, size_t nLen)
   {
#if defined(_WIN32) && (_MSC_VER >= 1400)
		_tcsncpy_s(pszDest, nLen, pszSrc, _TRUNCATE);
#else
      _tcsncpy(pszDest, pszSrc, nLen - 1);
      pszDest[nLen - 1] = 0;
#endif
      return pszDest;
   }

#ifdef UNICODE
   inline char *nx_strncpy_mb(char *pszDest, const char *pszSrc, size_t nLen)
   {
#if defined(_WIN32) && (_MSC_VER >= 1400)
		strncpy_s(pszDest, nLen, pszSrc, _TRUNCATE);
#else
      strncpy(pszDest, pszSrc, nLen - 1);
      pszDest[nLen - 1] = 0;
#endif
      return pszDest;
   }
#else
#define nx_strncpy_mb nx_strncpy
#endif

int LIBNETXMS_EXPORTABLE ConnectEx(SOCKET s, struct sockaddr *addr, int len, UINT32 timeout);
int LIBNETXMS_EXPORTABLE SendEx(SOCKET, const void *, size_t, int, MUTEX);
int LIBNETXMS_EXPORTABLE RecvEx(SOCKET nSocket, const void *pBuff, size_t nSize, int nFlags, UINT32 dwTimeout);
#endif   /* __cplusplus */

#ifdef __cplusplus
extern "C"
{
#endif
#if defined(_WIN32) || !(HAVE_DECL___BSWAP_32)
   UINT32 LIBNETXMS_EXPORTABLE __bswap_32(UINT32 dwVal);
#endif
#if defined(_WIN32) || !(HAVE_DECL___BSWAP_64)
   UINT64 LIBNETXMS_EXPORTABLE __bswap_64(UINT64 qwVal);
#endif
   double LIBNETXMS_EXPORTABLE __bswap_double(double dVal);
   void LIBNETXMS_EXPORTABLE __bswap_wstr(UCS2CHAR *pStr);

#if !defined(_WIN32) && !defined(_NETWARE)
#if defined(UNICODE_UCS2) || defined(UNICODE_UCS4)
   void LIBNETXMS_EXPORTABLE wcsupr(WCHAR *in);
#endif   
   void LIBNETXMS_EXPORTABLE strupr(char *in);
#endif
   
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

   int LIBNETXMS_EXPORTABLE BitsInMask(UINT32 dwMask);
   TCHAR LIBNETXMS_EXPORTABLE *IpToStr(UINT32 dwAddr, TCHAR *szBuffer);
#ifdef UNICODE
   char LIBNETXMS_EXPORTABLE *IpToStrA(UINT32 dwAddr, char *szBuffer);
#else
#define IpToStrA IpToStr
#endif
	TCHAR LIBNETXMS_EXPORTABLE *Ip6ToStr(BYTE *addr, TCHAR *buffer);
	TCHAR LIBNETXMS_EXPORTABLE *SockaddrToStr(struct sockaddr *addr, TCHAR *buffer);

   UINT32 LIBNETXMS_EXPORTABLE ResolveHostName(const TCHAR *pszName);
#ifdef UNICODE
   UINT32 LIBNETXMS_EXPORTABLE ResolveHostNameA(const char *pszName);
#else
#define ResolveHostNameA ResolveHostName
#endif

   void LIBNETXMS_EXPORTABLE *nx_memdup(const void *pData, UINT32 dwSize);
   void LIBNETXMS_EXPORTABLE nx_memswap(void *pBlock1, void *pBlock2, UINT32 dwSize);

   WCHAR LIBNETXMS_EXPORTABLE *BinToStrW(const BYTE *pData, UINT32 dwSize, WCHAR *pStr);
   char LIBNETXMS_EXPORTABLE *BinToStrA(const BYTE *pData, UINT32 dwSize, char *pStr);
#ifdef UNICODE
#define BinToStr BinToStrW
#else
#define BinToStr BinToStrA
#endif

   UINT32 LIBNETXMS_EXPORTABLE StrToBinW(const WCHAR *pStr, BYTE *pData, UINT32 dwSize);
   UINT32 LIBNETXMS_EXPORTABLE StrToBinA(const char *pStr, BYTE *pData, UINT32 dwSize);
#ifdef UNICODE
#define StrToBin StrToBinW
#else
#define StrToBin StrToBinA
#endif
   
   TCHAR LIBNETXMS_EXPORTABLE *MACToStr(const BYTE *pData, TCHAR *pStr);

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

   int LIBNETXMS_EXPORTABLE NumCharsA(const char *pszStr, char ch);
   int LIBNETXMS_EXPORTABLE NumCharsW(const WCHAR *pszStr, WCHAR ch);
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

	BOOL LIBNETXMS_EXPORTABLE RegexpMatchA(const char *pszStr, const char *pszExpr, BOOL bMatchCase);
	BOOL LIBNETXMS_EXPORTABLE RegexpMatchW(const WCHAR *pszStr, const WCHAR *pszExpr, BOOL bMatchCase);
#ifdef UNICODE
#define RegexpMatch RegexpMatchW
#else
#define RegexpMatch RegexpMatchA
#endif

	const TCHAR LIBNETXMS_EXPORTABLE *ExpandFileName(const TCHAR *name, TCHAR *buffer, size_t bufSize);
	void LIBNETXMS_EXPORTABLE Trim(TCHAR *str);
   bool LIBNETXMS_EXPORTABLE MatchString(const TCHAR *pattern, const TCHAR *str, bool matchCase);
	TCHAR LIBNETXMS_EXPORTABLE **SplitString(const TCHAR *source, TCHAR sep, int *numStrings);

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

   UINT32 LIBNETXMS_EXPORTABLE CalculateCRC32(const unsigned char *pData, UINT32 dwSize, UINT32 dwCRC);
   void LIBNETXMS_EXPORTABLE CalculateMD5Hash(const unsigned char *data, size_t nbytes, unsigned char *hash);
	void LIBNETXMS_EXPORTABLE MD5HashForPattern(const unsigned char *data, size_t patternSize, size_t fullSize, BYTE *hash);
   void LIBNETXMS_EXPORTABLE CalculateSHA1Hash(unsigned char *data, size_t nbytes, unsigned char *hash);
   void LIBNETXMS_EXPORTABLE SHA1HashForPattern(unsigned char *data, size_t patternSize, size_t fullSize, unsigned char *hash);
   BOOL LIBNETXMS_EXPORTABLE CalculateFileMD5Hash(const TCHAR *pszFileName, BYTE *pHash);
   BOOL LIBNETXMS_EXPORTABLE CalculateFileSHA1Hash(const TCHAR *pszFileName, BYTE *pHash);
   BOOL LIBNETXMS_EXPORTABLE CalculateFileCRC32(const TCHAR *pszFileName, UINT32 *pResult);

	void LIBNETXMS_EXPORTABLE ICEEncryptData(const BYTE *in, int inLen, BYTE *out, const BYTE *key);
	void LIBNETXMS_EXPORTABLE ICEDecryptData(const BYTE *in, int inLen, BYTE *out, const BYTE *key);

	BOOL LIBNETXMS_EXPORTABLE DecryptPassword(const TCHAR *login, const TCHAR *encryptedPasswd, TCHAR *decryptedPasswd);

   UINT32 LIBNETXMS_EXPORTABLE IcmpPing(UINT32 dwAddr, int iNumRetries, UINT32 dwTimeout,
                                       UINT32 *pdwRTT, UINT32 dwPacketSize);

   int LIBNETXMS_EXPORTABLE NxDCIDataTypeFromText(const TCHAR *pszText);

   HMODULE LIBNETXMS_EXPORTABLE DLOpen(const TCHAR *pszLibName, TCHAR *pszErrorText);
   void LIBNETXMS_EXPORTABLE DLClose(HMODULE hModule);
   void LIBNETXMS_EXPORTABLE *DLGetSymbolAddr(HMODULE hModule, const char *pszSymbol, TCHAR *pszErrorText);

	bool LIBNETXMS_EXPORTABLE ExtractNamedOptionValueW(const WCHAR *optString, const WCHAR *option, WCHAR *buffer, int bufSize);
	bool LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsBoolW(const WCHAR *optString, const WCHAR *option, bool defVal);
	long LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsIntW(const WCHAR *optString, const WCHAR *option, long defVal);

	bool LIBNETXMS_EXPORTABLE ExtractNamedOptionValueA(const char *optString, const char *option, char *buffer, int bufSize);
	bool LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsBoolA(const char *optString, const char *option, bool defVal);
	long LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsIntA(const char *optString, const char *option, long defVal);

#ifdef UNICODE
#define ExtractNamedOptionValue ExtractNamedOptionValueW
#define ExtractNamedOptionValueAsBool ExtractNamedOptionValueAsBoolW
#define ExtractNamedOptionValueAsInt ExtractNamedOptionValueAsIntW
#else
#define ExtractNamedOptionValue ExtractNamedOptionValueA
#define ExtractNamedOptionValueAsBool ExtractNamedOptionValueAsBoolA
#define ExtractNamedOptionValueAsInt ExtractNamedOptionValueAsIntA
#endif

#ifdef __cplusplus
	const TCHAR LIBNETXMS_EXPORTABLE *CodeToText(int iCode, CODE_TO_TEXT *pTranslator, const TCHAR *pszDefaultText = _T("Unknown"));
#else
	const TCHAR LIBNETXMS_EXPORTABLE *CodeToText(int iCode, CODE_TO_TEXT *pTranslator, const TCHAR *pszDefaultText);
#endif

#ifdef _WIN32
   TCHAR LIBNETXMS_EXPORTABLE *GetSystemErrorText(UINT32 dwError, TCHAR *pszBuffer, size_t iBufSize);
	BOOL LIBNETXMS_EXPORTABLE GetWindowsVersionString(TCHAR *versionString, int strSize);
	INT64 LIBNETXMS_EXPORTABLE GetProcessRSS();
#endif

#if !(HAVE_DAEMON)
   int LIBNETXMS_EXPORTABLE daemon(int nochdir, int noclose);
#endif

   UINT32 LIBNETXMS_EXPORTABLE inet_addr_w(const WCHAR *pszAddr);

#ifndef _WIN32
	BOOL LIBNETXMS_EXPORTABLE SetDefaultCodepage(const char *cp);
   int LIBNETXMS_EXPORTABLE WideCharToMultiByte(int iCodePage, UINT32 dwFlags, const WCHAR *pWideCharStr, 
                                                int cchWideChar, char *pByteStr, int cchByteChar, 
                                                char *pDefaultChar, BOOL *pbUsedDefChar);
   int LIBNETXMS_EXPORTABLE MultiByteToWideChar(int iCodePage, UINT32 dwFlags, const char *pByteStr, 
                                                int cchByteChar, WCHAR *pWideCharStr, 
                                                int cchWideChar);

#if !defined(UNICODE_UCS2) || !HAVE_WCSLEN
	int LIBNETXMS_EXPORTABLE ucs2_strlen(const UCS2CHAR *pStr);
#endif
#if !defined(UNICODE_UCS2) || !HAVE_WCSNCPY
	UCS2CHAR LIBNETXMS_EXPORTABLE *ucs2_strncpy(UCS2CHAR *pDst, const UCS2CHAR *pSrc, int nDstLen);
#endif
#if !defined(UNICODE_UCS2) || !HAVE_WCSDUP
	UCS2CHAR LIBNETXMS_EXPORTABLE *ucs2__tcsdup(const UCS2CHAR *pStr);
#endif

	size_t LIBNETXMS_EXPORTABLE ucs2_to_mb(const UCS2CHAR *src, int srcLen, char *dst, int dstLen);
	size_t LIBNETXMS_EXPORTABLE mb_to_ucs2(const char *src, int srcLen, UCS2CHAR *dst, int dstLen);
	UCS2CHAR LIBNETXMS_EXPORTABLE *UCS2StringFromMBString(const char *pszString);
	char LIBNETXMS_EXPORTABLE *MBStringFromUCS2String(const UCS2CHAR *pszString);

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

#endif	/* _WIN32 */

   WCHAR LIBNETXMS_EXPORTABLE *WideStringFromMBString(const char *pszString);
	WCHAR LIBNETXMS_EXPORTABLE *WideStringFromUTF8String(const char *pszString);
   char LIBNETXMS_EXPORTABLE *MBStringFromWideString(const WCHAR *pwszString);
   char LIBNETXMS_EXPORTABLE *UTF8StringFromWideString(const WCHAR *pwszString);
   
#ifdef _WITH_ENCRYPTION
	WCHAR LIBNETXMS_EXPORTABLE *ERR_error_string_W(int nError, WCHAR *pwszBuffer);
#endif

#ifdef UNICODE_UCS4
	size_t LIBNETXMS_EXPORTABLE ucs2_to_ucs4(const UCS2CHAR *src, int srcLen, WCHAR *dst, int dstLen);
	size_t LIBNETXMS_EXPORTABLE ucs4_to_ucs2(const WCHAR *src, int srcLen, UCS2CHAR *dst, int dstLen);
	size_t LIBNETXMS_EXPORTABLE ucs2_to_utf8(const UCS2CHAR *src, int srcLen, char *dst, int dstLen);
	UCS2CHAR LIBNETXMS_EXPORTABLE *UCS2StringFromUCS4String(const WCHAR *pwszString);
	WCHAR LIBNETXMS_EXPORTABLE *UCS4StringFromUCS2String(const UCS2CHAR *pszString);
#endif

#if !defined(_WIN32) && !HAVE_WSTAT
	int wstat(const WCHAR *_path, struct stat *_sbuf);
#endif

#if defined(UNICODE) && !defined(_WIN32)

#if !HAVE_WPOPEN
	FILE LIBNETXMS_EXPORTABLE *wpopen(const WCHAR *_command, const WCHAR *_type);
#endif
#if !HAVE_WFOPEN
	FILE LIBNETXMS_EXPORTABLE *wfopen(const WCHAR *_name, const WCHAR *_type);
#endif
#if !HAVE_WOPEN
	int LIBNETXMS_EXPORTABLE wopen(const WCHAR *, int, ...);
#endif
#if !HAVE_WCHMOD
	int LIBNETXMS_EXPORTABLE wchmod(const WCHAR *_name, int mode);
#endif
#if !HAVE_WCHDIR
	int wchdir(const WCHAR *_path);
#endif
#if !HAVE_WMKDIR
	int wmkdir(const WCHAR *_path, int mode);
#endif
#if !HAVE_WRMDIR
	int wrmdir(const WCHAR *_path);
#endif
#if !HAVE_WRENAME
	int wrename(const WCHAR *_oldpath, const WCHAR *_newpath);
#endif
#if !HAVE_WUNLINK
	int wunlink(const WCHAR *_path);
#endif
#if !HAVE_WREMOVE
	int wremove(const WCHAR *_path);
#endif
#if !HAVE_WSYSTEM
	int wsystem(const WCHAR *_cmd);
#endif
#if !HAVE_WMKSTEMP
	int wmkstemp(const WCHAR *_template);
#endif
#if !HAVE_WACCESS
	int waccess(const WCHAR *_path, int mode);
#endif
#if !HAVE_WGETENV
	WCHAR *wgetenv(const WCHAR *_string);
#endif
#if !HAVE_WCTIME
	WCHAR *wctime(const time_t *timep);
#endif
#if !HAVE_PUTWS
	int putws(const WCHAR *s);
#endif
#if !HAVE_WCSERROR && HAVE_STRERROR
	WCHAR *wcserror(int errnum);
#endif
#if !HAVE_WCSERROR_R && HAVE_STRERROR_R
#if HAVE_POSIX_STRERROR_R
	int wcserror_r(int errnum, WCHAR *strerrbuf, size_t buflen);
#else
	WCHAR *wcserror_r(int errnum, WCHAR *strerrbuf, size_t buflen);
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

#if !HAVE_WCSCASECMP && !defined(_WIN32)
	int LIBNETXMS_EXPORTABLE wcscasecmp(const wchar_t *s1, const wchar_t *s2);
#endif

#if !HAVE_WCSNCASECMP && !defined(_WIN32)
	int LIBNETXMS_EXPORTABLE wcsncasecmp(const wchar_t *s1, const wchar_t *s2, size_t n);
#endif

#ifdef _WIN32
#ifdef UNICODE
    DIRW LIBNETXMS_EXPORTABLE *wopendir(const WCHAR *filename);
    struct dirent_w LIBNETXMS_EXPORTABLE *wreaddir(DIRW *dirp);
    int LIBNETXMS_EXPORTABLE wclosedir(DIRW *dirp);

#define _topendir wopendir
#define _treaddir wreaddir
#define _tclosedir wclosedir
#else
#define _topendir opendir
#define _treaddir readdir
#define _tclosedir closedir
#endif

    DIR LIBNETXMS_EXPORTABLE *opendir(const char *filename);
    struct dirent LIBNETXMS_EXPORTABLE *readdir(DIR *dirp);
    int LIBNETXMS_EXPORTABLE closedir(DIR *dirp);

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

#ifdef UNDER_CE
int LIBNETXMS_EXPORTABLE _topen(TCHAR *pszName, int nFlags, ...);
int LIBNETXMS_EXPORTABLE read(int hFile, void *pBuffer, size_t nBytes);
int LIBNETXMS_EXPORTABLE write(int hFile, void *pBuffer, size_t nBytes);
#endif

BOOL LIBNETXMS_EXPORTABLE nxlog_open(const TCHAR *logName, UINT32 flags, const TCHAR *msgModule,
                                     unsigned int msgCount, const TCHAR **messages);
void LIBNETXMS_EXPORTABLE nxlog_close(void);
void LIBNETXMS_EXPORTABLE nxlog_write(DWORD msg, WORD wType, const char *format, ...);
BOOL LIBNETXMS_EXPORTABLE nxlog_set_rotation_policy(int rotationMode, int maxLogSize, int historySize, const TCHAR *dailySuffix);
BOOL LIBNETXMS_EXPORTABLE nxlog_rotate();

typedef void (*NxLogConsoleWriter)(const TCHAR *, ...);
void LIBNETXMS_EXPORTABLE nxlog_set_console_writer(NxLogConsoleWriter writer);

void LIBNETXMS_EXPORTABLE WriteToTerminal(const TCHAR *text);
void LIBNETXMS_EXPORTABLE WriteToTerminalEx(const TCHAR *format, ...);

#ifdef _WIN32
int LIBNETXMS_EXPORTABLE mkstemp(char *tmpl);
int LIBNETXMS_EXPORTABLE wmkstemp(WCHAR *tmpl);
#ifdef UNICODE
#define _tmkstemp wmkstemp
#else
#define _tmkstemp mkstemp
#endif
#endif

#ifndef _WIN32
int strcat_s(char *dst, size_t dstSize, const char *src);
int wcscat_s(WCHAR *dst, size_t dstSize, const WCHAR *src);
#endif

#ifdef __cplusplus
}
#endif


//
// C++ only finctions
//

#ifdef __cplusplus

TCHAR LIBNETXMS_EXPORTABLE *EscapeStringForXML(const TCHAR *str, int length);
String LIBNETXMS_EXPORTABLE EscapeStringForXML2(const TCHAR *str, int length = -1);
const char LIBNETXMS_EXPORTABLE *XMLGetAttr(const char **attrs, const char *name);
int LIBNETXMS_EXPORTABLE XMLGetAttrInt(const char **attrs, const char *name, int defVal);
UINT32 LIBNETXMS_EXPORTABLE XMLGetAttrUINT32(const char **attrs, const char *name, UINT32 defVal);
bool LIBNETXMS_EXPORTABLE XMLGetAttrBoolean(const char **attrs, const char *name, bool defVal);

#if !defined(_WIN32) && !defined(_NETWARE) && defined(NMS_THREADS_H_INCLUDED)
void LIBNETXMS_EXPORTABLE StartMainLoop(ThreadFunction pfSignalHandler, ThreadFunction pfMain);
#endif

void LIBNETXMS_EXPORTABLE InitSubAgentAPI(void (* writeLog)(int, int, const TCHAR *),
                                          void (* sendTrap1)(UINT32, const TCHAR *, const char *, va_list),
                                          void (* sendTrap2)(UINT32, const TCHAR *, int, TCHAR **),
                                          bool (* sendFile)(void *, UINT32, const TCHAR *, long),
                                          bool (* pushData)(const TCHAR *, const TCHAR *, UINT32));

#endif

#endif   /* _nms_util_h_ */
