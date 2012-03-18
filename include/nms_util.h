/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
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


//
// Return codes for IcmpPing()
//

#define ICMP_SUCCESS          0
#define ICMP_UNREACHEABLE     1
#define ICMP_TIMEOUT          2
#define ICMP_RAW_SOCK_FAILED  3
#define ICMP_API_ERROR        3


//
// Token types for configuration loader
//

#define CT_LONG         0
#define CT_STRING       1
#define CT_STRING_LIST  2
#define CT_END_OF_LIST  3
#define CT_BOOLEAN      4
#define CT_WORD         5
#define CT_IGNORE       6


//
// Return codes for NxLoadConfig()
//

#define NXCFG_ERR_OK       0
#define NXCFG_ERR_NOFILE   1
#define NXCFG_ERR_SYNTAX   2


//
// nxlog_open() flags
//

#define NXLOG_USE_SYSLOG		((DWORD)0x00000001)
#define NXLOG_PRINT_TO_STDOUT	((DWORD)0x00000002)
#define NXLOG_IS_OPEN         ((DWORD)0x80000000)


//
// nxlog rotation policy
//

#define NXLOG_ROTATION_DISABLED  0
#define NXLOG_ROTATION_DAILY     1
#define NXLOG_ROTATION_BY_SIZE   2


//
// _tcsdup() replacement
//

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


//
// Class for serial communications
//

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
public:
	Serial(void);
	~Serial(void);

	bool Open(const TCHAR *pszPort);
	void Close(void);
	void SetTimeout(int nTimeout);
	int Read(char *pBuff, int nSize); /* waits up to timeout and do single read */
	int ReadAll(char *pBuff, int nSize); /* read until timeout or out of space */
	bool Write(const char *pBuff, int nSize);
	void Flush(void);
	bool Set(int nSpeed, int nDataBits, int nParity, int nStopBits);
	bool Set(int nSpeed, int nDataBits, int nParity, int nStopBits, int nFlowControl);
	bool Restart(void);

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
};


//
// Class for table data storage
//

class CSCPMessage;

class LIBNETXMS_EXPORTABLE Table
{
private:
   int m_nNumRows;
   int m_nNumCols;
   TCHAR **m_ppData;
   TCHAR **m_ppColNames;
	LONG *m_colFormats;
	TCHAR *m_title;

public:
   Table();
   Table(CSCPMessage *msg);
   ~Table();

	int fillMessage(CSCPMessage &msg, int offset, int rowLimit);

   int getNumRows() { return m_nNumRows; }
   int getNumColumns() { return m_nNumCols; }
	const TCHAR *getTitle() { return CHECK_NULL_EX(m_title); }

	const TCHAR *getColumnName(int col) { return ((col >= 0) && (col < m_nNumCols)) ? m_ppColNames[col] : NULL; }
	LONG getColumnFormat(int col) { return ((col >= 0) && (col < m_nNumCols)) ? m_colFormats[col] : 0; }

	void setTitle(const TCHAR *title) { safe_free(m_title); m_title = (title != NULL) ? _tcsdup(title) : NULL; }
   int addColumn(const TCHAR *name, LONG format = 0);
   int addRow();

   void setAt(int nRow, int nCol, LONG nData);
   void setAt(int nRow, int nCol, DWORD dwData);
   void setAt(int nRow, int nCol, double dData);
   void setAt(int nRow, int nCol, INT64 nData);
   void setAt(int nRow, int nCol, QWORD qwData);
   void setAt(int nRow, int nCol, const TCHAR *pszData);
   void setPreallocatedAt(int nRow, int nCol, TCHAR *pszData);

