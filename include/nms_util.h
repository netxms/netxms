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

#include <nms_common.h>
#include <nms_cscp.h>
#include <nms_threads.h>

#define INVALID_INDEX         0xFFFFFFFF
#define CSCP_TEMP_BUF_SIZE    4096


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

class EXPORTABLE CSCPMessage
{
private:
   WORD m_wCode;
   DWORD m_dwId;
   DWORD m_dwNumVar;    // Number of variables
   CSCP_DF **m_ppVarList;   // List of variables

   void Set(DWORD dwVarId, BYTE bType, void *pValue);
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

   DWORD GetVariableLong(DWORD dwVarId);
   QWORD GetVariableInt64(DWORD dwVarId);
   WORD GetVariableShort(DWORD dwVarId);
   char *GetVariableStr(DWORD dwVarId, char *szBuffer = NULL, DWORD dwBufSize = 0);

   void DeleteAllVariables(void);
};


//
// Queue class
//

class EXPORTABLE Queue
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
   QWORD EXPORTABLE __bswap_64(QWORD qwVal);
   int EXPORTABLE RecvCSCPMessage(SOCKET hSocket, CSCP_MESSAGE *pMsg, CSCP_BUFFER *pBuffer);
   
   int EXPORTABLE BitsInMask(DWORD dwMask);
   char EXPORTABLE *IpToStr(DWORD dwAddr, char *szBuffer);

   void EXPORTABLE *MemAlloc(DWORD dwSize);
   void EXPORTABLE *MemReAlloc(void *pBlock, DWORD dwNewSize);
   void EXPORTABLE MemFree(void *pBlock);
}

#endif   /* _nms_util_h_ */
