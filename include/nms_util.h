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
	int Read(char *pBuff, int nSize);
	bool Write(char *pBuff, int nSize);
	void Flush(void);
	bool Set(int nSpeed, int nDataBits, int nParity, int nStopBits);
	bool Restart(void);

private:
	TCHAR *m_pszPort;
	int m_nTimeout;
	int m_nSpeed;
	int m_nDataBits;
	int m_nStopBits;
	int m_nParity;

#ifndef _WIN32
	int m_hPort;
#else
	HANDLE m_hPort;
#endif
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
#define htonq(x) __bswap_64(x)
#define ntohq(x) __bswap_64(x)
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
   
   INT64 LIBNETXMS_EXPORTABLE GetCurrentTimeMs(void);
   QWORD LIBNETXMS_EXPORTABLE FileSize(TCHAR *pszFileName);

   int LIBNETXMS_EXPORTABLE BitsInMask(DWORD dwMask);
   TCHAR LIBNETXMS_EXPORTABLE *IpToStr(DWORD dwAddr, TCHAR *szBuffer);
   DWORD LIBNETXMS_EXPORTABLE ResolveHostName(TCHAR *pszName);

   void LIBNETXMS_EXPORTABLE *nx_memdup(const void *pData, DWORD dwSize);
   void LIBNETXMS_EXPORTABLE nx_memswap(void *pBlock1, void *pBlock2, DWORD dwSize);

   void LIBNETXMS_EXPORTABLE BinToStr(BYTE *pData, DWORD dwSize, TCHAR *pStr);
   DWORD LIBNETXMS_EXPORTABLE StrToBin(TCHAR *pStr, BYTE *pData, DWORD dwSize);

   void LIBNETXMS_EXPORTABLE StrStrip(TCHAR *pszStr);
   BOOL LIBNETXMS_EXPORTABLE MatchString(const TCHAR *pattern, const TCHAR *string, BOOL matchCase);
   TCHAR LIBNETXMS_EXPORTABLE *ExtractWord(TCHAR *line, TCHAR *buffer);
   BOOL LIBNETXMS_EXPORTABLE IsValidObjectName(const TCHAR *pszName);
   BOOL LIBNETXMS_EXPORTABLE IsValidScriptName(const TCHAR *pszName);
   void LIBNETXMS_EXPORTABLE TranslateStr(TCHAR *pszString, TCHAR *pszSubStr, TCHAR *pszReplace);
   TCHAR LIBNETXMS_EXPORTABLE *GetCleanFileName(TCHAR *pszFileName);
   void LIBNETXMS_EXPORTABLE GetOSVersionString(TCHAR *pszBuffer);
   
   DWORD LIBNETXMS_EXPORTABLE CalculateCRC32(const unsigned char *pData, DWORD dwSize, DWORD dwCRC);
   void LIBNETXMS_EXPORTABLE CalculateMD5Hash(const unsigned char *data, int nbytes, unsigned char *hash);
   void LIBNETXMS_EXPORTABLE CalculateSHA1Hash(unsigned char *data, int nbytes, unsigned char *hash);
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

#ifdef _WIN32
   TCHAR LIBNETXMS_EXPORTABLE *GetSystemErrorText(DWORD dwError, TCHAR *pszBuffer, int iBufSize);
#endif

#if !(HAVE_DAEMON)
   int LIBNETXMS_EXPORTABLE daemon(int nochdir, int noclose);
#endif

   DWORD LIBNETXMS_EXPORTABLE inet_addr_w(WCHAR *pszAddr);

#ifndef _WIN32
   int LIBNETXMS_EXPORTABLE WideCharToMultiByte(int iCodePage, DWORD dwFlags, WCHAR *pWideCharStr, 
                                                int cchWideChar, char *pByteStr, int cchByteChar, 
                                                char *pDefaultChar, BOOL *pbUsedDefChar);
   int LIBNETXMS_EXPORTABLE MultiByteToWideChar(int iCodePage, DWORD dwFlags, char *pByteStr, 
                                                int cchByteChar, WCHAR *pWideCharStr, 
                                                int cchWideChar);
#endif

#if !(HAVE_STRTOLL)
   INT64 LIBNETXMS_EXPORTABLE strtoll(const char *nptr, char **endptr, int base);
#endif
#if !(HAVE_STRTOULL)
   QWORD LIBNETXMS_EXPORTABLE strtoull(const char *nptr, char **endptr, int base);
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
