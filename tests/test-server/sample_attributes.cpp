/*
** NetXMS - Network Management System
** Copyright (C) 2026 Raden Solutions
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
** File: sample_attributes.cpp
**
** Tests for per-sample attributes of DCI history (write path, read path,
** deletion) and for computation method registry.
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <nms_core.h>
#include <testtools.h>
#include <cmath>

/**
 * Create DCI with given origin on given object
 */
static shared_ptr<DCItem> CreateItem(const shared_ptr<Collector>& owner, const wchar_t *name, int origin)
{
   DCItem *dci = new DCItem(CreateUniqueId(IDG_ITEM), name, origin, DCI_DT_FLOAT, DC_POLLING_SCHEDULE_DEFAULT, nullptr,
         DC_RETENTION_DEFAULT, nullptr, owner, name);
   AssertTrue(owner->addDCObject(dci));
   shared_ptr<DCObject> object = owner->getDCObjectById(dci->getId(), 0);
   AssertNotNull(object.get());
   return static_pointer_cast<DCItem>(object);
}

/**
 * Read attributed samples, waiting for background database writers to store expected number of samples
 */
static void ReadSamples(const DCItem& dci, Timestamp from, Timestamp to, size_t expectedCount, std::vector<AttributedSample>& samples)
{
   for(int i = 0; i < 100; i++)
   {
      AssertTrue(ReadAttributedSamples(dci, from, to, samples));
      if (samples.size() == expectedCount)
         return;
      ThreadSleepMs(100);
   }
   AssertEquals(static_cast<int>(samples.size()), static_cast<int>(expectedCount));
}

/**
 * Get number of stored attribute records for given DCI
 */
static int GetAttributeRecordCount(uint32_t dciId)
{
   wchar_t query[256];
   nx_swprintf(query, 256, L"SELECT count(*) FROM dci_sample_attributes WHERE item_id=%u", dciId);
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, query);
   AssertNotNull(hResult);
   int count = DBGetFieldLong(hResult, 0, 0);
   DBFreeResult(hResult);
   DBConnectionPoolReleaseConnection(hdb);
   return count;
}

/**
 * Test sample attributes
 */
void TestSampleAttributes()
{
   StartTest(L"Sample attributes - computation method registry");
   MethodDescriptor method;
   method.methodId = L"test-method";
   method.version = L"1.0";
   method.engine = L"test-server";
   method.displayName = L"Test method";
   method.tier = MethodTier::OPEN;
   method.coverageLevel = 0.95;
   uint32_t methodId = RegisterComputationMethod(method);
   AssertTrue(methodId != 0);
   AssertEquals(RegisterComputationMethod(method), methodId);
   method.version = L"1.1";
   uint32_t methodId2 = RegisterComputationMethod(method);
   AssertTrue(methodId2 != 0);
   AssertTrue(methodId2 != methodId);
   EndTest();

   shared_ptr<Collector> owner = make_shared<Collector>(L"sample-attributes-test");
   NetObjInsert(owner, true, false);

   StartTest(L"Sample attributes - producer restriction");
   shared_ptr<DCItem> pushItem = CreateItem(owner, L"Test.Push", DS_PUSH_AGENT);
   SampleAttributes estimated;
   estimated.quality = SampleQuality::ESTIMATED;
   estimated.bounded = true;
   estimated.lower = 9.5;
   estimated.upper = 11.25;
   estimated.completeness = 0.75;
   estimated.methodId = methodId;
   AssertFalse(pushItem->processNewValue(Timestamp::now(), 10.0, estimated));

   shared_ptr<DCItem> dci = CreateItem(owner, L"Test.Computed", DS_COMPUTED);
   SampleAttributes invalid = estimated;
   invalid.upper = INFINITY;
   AssertFalse(dci->processNewValue(Timestamp::now(), 10.0, invalid));
   AssertFalse(dci->processNewValue(Timestamp::now(), NAN, estimated));
   EndTest();

   StartTest(L"Sample attributes - write and read");
   Timestamp base = Timestamp::fromMilliseconds(Timestamp::now().asMilliseconds() - 3600000);
   Timestamp t1 = base;
   Timestamp t2 = Timestamp::fromMilliseconds(base.asMilliseconds() + 60000);
   Timestamp t3 = Timestamp::fromMilliseconds(base.asMilliseconds() + 120000);
   Timestamp t4 = Timestamp::fromMilliseconds(base.asMilliseconds() + 180000);

   SampleAttributes missing;
   missing.quality = SampleQuality::MISSING;
   missing.bounded = true;   // Bounds should be ignored for missing sample
   missing.lower = 1;
   missing.upper = 2;
   missing.completeness = 0;
   missing.methodId = methodId2;

   AssertTrue(dci->processNewValue(t1, 10.5, SampleAttributes()));   // Default attributes, no attribute record
   AssertTrue(dci->processNewValue(t2, 10.0, estimated));
   AssertTrue(dci->processNewValue(t3, 0, missing));
   SampleAttributes proxy;
   proxy.quality = SampleQuality::PROXY;
   AssertTrue(dci->processNewValue(t4, 12.0, proxy));   // Non-default, unbounded

   std::vector<AttributedSample> samples;
   ReadSamples(*dci, t1, t4, 4, samples);

   // Newest first
   AssertTrue(samples[0].timestamp == t4);
   AssertTrue(samples[0].value == 12.0);
   AssertTrue(samples[0].attributes.quality == SampleQuality::PROXY);
   AssertFalse(samples[0].attributes.bounded);
   AssertTrue(samples[0].attributes.completeness == 1.0);
   AssertEquals(samples[0].attributes.methodId, static_cast<uint32_t>(0));

   AssertTrue(samples[1].timestamp == t3);
   AssertTrue(std::isnan(samples[1].value));
   AssertTrue(samples[1].attributes.quality == SampleQuality::MISSING);
   AssertFalse(samples[1].attributes.bounded);
   AssertEquals(samples[1].attributes.methodId, methodId2);

   AssertTrue(samples[2].timestamp == t2);
   AssertTrue(samples[2].value == 10.0);
   AssertTrue(samples[2].attributes.quality == SampleQuality::ESTIMATED);
   AssertTrue(samples[2].attributes.bounded);
   AssertTrue(samples[2].attributes.lower == 9.5);
   AssertTrue(samples[2].attributes.upper == 11.25);
   AssertTrue(samples[2].attributes.completeness == 0.75);
   AssertEquals(samples[2].attributes.methodId, methodId);

   AssertTrue(samples[3].timestamp == t1);
   AssertTrue(samples[3].value == 10.5);
   AssertTrue(samples[3].attributes.isDefault());

   AssertEquals(GetAttributeRecordCount(dci->getId()), 3);

   // Time range is inclusive on both ends
   ReadSamples(*dci, t2, t3, 2, samples);
   AssertTrue(samples[0].timestamp == t3);
   AssertTrue(samples[1].timestamp == t2);
   EndTest();

   StartTest(L"Sample attributes - delete entry");
   AssertTrue(dci->deleteEntry(t2));
   ReadSamples(*dci, t1, t4, 3, samples);
   AssertEquals(GetAttributeRecordCount(dci->getId()), 2);
   EndTest();

   StartTest(L"Sample attributes - delete all data");
   AssertTrue(dci->deleteAllData());
   ReadSamples(*dci, t1, t4, 0, samples);
   AssertEquals(GetAttributeRecordCount(dci->getId()), 0);
   EndTest();

   owner->deleteObject();
}
