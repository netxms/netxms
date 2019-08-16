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
#include <nxcrypto.h>
#include <uuid.h>

#ifdef _WIN32
#include <wincrypt.h>
#endif

#ifdef _WITH_ENCRYPTION
#include <openssl/ssl.h>
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
 * Default size hint
 */
#define NXCP_DEFAULT_SIZE_HINT   (4096)

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
   MemoryPool m_pool;

   NXCPMessage(const NXCP_MESSAGE *msg, int version);

   void *set(UINT32 fieldId, BYTE type, const void *value, bool isSigned = false, size_t size = 0, bool isUtf8 = false);
   void *get(UINT32 fieldId, BYTE requiredType, BYTE *fieldType = NULL) const;
   NXCP_MESSAGE_FIELD *find(UINT32 fieldId) const;
   bool isValid() { return m_version != -1; }

   TCHAR *getFieldAsString(UINT32 fieldId, MemoryPool *pool, TCHAR *buffer, size_t bufferSize) const;

public:
   NXCPMessage(int version = NXCP_VERSION);
   NXCPMessage(UINT16 code, UINT32 id, int version = NXCP_VERSION);
   NXCPMessage(NXCPMessage *msg);
   ~NXCPMessage();

   static NXCPMessage *deserialize(const NXCP_MESSAGE *rawMsg, int version = NXCP_VERSION);
   NXCP_MESSAGE *serialize(bool allowCompression = false) const;

   UINT16 getCode() const { return m_code; }
   void setCode(UINT16 code) { m_code = code; }

   UINT32 getId() const { return m_id; }
   void setId(UINT32 id) { m_id = id; }

   int getProtocolVersion() const { return m_version; }
   void setProtocolVersion(int version);
   int getEncodedProtocolVersion() const { return (m_flags & 0xF000) >> 12; }

   bool isEndOfFile() const { return (m_flags & MF_END_OF_FILE) ? true : false; }
   bool isEndOfSequence() const { return (m_flags & MF_END_OF_SEQUENCE) ? true : false; }
   bool isReverseOrder() const { return (m_flags & MF_REVERSE_ORDER) ? true : false; }
   bool isBinary() const { return (m_flags & MF_BINARY) ? true : false; }
   bool isControl() const { return (m_flags & MF_CONTROL) ? true : false; }
   bool isCompressedStream() const { return ((m_flags & (MF_COMPRESSED | MF_STREAM)) == (MF_COMPRESSED | MF_STREAM)) ? true : false; }

   const BYTE *getBinaryData() const { return m_data; }
   size_t getBinaryDataSize() const { return m_dataSize; }

   bool isFieldExist(UINT32 fieldId) const { return find(fieldId) != NULL; }
   int getFieldType(UINT32 fieldId) const;

   void setField(UINT32 fieldId, INT16 value) { set(fieldId, NXCP_DT_INT16, &value, true); }
   void setField(UINT32 fieldId, UINT16 value) { set(fieldId, NXCP_DT_INT16, &value, false); }
   void setField(UINT32 fieldId, INT32 value) { set(fieldId, NXCP_DT_INT32, &value, true); }
   void setField(UINT32 fieldId, UINT32 value) { set(fieldId, NXCP_DT_INT32, &value, false); }
   void setField(UINT32 fieldId, INT64 value) { set(fieldId, NXCP_DT_INT64, &value, true); }
   void setField(UINT32 fieldId, UINT64 value) { set(fieldId, NXCP_DT_INT64, &value, false); }
   void setField(UINT32 fieldId, double value) { set(fieldId, NXCP_DT_FLOAT, &value); }
   void setField(UINT32 fieldId, bool value) { INT16 v = value ? 1 : 0; set(fieldId, NXCP_DT_INT16, &v, true); }
   void setField(UINT32 fieldId, const String& value) { set(fieldId, (m_version >= 5) ? NXCP_DT_UTF8_STRING : NXCP_DT_STRING, (const TCHAR *)value); }
   void setField(UINT32 fieldId, const TCHAR *value) { if (value != NULL) set(fieldId, (m_version >= 5) ? NXCP_DT_UTF8_STRING : NXCP_DT_STRING, value); }
   void setField(UINT32 fieldId, const BYTE *value, size_t size) { set(fieldId, NXCP_DT_BINARY, value, false, size); }
   void setField(UINT32 fieldId, const InetAddress& value) { set(fieldId, NXCP_DT_INETADDR, &value); }
   void setField(UINT32 fieldId, const uuid& value) { set(fieldId, NXCP_DT_BINARY, value.getValue(), false, UUID_LENGTH); }
   void setField(UINT32 fieldId, const MacAddress& value) { set(fieldId, NXCP_DT_BINARY, value.value(), false, value.length()); }
