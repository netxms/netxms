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
** File: recommission.cpp
**
** Tests for Node::recommission() and the RECOMMISSION console command.
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <nms_core.h>
#include <testtools.h>

#define TEST_NODE_NAME  _T("Recommission Test Node")

/**
 * Run console command and return its output
 */
static String RunConsoleCommand(const TCHAR *command)
{
   StringBufferConsole console;
   ProcessConsoleCommand(command, &console);
   return String(console.getOutput());
}

/**
 * Decommission node one day ahead without clearing IP addresses
 */
static void Decommission(const shared_ptr<Node>& node)
{
   node->decommission(time(nullptr) + 86400, false);
   AssertTrue(node->isDecommissioned());
   AssertEquals(node->getStatus(), STATUS_UNMANAGED);
}

/**
 * Test node recommission
 */
void TestRecommission()
{
   StartTest(_T("Node recommission"));

   NewNodeData data(InetAddress::parse(_T("10.255.255.21")));
   shared_ptr<Node> node = make_shared<Node>(&data, 0);
   node->setName(TEST_NODE_NAME);
   node->unpublish();   // Keep poll manager away from this node
   NetObjInsert(node, true, false);

   shared_ptr<Container> container = make_shared<Container>();
   container->setName(_T("Recommission Test Container"));
   container->unpublish();
   NetObjInsert(container, true, false);

   // Decommissioned node cannot be managed
   Decommission(node);
   AssertTrue(node->getDecommissionTime() != 0);
   AssertFalse(node->setMgmtStatus(true));
   AssertEquals(node->getStatus(), STATUS_UNMANAGED);

   // Recommission clears the state and leaves node unmanaged; manage works afterwards
   node->recommission();
   AssertFalse(node->isDecommissioned());
   AssertEquals(static_cast<int64_t>(node->getDecommissionTime()), static_cast<int64_t>(0));
   AssertEquals(node->getStatus(), STATUS_UNMANAGED);
   AssertTrue(node->setMgmtStatus(true));
   AssertEquals(node->getStatus(), STATUS_UNKNOWN);
   node->setMgmtStatus(false);

   // Recommission during maintenance must survive end of maintenance
   Decommission(node);
   node->enterMaintenanceMode(0, _T("test"));
   node->recommission();
   node->leaveMaintenanceMode(0);
   AssertFalse(node->isDecommissioned());

   EndTest();

   StartTest(_T("RECOMMISSION console command"));

   TCHAR command[256];
   _sntprintf(command, 256, _T("recommission %u"), node->getId());

   Decommission(node);
   AssertTrue(_tcsstr(RunConsoleCommand(command), _T("recommissioned; node remains unmanaged")) != nullptr);
   AssertFalse(node->isDecommissioned());
   AssertEquals(node->getStatus(), STATUS_UNMANAGED);

   AssertTrue(_tcsstr(RunConsoleCommand(command), _T("is not decommissioned")) != nullptr);

   AssertTrue(_tcsstr(RunConsoleCommand(_T("recommission 999999999")), _T("does not exist")) != nullptr);

   Decommission(node);
   AssertTrue(_tcsstr(RunConsoleCommand(_T("recommission ") TEST_NODE_NAME), _T("recommissioned; node remains unmanaged")) != nullptr);
   AssertFalse(node->isDecommissioned());

   _sntprintf(command, 256, _T("recommission %u"), container->getId());
   AssertTrue(_tcsstr(RunConsoleCommand(command), _T("is not a node")) != nullptr);

   AssertTrue(_tcsstr(RunConsoleCommand(_T("recommission")), _T("Invalid or missing")) != nullptr);

   container->deleteObject();
   node->deleteObject();

   EndTest();
}
