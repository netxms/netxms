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
** File: test-libnxcore.cpp
**
** Unit test runner for server core library (libnxcore). Only utility and supplemental
** functions and classes that do not depend on an initialized server (database
** connection pool, loaded objects, user database, configuration cache) belong here.
** Tests that need a running server go to the test-server launcher.
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <nms_core.h>
#include <testtools.h>

void TestAICheckLogic();
void TestAIMemoryPromptBlock();
void TestAnomalyDetection();
void TestObjectIndex();
void TestPhysicalPlacement();
void TestSplitCommandLine();
void TestOpenSshKeyParser();

/**
 * Check that access change stamp never moves backwards when notifications for overlapping subtrees interleave
 */
static void TestAccessChangeStamp()
{
   StartTest(L"Access change stamp ordering");
   std::atomic<uint64_t> stamp(0);
   AtomicStoreMax(stamp, 2);
   AssertEquals(stamp.load(), static_cast<uint64_t>(2));
   AtomicStoreMax(stamp, 1);
   AssertEquals(stamp.load(), static_cast<uint64_t>(2));
   AtomicStoreMax(stamp, 3);
   AssertEquals(stamp.load(), static_cast<uint64_t>(3));
   EndTest();
}

/**
 * Check publication of a completed parent walk across both kinds of invalidation.
 */
static void TestInheritedRightsCache()
{
   StartTest(L"Inherited rights cache version");
   AccessList acl;
   AccessList::CacheVersion first, current;
   uint64_t rights;
   const uint32_t userId = 42;
   AssertFalse(acl.getUserRights(userId, &rights, &first));
   acl.cacheUserRights(userId, OBJECT_ACCESS_READ, first);
   AssertTrue(acl.getCachedUserRights(userId, first, &rights));
   AssertEquals(rights, static_cast<uint64_t>(OBJECT_ACCESS_READ));

   acl.clearCache();
   AssertFalse(acl.getUserRights(userId, &rights, &current));
   AssertFalse(acl.getCachedUserRights(userId, current, &rights));
   acl.cacheUserRights(userId, 0, current);
   acl.cacheUserRights(userId, OBJECT_ACCESS_READ, first);

   AccessList::CacheVersion oldGeneration = current;
   oldGeneration.generation--;
   acl.cacheUserRights(userId, OBJECT_ACCESS_READ, oldGeneration);
   AssertFalse(acl.getCachedUserRights(userId, oldGeneration, &rights));
   AssertTrue(acl.getCachedUserRights(userId, current, &rights));
   AssertEquals(rights, static_cast<uint64_t>(0));
   EndTest();
}

/**
 * main()
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);

#ifdef _WIN32
   WSADATA wsaData;
   WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

   TestAICheckLogic();
   TestAIMemoryPromptBlock();
   TestAnomalyDetection();
   TestObjectIndex();
   TestInheritedRightsCache();
   TestAccessChangeStamp();
   TestPhysicalPlacement();
   TestSplitCommandLine();
   TestOpenSshKeyParser();

   return 0;
}
