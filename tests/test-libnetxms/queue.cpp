#include <nms_common.h>
#include <nms_util.h>
#include <nxqueue.h>
#include <testtools.h>

/**
 * Test comparator for the queue
 */
static bool TestQueueComparator(const void *key, const void *value)
{
   return strcmp(static_cast<const char*>(key), static_cast<const char*>(value)) == 0;
}

/**
 * Test queue
 */
void TestQueue()
{
   Queue *q = new Queue(16, Ownership::False);

   StartTest(_T("Queue: put/get"));
   for(int i = 0; i < 40; i++)
      q->put(CAST_TO_POINTER(i + 1, void *));
   AssertEquals(q->size(), 40);
   AssertEquals(q->allocated(), 48);
   for(int i = 0; i < 40; i++)
   {
      void *p = q->get();
      AssertNotNull(p);
      AssertEquals(CAST_FROM_POINTER(p, int), i + 1);
   }
   AssertEquals(q->size(), 0);
   AssertNull(q->get());
   AssertNull(q->getOrBlock(100));
   EndTest();

   StartTest(_T("Queue: insert"));
   for (int i = 0; i < 20; i++)
      q->put((void*)"LowPriority");
   AssertEquals(q->size(), 20);
   q->insert((void*)"HighPriority");
   AssertEquals(q->size(), 21);
   AssertEquals(static_cast<char*>(q->get()), "HighPriority");
   AssertEquals(q->size(), 20);
   AssertEquals(static_cast<char*>(q->get()), "LowPriority");
   AssertEquals(q->size(), 19);
   EndTest();

   StartTest(_T("Queue: find/remove"));
   q->put((void*)"HighPriority");
   q->put((void*)"LowPriority");
   q->put((void*)"LowPriority");
   AssertEquals(q->size(), 22);
   AssertTrue(q->find("HighPriority", TestQueueComparator));
   AssertEquals(q->size(), 22);
   AssertTrue(q->remove("HighPriority", TestQueueComparator));
   AssertFalse(q->find("HighPriority", TestQueueComparator));
   EndTest();

   StartTest(_T("Queue: clear"));
   q->clear();
   AssertEquals(q->size(), 0);
   AssertEquals(q->allocated(), 16);
   EndTest();

   StartTest(_T("Queue: shrink"));
   for(int i = 0; i < 60; i++)
      q->put(CAST_TO_POINTER(i + 1, void *));
   AssertEquals(q->size(), 60);
   AssertEquals(q->allocated(), 64);
   for(int i = 0; i < 55; i++)
   {
      void *p = q->get();
      AssertNotNull(p);
      AssertEquals(CAST_FROM_POINTER(p, int), i + 1);
   }
   AssertEquals(q->size(), 5);
   AssertEquals(q->allocated(), 16);
   EndTest();

#if !WITH_ADDRESS_SANITIZER
   StartTest(_T("Queue: performance"));
   delete q;
   q = new Queue();
   int64_t startTime = GetCurrentTimeMs();
   for(int i = 0; i < 100; i++)
   {
      for(int j = 0; j < 10000; j++)
         q->put(CAST_TO_POINTER(j + 1, void *));
      AssertEquals(q->size(), 10000);

      void *p;
      while((p = q->get()) != nullptr)
         ;
      AssertEquals(q->size(), 0);
   }
   EndTest(GetCurrentTimeMs() - startTime);
#endif

   delete q;
}

struct TestObject
{
   uint32_t id;
   char *text;

   TestObject(uint32_t _id)
   {
      id = _id;
      text = MemAllocStringA(32);
      snprintf(text, 32, "%u", id);
   }
   TestObject(const char *_text)
   {
      id = 0;
      text = MemCopyStringA(_text);
   }
   ~TestObject()
   {
      MemFree(text);
   }
};

/**
 * Test comparator for the shared object queue
 */
static bool TestSharedObjectQueueComparator(const void *key, const void *value)
{
   return strcmp(static_cast<const char*>(key), static_cast<const shared_ptr<TestObject>*>(value)->get()->text) == 0;
}

/**
 * Test shared object queue
 */
