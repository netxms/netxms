/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: index.cpp
**
** Unit tests for AbstractIndexBase (sorted key/object index with lock-free readers
** and double-buffered writes) and its templates AbstractIndex, AbstractIndexWithDestructor
** and SharedPointerIndex.
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <nms_core.h>
#include <testtools.h>

/**
 * Number of live TestObject instances
 */
static VolatileCounter s_liveObjects = 0;

/**
 * Test object stored in index
 */
struct TestObject
{
   uint64_t id;
   int tag;

   TestObject(uint64_t _id, int _tag = 0) : id(_id), tag(_tag)
   {
      InterlockedIncrement(&s_liveObjects);
   }

   ~TestObject()
   {
      InterlockedDecrement(&s_liveObjects);
   }
};

/**
 * Check that keys() returns exactly the given keys in ascending order
 */
static void AssertKeys(const AbstractIndexBase& index, const uint64_t *expected, int count)
{
   IntegerArray<uint64_t> keys = index.keys();
   AssertEquals(keys.size(), count);
   AssertEquals(index.size(), count);
   for(int i = 0; i < count; i++)
   {
      AssertTrue(keys.get(i) == expected[i]);
      AssertTrue(index.contains(expected[i]));
   }
   for(int i = 1; i < count; i++)
      AssertTrue(keys.get(i - 1) < keys.get(i));
}

/**
 * Empty index behaviour and basic put/get/remove
 */
static void TestBasicOperations()
{
   StartTest(_T("AbstractIndex - basic operations"));

   int32_t baseline = s_liveObjects;

   AbstractIndex<TestObject> index(Ownership::False);
   AssertEquals(index.size(), 0);
   AssertNull(index.get(1));
   AssertNull(index.get(0));
   AssertFalse(index.contains(1));
   AssertEquals(index.keys().size(), 0);
   AssertFalse(index.isOwner());

   // Remove and clear on empty index must be no-ops
   index.remove(1);
   index.clear();
   AssertEquals(index.size(), 0);

   TestObject o1(1), o2(2), o3(3), o2b(2, 1);

   AssertFalse(index.put(1, &o1));
   AssertFalse(index.put(2, &o2));
   AssertFalse(index.put(3, &o3));
   AssertEquals(index.size(), 3);
   AssertTrue(index.get(1) == &o1);
   AssertTrue(index.get(2) == &o2);
   AssertTrue(index.get(3) == &o3);
   AssertNull(index.get(0));
   AssertNull(index.get(4));
   AssertTrue(index.contains(2));
   AssertFalse(index.contains(4));

   static const uint64_t keys123[] = { 1, 2, 3 };
   AssertKeys(index, keys123, 3);

   // Replacing existing key returns true and keeps size
   AssertTrue(index.put(2, &o2b));
   AssertEquals(index.size(), 3);
   AssertTrue(index.get(2) == &o2b);
   AssertKeys(index, keys123, 3);

   // Remove middle, then first, then last element
   index.remove(2);
   AssertEquals(index.size(), 2);
   AssertNull(index.get(2));
   AssertTrue(index.get(1) == &o1);
   AssertTrue(index.get(3) == &o3);
   static const uint64_t keys13[] = { 1, 3 };
   AssertKeys(index, keys13, 2);

   index.remove(2);  // already removed
   AssertEquals(index.size(), 2);

   index.remove(1);
   AssertEquals(index.size(), 1);
   AssertNull(index.get(1));
   AssertTrue(index.get(3) == &o3);

   index.remove(3);
   AssertEquals(index.size(), 0);
   AssertNull(index.get(3));
   AssertEquals(index.keys().size(), 0);

   // Index is usable again after becoming empty
   AssertFalse(index.put(7, &o1));
   AssertEquals(index.size(), 1);
   AssertTrue(index.get(7) == &o1);

   index.clear();
   AssertEquals(index.size(), 0);
   AssertNull(index.get(7));

   // Objects were not owned by index
   AssertEquals(static_cast<int32_t>(s_liveObjects), baseline + 4);

   EndTest();
}

