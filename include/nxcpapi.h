/* 
** NetXMS - Network Management System
** NXCP API
** Copyright (C) 2003-2010 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
   UINT32 dwBufSize;
   UINT32 dwBufPos;
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
   UINT32 m_dwId;
   UINT32 m_dwNumVar;    // Number of variables
   CSCP_DF **m_ppVarList;   // List of variables
   int m_nVersion;      // Protocol version

   void *Set(UINT32 dwVarId, BYTE bType, const void *pValue, UINT32 dwSize = 0);
   void *Get(UINT32 dwVarId, BYTE bType);
   UINT32 FindVariable(UINT32 dwVarId);

public:
   CSCPMessage(int nVersion = NXCP_VERSION);
   CSCPMessage(CSCPMessage *pMsg);
   CSCPMessage(CSCP_MESSAGE *pMsg, int nVersion = NXCP_VERSION);
   CSCPMessage(const char *xml);
   ~CSCPMessage();

   CSCP_MESSAGE *CreateMessage(void);
	char *CreateXML(void);
	void ProcessXMLToken(void *state, const char **attrs);
	void ProcessXMLData(void *state);

   WORD GetCode(void) { return m_wCode; }
   void SetCode(WORD wCode) { m_wCode = wCode; }

   UINT32 GetId(void) { return m_dwId; }
   void SetId(UINT32 dwId) { m_dwId = dwId; }

   BOOL IsVariableExist(UINT32 dwVarId) { return (FindVariable(dwVarId) != INVALID_INDEX) ? TRUE : FALSE; }
   BOOL IsEndOfSequence(void) { return (m_wFlags & MF_END_OF_SEQUENCE) ? TRUE : FALSE; }
   BOOL IsReverseOrder(void) { return (m_wFlags & MF_REVERSE_ORDER) ? TRUE : FALSE; }

   void SetVariable(UINT32 dwVarId, INT16 wValue) { Set(dwVarId, CSCP_DT_INT16, &wValue); }
   void SetVariable(UINT32 dwVarId, UINT16 wValue) { Set(dwVarId, CSCP_DT_INT16, &wValue); }
   void SetVariable(UINT32 dwVarId, INT32 dwValue) { Set(dwVarId, CSCP_DT_INTEGER, &dwValue); }
   void SetVariable(UINT32 dwVarId, UINT32 dwValue) { Set(dwVarId, CSCP_DT_INTEGER, &dwValue); }
   void SetVariable(UINT32 dwVarId, INT64 qwValue) { Set(dwVarId, CSCP_DT_INT64, &qwValue); }
   void SetVariable(UINT32 dwVarId, UINT64 qwValue) { Set(dwVarId, CSCP_DT_INT64, &qwValue); }
   void SetVariable(UINT32 dwVarId, double dValue) { Set(dwVarId, CSCP_DT_FLOAT, &dValue); }
   void SetVariable(UINT32 dwVarId, const TCHAR *pszValue) { Set(dwVarId, CSCP_DT_STRING, pszValue); }
   void SetVariable(UINT32 dwVarId, const TCHAR *pszValue, UINT32 maxLen) { Set(dwVarId, CSCP_DT_STRING, pszValue, maxLen); }
   void SetVariable(UINT32 dwVarId, BYTE *pValue, UINT32 dwSize) { Set(dwVarId, CSCP_DT_BINARY, pValue, dwSize); }
#ifdef UNICODE
   void SetVariableFromMBString(UINT32 dwVarId, const char *pszValue);
#else
   void SetVariableFromMBString(UINT32 dwVarId, const char *pszValue) { Set(dwVarId, CSCP_DT_STRING, pszValue); }
#endif
   void SetVariableToInt32Array(UINT32 dwVarId, UINT32 dwNumElements, const UINT32 *pdwData);
   BOOL SetVariableFromFile(UINT32 dwVarId, const TCHAR *pszFileName);

   UINT32 GetVariableLong(UINT32 dwVarId);
   UINT64 GetVariableInt64(UINT32 dwVarId);
   UINT16 GetVariableShort(UINT32 dwVarId);
   INT32 GetVariableShortAsInt32(UINT32 dwVarId);
   double GetVariableDouble(UINT32 dwVarId);
   TCHAR *GetVariableStr(UINT32 dwVarId, TCHAR *szBuffer = NULL, UINT32 dwBufSize = 0);
	char *GetVariableStrA(UINT32 dwVarId, char *pszBuffer = NULL, UINT32 dwBufSize = 0);
	char *GetVariableStrUTF8(UINT32 dwVarId, char *pszBuffer = NULL, UINT32 dwBufSize = 0);
   UINT32 GetVariableBinary(UINT32 dwVarId, BYTE *pBuffer, UINT32 dwBufSize);
   UINT32 GetVariableInt32Array(UINT32 dwVarId, UINT32 dwNumElements, UINT32 *pdwBuffer);

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
   UINT32 dwId;       // Message ID
   UINT32 dwTTL;      // Message time-to-live in milliseconds
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
   UINT32 m_dwMsgHoldTime;
   UINT32 m_dwNumElements;
   WAIT_QUEUE_ELEMENT *m_pElements;
   THREAD m_hHkThread;

   void lock() { MutexLock(m_mutexDataAccess); }
   void unlock() { MutexUnlock(m_mutexDataAccess); }
   void housekeeperThread();
   void *waitForMessageInternal(UINT16 wIsBinary, UINT16 wCode, UINT32 dwId, UINT32 dwTimeOut);
   
   static THREAD_RESULT THREAD_CALL mwqThreadStarter(void *);

public:
   MsgWaitQueue();
   ~MsgWaitQueue();

   void put(CSCPMessage *pMsg);
   void put(CSCP_MESSAGE *pMsg);
   CSCPMessage *waitForMessage(WORD wCode, UINT32 dwId, UINT32 dwTimeOut)
   {
      return (CSCPMessage *)waitForMessageInternal(0, wCode, dwId, dwTimeOut);
   }
   CSCP_MESSAGE *waitForRawMessage(WORD wCode, UINT32 dwId, UINT32 dwTimeOut)
   {
      return (CSCP_MESSAGE *)waitForMessageInternal(1, wCode, dwId, dwTimeOut);
   }
   
   void clear();
   void setHoldTime(UINT32 dwHoldTime) { m_dwMsgHoldTime = dwHoldTime; }
};

/**
 * NXCP encryption context
 */
