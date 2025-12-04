/*
** NetXMS - Network Management System
** NXCP API
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
#include <nxcrypto.h>

#ifdef __cplusplus
#include <istream>
#endif

#ifdef __cplusplus

struct MessageField;

/**
 * File upload append mode
 */
enum class FileTransferResumeMode : uint16_t
{
   OVERWRITE = 0, // Overwrite existing file
   CHECK = 1,     // Check if resume is possible
   RESUME = 2     // Resume file transfer (append to existing file part)
};

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
   uint16_t m_code;
   uint16_t m_flags;
   uint32_t m_id;
   MessageField *m_fields; // Message fields
   int m_version;          // Protocol version
   uint32_t m_controlData; // Data for control message
   BYTE *m_data;           // binary data
   size_t m_dataSize;      // binary data size
   MemoryPool m_pool;

   NXCPMessage(const NXCP_MESSAGE *msg, int version);

   void *set(uint32_t fieldId, BYTE type, const void *value, bool isSigned = false, size_t size = 0, bool isUtf8 = false);
   void *get(uint32_t fieldId, BYTE requiredType, BYTE *fieldType = NULL) const;
   NXCP_MESSAGE_FIELD *find(uint32_t fieldId) const;
   bool isValid() { return m_version != -1; }

   TCHAR *getFieldAsString(uint32_t fieldId, MemoryPool *pool, TCHAR *buffer, size_t bufferSize) const;

public:
   NXCPMessage(int version = NXCP_VERSION);
   NXCPMessage(uint16_t code, uint32_t id, int version = NXCP_VERSION);
   NXCPMessage(const NXCPMessage& msg);
   ~NXCPMessage();

   static NXCPMessage *deserialize(const NXCP_MESSAGE *rawMsg, int version = NXCP_VERSION);
   NXCP_MESSAGE *serialize(bool allowCompression = false) const;

   uint16_t getCode() const { return m_code; }
   void setCode(uint16_t code) { m_code = code; }

   uint32_t getId() const { return m_id; }
   void setId(uint32_t id) { m_id = id; }

   int getProtocolVersion() const { return m_version; }
   void setProtocolVersion(int version);
   int getEncodedProtocolVersion() const { return (m_flags & 0xF000) >> 12; }

   bool isEndOfFile() const { return (m_flags & MF_END_OF_FILE) ? true : false; }
   bool isEndOfSequence() const { return (m_flags & MF_END_OF_SEQUENCE) ? true : false; }
   bool isReverseOrder() const { return (m_flags & MF_REVERSE_ORDER) ? true : false; }
   bool isBinary() const { return (m_flags & MF_BINARY) ? true : false; }
   bool isControl() const { return (m_flags & MF_CONTROL) ? true : false; }
   bool isCompressedStream() const { return ((m_flags & (MF_COMPRESSED | MF_STREAM)) == (MF_COMPRESSED | MF_STREAM)) ? true : false; }

   uint32_t getControlData() const { return m_controlData; }
   void setControlData(uint32_t data) { m_controlData = data; }

   const BYTE *getBinaryData() const { return m_data; }
   size_t getBinaryDataSize() const { return m_dataSize; }

   bool isFieldExist(uint32_t fieldId) const { return find(fieldId) != nullptr; }
   int getFieldType(uint32_t fieldId) const;

   void setField(uint32_t fieldId, int16_t value) { set(fieldId, NXCP_DT_INT16, &value, true); }
   void setField(uint32_t fieldId, uint16_t value) { set(fieldId, NXCP_DT_INT16, &value, false); }
   void setField(uint32_t fieldId, int32_t value) { set(fieldId, NXCP_DT_INT32, &value, true); }
   void setField(uint32_t fieldId, uint32_t value) { set(fieldId, NXCP_DT_INT32, &value, false); }
#ifdef _WIN32
   void setField(uint32_t fieldId, DWORD value) { set(fieldId, NXCP_DT_INT32, &value, false); }
