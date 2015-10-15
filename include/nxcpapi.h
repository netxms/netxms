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

#include <nms_util.h>
#include <nms_threads.h>
#include <uuid.h>

#ifdef _WIN32
#include <wincrypt.h>
#endif

/**
 * Temporary buffer structure for RecvNXCPMessage() function
 */
typedef struct
{
   UINT32 bufferSize;
   UINT32 bufferPos;
   char buffer[NXCP_TEMP_BUF_SIZE];
} NXCP_BUFFER;


#ifdef __cplusplus

struct MessageField;

/**
 * Parsed NXCP message
 */
class LIBNETXMS_EXPORTABLE NXCPMessage
{
private:
   UINT16 m_code;
   UINT16 m_flags;
   UINT32 m_id;
   MessageField *m_fields; // Message fields
   int m_version;          // Protocol version
   BYTE *m_data;           // binary data
   size_t m_dataSize;      // binary data size

   void *set(UINT32 fieldId, BYTE type, const void *value, bool isSigned = false, size_t size = 0);
   void *get(UINT32 fieldId, BYTE requiredType, BYTE *fieldType = NULL);
   NXCP_MESSAGE_FIELD *find(UINT32 fieldId);

public:
   NXCPMessage(int version = NXCP_VERSION);
   NXCPMessage(NXCPMessage *msg);
   NXCPMessage(NXCP_MESSAGE *rawMag, int version = NXCP_VERSION);
   ~NXCPMessage();

   NXCP_MESSAGE *createMessage();

   UINT16 getCode() { return m_code; }
   void setCode(UINT16 code) { m_code = code; }

   UINT32 getId() { return m_id; }
   void setId(UINT32 id) { m_id = id; }

   bool isEndOfFile() { return (m_flags & MF_END_OF_FILE) ? true : false; }
   bool isEndOfSequence() { return (m_flags & MF_END_OF_SEQUENCE) ? true : false; }
   bool isReverseOrder() { return (m_flags & MF_REVERSE_ORDER) ? true : false; }
   bool isBinary() { return (m_flags & MF_BINARY) ? true : false; }

   BYTE *getBinaryData() { return m_data; }
   size_t getBinaryDataSize() { return m_dataSize; }

   bool isFieldExist(UINT32 fieldId) { return find(fieldId) != NULL; }
   int getFieldType(UINT32 fieldId);

   void setField(UINT32 fieldId, INT16 value) { set(fieldId, NXCP_DT_INT16, &value, true); }
   void setField(UINT32 fieldId, UINT16 value) { set(fieldId, NXCP_DT_INT16, &value, false); }
   void setField(UINT32 fieldId, INT32 value) { set(fieldId, NXCP_DT_INT32, &value, true); }
   void setField(UINT32 fieldId, UINT32 value) { set(fieldId, NXCP_DT_INT32, &value, false); }
   void setField(UINT32 fieldId, INT64 value) { set(fieldId, NXCP_DT_INT64, &value, true); }
   void setField(UINT32 fieldId, UINT64 value) { set(fieldId, NXCP_DT_INT64, &value, false); }
   void setField(UINT32 fieldId, double value) { set(fieldId, NXCP_DT_FLOAT, &value); }
   void setField(UINT32 fieldId, const TCHAR *value) { if (value != NULL) set(fieldId, NXCP_DT_STRING, value); }
   void setField(UINT32 fieldId, const TCHAR *value, size_t maxLen) { if (value != NULL) set(fieldId, NXCP_DT_STRING, value, false, maxLen); }
   void setField(UINT32 fieldId, const BYTE *value, size_t size) { set(fieldId, NXCP_DT_BINARY, value, false, size); }
   void setField(UINT32 fieldId, const InetAddress& value) { set(fieldId, NXCP_DT_INETADDR, &value); }
   void setField(UINT32 fieldId, const uuid& value) { set(fieldId, NXCP_DT_BINARY, value.getValue(), false, UUID_LENGTH); }
#ifdef UNICODE
   void setFieldFromMBString(UINT32 fieldId, const char *value);
#else
   void setFieldFromMBString(UINT32 fieldId, const char *value) { set(fieldId, NXCP_DT_STRING, value); }
#endif
   void setFieldFromTime(UINT32 fieldId, time_t value) { UINT64 t = (UINT64)value; set(fieldId, NXCP_DT_INT64, &t); }
   void setFieldFromInt32Array(UINT32 fieldId, size_t numElements, const UINT32 *elements);
   void setFieldFromInt32Array(UINT32 fieldId, IntegerArray<UINT32> *data);
   bool setFieldFromFile(UINT32 fieldId, const TCHAR *pszFileName);