/**
 * Keys inserted out of order must be kept sorted; maxKey tracking on remove
 */
static void TestKeyOrdering()
{
   StartTest(_T("AbstractIndex - key ordering"));

   AbstractIndex<TestObject> index(Ownership::False);

   const uint64_t big = 0xFFFFFFFFFFFFFFFFULL;
   TestObject o50(50), o10(10), o30(30), o20(20), o40(40), o0(0), oBig(big), o35(35), o60(60);

   index.put(50, &o50);
   index.put(10, &o10);   // smaller than max - triggers sort
   index.put(30, &o30);
   index.put(20, &o20);
   index.put(40, &o40);
   index.put(0, &o0);     // key 0 is a valid key
   index.put(big, &oBig); // largest possible key

   static const uint64_t keysA[] = { 0, 10, 20, 30, 40, 50, big };
   AssertKeys(index, keysA, 7);
   AssertTrue(index.get(0) == &o0);
   AssertTrue(index.get(10) == &o10);
   AssertTrue(index.get(20) == &o20);
   AssertTrue(index.get(30) == &o30);
   AssertTrue(index.get(40) == &o40);
   AssertTrue(index.get(50) == &o50);
   AssertTrue(index.get(big) == &oBig);
   AssertNull(index.get(5));
   AssertNull(index.get(25));
   AssertNull(index.get(big - 1));

   // Remove current maximum, then insert key between new maximum and old maximum
   index.remove(big);
   AssertNull(index.get(big));
   index.put(60, &o60);
   static const uint64_t keysB[] = { 0, 10, 20, 30, 40, 50, 60 };
   AssertKeys(index, keysB, 7);
   AssertTrue(index.get(60) == &o60);

   // Insert below maximum after removal
   index.remove(40);
   index.put(35, &o35);
   static const uint64_t keysC[] = { 0, 10, 20, 30, 35, 50, 60 };
   AssertKeys(index, keysC, 7);
   AssertTrue(index.get(35) == &o35);
   AssertNull(index.get(40));

   // Remove minimum
   index.remove(0);
   static const uint64_t keysD[] = { 10, 20, 30, 35, 50, 60 };
   AssertKeys(index, keysD, 6);
   AssertNull(index.get(0));

   EndTest();
}

/**
 * Object destruction by owning index (replace, remove, clear, destructor) and setOwner()
 */