#endif
   void setField(uint32_t fieldId, int64_t value) { set(fieldId, NXCP_DT_INT64, &value, true); }
   void setField(uint32_t fieldId, uint64_t value) { set(fieldId, NXCP_DT_INT64, &value, false); }
   void setField(uint32_t fieldId, double value) { set(fieldId, NXCP_DT_FLOAT, &value); }
   void setField(uint32_t fieldId, bool value) { INT16 v = value ? 1 : 0; set(fieldId, NXCP_DT_INT16, &v, true); }
   void setField(uint32_t fieldId, const SharedString& value) { set(fieldId, (m_version >= 5) ? NXCP_DT_UTF8_STRING : NXCP_DT_STRING, value.cstr()); }
   void setField(uint32_t fieldId, const String& value) { set(fieldId, (m_version >= 5) ? NXCP_DT_UTF8_STRING : NXCP_DT_STRING, value.cstr()); }
   void setField(uint32_t fieldId, const TCHAR *value) { if (value != nullptr) set(fieldId, (m_version >= 5) ? NXCP_DT_UTF8_STRING : NXCP_DT_STRING, value); }
   void setField(uint32_t fieldId, const BYTE *value, size_t size) { set(fieldId, NXCP_DT_BINARY, value, false, size); }
   void setField(uint32_t fieldId, const InetAddress& value) { set(fieldId, NXCP_DT_INETADDR, &value); }
   void setField(uint32_t fieldId, const uuid& value) { set(fieldId, NXCP_DT_BINARY, value.getValue(), false, UUID_LENGTH); }
   void setField(uint32_t fieldId, const MacAddress& value) { set(fieldId, NXCP_DT_BINARY, value.value(), false, value.length()); }
   void setField(uint32_t fieldId, Timestamp value) { setField(fieldId, value.asMilliseconds()); }
   void setField(uint32_t fieldId, const StringList &data);
   void setField(uint32_t fieldId, const StringSet &data);

#ifdef UNICODE
   void setFieldFromMBString(uint32_t fieldId, const char *value);
#else
   void setFieldFromMBString(uint32_t fieldId, const char *value) { if (value != nullptr) set(fieldId, (m_version >= 5) ? NXCP_DT_UTF8_STRING : NXCP_DT_STRING, value); }
#endif
   void setFieldFromUtf8String(uint32_t fieldId, const char *value) { if (value != nullptr) set(fieldId, (m_version >= 5) ? NXCP_DT_UTF8_STRING : NXCP_DT_STRING, value, false, 0, true); }
   void setFieldFromTime(uint32_t fieldId, time_t value) { uint64_t t = static_cast<uint64_t>(value); set(fieldId, NXCP_DT_INT64, &t); }
   void setFieldFromInt32Array(uint32_t fieldId, size_t numElements, const uint32_t *elements);
   void setFieldFromInt32Array(uint32_t fieldId, const IntegerArray<uint32_t>& data);
   void setFieldFromInt32Array(uint32_t fieldId, const IntegerArray<uint32_t> *data) { if (data != nullptr) { setFieldFromInt32Array(fieldId, *data); } else { set(fieldId, NXCP_DT_BINARY, NULL, false, 0); } }
   void setFieldFromInt32Array(uint32_t fieldId, const HashSet<uint32_t>& data);
   void setFieldFromInt32Array(uint32_t fieldId, const HashSet<uint32_t> *data)  { if (data != nullptr) { setFieldFromInt32Array(fieldId, *data); } else { set(fieldId, NXCP_DT_BINARY, NULL, false, 0); } };
   bool setFieldFromFile(uint32_t fieldId, const TCHAR *fileName);

   int16_t getFieldAsInt16(uint32_t fieldId) const;
   uint16_t getFieldAsUInt16(uint32_t fieldId) const;
   int32_t getFieldAsInt32(uint32_t fieldId) const;
   uint32_t getFieldAsUInt32(uint32_t fieldId) const;
   int64_t getFieldAsInt64(uint32_t fieldId) const;
   uint64_t getFieldAsUInt64(uint32_t fieldId) const;
   double getFieldAsDouble(uint32_t fieldId) const;
   bool getFieldAsBoolean(uint32_t fieldId) const;
   time_t getFieldAsTime(uint32_t fieldId) const;
   Timestamp getFieldAsTimestamp(uint32_t fieldId) const;
   size_t getFieldAsInt32Array(uint32_t fieldId, size_t numElements, uint32_t *buffer) const;
   size_t getFieldAsInt32Array(uint32_t fieldId, IntegerArray<uint32_t> *data) const;
   const BYTE *getBinaryFieldPtr(uint32_t fieldId, size_t *size) const;
   TCHAR *getFieldAsString(uint32_t fieldId, MemoryPool *pool) const { return getFieldAsString(fieldId, pool, nullptr, 0); }
   TCHAR *getFieldAsString(uint32_t fieldId, TCHAR *buffer = nullptr, size_t bufferSize = 0) const { return getFieldAsString(fieldId, nullptr, buffer, bufferSize); }
   TCHAR *getFieldAsString(uint32_t fieldId, TCHAR **buffer) const { MemFree(*buffer); *buffer = getFieldAsString(fieldId, nullptr, nullptr, 0); return *buffer; }
	char *getFieldAsMBString(uint32_t fieldId, char *buffer = nullptr, size_t bufferSize = 0) const;
	char *getFieldAsUtf8String(uint32_t fieldId, char *buffer = nullptr, size_t bufferSize = 0) const;
   SharedString getFieldAsSharedString(uint32_t fieldId, size_t maxSize = 0) const;
   size_t getFieldAsBinary(uint32_t fieldId, BYTE *buffer, size_t bufferSize) const;
   InetAddress getFieldAsInetAddress(uint32_t fieldId) const;
   MacAddress getFieldAsMacAddress(uint32_t fieldId) const;
   uuid getFieldAsGUID(uint32_t fieldId) const;

   void deleteAllFields();

   void disableEncryption() { m_flags |= MF_DONT_ENCRYPT; }
   void disableCompression() { m_flags |= MF_DONT_COMPRESS; }
   void setEndOfSequence() { m_flags |= MF_END_OF_SEQUENCE; }
   void setReverseOrderFlag() { m_flags |= MF_REVERSE_ORDER; }

   static StringBuffer dump(const NXCP_MESSAGE *msg, int version);
};

