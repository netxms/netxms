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
static THREAD_RESULT THREAD_CALL PosterThread(void *arg)
{
   NXCPMessage *msg = new NXCPMessage();
   msg->setCode(CMD_REQUEST_COMPLETED);
   msg->setId(42);
   ThreadSleepMs((UINT32)GetCurrentTimeMs() % 2000);
   ((MsgWaitQueue *)arg)->put(msg);
   return THREAD_OK;
}

/**
 * Test message wait queue
 */
void TestMsgWaitQueue()
{
   StartTest(_T("Message wait queue"));

   MsgWaitQueue *queue = new MsgWaitQueue;
   ThreadCreate(PosterThread, 0, queue);
   NXCPMessage *msg = queue->waitForMessage(CMD_REQUEST_COMPLETED, 42, 5000);
   AssertNotNull(msg);
   delete msg;

   msg = queue->waitForMessage(CMD_REQUEST_COMPLETED, 42, 1000);
   AssertNull(msg);

   delete queue;

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

   msg.setField(1, (UINT16)1234);
   AssertTrue(msg.getFieldAsUInt16(1) == 1234);

   msg.setField(1, (UINT32)1234);
   AssertTrue(msg.getFieldAsUInt32(1) == 1234);

   msg.setField(1, (UINT64)1234);
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

   StartTest(_T("NXCP message compression performance"));
   INT64 start = GetCurrentTimeMs();
   for(int i = 0; i < 10000; i++)
   {
      NXCP_MESSAGE *binMsg = msg.serialize(true);
      MemFree(binMsg);
   }
   EndTest(GetCurrentTimeMs() - start);
}