   void set(int nCol, LONG nData) { setAt(m_nNumRows - 1, nCol, nData); }
   void set(int nCol, DWORD dwData) { setAt(m_nNumRows - 1, nCol, dwData); }
   void set(int nCol, double dData) { setAt(m_nNumRows - 1, nCol, dData); }
   void set(int nCol, INT64 nData) { setAt(m_nNumRows - 1, nCol, nData); }
   void set(int nCol, QWORD qwData) { setAt(m_nNumRows - 1, nCol, qwData); }
   void set(int nCol, const TCHAR *pszData) { setAt(m_nNumRows - 1, nCol, pszData); }
   void setPreallocated(int nCol, TCHAR *pszData) { setPreallocatedAt(m_nNumRows - 1, nCol, pszData); }

   const TCHAR *getAsString(int nRow, int nCol);
   LONG getAsInt(int nRow, int nCol);
   DWORD getAsUInt(int nRow, int nCol);
   INT64 getAsInt64(int nRow, int nCol);
   QWORD getAsUInt64(int nRow, int nCol);
   double getAsDouble(int nRow, int nCol);
};


//
// Dynamic string class
//

class LIBNETXMS_EXPORTABLE String
{
protected:
   TCHAR *m_pszBuffer;
   DWORD m_dwBufSize;

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
   const String&  operator +=(const TCHAR *pszStr);
   operator const TCHAR*() { return CHECK_NULL_EX(m_pszBuffer); }

	char *getUTF8String();

	void addString(const TCHAR *pStr, DWORD dwLen);
	void addDynamicString(TCHAR *pszStr) { if (pszStr != NULL) { *this += pszStr; free(pszStr); } }

	void addMultiByteString(const char *pStr, DWORD dwSize, int nCodePage);
	void addWideCharString(const WCHAR *pStr, DWORD dwSize);

   void addFormattedString(const TCHAR *format, ...);
   void addFormattedStringV(const TCHAR *format, va_list args);
   void escapeCharacter(int ch, int esc);
   void translate(const TCHAR *pszSrc, const TCHAR *pszDst);

	DWORD getSize() { return m_dwBufSize > 0 ? m_dwBufSize - 1 : 0; }
	BOOL isEmpty() { return m_dwBufSize <= 1; }

	TCHAR *subStr(int nStart, int nLen, TCHAR *pszBuffer);
	TCHAR *subStr(int nStart, int nLen) { return subStr(nStart, nLen, NULL); }
	int find(const TCHAR *pszStr, int nStart = 0);

	void trim();
	void shrink(int chars = 1);
};


//
// String map class
//

class LIBNETXMS_EXPORTABLE StringMap
{
protected:
	DWORD m_size;
	TCHAR **m_keys;
	TCHAR **m_values;

	DWORD find(const TCHAR *key);

public:
	StringMap();
	StringMap(const StringMap &src);
	~StringMap();

	StringMap& operator =(const StringMap &src);

	void set(const TCHAR *key, const TCHAR *value);
	void setPreallocated(TCHAR *key, TCHAR *value);
	void set(const TCHAR *key, DWORD value);

	const TCHAR *get(const TCHAR *key);
	DWORD getULong(const TCHAR *key, DWORD defaultValue);
	bool getBoolean(const TCHAR *key, bool defaultValue);

	void remove(const TCHAR *key);
	void clear();

	DWORD getSize() { return m_size; }
	const TCHAR *getKeyByIndex(DWORD idx) { return (idx < m_size) ? CHECK_NULL_EX(m_keys[idx]) : NULL; }
	const TCHAR *getValueByIndex(DWORD idx) { return (idx < m_size) ? CHECK_NULL_EX(m_values[idx]) : NULL; }
};


//
// String list class
//

class LIBNETXMS_EXPORTABLE StringList
{
private:
	int m_count;
	int m_allocated;
	TCHAR **m_values;

public:
	StringList();
	~StringList();

	void add(const TCHAR *value);
	void addPreallocated(TCHAR *value);
	void add(LONG value);
	void add(DWORD value);
	void add(INT64 value);
	void add(QWORD value);
	void add(double value);
	void replace(int index, const TCHAR *value);
	void clear();
	int getSize() { return m_count; }
	const TCHAR *getValue(int index) { return ((index >=0) && (index < m_count)) ? m_values[index] : NULL; }
	int getIndex(const TCHAR *value);
	int getIndexIgnoreCase(const TCHAR *value);
};


