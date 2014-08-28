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

/**
 * Temporary buffer structure for RecvCSCPMessage() function
 */
typedef struct
{
   UINT32 dwBufSize;
   UINT32 dwBufPos;
   char szBuffer[CSCP_TEMP_BUF_SIZE];
} CSCP_BUFFER;


#ifdef __cplusplus

struct MessageField;

/**
 * Parsed NXCP message
 */
class LIBNETXMS_EXPORTABLE CSCPMessage
{
private:
   WORD m_code;
   WORD m_flags;
   UINT32 m_id;
   MessageField *m_fields;   // Message fields
   int m_version;    // Protocol version

   void *set(UINT32 fieldId, BYTE type, const void *value, UINT32 size = 0);
   void *get(UINT32 fieldId, BYTE type);
   CSCP_DF *find(UINT32 fieldId);

public:
   CSCPMessage(int nVersion = NXCP_VERSION);
   CSCPMessage(CSCPMessage *pMsg);
   CSCPMessage(CSCP_MESSAGE *pMsg, int nVersion = NXCP_VERSION);
   CSCPMessage(const char *xml);
   ~CSCPMessage();

   CSCP_MESSAGE *createMessage();
	char *createXML();
	void processXMLToken(void *state, const char **attrs);
	void processXMLData(void *state);

   WORD GetCode() { return m_code; }
   void SetCode(WORD code) { m_code = code; }

   UINT32 GetId() { return m_id; }
   void SetId(UINT32 id) { m_id = id; }

   bool isEndOfSequence() { return (m_flags & MF_END_OF_SEQUENCE) ? true : false; }
   bool isReverseOrder() { return (m_flags & MF_REVERSE_ORDER) ? true : false; }

   bool isFieldExist(UINT32 fieldId) { return find(fieldId) != NULL; }
   int getFieldType(UINT32 fieldId);

   void SetVariable(UINT32 dwVarId, INT16 wValue) { set(dwVarId, CSCP_DT_INT16, &wValue); }
   void SetVariable(UINT32 dwVarId, UINT16 wValue) { set(dwVarId, CSCP_DT_INT16, &wValue); }
   void SetVariable(UINT32 dwVarId, INT32 dwValue) { set(dwVarId, CSCP_DT_INTEGER, &dwValue); }
   void SetVariable(UINT32 dwVarId, UINT32 dwValue) { set(dwVarId, CSCP_DT_INTEGER, &dwValue); }
   void SetVariable(UINT32 dwVarId, INT64 qwValue) { set(dwVarId, CSCP_DT_INT64, &qwValue); }
   void SetVariable(UINT32 dwVarId, UINT64 qwValue) { set(dwVarId, CSCP_DT_INT64, &qwValue); }
   void SetVariable(UINT32 dwVarId, double dValue) { set(dwVarId, CSCP_DT_FLOAT, &dValue); }
   void SetVariable(UINT32 dwVarId, const TCHAR *value) { if (value != NULL) set(dwVarId, CSCP_DT_STRING, value); }
   void SetVariable(UINT32 dwVarId, const TCHAR *value, UINT32 maxLen) { if (value != NULL) set(dwVarId, CSCP_DT_STRING, value, maxLen); }
   void SetVariable(UINT32 dwVarId, BYTE *pValue, UINT32 dwSize) { set(dwVarId, CSCP_DT_BINARY, pValue, dwSize); }
#ifdef UNICODE
   void SetVariableFromMBString(UINT32 dwVarId, const char *pszValue);
#else
   void SetVariableFromMBString(UINT32 dwVarId, const char *pszValue) { set(dwVarId, CSCP_DT_STRING, pszValue); }
#endif
   void setFieldInt32Array(UINT32 dwVarId, UINT32 dwNumElements, const UINT32 *pdwData);
   void setFieldInt32Array(UINT32 dwVarId, IntegerArray<UINT32> *data);
   bool setFieldFromFile(UINT32 dwVarId, const TCHAR *pszFileName);