static void TestOwnership()
{
   StartTest(_T("AbstractIndex - ownership"));

   int32_t baseline = s_liveObjects;

   {
      AbstractIndexWithDestructor<TestObject> index(Ownership::True);
      AssertTrue(index.isOwner());

      index.put(1, new TestObject(1));
      index.put(2, new TestObject(2));
      index.put(3, new TestObject(3));
      AssertEquals(static_cast<int32_t>(s_liveObjects), baseline + 3);

      // Replace destroys old object
      TestObject *replacement = new TestObject(2, 1);
      AssertTrue(index.put(2, replacement));
      AssertEquals(static_cast<int32_t>(s_liveObjects), baseline + 3);
      AssertTrue(index.get(2) == replacement);

      // Remove destroys object
      index.remove(1);
      AssertEquals(static_cast<int32_t>(s_liveObjects), baseline + 2);
      index.remove(1);  // no-op
      AssertEquals(static_cast<int32_t>(s_liveObjects), baseline + 2);

      // Clear destroys all objects
      index.clear();
      AssertEquals(static_cast<int32_t>(s_liveObjects), baseline);
      AssertEquals(index.size(), 0);

      // Remaining objects are destroyed by index destructor
      index.put(10, new TestObject(10));
      index.put(11, new TestObject(11));
      AssertEquals(static_cast<int32_t>(s_liveObjects), baseline + 2);
   }
   AssertEquals(static_cast<int32_t>(s_liveObjects), baseline);

   // Non-owning index never destroys objects
   TestObject *external = new TestObject(5);
   {
      AbstractIndexWithDestructor<TestObject> index(Ownership::False);
      AssertFalse(index.isOwner());
      index.put(5, external);
      index.put(5, external);
      index.remove(5);
      index.put(5, external);
      index.clear();
      index.put(5, external);
      AssertEquals(static_cast<int32_t>(s_liveObjects), baseline + 1);
   }
   AssertEquals(static_cast<int32_t>(s_liveObjects), baseline + 1);

   // Ownership can be changed after construction
   {
      AbstractIndexWithDestructor<TestObject> index(Ownership::False);
      index.put(5, external);
      index.setOwner(Ownership::True);
      AssertTrue(index.isOwner());
      index.setOwner(Ownership::False);
      AssertFalse(index.isOwner());
      index.setOwner(Ownership::True);
   }
   AssertEquals(static_cast<int32_t>(s_liveObjects), baseline);

   // Base class default destructor releases memory with MemFree
   {
      AbstractIndexBase index(Ownership::True);
      for(uint64_t i = 0; i < 100; i++)
         index.put(i, MemAllocArray<char>(64));
      AssertTrue(index.put(50, MemAllocArray<char>(32)));
      index.remove(10);
      index.remove(99);
      AssertEquals(index.size(), 98);
      index.clear();
      AssertEquals(index.size(), 0);
      index.put(1, MemAllocArray<char>(16));
      index.put(2, MemAllocArray<char>(16));
   }

   EndTest();
}

/**
 * Startup mode: unsorted appends, lazy sorting on read, and switch back to normal mode
 */
