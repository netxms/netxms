#include <nms_common.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <testtools.h>
#include <sstream>

/**
 * NULL-safe _tcscmp()
 */
inline int safe_tcscmp(const TCHAR *s1, const TCHAR *s2)
{
   return _tcscmp(CHECK_NULL_EX(s1), CHECK_NULL_EX(s2));
}

/**
 * NULL-safe strcmp()
 */
inline int safe_strcmp(const char *s1, const char *s2)
{
   return strcmp(CHECK_NULL_EX_A(s1), CHECK_NULL_EX_A(s2));
}

/**
 * Test text
 */
static TCHAR longText[] = _T("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.");

/**
 * Poster thread
 */
static void PosterThread(MsgWaitQueue *queue)
{
   NXCPMessage *msg = new NXCPMessage(CMD_REQUEST_COMPLETED, 42);
   ThreadSleepMs(static_cast<uint32_t>(GetCurrentTimeMs() % 1000));
   queue->put(msg);
}

/**
 * Number of rounds for multi-reader test
 */
static const int ROUNDS = 100;

/**
 * Wait failures
 */
static VolatileCounter s_waitFailures;

static Mutex s_waiterLock;

/**
 * Waiter thread for multi-reader test
 */
static void WaiterThread(MsgWaitQueue *queue, uint32_t id)
{
   for(int i = 0; i < ROUNDS; i++)
   {
      s_waiterLock.lock();
      ThreadSleepMs(0);
      s_waiterLock.unlock();
      NXCPMessage *msg = queue->waitForMessage(CMD_REQUEST_COMPLETED, id + i, 1000);
      if (msg == nullptr)
      {
         InterlockedIncrement(&s_waitFailures);
      }
      delete msg;
   }
}

/**
 * Test message wait queue
 */
void TestMsgWaitQueue()
{
   StartTest(_T("Message wait queue - basic functions"));

   MsgWaitQueue queue;

   queue.put(new NXCPMessage(CMD_REQUEST_COMPLETED, 21));
   NXCPMessage *msg = queue.waitForMessage(CMD_REQUEST_COMPLETED, 21, 100);
   AssertNotNull(msg);
   delete msg;

   ThreadCreate(PosterThread, &queue);
   msg = queue.waitForMessage(CMD_REQUEST_COMPLETED, 42, 2000);
   AssertNotNull(msg);
   delete msg;

   msg = queue.waitForMessage(CMD_REQUEST_COMPLETED, 42, 1000);
   AssertNull(msg);

   EndTest();

   StartTest(_T("Message wait queue - multiple readers"));

   s_waitFailures = 0;

   const uint32_t MAX_THREADS = 300;
   THREAD threads[MAX_THREADS];
   for(uint32_t i = 0; i < MAX_THREADS; i++)
      threads[i] = ThreadCreateEx(WaiterThread, &queue, i * 1000);

   for(int n = 0; n < ROUNDS; n++)
   {
      for(uint32_t i = 0; i < MAX_THREADS; i++)
      {
         NXCPMessage *msg = new NXCPMessage(CMD_REQUEST_COMPLETED, i * 1000 + n);
         queue.put(msg);
      }
      ThreadSleepMs(static_cast<uint32_t>(GetCurrentTimeMs() % 50));
   }

   for(uint32_t i = 0; i < MAX_THREADS; i++)
      ThreadJoin(threads[i]);

   AssertEquals(static_cast<int32_t>(s_waitFailures), 0);

   EndTest();
}

/**
 * Set uncompressed size declared by compressed structured message
 */
static void SetDeclaredSize(NXCP_MESSAGE *msg, uint32_t size)
{
   size = htonl(size);
   memcpy(reinterpret_cast<BYTE*>(msg) + NXCP_HEADER_SIZE, &size, 4);
}

/**
 * Get uncompressed size declared by compressed structured message
 */
static uint32_t GetDeclaredSize(const NXCP_MESSAGE *msg)
{
   uint32_t size;
   memcpy(&size, reinterpret_cast<const BYTE*>(msg) + NXCP_HEADER_SIZE, 4);
   return ntohl(size);
}

/**
 * Check that compressed message is rejected by deserialize()
 */
static void AssertRejected(const NXCP_MESSAGE *msg)
{
   NXCPMessage *parsed = NXCPMessage::deserialize(msg);
   AssertNull(parsed);
}

