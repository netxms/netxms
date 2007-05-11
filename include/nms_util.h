/* $Id: nms_util.h,v 1.104 2007-05-11 15:44:46 victor Exp $ */

/* 
** NetXMS - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
** $module: nms_util.h
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


//
// Sirial communications
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
// Common constants
//

#define INVALID_INDEX         0xFFFFFFFF
#define CSCP_TEMP_BUF_SIZE    65536
#define MD5_DIGEST_SIZE       16
#define SHA1_DIGEST_SIZE      20


//
// Return codes for IcmpPing()
//

#define ICMP_SUCCESS          0
#define ICMP_UNREACHEABLE     1
#define ICMP_TIMEOUT          2
#define ICMP_RAW_SOCK_FAILED  3


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

	bool Open(TCHAR *pszPort);
	void Close(void);
	void SetTimeout(int nTimeout);
	int Read(char *pBuff, int nSize); /* waits up to timeout and do single read */
	int ReadAll(char *pBuff, int nSize); /* read until timeout or out of space */
	bool Write(char *pBuff, int nSize);
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

class LIBNETXMS_EXPORTABLE Table
{
private:
   int m_nNumRows;
   int m_nNumCols;
   TCHAR **m_ppData;
   TCHAR **m_ppColNames;

public:
   Table();
   ~Table();

   int GetNumRows(void) { return m_nNumRows; }
   int GetNumColumns(void) { return m_nNumCols; }

   void AddColumn(TCHAR *pszName);
   void AddRow(void);

   void SetAt(int nRow, int nCol, TCHAR *pszData);
   void SetAt(int nRow, int nCol, LONG nData);
   void SetAt(int nRow, int nCol, DWORD dwData);
   void SetAt(int nRow, int nCol, double dData);
   void SetAt(int nRow, int nCol, INT64 nData);
   void SetAt(int nRow, int nCol, QWORD qwData);

   void Set(int nCol, TCHAR *pszData) { SetAt(m_nNumRows - 1, nCol, pszData); }
   void Set(int nCol, LONG nData) { SetAt(m_nNumRows - 1, nCol, nData); }
   void Set(int nCol, DWORD dwData) { SetAt(m_nNumRows - 1, nCol, dwData); }
   void Set(int nCol, double dData) { SetAt(m_nNumRows - 1, nCol, dData); }
   void Set(int nCol, INT64 nData) { SetAt(m_nNumRows - 1, nCol, nData); }
   void Set(int nCol, QWORD qwData) { SetAt(m_nNumRows - 1, nCol, qwData); }

   TCHAR *GetAsString(int nRow, int nCol);
   LONG GetAsInt(int nRow, int nCol);
   DWORD GetAsUInt(int nRow, int nCol);
   INT64 GetAsInt64(int nRow, int nCol);
   QWORD GetAsUInt64(int nRow, int nCol);
   double GetAsDouble(int nRow, int nCol);
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
   ~String();

   void SetBuffer(TCHAR *pszBuffer);

   const String& operator =(TCHAR *pszStr);
   const String&  operator +=(TCHAR *pszStr);
   operator TCHAR*() { return CHECK_NULL_EX(m_pszBuffer); }

	void AddString(TCHAR *pStr, DWORD dwLen);
	void AddDynamicString(TCHAR *pszStr) { if (pszStr != NULL) { *this += pszStr; free(pszStr); } }

   void AddFormattedString(TCHAR *pszFormat, ...);
   void EscapeCharacter(int ch, int esc);
   void Translate(TCHAR *pszSrc, TCHAR *pszDst);

	DWORD Size(void) { return m_dwBufSize > 0 ? m_dwBufSize - 1 : 0; }

	TCHAR *SubStr(int nStart, int nLen, TCHAR *pszBuffer);
	TCHAR *SubStr(int nStart, int nLen) { return SubStr(nStart, nLen, NULL); }
	int Find(TCHAR *pszStr, int nStart = 0);
};


//
// String map class
//

class LIBNETXMS_EXPORTABLE StringMap
{
protected:
	DWORD m_dwSize;
	TCHAR **m_ppszKeys;
	TCHAR **m_ppszValues;

	DWORD Find(TCHAR *pszKey);

public:
	StringMap();
	~StringMap();