static void TestStartupMode()
{
   StartTest(_T("AbstractIndex - startup mode"));

   int32_t baseline = s_liveObjects;
   const int count = 2500;  // more than one 1024-element allocation block

   {
      AbstractIndexWithDestructor<TestObject> index(Ownership::True);
      index.setStartupMode(true);
      index.setStartupMode(true);  // repeated call is a no-op

      // Insert in descending order so that every element is out of order
      for(int i = count - 1; i >= 0; i--)
         AssertFalse(index.put(i * 2, new TestObject(i * 2)));
      AssertEquals(index.size(), count);

      // Reads sort the index on demand
      TestObject *o = index.get(100);
      AssertNotNull(o);
      AssertTrue(o->id == 100);
      AssertNull(index.get(101));
      AssertTrue(index.get(0)->id == 0);
      AssertTrue(index.get((count - 1) * 2)->id == static_cast<uint64_t>((count - 1) * 2));

      {
         IntegerArray<uint64_t> keys = index.keys();
         AssertEquals(keys.size(), count);
         for(int i = 0; i < count; i++)
            AssertTrue(keys.get(i) == static_cast<uint64_t>(i * 2));
      }

      // Further appends make the index dirty again; remove must see them sorted
      index.put(1, new TestObject(1));
      index.put(3, new TestObject(3));
      index.remove(2);
      index.remove(4);
      index.remove(5);  // not present
      AssertEquals(index.size(), count);
      AssertNull(index.get(2));
      AssertNull(index.get(4));
      AssertTrue(index.get(1)->id == 1);
      AssertTrue(index.get(3)->id == 3);
      AssertTrue(index.get(6)->id == 6);

      // Put in startup mode does not check for duplicates, it only appends
      AssertFalse(index.put(7, new TestObject(7)));
      AssertEquals(index.size(), count + 1);

      // Switch to normal mode: index must be sorted and fully readable
      index.setStartupMode(false);
      index.setStartupMode(false);  // repeated call is a no-op
      AssertEquals(index.size(), count + 1);
      AssertTrue(index.get(0)->id == 0);
      AssertTrue(index.get(1)->id == 1);
      AssertTrue(index.get(3)->id == 3);
      AssertTrue(index.get(7)->id == 7);
      AssertNull(index.get(2));
      AssertNull(index.get(4));
      AssertTrue(index.get((count - 1) * 2)->id == static_cast<uint64_t>((count - 1) * 2));

      {
         IntegerArray<uint64_t> keys = index.keys();
         AssertEquals(keys.size(), count + 1);
         for(int i = 1; i < keys.size(); i++)
            AssertTrue(keys.get(i - 1) < keys.get(i));
      }

      // Normal mode operations work on both index copies after the switch
      AssertTrue(index.put(7, new TestObject(7, 1)));
      AssertTrue(index.get(7)->tag == 1);
      AssertFalse(index.put(5, new TestObject(5)));
      AssertFalse(index.put(count * 2 + 100, new TestObject(count * 2 + 100)));
      index.remove(0);
      index.remove(6);
      AssertEquals(index.size(), count + 1);
      AssertNull(index.get(0));
      AssertNull(index.get(6));
      AssertTrue(index.get(5)->id == 5);
      AssertTrue(index.get(count * 2 + 100)->id == static_cast<uint64_t>(count * 2 + 100));
      AssertTrue(index.get(8)->id == 8);

      {
         IntegerArray<uint64_t> keys = index.keys();
         AssertEquals(keys.size(), count + 1);
         for(int i = 1; i < keys.size(); i++)
            AssertTrue(keys.get(i - 1) < keys.get(i));
      }

      AssertEquals(static_cast<int32_t>(s_liveObjects), baseline + count + 1);
   }
   AssertEquals(static_cast<int32_t>(s_liveObjects), baseline);

   // Leaving startup mode on empty index
   {
      AbstractIndexWithDestructor<TestObject> index(Ownership::True);
      index.setStartupMode(true);
      AssertEquals(index.size(), 0);
      AssertNull(index.get(1));
      index.remove(1);
      index.setStartupMode(false);
      AssertEquals(index.size(), 0);
      AssertNull(index.get(1));
      AssertFalse(index.put(1, new TestObject(1)));
      AssertFalse(index.put(2, new TestObject(2)));
      AssertTrue(index.put(1, new TestObject(1, 1)));
      AssertEquals(index.size(), 2);
      AssertTrue(index.get(1)->tag == 1);
      index.remove(2);
      AssertEquals(index.size(), 1);
      AssertEquals(static_cast<int32_t>(s_liveObjects), baseline + 1);
   }
   AssertEquals(static_cast<int32_t>(s_liveObjects), baseline);

   // Objects removed in startup mode by owning index are destroyed
   {
      AbstractIndexWithDestructor<TestObject> index(Ownership::True);
      index.setStartupMode(true);
      index.put(1, new TestObject(1));
      index.put(2, new TestObject(2));
      AssertEquals(static_cast<int32_t>(s_liveObjects), baseline + 2);
      index.remove(1);
      AssertEquals(static_cast<int32_t>(s_liveObjects), baseline + 1);
      index.setStartupMode(false);
      AssertEquals(index.size(), 1);
      AssertTrue(index.get(2)->id == 2);
   }
   AssertEquals(static_cast<int32_t>(s_liveObjects), baseline);

   EndTest();
}

/**
 * Large data set in normal mode: growth of element storage and sort on out-of-order insert
 */