#ifdef UNICODE
   void setFieldFromMBString(UINT32 fieldId, const char *value);
#else
   void setFieldFromMBString(UINT32 fieldId, const char *value) { set(fieldId, (m_version >= 5) ? NXCP_DT_UTF8_STRING : NXCP_DT_STRING, value); }
#endif
   void setFieldFromUtf8String(UINT32 fieldId, const char *value) { set(fieldId, (m_version >= 5) ? NXCP_DT_UTF8_STRING : NXCP_DT_STRING, value, false, 0, true); }
   void setFieldFromTime(UINT32 fieldId, time_t value) { UINT64 t = (UINT64)value; set(fieldId, NXCP_DT_INT64, &t); }
   void setFieldFromInt32Array(UINT32 fieldId, size_t numElements, const UINT32 *elements);
   void setFieldFromInt32Array(UINT32 fieldId, const IntegerArray<UINT32> *data);
   bool setFieldFromFile(UINT32 fieldId, const TCHAR *pszFileName);

   INT16 getFieldAsInt16(UINT32 fieldId) const;
   UINT16 getFieldAsUInt16(UINT32 fieldId) const;
   INT32 getFieldAsInt32(UINT32 fieldId) const;
   UINT32 getFieldAsUInt32(UINT32 fieldId) const;
   INT64 getFieldAsInt64(UINT32 fieldId) const;
   UINT64 getFieldAsUInt64(UINT32 fieldId) const;
   double getFieldAsDouble(UINT32 fieldId) const;
   bool getFieldAsBoolean(UINT32 fieldId) const;
   time_t getFieldAsTime(UINT32 fieldId) const;
   size_t getFieldAsInt32Array(UINT32 fieldId, UINT32 numElements, UINT32 *buffer) const;
   size_t getFieldAsInt32Array(UINT32 fieldId, IntegerArray<UINT32> *data) const;
   const BYTE *getBinaryFieldPtr(UINT32 fieldId, size_t *size) const;
   TCHAR *getFieldAsString(UINT32 fieldId, MemoryPool *pool) const { return getFieldAsString(fieldId, pool, NULL, 0); }
   TCHAR *getFieldAsString(UINT32 fieldId, TCHAR *buffer = NULL, size_t bufferSize = 0) const { return getFieldAsString(fieldId, NULL, buffer, bufferSize); }
	char *getFieldAsMBString(UINT32 fieldId, char *buffer = NULL, size_t bufferSize = 0) const;
	char *getFieldAsUtf8String(UINT32 fieldId, char *buffer = NULL, size_t bufferSize = 0) const;
   size_t getFieldAsBinary(UINT32 fieldId, BYTE *buffer, size_t bufferSize) const;
   InetAddress getFieldAsInetAddress(UINT32 fieldId) const;
   MacAddress getFieldAsMacAddress(UINT32 fieldId) const;
   uuid getFieldAsGUID(UINT32 fieldId) const;

   void deleteAllFields();

   void disableEncryption() { m_flags |= MF_DONT_ENCRYPT; }
   void setEndOfSequence() { m_flags |= MF_END_OF_SEQUENCE; }
   void setReverseOrderFlag() { m_flags |= MF_REVERSE_ORDER; }

   static String dump(const NXCP_MESSAGE *msg, int version);
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
   MUTEX m_encryptorLock;
   EVP_CIPHER_CTX *m_encryptor;
   EVP_CIPHER_CTX *m_decryptor;
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
   MSGRECV_DECRYPTION_FAILURE = 4,
   MSGRECV_PROTOCOL_ERROR = 5
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

   NXCPMessage *getMessageFromBuffer(bool *protocolError);