	void Set(TCHAR *pszKey, TCHAR *pszValue);
	TCHAR *Get(TCHAR *pszKey);
	void Clear(void);

	DWORD Size(void) { return m_dwSize; }
	TCHAR *GetKeyByIndex(DWORD idx) { return (idx < m_dwSize) ? m_ppszKeys[idx] : NULL; }
	TCHAR *GetValueByIndex(DWORD idx) { return (idx < m_dwSize) ? m_ppszValues[idx] : NULL; }
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
// getopt() prototype if needed
//

#ifdef _WIN32
#include <getopt.h>
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

#ifndef SWIGPERL

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
   inline TCHAR *nx_strncpy(TCHAR *pszDest, const TCHAR *pszSrc, int nLen)
   {
      _tcsncpy(pszDest, pszSrc, nLen - 1);
      pszDest[nLen - 1] = 0;
      return pszDest;
   }
#endif
#else
   TCHAR LIBNETXMS_EXPORTABLE *nx_strncpy(TCHAR *pszDest, TCHAR *pszSrc, int nLen);
#endif

#ifdef __cplusplus
extern "C"
{
#endif
	int LIBNETXMS_EXPORTABLE SendEx(SOCKET, const void *, size_t, int);
   int LIBNETXMS_EXPORTABLE RecvEx(SOCKET nSocket, const void *pBuff,
                                   size_t nSize, int nFlags, DWORD dwTimeout);

#if defined(_WIN32) || !(HAVE_DECL___BSWAP_32)
   DWORD LIBNETXMS_EXPORTABLE __bswap_32(DWORD dwVal);
#endif
#if defined(_WIN32) || !(HAVE_DECL___BSWAP_64)
   QWORD LIBNETXMS_EXPORTABLE __bswap_64(QWORD qwVal);
#endif
   double LIBNETXMS_EXPORTABLE __bswap_double(double dVal);
   void LIBNETXMS_EXPORTABLE __bswap_wstr(WCHAR *pStr);

#if !defined(_WIN32) && !defined(_NETWARE)
   void LIBNETXMS_EXPORTABLE strupr(TCHAR *in);
#endif
   
	void LIBNETXMS_EXPORTABLE QSortEx(void *base, size_t nmemb, size_t size, void *arg,
												 int (*compare)(const void *, const void *, void *));

   INT64 LIBNETXMS_EXPORTABLE GetCurrentTimeMs(void);
   QWORD LIBNETXMS_EXPORTABLE FileSize(TCHAR *pszFileName);

   int LIBNETXMS_EXPORTABLE BitsInMask(DWORD dwMask);
   TCHAR LIBNETXMS_EXPORTABLE *IpToStr(DWORD dwAddr, TCHAR *szBuffer);
   DWORD LIBNETXMS_EXPORTABLE ResolveHostName(TCHAR *pszName);

   void LIBNETXMS_EXPORTABLE *nx_memdup(const void *pData, DWORD dwSize);
   void LIBNETXMS_EXPORTABLE nx_memswap(void *pBlock1, void *pBlock2, DWORD dwSize);

   TCHAR LIBNETXMS_EXPORTABLE *BinToStr(BYTE *pData, DWORD dwSize, TCHAR *pStr);
   DWORD LIBNETXMS_EXPORTABLE StrToBin(TCHAR *pStr, BYTE *pData, DWORD dwSize);
   void LIBNETXMS_EXPORTABLE MACToStr(BYTE *pData, TCHAR *pStr);

   void LIBNETXMS_EXPORTABLE StrStrip(TCHAR *pszStr);
   BOOL LIBNETXMS_EXPORTABLE MatchString(const TCHAR *pattern, const TCHAR *string, BOOL matchCase);
	BOOL LIBNETXMS_EXPORTABLE RegexpMatch(TCHAR *pszStr, TCHAR *pszExpr, BOOL bMatchCase);
   TCHAR LIBNETXMS_EXPORTABLE *ExtractWord(TCHAR *line, TCHAR *buffer);
   int LIBNETXMS_EXPORTABLE NumChars(TCHAR *pszStr, int ch);
   BOOL LIBNETXMS_EXPORTABLE IsValidObjectName(const TCHAR *pszName);
   BOOL LIBNETXMS_EXPORTABLE IsValidScriptName(const TCHAR *pszName);
   void LIBNETXMS_EXPORTABLE TranslateStr(TCHAR *pszString, TCHAR *pszSubStr, TCHAR *pszReplace);
   TCHAR LIBNETXMS_EXPORTABLE *GetCleanFileName(TCHAR *pszFileName);
   void LIBNETXMS_EXPORTABLE GetOSVersionString(TCHAR *pszBuffer);
	BYTE LIBNETXMS_EXPORTABLE *LoadFile(TCHAR *pszFileName, DWORD *pdwFileSize);
 
   DWORD LIBNETXMS_EXPORTABLE CalculateCRC32(const unsigned char *pData, DWORD dwSize, DWORD dwCRC);
   void LIBNETXMS_EXPORTABLE CalculateMD5Hash(const unsigned char *data, size_t nbytes, unsigned char *hash);
   void LIBNETXMS_EXPORTABLE CalculateSHA1Hash(unsigned char *data, size_t nbytes, unsigned char *hash);
   BOOL LIBNETXMS_EXPORTABLE CalculateFileMD5Hash(TCHAR *pszFileName, BYTE *pHash);
   BOOL LIBNETXMS_EXPORTABLE CalculateFileSHA1Hash(TCHAR *pszFileName, BYTE *pHash);
   BOOL LIBNETXMS_EXPORTABLE CalculateFileCRC32(TCHAR *pszFileName, DWORD *pResult);

   DWORD LIBNETXMS_EXPORTABLE IcmpPing(DWORD dwAddr, int iNumRetries, DWORD dwTimeout,
                                       DWORD *pdwRTT, DWORD dwPacketSize);

   DWORD LIBNETXMS_EXPORTABLE NxLoadConfig(TCHAR *pszFileName, TCHAR *pszSection, 
                                           NX_CFG_TEMPLATE *pTemplateList, BOOL bPrint);
   int LIBNETXMS_EXPORTABLE NxDCIDataTypeFromText(TCHAR *pszText);

   HMODULE LIBNETXMS_EXPORTABLE DLOpen(TCHAR *szLibName, TCHAR *pszErrorText);
   void LIBNETXMS_EXPORTABLE DLClose(HMODULE hModule);
   void LIBNETXMS_EXPORTABLE *DLGetSymbolAddr(HMODULE hModule, TCHAR *szSymbol, TCHAR *pszErrorText);

   void LIBNETXMS_EXPORTABLE InitSubAgentsLogger(void (* pFunc)(int, TCHAR *));
   void LIBNETXMS_EXPORTABLE InitSubAgentsTrapSender(void (* pFunc1)(DWORD, int, TCHAR **),
                                                     void (* pFunc2)(DWORD, char *, va_list));

#ifdef _WIN32
   TCHAR LIBNETXMS_EXPORTABLE *GetSystemErrorText(DWORD dwError, TCHAR *pszBuffer, int iBufSize);
#endif

#if !(HAVE_DAEMON)
   int LIBNETXMS_EXPORTABLE daemon(int nochdir, int noclose);
#endif

   DWORD LIBNETXMS_EXPORTABLE inet_addr_w(WCHAR *pszAddr);

#ifndef _WIN32
	void LIBNETXMS_EXPORTABLE SetDefaultCodepage(char *cp);
   int LIBNETXMS_EXPORTABLE WideCharToMultiByte(int iCodePage, DWORD dwFlags, WCHAR *pWideCharStr, 
                                                int cchWideChar, char *pByteStr, int cchByteChar, 
                                                char *pDefaultChar, BOOL *pbUsedDefChar);
   int LIBNETXMS_EXPORTABLE MultiByteToWideChar(int iCodePage, DWORD dwFlags, char *pByteStr, 
                                                int cchByteChar, WCHAR *pWideCharStr, 
                                                int cchWideChar);
#if !HAVE_USEABLE_WCHAR
	int LIBNETXMS_EXPORTABLE nx_wcslen(const WCHAR *pStr);
	WCHAR LIBNETXMS_EXPORTABLE *nx_wcsncpy(WCHAR *pDst, WCHAR *pSrc, int nDstLen);
	WCHAR LIBNETXMS_EXPORTABLE *nx_wcsdup(const WCHAR *pStr);
#endif
#endif	/* _WIN32 */
   WCHAR LIBNETXMS_EXPORTABLE *WideStringFromMBString(char *pszString);
   char LIBNETXMS_EXPORTABLE *MBStringFromWideString(WCHAR *pwszString);
   char LIBNETXMS_EXPORTABLE *UTF8StringFromWideString(WCHAR *pwszString);

#ifdef _WITH_ENCRYPTION
	WCHAR LIBNETXMS_EXPORTABLE *ERR_error_string_W(int nError, WCHAR *pwszBuffer);
#endif

#ifdef UNICODE
INT64 LIBNETXMS_EXPORTABLE wcstoll(const WCHAR *nptr, WCHAR **endptr, int base);
QWORD LIBNETXMS_EXPORTABLE wcstoull(const WCHAR *nptr, WCHAR **endptr, int base);
#else
#if !(HAVE_STRTOLL)
   INT64 LIBNETXMS_EXPORTABLE strtoll(const char *nptr, char **endptr, int base);
#endif
#if !(HAVE_STRTOULL)
   QWORD LIBNETXMS_EXPORTABLE strtoull(const char *nptr, char **endptr, int base);
#endif
#endif

#ifdef _WIN32
#ifndef SWIGPERL
    DIR LIBNETXMS_EXPORTABLE *opendir(const char *filename);
    struct dirent LIBNETXMS_EXPORTABLE *readdir(DIR *dirp);
    int LIBNETXMS_EXPORTABLE closedir(DIR *dirp);
#endif
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

#if !defined(_WIN32) && !defined(_NETWARE) && defined(NMS_THREADS_H_INCLUDED)
void LIBNETXMS_EXPORTABLE StartMainLoop(THREAD_RESULT (THREAD_CALL * pfSignalHandler)(void *),
                                        THREAD_RESULT (THREAD_CALL * pfMain)(void *));
#endif

#ifdef __cplusplus
}
#endif

#endif   /* _nms_util_h_ */

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.103  2007/05/07 11:35:41  victor
- Unfinished changes in alarm generation code
- nxhttpd redesigned

Revision 1.102  2007/03/23 15:59:04  victor
- All certificates now stored in database
- CA certificates can be imported from management console
- Fixed problems with some "invisible" system rights
- Minor UI fixes

Revision 1.101  2007/02/09 22:38:07  victor
Crash dump generator added to console

Revision 1.100  2007/01/24 00:52:28  alk
Serial::ReadAll() added: read all data until timeout. returns data size, -1 it timeout w/o any data

Revision 1.99  2007/01/11 18:56:14  victor
- Added Process.COuntEx parameter
- Added filtering by command line and window title to process.XXX parameters

Revision 1.98  2006/12/17 10:31:37  victor
ODBC driver made compatible with latest unixODBC

Revision 1.97  2006/12/11 21:19:27  victor
Management pack creation almost working (only SNMP traps cannot be exported)

Revision 1.96  2006/11/03 08:58:56  victor
- Added utillity class "Table"
- Changed defines for iconv() usage on NetWare
- Semi-complete "alarm details" view

Revision 1.95  2006/10/17 21:20:25  victor
Finished Implementation of agent traps and added trap sending API for subagents

Revision 1.94  2006/10/05 05:24:05  victor
Minor changes

Revision 1.93  2006/10/01 20:43:37  victor
UNICODE-related fixes

Revision 1.92  2006/10/01 16:01:09  victor
Yet another Linux UNICODE fix

Revision 1.91  2006/10/01 15:47:39  victor
More UNICODE fixes

Revision 1.90  2006/10/01 15:26:28  victor
Some UNICODE-related fixes

Revision 1.89  2006/09/30 22:41:08  victor
Database driver API changed: now all SQL queries and results passed as UNICODE strings

Revision 1.88  2006/09/27 13:04:45  victor
Preparation for process information parameters

Revision 1.87  2006/09/27 04:15:25  victor
AIX: implemented Net.InterfaceList

Revision 1.86  2006/09/23 23:49:41  victor
Console working with UNICODE!

Revision 1.85  2006/09/10 06:59:36  victor
Fixed problmes with Win32 build

Revision 1.84  2006/09/07 22:02:06  alk
UNIX version of Serial rewritten
termio removed from configure (depricated in favour of termio_s_?)


*/
