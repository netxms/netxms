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

#define INVALID_INDEX         0xFFFFFFFF
#define CSCP_TEMP_BUF_SIZE    4096


//
// Return codes for IcmpPing()
//

#define ICMP_SUCCESS          0
#define ICMP_UNREACHEABLE     1
#define ICMP_TIMEOUT          2
#define ICMP_RAW_SOCK_FAILED  3


//
// getopt() prototype if needed
//

#ifdef _WIN32
#include <getopt.h>
#endif


//
// Temporary buffer structure for RecvCSCPMessage() function
//

typedef struct
{
   DWORD dwBufSize;
   DWORD dwBufPos;
   char szBuffer[CSCP_TEMP_BUF_SIZE];
} CSCP_BUFFER;


//
// Class for holding CSCP messages
//

class LIBNETXMS_EXPORTABLE CSCPMessage
{
private:
   WORD m_wCode;
   DWORD m_dwId;
   DWORD m_dwNumVar;    // Number of variables
   CSCP_DF **m_ppVarList;   // List of variables

   void Set(DWORD dwVarId, BYTE bType, void *pValue, DWORD dwSize = 0);
   void *Get(DWORD dwVarId, BYTE bType);
   DWORD FindVariable(DWORD dwVarId);

public:
   CSCPMessage();
   CSCPMessage(CSCP_MESSAGE *pMsg);
   ~CSCPMessage();

   CSCP_MESSAGE *CreateMessage(void);

   WORD GetCode(void) { return m_wCode; }
   void SetCode(WORD wCode) { m_wCode = wCode; }

   DWORD GetId(void) { return m_dwId; }
   void SetId(DWORD dwId) { m_dwId = dwId; }

   BOOL IsVariableExist(DWORD dwVarId) { return (FindVariable(dwVarId) != INVALID_INDEX) ? TRUE : FALSE; }

   void SetVariable(DWORD dwVarId, WORD wValue) { Set(dwVarId, DT_INT16, &wValue); }
   void SetVariable(DWORD dwVarId, DWORD dwValue) { Set(dwVarId, DT_INTEGER, &dwValue); }
   void SetVariable(DWORD dwVarId, QWORD qwValue) { Set(dwVarId, DT_INT64, &qwValue); }
   void SetVariable(DWORD dwVarId, char *szValue) { Set(dwVarId, DT_STRING, szValue); }
   void SetVariable(DWORD dwVarId, BYTE *pValue, DWORD dwSize) { Set(dwVarId, DT_BINARY, pValue, dwSize); }

   DWORD GetVariableLong(DWORD dwVarId);
   QWORD GetVariableInt64(DWORD dwVarId);
   WORD GetVariableShort(DWORD dwVarId);
   char *GetVariableStr(DWORD dwVarId, char *szBuffer = NULL, DWORD dwBufSize = 0);
   DWORD GetVariableBinary(DWORD dwVarId, BYTE *pBuffer, DWORD dwBufSize);

   void DeleteAllVariables(void);
};


//
// Queue class
//

class LIBNETXMS_EXPORTABLE Queue
{
private:
   MUTEX m_hQueueAccess;
   CONDITION m_hConditionNotEmpty;
   void **m_pElements;
   DWORD m_dwNumElements;
   DWORD m_dwBufferSize;
   DWORD m_dwFirst;
   DWORD m_dwLast;
   DWORD m_dwBufferIncrement;

   void Lock(void) { MutexLock(m_hQueueAccess, INFINITE); }
   void Unlock(void) { MutexUnlock(m_hQueueAccess); }

public:
   Queue(DWORD dwInitialSize = 256, DWORD dwBufferIncrement = 32);
   ~Queue();

   void Put(void *pObject);
   void *Get(void);
   void *GetOrBlock(void);
   DWORD Size(void) { return m_dwNumElements; }
};


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
#ifdef _WIN32
   QWORD LIBNETXMS_EXPORTABLE __bswap_64(QWORD qwVal);
#endif

#ifndef _WIN32
   void LIBNETXMS_EXPORTABLE strupr(char *in);
#endif
   
   int LIBNETXMS_EXPORTABLE RecvCSCPMessage(SOCKET hSocket, CSCP_MESSAGE *pMsg, CSCP_BUFFER *pBuffer);
   CSCP_MESSAGE LIBNETXMS_EXPORTABLE *CreateRawCSCPMessage(WORD wCode, DWORD dwId, DWORD dwDataSize, void *pData, CSCP_MESSAGE *pBuffer);
   
   int LIBNETXMS_EXPORTABLE BitsInMask(DWORD dwMask);
   char LIBNETXMS_EXPORTABLE *IpToStr(DWORD dwAddr, char *szBuffer);

   void LIBNETXMS_EXPORTABLE *MemAlloc(DWORD dwSize);
   void LIBNETXMS_EXPORTABLE *MemReAlloc(void *pBlock, DWORD dwNewSize);
   void LIBNETXMS_EXPORTABLE MemFree(void *pBlock);

   void LIBNETXMS_EXPORTABLE *nx_memdup(const void *pData, DWORD dwSize);
   char LIBNETXMS_EXPORTABLE *nx_strdup(const char *pSrc);

   void LIBNETXMS_EXPORTABLE StrStrip(char *pszStr);
   BOOL LIBNETXMS_EXPORTABLE MatchString(const char *pattern, const char *string, BOOL matchCase);
   
   DWORD LIBNETXMS_EXPORTABLE CalculateCRC32(const unsigned char *data, DWORD nbytes);

   DWORD LIBNETXMS_EXPORTABLE IcmpPing(DWORD dwAddr, int iNumRetries, DWORD dwTimeout, DWORD *pdwRTT);
}

#endif   /* _nms_util_h_ */