protected:
   virtual int readBytes(BYTE *buffer, size_t size, UINT32 timeout) = 0;

public:
   AbstractMessageReceiver(size_t initialSize, size_t maxSize);
   virtual ~AbstractMessageReceiver();

   virtual void cancel() = 0;

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
#ifndef _WIN32
   int m_controlPipe[2];
#endif

protected:
   virtual int readBytes(BYTE *buffer, size_t size, UINT32 timeout) override;

public:
   SocketMessageReceiver(SOCKET socket, size_t initialSize, size_t maxSize);
   virtual ~SocketMessageReceiver();

   virtual void cancel() override;
};

/**
 * Message receiver - comm channel implementation
 */
class LIBNETXMS_EXPORTABLE CommChannelMessageReceiver : public AbstractMessageReceiver
{
private:
   AbstractCommChannel *m_channel;

protected:
   virtual int readBytes(BYTE *buffer, size_t size, UINT32 timeout) override;

public:
   CommChannelMessageReceiver(AbstractCommChannel *channel, size_t initialSize, size_t maxSize);
   virtual ~CommChannelMessageReceiver();

   virtual void cancel() override;
};

#ifdef _WITH_ENCRYPTION

/**
 * Message receiver - SSL/TLS implementation
 */
class LIBNETXMS_EXPORTABLE TlsMessageReceiver : public AbstractMessageReceiver
{
private:
   SOCKET m_socket;
   SSL *m_ssl;
   MUTEX m_mutex;
#ifndef _WIN32
   int m_controlPipe[2];
#endif

protected:
   virtual int readBytes(BYTE *buffer, size_t size, UINT32 timeout) override;

public:
   TlsMessageReceiver(SOCKET socket, SSL *ssl, MUTEX mutex, size_t initialSize, size_t maxSize);
   virtual ~TlsMessageReceiver();

   virtual void cancel() override;
};

#endif /* _WITH_ENCRYPTION */

/**
 * Message receiver - UNIX socket/named pipe implementation
 */
class LIBNETXMS_EXPORTABLE PipeMessageReceiver : public AbstractMessageReceiver
{
private:
   HPIPE m_pipe;
#ifdef _WIN32
	HANDLE m_readEvent;
   HANDLE m_cancelEvent;
#else
   int m_controlPipe[2];
#endif

protected:
   virtual int readBytes(BYTE *buffer, size_t size, UINT32 timeout) override;

public:
   PipeMessageReceiver(HPIPE hpipe, size_t initialSize, size_t maxSize);
   virtual ~PipeMessageReceiver();

   virtual void cancel() override;
};

/**
 * NXCP stresam compression methods
 */
enum NXCPStreamCompressionMethod
{
   NXCP_STREAM_COMPRESSION_NONE = 0,
   NXCP_STREAM_COMPRESSION_LZ4 = 1,
   NXCP_STREAM_COMPRESSION_DEFLATE = 2
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

