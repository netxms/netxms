/* 
** NetXMS - Network Management System
** CSCP API Library
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: nxcscpapi.h
**
**/

#ifndef _nxcscpapi_h_
#define _nxcscpapi_h_

#include <nms_threads.h>


#ifdef _WIN32
#ifdef LIBNXCSCP_EXPORTS
#define LIBNXCSCP_EXPORTABLE __declspec(dllexport)
#else
#define LIBNXCSCP_EXPORTABLE __declspec(dllimport)
#endif
#else    /* _WIN32 */
#define LIBNXCSCP_EXPORTABLE
#endif


//
// Encryption methods
//

#define CSCP_ENCRYPTION_NONE           0
#define CSCP_ENCRYPTION_BLOWFISH_SHA1  1


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

class LIBNXCSCP_EXPORTABLE CSCPMessage
{
private:
   WORD m_wCode;
   WORD m_wFlags;
   DWORD m_dwId;
   DWORD m_dwNumVar;    // Number of variables
   CSCP_DF **m_ppVarList;   // List of variables

   void *Set(DWORD dwVarId, BYTE bType, void *pValue, DWORD dwSize = 0);
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
   BOOL IsEndOfSequence(void) { return (m_wFlags & MF_END_OF_SEQUENCE) ? TRUE : FALSE; }

   void SetVariable(DWORD dwVarId, WORD wValue) { Set(dwVarId, CSCP_DT_INT16, &wValue); }
   void SetVariable(DWORD dwVarId, DWORD dwValue) { Set(dwVarId, CSCP_DT_INTEGER, &dwValue); }
   void SetVariable(DWORD dwVarId, QWORD qwValue) { Set(dwVarId, CSCP_DT_INT64, &qwValue); }
   void SetVariable(DWORD dwVarId, double dValue) { Set(dwVarId, CSCP_DT_FLOAT, &dValue); }
   void SetVariable(DWORD dwVarId, TCHAR *pszValue) { Set(dwVarId, CSCP_DT_STRING, pszValue); }
   void SetVariable(DWORD dwVarId, BYTE *pValue, DWORD dwSize) { Set(dwVarId, CSCP_DT_BINARY, pValue, dwSize); }
   void SetVariableToInt32Array(DWORD dwVarId, DWORD dwNumElements, DWORD *pdwData);
   BOOL SetVariableFromFile(DWORD dwVarId, TCHAR *pszFileName);

   DWORD GetVariableLong(DWORD dwVarId);
   QWORD GetVariableInt64(DWORD dwVarId);
   WORD GetVariableShort(DWORD dwVarId);
   LONG GetVariableShortAsInt32(DWORD dwVarId);
   double GetVariableDouble(DWORD dwVarId);
   TCHAR *GetVariableStr(DWORD dwVarId, TCHAR *szBuffer = NULL, DWORD dwBufSize = 0);
   DWORD GetVariableBinary(DWORD dwVarId, BYTE *pBuffer, DWORD dwBufSize);
   DWORD GetVariableInt32Array(DWORD dwVarId, DWORD dwNumElements, DWORD *pdwBuffer);

   void DeleteAllVariables(void);

   void DisableEncryption(void) { m_wFlags |= MF_DONT_ENCRYPT; }
   void SetEndOfSequence(void) { m_wFlags |= MF_END_OF_SEQUENCE; }
};


//
// Message waiting queue element structure
//

typedef struct
{
   WORD wCode;       // Message code
   WORD wIsBinary;   // 1 for binary (raw) messages
   DWORD dwId;       // Message ID
   DWORD dwTTL;      // Message time-to-live in milliseconds
   void *pMsg;       // Pointer to message, either to CSCPMessage object or raw message
} WAIT_QUEUE_ELEMENT;


//
// Message waiting queue class
//

class LIBNXCSCP_EXPORTABLE MsgWaitQueue
{
private:
   MUTEX m_mutexDataAccess;
   CONDITION m_condStop;
   CONDITION m_condNewMsg;
   DWORD m_dwMsgHoldTime;
   DWORD m_dwNumElements;
   WAIT_QUEUE_ELEMENT *m_pElements;
   THREAD m_hHkThread;