static void TestLargeDataSet()
{
   StartTest(_T("AbstractIndex - large data set"));

   int32_t baseline = s_liveObjects;
   const int count = 2200;  // more than one 1024-element allocation block
   int64_t startTime = GetMonotonicClockTime();

   AbstractIndexWithDestructor<TestObject> index(Ownership::True);

   // Descending insert: each put lands below current maximum
   for(int i = count - 1; i >= 0; i--)
      AssertFalse(index.put(i, new TestObject(i)));
   AssertEquals(index.size(), count);
   for(int i = 0; i < count; i++)
   {
      TestObject *o = index.get(i);
      AssertNotNull(o);
      AssertTrue(o->id == static_cast<uint64_t>(i));
   }

   // Remove every odd key
   for(int i = 1; i < count; i += 2)
      index.remove(i);
   AssertEquals(index.size(), count / 2);
   for(int i = 0; i < count; i++)
   {
      if (i % 2 == 0)
         AssertTrue(index.get(i)->id == static_cast<uint64_t>(i));
      else
         AssertNull(index.get(i));
   }
   AssertEquals(static_cast<int32_t>(s_liveObjects), baseline + count / 2);

   // Ascending insert above current maximum
   for(int i = count; i < count * 2; i++)
      AssertFalse(index.put(i, new TestObject(i)));
   AssertEquals(index.size(), count / 2 + count);

   IntegerArray<uint64_t> keys = index.keys();
   AssertEquals(keys.size(), count / 2 + count);
   for(int i = 1; i < keys.size(); i++)
      AssertTrue(keys.get(i - 1) < keys.get(i));
   AssertTrue(keys.get(0) == 0);
   AssertTrue(keys.get(keys.size() - 1) == static_cast<uint64_t>(count * 2 - 1));

   index.clear();
   AssertEquals(index.size(), 0);
   AssertEquals(static_cast<int32_t>(s_liveObjects), baseline);

   EndTest(GetMonotonicClockTime() - startTime);
}

/**
 * Comparator with untyped context
 */
static bool TagComparatorUntyped(TestObject *object, void *context)
{
   return object->tag == *static_cast<int*>(context);
}

/**
 * Comparator with typed context
 */
static bool TagComparatorTyped(TestObject *object, int *context)
{
   return object->tag == *context;
}

/**
 * Enumeration callback with untyped context: collect visited IDs, stop after ID 30
 */
static EnumerationCallbackResult EnumeratorUntyped(TestObject *object, void *context)
{
   static_cast<IntegerArray<uint64_t>*>(context)->add(object->id);
   return (object->id >= 30) ? _STOP : _CONTINUE;
}

/**
 * Enumeration callback with typed context
 */
static EnumerationCallbackResult EnumeratorTyped(TestObject *object, IntegerArray<uint64_t> *context)
{
   context->add(object->id);
   return _CONTINUE;
}

/**
 * find, findAll and forEach in all their forms
 */
