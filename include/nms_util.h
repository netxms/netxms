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
#define CSCP_TEMP_BUF_SIZE    4096
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
// Modes for NxInitSharedData
//

#define MODE_SHM     1
#define MODE_DB      2


//
// getopt() prototype if needed
//

#ifdef _WIN32
#include <getopt.h>
#endif


//
// Functions
//


#if __BYTE_ORDER == __LITTLE_ENDIAN
#define htonq(x) __bswap_64(x)
#define ntohq(x) __bswap_64(x)
#else
#define htonq(x) (x)
#define ntohq(x) (x)
#endif

extern "C"
{
#if defined(_WIN32) || !(HAVE_DECL___BSWAP_64)
   QWORD LIBNETXMS_EXPORTABLE __bswap_64(QWORD qwVal);
#endif

#ifndef _WIN32
   void LIBNETXMS_EXPORTABLE strupr(char *in);
#endif
   
   INT64 LIBNETXMS_EXPORTABLE GetCurrentTimeMs(void);

   int LIBNETXMS_EXPORTABLE BitsInMask(DWORD dwMask);
   char LIBNETXMS_EXPORTABLE *IpToStr(DWORD dwAddr, char *szBuffer);

   void LIBNETXMS_EXPORTABLE *MemAlloc(DWORD dwSize);
   void LIBNETXMS_EXPORTABLE *MemReAlloc(void *pBlock, DWORD dwNewSize);
   void LIBNETXMS_EXPORTABLE MemFree(void *pBlock);

   void LIBNETXMS_EXPORTABLE *nx_memdup(const void *pData, DWORD dwSize);
   char LIBNETXMS_EXPORTABLE *nx_strdup(const char *pSrc);

   void LIBNETXMS_EXPORTABLE StrStrip(char *pszStr);
   BOOL LIBNETXMS_EXPORTABLE MatchString(const char *pattern, const char *string, BOOL matchCase);
   char LIBNETXMS_EXPORTABLE *ExtractWord(char *line, char *buffer);
   
   DWORD LIBNETXMS_EXPORTABLE CalculateCRC32(const unsigned char *data, DWORD nbytes);
   void LIBNETXMS_EXPORTABLE CalculateMD5Hash(const unsigned char *data, int nbytes, unsigned char *hash);
   void LIBNETXMS_EXPORTABLE CalculateSHA1Hash(unsigned char *data, int nbytes, unsigned char *hash);

   DWORD LIBNETXMS_EXPORTABLE IcmpPing(DWORD dwAddr, int iNumRetries, DWORD dwTimeout, DWORD *pdwRTT);
}

#endif   /* _nms_util_h_ */