/**
 * Unclaimed message in message wait queue
 */
struct WaitQueueUnclaimedMessage
{
   WaitQueueUnclaimedMessage *next;
   void *msg;
   time_t timestamp;
   uint32_t id;         // Message ID
   uint16_t code;       // Message code
   bool isBinary;       // true for binary (raw) messages
};

/**
 * Waiter in message wait queue
 */
struct WaitQueueWaiter
{
   WaitQueueWaiter *next;
   Condition wakeupCondition;
   void *msg;
   uint32_t id;         // Message ID
   uint16_t code;       // Message code
   bool isBinary;       // true for binary (raw) messages

   WaitQueueWaiter() : wakeupCondition(true)
   {
      next = nullptr;
      msg = nullptr;
      id = 0;
      code = 0;
      isBinary = 0;
   }

   WaitQueueWaiter(bool _isBinary, uint16_t _code, uint32_t _id) : wakeupCondition(true)
   {
      next = nullptr;
      msg = nullptr;
      id = _id;
      code = _code;
      isBinary = _isBinary;
   }
};

#ifdef _WIN32
template class LIBNETXMS_TEMPLATE_EXPORTABLE ObjectMemoryPool<WaitQueueUnclaimedMessage>;
template class LIBNETXMS_TEMPLATE_EXPORTABLE ObjectMemoryPool<WaitQueueWaiter>;
#endif

/**
 * Message waiting queue class
 */
class LIBNETXMS_EXPORTABLE MsgWaitQueue
{
private:
   ObjectMemoryPool<WaitQueueUnclaimedMessage> m_unclaimedMessagesPool;
   ObjectMemoryPool<WaitQueueWaiter> m_waitersPool;
   Mutex m_mutex;
   WaitQueueUnclaimedMessage *m_messagesHead;
   WaitQueueUnclaimedMessage *m_messagesTail;
   WaitQueueWaiter *m_waiters;
   uint32_t m_holdTime;
   time_t m_lastExpirationCheck;