static void TestSearchAndEnumeration()
{
   StartTest(_T("AbstractIndex - find/findAll/forEach"));

   int32_t baseline = s_liveObjects;

   AbstractIndexWithDestructor<TestObject> index(Ownership::True);
   index.put(30, new TestObject(30, 1));
   index.put(10, new TestObject(10, 1));
   index.put(50, new TestObject(50, 2));
   index.put(20, new TestObject(20, 2));
   index.put(40, new TestObject(40, 1));

   // find returns first match in key order
   int tag = 1;
   TestObject *o = index.find(TagComparatorUntyped, static_cast<void*>(&tag));
   AssertNotNull(o);
   AssertTrue(o->id == 10);

   tag = 2;
   o = index.find(TagComparatorTyped, &tag);
   AssertNotNull(o);
   AssertTrue(o->id == 20);

   o = index.find([](TestObject *e) { return e->id > 35; });
   AssertNotNull(o);
   AssertTrue(o->id == 40);

   tag = 3;
   AssertNull(index.find(TagComparatorUntyped, static_cast<void*>(&tag)));
   AssertNull(index.find(TagComparatorTyped, &tag));
   AssertNull(index.find([](TestObject *e) { return e->id == 25; }));

   // findAll returns all matches in key order; result array does not own objects
   tag = 1;
   ObjectArray<TestObject> *results = index.findAll(TagComparatorUntyped, static_cast<void*>(&tag));
   AssertNotNull(results);
   AssertEquals(results->size(), 3);
   AssertTrue(results->get(0)->id == 10);
   AssertTrue(results->get(1)->id == 30);
   AssertTrue(results->get(2)->id == 40);
   delete results;

   tag = 2;
   results = index.findAll(TagComparatorTyped, &tag);
   AssertEquals(results->size(), 2);
   AssertTrue(results->get(0)->id == 20);
   AssertTrue(results->get(1)->id == 50);
   delete results;

   results = index.findAll([](TestObject *e) { return e->id >= 30; });
   AssertEquals(results->size(), 3);
   AssertTrue(results->get(0)->id == 30);
   AssertTrue(results->get(1)->id == 40);
   AssertTrue(results->get(2)->id == 50);
   delete results;

   tag = 3;
   results = index.findAll(TagComparatorTyped, &tag);
   AssertEquals(results->size(), 0);
   delete results;

   AssertEquals(static_cast<int32_t>(s_liveObjects), baseline + 5);

   // forEach visits in key order and honours _STOP
   IntegerArray<uint64_t> visited;
   index.forEach(EnumeratorUntyped, static_cast<void*>(&visited));
   AssertEquals(visited.size(), 3);
   AssertTrue(visited.get(0) == 10);
   AssertTrue(visited.get(1) == 20);
   AssertTrue(visited.get(2) == 30);

   visited.clear();
   index.forEach(EnumeratorTyped, &visited);
   AssertEquals(visited.size(), 5);
   AssertTrue(visited.get(0) == 10);
   AssertTrue(visited.get(4) == 50);

   visited.clear();
   index.forEach(
      [&visited] (TestObject *e) -> EnumerationCallbackResult
      {
         visited.add(e->id);
         return (e->id == 20) ? _STOP : _CONTINUE;
      });
   AssertEquals(visited.size(), 2);
   AssertTrue(visited.get(0) == 10);
   AssertTrue(visited.get(1) == 20);

   // Enumeration and search on empty index
   index.clear();
   visited.clear();
   index.forEach(EnumeratorTyped, &visited);
   AssertEquals(visited.size(), 0);
   AssertNull(index.find([](TestObject *e) { return true; }));
   results = index.findAll([](TestObject *e) { return true; });
   AssertEquals(results->size(), 0);
   delete results;

   EndTest();
}

/**
 * SharedPointerIndex: reference counting and shared_ptr based API
 */
