#include <nms_common.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <testtools.h>

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
   AssertTrue(guid.compare(guid2) == 0);

   msg.setField(1, (UINT16)1234);
   AssertTrue(msg.getFieldAsUInt16(1) == 1234);

   msg.setField(1, (UINT32)1234);
   AssertTrue(msg.getFieldAsUInt32(1) == 1234);

   msg.setField(1, (UINT64)1234);
   AssertTrue(msg.getFieldAsUInt64(1) == 1234);

   EndTest();
}