   static StreamCompressor *create(NXCPStreamCompressionMethod method, bool compress, size_t maxBlockSize);
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

struct z_stream_s;

/**
 * Deflate stream compressor
 */
class LIBNETXMS_EXPORTABLE DeflateStreamCompressor : public StreamCompressor
{
private:
   z_stream_s *m_stream;
   BYTE *m_buffer;
   size_t m_bufferSize;
   bool m_compress;

public:
   DeflateStreamCompressor(bool compress, size_t maxBlockSize);
   virtual ~DeflateStreamCompressor();

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

typedef bool (*NXCPMessageNameResolver)(UINT16 code, TCHAR *buffer);


//
// Functions
//

#ifdef __cplusplus

void LIBNETXMS_EXPORTABLE NXCPInitBuffer(NXCP_BUFFER *nxcpBuffer);
int LIBNETXMS_EXPORTABLE RecvNXCPMessage(SOCKET hSocket, NXCP_MESSAGE *pMsg,
                                         NXCP_BUFFER *pBuffer, UINT32 dwMaxMsgSize,
                                         NXCPEncryptionContext **ppCtx,
                                         BYTE *pDecryptionBuffer, UINT32 dwTimeout);
int LIBNETXMS_EXPORTABLE RecvNXCPMessage(AbstractCommChannel *channel, NXCP_MESSAGE *pMsg,
                                         NXCP_BUFFER *pBuffer, UINT32 dwMaxMsgSize,
                                         NXCPEncryptionContext **ppCtx,
                                         BYTE *pDecryptionBuffer, UINT32 dwTimeout);
int LIBNETXMS_EXPORTABLE RecvNXCPMessageEx(SOCKET hSocket, NXCP_MESSAGE **msgBuffer,
                                           NXCP_BUFFER *nxcpBuffer, UINT32 *bufferSize,
                                           NXCPEncryptionContext **ppCtx,
                                           BYTE **decryptionBuffer, UINT32 dwTimeout,
														 UINT32 maxMsgSize);
int LIBNETXMS_EXPORTABLE RecvNXCPMessageEx(AbstractCommChannel *channel, NXCP_MESSAGE **msgBuffer,
                                           NXCP_BUFFER *nxcpBuffer, UINT32 *bufferSize,
                                           NXCPEncryptionContext **ppCtx,
                                           BYTE **decryptionBuffer, UINT32 dwTimeout,
                                           UINT32 maxMsgSize);
NXCP_MESSAGE LIBNETXMS_EXPORTABLE *CreateRawNXCPMessage(UINT16 code, UINT32 id, UINT16 flags,
                                                        const void *data, size_t dataSize,
                                                        NXCP_MESSAGE *buffer, bool allowCompression);
bool LIBNETXMS_EXPORTABLE SendFileOverNXCP(SOCKET hSocket, UINT32 dwId, const TCHAR *pszFile,
                                           NXCPEncryptionContext *pCtx, long offset,
														 void (* progressCallback)(INT64, void *), void *cbArg,
														 MUTEX mutex, NXCPStreamCompressionMethod compressionMethod = NXCP_STREAM_COMPRESSION_NONE,
                                           VolatileCounter *cancellationFlag = NULL);
bool LIBNETXMS_EXPORTABLE SendFileOverNXCP(AbstractCommChannel *channel, UINT32 dwId, const TCHAR *pszFile,
                                           NXCPEncryptionContext *pCtx, long offset,
                                           void (* progressCallback)(INT64, void *), void *cbArg,
                                           MUTEX mutex, NXCPStreamCompressionMethod compressionMethod = NXCP_STREAM_COMPRESSION_NONE,
                                           VolatileCounter *cancellationFlag = NULL);
bool LIBNETXMS_EXPORTABLE NXCPGetPeerProtocolVersion(SOCKET s, int *pnVersion, MUTEX mutex);
bool LIBNETXMS_EXPORTABLE NXCPGetPeerProtocolVersion(AbstractCommChannel *channel, int *pnVersion, MUTEX mutex);

TCHAR LIBNETXMS_EXPORTABLE *NXCPMessageCodeName(UINT16 wCode, TCHAR *buffer);
void LIBNETXMS_EXPORTABLE NXCPRegisterMessageNameResolver(NXCPMessageNameResolver r);
void LIBNETXMS_EXPORTABLE NXCPUnregisterMessageNameResolver(NXCPMessageNameResolver r);

bool LIBNETXMS_EXPORTABLE InitCryptoLib(UINT32 dwEnabledCiphers);
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
RSA LIBNETXMS_EXPORTABLE *RSAKeyFromData(const BYTE *data, size_t size, bool withPrivate);
void LIBNETXMS_EXPORTABLE RSAFree(RSA *key);
RSA LIBNETXMS_EXPORTABLE *RSAGenerateKey(int bits);

#ifdef _WIN32
BOOL LIBNETXMS_EXPORTABLE SignMessageWithCAPI(BYTE *pMsg, UINT32 dwMsgLen, const CERT_CONTEXT *pCert,
												          BYTE *pBuffer, size_t bufferSize, UINT32 *pdwSigLen);
#endif

#endif

#endif   /* _nxcpapi_h_ */