/**
 * Test validation of uncompressed size declared by compressed messages
 */
static void TestCompressedMessageValidation()
{
   static const uint32_t HUGE_SIZE = 0x10000000;
   static const size_t ZERO_BLOCK_SIZE = 1024 * 1024;

   StartTest(_T("NXCP message compression - declared size of structured message"));

   NXCPMessage msg(CMD_REQUEST_COMPLETED, 1);
   msg.setField(100, longText);
   NXCP_MESSAGE *binMsg = msg.serialize(true);
   AssertTrue((ntohs(binMsg->flags) & MF_COMPRESSED) != 0);
   uint32_t declaredSize = GetDeclaredSize(binMsg);
   AssertFalse(NXCPMessage::dump(binMsg, NXCP_VERSION).contains(_T("format error")));

   SetDeclaredSize(binMsg, HUGE_SIZE);
   AssertRejected(binMsg);
   AssertTrue(NXCPMessage::dump(binMsg, NXCP_VERSION).contains(_T("format error")));

   binMsg->flags |= htons(MF_STREAM);
   AssertRejected(binMsg);
   AssertTrue(NXCPMessage::dump(binMsg, NXCP_VERSION).contains(_T("format error")));
   binMsg->flags &= ~htons(MF_STREAM);

   SetDeclaredSize(binMsg, declaredSize + 8);
   AssertRejected(binMsg);
   AssertTrue(NXCPMessage::dump(binMsg, NXCP_VERSION).contains(_T("Cannot decompress message")));

   SetDeclaredSize(binMsg, declaredSize - 8);
   AssertRejected(binMsg);
   AssertTrue(NXCPMessage::dump(binMsg, NXCP_VERSION).contains(_T("Cannot decompress message")));

   SetDeclaredSize(binMsg, declaredSize);
   NXCPMessage *parsed = NXCPMessage::deserialize(binMsg);
   AssertNotNull(parsed);
   delete parsed;
   MemFree(binMsg);

   EndTest();

   StartTest(_T("NXCP message compression - declared size above message size limit"));

   // Compressed payload is long enough for declared size to pass compression ratio check
   static const size_t LARGE_MESSAGE_SIZE = 1400000;
   static const uint32_t OVERSIZE = 0x50000000;
   AssertTrue(static_cast<uint64_t>(LARGE_MESSAGE_SIZE - NXCP_HEADER_SIZE - 4) * 1032 > OVERSIZE);
   binMsg = static_cast<NXCP_MESSAGE*>(MemAllocZeroed(LARGE_MESSAGE_SIZE));
   binMsg->code = htons(CMD_REQUEST_COMPLETED);
   binMsg->flags = htons(MF_COMPRESSED | MF_NXCP_VERSION(NXCP_VERSION));
   binMsg->size = htonl(static_cast<uint32_t>(LARGE_MESSAGE_SIZE));
   SetDeclaredSize(binMsg, OVERSIZE);
   AssertRejected(binMsg);
   AssertTrue(NXCPMessage::dump(binMsg, NXCP_VERSION).contains(_T("format error")));

   binMsg->flags = htons(MF_BINARY | MF_COMPRESSED | MF_NXCP_VERSION(NXCP_VERSION));
   binMsg->numFields = htonl(OVERSIZE);
   AssertRejected(binMsg);
   MemFree(binMsg);

   EndTest();

   StartTest(_T("NXCP message compression - declared size of binary message"));

   char text[1024];
   memset(text, 'A', sizeof(text));
   binMsg = CreateRawNXCPMessage(CMD_FILE_DATA, 1, 0, text, sizeof(text), nullptr, true);
   AssertTrue((ntohs(binMsg->flags) & MF_COMPRESSED) != 0);

   binMsg->numFields = htonl(HUGE_SIZE);
   AssertRejected(binMsg);

   binMsg->numFields = htonl(sizeof(text) + 1);
   AssertRejected(binMsg);

   binMsg->numFields = htonl(sizeof(text) - 1);
   AssertRejected(binMsg);

   // Java sender does not put size of binary data into first four bytes of payload
   binMsg->numFields = htonl(sizeof(text));
   SetDeclaredSize(binMsg, NXCP_HEADER_SIZE + 7);
   parsed = NXCPMessage::deserialize(binMsg);
   AssertNotNull(parsed);
   AssertEquals(parsed->getBinaryDataSize(), sizeof(text));
   AssertTrue(!memcmp(parsed->getBinaryData(), text, sizeof(text)));
   delete parsed;
   MemFree(binMsg);

   EndTest();

   StartTest(_T("NXCP message compression - highest compression ratio"));

   BYTE *zeroes = MemAllocArray<BYTE>(ZERO_BLOCK_SIZE);

   NXCPMessage zmsg(CMD_REQUEST_COMPLETED, 2);
   zmsg.setField(1, zeroes, ZERO_BLOCK_SIZE);
   binMsg = zmsg.serialize(true);
   AssertTrue((ntohs(binMsg->flags) & MF_COMPRESSED) != 0);
   AssertTrue(ZERO_BLOCK_SIZE / (ntohl(binMsg->size) - NXCP_HEADER_SIZE - 4) > 900);
   parsed = NXCPMessage::deserialize(binMsg);
   AssertNotNull(parsed);
   size_t size;
   const BYTE *data = parsed->getBinaryFieldPtr(1, &size);
   AssertEquals(size, ZERO_BLOCK_SIZE);
   AssertTrue(!memcmp(data, zeroes, ZERO_BLOCK_SIZE));
   delete parsed;
   AssertFalse(NXCPMessage::dump(binMsg, NXCP_VERSION).contains(_T("format error")));
   MemFree(binMsg);

   binMsg = CreateRawNXCPMessage(CMD_FILE_DATA, 2, 0, zeroes, ZERO_BLOCK_SIZE, nullptr, true);
   AssertTrue((ntohs(binMsg->flags) & MF_COMPRESSED) != 0);
   AssertTrue(ZERO_BLOCK_SIZE / (ntohl(binMsg->size) - NXCP_HEADER_SIZE - 4) > 900);
   parsed = NXCPMessage::deserialize(binMsg);
   AssertNotNull(parsed);
   AssertEquals(parsed->getBinaryDataSize(), ZERO_BLOCK_SIZE);
   AssertTrue(!memcmp(parsed->getBinaryData(), zeroes, ZERO_BLOCK_SIZE));
   delete parsed;
   MemFree(binMsg);

   MemFree(zeroes);

   EndTest();
}