   void put(bool isBinary, uint16_t code, uint32_t id, void *msg);
   void *waitForMessage(bool isBinary, uint16_t code, uint32_t id, uint32_t timeout);

public:
   MsgWaitQueue();
   ~MsgWaitQueue();

   void put(NXCPMessage *msg)
   {
      put(false, msg->getCode(), msg->getId(), msg);
   }
   void put(NXCP_MESSAGE *msg)
   {
      put(true, msg->code, msg->id, msg);
   }
   NXCPMessage *waitForMessage(uint16_t code, uint32_t id, uint32_t timeout)
   {
      return static_cast<NXCPMessage*>(waitForMessage(false, code, id, timeout));
   }
   NXCP_MESSAGE *waitForRawMessage(uint16_t code, uint32_t id, uint32_t timeout)
   {
      return static_cast<NXCP_MESSAGE*>(waitForMessage(true, code, id, timeout));
   }

   void clear();
   void setHoldTime(uint32_t holdTime) { m_holdTime = holdTime; }
};

/**
 * NXCP encryption context
 */
class LIBNETXMS_EXPORTABLE NXCPEncryptionContext
{
private:
	int m_cipher;
	BYTE *m_sessionKey;
	int m_keyLength;
	BYTE m_iv[EVP_MAX_IV_LENGTH];
#ifdef _WITH_ENCRYPTION
   Mutex m_encryptorLock;
   EVP_CIPHER_CTX *m_encryptor;
   EVP_CIPHER_CTX *m_decryptor;
#endif

	NXCPEncryptionContext();
   bool initCipher(int cipher);

public:
	static NXCPEncryptionContext *create(NXCPMessage *msg, RSA_KEY privateKey);
	static NXCPEncryptionContext *create(uint32_t ciphers);

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
   MSGRECV_PROTOCOL_ERROR = 5,
   MSGRECV_WANT_READ = 6,
   MSGRECV_WANT_WRITE = 7
};

#ifdef _WIN32
template class LIBNETXMS_TEMPLATE_EXPORTABLE shared_ptr<NXCPEncryptionContext>;
#endif

/**
 * Message receiver - abstract base class
 */
class LIBNETXMS_EXPORTABLE AbstractMessageReceiver
{
private:
   BYTE *m_buffer;
   BYTE *m_decryptionBuffer;
   shared_ptr<NXCPEncryptionContext> m_encryptionContext;
   size_t m_initialSize;
   size_t m_size;
   size_t m_maxSize;
   size_t m_dataSize;
   ssize_t m_bytesToSkip;

   NXCPMessage *getMessageFromBuffer(bool *protocolError, bool *decryptionError);

protected:
   virtual ssize_t readBytes(BYTE *buffer, size_t size, uint32_t timeout) = 0;

public:
   AbstractMessageReceiver(size_t initialSize, size_t maxSize);
   virtual ~AbstractMessageReceiver();

   virtual void cancel() = 0;

   void setEncryptionContext(const shared_ptr<NXCPEncryptionContext>& ctx) { m_encryptionContext = ctx; }

