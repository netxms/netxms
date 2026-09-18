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
** File: delegate.cpp
**
** Tests for object and DCI membership lookups on delegate objects (network maps and dashboards)
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <nms_core.h>
#include <testtools.h>

/**
 * Number of dashboard rebuilds done by writer thread in concurrent lookup test
 */
#define REBUILD_COUNT   300

/**
 * Set by writer thread when all rebuilds are done
 */
static Condition s_rebuildsDone(true);

/**
 * Create dashboard update document with given number of elements referencing given object
 * and consecutive DCI IDs starting at given base
 */
static json_t *CreateDashboardUpdate(uint32_t objectId, uint32_t dciBase, int count)
{
   json_t *elements = json_array();
   for(int i = 0; i < count; i++)
   {
      json_t *data = json_object();
      json_object_set_new(data, "objectId", json_integer(objectId));
      json_object_set_new(data, "dciId", json_integer(dciBase + i));
      json_t *element = json_object();
      json_object_set_new(element, "type", json_integer(0));
      json_object_set_new(element, "data", data);
      json_array_append_new(elements, element);
   }
   json_t *document = json_object();
   json_object_set_new(document, "elements", elements);
   return document;
}

/**
 * Replace dashboard elements with given number of elements referencing given object and DCIs
 */
static uint32_t UpdateDashboard(Dashboard *dashboard, uint32_t objectId, uint32_t dciBase, int count)
{
   json_t *document = CreateDashboardUpdate(objectId, dciBase, count);
   uint32_t rcc = dashboard->NetObj::modifyFromJSON(document, nullptr);
   json_decref(document);
   return rcc;
}

/**
 * Writer thread for concurrent lookup test - rebuilds dashboard DCI set alternating between two
 * ranges of DCI IDs (last rebuild uses range starting at 2000)
 */
static void RebuildDashboard(Dashboard *dashboard, uint32_t objectId)
{
   for(int i = 0; i < REBUILD_COUNT; i++)
      UpdateDashboard(dashboard, objectId, (i % 2 == 0) ? 1000 : 2000, 200);
   s_rebuildsDone.set();
}

/**
 * Test object and DCI membership lookups on delegate objects
 */
void TestDelegateObjects()
{
   shared_ptr<Container> container = make_shared<Container>();
   container->setName(L"delegate-test-container");
   NetObjInsert(container, true, false);

   shared_ptr<Dashboard> dashboard = make_shared<Dashboard>(L"delegate-test-dashboard");
   NetObjInsert(dashboard, true, false);

   StartTest(L"Delegate object membership after dashboard update");
   AssertTrue(dashboard->isDelegate());
   AssertEquals(UpdateDashboard(dashboard.get(), container->getId(), 100, 3), RCC_SUCCESS);
   DelegateObject *delegate = dashboard->getAsDelegate();
   AssertTrue(delegate->containsObject(container));
   AssertTrue(delegate->containsDci(100));
   AssertTrue(delegate->containsDci(102));
   AssertFalse(delegate->containsDci(103));
   EndTest();

   StartTest(L"Delegate object lookups during concurrent dashboard updates");
   THREAD writer = ThreadCreateEx(RebuildDashboard, dashboard.get(), container->getId());
   uint32_t dciId = 1000;
   bool objectAlwaysFound = true;   // Every rebuild references the container; asserted after writer is joined
   while (!s_rebuildsDone.wait(0))
   {
      if (!delegate->containsObject(container))
         objectAlwaysFound = false;
      delegate->containsDci(dciId);
      dciId = (dciId < 2199) ? dciId + 1 : 1000;
   }
   ThreadJoin(writer);
   AssertTrue(objectAlwaysFound);
   AssertTrue(delegate->containsDci(2000));
   AssertTrue(delegate->containsDci(2199));
   AssertFalse(delegate->containsDci(1000));
   AssertFalse(delegate->containsDci(100));
   EndTest();
}