   INT16 getFieldAsInt16(UINT32 fieldId);
   INT32 getFieldAsInt32(UINT32 fieldId);
   INT64 getFieldAsInt64(UINT32 fieldId);
   double getFieldAsDouble(UINT32 fieldId);
   bool getFieldAsBoolean(UINT32 fieldId);
   UINT32 getFieldAsInt32Array(UINT32 fieldId, UINT32 numElements, UINT32 *buffer);
   UINT32 getFieldAsInt32Array(UINT32 fieldId, IntegerArray<UINT32> *data);
   BYTE *getBinaryFieldPtr(UINT32 fieldId, size_t *size);

   UINT32 GetVariableLong(UINT32 dwVarId);
   UINT64 GetVariableInt64(UINT32 dwVarId);
   UINT16 GetVariableShort(UINT32 dwVarId);
   TCHAR *GetVariableStr(UINT32 dwVarId, TCHAR *szBuffer = NULL, UINT32 dwBufSize = 0);
	char *GetVariableStrA(UINT32 dwVarId, char *pszBuffer = NULL, UINT32 dwBufSize = 0);
	char *GetVariableStrUTF8(UINT32 dwVarId, char *pszBuffer = NULL, UINT32 dwBufSize = 0);
   UINT32 GetVariableBinary(UINT32 dwVarId, BYTE *pBuffer, UINT32 dwBufSize);

   void deleteAllVariables();

   void disableEncryption() { m_flags |= MF_DONT_ENCRYPT; }
   void setEndOfSequence() { m_flags |= MF_END_OF_SEQUENCE; }
   void setReverseOrderFlag() { m_flags |= MF_REVERSE_ORDER; }

   static String dump(CSCP_MESSAGE *msg, int version);
};

/**
 * Message waiting queue element structure
 */
typedef struct
{
   void *msg;         // Pointer to message, either to CSCPMessage object or raw message
   UINT32 id;         // Message ID
   UINT32 ttl;        // Message time-to-live in milliseconds
   UINT16 code;       // Message code
   UINT16 isBinary;   // 1 for binary (raw) messages
} WAIT_QUEUE_ELEMENT;

/**
 * Max number of waiting threads in message queue
 */
#define MAX_MSGQUEUE_WAITERS     32

/**
 * Message waiting queue class
 */
class LIBNETXMS_EXPORTABLE MsgWaitQueue
{
private:
#ifdef _WIN32
   CRITICAL_SECTION m_mutex;
   HANDLE m_wakeupEvents[MAX_MSGQUEUE_WAITERS];
   BYTE m_waiters[MAX_MSGQUEUE_WAITERS];
#else
   pthread_mutex_t m_mutex;
   pthread_cond_t m_wakeupCondition;
#endif
   CONDITION m_stopCondition;
   UINT32 m_holdTime;
   int m_size;
   int m_allocated;
   WAIT_QUEUE_ELEMENT *m_elements;
   THREAD m_hHkThread;

   void housekeeperThread();
   void *waitForMessageInternal(UINT16 isBinary, UINT16 code, UINT32 id, UINT32 timeout);

   void lock()
   {
#ifdef _WIN32
      EnterCriticalSection(&m_mutex);
#else
      pthread_mutex_lock(&m_mutex);
#endif
   }

   void unlock()
   {
#ifdef _WIN32
      LeaveCriticalSection(&m_mutex);
#else
      pthread_mutex_unlock(&m_mutex);
#endif
   }

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
   void setHoldTime(UINT32 holdTime) { m_holdTime = holdTime; }
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
#ifdef _WITH_ENCRYPTION
   EVP_CIPHER_CTX m_encryptor;
   EVP_CIPHER_CTX m_decryptor;
   MUTEX m_encryptorLock;
#endif

	NXCPEncryptionContext();
   bool initCipher(int cipher);

public:
	static NXCPEncryptionContext *create(CSCPMessage *msg, RSA *privateKey);
	static NXCPEncryptionContext *create(UINT32 ciphers);

	virtual ~NXCPEncryptionContext();

   CSCP_ENCRYPTED_MESSAGE *encryptMessage(CSCP_MESSAGE *msg);
   bool decryptMessage(CSCP_ENCRYPTED_MESSAGE *msg, BYTE *decryptionBuffer);

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

BOOL LIBNETXMS_EXPORTABLE InitCryptoLib(UINT32 dwEnabledCiphers, void (*debugCallback)(int, const TCHAR *, va_list args));
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