   NXCPMessage *readMessage(uint32_t timeout, MessageReceiverResult *result, bool allowReadBytes = true);
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
   virtual ssize_t readBytes(BYTE *buffer, size_t size, uint32_t timeout) override;

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
   shared_ptr<AbstractCommChannel> m_channel;

protected:
   virtual ssize_t readBytes(BYTE *buffer, size_t size, uint32_t timeout) override;

public:
   CommChannelMessageReceiver(const shared_ptr<AbstractCommChannel>& channel, size_t initialSize, size_t maxSize);

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
   Mutex *m_mutex;
#ifndef _WIN32
   int m_controlPipe[2];
#endif

protected:
   virtual ssize_t readBytes(BYTE *buffer, size_t size, uint32_t timeout) override;

public:
   TlsMessageReceiver(SOCKET socket, SSL *ssl, Mutex *mutex, size_t initialSize, size_t maxSize);
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
   virtual ssize_t readBytes(BYTE *buffer, size_t size, uint32_t timeout) override;

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

typedef bool (*NXCPMessageNameResolver)(uint16_t code, TCHAR *buffer);


//
// Functions
//

#ifdef __cplusplus

NXCP_MESSAGE LIBNETXMS_EXPORTABLE *CreateRawNXCPMessage(uint16_t code, uint32_t id, uint16_t flags, const void *data, size_t dataSize,
      NXCP_MESSAGE *buffer, bool allowCompression);
bool LIBNETXMS_EXPORTABLE NXCPGetPeerProtocolVersion(SOCKET s, int *pnVersion, Mutex *mutex);
bool LIBNETXMS_EXPORTABLE NXCPGetPeerProtocolVersion(const shared_ptr<AbstractCommChannel>& channel, int *pnVersion, Mutex *mutex);

bool LIBNETXMS_EXPORTABLE SendFileOverNXCP(SOCKET hSocket, uint32_t requestId, const TCHAR *fileName, NXCPEncryptionContext *ectx, off64_t offset,
         void (* progressCallback)(size_t, void *), void *cbArg, Mutex *mutex, NXCPStreamCompressionMethod compressionMethod = NXCP_STREAM_COMPRESSION_NONE,
         VolatileCounter *cancellationFlag = nullptr);
bool LIBNETXMS_EXPORTABLE SendFileOverNXCP(AbstractCommChannel *channel, uint32_t requestId, const TCHAR *fileName, NXCPEncryptionContext *ectx, off64_t offset,
         void (* progressCallback)(size_t, void *), void *cbArg, Mutex *mutex, NXCPStreamCompressionMethod compressionMethod = NXCP_STREAM_COMPRESSION_NONE,
         VolatileCounter *cancellationFlag = nullptr);
bool LIBNETXMS_EXPORTABLE SendFileOverNXCP(SOCKET hSocket, uint32_t requestId, std::istream *stream, NXCPEncryptionContext *ectx, off64_t offset,
         void (* progressCallback)(size_t, void *), void *cbArg, Mutex *mutex, NXCPStreamCompressionMethod compressionMethod = NXCP_STREAM_COMPRESSION_NONE,
         VolatileCounter *cancellationFlag = nullptr);
bool LIBNETXMS_EXPORTABLE SendFileOverNXCP(AbstractCommChannel *channel, uint32_t requestId, std::istream *stream, NXCPEncryptionContext *ectx, off64_t offset,
         void (* progressCallback)(size_t, void *), void *cbArg, Mutex *mutex, NXCPStreamCompressionMethod compressionMethod = NXCP_STREAM_COMPRESSION_NONE,
         VolatileCounter *cancellationFlag = nullptr);

TCHAR LIBNETXMS_EXPORTABLE *NXCPMessageCodeName(uint16_t vode, TCHAR *buffer);
void LIBNETXMS_EXPORTABLE NXCPRegisterMessageNameResolver(NXCPMessageNameResolver r);
void LIBNETXMS_EXPORTABLE NXCPUnregisterMessageNameResolver(NXCPMessageNameResolver r);

uint32_t LIBNETXMS_EXPORTABLE NXCPGetSupportedCiphers();
String LIBNETXMS_EXPORTABLE NXCPGetSupportedCiphersAsText();
uint32_t LIBNETXMS_EXPORTABLE SetupEncryptionContext(NXCPMessage *msg, NXCPEncryptionContext **ppCtx, NXCPMessage **ppResponse, RSA_KEY privateKey, int nNXCPVersion);
void LIBNETXMS_EXPORTABLE PrepareKeyRequestMsg(NXCPMessage *msg, RSA_KEY serverKey, bool useX509Format);

/**
 * Encrypt message
 */
static inline NXCP_ENCRYPTED_MESSAGE *NXCPEncryptMessage(NXCPEncryptionContext *ctx, NXCP_MESSAGE *msg)
{
   return (ctx != nullptr) ? ctx->encryptMessage(msg) : nullptr;
}

/**
 * Decrypt message
 */
static inline bool NXCPDecryptMessage(NXCPEncryptionContext *ctx, NXCP_ENCRYPTED_MESSAGE *msg, BYTE *pDecryptionBuffer)
{
   return (ctx != nullptr) ? ctx->decryptMessage(msg, pDecryptionBuffer) : false;
}

#endif   /* __cplusplus */

#endif   /* _nxcpapi_h_ */
