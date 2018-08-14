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
