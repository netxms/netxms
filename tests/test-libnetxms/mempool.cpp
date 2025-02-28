#include <nms_common.h>
#include <nms_util.h>
#include <testtools.h>

/**
 * Test object class
 */
class TestClass
{
public:
   void *dummy;
   int index;
   bool initialized;
   bool deleted;

   TestClass()
   {
      dummy = this;
      index = -1;
      initialized = true;
      deleted = false;
   }

   TestClass(int i)
   {
      dummy = this;
      index = i;
      initialized = true;
      deleted = false;
   }

   ~TestClass()
   {
      deleted = true;
   }
};

static MemoryPool CreateMemoryPool(TCHAR **s)
{
   MemoryPool pool(256);
   *s = pool.copyString(_T("Move test string"));
   return pool;
}

/**
 * Test memory pool
 */
void TestMemoryPool()
{
   StartTest(_T("Memory pool"));

   MemoryPool pool(256);
   AssertEquals(pool.getRegionCount(), 0);
   int *x = static_cast<int*>(pool.allocate(sizeof(int)));
   AssertNotNull(x);
   *x = 42;
   int *y = static_cast<int*>(pool.allocate(sizeof(int)));
   AssertNotNull(y);
   AssertNotEquals(x, y);
   AssertEquals(pool.getRegionCount(), 1);

   int *array1 = pool.allocateArray<int>(16);
   AssertNotNull(array1);
   memset(array1, 0x7F, 16 * sizeof(int));

   int *array2 = pool.allocateArray<int>(256);
   AssertNotNull(array2);

   TestClass *object = pool.create<TestClass>();
   AssertNotNull(object);
   AssertTrue(object->initialized);

   void *bytes = pool.allocate(253);
   AssertNotNull(bytes);
   object = pool.create<TestClass>();
   AssertNotNull(object);
   AssertTrue(object->initialized);
   AssertTrue(reinterpret_cast<uint64_t>(object) % 8 == 0);  // Check alignment

   EndTest();

   StartTest(_T("Memory pool - move"));

   MemoryPool *spool = new MemoryPool(256);
   AssertEquals(spool->getRegionCount(), 0);

   int *i = static_cast<int*>(spool->allocate(sizeof(int)));
   AssertNotNull(i);
   *i = 42;
   TCHAR *s1 = spool->copyString(_T("Some string #1"));
   AssertNotNull(s1);
   AssertEquals(spool->getRegionCount(), 1);

   spool->allocateArray<BYTE>(250);   // will trigger new region allocation
   AssertEquals(spool->getRegionCount(), 2);

   TCHAR *s2 = spool->copyString(_T("Some string #2"));
   AssertNotNull(s2);
   AssertEquals(spool->getRegionCount(), 3);

   MemoryPool dpool(std::move(*spool));
   TCHAR *s3 = dpool.copyString(_T("Some string #3"));
   AssertNotNull(s3);

   AssertEquals(spool->getRegionCount(), 0);
   AssertEquals(dpool.getRegionCount(), 3);

   delete spool;  // All allocated memory should be still valid after that point

   AssertEquals(*i, 42);
   AssertEquals(s1, _T("Some string #1"));
   AssertEquals(s2, _T("Some string #2"));
   AssertEquals(s3, _T("Some string #3"));

   dpool = CreateMemoryPool(&s1);
   AssertEquals(s1, _T("Move test string"));

   EndTest();
}

/**
 * Test object memory pool
 */
void TestObjectMemoryPool()
{
   StartTest(_T("Object memory pool"));
   ObjectMemoryPool<TestClass> pool(64);

   TestClass *o = pool.create();
   AssertTrue(o->initialized);
   AssertEquals(o->index, -1);
   AssertFalse(o->deleted);

   o->index = -42;
   pool.destroy(o);

   TestClass *o2 = pool.allocate();
   AssertTrue(o == o2);
   pool.free(o2);

   TestClass *o3;
   for(int i = 0; i < 200; i++)
   {
      o = new (pool.allocate()) TestClass(i);
      AssertEquals(o->index, i);
      if (i == 20)
         o2 = o;
      if (i == 70)
         o3 = o;
   }

   pool.destroy(o2);
   pool.destroy(o3);

   AssertTrue(pool.allocate() == o3);
   AssertTrue(pool.allocate() == o2);

   EndTest();
}