/**
 * Test message class
 */
void TestMessageClass()
{
   StartTest(_T("NXCPMessage class"));

   NXCPMessage msg;

   uuid guid = uuid::generate();
   msg.setField(1, guid);
   uuid guid2 = msg.getFieldAsGUID(1);
   AssertTrue(guid.equals(guid2));

   msg.setField(1, static_cast<uint16_t>(1234));
   AssertTrue(msg.getFieldAsUInt16(1) == 1234);

   msg.setField(1, static_cast<uint32_t>(1234));
   AssertTrue(msg.getFieldAsUInt32(1) == 1234);

   msg.setField(1, static_cast<uint64_t>(1234));
   AssertTrue(msg.getFieldAsUInt64(1) == 1234);

   msg.setField(1, _T("test text"));
   TCHAR buffer[64];
   char buffer2[64];
   AssertTrue(!safe_tcscmp(msg.getFieldAsString(1, buffer, 64), _T("test text")));
   AssertTrue(!safe_strcmp(msg.getFieldAsMBString(1, buffer2, 64), "test text"));
   AssertTrue(!safe_strcmp(msg.getFieldAsUtf8String(1, buffer2, 64), "test text"));

   msg.setFieldFromUtf8String(1, "test text 2");
   AssertTrue(!safe_tcscmp(msg.getFieldAsString(1, buffer, 64), _T("test text 2")));
   AssertTrue(!safe_strcmp(msg.getFieldAsMBString(1, buffer2, 64), "test text 2"));
   AssertTrue(!safe_strcmp(msg.getFieldAsUtf8String(1, buffer2, 64), "test text 2"));

   msg.setFieldFromMBString(12, "test text 3");
   AssertTrue(!safe_tcscmp(msg.getFieldAsString(12, buffer, 64), _T("test text 3")));
   AssertTrue(!safe_strcmp(msg.getFieldAsMBString(12, buffer2, 64), "test text 3"));
   AssertTrue(!safe_strcmp(msg.getFieldAsUtf8String(12, buffer2, 64), "test text 3"));

   // getFieldAsString / getFieldAsUtf8String with Buffer (BufferImpl) argument.
   // At default protocol version (>= 5) string fields are stored as NXCP_DT_UTF8_STRING.
   {
      static const TCHAR shortText[] = _T("short");
      // Long enough to overflow the inline storage of a default-size Buffer (inline
      // capacity is 32 bytes -> 16 WCHARs / 32 chars), forcing heap promotion.
      static const TCHAR longBufText[] = _T("this string is definitely longer than the inline buffer capacity");
      static const char *longBufTextUtf8 = "this string is definitely longer than the inline buffer capacity";

      msg.setField(200, shortText);
      msg.setField(201, longBufText);

      Buffer<TCHAR> tbuf;
      AssertTrue(!safe_tcscmp(msg.getFieldAsString(200, tbuf), _T("short")));
      AssertTrue(tbuf.isInternal());
      AssertTrue(!safe_tcscmp(msg.getFieldAsString(201, tbuf), longBufText));
      AssertTrue(!tbuf.isInternal());

      Buffer<char> cbuf;
      AssertTrue(!safe_strcmp(msg.getFieldAsUtf8String(200, cbuf), "short"));
      AssertTrue(cbuf.isInternal());
      AssertTrue(!safe_strcmp(msg.getFieldAsUtf8String(201, cbuf), longBufTextUtf8));
      AssertTrue(!cbuf.isInternal());

      // Missing field -> nullptr returned, buffer left usable
      Buffer<TCHAR> tmissing;
      AssertNull(msg.getFieldAsString(999, tmissing));
      Buffer<char> cmissing;
      AssertNull(msg.getFieldAsUtf8String(999, cmissing));

      // Buffer object reused across calls must be reset, not appended to
      Buffer<char> reuse;
      msg.getFieldAsUtf8String(201, reuse);
      AssertTrue(!safe_strcmp(msg.getFieldAsUtf8String(200, reuse), "short"));
      Buffer<TCHAR> treuse;
      msg.getFieldAsString(201, treuse);
      AssertTrue(!safe_tcscmp(msg.getFieldAsString(200, treuse), _T("short")));

      // Same fields after downgrade to protocol version 4 are stored as NXCP_DT_STRING
      msg.setProtocolVersion(4);
      Buffer<TCHAR> tbuf2;
      AssertTrue(!safe_tcscmp(msg.getFieldAsString(200, tbuf2), _T("short")));
      AssertTrue(!safe_tcscmp(msg.getFieldAsString(201, tbuf2), longBufText));
      Buffer<char> cbuf2;
      AssertTrue(!safe_strcmp(msg.getFieldAsUtf8String(200, cbuf2), "short"));
      AssertTrue(!safe_strcmp(msg.getFieldAsUtf8String(201, cbuf2), longBufTextUtf8));

      // Allocating variant (buffer == nullptr) on NXCP_DT_STRING fields must not truncate
      char *allocated = msg.getFieldAsUtf8String(200);
      AssertNotNull(allocated);
      AssertTrue(!strcmp(allocated, "short"));
      MemFree(allocated);
      allocated = msg.getFieldAsUtf8String(201);
      AssertNotNull(allocated);
      AssertTrue(!strcmp(allocated, longBufTextUtf8));
      MemFree(allocated);

      msg.setProtocolVersion(NXCP_VERSION);
   }

   // Test JSON serialization
   json_t *json = json_object();
   json_object_set_new(json, "key1", json_string("value1"));
   json_object_set_new(json, "key2", json_integer(42));
   msg.setField(100, json);
   json_decref(json);
   json_t *jsonOut = msg.getFieldAsJson(100);
   AssertNotNull(jsonOut);
   const char *s = json_object_get_string_utf8(jsonOut, "key1", nullptr);
   AssertNotNull(s);
   AssertTrue(!strcmp(s, "value1"));
   int i = json_object_get_int32(jsonOut, "key2");
   AssertTrue(i == 42);
   json_decref(jsonOut);

   // Test UTF8-STRING to STRING conversion on protocol version change
   msg.setProtocolVersion(4);
   AssertTrue(!safe_tcscmp(msg.getFieldAsString(1, buffer, 64), _T("test text 2")));
   AssertTrue(!safe_tcscmp(msg.getFieldAsString(12, buffer, 64), _T("test text 3")));
   AssertTrue(!safe_strcmp(msg.getFieldAsMBString(12, buffer2, 64), "test text 3"));
   AssertTrue(!safe_strcmp(msg.getFieldAsUtf8String(12, buffer2, 64), "test text 3"));

   EndTest();

   StartTest(_T("NXCP message compression"));

   msg.setField(100, longText);
   NXCP_MESSAGE *binMsg = msg.serialize(true);
   AssertNotNull(binMsg);
   AssertTrue((ntohs(binMsg->flags) & MF_COMPRESSED) != 0);

   NXCPMessage *dmsg = NXCPMessage::deserialize(binMsg);
   AssertNotNull(dmsg);
   TCHAR *longTextOut = dmsg->getFieldAsString(100);
   AssertNotNull(longTextOut);
   AssertTrue(!_tcscmp(longTextOut, longText));
   MemFree(longTextOut);
   delete dmsg;
   MemFree(binMsg);

   EndTest();

   TestCompressedMessageValidation();

#if !WITH_ADDRESS_SANITIZER
   StartTest(_T("NXCP message compression performance"));
   INT64 start = GetCurrentTimeMs();
   for(int i = 0; i < 10000; i++)
   {
      // We should call deleteAllFields() because NXCPMessage
      // uses internal allocate-only memory pool. Normally it
      // is not a problem because messages usually serialized only once.
      // In this case deleteAllFields() will clean internal memory pool
      // so it will not consume too much memory.
      msg.deleteAllFields();
      msg.setField(100, longText);
      NXCP_MESSAGE *binMsg = msg.serialize(true);
      MemFree(binMsg);
   }
   EndTest(GetCurrentTimeMs() - start);
#endif
}