static void TestSharedPointerIndex()
{
   StartTest(_T("SharedPointerIndex"));

   shared_ptr<TestObject> external = make_shared<TestObject>(2, 2);
   int32_t baseline = s_liveObjects;

   {
      SharedPointerIndex<TestObject> index;
      AssertTrue(index.isOwner());
      AssertEquals(index.size(), 0);
      AssertFalse(index.get(1));
      AssertFalse(index.contains(1));

      // Put by raw pointer and by shared pointer
      AssertFalse(index.put(1, new TestObject(1, 1)));
      AssertFalse(index.put(2, external));
      AssertFalse(index.put(3, make_shared<TestObject>(3, 1)));
      AssertEquals(index.size(), 3);
      AssertEquals(static_cast<int32_t>(external.use_count()), 2);

      // get returns a copy of the shared pointer
      {
         shared_ptr<TestObject> o = index.get(2);
         AssertTrue(o);
         AssertTrue(o.get() == external.get());
         AssertEquals(static_cast<int32_t>(external.use_count()), 3);
      }
      AssertEquals(static_cast<int32_t>(external.use_count()), 2);
      AssertTrue(index.get(1)->id == 1);
      AssertTrue(index.get(3)->id == 3);
      AssertFalse(index.get(4));
      AssertTrue(index.contains(3));
      AssertFalse(index.contains(4));

      // Replace releases old reference
      AssertTrue(index.put(2, new TestObject(2, 5)));
      AssertEquals(static_cast<int32_t>(external.use_count()), 1);
      AssertTrue(index.get(2)->tag == 5);
      AssertEquals(index.size(), 3);
      AssertEquals(static_cast<int32_t>(s_liveObjects), baseline + 3);

      // Remove releases reference
      AssertTrue(index.put(2, external));
      AssertEquals(static_cast<int32_t>(external.use_count()), 2);
      AssertEquals(static_cast<int32_t>(s_liveObjects), baseline + 2);
      index.remove(2);
      AssertEquals(static_cast<int32_t>(external.use_count()), 1);
      AssertFalse(index.get(2));
      AssertEquals(index.size(), 2);

      // getAll returns copies in key order
      index.put(2, external);
      unique_ptr<SharedObjectArray<TestObject>> all = index.getAll();
      AssertEquals(all->size(), 3);
      AssertTrue(all->get(0)->id == 1);
      AssertTrue(all->get(1)->id == 2);
      AssertTrue(all->get(2)->id == 3);
      AssertEquals(static_cast<int32_t>(external.use_count()), 3);
      all.reset();
      AssertEquals(static_cast<int32_t>(external.use_count()), 2);

      // find variants
      int tag = 2;
      shared_ptr<TestObject> found = index.find(TagComparatorUntyped, static_cast<void*>(&tag));
      AssertTrue(found);
      AssertTrue(found.get() == external.get());
      AssertEquals(static_cast<int32_t>(external.use_count()), 3);
      found.reset();

      tag = 1;
      found = index.find(TagComparatorTyped, &tag);
      AssertTrue(found);
      AssertTrue(found->id == 1);

      found = index.find([](TestObject *e) { return e->id == 3; });
      AssertTrue(found);
      AssertTrue(found->id == 3);

      tag = 9;
      AssertFalse(index.find(TagComparatorUntyped, static_cast<void*>(&tag)));
      AssertFalse(index.find(TagComparatorTyped, &tag));
      AssertFalse(index.find([](TestObject *e) { return false; }));
      found.reset();

      // findAll variants
      tag = 1;
      unique_ptr<SharedObjectArray<TestObject>> results = index.findAll(TagComparatorUntyped, static_cast<void*>(&tag));
      AssertEquals(results->size(), 2);
      AssertTrue(results->get(0)->id == 1);
      AssertTrue(results->get(1)->id == 3);

      results = index.findAll(TagComparatorTyped, &tag);
      AssertEquals(results->size(), 2);

      results = index.findAll([](TestObject *e) { return e->id >= 2; });
      AssertEquals(results->size(), 2);
      AssertTrue(results->get(0)->id == 2);
      AssertTrue(results->get(1)->id == 3);
      AssertEquals(static_cast<int32_t>(external.use_count()), 3);
      results.reset();
      AssertEquals(static_cast<int32_t>(external.use_count()), 2);

      tag = 9;
      results = index.findAll(TagComparatorTyped, &tag);
      AssertEquals(results->size(), 0);
      results.reset();

      // forEach variants
      IntegerArray<uint64_t> visited;
      index.forEach(EnumeratorUntyped, static_cast<void*>(&visited));
      AssertEquals(visited.size(), 3);
      AssertTrue(visited.get(0) == 1);
      AssertTrue(visited.get(2) == 3);

      visited.clear();
      index.forEach(EnumeratorTyped, &visited);
      AssertEquals(visited.size(), 3);

      visited.clear();
      index.forEach(
         [&visited] (TestObject *e) -> EnumerationCallbackResult
         {
            visited.add(e->id);
            return (e->id == 2) ? _STOP : _CONTINUE;
         });
      AssertEquals(visited.size(), 2);
      AssertTrue(visited.get(1) == 2);

      // clear releases all references
      index.clear();
      AssertEquals(index.size(), 0);
      AssertEquals(static_cast<int32_t>(external.use_count()), 1);
      AssertEquals(static_cast<int32_t>(s_liveObjects), baseline);

      // Remaining references are released by index destructor
      index.put(2, external);
      index.put(4, new TestObject(4));
      AssertEquals(static_cast<int32_t>(external.use_count()), 2);
      AssertEquals(static_cast<int32_t>(s_liveObjects), baseline + 1);
   }
   AssertEquals(static_cast<int32_t>(external.use_count()), 1);
   AssertEquals(static_cast<int32_t>(s_liveObjects), baseline);

   // Startup mode with shared pointers
   {
      SharedPointerIndex<TestObject> index;
      index.setStartupMode(true);
      for(int i = 100; i > 0; i--)
         index.put(i, make_shared<TestObject>(i));
      AssertEquals(index.size(), 100);
      AssertTrue(index.get(50)->id == 50);
      index.remove(50);
      index.setStartupMode(false);
      AssertEquals(index.size(), 99);
      AssertFalse(index.get(50));
      AssertTrue(index.get(1)->id == 1);
      AssertTrue(index.get(100)->id == 100);
      AssertEquals(static_cast<int32_t>(s_liveObjects), baseline + 99);
   }
   AssertEquals(static_cast<int32_t>(s_liveObjects), baseline);

   EndTest();
}

