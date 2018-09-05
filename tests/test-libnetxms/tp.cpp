#include <nms_common.h>
#include <nms_util.h>
#include <testtools.h>

static void EmptyWorkload(void *arg)
{
}

static void SlowWorkload(void *arg)
{
   ThreadSleepMs(1000);
}

void TestThreadPool()
{
   StartTest(_T("Thread pool - create"));
   ThreadPool *p = ThreadPoolCreate(_T("TEST"), 4, 32, 0);
   AssertNotNull(p);
   EndTest();

   StartTest(_T("Thread pool - get info"));
   ThreadPoolInfo info;
   ThreadPoolGetInfo(p, &info);
   AssertEquals(info.curThreads, 4);
   AssertEquals(info.minThreads, 4);
   AssertEquals(info.maxThreads, 32);
   EndTest();

   StartTest(_T("Thread pool - low load"));
   ThreadPoolExecute(p, EmptyWorkload, NULL);
   ThreadSleepMs(500);
   ThreadPoolGetInfo(p, &info);
   AssertEquals(info.curThreads, 4);
   AssertEquals(info.activeRequests, 0);
   EndTest();

   StartTest(_T("Thread pool - high load"));
   for(int i = 0; i < 40; i++)
   {
      ThreadPoolExecute(p, SlowWorkload, NULL);
   }
   ThreadSleepMs(2000);
   ThreadPoolGetInfo(p, &info);
   AssertTrue(info.activeRequests > 0);
   AssertEquals(info.totalRequests, 41);
   AssertTrue(info.averageWaitTime > 0);
   EndTest();

   ThreadPoolDestroy(p);
}

static Mutex s_waitTimeTestLock1;
static Mutex s_waitTimeTestLock2;

static void CountAndMaxWaitThread(void *arg)
{
   static_cast<Mutex*>(arg)->lock();
   ThreadSleepMs(20);
   static_cast<Mutex*>(arg)->unlock();
}

void TestThreadCountAndMaxWaitTime()
{
   StartTest(_T("Thread pool - serialized count and max wait time"));
   ThreadPool *threadPool = ThreadPoolCreate(_T("MAIN"), 8, 256);    
   
   s_waitTimeTestLock1.lock();
   s_waitTimeTestLock2.lock();
   
   ThreadPoolExecuteSerialized(threadPool, _T("Test1"), CountAndMaxWaitThread, &s_waitTimeTestLock1);
   ThreadPoolExecuteSerialized(threadPool, _T("Test2"), CountAndMaxWaitThread, &s_waitTimeTestLock1);
   ThreadPoolExecuteSerialized(threadPool, _T("Test1"), CountAndMaxWaitThread, &s_waitTimeTestLock1);
   ThreadPoolExecuteSerialized(threadPool, _T("Test1"), CountAndMaxWaitThread, &s_waitTimeTestLock2);

   ThreadSleepMs(100);  // yield CPU
   
   AssertEquals(2, ThreadPoolGetSerializedRequestCount(threadPool,  _T("Test1")));
   AssertEquals(0, ThreadPoolGetSerializedRequestCount(threadPool,  _T("Test2")));
   AssertEquals(0, ThreadPoolGetSerializedRequestMaxWaitTime(threadPool, _T("Test1")));
   
   s_waitTimeTestLock1.unlock();
   ThreadSleepMs(100);
   AssertTrue(ThreadPoolGetSerializedRequestMaxWaitTime(threadPool,  _T("Test1")) >= 140);

   s_waitTimeTestLock2.unlock();
   ThreadSleepMs(200);
   AssertEquals(0, ThreadPoolGetSerializedRequestCount(threadPool,  _T("Test1")));
   AssertEquals(0, ThreadPoolGetSerializedRequestCount(threadPool,  _T("Test2")));
   AssertEquals(0, ThreadPoolGetSerializedRequestMaxWaitTime(threadPool, _T("Test1")));
    
   ThreadPoolDestroy(threadPool);
   EndTest();
}