/**
 * Test code for custom message name resolver
 */
#define TEST_CUSTOM_CODE   static_cast<uint16_t>(0x0500)
#define TEST_CUSTOM_NAME   _T("CMD_TEST_CUSTOM_RESOLVER")

/**
 * Custom message name resolver used by TestNXCPMessageCodeName()
 */
static bool TestMessageNameResolver(uint16_t code, TCHAR *buffer)
{
   if (code == TEST_CUSTOM_CODE)
   {
      _tcscpy(buffer, TEST_CUSTOM_NAME);
      return true;
   }
   return false;
}

/**
 * Test NXCP message code to name resolution
 */
void TestNXCPMessageCodeName()
{
   TCHAR buffer[128];

   // Codes spanning the built-in table. A misaligned entry (e.g. a missing comma
   // merging two adjacent string literals into one) would shift every subsequent
   // name and fail here.
   StartTest(_T("NXCP message code name - built-in table"));
   AssertEquals(NXCPMessageCodeName(CMD_LOGIN, buffer), _T("CMD_LOGIN"));
   AssertEquals(NXCPMessageCodeName(CMD_KEEPALIVE, buffer), _T("CMD_KEEPALIVE"));
   AssertEquals(NXCPMessageCodeName(CMD_REQUEST_COMPLETED, buffer), _T("CMD_REQUEST_COMPLETED"));
   AssertEquals(NXCPMessageCodeName(CMD_RESTORE_DEVICE_CONFIG, buffer), _T("CMD_RESTORE_DEVICE_CONFIG"));
   AssertEquals(NXCPMessageCodeName(CMD_GET_TRAFFIC_CONNECTORS, buffer), _T("CMD_GET_TRAFFIC_CONNECTORS"));
   AssertEquals(NXCPMessageCodeName(CMD_QUERY_TRAFFIC_DATA, buffer), _T("CMD_QUERY_TRAFFIC_DATA"));
   AssertEquals(NXCPMessageCodeName(CMD_GET_AGENT_EFFECTIVE_CONFIG, buffer), _T("CMD_GET_AGENT_EFFECTIVE_CONFIG"));
   EndTest();

   StartTest(_T("NXCP message code name - reporting server table"));
   AssertEquals(NXCPMessageCodeName(CMD_RS_LIST_REPORTS, buffer), _T("CMD_RS_LIST_REPORTS"));
   AssertEquals(NXCPMessageCodeName(CMD_RS_DEPLOY_REPORT_PACKAGE, buffer), _T("CMD_RS_DEPLOY_REPORT_PACKAGE"));
   EndTest();

   StartTest(_T("NXCP message code name - unknown code fallback"));
   AssertEquals(NXCPMessageCodeName(TEST_CUSTOM_CODE, buffer), _T("CMD_0x0500"));
   EndTest();

   StartTest(_T("NXCP message code name - custom resolver"));
   NXCPRegisterMessageNameResolver(TestMessageNameResolver);
   AssertEquals(NXCPMessageCodeName(TEST_CUSTOM_CODE, buffer), TEST_CUSTOM_NAME);
   // Codes the custom resolver does not handle must still fall back
   AssertEquals(NXCPMessageCodeName(static_cast<uint16_t>(0x0501), buffer), _T("CMD_0x0501"));
   // Built-in table must take precedence over custom resolvers
   AssertEquals(NXCPMessageCodeName(CMD_LOGIN, buffer), _T("CMD_LOGIN"));
   NXCPUnregisterMessageNameResolver(TestMessageNameResolver);
   // After unregister the code is unknown again
   AssertEquals(NXCPMessageCodeName(TEST_CUSTOM_CODE, buffer), _T("CMD_0x0500"));
   EndTest();
}

