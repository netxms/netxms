#include <nms_common.h>
#include <nms_util.h>
#include <testtools.h>

static int s_count;
static int s_increment;
static int s_val;

static THREAD_RESULT THREAD_CALL MutexWrapperWorkerThread(void *arg)
{
   Mutex m = *((Mutex *)arg);
   ThreadSleepMs(rand() % 10);
   m.lock();
   for(int i = 0; i < 10000; i++)
      s_val = s_val + s_increment / 2;
   m.unlock();
   return THREAD_OK;
}

void TestMutexWrapper()
{
   StartTest(_T("Mutex wrapper"));

   for(int n = 0; n < 10; n++)
   {
      s_val = 0;
      srand((unsigned int)time(NULL));
      s_count = 100 + (rand() % 100);
      s_increment = (rand() % 5 + 1) * 2;

      Mutex m1;
      Mutex m2 = m1;

      THREAD t[200];
      m1.lock();
      for(int i = 0; i < s_count; i++)
         t[i] = ThreadCreateEx(MutexWrapperWorkerThread, 0, i % 2 ? &m2 : &m1);
      m2.unlock();
      for(int i = 0; i < s_count; i++)
         ThreadJoin(t[i]);

      AssertEquals(s_val, s_count * 10000 * s_increment / 2);
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

static THREAD_RESULT THREAD_CALL ConditionWrapperWorkerThread(void *arg)
{
   Condition c = *((Condition *)arg);
   if (c.wait(5000))
      InterlockedIncrement(&s_condPass);
   return THREAD_OK;
}

void TestConditionWrapper()
{
   StartTest(_T("Condition wrapper"));
   Condition c1(true);
   Condition c2(c1);
   THREAD t[200];
   s_count = 100 + (rand() % 100);
   for(int i = 0; i < s_count; i++)
      t[i] = ThreadCreateEx(ConditionWrapperWorkerThread, 0, i % 2 ? &c2 : &c1);
   c2.set();
   for(int i = 0; i < s_count; i++)
      ThreadJoin(t[i]);
   AssertEquals(s_condPass, s_count);
   EndTest();
}