void TestSharedObjectQueue()
{
   auto q = new SharedObjectQueue<TestObject>(16);

   StartTest(_T("SharedObjectQueue: put/get"));
   for(int i = 0; i < 40; i++)
      q->put(make_shared<TestObject>(i + 1));
   AssertEquals(q->size(), 40);
   AssertEquals(q->allocated(), 48);
   for(uint32_t i = 0; i < 40; i++)
   {
      shared_ptr<TestObject> p = q->get();
      AssertTrue(p != nullptr);
      AssertEquals(p->id, i + 1);
   }
   AssertEquals(q->size(), 0);
   EndTest();

   StartTest(_T("SharedObjectQueue: insert"));
   for (int i = 0; i < 20; i++)
      q->put(make_shared<TestObject>("LowPriority"));
   AssertEquals(q->size(), 20);
   q->insert(make_shared<TestObject>("HighPriority"));
   AssertEquals(q->size(), 21);
   AssertEquals(q->get()->text, "HighPriority");
   AssertEquals(q->size(), 20);
   AssertEquals(q->get()->text, "LowPriority");
   AssertEquals(q->size(), 19);
   EndTest();

   StartTest(_T("SharedObjectQueue: find/remove"));
   q->put(make_shared<TestObject>("HighPriority"));
   q->put(make_shared<TestObject>("LowPriority"));
   q->put(make_shared<TestObject>("LowPriority"));
   AssertEquals(q->size(), 22);
   AssertTrue(q->find("HighPriority", TestSharedObjectQueueComparator));
   AssertEquals(q->size(), 22);
   AssertTrue(q->remove("HighPriority", TestSharedObjectQueueComparator));
   AssertFalse(q->find("HighPriority", TestSharedObjectQueueComparator));
   EndTest();

   StartTest(_T("SharedObjectQueue: clear"));
   q->clear();
   AssertEquals(q->size(), 0);
   AssertEquals(q->allocated(), 16);
   EndTest();

   StartTest(_T("SharedObjectQueue: shrink"));
   for(int i = 0; i < 60; i++)
      q->put(make_shared<TestObject>(i + 1));
   AssertEquals(q->size(), 60);
   AssertEquals(q->allocated(), 64);
   for(uint32_t i = 0; i < 55; i++)
   {
      shared_ptr<TestObject> p = q->get();
      AssertTrue(p != nullptr);
      AssertEquals(p->id, i + 1);
   }
   AssertEquals(q->size(), 5);
   AssertEquals(q->allocated(), 16);
   EndTest();

   delete q;
}

/**
 * Test structure
 */
struct TestStruct
{
   int64_t id;
   const char *text;
};

/**
 * Test queue
 */
void TestSQueue()
{
   SQueue<TestStruct> *q = new SQueue<TestStruct>(16);
   TestStruct s;

   StartTest(_T("SQueue: put/get"));
   AssertFalse(q->getOrBlock(&s, 100));
   for(int i = 0; i < 40; i++)
      q->put(TestStruct { i + 1, "put/get" });
   AssertEquals(q->size(), 40);
   AssertEquals(q->allocated(), 48);
   for(int64_t i = 0; i < 40; i++)
   {
      AssertTrue(q->get(&s));
      AssertEquals(s.id, i + 1);
   }
   AssertEquals(q->size(), 0);
   AssertFalse(q->get(&s));
   AssertFalse(q->getOrBlock(&s, 100));
   q->put(TestStruct { 111, "put/get" });
   AssertTrue(q->getOrBlock(&s, 100));
   AssertEquals(q->size(), 0);
   EndTest();

   StartTest(_T("SQueue: insert"));
   for (int i = 0; i < 20; i++)
      q->put(TestStruct { i, "LowPriority" });
   AssertEquals(q->size(), 20);
   q->insert(TestStruct { 0, "HighPriority" });
   AssertEquals(q->size(), 21);
   AssertTrue(q->get(&s));
   AssertEquals(s.text, "HighPriority");
   AssertEquals(q->size(), 20);
   AssertTrue(q->get(&s));
   AssertEquals(s.text, "LowPriority");
   AssertEquals(q->size(), 19);
   EndTest();

   StartTest(_T("SQueue: clear"));
   q->clear();
   AssertEquals(q->size(), 0);
   AssertFalse(q->get(&s));
   AssertEquals(q->allocated(), 16);
   EndTest();

   StartTest(_T("SQueue: shrink"));
   for(int i = 0; i < 60; i++)
      q->put(TestStruct { i + 1, "shrink" });
   AssertEquals(q->size(), 60);
   AssertEquals(q->allocated(), 64);
   for(int64_t i = 0; i < 55; i++)
   {
      AssertTrue(q->get(&s));
      AssertEquals(s.id, i + 1);
   }
   AssertEquals(q->size(), 5);
   AssertEquals(q->allocated(), 16);
   EndTest();

#if !WITH_ADDRESS_SANITIZER
   StartTest(_T("SQueue: performance"));
   delete q;
   q = new SQueue<TestStruct>();
   int64_t startTime = GetCurrentTimeMs();
   for(int i = 0; i < 100; i++)
   {
      for(int j = 0; j < 10000; j++)
         q->put(TestStruct { j + 1, "performance" });
      AssertEquals(q->size(), 10000);

      TestStruct s;;
      while(q->get(&s))
         ;
      AssertEquals(q->size(), 0);
   }
   EndTest(GetCurrentTimeMs() - startTime);
#endif

   delete q;
}