/**
 * Concurrent readers must always observe a consistent index while writer modifies it
 */
static void TestConcurrentAccess()
{
   StartTest(_T("AbstractIndex - concurrent access"));

   const int keyCount = 32;
   const int writerCycles = 5;
   const int readerCount = 4;

   int64_t startTime = GetMonotonicClockTime();

   // Objects outlive the index so readers can dereference whatever they get
   ObjectArray<TestObject> objects(keyCount, 16, Ownership::True);
   for(int i = 0; i < keyCount; i++)
      objects.add(new TestObject(i));

   AbstractIndex<TestObject> index(Ownership::False);
   VolatileCounter stop = 0;
   VolatileCounter readCount = 0;

   auto reader = [&] ()
   {
      uint64_t key = 0;
      while(stop == 0)
      {
         for(int i = 0; i < 100; i++)
         {
            TestObject *o = index.get(key);
            if (o != nullptr)
               AssertTrue(o->id == key);
            key = (key + 7) % keyCount;
         }

         uint64_t prev = 0;
         bool first = true;
         index.forEach(
            [&prev, &first] (TestObject *o) -> EnumerationCallbackResult
            {
               if (!first)
                  AssertTrue(o->id > prev);
               first = false;
               prev = o->id;
               return _CONTINUE;
            });

         IntegerArray<uint64_t> keys = index.keys();
         AssertTrue(keys.size() <= keyCount);
         for(int i = 1; i < keys.size(); i++)
            AssertTrue(keys.get(i - 1) < keys.get(i));

         InterlockedIncrement(&readCount);
      }
   };

   THREAD readers[readerCount];
   for(int i = 0; i < readerCount; i++)
      readers[i] = ThreadCreateEx(reader);

   // Make sure all readers are running before modifications start
   while(readCount < readerCount)
      ThreadSleepMs(1);

   for(int cycle = 0; cycle < writerCycles; cycle++)
   {
      // Fill in pseudo-random order
      for(int i = 0; i < keyCount; i++)
      {
         int k = (i * 37 + cycle) % keyCount;
         index.put(k, objects.get(k));
      }
      AssertEquals(index.size(), keyCount);

      // Replace all
      for(int i = 0; i < keyCount; i++)
         AssertTrue(index.put(i, objects.get(i)));
      AssertEquals(index.size(), keyCount);

      // Remove in different pseudo-random order
      for(int i = 0; i < keyCount; i++)
      {
         int k = (i * 11 + cycle * 3) % keyCount;
         index.remove(k);
      }
      AssertEquals(index.size(), 0);
   }

   InterlockedIncrement(&stop);
   for(int i = 0; i < readerCount; i++)
      ThreadJoin(readers[i]);

   EndTest(GetMonotonicClockTime() - startTime);
}

/**
 * Test entry point
 */
void TestObjectIndex()
{
   TestBasicOperations();
   TestKeyOrdering();
   TestOwnership();
   TestStartupMode();
   TestLargeDataSet();
   TestSearchAndEnumeration();
   TestSharedPointerIndex();
   TestConcurrentAccess();
}
