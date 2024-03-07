#include <nms_common.h>
#include <nms_util.h>
#include <nxtask.h>
#include <testtools.h>

static void EmptyWorkload()
{
}

static void SlowWorkload()
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
   ThreadPoolExecute(p, EmptyWorkload);
   ThreadSleepMs(500);
   ThreadPoolGetInfo(p, &info);
   AssertEquals(info.curThreads, 4);
   AssertEquals(info.activeRequests, 0);
   EndTest();

   StartTest(_T("Thread pool - high load"));
   for(int i = 0; i < 40; i++)
   {
      ThreadPoolExecute(p, SlowWorkload);
   }
   ThreadSleepMs(2000);
   ThreadPoolGetInfo(p, &info);
   AssertTrue(info.activeRequests > 0);
   AssertEquals(info.totalRequests, 41);
   AssertTrue(info.waitTimeEMA > 0);
   EndTest();

   StartTest(_T("Thread pool - background tasks"));
   SetBackgroundTaskRetentionTime(5);

   bool completed = false;
   shared_ptr<BackgroundTask> task = CreateBackgroundTask(p,
      [&completed] (BackgroundTask *task) -> bool
      {
         completed = true;
         return true;
      });
   AssertTrue(task->waitForCompletion());
   AssertTrue(completed);
   AssertTrue(task->isFinished());
   AssertFalse(task->isFailed());
   AssertEquals(task->getState(), BackgroundTaskState::COMPLETED);
   uint64_t id = task->getId();
   task.reset();
   AssertNotNull(GetBackgroundTask(id));
   ThreadSleep(10);
   AssertNull(GetBackgroundTask(id));

   completed = false;
   task = CreateBackgroundTask(p,
      [&completed] (BackgroundTask *task) -> bool
      {
         completed = true;
         return task->failure(_T("Test failure %d"), 10);
      });
   AssertTrue(task->waitForCompletion());
   AssertTrue(completed);
   AssertTrue(task->isFinished());
   AssertTrue(task->isFailed());
   AssertEquals(task->getState(), BackgroundTaskState::FAILED);
   AssertTrue(!_tcscmp(task->getFailureReson(), _T("Test failure 10")));
   id = task->getId();
   task.reset();
   AssertNotNull(GetBackgroundTask(id));
   ThreadSleep(10);
   AssertNull(GetBackgroundTask(id));

   EndTest();

   StartTest(_T("Thread pool - destroy"));
   ThreadPoolDestroy(p);
   EndTest();
}

static VolatileCounter s_delayedExecutionCounter;

static void DelayedExecutionCallback()
{
   InterlockedIncrement(&s_delayedExecutionCounter);
}

void TestThreadPoolDelayedExecution()
{
   StartTest(_T("Thread pool - delayed execution"));
   ThreadPool *p = ThreadPoolCreate(_T("TEST2"), 16, 32, 0);

   s_delayedExecutionCounter = 0;
   for(int i = 0; i < 10000; i++)
   {
      ThreadPoolScheduleRelative(p, GenerateRandomNumber(200, 700), DelayedExecutionCallback);
   }

   ThreadPoolInfo info;
   while(true)
   {
      ThreadPoolGetInfo(p, &info);
      if ((info.activeRequests == 0) && (info.scheduledRequests == 0))
         break;
      ThreadSleepMs(10);
   }

   AssertEquals(s_delayedExecutionCounter, 10000);

   ThreadPoolDestroy(p);
   EndTest();
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
