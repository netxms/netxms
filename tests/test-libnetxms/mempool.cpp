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

/**
 * Test memory pool
 */
void TestMemoryPool()
{
   StartTest(_T("Memory pool"));

   MemoryPool *pool = new MemoryPool(256);
   int *x = static_cast<int*>(pool->allocate(sizeof(int)));
   AssertNotNull(x);
   *x = 42;
   int *y = static_cast<int*>(pool->allocate(sizeof(int)));
   AssertNotNull(y);
   AssertNotEquals(x, y);

   int *array1 = pool->allocateArray<int>(16);
   AssertNotNull(array1);
   memset(array1, 0x7F, 16 * sizeof(int));

   int *array2 = pool->allocateArray<int>(256);
   AssertNotNull(array2);

   TestClass *object = pool->create<TestClass>();
   AssertNotNull(object);
   AssertTrue(object->initialized);

   void *bytes = pool->allocate(253);
   AssertNotNull(bytes);
   object = pool->create<TestClass>();
   AssertNotNull(object);
   AssertTrue(object->initialized);
   AssertTrue(reinterpret_cast<UINT64>(object) % 8 == 0);  // Check alignment

   delete pool;
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