   void Lock(void) { MutexLock(m_mutexDataAccess, INFINITE); }
   void Unlock(void) { MutexUnlock(m_mutexDataAccess); }
   void HousekeeperThread(void);
   void *WaitForMessageInternal(WORD wIsBinary, WORD wCode, DWORD dwId, DWORD dwTimeOut);
   
   static THREAD_RESULT THREAD_CALL MWQThreadStarter(void *);

public:
   MsgWaitQueue();
   ~MsgWaitQueue();

   void Put(CSCPMessage *pMsg);
   void Put(CSCP_MESSAGE *pMsg);
   CSCPMessage *WaitForMessage(WORD wCode, DWORD dwId, DWORD dwTimeOut)
   {
      return (CSCPMessage *)WaitForMessageInternal(0, wCode, dwId, dwTimeOut);
   }
   CSCP_MESSAGE *WaitForRawMessage(WORD wCode, DWORD dwId, DWORD dwTimeOut)
   {
      return (CSCP_MESSAGE *)WaitForMessageInternal(1, wCode, dwId, dwTimeOut);
   }
   
   void Clear(void);
   void SetHoldTime(DWORD dwHoldTime) { m_dwMsgHoldTime = dwHoldTime; }
};


//
// Functions
//

#ifdef __cplusplus
extern "C" {
#endif

int LIBNXCSCP_EXPORTABLE RecvCSCPMessage(SOCKET hSocket, CSCP_MESSAGE *pMsg,
                                         CSCP_BUFFER *pBuffer, DWORD dwMaxMsgSize,
                                         CSCP_ENCRYPTION_CONTEXT **ppCtx,
                                         BYTE *pDecryptionBuffer, DWORD dwTimeout);
CSCP_MESSAGE LIBNXCSCP_EXPORTABLE *CreateRawCSCPMessage(WORD wCode, DWORD dwId,
                                                        DWORD dwDataSize, void *pData,
                                                        CSCP_MESSAGE *pBuffer);
TCHAR LIBNXCSCP_EXPORTABLE *CSCPMessageCodeName(WORD wCode, TCHAR *pszBuffer);
BOOL LIBNXCSCP_EXPORTABLE SendFileOverCSCP(SOCKET hSocket, DWORD dwId, TCHAR *pszFile,
                                           CSCP_ENCRYPTION_CONTEXT *pCtx);
   
BOOL LIBNXCSCP_EXPORTABLE InitCryptoLib(DWORD dwEnabledCiphers);
DWORD LIBNXCSCP_EXPORTABLE CSCPGetSupportedCiphers(void);
CSCP_ENCRYPTED_MESSAGE LIBNXCSCP_EXPORTABLE 
   *CSCPEncryptMessage(CSCP_ENCRYPTION_CONTEXT *pCtx, CSCP_MESSAGE *pMsg);
BOOL LIBNXCSCP_EXPORTABLE CSCPDecryptMessage(CSCP_ENCRYPTION_CONTEXT *pCtx,
                                             CSCP_ENCRYPTED_MESSAGE *pMsg,
                                             BYTE *pDecryptionBuffer);
DWORD LIBNXCSCP_EXPORTABLE SetupEncryptionContext(CSCPMessage *pMsg, 
                                                  CSCP_ENCRYPTION_CONTEXT **ppCtx,
                                                  CSCPMessage **ppResponse,
                                                  RSA *pPrivateKey);
void LIBNXCSCP_EXPORTABLE DestroyEncryptionContext(CSCP_ENCRYPTION_CONTEXT *pCtx);
void LIBNXCSCP_EXPORTABLE PrepareKeyRequestMsg(CSCPMessage *pMsg, RSA *pServerKey);
RSA LIBNXCSCP_EXPORTABLE *LoadRSAKeys(TCHAR *pszKeyFile);

#ifdef __cplusplus
}
#endif

#endif   /* _nxcscpapi_h_ */