   INT16 getFieldAsInt16(UINT32 fieldId);
   UINT16 getFieldAsUInt16(UINT32 fieldId);
   INT32 getFieldAsInt32(UINT32 fieldId);
   UINT32 getFieldAsUInt32(UINT32 fieldId);
   INT64 getFieldAsInt64(UINT32 fieldId);
   UINT64 getFieldAsUInt64(UINT32 fieldId);
   double getFieldAsDouble(UINT32 fieldId);
   bool getFieldAsBoolean(UINT32 fieldId);
   time_t getFieldAsTime(UINT32 fieldId);
   UINT32 getFieldAsInt32Array(UINT32 fieldId, UINT32 numElements, UINT32 *buffer);
   UINT32 getFieldAsInt32Array(UINT32 fieldId, IntegerArray<UINT32> *data);
   BYTE *getBinaryFieldPtr(UINT32 fieldId, size_t *size);
   TCHAR *getFieldAsString(UINT32 fieldId, TCHAR *buffer = NULL, size_t bufferSize = 0);
	char *getFieldAsMBString(UINT32 fieldId, char *buffer = NULL, size_t bufferSize = 0);
	char *getFieldAsUtf8String(UINT32 fieldId, char *buffer = NULL, size_t bufferSize = 0);
   UINT32 getFieldAsBinary(UINT32 fieldId, BYTE *buffer, size_t bufferSize);
   InetAddress getFieldAsInetAddress(UINT32 fieldId);
   uuid getFieldAsGUID(UINT32 fieldId);

   void deleteAllFields();

   void disableEncryption() { m_flags |= MF_DONT_ENCRYPT; }
   void setEndOfSequence() { m_flags |= MF_END_OF_SEQUENCE; }
   void setReverseOrderFlag() { m_flags |= MF_REVERSE_ORDER; }

   static String dump(NXCP_MESSAGE *msg, int version);
};

/**
 * Message waiting queue element structure
 */
typedef struct
{
   void *msg;         // Pointer to message, either to NXCPMessage object or raw message
   UINT64 sequence;   // Sequence number
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
#if defined(_WIN32)
   CRITICAL_SECTION m_mutex;
   HANDLE m_wakeupEvents[MAX_MSGQUEUE_WAITERS];
   BYTE m_waiters[MAX_MSGQUEUE_WAITERS];
#elif defined(_USE_GNU_PTH)
   pth_mutex_t m_mutex;
   pth_cond_t m_wakeupCondition;
#else
   pthread_mutex_t m_mutex;
   pthread_cond_t m_wakeupCondition;
#endif
   UINT32 m_holdTime;
   int m_size;
   int m_allocated;
   WAIT_QUEUE_ELEMENT *m_elements;
   UINT64 m_sequence;

   void *waitForMessageInternal(UINT16 isBinary, UINT16 code, UINT32 id, UINT32 timeout);

   void lock()
   {
#ifdef _WIN32
      EnterCriticalSection(&m_mutex);
#elif defined(_USE_GNU_PTH)
      pth_mutex_acquire(&m_mutex, FALSE, NULL);
#else
      pthread_mutex_lock(&m_mutex);
#endif
   }

   void unlock()
   {
#ifdef _WIN32
      LeaveCriticalSection(&m_mutex);
#elif defined(_USE_GNU_PTH)
      pth_mutex_release(&m_mutex);
#else
      pthread_mutex_unlock(&m_mutex);
#endif
   }

   void housekeeperRun();

   static MUTEX m_housekeeperLock;
   static HashMap<UINT64, MsgWaitQueue> *m_activeQueues;
   static CONDITION m_shutdownCondition;
   static THREAD m_housekeeperThread;
   static EnumerationCallbackResult houseKeeperCallback(const void *key, const void *object, void *arg);
   static THREAD_RESULT THREAD_CALL housekeeperThread(void *);
   static EnumerationCallbackResult diagInfoCallback(const void *key, const void *object, void *arg);

public:
   MsgWaitQueue();
   ~MsgWaitQueue();

   void put(NXCPMessage *pMsg);
   void put(NXCP_MESSAGE *pMsg);
   NXCPMessage *waitForMessage(WORD wCode, UINT32 dwId, UINT32 dwTimeOut)
   {
      return (NXCPMessage *)waitForMessageInternal(0, wCode, dwId, dwTimeOut);
   }
   NXCP_MESSAGE *waitForRawMessage(WORD wCode, UINT32 dwId, UINT32 dwTimeOut)
   {
      return (NXCP_MESSAGE *)waitForMessageInternal(1, wCode, dwId, dwTimeOut);
   }

   void clear();
   void setHoldTime(UINT32 holdTime) { m_holdTime = holdTime; }

   static void shutdown();
   static String getDiagInfo();
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
	static NXCPEncryptionContext *create(NXCPMessage *msg, RSA *privateKey);
	static NXCPEncryptionContext *create(UINT32 ciphers);

	virtual ~NXCPEncryptionContext();

   NXCP_ENCRYPTED_MESSAGE *encryptMessage(NXCP_MESSAGE *msg);
   bool decryptMessage(NXCP_ENCRYPTED_MESSAGE *msg, BYTE *decryptionBuffer);

	int getCipher() { return m_cipher; }
	BYTE *getSessionKey() { return m_sessionKey; }
	int getKeyLength() { return m_keyLength; }
	BYTE *getIV() { return m_iv; }
};

