#include <nms_common.h>
#include <nms_util.h>
#include <testtools.h>

static int s_count;
static int s_increment;
static int s_val;

static void MutexWorkerThread(Mutex *m)
{
   m->lock();
   for(int i = 0; i < 100; i++)
      s_val += s_increment;
   m->unlock();
}

void TestMutex()
{
   StartTest(_T("Mutex"));
   for (int n = 0; n < 10; n++)
   {
      s_val = 0;
      srand((unsigned int)time(nullptr));
      s_count = 100 + (rand() % 100);
      s_increment = (rand() % 5 + 1) * 2;

      Mutex m;

      THREAD t[200];
      for (int i = 0; i < s_count; i++)
         t[i] = ThreadCreateEx(MutexWorkerThread, &m);
      for (int i = 0; i < s_count; i++)
         ThreadJoin(t[i]);

      AssertEquals(s_val, s_count * 100 * s_increment);
   }
   EndTest();
}

static THREAD_RESULT THREAD_CALL RWLockWrapperWorkerThread(void *arg)
{
   RWLock l = *((RWLock *)arg);
   ThreadSleepMs(rand() % 10);
   l.writeLock();
   for(int i = 0; i < 10000; i++)
      s_val = s_val + s_increment / 2;
   l.unlock();
   return THREAD_OK;
}

void TestRWLockWrapper()
{
   StartTest(_T("R/W lock wrapper"));

   for(int n = 0; n < 10; n++)
   {
      s_val = 0;
      srand((unsigned int)time(NULL));
      s_count = 100 + (rand() % 100);
      s_increment = (rand() % 5 + 1) * 2;

      RWLock l1;
      RWLock l2 = l1;

      THREAD t[200];
      l1.readLock();
      for(int i = 0; i < s_count; i++)
         t[i] = ThreadCreateEx(RWLockWrapperWorkerThread, 0, i % 2 ? &l2 : &l1);
      l2.unlock();
      for(int i = 0; i < s_count; i++)
         ThreadJoin(t[i]);

      AssertEquals(s_val, s_count * 10000 * s_increment / 2);
   }
   EndTest();
}

static VolatileCounter s_condPass = 0;

static void ConditionWorkerThread(Condition *c)
{
   if (c->wait(5000))
      InterlockedIncrement(&s_condPass);
}

static void ConditionWorkerThread2(Condition *c)
{
   ThreadSleepMs(1000);
   c->set();
}

void TestCondition()
{
   StartTest(_T("Condition"));
   Condition c(true);
   THREAD t[200];
   s_count = 100 + (rand() % 100);
   for(int i = 0; i < s_count; i++)
   {
      t[i] = ThreadCreateEx(ConditionWorkerThread, &c);
      AssertNotEquals(t[i], INVALID_THREAD_HANDLE);
   }
   c.set();
   for(int i = 0; i < s_count; i++)
   {
      AssertTrue(ThreadJoin(t[i]));
   }
   AssertEquals(s_condPass, s_count);
   EndTest();

   StartTest(_T("Condition - timeout"));
   Condition c2(true);
   THREAD th2 = ThreadCreateEx(ConditionWorkerThread2, &c2);
   AssertFalse(c2.wait(200));
   ThreadSleepMs(1000);
   AssertTrue(c2.wait(500));
   ThreadJoin(th2);
   EndTest();

   StartTest(_T("Condition - timed wait"));
   Condition c3(true);
   THREAD th3 = ThreadCreateEx(ConditionWorkerThread2, &c3);
   AssertTrue(c3.wait(2000));
   ThreadJoin(th3);
   EndTest();
}
