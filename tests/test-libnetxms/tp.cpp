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
   AssertEquals(info.totalRequests, _ULL(41));
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
   AssertEquals((int32_t)task->getState(), (int32_t)BackgroundTaskState::COMPLETED);
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
   AssertEquals((int32_t)task->getState(), (int32_t)BackgroundTaskState::FAILED);
   AssertEquals(task->getFailureReson(), _T("Test failure 10"));
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

const int DELAYED_EXEC_TEST_ITERATIONS = 10000;

void TestThreadPoolDelayedExecution()
{
   StartTest(_T("Thread pool - delayed execution"));
   ThreadPool *p = ThreadPoolCreate(_T("TEST2"), 64, 128, 0);

   s_delayedExecutionCounter = 0;
   for(int i = 0; i < DELAYED_EXEC_TEST_ITERATIONS; i++)
   {
      ThreadPoolScheduleRelative(p, GenerateRandomNumber(200, 700), DelayedExecutionCallback);
   }

   ThreadPoolInfo info;
   while(true)
   {
      ThreadSleepMs(10);

      ThreadPoolGetInfo(p, &info);
      if ((info.activeRequests == 0) && (info.scheduledRequests == 0))
      {
         ThreadSleepMs(10);
         break;
      }
   }

   AssertEquals(InterlockedIncrement(&s_delayedExecutionCounter), DELAYED_EXEC_TEST_ITERATIONS + 1);

   ThreadPoolGetInfo(p, &info);
   AssertEquals(info.activeRequests, 0);
   AssertEquals(info.scheduledRequests, 0);
   AssertEquals(info.totalRequests, static_cast<uint64_t>(DELAYED_EXEC_TEST_ITERATIONS));

   bool stage1 = false, stage2 = false;
   ThreadPoolScheduleRelative(p, 2000, [&stage2] () -> void { stage2 = true; });
   ThreadPoolScheduleRelative(p, 1000, [&stage1] () -> void { stage1 = true; });

   AssertFalse(stage1);
   AssertFalse(stage2);

   ThreadSleepMs(1100);
   AssertTrue(stage1);
   AssertFalse(stage2);

   ThreadSleepMs(1100);
   AssertTrue(stage1);
   AssertTrue(stage2);

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

   AssertEquals(ThreadPoolGetSerializedRequestCount(threadPool,  _T("Test1")), 2);
   AssertEquals(ThreadPoolGetSerializedRequestCount(threadPool,  _T("Test2")), 0);
   
   s_waitTimeTestLock1.unlock();
   ThreadSleepMs(100);
   AssertTrue(ThreadPoolGetSerializedRequestMaxWaitTime(threadPool,  _T("Test1")) >= 140);

   s_waitTimeTestLock2.unlock();
   ThreadSleepMs(200);
   AssertEquals(ThreadPoolGetSerializedRequestCount(threadPool,  _T("Test1")), 0);
   AssertEquals(ThreadPoolGetSerializedRequestCount(threadPool,  _T("Test2")), 0);
   AssertEquals(ThreadPoolGetSerializedRequestMaxWaitTime(threadPool, _T("Test1")), static_cast<uint32_t>(0));

   ThreadPoolDestroy(threadPool);
   EndTest();
}