/**
 * Communication channel that counts sent messages and fails every send when requested
 */
class TestCommChannel : public AbstractCommChannel
{
private:
   bool m_failSend;

public:
   int sendCount;

   TestCommChannel(bool failSend) : AbstractCommChannel()
   {
      m_failSend = failSend;
      sendCount = 0;
   }

   virtual ssize_t send(const void *data, size_t size, Mutex *mutex) override
   {
      sendCount++;
      return m_failSend ? -1 : static_cast<ssize_t>(size);
   }
   virtual ssize_t recv(void *buffer, size_t size, uint32_t timeout) override { return -1; }
   virtual int poll(uint32_t timeout, bool write) override { return -1; }
   virtual void backgroundPoll(uint32_t timeout, void (*callback)(BackgroundSocketPollResult, AbstractCommChannel*, void*), void *context) override { }
   virtual int shutdown() override { return 0; }
   virtual void close() override { }
};

/**
 * Test file sending over encrypted NXCP channel
 */
void TestSendFileOverEncryptedChannel()
{
   std::string content(256 * 1024, 'x');
   NXCPEncryptionContext *ectx = NXCPEncryptionContext::create(NXCP_SUPPORT_AES_256);

   StartTest(_T("SendFileOverNXCP - encrypted channel"));
   AssertNotNull(ectx);
   std::istringstream okStream(content);
   TestCommChannel okChannel(false);
   AssertTrue(SendFileOverNXCP(&okChannel, 1, &okStream, ectx, 0, nullptr, nullptr, nullptr, NXCP_STREAM_COMPRESSION_NONE, nullptr, 0));
   AssertTrue(okChannel.sendCount > 2);
   EndTest();

   StartTest(_T("SendFileOverNXCP - encrypted channel send failure"));
   std::istringstream failStream(content);
   TestCommChannel failChannel(true);
   AssertFalse(SendFileOverNXCP(&failChannel, 1, &failStream, ectx, 0, nullptr, nullptr, nullptr, NXCP_STREAM_COMPRESSION_NONE, nullptr, 0));
   AssertEquals(failChannel.sendCount, 2);  // first data block and abort
   EndTest();

   StartTest(_T("NXCP encryption - inner message size validation"));
   BYTE decryptionBuffer[1024];
   NXCPMessage validMsg(CMD_KEEPALIVE, 42);
   validMsg.setField(1, _T("test"));
   NXCP_MESSAGE *rawMsg = validMsg.serialize();
   NXCP_ENCRYPTED_MESSAGE *emsg = ectx->encryptMessage(rawMsg);
   AssertNotNull(emsg);
   AssertTrue(ectx->decryptMessage(emsg, decryptionBuffer));
   NXCPMessage *decodedMsg = NXCPMessage::deserialize(reinterpret_cast<NXCP_MESSAGE*>(emsg));
   AssertNotNull(decodedMsg);
   AssertEquals(static_cast<int>(decodedMsg->getCode()), static_cast<int>(CMD_KEEPALIVE));
   AssertEquals(static_cast<int>(decodedMsg->getId()), 42);
   delete decodedMsg;
   MemFree(emsg);
   MemFree(rawMsg);

   // Inner message with matching checksum and size that is below header size or not aligned
   static const uint32_t invalidSizes[] = { 0, 8, 12, 20 };
   for(int i = 0; i < 4; i++)
   {
      BYTE source[64];
      memset(source, 0, sizeof(source));
      NXCP_MESSAGE *badMsg = reinterpret_cast<NXCP_MESSAGE*>(source);
      badMsg->code = htons(CMD_KEEPALIVE);
      badMsg->size = htonl(invalidSizes[i]);
      badMsg->numFields = htonl(0x7FFFFFFF);
      emsg = ectx->encryptMessage(badMsg);
      AssertNotNull(emsg);
      AssertFalse(ectx->decryptMessage(emsg, decryptionBuffer));
      MemFree(emsg);
   }

   // Envelope with padding larger than encrypted data
   rawMsg = validMsg.serialize();
   emsg = ectx->encryptMessage(rawMsg);
   AssertNotNull(emsg);
   emsg->padding = 255;
   AssertFalse(ectx->decryptMessage(emsg, decryptionBuffer));
   MemFree(emsg);
   MemFree(rawMsg);

   // Envelope with valid header, message and checksum, but invalid PKCS padding. Last cipher block
   // contains only padding, so all other checks pass and only padding validation can reject it.
   rawMsg = validMsg.serialize();
   uint32_t rawMsgSize = ntohl(rawMsg->size);
   BYTE plaintext[512];
   memset(plaintext, 0, sizeof(plaintext));
   NXCP_ENCRYPTED_PAYLOAD_HEADER *payloadHeader = reinterpret_cast<NXCP_ENCRYPTED_PAYLOAD_HEADER*>(plaintext);
   payloadHeader->dwChecksum = htonl(CalculateCRC32(reinterpret_cast<BYTE*>(rawMsg), rawMsgSize, 0));
   memcpy(plaintext + NXCP_EH_ENCRYPTED_BYTES, rawMsg, rawMsgSize);
   size_t plaintextSize = NXCP_EH_ENCRYPTED_BYTES + rawMsgSize;
   plaintextSize += (16 - plaintextSize % 16) % 16 + 16;  // Fill last data block and add padding block of zeroes (invalid PKCS padding)
   AssertTrue(plaintextSize <= sizeof(plaintext));
   emsg = static_cast<NXCP_ENCRYPTED_MESSAGE*>(MemAllocZeroed(NXCP_EH_UNENCRYPTED_BYTES + plaintextSize));
   emsg->code = htons(CMD_ENCRYPTED_MESSAGE);
   emsg->size = htonl(static_cast<uint32_t>(NXCP_EH_UNENCRYPTED_BYTES + plaintextSize));
   EVP_CIPHER_CTX *encryptor = EVP_CIPHER_CTX_new();
   AssertTrue(EVP_EncryptInit_ex(encryptor, EVP_aes_256_cbc(), nullptr, ectx->getSessionKey(), ectx->getIV()) == 1);
   EVP_CIPHER_CTX_set_padding(encryptor, 0);
   int outSize;
   AssertTrue(EVP_EncryptUpdate(encryptor, emsg->data, &outSize, plaintext, static_cast<int>(plaintextSize)) == 1);
   AssertEquals(outSize, static_cast<int>(plaintextSize));
   AssertTrue(EVP_EncryptFinal_ex(encryptor, emsg->data + outSize, &outSize) == 1);
   AssertEquals(outSize, 0);
   EVP_CIPHER_CTX_free(encryptor);
   AssertFalse(ectx->decryptMessage(emsg, decryptionBuffer));
   MemFree(emsg);
   MemFree(rawMsg);

   // Parser rejects message with size below header size
   NXCP_MESSAGE shortMsg;
   memset(&shortMsg, 0, sizeof(shortMsg));
   shortMsg.code = htons(CMD_KEEPALIVE);
   shortMsg.size = htonl(8);
   shortMsg.numFields = htonl(0x7FFFFFFF);
   AssertNull(NXCPMessage::deserialize(&shortMsg));
   EndTest();

   delete ectx;
}
