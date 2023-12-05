#include <nms_common.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <testtools.h>

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

   AssertEquals(s_waitFailures, 0);

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