//
// Dynamic array class
//

class LIBNETXMS_EXPORTABLE Array
{
private:
	int m_size;
	int m_allocated;
	int m_grow;
	void **m_data;
	bool m_objectOwner;

protected:
	virtual void destroyObject(void *object);

public:
	Array(int initial = 0, int grow = 16, bool owner = false);
	virtual ~Array();

	int add(void *object);
	void *get(int index) { return ((index >= 0) && (index < m_size)) ? m_data[index] : NULL; }
	void set(int index, void *object);
	void replace(int index, void *object);
	void remove(int index);
	void clear();

	int size() { return m_size; }

	void setOwner(bool owner) { m_objectOwner = owner; }
	bool isOwner() { return m_objectOwner; }
};

template <class T> class ObjectArray : public Array
{
protected:
	virtual void destroyObject(void *object) { delete (T*)object; }

public:
	ObjectArray(int initial = 0, int grow = 16, bool owner = false) : Array(initial, grow, owner) { }
	virtual ~ObjectArray() { }

	int add(T *object) { return Array::add((void *)object); }
	T *get(int index) { return (T*)Array::get(index); }
	void set(int index, T *object) { Array::set(index, (void *)object); }
	void replace(int index, T *object) { Array::replace(index, (void *)object); }
};


//
// Auxilliary class to hold dynamically allocated array of structures
//

template <class T> class StructArray
{
private:
	int m_count;
	T *m_data;

public:
	StructArray(T *data, int count) { m_data = data; m_count = count; }
	~StructArray() { safe_free(m_data); }

	int size() { return m_count; }
	T *get(int index) { return ((index >= 0) && (index < m_count)) ? &m_data[index] : NULL; }
};


//
// Auxilliary class for objects which counts references and
// destroys itself wheren reference count falls to 0
//

class LIBNETXMS_EXPORTABLE RefCountObject
{
private:
	int m_refCount;
	MUTEX m_mutex;

public:
	RefCountObject();
	virtual ~RefCountObject();

	void incRefCount();
	void decRefCount();
};


//
// Network connection
//

class LIBNETXMS_EXPORTABLE SocketConnection
{
protected:
	SOCKET m_socket;
	char m_data[4096];
	int m_dataPos;

public:
	SocketConnection();
	virtual ~SocketConnection();

	bool connectTCP(const TCHAR *hostName, WORD port, DWORD timeout);
	void disconnect();

	bool canRead(DWORD timeout);
	int read(char *pBuff, int nSize, DWORD timeout = INFINITE);
	bool waitForText(const char *text, int timeout);
	
	int write(const char *pBuff, int nSize);
	bool writeLine(const char *line);

	static SocketConnection *createTCPConnection(const TCHAR *hostName, WORD port, DWORD timeout);
};

#endif   /* __cplusplus */


//
// Configuration item template for configuration loader
//

typedef struct
{
   TCHAR szToken[64];
   BYTE iType;
   BYTE cSeparator;     // Separator character for lists
   WORD wListElements;  // Number of list elements, should be set to 0 before calling NxLoadConfig()
   DWORD dwBufferSize;  // Buffer size for strings or flag to be set for CT_BOOLEAN
   DWORD dwBufferPos;   // Should be set to 0
   void *pBuffer;
} NX_CFG_TEMPLATE;


//
// Code translation structure
//

typedef struct  __CODE_TO_TEXT
{
   int code;
   const TCHAR *text;
} CODE_TO_TEXT;


//
// getopt() prototype if needed
//

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
#ifndef LIBNETXMS_INLINE
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
#endif
#else
TCHAR LIBNETXMS_EXPORTABLE *nx_strncpy(TCHAR *pszDest, const TCHAR *pszSrc, size_t nLen);
#endif