class LIBNETXMS_EXPORTABLE NXCPEncryptionContext : public RefCountObject
{
private:
	int m_cipher;
	BYTE *m_sessionKey;
	int m_keyLength;
	BYTE m_iv[EVP_MAX_IV_LENGTH];

	NXCPEncryptionContext();

public:
	static NXCPEncryptionContext *create(CSCPMessage *msg, RSA *privateKey);
	static NXCPEncryptionContext *create(UINT32 ciphers);

	virtual ~NXCPEncryptionContext();

	int getCipher() { return m_cipher; }
	BYTE *getSessionKey() { return m_sessionKey; }
	int getKeyLength() { return m_keyLength; }
	BYTE *getIV() { return m_iv; }
};

#else    /* __cplusplus */

typedef void CSCPMessage;
typedef void NXCPEncryptionContext;

#endif


//
// Functions
//

#ifdef __cplusplus

int LIBNETXMS_EXPORTABLE RecvNXCPMessage(SOCKET hSocket, CSCP_MESSAGE *pMsg,
                                         CSCP_BUFFER *pBuffer, UINT32 dwMaxMsgSize,
                                         NXCPEncryptionContext **ppCtx,
                                         BYTE *pDecryptionBuffer, UINT32 dwTimeout);
int LIBNETXMS_EXPORTABLE RecvNXCPMessageEx(SOCKET hSocket, CSCP_MESSAGE **msgBuffer,
                                           CSCP_BUFFER *nxcpBuffer, UINT32 *bufferSize,
                                           NXCPEncryptionContext **ppCtx, 
                                           BYTE **decryptionBuffer, UINT32 dwTimeout,
														 UINT32 maxMsgSize);
CSCP_MESSAGE LIBNETXMS_EXPORTABLE *CreateRawNXCPMessage(WORD wCode, UINT32 dwId, WORD wFlags,
                                                        UINT32 dwDataSize, void *pData,
                                                        CSCP_MESSAGE *pBuffer);
TCHAR LIBNETXMS_EXPORTABLE *NXCPMessageCodeName(WORD wCode, TCHAR *pszBuffer);
BOOL LIBNETXMS_EXPORTABLE SendFileOverNXCP(SOCKET hSocket, UINT32 dwId, const TCHAR *pszFile,
                                           NXCPEncryptionContext *pCtx, long offset,
														 void (* progressCallback)(INT64, void *), void *cbArg,
														 MUTEX mutex);
BOOL LIBNETXMS_EXPORTABLE NXCPGetPeerProtocolVersion(SOCKET hSocket, int *pnVersion, MUTEX mutex);
   
BOOL LIBNETXMS_EXPORTABLE InitCryptoLib(UINT32 dwEnabledCiphers);
UINT32 LIBNETXMS_EXPORTABLE CSCPGetSupportedCiphers();
CSCP_ENCRYPTED_MESSAGE LIBNETXMS_EXPORTABLE *CSCPEncryptMessage(NXCPEncryptionContext *pCtx, CSCP_MESSAGE *pMsg);
BOOL LIBNETXMS_EXPORTABLE CSCPDecryptMessage(NXCPEncryptionContext *pCtx,
                                             CSCP_ENCRYPTED_MESSAGE *pMsg,
                                             BYTE *pDecryptionBuffer);
UINT32 LIBNETXMS_EXPORTABLE SetupEncryptionContext(CSCPMessage *pMsg, 
                                                  NXCPEncryptionContext **ppCtx,
                                                  CSCPMessage **ppResponse,
                                                  RSA *pPrivateKey, int nNXCPVersion);
void LIBNETXMS_EXPORTABLE PrepareKeyRequestMsg(CSCPMessage *pMsg, RSA *pServerKey, bool useX509Format);
RSA LIBNETXMS_EXPORTABLE *LoadRSAKeys(const TCHAR *pszKeyFile);

#ifdef _WIN32
BOOL LIBNETXMS_EXPORTABLE SignMessageWithCAPI(BYTE *pMsg, UINT32 dwMsgLen, const CERT_CONTEXT *pCert,
												          BYTE *pBuffer, UINT32 dwBufSize, UINT32 *pdwSigLen);
#endif

#endif

#endif   /* _nxcpapi_h_ */
