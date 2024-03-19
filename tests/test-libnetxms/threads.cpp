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

static void MutexWorkerThread2(Mutex *m)
{
   m->lock();
   s_val = 1;
   ThreadSleepMs(800);
   m->unlock();
}

void TestMutex()
{
   StartTest(_T("Mutex"));
   for (int n = 0; n < 10; n++)
   {
      s_val = 0;
      s_count = GenerateRandomNumber(100, 200);
      s_increment = GenerateRandomNumber(1, 5) * 2;

      Mutex m;

      THREAD t[200];
      for (int i = 0; i < s_count; i++)
         t[i] = ThreadCreateEx(MutexWorkerThread, &m);
      for (int i = 0; i < s_count; i++)
         ThreadJoin(t[i]);

      AssertEquals(s_val, s_count * 100 * s_increment);
   }
   EndTest();

   StartTest(_T("Mutex - timed lock"));
   Mutex m;
   s_val = 0;
   THREAD t = ThreadCreateEx(MutexWorkerThread2, &m);
   ThreadSleepMs(100);
   AssertFalse(m.timedLock(200));
   AssertTrue(m.timedLock(1000));
   m.unlock();
   ThreadJoin(t);
   AssertEquals(s_val, 1);
   EndTest();
}

static Mutex s_lockGuardTestMutex(MutexType::FAST);
static int32_t s_lockGuardCounter;

static void LockGuardWorker()
{
   LockGuard lg(s_lockGuardTestMutex);
   for(int i = 0; i < 100; i++)
      s_lockGuardCounter++;
}

void TestUniqueLock()
{
   StartTest(_T("LockGuard"));
   s_lockGuardCounter = 0;
   THREAD t[200];
   for (int i = 0; i < 200; i++)
      t[i] = ThreadCreateEx(LockGuardWorker);
   for (int i = 0; i < 200; i++)
      ThreadJoin(t[i]);
   AssertEquals(s_lockGuardCounter, 200 * 100);
   AssertTrue(s_lockGuardTestMutex.tryLock());
   s_lockGuardTestMutex.unlock();

   if (s_lockGuardCounter > 0)
   {
      LockGuard lockGuard(s_lockGuardTestMutex);
      s_lockGuardCounter = 0;
   }
   AssertEquals(s_lockGuardCounter, 0);
   AssertTrue(s_lockGuardTestMutex.tryLock());
   s_lockGuardTestMutex.unlock();

   EndTest();
}

static void RWLockWorkerThread(RWLock *rwlock)
{
   ThreadSleepMs(GenerateRandomNumber(0, 10));
   rwlock->writeLock();
   for(int i = 0; i < 10000; i++)
      s_val = s_val + s_increment / 2;
   rwlock->unlock();
}

void TestRWLock()
{
   StartTest(_T("R/W lock"));

   for(int n = 0; n < 10; n++)
   {
      s_val = 0;
      s_count = GenerateRandomNumber(100, 200);
      s_increment = GenerateRandomNumber(1, 5) * 2;

      RWLock rwlock;

      THREAD t[200];
      rwlock.readLock();
      for(int i = 0; i < s_count; i++)
         t[i] = ThreadCreateEx(RWLockWorkerThread, &rwlock);
      rwlock.unlock();
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
   s_count = GenerateRandomNumber(100, 200);
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