#ifdef __cplusplus
int LIBNETXMS_EXPORTABLE ConnectEx(SOCKET s, struct sockaddr *addr, int len, DWORD timeout);
int LIBNETXMS_EXPORTABLE SendEx(SOCKET, const void *, size_t, int, MUTEX);
int LIBNETXMS_EXPORTABLE RecvEx(SOCKET nSocket, const void *pBuff,
                                size_t nSize, int nFlags, DWORD dwTimeout);
#endif

#ifdef __cplusplus
extern "C"
{
#endif
#if defined(_WIN32) || !(HAVE_DECL___BSWAP_32)
   DWORD LIBNETXMS_EXPORTABLE __bswap_32(DWORD dwVal);
#endif
#if defined(_WIN32) || !(HAVE_DECL___BSWAP_64)
   QWORD LIBNETXMS_EXPORTABLE __bswap_64(QWORD qwVal);
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

   INT64 LIBNETXMS_EXPORTABLE GetCurrentTimeMs(void);
   QWORD LIBNETXMS_EXPORTABLE FileSize(const TCHAR *pszFileName);

   int LIBNETXMS_EXPORTABLE BitsInMask(DWORD dwMask);
   TCHAR LIBNETXMS_EXPORTABLE *IpToStr(DWORD dwAddr, TCHAR *szBuffer);
#ifdef UNICODE
   char LIBNETXMS_EXPORTABLE *IpToStrA(DWORD dwAddr, char *szBuffer);
#else
#define IpToStrA IpToStr
#endif
	TCHAR LIBNETXMS_EXPORTABLE *Ip6ToStr(BYTE *addr, TCHAR *buffer);
	TCHAR LIBNETXMS_EXPORTABLE *SockaddrToStr(struct sockaddr *addr, TCHAR *buffer);

   DWORD LIBNETXMS_EXPORTABLE ResolveHostName(const TCHAR *pszName);
#ifdef UNICODE
   DWORD LIBNETXMS_EXPORTABLE ResolveHostNameA(const char *pszName);
#else
#define ResolveHostNameA ResolveHostName
#endif

   void LIBNETXMS_EXPORTABLE *nx_memdup(const void *pData, DWORD dwSize);
   void LIBNETXMS_EXPORTABLE nx_memswap(void *pBlock1, void *pBlock2, DWORD dwSize);

   TCHAR LIBNETXMS_EXPORTABLE *BinToStr(const BYTE *pData, DWORD dwSize, TCHAR *pStr);
   DWORD LIBNETXMS_EXPORTABLE StrToBin(const TCHAR *pStr, BYTE *pData, DWORD dwSize);
   TCHAR LIBNETXMS_EXPORTABLE *MACToStr(const BYTE *pData, TCHAR *pStr);

   void LIBNETXMS_EXPORTABLE StrStrip(TCHAR *pszStr);
   void LIBNETXMS_EXPORTABLE Trim(TCHAR *str);
   BOOL LIBNETXMS_EXPORTABLE MatchString(const TCHAR *pattern, const TCHAR *string, BOOL matchCase);
	BOOL LIBNETXMS_EXPORTABLE RegexpMatch(const TCHAR *pszStr, const TCHAR *pszExpr, BOOL bMatchCase);
   const TCHAR LIBNETXMS_EXPORTABLE *ExtractWord(const TCHAR *line, TCHAR *buffer);
	TCHAR LIBNETXMS_EXPORTABLE **SplitString(const TCHAR *source, TCHAR sep, int *numStrings);
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

#ifdef __cplusplus
   BOOL LIBNETXMS_EXPORTABLE IsValidObjectName(const TCHAR *pszName, BOOL bExtendedChars = FALSE);
#endif
   BOOL LIBNETXMS_EXPORTABLE IsValidScriptName(const TCHAR *pszName);
   /* deprecated */ void LIBNETXMS_EXPORTABLE TranslateStr(TCHAR *pszString, const TCHAR *pszSubStr, const TCHAR *pszReplace);
   const TCHAR LIBNETXMS_EXPORTABLE *GetCleanFileName(const TCHAR *pszFileName);
   void LIBNETXMS_EXPORTABLE GetOSVersionString(TCHAR *pszBuffer, int nBufSize);
	BYTE LIBNETXMS_EXPORTABLE *LoadFile(const TCHAR *pszFileName, DWORD *pdwFileSize);
#ifdef UNICODE
	BYTE LIBNETXMS_EXPORTABLE *LoadFileA(const char *pszFileName, DWORD *pdwFileSize);
#else
#define LoadFileA LoadFile
#endif

   DWORD LIBNETXMS_EXPORTABLE CalculateCRC32(const unsigned char *pData, DWORD dwSize, DWORD dwCRC);
   void LIBNETXMS_EXPORTABLE CalculateMD5Hash(const unsigned char *data, size_t nbytes, unsigned char *hash);
	void LIBNETXMS_EXPORTABLE MD5HashForPattern(const unsigned char *data, size_t patternSize, size_t fullSize, BYTE *hash);
   void LIBNETXMS_EXPORTABLE CalculateSHA1Hash(unsigned char *data, size_t nbytes, unsigned char *hash);
   void LIBNETXMS_EXPORTABLE SHA1HashForPattern(unsigned char *data, size_t patternSize, size_t fullSize, unsigned char *hash);
   BOOL LIBNETXMS_EXPORTABLE CalculateFileMD5Hash(const TCHAR *pszFileName, BYTE *pHash);
   BOOL LIBNETXMS_EXPORTABLE CalculateFileSHA1Hash(const TCHAR *pszFileName, BYTE *pHash);
   BOOL LIBNETXMS_EXPORTABLE CalculateFileCRC32(const TCHAR *pszFileName, DWORD *pResult);

	void LIBNETXMS_EXPORTABLE ICEEncryptData(const BYTE *in, int inLen, BYTE *out, const BYTE *key);
	void LIBNETXMS_EXPORTABLE ICEDecryptData(const BYTE *in, int inLen, BYTE *out, const BYTE *key);

	BOOL LIBNETXMS_EXPORTABLE DecryptPassword(const TCHAR *login, const TCHAR *encryptedPasswd, TCHAR *decryptedPasswd);

   DWORD LIBNETXMS_EXPORTABLE IcmpPing(DWORD dwAddr, int iNumRetries, DWORD dwTimeout,
                                       DWORD *pdwRTT, DWORD dwPacketSize);

   /* deprecated:
	DWORD LIBNETXMS_EXPORTABLE NxLoadConfig(const TCHAR *pszFileName, const TCHAR *pszSection, 
                                           NX_CFG_TEMPLATE *pTemplateList, BOOL bPrint);*/
   int LIBNETXMS_EXPORTABLE NxDCIDataTypeFromText(const TCHAR *pszText);

   HMODULE LIBNETXMS_EXPORTABLE DLOpen(const TCHAR *pszLibName, TCHAR *pszErrorText);
   void LIBNETXMS_EXPORTABLE DLClose(HMODULE hModule);
   void LIBNETXMS_EXPORTABLE *DLGetSymbolAddr(HMODULE hModule, const TCHAR *pszSymbol, TCHAR *pszErrorText);

	BOOL LIBNETXMS_EXPORTABLE ExtractNamedOptionValue(const TCHAR *optString, const TCHAR *option, TCHAR *buffer, int bufSize);
	BOOL LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsBool(const TCHAR *optString, const TCHAR *option, BOOL defVal);
	long LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsInt(const TCHAR *optString, const TCHAR *option, long defVal);

#ifdef __cplusplus
	const TCHAR LIBNETXMS_EXPORTABLE *CodeToText(int iCode, CODE_TO_TEXT *pTranslator, const TCHAR *pszDefaultText = _T("Unknown"));
#else
	const TCHAR LIBNETXMS_EXPORTABLE *CodeToText(int iCode, CODE_TO_TEXT *pTranslator, const TCHAR *pszDefaultText);
#endif

#ifdef _WIN32
   TCHAR LIBNETXMS_EXPORTABLE *GetSystemErrorText(DWORD dwError, TCHAR *pszBuffer, size_t iBufSize);
	BOOL LIBNETXMS_EXPORTABLE GetWindowsVersionString(TCHAR *versionString, int strSize);
	INT64 LIBNETXMS_EXPORTABLE GetProcessRSS();
#endif

#if !(HAVE_DAEMON)
   int LIBNETXMS_EXPORTABLE daemon(int nochdir, int noclose);
#endif

   DWORD LIBNETXMS_EXPORTABLE inet_addr_w(const WCHAR *pszAddr);

#ifndef _WIN32
	BOOL LIBNETXMS_EXPORTABLE SetDefaultCodepage(const char *cp);
   int LIBNETXMS_EXPORTABLE WideCharToMultiByte(int iCodePage, DWORD dwFlags, const WCHAR *pWideCharStr, 
                                                int cchWideChar, char *pByteStr, int cchByteChar, 
                                                char *pDefaultChar, BOOL *pbUsedDefChar);
   int LIBNETXMS_EXPORTABLE MultiByteToWideChar(int iCodePage, DWORD dwFlags, const char *pByteStr, 
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

#ifndef UNICODE
	size_t LIBNETXMS_EXPORTABLE ucs2_to_mb(const UCS2CHAR *src, int srcLen, char *dst, int dstLen);
	size_t LIBNETXMS_EXPORTABLE mb_to_ucs2(const char *src, int srcLen, UCS2CHAR *dst, int dstLen);
	UCS2CHAR LIBNETXMS_EXPORTABLE *UCS2StringFromMBString(const char *pszString);
	char LIBNETXMS_EXPORTABLE *MBStringFromUCS2String(const UCS2CHAR *pszString);
#endif

#ifdef UNICODE
	int LIBNETXMS_EXPORTABLE nx_wprintf(const WCHAR *format, ...);
	int LIBNETXMS_EXPORTABLE nx_fwprintf(FILE *fp, const WCHAR *format, ...);
	int LIBNETXMS_EXPORTABLE nx_swprintf(WCHAR *buffer, size_t size, const WCHAR *format, ...);
	int LIBNETXMS_EXPORTABLE nx_vwprintf(const WCHAR *format, va_list args);
	int LIBNETXMS_EXPORTABLE nx_vfwprintf(FILE *fp, const WCHAR *format, va_list args);
	int LIBNETXMS_EXPORTABLE nx_vswprintf(WCHAR *buffer, size_t size, const WCHAR *format, va_list args);
#endif

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

#ifdef UNICODE

#if !HAVE_WFOPEN
	FILE LIBNETXMS_EXPORTABLE *wfopen(const WCHAR *_name, const WCHAR *_type);
#endif
#if !HAVE_WOPEN
	int LIBNETXMS_EXPORTABLE wopen(const WCHAR *, int, ...);
#endif
#if !HAVE_WSTAT
	int wstat(const WCHAR *_path, struct stat *_sbuf);
#endif
#if !HAVE_WRENAME
	int wrename(const WCHAR *_oldpath, const WCHAR *_newpath);
#endif
#if !HAVE_WUNLINK
	int wunlink(const WCHAR *_path);
#endif
#if !HAVE_WGETENV
	WCHAR *wgetenv(const WCHAR *_string);
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

#endif	/* UNICODE */

#if !HAVE_STRTOLL
	INT64 LIBNETXMS_EXPORTABLE strtoll(const char *nptr, char **endptr, int base);
#endif
#if !HAVE_STRTOULL
	QWORD LIBNETXMS_EXPORTABLE strtoull(const char *nptr, char **endptr, int base);
#endif

#if !HAVE_WCSTOLL
	INT64 LIBNETXMS_EXPORTABLE wcstoll(const WCHAR *nptr, WCHAR **endptr, int base);
#endif
#if !HAVE_WCSTOULL
	QWORD LIBNETXMS_EXPORTABLE wcstoull(const WCHAR *nptr, WCHAR **endptr, int base);
#endif

#if !HAVE_WCSDUP && !defined(_WIN32)
	WCHAR LIBNETXMS_EXPORTABLE *wcsdup(const WCHAR *src);
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

#endif

#if defined(_WIN32) || !(HAVE_SCANDIR)
   int LIBNETXMS_EXPORTABLE scandir(const char *dir, struct dirent ***namelist,
               int (*select)(const struct dirent *),
               int (*compar)(const struct dirent **, const struct dirent **));
   int LIBNETXMS_EXPORTABLE alphasort(const struct dirent **a, const struct dirent **b);
#endif

#ifdef UNDER_CE
   int LIBNETXMS_EXPORTABLE _topen(TCHAR *pszName, int nFlags, ...);
   int LIBNETXMS_EXPORTABLE read(int hFile, void *pBuffer, size_t nBytes);
   int LIBNETXMS_EXPORTABLE write(int hFile, void *pBuffer, size_t nBytes);
#endif

BOOL LIBNETXMS_EXPORTABLE nxlog_open(const TCHAR *logName, DWORD flags, const TCHAR *msgModule,
                                     unsigned int msgCount, const TCHAR **messages);
void LIBNETXMS_EXPORTABLE nxlog_close(void);
void LIBNETXMS_EXPORTABLE nxlog_write(DWORD msg, WORD wType, const char *format, ...);
BOOL LIBNETXMS_EXPORTABLE nxlog_set_rotation_policy(int rotationMode, int maxLogSize, int historySize, const TCHAR *dailySuffix);
BOOL LIBNETXMS_EXPORTABLE nxlog_rotate();
void LIBNETXMS_EXPORTABLE nxlog_set_console_writer(void (*writer)(const TCHAR *, ...));

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

#ifdef __cplusplus
}
#endif


//
// C++ only finctions
//

#ifdef __cplusplus

TCHAR LIBNETXMS_EXPORTABLE *EscapeStringForXML(const TCHAR *string, int length);
String LIBNETXMS_EXPORTABLE EscapeStringForXML2(const TCHAR *string, int length = -1);
const char LIBNETXMS_EXPORTABLE *XMLGetAttr(const char **attrs, const char *name);
int LIBNETXMS_EXPORTABLE XMLGetAttrInt(const char **attrs, const char *name, int defVal);
DWORD LIBNETXMS_EXPORTABLE XMLGetAttrDWORD(const char **attrs, const char *name, DWORD defVal);
bool LIBNETXMS_EXPORTABLE XMLGetAttrBoolean(const char **attrs, const char *name, bool defVal);

#if !defined(_WIN32) && !defined(_NETWARE) && defined(NMS_THREADS_H_INCLUDED)
void LIBNETXMS_EXPORTABLE StartMainLoop(THREAD_RESULT (THREAD_CALL * pfSignalHandler)(void *),
                                        THREAD_RESULT (THREAD_CALL * pfMain)(void *));
#endif

void LIBNETXMS_EXPORTABLE InitSubAgentAPI(void (* writeLog)(int, int, const TCHAR *),
                                          void (* sendTrap1)(DWORD, const char *, va_list),
                                          void (* sendTrap2)(DWORD, int, TCHAR **),
                                          BOOL (* sendFile)(void *, DWORD, const TCHAR *, long),
                                          BOOL (* pushData)(const TCHAR *, const TCHAR *));

#endif

#endif   /* _nms_util_h_ */