/**
 * Message receiver result codes
 */
enum MessageReceiverResult
{
   MSGRECV_SUCCESS = 0,
   MSGRECV_CLOSED = 1,
   MSGRECV_TIMEOUT = 2,
   MSGRECV_COMM_FAILURE = 3,
   MSGRECV_DECRYPTION_FAILURE = 4
};

/**
 * Message receiver - abstract base class
 */
class LIBNETXMS_EXPORTABLE AbstractMessageReceiver
{
private:
   BYTE *m_buffer;
   BYTE *m_decryptionBuffer;
   NXCPEncryptionContext *m_encryptionContext;
   size_t m_initialSize;
   size_t m_size;
   size_t m_maxSize;
   size_t m_dataSize;
   size_t m_bytesToSkip;

   NXCPMessage *getMessageFromBuffer();

protected:
   virtual int readBytes(BYTE *buffer, size_t size, UINT32 timeout) = 0;

public:
   AbstractMessageReceiver(size_t initialSize, size_t maxSize);
   virtual ~AbstractMessageReceiver();

   void setEncryptionContext(NXCPEncryptionContext *ctx) { m_encryptionContext = ctx; }

   NXCPMessage *readMessage(UINT32 timeout, MessageReceiverResult *result);
   NXCP_MESSAGE *getRawMessageBuffer() { return (NXCP_MESSAGE *)m_buffer; }

   static const TCHAR *resultToText(MessageReceiverResult result);
};

/**
 * Message receiver - socket implementation
 */
class LIBNETXMS_EXPORTABLE SocketMessageReceiver : public AbstractMessageReceiver
{
private:
   SOCKET m_socket;

protected:
   virtual int readBytes(BYTE *buffer, size_t size, UINT32 timeout);

public:
   SocketMessageReceiver(SOCKET socket, size_t initialSize, size_t maxSize);
   virtual ~SocketMessageReceiver();
};

/**
 * Message receiver - UNIX socket/named pipe implementation
 */
class LIBNETXMS_EXPORTABLE PipeMessageReceiver : public AbstractMessageReceiver
{
private:
   HPIPE m_pipe;
#ifdef _WIN32
	HANDLE m_readEvent;
#endif

protected:
   virtual int readBytes(BYTE *buffer, size_t size, UINT32 timeout);

public:
   PipeMessageReceiver(HPIPE pipe, size_t initialSize, size_t maxSize);
   virtual ~PipeMessageReceiver();
};

/**
 * NXCP compression methods
 */
enum NXCPCompressionMethod
{
   NXCP_COMPRESSION_NONE = 0,
   NXCP_COMPRESSION_LZ4 = 1
};

/**
 * Abstract stream compressor
 */
class LIBNETXMS_EXPORTABLE StreamCompressor
{
public:
   virtual ~StreamCompressor();

   virtual size_t compress(const BYTE *in, size_t inSize, BYTE *out, size_t maxOutSize) = 0;
   virtual size_t decompress(const BYTE *in, size_t inSize, const BYTE **out) = 0;
   virtual size_t compressBufferSize(size_t dataSize) = 0;

   static StreamCompressor *create(NXCPCompressionMethod method, bool compress, size_t maxBlockSize);
};

/**
 * Dummy stream compressor
 */
class LIBNETXMS_EXPORTABLE DummyStreamCompressor : public StreamCompressor
{
public:
   virtual ~DummyStreamCompressor();

   virtual size_t compress(const BYTE *in, size_t inSize, BYTE *out, size_t maxOutSize);
   virtual size_t decompress(const BYTE *in, size_t inSize, const BYTE **out);
   virtual size_t compressBufferSize(size_t dataSize);
};

struct __LZ4_stream_t;
struct __LZ4_streamDecode_t;

/**
 * LZ4 stream compressor
 */
class LIBNETXMS_EXPORTABLE LZ4StreamCompressor : public StreamCompressor
{
private:
   union
   {
      __LZ4_stream_t *encoder;
      __LZ4_streamDecode_t *decoder;
   } m_stream;
   char *m_buffer;
   size_t m_maxBlockSize;
   size_t m_bufferSize;
   size_t m_bufferPos;
   bool m_compress;

public:
   LZ4StreamCompressor(bool compress, size_t maxBlockSize);
   virtual ~LZ4StreamCompressor();

