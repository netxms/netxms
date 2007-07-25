/* 
** NetXMS - Network Management System
** NXCP API
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
** File: nxcpapi.h
**
**/

#ifndef _nxcpapi_h_
#define _nxcpapi_h_

#include <nms_threads.h>
#include <nms_util.h>

#ifdef _WIN32
#include <wincrypt.h>
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


#ifdef __cplusplus

//
// Class for holding CSCP messages
//

class LIBNETXMS_EXPORTABLE CSCPMessage
{
private:
   WORD m_wCode;
   WORD m_wFlags;
   DWORD m_dwId;
   DWORD m_dwNumVar;    // Number of variables
   CSCP_DF **m_ppVarList;   // List of variables
   int m_nVersion;      // Protocol version

   void *Set(DWORD dwVarId, BYTE bType, void *pValue, DWORD dwSize = 0);
   void *Get(DWORD dwVarId, BYTE bType);
   DWORD FindVariable(DWORD dwVarId);

public:
   CSCPMessage(int nVersion = NXCP_VERSION);
   CSCPMessage(CSCPMessage *pMsg);
   CSCPMessage(CSCP_MESSAGE *pMsg, int nVersion = NXCP_VERSION);
   ~CSCPMessage();

   CSCP_MESSAGE *CreateMessage(void);

   WORD GetCode(void) { return m_wCode; }
   void SetCode(WORD wCode) { m_wCode = wCode; }

   DWORD GetId(void) { return m_dwId; }
   void SetId(DWORD dwId) { m_dwId = dwId; }

   BOOL IsVariableExist(DWORD dwVarId) { return (FindVariable(dwVarId) != INVALID_INDEX) ? TRUE : FALSE; }
   BOOL IsEndOfSequence(void) { return (m_wFlags & MF_END_OF_SEQUENCE) ? TRUE : FALSE; }
   BOOL IsReverseOrder(void) { return (m_wFlags & MF_REVERSE_ORDER) ? TRUE : FALSE; }

   void SetVariable(DWORD dwVarId, WORD wValue) { Set(dwVarId, CSCP_DT_INT16, &wValue); }
   void SetVariable(DWORD dwVarId, DWORD dwValue) { Set(dwVarId, CSCP_DT_INTEGER, &dwValue); }
   void SetVariable(DWORD dwVarId, QWORD qwValue) { Set(dwVarId, CSCP_DT_INT64, &qwValue); }
   void SetVariable(DWORD dwVarId, double dValue) { Set(dwVarId, CSCP_DT_FLOAT, &dValue); }
   void SetVariable(DWORD dwVarId, TCHAR *pszValue) { Set(dwVarId, CSCP_DT_STRING, pszValue); }
   void SetVariable(DWORD dwVarId, BYTE *pValue, DWORD dwSize) { Set(dwVarId, CSCP_DT_BINARY, pValue, dwSize); }
   void SetVariableToInt32Array(DWORD dwVarId, DWORD dwNumElements, DWORD *pdwData);
   BOOL SetVariableFromFile(DWORD dwVarId, const TCHAR *pszFileName);

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
   void SetReverseOrderFlag(void) { m_wFlags |= MF_REVERSE_ORDER; }
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

class LIBNETXMS_EXPORTABLE MsgWaitQueue
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

#else    /* __cplusplus */

typedef void CSCPMessage;

#endif


//
// Functions
//

#ifdef __cplusplus
extern "C" {
#endif

int LIBNETXMS_EXPORTABLE RecvNXCPMessage(SOCKET hSocket, CSCP_MESSAGE *pMsg,
                                         CSCP_BUFFER *pBuffer, DWORD dwMaxMsgSize,
                                         CSCP_ENCRYPTION_CONTEXT **ppCtx,
                                         BYTE *pDecryptionBuffer, DWORD dwTimeout);
CSCP_MESSAGE LIBNETXMS_EXPORTABLE *CreateRawNXCPMessage(WORD wCode, DWORD dwId, WORD wFlags,
                                                        DWORD dwDataSize, void *pData,
                                                        CSCP_MESSAGE *pBuffer);
TCHAR LIBNETXMS_EXPORTABLE *NXCPMessageCodeName(WORD wCode, TCHAR *pszBuffer);
BOOL LIBNETXMS_EXPORTABLE SendFileOverNXCP(SOCKET hSocket, DWORD dwId, TCHAR *pszFile,
                                           CSCP_ENCRYPTION_CONTEXT *pCtx);
BOOL LIBNETXMS_EXPORTABLE NXCPGetPeerProtocolVersion(SOCKET hSocket, int *pnVersion);
   
BOOL LIBNETXMS_EXPORTABLE InitCryptoLib(DWORD dwEnabledCiphers);
DWORD LIBNETXMS_EXPORTABLE CSCPGetSupportedCiphers(void);
CSCP_ENCRYPTED_MESSAGE LIBNETXMS_EXPORTABLE 
   *CSCPEncryptMessage(CSCP_ENCRYPTION_CONTEXT *pCtx, CSCP_MESSAGE *pMsg);
BOOL LIBNETXMS_EXPORTABLE CSCPDecryptMessage(CSCP_ENCRYPTION_CONTEXT *pCtx,
                                             CSCP_ENCRYPTED_MESSAGE *pMsg,
                                             BYTE *pDecryptionBuffer);
DWORD LIBNETXMS_EXPORTABLE SetupEncryptionContext(CSCPMessage *pMsg, 
                                                  CSCP_ENCRYPTION_CONTEXT **ppCtx,
                                                  CSCPMessage **ppResponse,
                                                  RSA *pPrivateKey, int nNXCPVersion);
void LIBNETXMS_EXPORTABLE DestroyEncryptionContext(CSCP_ENCRYPTION_CONTEXT *pCtx);
void LIBNETXMS_EXPORTABLE PrepareKeyRequestMsg(CSCPMessage *pMsg, RSA *pServerKey);
RSA LIBNETXMS_EXPORTABLE *LoadRSAKeys(const TCHAR *pszKeyFile);

#ifdef _WIN32
BOOL LIBNETXMS_EXPORTABLE SignMessageWithCAPI(BYTE *pMsg, DWORD dwMsgLen, const CERT_CONTEXT *pCert,
												          BYTE *pBuffer, DWORD dwBufSize, DWORD *pdwSigLen);
#endif

#ifdef __cplusplus
}
#endif

#endif   /* _nxcpapi_h_ */