   virtual size_t compress(const BYTE *in, size_t inSize, BYTE *out, size_t maxOutSize);
   virtual size_t decompress(const BYTE *in, size_t inSize, const BYTE **out);
   virtual size_t compressBufferSize(size_t dataSize);
};

#if 0
/**
 * NXCP message consumer interface
 */
class LIBNETXMS_EXPORTABLE MessageConsumer
{
public:
   virtual SOCKET getSocket() = 0;
   virtual void processMessage(NXCPMessage *msg) = 0;
};

/**
 * Socket receiver - manages receiving NXCP messages from multiple sockets
 */
class LIBNETXMS_EXPORTABLE SocketReceiver
{
private:
   THREAD m_thread;
   HashMap<SOCKET, MessageConsumer> *m_consumers;

   static int m_maxSocketsPerThread;
   static ObjectArray<SocketReceiver> *m_receivers;

public:
   static void start();
   static void shutdown();

   static void addConsumer(MessageConsumer *mc);
   static void removeConsumer(MessageConsumer *mc);

   static String getDiagInfo();
};
#endif

#else    /* __cplusplus */

typedef void NXCPMessage;
typedef void NXCPEncryptionContext;

#endif


//
// Functions
//

#ifdef __cplusplus

int LIBNETXMS_EXPORTABLE RecvNXCPMessage(SOCKET hSocket, NXCP_MESSAGE *pMsg,
                                         NXCP_BUFFER *pBuffer, UINT32 dwMaxMsgSize,
                                         NXCPEncryptionContext **ppCtx,
                                         BYTE *pDecryptionBuffer, UINT32 dwTimeout);
int LIBNETXMS_EXPORTABLE RecvNXCPMessageEx(SOCKET hSocket, NXCP_MESSAGE **msgBuffer,
                                           NXCP_BUFFER *nxcpBuffer, UINT32 *bufferSize,
                                           NXCPEncryptionContext **ppCtx,
                                           BYTE **decryptionBuffer, UINT32 dwTimeout,
														 UINT32 maxMsgSize);
NXCP_MESSAGE LIBNETXMS_EXPORTABLE *CreateRawNXCPMessage(WORD wCode, UINT32 dwId, WORD flags,
                                                        UINT32 dwDataSize, void *pData,
                                                        NXCP_MESSAGE *pBuffer);
TCHAR LIBNETXMS_EXPORTABLE *NXCPMessageCodeName(WORD wCode, TCHAR *buffer);
BOOL LIBNETXMS_EXPORTABLE SendFileOverNXCP(SOCKET hSocket, UINT32 dwId, const TCHAR *pszFile,
                                           NXCPEncryptionContext *pCtx, long offset,
														 void (* progressCallback)(INT64, void *), void *cbArg,
														 MUTEX mutex, NXCPCompressionMethod compressionMethod = NXCP_COMPRESSION_NONE);
BOOL LIBNETXMS_EXPORTABLE NXCPGetPeerProtocolVersion(SOCKET hSocket, int *pnVersion, MUTEX mutex);

bool LIBNETXMS_EXPORTABLE InitCryptoLib(UINT32 dwEnabledCiphers, void (*debugCallback)(int, const TCHAR *, va_list args));
UINT32 LIBNETXMS_EXPORTABLE NXCPGetSupportedCiphers();
String LIBNETXMS_EXPORTABLE NXCPGetSupportedCiphersAsText();
NXCP_ENCRYPTED_MESSAGE LIBNETXMS_EXPORTABLE *NXCPEncryptMessage(NXCPEncryptionContext *pCtx, NXCP_MESSAGE *pMsg);
bool LIBNETXMS_EXPORTABLE NXCPDecryptMessage(NXCPEncryptionContext *pCtx,
                                             NXCP_ENCRYPTED_MESSAGE *pMsg,
                                             BYTE *pDecryptionBuffer);
UINT32 LIBNETXMS_EXPORTABLE SetupEncryptionContext(NXCPMessage *pMsg,
                                                  NXCPEncryptionContext **ppCtx,
                                                  NXCPMessage **ppResponse,
                                                  RSA *pPrivateKey, int nNXCPVersion);
void LIBNETXMS_EXPORTABLE PrepareKeyRequestMsg(NXCPMessage *pMsg, RSA *pServerKey, bool useX509Format);
RSA LIBNETXMS_EXPORTABLE *LoadRSAKeys(const TCHAR *pszKeyFile);

#ifdef _WIN32
BOOL LIBNETXMS_EXPORTABLE SignMessageWithCAPI(BYTE *pMsg, UINT32 dwMsgLen, const CERT_CONTEXT *pCert,
												          BYTE *pBuffer, size_t bufferSize, UINT32 *pdwSigLen);
#endif

#endif

#endif   /* _nxcpapi_h_ */
